#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "ipclib.h"

constexpr int st_ready{2};
constexpr int st_obsolete{3};
/**
 * Connection struct. Keep sd, status, nthread, sockaddr_in, entry, last_op and num_ops. 
 */
struct connection{
    int next_info;
    int nthread;
    int status;			    
    int sd;             
    sockaddr_in sockaddr;
    time_t entry;
    time_t last_op;
    long num_ops;
};

/**
 * Connections class. 
 */
class connections{
    bool initialized{false};                // Is it initialized?
    int first_free{0};                      // First free connection entry. 
    int nThreads{0};                        // Number of thread pairs running. 
    int MaxConn{0};                         // Max number of connections. 
    bool deleteOnExit{false};               // Delete on exit 
    
    connection *current_connections{nullptr};   // Pointer to the connection array.
    int *msg_queues{nullptr};                   // Pointer to msg queues id (IPC) array. 
public:

    connections(int MaxConnections, int NumThreads, bool deleteonexit);
    ~connections();
    void DisableDelete() { deleteOnExit=false; }
    void EnableDelete() { deleteOnExit=true; }
    int *get_queue_addr(){ return msg_queues;}
    void mark_obsolete(int idx);
    void delete_obsolete(int idx);
    int clean_repeated_ip(sockaddr_in *ppal, Semaphore &sem);
    int register_new_conn(int nthread, int sd, sockaddr_in s_in, Semaphore &sem);
    int unregister_conn(int idx, Semaphore &sem);
    int ending_operation(int idx, Semaphore &sem, connection &cur_conn);
    int check_obsolete(int idx_con, Semaphore &sem);
};
	

#endif