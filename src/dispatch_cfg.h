#ifndef DISPATCH_CFG_H
#define DISPATCH_CFG_H

#include "common.h"
#include <vector>
#include <json/json.h>
#include <json/value.h>

using namespace std::string_literals;
auto const DEFAULT_IP = "127.0.0.1"s;
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

/**
 * Dispatcher configuration 
 */
struct dispatch_cfg {
    int NumDispatch;                // Number of dispatcher
    std::string IP;                 // IP of dispatcher
    int Port;                       // Port of dispatcher
    int NumThreads;                 // Number of threads pair to run concurrently
    std::string TuxCliProg;         // Transactional Client program.
    std::string TuxCliSetup;        // Transactional Client configuration file. 
    int LogLevel;                   // Level of log.
    int MaxConnections;             // Number of max connections.
    int StopTimeout;                // Wait until there are no st_ready running on connection array. 
    std::string IpcFile;            // IPC file path for this dispatcher.

    friend std::ostream& operator<<( std::ostream& os, const dispatch_cfg& v )
    {
	    os << v.NumDispatch << ":" << v.IP << ":" << v.Port << ":" << v.NumThreads << ":" << v.TuxCliProg << ":" << \
            v.TuxCliSetup << ":" << v.LogLevel << ":" << v.MaxConnections << ":" << v.StopTimeout << ":" << v.IpcFile;
	    return os;
    }
};

/**
 * Class of all Dispatchers configuration. 
 */
class all_dispatch_cfg final
{
    Json::Value _m_json;
    int NumDispatchers{0};                              // Number of Dispatchers
    std::vector<dispatch_cfg> dispatchers_cfg;          // Vector of all configuration dispatcher
    bool loaded{false};                                 // true=loaded false=not loaded
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
    all_dispatch_cfg() = delete;
    explicit all_dispatch_cfg(const Json::Value json):_m_json{json}{ load_all_info();}
    all_dispatch_cfg(all_dispatch_cfg const &) = delete;
    all_dispatch_cfg(all_dispatch_cfg &&) = delete;
    all_dispatch_cfg& operator=(all_dispatch_cfg const &) = delete;
    all_dispatch_cfg& operator=(all_dispatch_cfg &&) = delete;
    ~all_dispatch_cfg() = default;
    
    // Gets all json values
    const Json::Value &get_json() const { return _m_json;}
    // Load all configuration file
    void load_all_info();
    // Creates a default dispatcher configuration file 
    void create_cfg_values(std::string file_dir);
    // Returns true if configuration file is correctly loaded, false otherwise. 
    operator bool() const {return loaded == true;}
    // Log all configuration file 
    void show_all_config();
    // Gets number of dispatchers loaded on configuration file
    int get_num_dispatchers() const { return NumDispatchers;}
    // Gets dispatch_cfg struct vector. 
    std::vector<dispatch_cfg> &get_all_dispatch_info(){ return dispatchers_cfg;}
};

#endif