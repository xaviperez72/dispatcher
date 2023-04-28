#ifndef CHECKER_PIDS_H_
#define CHECKER_PIDS_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <signal.h>
#include <sys/wait.h>

#ifdef  __cplusplus
}
#endif

#include "common.h"
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <functional>
#include <algorithm>
#include <atomic>         // std::atomic, std::atomic_flag, ATOMIC_FLAG_INIT

struct checker_struct 
{
    std::function<int()> _caller;
    time_t _last_fork{0};
    pid_t  _pid{0};
    std::string _procname;
};

class checker_pids {
    std::vector<checker_struct> _pids;
    checker_struct _dead;
    
    sighandler_t _previousInterruptHandler_int{nullptr};
    sighandler_t _previousInterruptHandler_usr1{nullptr};
    sighandler_t _previousInterruptHandler_term{nullptr};

public:
    
    static checker_pids *_me;    
    static std::atomic<bool> _keep_accepting;
    static std::atomic<bool> _keep_working;
    static std::atomic<bool> _forker;

    checker_pids() = default;
    ~checker_pids(){ LOG_DEBUG << "Destructor checker_pids"; }
    void add(std::function<int()> _call, std::string procname);
    static void sigterm_func(int s);
    void clear(){ _pids.clear(); }   // Temporal solution to clean up all shared_ptr to IPC's of Dispatcher Object
    void StoppingChildren();

    int operator()();
};

#endif