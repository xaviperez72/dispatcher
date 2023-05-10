# Dispatcher

Dispatcher tcp/ip multithread and multiprocess
---
Dispatcher object has the following objects:
1. Shared Memory (IPC) to keep all connections defined by MaxConnections in Json config file. It keeps sd, sockaddr_in, status, last_op time for all sockets connections. 
2. Semaphore (IPC) to update previous Shared Memory (IPC) and avoid data races between processes and threads. 
___
An accept thread accepts connections and give socket data to reader thread. 


