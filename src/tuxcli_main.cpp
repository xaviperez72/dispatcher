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
#include "checker_pids.h"
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

checker_pids chk_procs;

class TuxClient 
{
    int idx_cli{0};
    MessageQueue &common_queue;
public:
    TuxClient(int idx, MessageQueue &queue):idx_cli{idx},common_queue{queue}{}
    int operator()();
};

int TuxClient::operator()()
{
    LOG_DEBUG << "Running TuxClient " << idx_cli;

    while(chk_procs._keep_accepting.load()) 
    {
        std::string msgin;
        protomsg::st_protomsg v_protomsg;

        common_queue.rcv(&v_protomsg, msgin);

        if(keep_accepting) 
        {
            LOG_DEBUG << idx_cli << " Received msg: " << v_protomsg;

            msgin += " OK - TX DONE!! ";
            sleep(1);

            LOG_DEBUG << idx_cli << " TX EXECUTED OK!! ";

            MessageQueue msg_resp(v_protomsg.q_write);
        
            if(msg_resp) {
                msg_resp.send(&v_protomsg,msgin);
            }
        }
    }

    LOG_DEBUG << "Ending TuxClient " << idx_cli;

    return 0;
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


    // Getting configs files (IPCS and tuxclisetup ) .....

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

    const Json::Value ipcs_json = ipcfile.get_json();

    // We just need common queue id:
    int msg_common_id = ipcs_json["msg_common_id"].asInt();

    MessageQueue common_queue(msg_common_id);
    if(!common_queue)
    {
        LOG_ERROR << "Common Queue error: " << msg_common_id;
        exit(1);
    }

    // LoadIPTXTable - Load all allowed transaction ID on Shared Memory
    
    // LoadTRMLTable - Load all allowed terminal ID on Shared Memory


    auto previousInterruptHandler_sigsegv = signal(SIGSEGV, sigterm_func);


    // Launching all processess...

    const Json::Value setup_json = tuxclisetup.get_json();

    int min_tux_cli = setup_json["min_tux_cli"].asInt();
    int max_tux_cli = setup_json["max_tux_cli"].asInt();
    int xml_n_term  = setup_json["xml_n_term"].asInt();

    for(int i=0; i < min_tux_cli; i++) {
        TuxClient tux(i,common_queue);
        chk_procs.add(tux,"TuxClient");
    }

    // Launching all processes... 
    int ret=chk_procs();

    LOG_DEBUG << "Ending tuxcli_main";
    
    (void)signal(SIGSEGV, previousInterruptHandler_sigsegv);

    return 0;
}
