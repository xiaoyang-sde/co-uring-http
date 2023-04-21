cmake_minimum_required(VERSION 3.10)

project(co_uring_http)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(co_uring_http STATIC)
target_compile_options(co_uring_http PRIVATE -g -Og -stdlib=libstdc++ -Wall -Werror)

target_include_directories(co_uring_http
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB SOURCES "src/*.cpp")
target_sources(co_uring_http PRIVATE ${SOURCES})

file(GLOB EXAMPLES_SOURCES examples/*.cpp)
foreach(examples_src ${EXAMPLES_SOURCES})
  get_filename_component(examples_name ${examples_src} NAME_WE)
  add_executable(${examples_name} ${examples_src})
  target_link_libraries(${examples_name} PRIVATE co_uring_http)
endforeach()
