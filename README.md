# Dispatcher

## Dispatcher tcp/ip multithread and multiprocess

It can manage multiple dispatchers listening on IP:Port for each. 
All configuration file is in a Json file. 
Every dispatcher has the following Json setup:

1. "IP" : "127.0.0.1", // IP of Dispatcher Server
2. "IpcFile" : "/home/xaviperez72/.dispatch/ipcfile_dispatch01",  // Saves ipcs on file
3. "LogLevel" : 3, // Log level
4. "MaxConnections" : 1500,  // MaxConnections allowed 
5. "NumThreads" : 20, // Number of thread pair (Reader_Thread and Writer_Thread).
6. "Port" : 9000, // Port of Dispatcher Server
7. "StopTimeout" : 30,  // 30 seconds to wait last operations to perform until definitive shutdown. 
8. "TuxCliProg" : "/home/xaviperez72/Documents/POC/DIS/build/tuxcli_tcp",   // Transactional client to process incoming messages
9. "TuxCliSetup" : "/home/xaviperez72/.dispatch/tuxconfig.json01"  // Transactional setup file
___
Dispatcher object has the following objects:^

1. Shared Memory (IPC) to keep all connections defined by MaxConnections in Json config file. It keeps sd, sockaddr_in, status, last_op time for all sockets connections.
2. Semaphore (IPC) to update previous Shared Memory (IPC) and avoid data races between processes and threads.
3. Message Queue (IPC) to put all incoming messages. It is the common message queue for TuxCliProg. Several processes read from this queue to process request (incoming message).
4. A vector of thread_pair object (Json NumThreads value). It contains Reader_Thread which reads incoming tcp/ip messages. Writer_Thread which writes outgoing tcp/ip messages. 
5. A vector of Message Queue. One queue for each Writer_Thread who read from this queue and send the message to the corresponding tcp/ip connection. 
6. Shared Memory (IPC) to keep all allowed IP's to connect.
7. Shared pointer to atomic variables for threads to keep running. When false they finish execution. 
8. Config struct of previous section with all Json values. 
___

### How it works - message flow

1. Dispatcher has an Accept_Thread for accepting new connections. 
2. When a new connections has started then Accept_Thread checks IP to allow or forbid connection.
3. If IP is allowed to enter then Accept_Thread registers the new entry on Shared Memory (connections struct).
4. Then checks which thread pair has less connections on their sockdata list.
4. Thread_Pair.add_sockdata lock a mutex and puts the new connection on the assigned thread_pair. 
5. Accept_Thread send a byte via pipe to Reader_Thread. 
6. Reader_Thread weakup from select function listening on Pipe and its own sockadata list. 
7. Reader_Thread update sockdata list to make the select again. Then it process the incoming message. 
8. Reader_Thread puts the new message in the common queue message.
9. TuxCliProg is a battery of process who read messages from common queue. 
10. TuxCliProg run thre request and the response goes back to a specific queue indicated on the incoming message. 
11. Writer_Thread only reads message from its own queue and update Shared Memory (connections struct) with all info (increase num_ops, time on last_op, etc..)
12. Writer_Thread send the message to socket descriptor indicated on shared memory connection struct. 
13. The response is received by the tcp/ip client. 


