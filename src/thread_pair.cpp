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
    int status;
    Socket sock;
    sock.sock = sdt.sd;
    std::string msgin;
    std::string msgout;
    Json::Value json_msg;
    protomsg::st_protomsg v_protomsg;

    LOG_DEBUG << "Reading from sd " << sdt.sd;

    if((status=sock.socket_read(msgin,1024)) <= 0) 
    {
        LOG_ERROR << "Socket_read error: " << errno << ":" << strerror(errno);
        LOG_ERROR << "Socket sd: " << sdt.sd << " IP:Port: " << 
               inet_ntoa(sdt.sockaddr.sin_addr) << ":" << ntohs(sdt.sockaddr.sin_port);

        remove_sockdata(sdt);
        _p_cur_connections->unregister_conn(sdt.idx_con, *_shpt_semIPCfile);
        close(sdt.sd);
        return;
    }
    sdt.rcvinfo = msgin;

    // TO DO - Check IP is in table. Not needed since AccceptThread checks for valid IP.

    if(!Getting_Json_Msg_Received(msgin, v_protomsg, msgout))
    {
        LOG_ERROR << "Error Message Format (json): " << msgin;
        LOG_ERROR << "Socket sd: " << sdt.sd << " IP:Port: " << 
            inet_ntoa(sdt.sockaddr.sin_addr) << ":" << ntohs(sdt.sockaddr.sin_port);

        // Send back MSGIN original.
        v_protomsg.q_write = _common_queue.getid();  // Not usefull in this case...
        if(!_write_queue.send(&v_protomsg, msgin)) 
        {
            LOG_ERROR << "Impossible to send to Writer_Thread through _write_queue: " << sdt.sd;
            remove_sockdata(sdt);
            _p_cur_connections->unregister_conn(sdt.idx_con, *_shpt_semIPCfile);
            close(sdt.sd);
            return;
        }
    }
    else 
    {
        LOG_DEBUG << "Sending to common queue for TuxCli...";
        // Send MSGOUT to common queue to process by TuxCli 
        v_protomsg.q_write = _write_queue.getid();  // Queue to respond to proper Writer_Thread...
        if(!_common_queue.send(&v_protomsg, msgout)) 
        {
            LOG_ERROR << "Impossible to send to TuxCli through _common_queue: " << sdt.sd;
            remove_sockdata(sdt);
            _p_cur_connections->unregister_conn(sdt.idx_con, *_shpt_semIPCfile);
            close(sdt.sd);
            return;
        }
    }
}

int thread_pair::Getting_Json_Msg_Received(string &msgin, protomsg::st_protomsg &v_protomsg, string &msgout)
{
    Json::Value json_msg;
    stringstream ss(msgin);
    ss >> json_msg;

    v_protomsg.terf = json_msg["TERF"].asInt();
    v_protomsg.terl = json_msg["TERL"].asInt();
    strncpy(v_protomsg.guid, json_msg["GUID"].asCString(), sizeof(v_protomsg.guid)-1);
    strncpy(v_protomsg.pid, json_msg["PID"].asCString(), sizeof(v_protomsg.pid)-1);
    strncpy(v_protomsg.aid, json_msg["AID"].asCString(), sizeof(v_protomsg.aid)-1);
    strncpy(v_protomsg.cabx, json_msg["CABX"].asCString(), sizeof(v_protomsg.cabx)-1);
    msgout = std::string(json_msg["MSG"].asCString());

    v_protomsg.mtype = protomsg::TYPE_NORMAL_MSG;

    LOG_DEBUG << "v_protomsg:" << v_protomsg;

    if (v_protomsg.terf==0 || v_protomsg.terl == 0 || 
        strlen(v_protomsg.guid) == 0 || strlen(v_protomsg.pid) == 0 || 
        strlen(v_protomsg.aid) == 0 || strlen(v_protomsg.cabx) == 0 || 
        msgout.size() == 0)
    {
        return 0;
    }
    
    return 1;
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

        LOG_DEBUG << "Accepting ndfs=" << nfds << " connections. " << sizeof(readset);

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
            read(readpipe,protopipe::GETPIPEMSG,protopipe::LEN_PIPEMSG);
            if(protopipe::GETPIPEMSG[0]==protopipe::ENDING_PIPE[0]) {
                LOG_DEBUG << "ENDING_PIPE received.";
                continue;   // TO REMOVE. Maybe it needs to finish all messages. 
            }
            else 
                LOG_DEBUG << "WEAKUP_PIPE received.";

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

        if(v_protomsg.mtype == protomsg::TYPE_ENDING_MSG) 
        {
            LOG_DEBUG << "Received protomsg::TYPE_ENDING_MSG"; 
            continue;
        }

        Prepare_Msg_Json_To_Send(v_protomsg, msg, json_msg);
        
        _p_cur_connections->ending_operation(v_protomsg.idx, *_shpt_semIPCfile, cur_conn);

        // STEP 2 - Send msg to socket 

        send_sock.sock = cur_conn.sd;

        stringstream ss;
        string field;
        ss << json_msg;
    
        while (getline(ss, field)) 
            msgout += field;
    
        LOG_DEBUG << "Sending msgout: " << msgout; 

        send_sock.socket_write(msgout);
        
        msgout.clear();
    }

    //pthread_sigmask(SIG_SETMASK, &_shpt_sigsyn->_sigset_old, nullptr);
    LOG_DEBUG << "Ending writer_thread " << idx_thp;
}

int thread_pair::add_sockdata(socket_data_t sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    _sockdata.emplace_back(sdt);
    
    LOG_DEBUG << "add_sockdata OK!! " << sdt.sd;
    return 0;
}

int thread_pair::remove_sockdata(const socket_data_t &sdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    std::list<socket_data_t>::iterator findIter = std::find(_sockdata.begin(), _sockdata.end(), sdt);

    if(findIter == _sockdata.end()) {
        LOG_DEBUG << "remove_sockdata NOT FOUND!! " << sdt.sd;
        return -1;
    }
    else {
        socket_data_t item = *findIter;
        _sockdata.remove(item);
        LOG_DEBUG << "remove_sockdata OK!! " << sdt.sd;
        return 0;
    }
}

int thread_pair::get_sockdata_list(list<socket_data_t> &lsdt)
{
    std::lock_guard<std::mutex> l_guard{_accept_mutex};

    lsdt = _sockdata;
    
    LOG_DEBUG << "get_sockdata_list OK!! lsdt.size()=" << lsdt.size(); 

    return 0;
}
