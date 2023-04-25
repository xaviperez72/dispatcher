#include "dispatcher.h"

Dispatcher::Dispatcher(dispatch_cfg cfg, std::shared_ptr<checker_pids> shpt_pids)
        :_config{cfg},
        _sharedptr_pids{shpt_pids}
{
    LOG_DEBUG << "DISPATCHER CTOR config ";
}

void Dispatcher::Launch_All_Threads()
{
    _v_thread_pair.reserve(_config.NumThreads);
    
    for(int i=0; i<_config.NumThreads; i++) {
        _v_thread_pair.emplace_back(*(_p_cur_connections->get_queue_addr()+i), *_shpt_Common_Msg_Queue, i+1, _sharedptr_pids, _p_cur_connections, _shpt_sigsyn);
        // _v_thread_pair.emplace_back(std::move(thread_pair(*(_p_cur_connections->get_queue_addr()+i), *_shpt_Common_Msg_Queue, i+1, _sharedptr_pids, _p_cur_connections)));
    }

    LOG_DEBUG << "All Threads launched!! ---------------------------";
}

void Dispatcher::Prepare_Server_Socket()
{
    //AF_INET (Internet mode) SOCK_STREAM (TCP mode) 0 (Protocol any)
    _server_socket = Socket(AF_INET,SOCK_STREAM,0); 

    //You can reuse the address and the port
    int optVal = 1;
    _server_socket.socket_set_opt(SOL_SOCKET, SO_REUSEADDR, &optVal); 
    
    //Bind socket 
    _server_socket.bind(_config.IP, std::to_string(_config.Port)); 

    //Start listening for incoming connections (10 => maximum of 10 Connections in Queue)
    _server_socket.listen(10); 
}

void Dispatcher::Signal_Handler_For_Threads()
{
    _shpt_sigsyn = std::make_shared<signal_synch>();

    sigemptyset(&_shpt_sigsyn->_sigset);
    sigaddset(&_shpt_sigsyn->_sigset, SIGINT);
    sigaddset(&_shpt_sigsyn->_sigset, SIGUSR1);
    sigaddset(&_shpt_sigsyn->_sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &_shpt_sigsyn->_sigset, nullptr);

    auto signal_handler = [this]() 
    {
        int signum = 0;
        // wait until a signal is delivered:
        LOG_DEBUG << "LAMBDA signal_handler" << std::endl;
        sigwait(&_shpt_sigsyn->_sigset, &signum);
        _sharedptr_pids->_keep_accepting.store(false);
        _sharedptr_pids->_keep_working.store(false);
        LOG_DEBUG << "signal_handler WEAK-UP - before notify_all" << std::endl;
        // notify all waiting workers to check their predicate:
        _shpt_sigsyn->_cv.notify_all();
        return signum;
    };

    _shpt_sigsyn->_ft_signal_handler = std::async(std::launch::async, signal_handler);
}

int Dispatcher::Accept_by_Select()
{
    int nfound;
    fd_set readfds;
    static struct timeval timeout;

    FD_ZERO(&readfds);
    int n_sock_sel = 0;
      
    FD_SET(_server_socket.sock, &readfds);                
    n_sock_sel=(n_sock_sel>_server_socket.sock)? n_sock_sel : _server_socket.sock+1;

    timeout.tv_sec = 1;
    timeout.tv_usec= 0;

    if ((nfound = select(n_sock_sel,&readfds,0,0,&timeout)) == -1) 
    {
        LOG_ERROR << "select failed!!" << strerror(errno);
        raise(SIGINT);
    }
    else {
        if(nfound==0)    // timeout 
        {
            return 0;
        }
        if (FD_ISSET(_server_socket.sock,&readfds)) 
        {
            return 1;
        }
    }
    return 0;
}

