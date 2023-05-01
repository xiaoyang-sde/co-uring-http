# `co-uring-http`

`co-uring-http` is a high-performance HTTP server built on C++ 20 coroutines and `io_uring`. This project serves as an exploration of the latest features of Linux kernel and is not recommended for production use. For learning purposes, `co-uring-http` re-implements general-purpose coroutine primitives (such as `task<T>`, `sync_wait<task<T>>`, etc.) instead of utilizing existing libraries.

- Leverages C++ 20 coroutines to manage clients and handle HTTP requests, which simplifies the mental overhead of writing asynchronous code.
- Leverages `io_uring` for handling async I/O operations, such as `accept()`, `send()`, `recv()`, and `splice()`, reducing the number of system calls.
- Leverages ring-mapped buffers to minimize buffer allocation costs and reduce data transfer between user and kernel space.
- Leverages multishot accept in `io_uring` to decrease the overhead of issuing `accept()` requests.
- Implements a thread pool to utilize all logical processors for optimal hardware parallelism.
- Manages the lifetime of `io_uring`, file descriptors, and the thread pool using RAII classes.

## Design Document

### Component

- Coroutine
  - `task` (`task.hpp`): The `task` class represents a coroutine that does not start until it is awaited.
  - `thread_pool` (`thread_pool.hpp`): The `thread_pool` class provides an abstraction that allows a coroutine to be scheduled on fixed-size pool of threads. The number of threads is limited to the number of logical processors, allowing for hardware parallelism.

- File Descriptor
  - `file_descriptor` (`file_descriptor.hpp`): The `file_descriptor` class owns a file descriptor. The `file_descriptor.hpp` file provides general-purpose functions that works with the `file_descriptor` class, such as `open()`, `pipe()`, and `splice()`.
  - `server_socket` (`socket.hpp`): The `server_socket` class extends the `file_descriptor` class and represents the listening socket that could accept clients. It provides an `accept()` method, which records if there is an existing multishot `accept` request in `io_uring` and submits a new one if none exists.
  - `client_socket` (`socket.hpp`): The `client_socket` class extends the `file_descriptor` class and represents the socket that could communicate with the client. It provides a `send()` method, which submits a `send` request to `io_uring` and a `recv()` method, which submits a `recv` request to `io_uring`.

- `io_uring`
  - `io_uring` (`io_uring.hpp`): The `io_uring` class is a `thread_local` singleton, which owns the submission queue and completion queue of `io_uring`.
  - `buffer_ring` (`buffer_ring.hpp`): The `buffer_ring` class is a `thread_local` singleton, manages a collection of buffers that are supplied to `io_uring`. When an incoming HTTP request arrives, `io_uring` selects a buffer from the available pool and populates it with the received data, eliminating the need for allocating a new buffer for each request. Once the `http_parser` finishes parsing the request, the server returns the buffer to `io_uring`, enabling its reuse for future requests. The number of buffers and and the size of each buffer are defined in `constant.hpp`, which can be adjusted based on the estimated workload of the HTTP server.

- HTTP Server
  - `http_server` (`http_server.hpp`): The `http_server` class creates a `thread_worker` task for each thread in the `thread_pool` and waits for these tasks to finish. The tasks will finish when an exception occurs.
  - `thread_worker` (`http_server.hpp`): The `thread_worker` class contains a collection of coroutines that will be scheduled on each thread. When the class is constructed, it spawns `thread_worker::accept_client()` and `thread_worker::event_loop()`.
    - The `thread_worker::event_loop()` coroutine processes events in the completion queue of `io_uring` and resumes the coroutine that is waiting for the completion of that event.
    - The `thread_worker::accept_client()` coroutine invokes `server_socket::accept()` to submit a multishot `accept()` request to `io_uring`, which will generate a completion queue event for each incoming client. When a client arrives, it spawns the `thread_worker::handle_client()` coroutine.
    - The `thread_worker::handle_client()` coroutine invokes `client_socket::recv()` to wait for an HTTP request. When a request arrives, it parses the request with `http_parser` (`http_parser.hpp`), constructs an `http_response` (`http_message.hpp`), and sends the response with `client_socket::send()`.

```cpp
auto thread_worker::handle_client(client_socket client_socket) -> task<> {
  http_parser http_parser;
  buffer_ring &buffer_ring = buffer_ring::get_instance();
  while (true) {
    const auto [recv_buffer_id, recv_buffer_size] = co_await client_socket.recv(BUFFER_SIZE);
    const std::span<char> recv_buffer = buffer_ring.borrow_buffer(recv_buffer_id, recv_buffer_size);

    if (const auto parse_result = http_parser.parse_packet(recv_buffer); parse_result.has_value()) {
      const http_request &http_request = parse_result.value();

      // Processes the `http_request` and constructs an `http_response`
      // with a status that is either `200` or `404`
      // (Please refer to the source code)

      std::string send_buffer = http_response.serialize();
      co_await client_socket.send(send_buffer, send_buffer.size());
    }

    buffer_ring.return_buffer(recv_buffer_id);
  }
}
```

### Workflow

- The `http_server` creates a `thread_worker` task for each thread in the `thread_pool` and awaits their completion.
- Each `thread_worker` creates a socket with the `SO_REUSEPORT` option, allowing the reuse of the same port, and spawns the `thread_worker::accept_client()` and `thread_worker::event_loop()` coroutines.
- Upon a client arrival, the `thread_worker::accept_client()` coroutine spawns a `thread_worker::handle_client()` coroutine to handle HTTP requests for that client.
- When either `thread_worker::accept_client()` or `thread_worker::handle_client()` awaits an asynchronous I/O operation (such as `send()` or `recv()`), it suspends its execution and submits a request to the submission queue of `io_uring`. Execution control is then transferred back to `thread_worker::event_loop()`.
- The `thread_worker::event_loop()` processes events in the completion queue of `io_uring`. For each event, it identifies the coroutine that is awaiting that event and resumes its execution.

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

`co-uring-http` uses Clang to take advantage of the LLVM toolchain (`clangd`, `lldb`, `clang-format`, etc.). However, since `libc++` lacks support for certain C++ 20 features such as `jthread`, the project links with GCC's `libstdc++`. The latest Linux kernel is required because the project leverages latest features of `io_uring`.

The `.devcontainer/Dockerfile` provides a container image based on `ubuntu:lunar` with these dependencies installed. Please note that the Linux virtual machine of Docker on macOS is based on Linux kernel 5.15, which doesn't meet the requirement of `co-uring-http`.

## Build

- Generate build configuration with CMake:

```console
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -B build -G "Unix Makefiles"
```

- Build and run `co_uring_http`, which will listen on `localhost:8080` and serves static files:

```console
make -C build -j$(nproc)
./build/co_uring_http
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
