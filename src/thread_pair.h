#ifndef THREAD_PAIR_H
#define THREAD_PAIR_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <future>
#include <list>
#include <algorithm>
#include <json/json.h>
#include <json/value.h>

#include "common.h"
#include "ipclib.h"
#include "checker_pids.h"
#include "connections.h"
#include "protocol_msg.h"
#include "Socket.h"

/*
struct signal_synch
{
    sigset_t _sigset_new;
    sigset_t _sigset_old;
    std::mutex _cv_mutex;
    std::condition_variable _cv;
    std::future<int>  _ft_signal_handler;
    ~signal_synch(){ LOG_DEBUG << "Dtor signal_synch."; }
};
*/

struct socket_data_t {
	int	        sd;  				// Socket Descriptor.
	sockaddr_in sockaddr;			// OK Below
	int		    idx_con;			// Index to Array conexiones 
	std::string      rcvinfo;       // String to keep incoming msg.
    bool operator==(const socket_data_t &c) const 
        {   
            // LOG_DEBUG << "        sd " << sd << ":" << c.sd;
            // LOG_DEBUG << "    s_addr " << sockaddr.sin_addr.s_addr << ":" << c.sockaddr.sin_addr.s_addr;
            // LOG_DEBUG << "sin_family " << sockaddr.sin_family << ":" << c.sockaddr.sin_family;
            // LOG_DEBUG << "  sin_port " << sockaddr.sin_port << ":" << c.sockaddr.sin_port;
            // LOG_DEBUG << "   idx_con " << idx_con << ":" << c.idx_con;
            return sd==c.sd && sockaddr.sin_addr.s_addr==c.sockaddr.sin_addr.s_addr && sockaddr.sin_family==c.sockaddr.sin_family && 
            sockaddr.sin_port==c.sockaddr.sin_port && idx_con==c.idx_con;
        };
};

class thread_pair
{
public:
    std::thread th_r;
    std::thread th_w;
    
private:
    int _idx_thp{-1};
    int _pipe[2]{-1,-1};
    MessageQueue _write_queue;
    MessageQueue _common_queue;
    std::mutex _accept_mutex;
    //shared_ptr<mutex> _accept_mutex;
    std::list<socket_data_t> _sockdata;
    std::shared_ptr<keep_running_flags> _sharedptr_keep_running;
    std::shared_ptr<connections> _p_cur_connections;
    //std::shared_ptr<signal_synch> _shpt_sigsyn;
    std::shared_ptr<Semaphore> _shpt_semIPCfile;

public:
    thread_pair() = delete;
    thread_pair(MessageQueue write_queue_id, MessageQueue common_queue, int idx, std::shared_ptr<keep_running_flags> shpt_keep_running, 
        std::shared_ptr<connections> shpt_conn, 
        //std::shared_ptr<signal_synch> shpt_sigsyn, 
        std::shared_ptr<Semaphore> shpt_sem);
    // It compiles with shared_ptr 
    thread_pair(const thread_pair&) { LOG_DEBUG << "XAVI - COPY CTOR thread_pair"; }; // NOT CALLED!! run emplace_back : default - let emplace_back( ) work!! 
    // thread_pair(const thread_pair&) = delete; // fail 
    thread_pair(thread_pair&&) = delete;     // run emplace_back(std::move( )) : default - fail!!
    thread_pair& operator=(const thread_pair&) = default;
    thread_pair& operator=(thread_pair&&) = delete;

    void reader_thread(int idx_thp);
    void writer_thread(int idx_thp);

    int add_sockdata(socket_data_t sdt);
    int remove_sockdata(const socket_data_t &sdt);
    int get_sockdata_list(std::list<socket_data_t> &lsdt);
    int get_size_of_sock_list() const { return _sockdata.size();}
    int get_read_pipe() const { return _pipe[0];}
    int get_write_pipe() const { return _pipe[1];}
    int get_id() const { return _idx_thp;}
    int get_idx() const { return _idx_thp-1;}
    MessageQueue& get_write_queue() { return _write_queue; }
    void Prepare_Msg_Json_To_Send(protomsg::st_protomsg &v_protomsg, std::string msg, Json::Value &json_msg);
    void Attending_Read_Socket(socket_data_t &sdt);
    int Getting_Json_Msg_Received(std::string &msgin, protomsg::st_protomsg &v_protomsg, std::string &msgout);
};

#endif