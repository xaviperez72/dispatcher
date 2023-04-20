#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "ipclib.h"

using namespace std;

struct sockdata {
	int sd;				    // Socket Descriptor
	sockaddr_in sockaddr;	// Struct sockaddr_in
	int idx_con;			// Index to Conections 
	string rcvmsg;			// Received msg
};

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
    int clean_ip(sockaddr_in *ppal);
};
	

#endif