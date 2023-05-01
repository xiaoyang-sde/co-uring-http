# `co-uring-http`

`co-uring-http` is a high-performance HTTP server built on C++20 coroutines and `io_uring`. This project serves as an exploration of the latest Linux kernel and C++ standard features and is not recommended for production use. For learning purposes, `co-uring-http` re-implements general-purpose coroutine primitives (such as `task<T>`, `sync_wait<task<T>>`, etc.) instead of utilizing existing libraries. This document is a draft, and a more detailed design document will be provided later.

The high-level design of `co-uring-http` consists of a thread pool with a number of threads equal to or less than the number of logical processors. Each thread includes a socket bound to the same port using the `SO_REUSEPORT` option and spawns a coroutine to accept clients while running an event loop to process events in the `io_uring` completion queue. When a client arrives, the `handle_client()` coroutine is spawned to handle HTTP requests for that client. As the coroutine issues async operations, it is suspended and transfers control back to the event loop, which then resumes the coroutine when the async operations are complete.

- Leverages C++20 coroutines to manage clients and handle HTTP requests, which simplifies the mental overhead of writing asynchronous code.
- Leverages `io_uring` for handling async I/O operations, such as `accept()`, `send()`, `recv()`, and `splice()`, reducing the number of system calls.
- Leverages ring-mapped buffers to minimize buffer allocation costs and reduce data transfer between user and kernel space.
- Leverages multishot accept in `io_uring` to decrease the overhead of issuing `accept()` requests.
- Implements a thread pool to utilize all logical processors for optimal hardware parallelism.
- Manages the lifetime of `io_uring`, file descriptors, and the thread pool using RAII classes.

## Performance

The benchmark is performed with the [`hey` benchmark tool](https://github.com/rakyll/hey), which sends 100 batches of requests, with each batch containing 10,000 concurrent clients requesting a file of 1024 bytes in size. `co-uring-http` serves 88,160 requests per second and handles 99% of requests within 0.5 seconds.

The benchmark is performed on WSL with Linux kernel version `6.3.0-microsoft-standard-WSL2`, which is based on Intel i5-12400 with 12 logical processors, 16 GB memories, and Samsung PM9A1 256 GB (OEM version of Samsung 980 Pro).

```console
./hey -n 1000000 -c 10000 http://127.0.0.1:8080/1k

Summary:
  Total:        11.3429 secs
  Slowest:      1.2630 secs
  Fastest:      0.0000 secs
  Average:      0.0976 secs
  Requests/sec: 88160.9738

  Total data:   1024000000 bytes
  Size/request: 1024 bytes

Response time histogram:
  0.000 [1]     |
  0.126 [701093]        |■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  0.253 [259407]        |■■■■■■■■■■■■■■■
  0.379 [24843] |■
  0.505 [4652]  |
  0.632 [678]   |
  0.758 [1933]  |
  0.884 [1715]  |
  1.010 [489]   |
  1.137 [5111]  |
  1.263 [78]    |
```

## Requirement

- Linux Kernel 6.3
- CMake 3.10
- Clang 14
- libstdc++ 11.3 (could be installed with GCC 11.3)
- [liburing](https://github.com/axboe/liburing) 2.3

`co-uring-http` uses Clang to take advantage of the LLVM toolchain (`clangd`, `lldb`, `clang-format`, etc.). However, since `libc++` lacks support for certain C++20 features such as `jthread`, the project links with GCC's `libstdc++`. The latest Linux kernel is required because the project leverages latest features of `io_uring`.

The `.devcontainer/Dockerfile` provides a container image based on `ubuntu:lunar` with these dependencies installed. Please note that the Linux virtual machine of Docker on macOS is based on Linx kernel 5.15, so a Docker engine with native or WSL backend is recommended.

## Build

- Generate build configuration with CMake:

```console
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -B build -G "Unix Makefiles"
```

- Build and run `co_uring_http`, which will listen on `localhost:8080` and serves static files in the working directory:

```console
make -j$(nproc)
./co_uring_http
```

## Update WSL Kernel

The default Linux kernel version for the Windows Subsystem for Linux (WSL) is 5.15, which does not support certain `io_uring` features, such as multi-shot accept or ring-mapped buffers. However, WSL provides a [`.wslconfig`](https://learn.microsoft.com/en-us/windows/wsl/wsl-config) file that enables the use of a custom-built kernel image. It's recommended to build the kernel within an existing WSL instance.

- Install the build dependencies:

```console
sudo apt install git bc build-essential flex bison libssl-dev libelf-dev dwarves
```

- Download the latest kernel source code:

```console
wget https://github.com/torvalds/linux/archive/refs/tags/v6.3.tar.gz -O v6.3.tar.gz
tar -xf v6.3.tar.gz
```

- Download the build configuration file for WSL:

```console
wget https://raw.githubusercontent.com/microsoft/WSL2-Linux-Kernel/linux-msft-wsl-6.1.y/Microsoft/config-wsl
cp config-wsl linux-6.3/arch/x86/configs/wsl_defconfig
```

- Build the kernel:

```console
cd linux-6.3
make KCONFIG_CONFIG=arch/x86/configs/wsl_defconfig -j$(nproc)
```

- Clone the kernel image to `$env:USERPROFILE` (default user path) and set `.wslconfig` to use the kernel image:

```console
powershell.exe /C 'Copy-Item .\arch\x86\boot\bzImage $env:USERPROFILE'
powershell.exe /C 'Write-Output [wsl2]`nkernel=$env:USERPROFILE\bzImage | % {$_.replace("\","\\")} | Out-File $env:USERPROFILE\.wslconfig -encoding ASCII'
```

- Restart WSL:

```console
wsl --shutdown
```
