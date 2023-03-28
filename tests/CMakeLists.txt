include(FetchContent)

FetchContent_Declare(Catch2
  SYSTEM
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        97c48e0c343d26d50764fafdc90b1e630fbd10ce) # v3.1.0
FetchContent_MakeAvailable(Catch2)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE
  fschuetz04::simcpp20
  Catch2::Catch2WithMain
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(tests PRIVATE "$<$<CONFIG:DEBUG>:${my_debug_options}>")
  target_compile_options(tests PRIVATE "$<$<CONFIG:DEBUG>:${debug_clang_options}>"
    "$<$<NOT:$<CONFIG:Debug>>:${release_clang_options}>")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(tests PRIVATE "$<$<CONFIG:DEBUG>:${my_debug_options}>")
  target_compile_options(tests PRIVATE "$<$<CONFIG:DEBUG>:${debug_gcc_options}>"
    "$<$<NOT:$<CONFIG:Debug>>:${release_gcc_options}>")
else()
  message(WARNING "Your compiler, ${CMAKE_CXX_COMPILER_ID}, is untested with Grazer but maybe still works. Tested compilers are gcc and clang.")
endif()


# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#   target_compile_options(tests PRIVATE -Wall -Wextra)
# endif()
