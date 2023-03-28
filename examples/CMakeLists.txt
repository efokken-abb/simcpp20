include(FetchContent)

OPTION(BUILD_TESTS OFF)
FetchContent_Declare(units
  SYSTEM
  GIT_REPOSITORY https://github.com/nholthaus/units
  GIT_TAG        1e2c7ed7cd8e61db7a82b8827caef7ed49b01831)
FetchContent_MakeAvailable(units)

set(TARGETS
  all_of
  any_of
  bank_renege
  carwash
  clocks
  clocks_units
  machine_shop
  ping_pong
  value_event
  value_process)





foreach(TARGET ${TARGETS})
  add_executable(${TARGET} ${TARGET}.cpp)
  target_link_libraries(${TARGET} PRIVATE fschuetz04::simcpp20)


  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:DEBUG>:${my_debug_options}>")
    target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:DEBUG>:${debug_clang_options}>"
      "$<$<NOT:$<CONFIG:Debug>>:${release_clang_options}>")
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:DEBUG>:${my_debug_options}>")
    target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:DEBUG>:${debug_gcc_options}>"
      "$<$<NOT:$<CONFIG:Debug>>:${release_gcc_options}>")
  else()
    message(WARNING "Your compiler, ${CMAKE_CXX_COMPILER_ID}, is untested with Grazer but maybe still works. Tested compilers are gcc and clang.")
  endif()

endforeach()


target_link_libraries(clocks_units PRIVATE units)