int Dispatcher::Accept_Thread()
{
    // Only father (checker_pids) can delete IPCS.

    _shpt_shmIPCfile->DisableDelete();
    _shpt_semIPCfile->DisableDelete();
    _shpt_Common_Msg_Queue->DisableDelete();
    _shpt_shmAllowedIPs->DisableDelete();
    _p_cur_connections->DisableDelete();

    Signal_Handler_For_Threads();

    Launch_All_Threads();

    Prepare_Server_Socket();
    
    while(_sharedptr_pids->_keep_accepting.load()) 
    {

        if(Accept_by_Select()==0) // timeout - Check _keep_accepting...
            continue;

        // Accept the incoming connection and creates a new Socket to the client
        Socket newSocket = _server_socket.accept(); 
        
        // Checks if IP is in Table.

        // TO DO...


        // Get all socket_data_t info:

        socket_data_t sd_info;
        sd_info.sd = newSocket.sock;
        memcpy(&sd_info.sockaddr, newSocket.address_info.ai_addr,sizeof(struct sockaddr_in));

        // Clean possible obsoletes.

        int nthread = _p_cur_connections->clean_repeated_ip(&sd_info.sockaddr, *_shpt_semIPCfile);

        // It currently exists, remove it. 
        if(nthread >= 0)
        {
            int msg_qid=*(_p_cur_connections->get_queue_addr() + nthread);

            MessageQueue write_queue(msg_qid);
            assert(write_queue && "write_queue doesn't exists");
            write_queue.send<decltype(protopipe::WEAKUP)>(protopipe::TYPE_WEAKUP, protopipe::WEAKUP);
        }

        // Get less charged.
        int th_id = LessCharged();

        if ((sd_info.idx_con = _p_cur_connections->register_new_conn(th_id, sd_info.sd, sd_info.sockaddr, *_shpt_semIPCfile)) < 0)
        {
            close(sd_info.sd);
            if(sd_info.idx_con >= 0)
                _p_cur_connections->unregister_conn(sd_info.idx_con, *_shpt_semIPCfile);
            continue;
        }   

        // Assign sd to thread_pair...
        if(Assign_connection_to_thread_pair(th_id, &sd_info) < 0)
        {
            close(sd_info.sd);
            if(sd_info.idx_con >= 0)
                _p_cur_connections->unregister_conn(sd_info.idx_con, *_shpt_semIPCfile);
        }
    }
    
    _server_socket.close();

    // wait for signal handler to complete
    int signal = _shpt_sigsyn->_ft_signal_handler.get();
    LOG_DEBUG << "ok   - RECEIVED SIGNAL FROM FUTURE _ft_signal_handler: " << signal << std::endl;
    return 0;
}

int Dispatcher::Assign_connection_to_thread_pair(int th_id, socket_data_t *sd_info)
{
    // TO DO - Check result or exceptions...
    if (_v_thread_pair[th_id].add_sockdata(*sd_info) < 0)
        return -1;

    // Send WEAKUP byte to notify there is a new connection...
    write(_v_thread_pair[th_id].get_write_pipe(), &protopipe::WEAKUP, protopipe::TYPE_WEAKUP);
    return 0;
}

int Dispatcher::LaunchTuxCli() 
{
    char strmsgq[20];
    char strshmip[20];

    sprintf(strmsgq, "%d", _shpt_Common_Msg_Queue->getid());
    sprintf(strshmip,"%d", _shpt_shmAllowedIPs->getid());
    
    LOG_DEBUG << "Launching Forker Tuxcli:" << _config.TuxCliProg.c_str() << " " << strmsgq << " " << strshmip;

    execl(_config.TuxCliProg.c_str(), "Fork_TuxCli" ,strmsgq, strshmip, (char*)0);

    LOG_ERROR << "error execl:" << strerror(errno);
  
    return -1;

}

int Dispatcher::LessCharged()
{
    // return less_charged;

    auto it_th = std::min_element(_v_thread_pair.begin(),_v_thread_pair.end(),
        [](thread_pair &a, thread_pair &b){ return a.get_size_of_sock_list() < b.get_size_of_sock_list();}
        );
    return it_th->get_idx();
}

Dispatcher::operator bool()
{
    return true;
}

