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
#include "protocol_msg.h"

struct allowed_ips
{
    bool trace{false};
    bool allowed{true};
    in_addr ip;
};

class Dispatcher {
    dispatch_cfg _config;

    std::vector<thread_pair> _v_thread_pair;

    std::shared_ptr<keep_running_flags> _sharedptr_keep_running;

    std::shared_ptr<Semaphore> _shpt_semIPCfile;

    std::shared_ptr<SharedMemory> _shpt_shmIPCfile;

    Socket _server_socket;
    // Shared Connections between dispatcher and tuxclients
    std::shared_ptr<connections> _p_cur_connections;

    // Common message queue - All receiving messages go here. 
    std::shared_ptr<MessageQueue> _shpt_Common_Msg_Queue;
    
    // Allowed IP's - Used by Accept_Thread (mainly), Reader and Writer for logging purposes. 
    // Sempahore not needed.
    std::shared_ptr<SharedMemory> _shpt_shmAllowedIPs;
    allowed_ips *_allowed_ips;

    //std::shared_ptr<signal_synch> _shpt_sigsyn;

public:
    Dispatcher() = delete;
    Dispatcher(dispatch_cfg cfg, std::shared_ptr<keep_running_flags> shpt_keep_running);
    ~Dispatcher();

    // Constructor de copia
    Dispatcher(const Dispatcher& other);

    // Operador de asignación por copia
    Dispatcher& operator=(const Dispatcher& other);

    // Constructor de movimiento
    Dispatcher(Dispatcher&& other) noexcept;

    // Operador de asignación por movimiento
    Dispatcher& operator=(Dispatcher&& other) noexcept;
    
    void Launch_All_Threads();
    void Prepare_Server_Socket();
    // void Signal_Handler_For_Threads();
    int IPC_Setting_Up();
    int Accept_Thread();
    int Assign_connection_to_thread_pair(int th_id, socket_data_t *sd_info);
    int Accept_by_Select();
    void Ending_all_threads();
    //void Show_All_Shared_Ptr();

    int LessCharged();
    int LaunchTuxCli();
    operator bool();
    int operator()();
};

#endif