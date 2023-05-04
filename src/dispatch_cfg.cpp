#include "dispatch_cfg.h"

using namespace std;

void all_dispatch_cfg::load_all_info()
{
    int num_disp = getNumDispatchers();
    
    PLOG_DEBUG_IF(loglevel) << "Total dispatchers " << num_disp;

    for(int i=1; i < (num_disp+1); i++) 
    {
        std::string dispatchXX;

        if(i<10)
            dispatchXX=string("dispatch0") + to_string(i);
        else
            dispatchXX=string("dispatch") + to_string(i);

        auto IP=getDispatcherXX_IP(dispatchXX.c_str());
        auto Port=getDispatcherXX_Port(dispatchXX.c_str());
        auto NumThreads=getDispatcherXX_NumThreads(dispatchXX.c_str());
        auto TuxCliProg=getDispatcherXX_TuxCliProg(dispatchXX.c_str());
        auto TuxCliSetup=getDispatcherXX_TuxCliSetup(dispatchXX.c_str());
        auto LogLevel=getDispatcherXX_LogLevel(dispatchXX.c_str());
        auto MaxConnections=getDispatcherXX_MaxConnections(dispatchXX.c_str());
        auto StopTimeout=getDispatcherXX_StopTimeout(dispatchXX.c_str());
        auto IpcFile=getDispatcherXX_IpcFile(dispatchXX.c_str());
        dispatch_cfg d={i,IP,Port,NumThreads,TuxCliProg,TuxCliSetup,LogLevel,MaxConnections,StopTimeout,IpcFile};
        dispatchers_cfg.emplace_back(d);
        PLOG_DEBUG_IF(loglevel) << "Processed " << num_disp; 
        loaded=true;
        NumDispatchers = i;
    }

}

void all_dispatch_cfg::create_cfg_values(string file_dir)
{
    PLOG_DEBUG_IF(loglevel) << "Creating config file...";
    _m_json.clear();
    const std::string dispatch01{"dispatch01"};
    //auto const dispatch01 = "dispatch01";

    _m_json[FIELD_NOTE]    = "Default Configuration File (auto-created)";
    _m_json[FIELD_VERSION] = CFGFILE_VERSION;
    _m_json[FIELD_NUMDISPATCHERS] = 1;
    _m_json[dispatch01.c_str()][FIELD_IP] = DEFAULT_IP;
    _m_json[dispatch01.c_str()][FIELD_PORT] = DEFAULT_PORT;
    _m_json[dispatch01.c_str()][FIELD_NUMTHREADS] = DEFAULT_NUMTHREADS;
    _m_json[dispatch01.c_str()][FIELD_TUXCLIPROG] = file_dir + DEFAULT_TUXCLIPROG;
    _m_json[dispatch01.c_str()][FIELD_TUXCLISETUP] = file_dir + DEFAULT_TUXCLISETUP;
    _m_json[dispatch01.c_str()][FIELD_LOGLEVEL] = plog::warning;
    _m_json[dispatch01.c_str()][FIELD_MAXCONN] = DEFAULT_MAXCONN;
    _m_json[dispatch01.c_str()][FIELD_STOPTIMEOUT] = DEFAULT_STOPTIMEOUT;             
    _m_json[dispatch01.c_str()][FIELD_IPCFILE] = file_dir + DEFAULT_IPCFILE + dispatch01;
}

int all_dispatch_cfg::getNumDispatchers() const
{
    return _m_json["2_NumDispatchers"].asInt();
}

const char* all_dispatch_cfg::getDispatcherXX_IP(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["IP"].asCString();
}

int all_dispatch_cfg::getDispatcherXX_Port(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["Port"].asInt();
}

int all_dispatch_cfg::getDispatcherXX_NumThreads(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["NumThreads"].asInt();
}

const char* all_dispatch_cfg::getDispatcherXX_TuxCliProg(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["TuxCliProg"].asCString();
}

const char* all_dispatch_cfg::getDispatcherXX_TuxCliSetup(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["TuxCliSetup"].asCString();
}

int all_dispatch_cfg::getDispatcherXX_LogLevel(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["LogLevel"].asInt();
}

int all_dispatch_cfg::getDispatcherXX_MaxConnections(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["MaxConnections"].asInt();
}

int all_dispatch_cfg::getDispatcherXX_StopTimeout(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["StopTimeout"].asInt();
}

const char* all_dispatch_cfg::getDispatcherXX_IpcFile(const std::string dispatchXX) const
{
    return _m_json[dispatchXX]["IpcFile"].asCString();
}

void all_dispatch_cfg::show_all_config()
{
    PLOG_DEBUG  << "----------------------------------------------------------";
    PLOG_DEBUG  << "CONFIG FILE: ";
    for(auto v : dispatchers_cfg)
    {
        PLOG_DEBUG  << v;
    }
    PLOG_DEBUG  << "----------------------------------------------------------";
}