# High-Performance Multithreaded Web Server in C++

![Language](https://img.shields.io/badge/language-C++-00599C?style=flat-square)
![Threads](https://img.shields.io/badge/concurrency-pthreads-orange?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-green?style=flat-square)

A raw, lightweight, and high-throughput HTTP server built from scratch using **C++** and **POSIX Threads (pthreads)**. Designed to handle high concurrency with minimal latency, achieving **48,000+ requests per second** on local benchmarks.

Includes a **Real-time Dashboard** to monitor active threads, memory usage, and uptime.

---

## Visual Demo

**Real-time Server Dashboard**
*Below is the live dashboard visualizing thread activity and memory consumption during a load test.*

![Dashboard Screenshot](https://github.com/gaurav337/ServerCPP/blob/main/WebServer/ss1.png?raw=true)
![Dashboard Screenshot](https://github.com/gaurav337/ServerCPP/blob/main/WebServer/ss.png?raw=true)

---

## Performance Benchmark

Tested using **ApacheBench (ab)** on a local machine (Concurrency: 50, Requests: 20,000).

| Metric | Result |
| :--- | :--- |
| **Requests per Second** | **48,406.69 [#/sec]** |
| **Time per Request** | **0.021 ms** (mean) |
| **Transfer Rate** | **223 MB/sec** |
| **Latency (99%)** | **< 3ms** |

> **Note:** This implementation significantly outperforms standard single-threaded implementations (e.g., basic Python Flask dev server) by reducing interpretation and middleware overhead.

---

## Technical Architecture

This server achieves superior performance by bypassing the heavy middleware layers found in standard frameworks and interacting directly with the OS kernel.

### 1. Direct Socket Interface
The server utilizes raw Linux system calls (`socket`, `bind`, `listen`, `accept`) rather than relying on heavy wrapper libraries. This ensures **zero middleware overhead**, allowing requests to flow directly from the network stack to the logic handler without unnecessary abstraction layers.

### 2. Custom Thread Pool (Producer-Consumer)
Dynamic thread creation for every incoming request is computationally expensive (context switching).
* **Solution:** The server implements a **Producer-Consumer Thread Pool** that pre-allocates a fixed number of worker threads (e.g., 100) at startup.
* **Benefit:** Eliminates the overhead of creating/destroying threads per request and stabilizes CPU usage under load.

### 3. In-Memory Caching (Zero-Copy)
* **Strategy:** Static assets (like `index.html`) are loaded directly into **RAM** (`std::string`) at startup.
* **Benefit:** Serving a request involves **zero disk I/O**, resulting in consistent sub-millisecond response times.

---

## Scalability & Design Decisions

### Current Approach: Heap Optimization
The server currently utilizes **In-Memory Caching** by loading files into the Heap. This strategy is explicitly optimized for speed when serving small static assets (dashboards, config files), ensuring maximum throughput.

### Future Scalability: Large File Support
While the current approach is faster, loading gigabyte-sized files into RAM would exhaust heap memory. To scale this for a production-grade file server hosting large media, the architecture will be updated to use **Memory Mapping (`mmap`)** or **Chunked Streaming**. This allows the OS to handle paging efficiently without bloating process memory.

---

## Real-Time Dashboard Features

The server hosts a lightweight HTML/JS dashboard at `http://localhost:8080/`.

* **Live Metrics:** Updates every 500ms via AJAX.
* **Stats Monitored:** Total Requests, Active Threads, Memory Usage (KB), and Server Uptime.
* **Integrated Benchmark:** Ability to trigger server-side stress tests directly from the UI.

---

## Getting Started

### Prerequisites
* Linux/MacOS (or WSL on Windows)
* `g++` (Compiler)
* `make` (Build system)
* `apache2-utils` (Optional: For running benchmarks manually)

### Installation & Run

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/gaurav337/ServerCPP.git](https://github.com/gaurav337/ServerCPP.git)
    cd ServerCPP
    ```

2.  **Build the project:**
    ```bash
    make
    ```

3.  **Run the server:**
    ```bash
    ./server
    ```
    *Server will start on port 8080 with 100 worker threads.*

4.  **View the Dashboard:**
    Navigate to `http://localhost:8080` in your web browser.

---

## Project Structure

```text
├── main.cpp          # Entry point
├── server.cpp        # Server logic (Socket handling, Routing)
├── server.h          # Server class definition
├── ThreadPool.h      # Custom Thread Pool implementation
├── Makefile          # Build script
└── public/
    └── index.html    # Dashboard Frontend

```

---

## Roadmap

* [ ] **Asynchronous I/O:** Implement `epoll` (Linux) or `kqueue` (Mac) to handle C10k (10,000+ concurrent connections).
* [ ] **Dynamic Content:** Add support for CGI or simplified dynamic routing.
* [ ] **Security:** Integrate OpenSSL for HTTPS support.

---

### Author

**Gaurav**

* [GitHub](https://github.com/gaurav337)
* [LinkedIn](https://linkedin.com/in/gauravkrjangid)
