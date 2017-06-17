#ifndef _EWXCONFIG_H
#define _EWXCONFIG_H

#include <string>
using std::string;
#include <map>
using std::map;
#include <vector>
using std::vector;

#include "wx/fileconf.h"

#include "util/CompareOps.H"


class ewxConfig : public wxFileConfig
{
public:
  static ewxConfig * getConfig(const string & name);

  static void closeConfigs();

  using wxFileConfig::Write;
  bool Write(const wxString& key, const vector<string>& value);

  using wxFileConfig::Read;
  bool Read(const wxString& key, vector<string>* value);
  bool Read(const wxString& key, vector<string>* value,
            const vector<string>& defaultValue);

protected:
  ewxConfig();

  ewxConfig(const wxString& appName = wxEmptyString,
            const wxString& vendorName = wxEmptyString,
            const wxString& localFilename = wxEmptyString,
            const wxString& globalFilename = wxEmptyString,
            long style = 0,
            wxMBConv& conv = wxConvUTF8);

  bool DoWriteStringVector(const wxString& key, const vector<string>& value);

  bool DoReadStringVector(const wxString& key, vector<string>* value);

private:
  static map<string, ewxConfig*, ltstring> p_configs;
};

#endif
