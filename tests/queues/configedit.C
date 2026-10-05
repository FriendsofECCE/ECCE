// Command-line front end to ConfigFile, MachineConfigDraft and MiniJson for
// tests/queues/configedit_test.py.
//
//   configedit file  FILE (set KEY VALUE | remove KEY | clear KEY)...
//       edit FILE in place, other lines untouched; "load, save" with no
//       operations rewrites it unchanged.  Prints the file's warnings.
//   configedit check FILE         print the grammar warnings for FILE
//   configedit json               stdin: one JSON value per line; prints
//       each as MiniJson::dump() text
//   configedit draft --mode user|admin|remote --site F --user F
//       --explain JSONFILE [--merged MACHINE] [--write]
//       (set KEY VALUE | remove KEY | clear KEY)...
//       load the explain output into the draft, apply the edits to the
//       draft, print one JSON line per key (tag, effective value, and the
//       value C++ alone would see), and with --write apply the changed keys
//       to the edited file.  --merged first checks the draft against
//       RefMachine::config(MACHINE) for the C++ keys.
#include <fstream>
#include <iostream>
#include <sstream>

#include "tdat/ConfigFile.H"
#include "tdat/MachineConfigDraft.H"
#include "tdat/RefMachine.H"
#include "util/MiniJson.H"

using std::string;
using std::vector;

static int usage()
{
  std::cerr << "usage: configedit file|check|json|draft ...  (see the "
               "header of tests/queues/configedit.C)" << std::endl;
  return 2;
}

static string jstr(const string& s)
{
  MiniJson v;
  string err, text = "[\"";
  // Let MiniJson do the escaping: a string it cannot represent is a bug.
  for (size_t i = 0; i < s.size(); i++) {
    unsigned char c = s[i];
    char buf[8];
    if (c == '"' || c == '\\') { text += '\\'; text += (char)c; }
    else if (c < 0x20) {
      snprintf(buf, sizeof buf, "\\u%04x", c);
      text += buf;
    } else text += (char)c;
  }
  text += "\"]";
  if (!MiniJson::parse(text, v, err))
    return "null";
  return v.items()[0].dump();
}

static bool slurp(const string& path, string& out)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in)
    return false;
  std::ostringstream b;
  b << in.rdbuf();
  out = b.str();
  return true;
}

static int fileCmd(int argc, char** argv)
{
  if (argc < 3)
    return usage();
  ConfigFile f;
  if (!f.load(argv[2])) {
    std::cerr << "cannot read " << argv[2] << std::endl;
    return 1;
  }
  for (int i = 3; i < argc; ) {
    string op = argv[i];
    string err;
    if (op == "set" && i + 2 < argc) {
      if (!f.set(argv[i + 1], argv[i + 2], &err)) {
        std::cout << "REFUSED " << argv[i + 1] << ": " << err << std::endl;
        return 3;
      }
      i += 3;
    } else if (op == "remove" && i + 1 < argc) {
      f.remove(argv[i + 1]); i += 2;
    } else if (op == "clear" && i + 1 < argc) {
      f.clear(argv[i + 1]); i += 2;
    } else {
      return usage();
    }
  }
  string err;
  if (!f.save(&err)) {
    std::cerr << err << std::endl;
    return 1;
  }
  return 0;
}

static int checkCmd(int argc, char** argv)
{
  if (argc != 3)
    return usage();
  ConfigFile f;
  if (!f.load(argv[2]))
    return 1;
  for (size_t i = 0; i < f.warnings().size(); i++)
    std::cout << f.warnings()[i] << std::endl;
  return 0;
}

static int jsonCmd()
{
  string line;
  while (std::getline(std::cin, line)) {
    MiniJson v;
    string err;
    if (!MiniJson::parse(line, v, err)) {
      std::cout << "ERROR " << err << std::endl;
      continue;
    }
    std::cout << v.dump() << std::endl;
  }
  return 0;
}

static int draftCmd(int argc, char** argv)
{
  string mode = "user", site, user, explainFile, merged;
  bool write = false;
  int i = 2;
  for (; i < argc && argv[i][0] == '-'; ) {
    string a = argv[i];
    if (a == "--write") { write = true; i++; }
    else if (i + 1 >= argc) return usage();
    else if (a == "--mode") { mode = argv[i + 1]; i += 2; }
    else if (a == "--site") { site = argv[i + 1]; i += 2; }
    else if (a == "--user") { user = argv[i + 1]; i += 2; }
    else if (a == "--explain") { explainFile = argv[i + 1]; i += 2; }
    else if (a == "--merged") { merged = argv[i + 1]; i += 2; }
    else return usage();
  }
  MachineConfigDraft::Mode m = mode == "admin" ? MachineConfigDraft::AdminMode
      : mode == "remote" ? MachineConfigDraft::RemoteMode
      : MachineConfigDraft::UserMode;
  MachineConfigDraft d(m, site, user);
  string text, err;
  if (!slurp(explainFile, text)) {
    std::cerr << "cannot read " << explainFile << std::endl;
    return 1;
  }
  if (!d.loadExplain(text, err)) {
    std::cerr << err << std::endl;
    return 1;
  }
  int rc = 0;
  if (!merged.empty()) {
    vector<string> bad = d.compareMerged(RefMachine::config(merged),
                                         MachineConfigDraft::cppKeys());
    for (size_t k = 0; k < bad.size(); k++)
      std::cout << "MISMATCH " << bad[k] << std::endl;
    if (!bad.empty())
      rc = 1;
  }
  for (; i < argc; ) {
    string op = argv[i];
    if (op == "set" && i + 2 < argc) {
      d.setValue(argv[i + 1], argv[i + 2]); i += 3;
    } else if (op == "remove" && i + 1 < argc) {
      d.useInherited(argv[i + 1]); i += 2;
    } else if (op == "clear" && i + 1 < argc) {
      if (!d.clear(argv[i + 1]))
        std::cout << "REFUSED clear " << argv[i + 1] << std::endl;
      i += 2;
    } else {
      return usage();
    }
  }
  std::cout << "dirty " << (d.isDirty() ? 1 : 0) << std::endl;
  vector<string> keys = d.keys();
  for (size_t k = 0; k < keys.size(); k++) {
    string v, c;
    bool a = d.effective(keys[k], v), b = d.effective(keys[k], c, true);
    std::cout << "{\"cpp\":" << (b ? jstr(c) : "null") << ",\"effective\":"
              << (a ? jstr(v) : "null") << ",\"key\":" << jstr(keys[k])
              << ",\"tag\":" << jstr(MachineConfigDraft::tagText(
                                         d.tag(keys[k]))) << "}" << std::endl;
  }
  if (write) {
    ConfigFile f;
    if (!f.load(d.editedFile()) || !d.applyTo(f, err) || !f.save(&err)) {
      std::cout << "WRITE FAILED " << err << std::endl;
      return 3;
    }
    d.markSaved();
  }
  return rc;
}

int main(int argc, char** argv)
{
  if (argc < 2)
    return usage();
  string cmd = argv[1];
  if (cmd == "file") return fileCmd(argc, argv);
  if (cmd == "check") return checkCmd(argc, argv);
  if (cmd == "json") return jsonCmd();
  if (cmd == "draft") return draftCmd(argc, argv);
  return usage();
}
