cmake_minimum_required(VERSION 3.1)

project(co_uring_http)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(co_uring_http STATIC)

target_link_libraries(co_uring_http PRIVATE asan)
target_link_libraries(co_uring_http PRIVATE ubsan)
target_link_libraries(co_uring_http PRIVATE uring)

target_compile_options(
  co_uring_http PRIVATE
  -g -Og -stdlib=libstdc++ -Wall -Werror
  -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined
)

target_include_directories(co_uring_http
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB SOURCES src/*.cpp)
target_sources(co_uring_http PRIVATE ${SOURCES})

file(GLOB BINARIES_SOURCES bin/*.cpp)
foreach(file_source ${BINARIES_SOURCES})
  get_filename_component(file_name ${file_source} NAME_WE)
  add_executable(${file_name} ${file_source})
  target_link_libraries(${file_name} PRIVATE co_uring_http)
endforeach()
