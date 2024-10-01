# Client-Server Socket Program in C

## Overview
This project implements a client-server socket communication using TCP protocol in C, where both client and server run on separate virtual machines (VMs) or containers. The server is designed to handle multiple concurrent clients using multithreading, and the client can initiate multiple concurrent connection requests. CPU pinning is used for performance analysis with the help of the `taskset` command, while the `perf` tool is used to evaluate the performance of various implementations.

## Features

### 1. Server Setup
- The server creates a TCP socket and listens for incoming client connections.
- Upon receiving a client request, the server creates a new thread to handle the connection, enabling concurrent client connections.
- Each connection between client and server is established with a 4-tuple (server IP, server port, client IP, client port).

### 2. Multithreaded Server
- The server uses the `pthread` library for multithreading. Each client connection spawns a new thread, which handles the client request while the main server thread continues listening for new connections on the same port.

### 3. Client Setup
- The client creates a socket and initiates the TCP connection with the server.
- The client program supports establishing `n` concurrent connections, where `n` is provided as a command-line argument.

### 4. Request for CPU Usage Information
- After establishing a connection, the client requests the top **two** CPU-consuming processes from the server.
- The server retrieves this information by reading the `/proc/[pid]/stat` file to find details such as:
  - Process name
  - Process ID (pid)
  - CPU usage time in user and kernel mode (reported in clock ticks).

### 5. Server Response
- The server responds to the client with the details of the top two CPU-consuming processes.
- The client prints this information and closes the connection.

## Performance Analysis

### Single-Threaded TCP Client-Server
- Analysis of the performance of a simple, single-threaded TCP client-server implementation.

### Concurrent TCP Client-Server
- Performance analysis of the multithreaded server handling multiple client connections.

### TCP Client-Server Using `select`
- Evaluation of performance using the `select` system call to manage multiple connections efficiently.

## Performance Metrics
Utilize the `perf` tool to analyze performance metrics such as:
- CPU clocks
- Cache misses
- Context switches

These metrics can be taken across varying numbers of concurrent connections to assess the efficiency and performance of each implementation.

## Requirements
- GCC compiler
- `pthread` library
- `perf` tool (for performance analysis)

## Usage
1. Compile the server and client programs:
   ```bash
   gcc -o server server.c -lpthread
   gcc -o client client.c
