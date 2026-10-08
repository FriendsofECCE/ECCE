/*
 * ecce-localdata                 print the session's local data folder,
 *                                or nothing for a data server (#216)
 * ecce-localdata move FROM TO    move a data folder (LocalData::move);
 *                                exit 0 moved, 2 target not empty,
 *                                3 in use, 1 failed
 * ecce-localdata in-use DIR      exit 0 if a process holds DIR
 * ecce-localdata pref on|off     set the "local data folder" preference
 *                                (ecce-first-start, #240)
 * ecce-localdata pref-state      print on, off or unset
 *
 * The wrappers use the first form to decide, once per session, whether
 * a data server is wanted; the others are for tests and for scripts.
 */
#include <iostream>
#include <string>
#include "util/LocalData.H"
#include "util/PreferenceLabels.H"
#include "util/Preferences.H"

int main(int argc, char **argv)
{
  std::string cmd = argc > 1 ? argv[1] : "";
  if (cmd.empty()) {
    std::cout << LocalData::dir() << std::endl;
    return 0;
  }
  if (cmd == "move" && argc == 4) {
    std::string msg;
    LocalData::MoveResult r = LocalData::move(argv[2], argv[3], msg);
    if (!msg.empty()) std::cout << msg << std::endl;
    switch (r) {
      case LocalData::MOVED: case LocalData::SAME: return 0;
      case LocalData::TARGET_NOT_EMPTY: return 2;
      case LocalData::IN_USE: return 3;
      default: return 1;
    }
  }
  if (cmd == "pref" && argc == 3) {
    std::string v = argv[2];
    if (v != "on" && v != "off") return 64;
    LocalData::setPref(v == "on", LocalData::prefFolder(), LocalData::prefMoveTo());
    return 0;
  }
  if (cmd == "pref-state") {
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    bool on = false;
    std::cout << (pref.getBool(PrefLabels::LOCALDATA, on) ? (on ? "on" : "off")
                                                           : "unset") << std::endl;
    return 0;
  }
  if (cmd == "in-use" && argc == 3) return LocalData::inUse(argv[2]) ? 0 : 1;
  std::cerr << "usage: ecce-localdata [move FROM TO | in-use DIR | pref on|off | pref-state]" << std::endl;
  return 64;
}
