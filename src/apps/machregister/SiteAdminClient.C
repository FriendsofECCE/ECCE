/**
 *  @file
 *
 *  See SiteAdminClient.H.  Nothing the administrator typed is ever part of a
 *  command: the request travels as a file, and the only command text is the
 *  fixed lines below.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fstream>
#include <sstream>
#include <vector>

#include "util/Ecce.H"
#include "tdat/SiteRequest.H"
#include "comm/DirectTransport.H"
#include "comm/RCommand.H"

#include "SiteAdminClient.H"

using std::string;

static string withoutCR(const string& s)
{
    string out;
    for (char c : s)
        if (c != '\r')
            out += c;
    return out;
}

static string lastLines(const string& text, size_t n)
{
    string t = text;
    while (!t.empty() && t[t.size() - 1] == '\n')
        t.erase(t.size() - 1);
    size_t pos = t.size();
    for (size_t i = 0; i < n && pos != string::npos && pos > 0; i++)
        pos = t.rfind('\n', pos - 1);
    return pos == string::npos || pos == 0 ? t : t.substr(pos + 1);
}

string SiteAdminClient::serverHost(string& err)
{
    string path = string(Ecce::ecceHome()) + "/siteconfig/RemoteServer/DataServers";
    std::ifstream in(path.c_str());
    std::stringstream ss;
    ss << in.rdbuf();
    string text = ss.str();
    size_t url = text.find("<Url>");
    size_t scheme = url == string::npos ? url : text.find("://", url);
    if (scheme == string::npos)
    {
        err = "This computer has no central server configured (" + path +
              "); run ecce-remote-setup <server> first.";
        return "";
    }
    size_t start = scheme + 3;
    size_t end = text.find_first_of(":/<", start);
    string host = text.substr(start, end == string::npos ? string::npos
                                                          : end - start);
    //  A host name, nothing that ssh could read as an option.
    if (host.empty() || host[0] == '-' ||
        host.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")
            != string::npos)
    {
        err = "The central server in " + path + " is not a host name: '" +
              host + "'.";
        return "";
    }
    return host;
}


bool SiteAdminClient::send(const SiteRequest& request, string& report)
{
    string host = serverHost(report);
    if (host.empty())
        return false;
    const char* l = getenv("ECCE_SITE_ADMIN_LOGIN");
    string login = l ? l : "";

    const char* tmpdir = getenv("TMPDIR");
    string base = string(tmpdir && *tmpdir ? tmpdir : "/tmp") + "/ecce-site-XXXXXX";
    std::vector<char> buf(base.begin(), base.end());
    buf.push_back('\0');
    char* dir = mkdtemp(&buf[0]);
    if (dir == NULL)
    {
        report = "Cannot make a temporary directory for the request.";
        return false;
    }
    string local = string(dir) + "/request";
    {
        std::ofstream f(local.c_str(), std::ios::binary);
        f << request.encode();
        if (!f)
        {
            report = "Cannot write " + local + ".";
            rmdir(dir);
            return false;
        }
    }
    chmod(local.c_str(), 0600);

    bool ok = false;
    {
        RCommand rc(host, "ssh", "bash", login);
        string out;
        if (!rc.isOpen())
            report = "Cannot log in to " + host + " with ssh: " + rc.commError();
        else if (!rc.execout("mktemp -d \"${TMPDIR:-/tmp}/ecce-site.XXXXXX\"",
                             out, "", 60))
            report = "Cannot make a temporary directory on " + host + ": " +
                     withoutCR(out) + rc.commError();
        else
        {
            string rdir = withoutCR(out);
            while (!rdir.empty() && rdir[rdir.size() - 1] == '\n')
                rdir.erase(rdir.size() - 1);
            const char* files[] = { local.c_str(), NULL };
            if (rdir.empty() || rdir[0] != '/' ||
                rdir.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "0123456789._/-") != string::npos)
                report = "Unexpected answer from mktemp on " + host + ": " + rdir;
            else if (!rc.cd(rdir) || !rc.shellput(files, "."))
                report = "Cannot copy the request to " + host + ": " +
                         rc.commError();
            else
            {
                //  Fixed text; the request is the file named "request" in
                //  the directory the transport runs this in.
                ok = rc.execout("ecce-site-admin apply request; s=$?\n"
                                "rm -f request; d=$(pwd); cd / && rmdir \"$d\"\n"
                                "exit $s", out, "", 300);
                out = withoutCR(out);
                size_t e = out.rfind("ecce-site-admin: error: ");
                if (ok)
                    report = lastLines(out, 2);
                else if (e != string::npos)
                    report = out.substr(e + 24);
                else if (out.find("ecce-site-admin: not found") != string::npos
                         || out.find("command not found") != string::npos)
                    report = "ecce-site-admin was not found on " + host +
                             "; it comes with ECCE 9.0 (ecce-client) there.";
                else
                    report = "Saving on " + host + " failed: " +
                             lastLines(out, 4);
                if (ok && out.find("ecce-site-admin: ok") == string::npos)
                {
                    ok = false;
                    report = "Unexpected answer from " + host + ": " +
                             lastLines(out, 4);
                }
            }
        }
    }
    unlink(local.c_str());
    rmdir(dir);
    return ok;
}


bool SiteAdminClient::refresh(string& report)
{
    string home = Ecce::ecceHome();
    string site = home + "/siteconfig";
    if (access(site.c_str(), W_OK) != 0)
    {
        report = "This computer's copy of the server's settings (" + site +
                 ") was not updated, because you cannot write it. Update it "
                 "with:\n    sudo ecce-remote-setup --refresh";
        return false;
    }
    std::vector<string> argv;
    argv.push_back(home + "/bin/ecce-remote-setup");
    argv.push_back("--refresh");
    TransportResult r = DirectTransport::runProcess(argv, "", -1, -1, 120);
    if (r.status != 0)
    {
        report = "This computer's copy of the server's settings was not "
                 "updated (ecce-remote-setup --refresh: " +
                 (r.error.empty() ? lastLines(r.out + r.err, 3) : r.error) +
                 ").";
        return false;
    }
    return true;
}
