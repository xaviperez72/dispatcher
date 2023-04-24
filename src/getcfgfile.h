#ifndef GETCFGFILE_H
#define GETCFGFILE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#ifdef  __cplusplus
}
#endif

#ifdef _WIN32
   constexpr char sep = '\\';
#else
   constexpr char sep = '/';
#endif

#include "common.h"
#include <vector>
#include <json/json.h>
#include <json/value.h>
#include <fstream>

class GetCfgFile 
{
public:
    GetCfgFile(std::string file, bool createdir = false);
    ~GetCfgFile() = default;

    void save_cfg_file(Json::Value json);
    Json::Value &get_json(){ return _m_json;};
    std::string get_file_dir() {return _file_dir;};
    std::string get_file_name() {return _filename;};
    operator bool() const;

private:
    std::string ExtractFileName(const std::string& s);
    std::string ExtractFileDir(const std::string& s); 

    std::string _filename;
    std::string _file_dir;
    bool _readed{false};
    bool _saved{false};
    Json::Value _m_json;
};

#endif
