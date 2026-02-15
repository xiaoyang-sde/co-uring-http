# `co-uring-http`

`co-uring-http` is a high-performance HTTP server built with `io_uring` and C++20 coroutines. This project explores the latest advancements in C++ and the Linux kernel, making it an experimental endeavor, so it's not recommended for production use.

`co-uring-http` implements core server logic using coroutines for listening for incoming connections, receiving and parsing HTTP requests, and sending HTTP responses. Each coroutine that performs an I/O operation suspends, submits the request to `io_uring`, and returns control to an event loop. When the I/O operation completes, the event loop resumes the corresponding coroutine. To optimize `io_uring` performance, `co-uring-http` uses ring-mapped buffers to minimize heap allocations and submits multishot accept requests to reduce event creation overhead.

The project showcases modern C++ features from C++20 and C++23. (Most non-trivial C++26 features are still under development, so there's limited scope for exploration.) The codebase is built on C++ modules, including the `std` module. It uses `std::expected` for error handling instead of exceptions and adopts `std::source_location` for improved error diagnostics. The HTTP parser and serializer are built with `std::ranges` and `std::format`. The codebase makes extensive use of constraints and concepts in templates, such as general-purpose coroutine primitives like `task` and `wait`.

## Requirement

- Linux Kernel 6.10
- Clang 18 with `libc++`
  - GCC 14 with `libstdc++` doesn't support crucial C++23 features like [P1659R3](https://wg21.link/P1659R3), [P2286R8](https://wg21.link/P2286R8), and [P2465R3](https://wg21.link/P2465R3).
- CMake 3.30 with Ninja 1.12
- `liburing` 2.6

## Build

Generate the build configuration with CMake:

```console
cmake -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_CXX_COMPILER:FILEPATH=$(which clang++) -B build -G Ninja
```
> Optional: Due to [known issues](https://github.com/llvm/llvm-project/issues/172497#issue-3735193825),You may need to force the path to be specified depending on your compilation environment. Use `-DCMAKE_CXX_STDLIB_MODULES_JSON`


Build and run, which will listen on `localhost:8080` and serves static files:

```console
ninja -C build
./build/main
```

## Performance

The benchmark was conducted using the [`hey` benchmark tool](https://github.com/rakyll/hey), which sends 200 batches of requests, each batch containing 5,000 concurrent clients requesting a 1024-byte file. `co-uring-http` serves 81,816 requests per second and handles 95% of the requests within 0.1 seconds.

The benchmark was conducted on an [OrbStack](https://orbstack.dev) virtual machine running on MacBook Air (M1, 2020) with Linux kernel version `6.9.8`. The virtual machine has 8 cores and 8 GB of RAM. The executable is compiled with Clang 18 with optimization level `O3`.

```console
./hey -n 1000000 -c 5000 http://127.0.0.1:8080/1k

Summary:
  Total:        12.2225 secs
  Slowest:      0.3702 secs
  Fastest:      0.0000 secs
  Average:      0.0530 secs
  Requests/sec: 81816.5124

  Total data:   1024000000 bytes
  Size/request: 1024 bytes

Response time histogram:
  0.000 [1]     |
  0.037 [2054]  |
  0.074 [911261]        |■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.111 [59536] |■■■
  0.148 [746]   |
  0.185 [0]     |
  0.222 [14526] |■
  0.259 [6611]  |
  0.296 [3536]  |
  0.333 [1671]  |
  0.370 [58]    |

Latency distribution:
  10% in 0.0412 secs
  25% in 0.0421 secs
  50% in 0.0438 secs
  75% in 0.0490 secs
  90% in 0.0715 secs
  95% in 0.0811 secs
  99% in 0.2271 secs

Details (average, fastest, slowest):
  DNS+dialup:   0.0003 secs, 0.0000 secs, 0.0999 secs
  DNS-lookup:   0.0000 secs, 0.0000 secs, 0.0000 secs
  req write:    0.0001 secs, 0.0000 secs, 0.0415 secs
  resp wait:    0.0033 secs, 0.0000 secs, 0.0615 secs
  resp read:    0.0487 secs, 0.0000 secs, 0.2822 secs

Status code distribution:
  [200] 1000000 responses
```
