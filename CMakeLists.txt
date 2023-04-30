cmake_minimum_required(VERSION 3.10)

project(co_uring_http)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(IWYU_PROGRAM NAMES include-what-you-use)
if(IWYU_PROGRAM)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PROGRAM})
endif()

file(GLOB SOURCE_FILE src/*.cpp)
add_executable(co_uring_http ${SOURCE_FILE})

target_include_directories(co_uring_http PRIVATE include)
target_compile_options(co_uring_http PRIVATE -Wall -Wextra)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  target_link_libraries(co_uring_http PRIVATE asan ubsan uring)
  target_compile_options(co_uring_http PRIVATE -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)
else()
  target_link_libraries(co_uring_http PRIVATE uring)
endif()