int Dispatcher::IPC_Setting_Up()
{
    Json::Value ipcs_json;
    int shm_conn_id=-1, sem_conn_id=-1, msg_common_id=-1, shm_ips_id=-1;

    GetCfgFile ipcfile(_config.IpcFile);
    if(ipcfile)
    {
        ipcs_json = ipcfile.get_json();
        sem_conn_id = ipcs_json["sem_conn_id"].asInt();
        shm_conn_id = ipcs_json["shm_conn_id"].asInt();
        msg_common_id = ipcs_json["msg_common_id"].asInt();
        shm_ips_id = ipcs_json["shm_ips_id"].asInt();
        
        // Try to get all info.
        Semaphore sem(sem_conn_id);
        if(sem) {
            LOG_ERROR << "Semaphore created previously (" << sem_conn_id << ") and it is running now. File: " << ipcfile.get_file_name();
            return -1;
        }
        else {
            SharedMemory shm(shm_conn_id);
            if(shm) {
                LOG_ERROR << "SharedMemory created previously (" << shm_conn_id << ") and it is running now. File: " << ipcfile.get_file_name();
                return -1;
            }
            else {
                MessageQueue msg(msg_common_id);
                if(msg) {
                    LOG_ERROR << "MessageQueue created previously (" << msg_common_id << ") and it is running now. File: " << ipcfile.get_file_name();
                    return -1;
                }
                else {
                    SharedMemory shmip(shm_ips_id);
                    if(shmip) {
                        LOG_ERROR << "SharedMemory created previously (" << shm_ips_id << ") and it is running now. File: " << ipcfile.get_file_name();
                        return -1;
                    }
                    else {
                        LOG_DEBUG << "ipcfile obsolete, values (shm_conn_id,sem_conn_id): " << shm_ips_id << "," << sem_conn_id;            
                    }
                }
            }
        }
    }

    LOG_DEBUG << "Should be 0 (creating). shm_conn_id,sem_conn_id,msg_common_id,shm_ips_id:" << 
                shm_conn_id << "," << sem_conn_id << "," << msg_common_id << "," << shm_ips_id;
    
    _shpt_semIPCfile = std::make_shared<Semaphore>(IPC_PRIVATE, 1, 1, true);

    if(!*_shpt_semIPCfile)
    {
        LOG_ERROR << "Semaphore not created";
        return -1;
    }
    
    _shpt_semIPCfile->Lock();

    //  Creating Shared Memory

    int shm_len = sizeof(connections)+(sizeof(connection)*_config.MaxConnections)+(sizeof(int)*_config.NumThreads);

    _shpt_shmIPCfile = std::make_shared<SharedMemory>(IPC_PRIVATE,shm_len, true);

    if(!*_shpt_shmIPCfile) 
    {
        _shpt_semIPCfile->Unlock();   
        LOG_ERROR << "SharedMemory not created";
        return -1;
    }

    // PLACEMENT NEW 
    // SHARED POINTER POINTING TO SHARED MEMORY: DELETER NEEDED: calls constructor and deleter on destruction
    std::shared_ptr<connections> p_conn( new (_shpt_shmIPCfile->getaddr()) connections(_config.MaxConnections,_config.NumThreads, true), 
                [](connections *p){ LOG_DEBUG << "connections own deleter: "; p->~connections(); }
    );     

    _p_cur_connections = p_conn;

    _shpt_semIPCfile->Unlock();
    
    // STEP 2 - Common message queue 

    _shpt_Common_Msg_Queue = std::make_shared<MessageQueue>(IPC_PRIVATE,true);

    if(!*_shpt_Common_Msg_Queue)
    {
        return -1;
    }
  
    // STEP 3 - Allowed IP's - TO DO a better implementation (2 IP's by now)

    shm_len = sizeof(allowed_ips)*2;

    _shpt_shmAllowedIPs = std::make_shared<SharedMemory>(IPC_PRIVATE,shm_len, true);

    _allowed_ips = static_cast<allowed_ips *>(_shpt_shmAllowedIPs->getaddr());

    _allowed_ips[0].allowed = true;
    _allowed_ips[0].trace = false;
    inet_aton("127.0.0.1", &(_allowed_ips[0].ip));
    
    _allowed_ips[1].allowed = true;
    _allowed_ips[1].trace = false;
    inet_aton("127.0.1.1", &(_allowed_ips[1].ip));

    // SAVE ON IPC FILE

    ipcs_json["shm_conn_id"] = _shpt_shmIPCfile->getid();
    ipcs_json["sem_conn_id"] = _shpt_semIPCfile->getid();
    ipcs_json["msg_common_id"] = _shpt_Common_Msg_Queue->getid();
    ipcs_json["shm_ips_id"] = _shpt_shmAllowedIPs->getid();
    ipcfile.save_cfg_file(ipcs_json);

    return 0;
}

int Dispatcher::operator()()
{
    LOG_DEBUG << "Dispatcher Operator " << _config.NumDispatch;

    // STEP 1 - Create all IPCs 

    if(IPC_Setting_Up() < 0)
        return -1;
    
    // STEP 4 - LaunchTuxCli and Accept_Thread...

    checker_pids chk_procs;
    
    chk_procs.add(std::bind(&Dispatcher::LaunchTuxCli, this),"LaunchTuxCli");

    chk_procs.add(std::bind(&Dispatcher::Accept_Thread, this),"Accept_Thread");

    LOG_DEBUG << "Attached to Connections: " << _shpt_shmIPCfile->get_nattach();

    // Launching all processes... 
    int ret=chk_procs();

    if(ret) // forker = 1 --> delete all resources...
    {
        LOG_DEBUG << "EnableDelete all";
        _shpt_shmIPCfile->EnableDelete();
        _shpt_semIPCfile->EnableDelete();
        _shpt_Common_Msg_Queue->EnableDelete();
        _shpt_shmAllowedIPs->EnableDelete();
        _p_cur_connections->EnableDelete();


    }
    
    LOG_DEBUG << "Ending operator()";
    return ret;
}