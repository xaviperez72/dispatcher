#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <json/json.h>
#include <json/value.h>
#include <memory>

#include "common.h"
#include "dispatch_cfg.h"
#include "checker_pids.h"
#include "Socket.h"
#include "connections.h"
#include "getcfgfile.h"
#include "ipclib.h"
#include "thread_pair.h"
#include "proto_pipe.h"

struct allowed_ips
{
    bool trace{false};
    bool allowed{true};
    in_addr ip;
};

class Dispatcher {
    std::vector<thread_pair> _v_thread_pair;

    std::shared_ptr<checker_pids> _sharedptr_pids;
    dispatch_cfg _config;
    Socket _server_socket;
    std::shared_ptr<Semaphore> _shpt_semIPCfile;
    std::shared_ptr<SharedMemory> _shpt_shmIPCfile;
    // Shared Connections between dispatcher and tuxclients - MAX_CONNECTIONS
    std::shared_ptr<connections> _p_cur_connections;

    // Common message queue - All receiving messages go here. 
    std::shared_ptr<MessageQueue> _shpt_Common_Msg_Queue;
    
    // Allowed IP's - Used by Accept_Thread (mainly), Reader and Writer for logging purposes. 
    // Sempahore not needed.
    std::shared_ptr<SharedMemory> _shpt_shmAllowedIPs;
    allowed_ips *_allowed_ips;

public:
    Dispatcher() = delete;
    Dispatcher(dispatch_cfg cfg, std::shared_ptr<checker_pids> shpt_pids);

    void Launch_All_Threads();
    void Prepare_Server_Socket();
    int IPC_Setting_Up();
    int Accept_Thread();

    int LessCharged();
    int LaunchTuxCli();
    operator bool();
    int operator()();
};

#endif