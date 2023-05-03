#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "ipclib.h"

constexpr int st_ready{2};
constexpr int st_obsolete{3};

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

class connections{
    int initialized{0};
    int first_free{0};
    int nThreads{0};
    int MaxConn{0};
    bool deleteOnExit{false};
    connection *current_connections{nullptr};
    int *msg_queues{nullptr};
public:
    connections(int MaxConnections, int NumThreads, bool deleteonexit);
    ~connections();
    void DisableDelete() { deleteOnExit=false; }
    void EnableDelete() { deleteOnExit=true; }
    connection *get_conn_addr(){ return current_connections;}
    int *get_queue_addr(){ return msg_queues;}
    int get_nthreads(){ return nThreads;}
    void mark_obsolete(int idx);
    void delete_obsolete(int idx);
    int clean_repeated_ip(sockaddr_in *ppal, Semaphore &sem);
    int register_new_conn(int nthread, int sd, sockaddr_in s_in, Semaphore &sem);
    int unregister_conn(int idx, Semaphore &sem);
    int ending_operation(int idx, Semaphore &sem, connection &cur_conn);
    int check_obsolete(int idx_con, Semaphore &sem);
};
	

#endif