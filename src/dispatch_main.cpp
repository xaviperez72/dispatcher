/***************************************
 * GLOBAL VARS - IN COMMON.H
 ***************************************/
#if !defined LOGLEVEL
#define LOGLEVEL
int loglevel;
#endif

#include "common.h"
#include "ipclib.h"
#include "getcfgfile.h"
#include "dispatch_cfg.h"
#include "dispatcher.h"
#include "checker_pids.h"
#include "Socket.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Initializers/ConsoleInitializer.h"

using namespace std;

int RunOnBackground() 
{
    if (getppid() != 1)     /* parent != init */
    { 
        setsid();
        switch(fork()) 
        {
            case 0:
                close(STDIN_FILENO);   
                return 0;
            case -1:
                LOG_ERROR << "fork background: " << strerror(errno);
                return -1;
            default:
                exit(0);
        }
    }
    return 0;
}

using namespace std::string_literals;
auto const CFGFILE_PATH = getpwuid(getuid()) -> pw_dir + "/.dispatch/config.json"s;


// It helps to realize about COREDUMPS
sighandler_t _previousInterruptHandler_sigsegv;
void sigsegv_func(int s) 
{
    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << " ERROR: " << strerror(errno);
    (void)signal(SIGSEGV, _previousInterruptHandler_sigsegv);
}

int main()
{
    // TO DO 
    // Get level of log from file config. Now it is only DEBUG.
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    loglevel=plog::verbose;
    plog::init(plog::verbose, &consoleAppender);

    _previousInterruptHandler_sigsegv = signal(SIGSEGV, sigsegv_func);

    GetCfgFile f_config(CFGFILE_PATH,true);  // Create directory if it doesn't exist.

    all_dispatch_cfg cfg(f_config.get_json());

    // Error reading config file or bad json format
    if(!f_config || !cfg)
    {
        cfg.create_cfg_values(f_config.get_file_dir());
        f_config.save_cfg_file(cfg.get_json());
        cfg.load_all_info();
    }
    
    cfg.show_all_config();

    if( RunOnBackground() < 0 )
    {
        LOG_ERROR << "Can not run on background!";
        return -1;
    }
    LOG_DEBUG << "Running on background, parent pid = " << getppid();

    // Object that perform fork for each functor on its vector. 
    checker_pids checking_pids;
    
    std::shared_ptr<checker_pids> shared_pids(&checking_pids, [](checker_pids *){});

    // Config every dispatcher:

    vector<Dispatcher> v_dispatchers;
    v_dispatchers.reserve(cfg.get_num_dispatchers());

    for(auto config : cfg.get_all_dispatch_info())
    {
       v_dispatchers.emplace_back(config,shared_pids);
    }

    // Prepare for fork() and checking
    for(auto dispat : v_dispatchers)
    {
        checking_pids.add(dispat,"Dispatcher");
    }

    // Launching all dispatchers... 
    int ret=checking_pids();

    LOG_DEBUG << "Ending process main";
    
    return ret;
}
