#include "thread_pair.h"

using namespace std;

thread_pair::thread_pair(int write_queue_id, MessageQueue common_queue, int idx_thp, 
        std::shared_ptr<checker_pids> shpt_pids, std::shared_ptr<connections> shpt_conn, 
        std::shared_ptr<signal_synch> shpt_sigsyn, std::shared_ptr<Semaphore> shpt_sem)
{
    _shpt_sigsyn = shpt_sigsyn;
    _write_queue = MessageQueue(write_queue_id);

    assert(_write_queue && "write_queue not operative.");
    assert(common_queue && "read_queue (common queue) not operative.");

    // _accept_mutex = std::make_shared<mutex>();

    _common_queue = common_queue;
    _idx_thp = idx_thp;
    _sharedptr_pids = shpt_pids;
    _p_cur_connections = shpt_conn;
    _shpt_semIPCfile = shpt_sem;

    if (pipe(_pipe) < 0 ) {
        assert(false && "Cannot create a pipe");
	}

    th_r = std::thread(&thread_pair::reader_thread, this, idx_thp);
    th_w = std::thread(&thread_pair::writer_thread, this, idx_thp);
}


void thread_pair::Attending_Read_Socket(socket_data_t &sdt)
{
    
}

void thread_pair::reader_thread(int idx_thp)
{
    list<socket_data_t> lsdt;
    int readpipe=get_read_pipe();
    LOG_DEBUG << "reader_thread " << idx_thp;

    while(_sharedptr_pids->_keep_accepting.load()==true) 
    {
        fd_set readset;
        int nfds, ret_sel;

        /*
        std::unique_lock<std::mutex> lck(_shpt_sigsyn->_cv_mutex);

        auto shpt_pids = _sharedptr_pids;
        // wait for up to 10 seconds
        _shpt_sigsyn->_cv.wait_for(lck, std::chrono::seconds(10),
              [&shpt_pids]() { return !shpt_pids->_keep_accepting.load(); });
        */

        //sleep(1);
        // STEP 1 - Getting all sockets descriptors to SELECT.

        get_sockdata_list(lsdt);

        FD_ZERO(&readset);
        nfds = 0; 

        FD_SET(readpipe, &readset);
        nfds = (readpipe<nfds) ? nfds : (readpipe+1);
        
        for(auto it=lsdt.begin(); it != lsdt.end(); it++) 
        {
            FD_SET(it->sd, &readset);
            nfds = (it->sd < nfds) ? nfds : (it->sd+1);
        }

        if((ret_sel=select(nfds, &readset, NULL, NULL, NULL)) < 0 ) 
        {
            int someerror=errno;
            LOG_ERROR << "SELECT FAILED: (" << ret_sel << ") " << strerror(someerror);
            if( someerror == EINTR ) 
                continue;
            return;
        }

        if( FD_ISSET(readpipe, &readset) ) 
        {
            read(readpipe,protopipe::GETWEAKUP,protopipe::LEN_WEAKUP);
        }

        for(auto it=lsdt.begin(); it != lsdt.end(); it++) 
        {
            if(_p_cur_connections->check_obsolete(it->idx_con, *_shpt_semIPCfile) != 0 ) 
            {
                /* Obsolete or some issues */

                LOG_DEBUG << "SD obsolete (deleted): " << it->sd << " IP:Port: " << 
                    inet_ntoa(it->sockaddr.sin_addr) << ":" << ntohs(it->sockaddr.sin_port);
                
                remove_sockdata(*it);
                close(it->sd);
            } 
	        else 
	        if(FD_ISSET(it->sd, &readset) ) 
            {
  
                // Getting message from socket...
                Attending_Read_Socket(*it);

            }
        }

    }

    // pthread_sigmask(SIG_SETMASK, &_shpt_sigsyn->_sigset_old, nullptr);

    LOG_DEBUG << "Ending reader_thread " << idx_thp;
}

void thread_pair::Prepare_Msg_Json_To_Send(protomsg::st_protomsg &v_protomsg, string msg, Json::Value &json_msg)
{
    json_msg["TERF"] = v_protomsg.terf;
    json_msg["TERL"] = v_protomsg.terl;
    json_msg["GUID"] = std::string(v_protomsg.guid);
    json_msg["PID"] = std::string(v_protomsg.pid);
    json_msg["AID"] = std::string(v_protomsg.aid);
    json_msg["CABX"] = std::string(v_protomsg.cabx);
    json_msg["MSG"] = msg;
}

void thread_pair::writer_thread(int idx_thp)
{
    long type;
    std::string msg, msgout;
    Json::Value json_msg;
    protomsg::st_protomsg v_protomsg;
    connection cur_conn;
    Socket send_sock;

    LOG_DEBUG << "writer_thread " << idx_thp;

    while(_sharedptr_pids->_keep_working.load()==true) 
    {
        /*
        std::unique_lock<std::mutex> lck(_shpt_sigsyn->_cv_mutex);

        auto shpt_pids = _sharedptr_pids;
        // wait for up to 10 seconds
        _shpt_sigsyn->_cv.wait_for(lck, std::chrono::seconds(10),
              [&shpt_pids]() { return !shpt_pids->_keep_working.load(); });
        */
        //sleep(1);
        // STEP 1 - ReadFromQueue  (idx_con on message)
        
        _write_queue.rcv(&v_protomsg, msg);

        Prepare_Msg_Json_To_Send(v_protomsg, msg, json_msg);
        
        _p_cur_connections->ending_operation(v_protomsg.idx, *_shpt_semIPCfile, cur_conn);

        // STEP 2 - Send msg to socket 

        send_sock.sock = cur_conn.sd;

        stringstream ss;
        ss << json_msg;
        ss >> msgout;

        LOG_DEBUG << "msgout: " << msgout; 

        send_sock.socket_write(msgout);
        
    }

    //pthread_sigmask(SIG_SETMASK, &_shpt_sigsyn->_sigset_old, nullptr);
    LOG_DEBUG << "Ending writer_thread " << idx_thp;
}

int thread_pair::add_sockdata(socket_data_t sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    _sockdata.emplace_back(sdt);
    
    return 0;
}

int thread_pair::remove_sockdata(const socket_data_t &sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    std::list<socket_data_t>::iterator findIter = std::find(_sockdata.begin(), _sockdata.end(), sdt);

    if(findIter == _sockdata.end())
        return -1;
    else
        return 0;
}

int thread_pair::get_sockdata_list(list<socket_data_t> &lsdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    lsdt = _sockdata;
    
    return 0;
}
