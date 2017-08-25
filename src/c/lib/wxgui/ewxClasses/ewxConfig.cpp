#include "wx/fileconf.h"

#include "util/Ecce.H"
#include "util/CompareOps.H"
#include "wxgui/ewxConfig.H"


map<string, ewxConfig*, ltstring> ewxConfig::p_configs;


ewxConfig * ewxConfig::getConfig(const string& name)
{
  ewxConfig * ret = NULL;

  map<string,  ewxConfig*, ltstring>::iterator cur = p_configs.find(name);
  if (cur != p_configs.end())
    ret = cur->second;
  else {
    ret = new ewxConfig(_T("ECCE"), _T("PNNL"),
                        (Ecce::realUserPrefPath()+name).c_str());
    p_configs[name] = ret;
    
  }
  return ret;
}


/**
 * This function should be called at the end of application so that config
 * objects will be properly deleted. Also this will ensure configs are saved.
 */
void ewxConfig::closeConfigs()
{
  map<string, ewxConfig*, ltstring>::iterator iter;
  for(iter=p_configs.begin(); iter != p_configs.end(); ++iter) {
    delete iter->second;
  }
  p_configs.clear();
}


ewxConfig::ewxConfig()
{
}


ewxConfig::ewxConfig(const wxString& appName, const wxString& vendorName,
                     const wxString& localFilename,
                     const wxString& globalFilename,
                     long style, wxMBConv& conv):
  wxFileConfig(appName, vendorName, localFilename, globalFilename, style, conv)
{}


// implement single Write() overloads for the given type in terms of DoWrite()
#define IMPLEMENT_WRITE_FOR_TYPE(name, type)                                \
    bool ewxConfig::Write(const wxString& key, const type& value)           \
    {                                                                       \
        return DoWrite##name(key, value);                                   \
    }                                                                       \

IMPLEMENT_WRITE_FOR_TYPE(StringVector, vector<string>)

#undef IMPLEMENT_WRITE_FOR_TYPE


// implement both Read() overloads for the given type in terms of DoRead()
#define IMPLEMENT_READ_FOR_TYPE(name, type, deftype, extra)                 \
    bool ewxConfig::Read(const wxString& key, type *val)                 \
    {                                                                       \
        wxCHECK_MSG( val, false, _T("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        if ( !DoRead##name(key, val) )                                      \
            return false;                                                   \
                                                                            \
        *val = extra(*val);                                                 \
                                                                            \
        return true;                                                        \
    }                                                                       \
                                                                            \
    bool ewxConfig::Read(const wxString& key,                            \
                            type *val,                                      \
                            const deftype& defVal)                          \
    {                                                                       \
        wxCHECK_MSG( val, false, _T("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        bool read = DoRead##name(key, val);                                 \
        if ( !read )                                                        \
        {                                                                   \
            if ( IsRecordingDefaults() )                                    \
            {                                                               \
                ((ewxConfig *)this)->DoWrite##name(key, defVal);         \
            }                                                               \
                                                                            \
            *val = defVal;                                                  \
        }                                                                   \
                                                                            \
        *val = extra(*val);                                                 \
                                                                            \
        return read;                                                        \
    }

IMPLEMENT_READ_FOR_TYPE(StringVector, vector<string>, vector<string>, vector<string>)

#undef IMPLEMENT_READ_FOR_TYPE


bool ewxConfig::DoWriteStringVector(const wxString& key,
                                    const vector<string>& value)
{
  bool ret = true;

  for (int i = 0; i < value.size(); i++) {
    wxString stringIndex;
    stringIndex << key << "/" << i;
    ret &= Write(stringIndex, value[i]);
    if (!ret) break; // bail early if failure detected
  }

  return ret;
}


bool ewxConfig::DoReadStringVector(const wxString& key,
                                   vector<string>* value)
{
  bool ret = true;
  vector<string> returnValue;

  for (int i = 0; i >= 0; i++) { // infinite loop scenario, must break manually
    wxString stringIndex, stringValue;
    stringIndex << key << "/" << i;
    ret &= Read(stringIndex, &stringValue);
    if (!ret) break;
    returnValue.push_back(string(stringValue));
  }

  // always sets value parameter - can't figure out any other way to enumerate
  // config values since built-in enumeration functions weren't working as
  // documented
  *value = returnValue;

  // must always be true, given the above comment
  return true;
}

