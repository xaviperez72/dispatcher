/***************************************
 * GLOBAL VARS - IN COMMON.H
 ***************************************/
#if !defined LOGLEVEL
#define LOGLEVEL
int loglevel;
#endif

#include <memory>
#include <map>
#include <json/json.h>

#include "common.h"
#include "ipclib.h"
#include "getcfgfile.h"
#include "dispatch_cfg.h"
#include "dispatcher.h"
#include "Socket.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Initializers/ConsoleInitializer.h"

using namespace std;

using namespace std::string_literals;
auto const CFGFILE_PATH = getpwuid(getuid()) -> pw_dir + "/.dispatch/tuxconfig.json"s;

auto keep_accepting = true;

void sigterm_func(int s) 
{
    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " keep_accepting " << keep_accepting;

    keep_accepting = false;

    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " keep_accepting " << keep_accepting;
}

int main()
{
    // TO DO 
    // Get level of log from file config. Now it is only DEBUG.
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    loglevel=plog::verbose;
    plog::init(plog::verbose, &consoleAppender);

    GetCfgFile f_config(CFGFILE_PATH,true);  // Create directory if it doesn't exist.

    LOG_DEBUG << "tuxcli_main START!!";

    auto previousInterruptHandler_int = signal(SIGINT, sigterm_func);
    auto previousInterruptHandler_usr1 = signal(SIGUSR1, sigterm_func);
    auto previousInterruptHandler_term = signal(SIGTERM, sigterm_func);

    while(keep_accepting) 
    {
        sleep(5);
        LOG_DEBUG << "tuxcli_main";
    }

    LOG_DEBUG << "Ending tuxcli_main";
    
    (void)signal(SIGINT, previousInterruptHandler_int);
    (void)signal(SIGUSR1, previousInterruptHandler_usr1);
    (void)signal(SIGTERM, previousInterruptHandler_term);

    return 0;
}
