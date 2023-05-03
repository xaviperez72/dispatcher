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

void Prepare_Msg_Json_To_Send(protomsg::st_protomsg &v_protomsg, string msg, Json::Value &json_msg)
{
    json_msg["TERF"] = v_protomsg.terf;
    json_msg["TERL"] = v_protomsg.terl;
    json_msg["GUID"] = std::string(v_protomsg.guid);
    json_msg["PID"] = std::string(v_protomsg.pid);
    json_msg["AID"] = std::string(v_protomsg.aid);
    json_msg["CABX"] = std::string(v_protomsg.cabx);
    json_msg["MSG"] = msg;
}

int main()
{
    // TO DO 
    // Get level of log from file config. Now it is only DEBUG.
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    loglevel=plog::verbose;
    plog::init(plog::verbose, &consoleAppender);

    protomsg::st_protomsg v_protomsg;
    std::string msg_to_send, msgout;
    Json::Value json_msg;

    LOG_DEBUG << "CLIENT_main START!!";

    auto previousInterruptHandler_int = signal(SIGINT, sigterm_func);
    auto previousInterruptHandler_usr1 = signal(SIGUSR1, sigterm_func);
    auto previousInterruptHandler_term = signal(SIGTERM, sigterm_func);

    string ip = Socket::ipFromHostName("localhost"); //Get ip addres from hostname
    string port = "9000"; 
    Socket *sock = new Socket(AF_INET,SOCK_STREAM,0);  //AF_INET (Internet mode) SOCK_STREAM (TCP mode) 0 (Protocol any)

    sock->connect(ip, port); //Connect to localhost

    msg_to_send = "Well, this is the first message going and coming.";

    Prepare_Msg_Json_To_Send(v_protomsg, msg_to_send, json_msg);

    stringstream ss;
    ss << json_msg;
    ss >> msgout;

    LOG_DEBUG << "Sending msgout: " << msgout; 

    sock->socket_write(msgout);

    int seconds = 10;//Wait 10 second for response

    vector<Socket> reads(1);

    reads[0] = *sock;

    //Socket::select waits until sock reveives some input (for example the answer from google.com)
    if(sock->select(&reads, NULL, NULL, seconds) < 1)
    {
        //Something went wrong
        LOG_ERROR << "Error on select: " << errno << " : " << strerror(errno);
    }
    else
    {
        string buffer;
        sock->socket_read(buffer, 2048);//Read 1024 bytes of the answer
        LOG_DEBUG << buffer;
    }

    LOG_DEBUG << "Ending CLIENT_main";
    
    (void)signal(SIGINT, previousInterruptHandler_int);
    (void)signal(SIGUSR1, previousInterruptHandler_usr1);
    (void)signal(SIGTERM, previousInterruptHandler_term);

    return 0;
}
