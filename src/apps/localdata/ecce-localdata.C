/*
 * ecce-localdata                 print the session's local data folder,
 *                                or nothing for a data server (#216)
 * ecce-localdata move FROM TO    move a data folder (LocalData::move);
 *                                exit 0 moved, 2 target not empty,
 *                                3 in use, 1 failed
 * ecce-localdata in-use DIR      exit 0 if a process holds DIR
 *
 * The wrappers use the first form to decide, once per session, whether
 * a data server is wanted; the others are for tests and for scripts.
 */
#include <iostream>
#include <string>
#include "util/LocalData.H"

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
  if (cmd == "in-use" && argc == 3) return LocalData::inUse(argv[2]) ? 0 : 1;
  std::cerr << "usage: ecce-localdata [move FROM TO | in-use DIR]" << std::endl;
  return 64;
}
