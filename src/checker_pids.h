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
#include <atomic>

/**
 * Process struct 
 */
struct checker_struct 
{
    std::function<int()> _caller;       // Functor callback
    time_t _last_fork{0};               // Time of last fork
    pid_t  _pid{0};                     // pid of the process
    std::string _procname;              // Process name
};

/**
 * Creates processes (fork) and keep them alive until signal is captured. 
 */
class checker_pids final {
    std::vector<checker_struct> _pids;      // Vector of processes
    checker_struct _dead;                   // Gets dead process on wait()
    
    sighandler_t _previousInterruptHandler_int{nullptr};    // Signal handler for SIGINT
    sighandler_t _previousInterruptHandler_usr1{nullptr};   // Signal handler for SIGUSR1
    sighandler_t _previousInterruptHandler_term{nullptr};   // Signal handler for SIGTERM

public:
    
    static checker_pids *_me;                               // Pointer to an existing instance.
    static std::atomic<bool> _keep_accepting;               // When true keeps processes running.
    static std::atomic<bool> _keep_working;                 // When true keeps processes running.
    static std::atomic<bool> _forker;                       // Identifies which process is the forker.
    
    // Static signal handler.
    static void sigterm_func(int s);

    checker_pids() = default;
    checker_pids(checker_pids const &) = delete;
    checker_pids(checker_pids &&) = delete;
    checker_pids& operator=(checker_pids const &) = delete;
    checker_pids& operator=(checker_pids &&) = delete;

    ~checker_pids(){ LOG_DEBUG << "Destructor checker_pids"; }

    // Adds a new process (functor callback and process name)
    void add(std::function<int()> _call, std::string procname);

    // Clears all _pics vector.
    void clear(){ _pids.clear(); }   // Temporal solution to clean up all shared_ptr to IPC's of Dispatcher Object

    // Sends a SIGUSR1 to children processes. 
    void StoppingChildren();

    // Launch and keep working all processes. 
    int operator()();
};

#endif