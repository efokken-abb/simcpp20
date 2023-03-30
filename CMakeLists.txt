cmake_minimum_required(VERSION 3.25)

if(DEFINED PROJECT_NAME)
  set(MAIN_PROJECT OFF)
else()
  set(MAIN_PROJECT ON)
endif()

project(simcpp20
  VERSION      0.1.0
  DESCRIPTION  "Discrete-event simulation in C++20 using coroutines"
  HOMEPAGE_URL "https://github.com/fschuetz04/simcpp20"
  LANGUAGES    CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)








if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("-stdlib=libc++")
  add_link_options("-stdlib=libc++")
endif()



# for sanitizing in debug mode:
set(MY_SANITIZE_OPTIONS -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined)
add_compile_options("$<$<CONFIG:DEBUG>:${MY_SANITIZE_OPTIONS}>")
add_link_options("$<$<CONFIG:DEBUG>:${MY_SANITIZE_OPTIONS}>")






add_subdirectory(include/fschuetz04/simcpp20)

add_compile_definitions("$<$<CONFIG:DEBUG>:_GLIBCXX_DEBUG>")

set(my_debug_options -O0 -Werror -pedantic-errors -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wundef
  -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-overflow=5 -Wunused -Wno-error=unused-parameter -Wno-error=unused -Wno-error=unused-variable -Wno-error=unused-value -Wdelete-non-virtual-dtor -Wno-error=undef)
set(debug_clang_options "-Wunreachable-code;-fcolor-diagnostics;-Wno-error=unused-private-field;-Wno-error=unknown-warning-option")

set(debug_gcc_options "-Wsuggest-override;-fdiagnostics-color;-Wno-error=strict-overflow;-Wno-error=unused-but-set-variable")

set(release_clang_options "")
set(release_gcc_options "")


option(FSCHUETZ04_SIMCPP20_BUILD_TESTS "Build tests" ${MAIN_PROJECT})
if(FSCHUETZ04_SIMCPP20_BUILD_TESTS)
  add_subdirectory(tests)
endif()

option(FSCHUETZ04_SIMCPP20_BUILD_EXAMPLES "Build examples" ${MAIN_PROJECT})
if(FSCHUETZ04_SIMCPP20_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
