// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#pragma once

#include <cassert>    // assert
#include <cmath>      // std::log2
#include <coroutine>  // std::coroutine_handle, std::suspend_never
#include <functional> // std::function
#include <optional>   // std::optional, std::nullopt
#include <vector>     // std::vector

namespace simcpp20 {
template <typename Time> class simulation;

/**
 * Can be awaited by processes.
 *
 * @tparam Time Type used for simulation time.
 */
template <typename Time = double> class event {
public:
  class promise_type;

  /**
   * Construct a new pending event.
   *
   * @param simulation Reference to simulation.
   */
  explicit event(simulation<Time> &sim) : data_{new data(sim)} {
    data_->use_count += 1;
  }

  /// Destruct this event.
  ~event() { decrement_use_count(); }

  /**
   * Copy construct an event.
   *
   * @param other Event to copy.
   */
  event(const event &other) : data_{other.data_} { data_->use_count += 1; }

  /**
   * Move construct an event.
   *
   * @param other Event to move.
   */
  event(event &&other) noexcept : data_{std::exchange(other.data_, nullptr)} {}

  /**
   * Replace this event by copying.
   *
   * @param other Event to replace this event with.
   * @return Reference to this event.
   */
  event &operator=(const event &other) {
    decrement_use_count();
    data_ = other.data_;
    data_->use_count += 1;
    return *this;
  }

  /**
   * Replace this event by moving.
   *
   * @param other Event to replace this event with.
   * @return Reference to this event.
   */
  event &operator=(event &&other) noexcept {
    decrement_use_count();
    data_ = std::exchange(other.data_, nullptr);
    return *this;
  }

  /**
   * Set the event state to triggered and schedule it to be processed
   * immediately.
   *
   * If the event is not pending, nothing is done.
   *
   * TODO(fschuetz04): Check whether used on a process?
   */
  void trigger() const {
    if (!pending()) {
      return;
    }

    data_->sim.schedule(*this);
    data_->state_ = state::triggered;
  }

  /**
   * Set the event state to aborted and destroy all coroutines waiting for it.
   *
   * If the event is not pending, nothing is done.
   *
   * TODO(fschuetz04): Check whether used on a process? Destroy process
   * coroutine?
   */
  void abort() const {
    if (!pending()) {
      return;
    }

    data_->state_ = state::aborted;

    for (auto &handle : data_->handles) {
      handle.destroy();
    }
    data_->handles.clear();

    data_->cbs.clear();
  }

  /**
   * Add a callback to the event to be called when the event is processed.
   *
   * @param cb Callback to add to the event.
   */
  void add_callback(std::function<void(const event<Time> &)> cb) const {
    if (processed() || aborted()) {
      return;
    }

    data_->cbs.emplace_back(cb);
  }

  /// @return Whether the event is pending.
  bool pending() const { return data_->state_ == state::pending; }

  /// @return Whether the event is triggered or processed.
  bool triggered() const {
    return data_->state_ == state::triggered || processed();
  }

  /// @return Whether the event is processed.
  bool processed() const { return data_->state_ == state::processed; }

  /// @return Whether the event is aborted.
  bool aborted() const { return data_->state_ == state::aborted; }

  /**
   * @return Whether the event is already processed and a waiting coroutine must
   * not be paused.
   */
  bool await_ready() const { return processed(); }

  /**
   * Suspend a process and resume it when the event is processed.
   *
   * @param handle Corotuine handle of the process.
   */
  void await_suspend(std::coroutine_handle<promise_type> handle) {
    if (aborted()) {
      handle.destroy();
      return;
    }

    data_->handles.push_back(handle);

    decrement_use_count();
    handle_ = handle;
  }

  /// Resume a process.
  void await_resume() {
    data_->use_count += 1;
    auto handle = std::exchange(handle_, std::nullopt);

    if (handle.value().promise().ev.aborted()) {
      throw nullptr;
    }
  }

  /**
   * Alias for sim.any_of. Create a pending event which is triggered when this
   * event or the given event is processed.
   *
   * @param other Given event.
   * @return Created event.
   */
  event<Time> operator|(const event<Time> &other) const {
    return data_->sim.any_of({*this, other});
  }

  /**
   * Alias for sim.all_of. Create a pending event which is triggered when this
   * event and the given event is processed.
   *
   * @param other Given event.
   * @return Created event.
   */
  event<Time> operator&(const event<Time> &other) const {
    return data_->sim.all_of({*this, other});
  }

  bool operator==(const event<Time> &other) const {
    return data_ == other.data_;
  }

  /// Promise type for process coroutines.
  class promise_type {
  public:
    /**
     * Construct a new promise type instance.
     *
     * @tparam Args Additional arguments passed to the process function. These
     * arguments are ignored.
     * @param sim Reference to the simulation.
     */
    template <typename... Args>
    explicit promise_type(simulation<Time> &sim, Args &&...)
        : sim{sim}, ev{sim} {}

    /**
     * Construct a new promise type instance.
     *
     * @tparam Class Class instance if the process function is a lambda or a
     * member function of a class. This argument is ignored.
     * @tparam Args Additional arguments passed to the process function. These
     * arguments are ignored.
     * @param sim Reference to the simulation.
     */
    template <typename Class, typename... Args>
    explicit promise_type(Class &&, simulation<Time> &sim, Args &&...)
        : sim{sim}, ev{sim} {}

    /**
     * Construct a new promise type instance.
     *
     * @tparam Class Class instance if the process function is a lambda or a
     * member function of a class. Must contain a member variable sim
     * referencing the simulation.
     * @tparam Args Additional arguments passed to the process function. These
     * arguments are ignored.
     * @param c Class instance.
     */
    template <typename Class, typename... Args>
    explicit promise_type(Class &&c, Args &&...) : sim{c.sim}, ev{c.sim} {}

#ifdef __INTELLISENSE__
    /**
     * Fix IntelliSense complaining about missing default constructor for the
     * promise type.
     */
    promise_type();
#endif

    /// @return Event which will be triggered when the process finishes.
    event<Time> get_return_object() const { return ev; }

    /**
     * Register the process to be started immediately via an initial event.
     *
     * @return Initial event.
     */
    event<Time> initial_suspend() const { return sim.timeout(Time{0}); }

    /// @return Awaitable which is always ready.
    std::suspend_never final_suspend() const noexcept { return {}; }

    /// No-op.
    void unhandled_exception() const { assert(false); }

    /// Trigger the underlying event since the process finished.
    void return_void() const { ev.trigger(); }

    /// Refernece to the simulation.
    simulation<Time> &sim;

    /// Underlying event which is triggered when the process finishes.
    const event<Time> ev;
  };

private:
  /**
   * Process the event.
   *
   * Set the event state to processed. Call all callbacks for this event.
   * Resume all coroutines waiting for this event.
   */
  void process() const {
    if (processed() || aborted()) {
      return;
    }

    data_->state_ = state::processed;

    for (auto &handle : data_->handles) {
      handle.resume();
    }
    data_->handles.clear();

    for (auto &cb : data_->cbs) {
      cb(*this);
    }
    data_->cbs.clear();
  }

  /// Decrement the use count and free the shared state if it reaches 0.
  void decrement_use_count() {
    if (handle_.has_value() || data_ == nullptr) {
      return;
    }

    data_->use_count -= 1;
    if (data_->use_count == 0) {
      delete std::exchange(data_, nullptr);
    }
  }

  /// State of an event.
  enum class state {
    /**
     * Event is not yet triggered or aborted. This is the initial state of a new
     * event.
     */
    pending,

    /**
     * Event is triggered and will be processed at the current simulation time.
     */
    triggered,

    /// Event is processed.
    processed,

    /// Event is aborted.
    aborted
  };

  /// Shared data of the event.
  class data {
  public:
    /// Construct a new shared data instance.
    explicit data(simulation<Time> &sim) : sim{sim} {}

    /// Destroy all coroutines still waiting for the event.
    ~data() {
      for (auto &handle : handles) {
        handle.destroy();
      }
    }

    /// Use count of the event.
    int use_count = 0;

    /// State of the event.
    state state_ = state::pending;

    /// Coroutine handles of processes waiting for this event.
    std::vector<std::coroutine_handle<promise_type>> handles{};

    /// Callbacks for this event.
    std::vector<std::function<void(const event<Time> &)>> cbs{};

    /// Reference to the simulation.
    simulation<Time> &sim;
  };

  /// Shared data of the event.
  data *data_;

  /// Handle of the coroutine waiting for this event, if any.
  std::optional<std::coroutine_handle<promise_type>> handle_ = std::nullopt;

  /// The simulation needs access to process.
  friend class simulation<Time>;

  /// std::hash needs access to data_.
  friend class std::hash<event<Time>>;
};
} // namespace simcpp20

namespace std {
template <typename Time> struct hash<simcpp20::event<Time>> {
  size_t operator()(const simcpp20::event<Time> &ev) const {
    static const size_t shift = std::log2(1 + sizeof(simcpp20::event<Time>));
    return reinterpret_cast<size_t>(ev.data_) >> shift;
  }
};
} // namespace std
