#include "dispatcher.h"

Dispatcher::Dispatcher(dispatch_cfg cfg, std::shared_ptr<checker_pids> shpt_pids)
        :_config{cfg},
        _sharedptr_pids{shpt_pids}
{
}

void Dispatcher::Launch_All_Threads()
{
    _v_thread_pair.reserve(_config.NumThreads);

    for(int i=0; i<_config.NumThreads; i++) {
        _v_thread_pair.emplace_back(*(_p_cur_connections->get_queue_addr()+i), *_shpt_Common_Msg_Queue, i+1, _sharedptr_pids, _p_cur_connections);
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

int Dispatcher::Accept_Thread()
{
    // Only father (checker_pids) can delete IPCS.

    _shpt_shmIPCfile->DisableDelete();
    _shpt_semIPCfile->DisableDelete();
    _shpt_Common_Msg_Queue->DisableDelete();
    _shpt_shmAllowedIPs->DisableDelete();
    _p_cur_connections->DisableDelete();

    Launch_All_Threads();

    Prepare_Server_Socket();
    
    while(_sharedptr_pids->_keep_accepting) 
    {
        // Accept the incoming connection and creates a new Socket to the client
        Socket newSocket = _server_socket.accept(); 
        
        // Checks if IP is in Table.

        // Clean possible obsoletes.
        
        sockaddr_in *s_in = (struct sockaddr_in *) newSocket.address_info.ai_addr;

        int nthread = _p_cur_connections->clean_ip(s_in);

        if(nthread != -1)
        {
            int msg_qid=*(_p_cur_connections->get_queue_addr() + nthread);

            MessageQueue write_queue(msg_qid);
            if(write_queue)
            {
                write_queue.send<decltype(WEAKUP)>(TYPE_WEAKUP, WEAKUP);
            }
            else
                LOG_ERROR << "MessageQueue (write_queue) " << msg_qid << " doesn't exists.";
            
        }

        LOG_DEBUG << "Accept_Thread";
    }
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
    return 1;
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
    else {
        //LOG_DEBUG << "exiting to avoid destruction disaster.";
        //exit(1);
    }
    
    LOG_DEBUG << "Ending operator()";
    return ret;
}