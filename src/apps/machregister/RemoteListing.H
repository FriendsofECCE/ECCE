//  The wx-free part of the remote Browse dialog: what to run on the machine
//  and how its output becomes a directory listing.  Header-only so a test
//  can compile it without wx.
#ifndef REMOTELISTING_HH
#define REMOTELISTING_HH

#include <string>
#include <vector>

namespace RemoteListing
{
    struct Entries
    {
        std::vector<std::string> dirs;   // without the trailing '/'
        std::vector<std::string> files;
    };

    //  command() marks directories with '/'; no other marker is added, so a
    //  name is a file unless it ends in '/'.  -A never lists "." or "..".
    inline Entries parse(const std::string& output)
    {
        Entries e;
        size_t pos = 0;
        while (pos < output.size())
        {
            size_t nl = output.find('\n', pos);
            if (nl == std::string::npos) nl = output.size();
            std::string line = output.substr(pos, nl - pos);
            pos = nl + 1;
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.empty()) continue;
            if (line[line.size() - 1] == '/')
            {
                line.erase(line.size() - 1);
                if (!line.empty()) e.dirs.push_back(line);
            }
            else
                e.files.push_back(line);
        }
        return e;
    }

    //  Collapses "//" and a trailing '/'; "." and ".." segments are resolved
    //  textually (the chooser only descends or goes up).
    inline std::string normalize(const std::string& path)
    {
        std::vector<std::string> seg;
        size_t pos = 0;
        while (pos <= path.size())
        {
            size_t s = path.find('/', pos);
            if (s == std::string::npos) s = path.size();
            std::string part = path.substr(pos, s - pos);
            pos = s + 1;
            if (part.empty() || part == ".") continue;
            if (part == "..") { if (!seg.empty()) seg.pop_back(); continue; }
            seg.push_back(part);
        }
        std::string out;
        for (size_t i = 0; i < seg.size(); i++) out += "/" + seg[i];
        return out.empty() ? "/" : out;
    }

    inline std::string parent(const std::string& dir)
    {
        return normalize(normalize(dir) + "/..");
    }

    inline std::string join(const std::string& dir, const std::string& name)
    {
        return normalize(dir + "/" + name);
    }

    //  Directory to start in: that of an absolute `current` path (the path
    //  itself when it ends in '/'), else `home`, else "/".
    inline std::string startDir(const std::string& current,
                                const std::string& home)
    {
        if (!current.empty() && current[0] == '/')
        {
            if (current[current.size() - 1] == '/') return normalize(current);
            return parent(current);
        }
        return home.empty() || home[0] != '/' ? "/" : normalize(home);
    }

    //  Single-quoted for the remote shell, so a name with spaces or quotes
    //  stays one word.
    inline std::string shellQuote(const std::string& s)
    {
        std::string q = "'";
        for (size_t i = 0; i < s.size(); i++)
            q += s[i] == '\'' ? std::string("'\\''") : std::string(1, s[i]);
        return q + "'";
    }

    //  The command text handed to `sh -c` on the machine.
    inline std::string command(const std::string& dir)
    {
        //  test -d follows symlinks, so a link to a folder (common for
        //  program installs) is entered rather than chosen; `ls -p` would not
        //  mark it, and `ls -L` fails on one broken link.
        return "cd -- " + shellQuote(dir) + " && ls -1A | while IFS= read -r f; "
               "do if [ -d \"$f\" ]; then printf '%s/\\n' \"$f\"; "
               "else printf '%s\\n' \"$f\"; fi; done";
    }
}

#endif
