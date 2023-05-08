#include "getcfgfile.h"

using namespace std;

string GetCfgFile::ExtractFileName(const string& s) 
{
   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}

string GetCfgFile::ExtractFileDir(const string& s) 
{
   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      return(s.substr(0, i));
   }

   return("");
}

GetCfgFile::GetCfgFile(std::string file, bool createdir)
        :_filename{file}
{
    if(createdir) 
    {
        _file_dir = ExtractFileDir(_filename);
        // ensure that .dispatch directory exists
        mkdir(_file_dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    }

    std::ifstream cfgfile(_filename);

    if (cfgfile.good())
    {
        try
        {
            cfgfile >> _m_json;
            cfgfile.close();

            _readed = true;

        }
        catch (std::exception const& exc) // Json::exception is std::exception in the reader.h
        {
            PLOG_ERROR << "Config file found: " << _filename << "\nbut it's corrupted: " << exc.what();
        }
    }
    else
    {
        PLOG_DEBUG_IF(loglevel) << "Config file not found: " << _filename << endl;
    }
}

GetCfgFile::operator bool() const 
{
    return _readed == true;
}


void GetCfgFile::save_cfg_file(const Json::Value json)
{
    _m_json = json;

    PLOG_DEBUG_IF(loglevel) << __func__ ;

    std::ofstream cfgfile_new(_filename);
    cfgfile_new << _m_json;
    cfgfile_new.flush();
    cfgfile_new.close();
    _saved = true;

    PLOG_DEBUG_IF(loglevel) << "Config file saved in " << _filename << ".";
}

