/**
 * @file
 *
 * The servers a user has added (#767): SSH folders and ECCE servers, kept
 * per user in one plain file, ~/.ECCE/AddedServers, one line per server:
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
    // Added ECCE servers of this process, and the login made on each.  The
    // session's own server is not one: its user stays Ecce::serverUser().
    // ref is EcceURL::getRef() (scheme, host, port), so two servers never
    // share a login.
    static string refOf(const string& address);
    static void markAdded(const string& ref);
    static void clearAdded();
    static bool isAdded(const string& ref);
    static void setLogin(const string& ref, const string& user);  // "" forgets
    static string loginFor(const string& ref);
    // The account whose home /Ecce/users/<account> is meant under url: an
    // added server's own login ("" before it), else Ecce::serverUser().
    static string userFor(const string& url);
    // The address of the added SSH folder that holds url ("" if none): the
    // key of its tree.  Its login is loginFor(address), set once connected.
    static string sshRootOf(const string& url);
    // Processes other than the Organizer (the job store, Launcher, Builder)
    // never see markAdded/setLogin.  They find an added server by its entry
    // in the user list and its login in the session credential store, which
    // AuthCache registers here (util cannot call tdat).
    typedef string (*LoginProvider)(const string& ref);
    static void setLoginProvider(LoginProvider p);
    static bool parseSsh(const string& address, string& user, string& host,
                         string& folder);
};

#endif
