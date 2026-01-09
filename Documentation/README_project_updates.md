# Project Updates & Usage Guide

## 🚀 Recent Changes

### 1. Advanced Synchronization (Client & Monitor)
-   **Binary Semaphore (Exclusive Access)**:
    -   The `client_app` initializes a binary semaphore (value 1).
    -   This enforces strictly exclusive access to the Shared Memory segment. Use of `sem_lock` (wait) and `sem_unlock` (signal) ensures that only one process (Client or Monitor) can access the memory at a time.
-   **Process-Shared Mutex**:
    -   A `pthread_mutex_t` has been added to the Shared Memory (`ClientState`) structure.
    -   It is configured with `PTHREAD_PROCESS_SHARED` to allow robust locking between independent processes (Client vs Monitor).
    -   **Critical Section**: The update of IP/Status in Shared Memory is now strictly protected by this Mutex combined with the Semaphore.

### 2. Multi-Client Monitoring
-   **PID-Based Identification**: Clients now use their Process ID (PID) to generate unique IPC keys (Shared Memory & Semaphore).
-   **Dynamic Monitor**: The `monitor_app` automatically scans the directory for active client sockets (`client_sock_<PID>`), attaches to their specific Shared Memory, and displays their status in a real-time table.

### 3. Persistence
-   **Text-Based**: The server persists lease information in `leases.txt` (Human-readable).
-   **Cleanup**: Binary databases (`leases.db`) have been completely removed.

### 4. Enhanced Internal Architecture
-   **System V Message Queues**:
    -   Used for thread-safe logging. The `logger_thread_func` consumes messages sent by the main thread, network thread, and even the forked child process.
-   **Lease Renewal via Fork/Pipe**:
    -   The client forks a child process specifically to handle lease renewal timers.
    -   **Pipe**: The parent process uses a pipe to send control signals ("STOP") to the child when the lease is released or the application exits.

---

## 🛠️ How to Run the Project

### A. Automated Simulation (Recommended)
This script recompiles the entire project from the root and launches 3 clients simultaneously to demonstrate the multi-client capabilities.

```bash
cd Client
./simulare_clienti.sh
```
*(You can also run `make test` from the root directory)*

### B. Manual Execution with Monitor

1.  **Start Server**:
    ```bash
    ./server_app
    ```

2.  **Start Monitor**:
    Open a new terminal. This will show an empty table initially.
    ```bash
    ./monitor_app
    ```

3.  **Start Clients**:
    Open new terminals or run the simulation script to start clients.
    ```bash
    # Option 1: Manual single client
    ./client_app
    
    # Option 2: Run simulation of 3 clients
    cd Client && ./simulare_clienti.sh
    ```

4.  **Observe**:
    Watch the `monitor_app` terminal. It will detect the new processes (PIDs), lock the Mutex, read the state, and update the table in real-time!

---

## ✅ Verification Checklist
-   [x] **Server**: Allocates IPs and saves to `leases.txt`.
-   [x] **Client**: Connects, receives IP, protects memory with Mutex+Sem.
-   [x] **Monitor**: Displays "BOUND" status for multiple clients concurrently.
