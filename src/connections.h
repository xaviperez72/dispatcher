#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "ipclib.h"

constexpr int st_free{0};
constexpr int st_ready{2};
constexpr int st_obsolete{3};
/**
 * Connection struct. Keep sd, status, nthread, sockaddr_in, entry, last_op and num_ops. 
 */
struct connection{
    int next_info{0};               // Next free connection
    int nthread{0};                 // idx of thread pair.
    int status{st_free};            // connection status 
    int sd{-1};                     // Socket descriptor
    sockaddr_in sockaddr{};         // IP:port info
    time_t entry{};                 // time of 1st operation entry
    time_t last_op{};               // time of last operation.
    long num_ops{0};                // Number of processed operations. 
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
   /**
    * connections Constructor
    *
    * @param MaxConnections Max number of connections. 
    * @param NumThreads = Number of thread pairs running
    * @param deleteonexit = (false = don't remove msg_queues on destruction. true = remove it)
    */
    connections(int MaxConnections, int NumThreads, bool deleteonexit);
    // connections Destructor - Destroy all msg_queues if deleteOnExit is true
    ~connections();
    
    // Disable delete on destruction.
    void DisableDelete() { deleteOnExit=false; }
    // Enable delete on destruction.
    void EnableDelete() { deleteOnExit=true; }
    // Get msg_queues pointer.
    int *get_queue_addr(){ return msg_queues;}
    // Marks a connection as obsolete (st_obsolete=3) on the idx position of the connection array
    void mark_obsolete(int idx);
    // Marks a connection as free (st_obsolete=3) on the idx position of the connection array
    void delete_obsolete(int idx);
   /**
    * Clean repeated IP in connection array. 
    *
    * @param ppal sockaddr_in data (IP:port)
    * @param sem Semaphore to lock to avoid data races.
    */
    int clean_repeated_ip(sockaddr_in *ppal, Semaphore &sem);
   /**
    * Registers new connection on array. 
    *
    * @param nthread Thread pair idx
    * @param sd Socket descriptor
    * @param s_in sockaddr_in info
    * @param sem Semaphore to lock to avoid data races.
    */
    int register_new_conn(int nthread, int sd, sockaddr_in s_in, Semaphore &sem);
    /**
    * Unregisters connection on array. 
    *
    * @param idx Index on connection array
    * @param sem Semaphore to lock to avoid data races.
    */
    int unregister_conn(int idx, Semaphore &sem);
    /**
    * Sets connection operation info: add 1 to operation counter, gets time
    *
    * @param idx Index on connection array
    * @param sem Semaphore to lock to avoid data races.
    * @param cur_conn Gets connection struct of connection array (idx position)
    */
    int ending_operation(int idx, Semaphore &sem, connection &cur_conn);
    /**
    * if connection[idx_con] is obsolete marks it as st_obsolete=2. 
    *
    * @param idx Index on connection array
    * @param sem Semaphore to lock to avoid data races.
    */
    int check_obsolete(int idx_con, Semaphore &sem);
};
	

#endif