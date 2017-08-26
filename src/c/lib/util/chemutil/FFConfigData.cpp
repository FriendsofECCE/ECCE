/**
 * @file
 *
 */

#include "util/FFConfigData.hpp"
#include "util/Preferences.hpp"

vector<string> FFConfigData::getFFFiles()
{
  return readFor("FF");
}



vector<string> FFConfigData::getSegDirs()
{
  return readFor("SEG");
}



vector<string> FFConfigData::readFor(string tag)
{
  vector<string> ret;

  Preferences * prefs = new Preferences("WxFFDataConfig");
  if (! prefs->isValid()) {
    delete prefs;
    prefs = new Preferences("Builder");
  }
  // default force field type to NWChem
  string ffname = "NWChem";
  prefs->getString("FFTYPE", ffname);

  string keytag = ffname + tag;
  if (prefs->getStringList(keytag, ret)) {
    // what to do if this fails?
  }

  return ret;
}

