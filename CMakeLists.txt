cmake_minimum_required(VERSION 3.30)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.2)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444") 
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.2)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0.0)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30)
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
endif()
set(CMAKE_CXX_MODULE_STD TRUE)
project(co_uring_http LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_library(${CMAKE_PROJECT_NAME})
target_sources(
  ${CMAKE_PROJECT_NAME}
  PUBLIC
  FILE_SET CXX_MODULES FILES
  src/common/deferred_initialization.cppm
  src/common/lib.cppm
  src/coroutine/lib.cppm
  src/coroutine/spawn.cppm
  src/coroutine/task.cppm
  src/coroutine/thread_pool.cppm
  src/coroutine/trait.cppm
  src/coroutine/wait.cppm
  src/http/lib.cppm
  src/http/message.cppm
  src/http/server.cppm
  src/io/buf_ring.cppm
  src/io/constant.cppm
  src/io/error.cppm
  src/io/file.cppm
  src/io/io_uring.cppm
  src/io/lib.cppm
  src/io/socket.cppm
  src/lib.cppm
)
target_compile_features(
  ${CMAKE_PROJECT_NAME}
  PRIVATE cxx_std_23
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE -pedantic
  PRIVATE -Wall
  PRIVATE -Wextra
)
target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE __cpp_concepts=202002L
)

add_executable(main)
target_sources(
  main
  PRIVATE src/main.cpp
)
target_link_libraries(
  main
  PRIVATE ${CMAKE_PROJECT_NAME}
  PRIVATE uring
)
target_compile_features(
  main
  PRIVATE cxx_std_23
)
target_compile_options(
  main
  PRIVATE -pedantic
  PRIVATE -Wall
  PRIVATE -Wextra
)
target_compile_definitions(
  main
  PRIVATE __cpp_concepts=202002L
)
