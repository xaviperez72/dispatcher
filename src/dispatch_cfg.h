#ifndef DISPATCH_CFG_H
#define DISPATCH_CFG_H

#include "common.h"
#include <vector>
#include <json/json.h>
#include <json/value.h>

using namespace std::string_literals;
auto const DEFAULT_IP = "127.0.0.1";
auto const DEFAULT_PORT = 9000; 
auto const DEFAULT_NUMTHREADS = 20;
auto const DEFAULT_TUXCLIPROG = "/TuxCliProg"s;
auto const DEFAULT_TUXCLISETUP = "/TuxCliSetup"s;
auto const DEFAULT_MAXCONN = 1500;
auto const DEFAULT_STOPTIMEOUT = 30;
auto const DEFAULT_IPCFILE = "/ipcfile_"s;

auto const FIELD_NOTE = "1_Note"s;
auto const FIELD_VERSION = "1_Version"s;
auto const FIELD_NUMDISPATCHERS = "2_NumDispatchers"s;
auto const FIELD_IP = "IP"s;
auto const FIELD_PORT = "Port"s;
auto const FIELD_NUMTHREADS = "NumThreads"s;
auto const FIELD_TUXCLIPROG = "TuxCliProg"s;
auto const FIELD_TUXCLISETUP = "TuxCliSetup"s;
auto const FIELD_LOGLEVEL = "LogLevel"s;
auto const FIELD_MAXCONN = "MaxConnections"s;
auto const FIELD_STOPTIMEOUT = "StopTimeout"s;
auto const FIELD_IPCFILE = "IpcFile"s;

static constexpr int CFGFILE_VERSION = 1;

struct dispatch_cfg {
    int NumDispatch;
    std::string IP;
    int Port;
    int NumThreads;
    std::string TuxCliProg;
    std::string TuxCliSetup;
    int LogLevel;
    int MaxConnections;
    int StopTimeout;
    std::string IpcFile;

    friend std::ostream& operator<<( std::ostream& os, const dispatch_cfg& v )
    {
	    os << v.NumDispatch << ":" << v.IP << ":" << v.Port << ":" << v.NumThreads << ":" << v.TuxCliProg << ":" << \
            v.TuxCliSetup << ":" << v.LogLevel << ":" << v.MaxConnections << ":" << v.StopTimeout << ":" << v.IpcFile;
	    return os;
    }
};

class all_dispatch_cfg {
    Json::Value _m_json;
    int NumDispatchers{0};
    std::vector<dispatch_cfg> dispatchers_cfg;
    bool loaded{false};
    int         getNumDispatchers() const;
    const char* getDispatcherXX_IP(const std::string dispatchXX) const;
    int         getDispatcherXX_Port(const std::string dispatchXX) const;
    int         getDispatcherXX_NumThreads(const std::string dispatchXX) const;
    const char* getDispatcherXX_TuxCliProg(const std::string dispatchXX) const;
    const char* getDispatcherXX_TuxCliSetup(const std::string dispatchXX) const;
    int         getDispatcherXX_LogLevel(const std::string dispatchXX) const;
    int         getDispatcherXX_MaxConnections(const std::string dispatchXX) const;
    int         getDispatcherXX_StopTimeout(const std::string dispatchXX) const;
    const char* getDispatcherXX_IpcFile(const std::string dispatchXX) const;
public:
    explicit all_dispatch_cfg(Json::Value json):_m_json{json}{ load_all_info();}
    Json::Value &get_json(){ return _m_json;}
    void load_all_info();
    void create_cfg_values(std::string file_dir);
    operator bool() const {return loaded == true;}
    void show_all_config();
    int get_num_dispatchers() const { return NumDispatchers;}
    std::vector<dispatch_cfg> &get_all_dispatch_info(){ return dispatchers_cfg;}
};

#endif