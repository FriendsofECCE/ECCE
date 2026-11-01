/**
 * @file
 *
 * The servers a user has added (#767): SSH folders and ECCE servers, kept
 * per user in one plain file, ~/.ECCE/ServerList, one line per server:
 *
 *     <type><TAB><name><TAB><address>
 *
 * type is "ssh" (address sftp://user@host/folder) or "ecce" (address
 * https://host[:port]).  wx-free; Edit > Preferences > Data lists them.
 * Test and script hook: `ecce-localdata servers ...`.
 */
#ifndef SERVERLIST_HH
#define SERVERLIST_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

struct ServerEntry
{
  string type;      // "ssh" or "ecce"
  string name;
  string address;
};

class ServerList
{
  public:
    static string file();
    static vector<ServerEntry> load();
    static bool save(const vector<ServerEntry>& list);

    // "" when added; else a sentence for the user (nothing is changed).
    static string add(const ServerEntry& e);
    // Replaces the entry called oldName; the same checks as add().
    static string replace(const string& oldName, const ServerEntry& e);
    static bool remove(const string& name);

    // "" when the entry is acceptable, else what is wrong with it.
    static string check(const ServerEntry& e);

    static string typeLabel(const string& type);   // "SSH folder", "ECCE server"
    // sftp://user@host/folder, from its parts; and back.
    static string sshAddress(const string& user, const string& host,
                             const string& folder);
    static bool parseSsh(const string& address, string& user, string& host,
                         string& folder);
};

#endif
