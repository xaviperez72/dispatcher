#ifndef THREAD_PAIR_H
#define THREAD_PAIR_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <list>
#include <algorithm>

#include "common.h"
#include "ipclib.h"
#include "checker_pids.h"
#include "connections.h"

using namespace std;

struct socket_data_t {
	int	        sd;  				// Socket Descriptor.
	sockaddr_in sockaddr;			// OK Below
	int		    shm_reg;			// Index to Array conexiones 
	string      rcvinfo;            // String to keep incoming msg.
    bool operator==(const socket_data_t &c) const 
        { return sd==c.sd && sockaddr.sin_addr.s_addr==c.sockaddr.sin_addr.s_addr && sockaddr.sin_family==c.sockaddr.sin_family && 
            sockaddr.sin_port==c.sockaddr.sin_port && sockaddr.sin_zero==c.sockaddr.sin_zero && shm_reg==c.shm_reg;};
};

class thread_pair
{
    int _idx;
    int _pipe[2];
    MessageQueue _write_queue;
    MessageQueue _common_queue;
    mutex _accept_mutex;
    //shared_ptr<mutex> _accept_mutex;
    list<socket_data_t> _sockdata;
    shared_ptr<checker_pids> _sharedptr_pids;
    shared_ptr<connections> _p_cur_connections;

public:
    thread_pair() = delete;
    thread_pair(int write_queue_id, MessageQueue common_queue, int idx, 
                shared_ptr<checker_pids> shpt_pids, shared_ptr<connections> shpt_conn);
    // It compiles with shared_ptr 
    thread_pair(const thread_pair&) { LOG_DEBUG << "XAVI - COPY CTOR thread_pair"; }; // run emplace_back : default - let emplace_back( ) work!! 
    thread_pair(thread_pair&&) = delete;     // run emplace_back(std::move( )) : default - fail!!
    thread_pair& operator=(const thread_pair&) = delete;
    thread_pair& operator=(thread_pair&&) = delete;

    void reader_thread(int idx);
    void writer_thread(int idx);

    int add_sockdata(socket_data_t sdt);
    int remove_sockdata(const socket_data_t &sdt);
    int get_sockdata_list(list<socket_data_t> &lsdt);
    int get_read_pipe() { return _pipe[0];}
    int get_write_pipe() { return _pipe[1];}
};

#endif