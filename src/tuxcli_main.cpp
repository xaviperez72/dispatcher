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

auto keep_accepting = true;

void sigterm_func(int s) 
{
    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " keep_accepting " << keep_accepting;

    keep_accepting = false;

    LOG_DEBUG << "Received signal " << s << " : " << strsignal(s) << \
                 " keep_accepting " << keep_accepting;
}

int main(int argc, char **argv)
{
    // TO DO 
    // Get level of log from file config. Now it is only DEBUG.
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    loglevel=plog::verbose;
    plog::init(plog::verbose, &consoleAppender);

    if (argc != 3) 
    {
        LOG_ERROR << "Uso: " << argv[0] << " <IpcFile> <TuxCliSetup> \n";
        exit(1);
    }

    LOG_DEBUG << "tuxcli_main START!!";

    std::string ipcfile_str(argv[1]);
    std::string tuxclisetup_str(argv[2]);

    GetCfgFile ipcfile(ipcfile_str);
    GetCfgFile tuxclisetup(tuxclisetup_str);

    if(!ipcfile || !tuxclisetup)
    {
        if(!ipcfile)
            LOG_ERROR << "File error: " << ipcfile_str;
        else
            LOG_ERROR << "File error: " << tuxclisetup_str;
        exit(1);
    }

    Json::Value ipcs_json = ipcfile.get_json();

    // We just need common queue id:
    int msg_common_id = ipcs_json["msg_common_id"].asInt();

    MessageQueue common_queue(msg_common_id);
    if(!common_queue)
    {
        LOG_ERROR << "Common Queue error: " << msg_common_id;
        exit(1);
    }

    auto previousInterruptHandler_int = signal(SIGINT, sigterm_func);
    auto previousInterruptHandler_usr1 = signal(SIGUSR1, sigterm_func);
    auto previousInterruptHandler_term = signal(SIGTERM, sigterm_func);

    while(keep_accepting) 
    {
        std::string msgin;
        protomsg::st_protomsg v_protomsg;

        common_queue.rcv(&v_protomsg, msgin);

        if(keep_accepting) 
        {
            LOG_DEBUG << "Received msg: " << v_protomsg;

            msgin += " OK - TX DONE!! ";
            sleep(1);

            LOG_DEBUG << "TX EXECUTED OK!! ";

            MessageQueue msg_resp(v_protomsg.q_write);
        
            if(msg_resp) {
                msg_resp.send(&v_protomsg,msgin);
            }
        }
    }

    LOG_DEBUG << "Ending tuxcli_main";
    
    (void)signal(SIGINT, previousInterruptHandler_int);
    (void)signal(SIGUSR1, previousInterruptHandler_usr1);
    (void)signal(SIGTERM, previousInterruptHandler_term);

    return 0;
}
