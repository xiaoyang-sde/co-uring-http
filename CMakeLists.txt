cmake_minimum_required(VERSION 3.10)

project(co_uring_http)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(
  CMAKE_CXX_CLANG_TIDY
  clang-tidy;-header-filter=.
)

file(GLOB SOURCE_FILE src/*.cpp)
add_executable(co_uring_http ${SOURCE_FILE})

target_include_directories(co_uring_http PRIVATE include)
target_compile_options(co_uring_http PRIVATE -stdlib=libstdc++ -Wall -Wextra)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  target_link_libraries(co_uring_http PRIVATE asan ubsan uring)
  target_compile_options(
    co_uring_http PRIVATE
    -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined
  )
else()
  target_link_libraries(co_uring_http PRIVATE uring)
endif()
