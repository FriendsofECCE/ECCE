//  Allocation profiles (#541): a machine can hold several named sets of
//  allocation account, run directory, scratch directory and queue. The
//  machine's own MachPrefs line is its "Default" profile, so a user with one
//  allocation sees no change and older ECCE versions, which never read the
//  MachProfiles file, keep working.
//
//  Header-only and free of ECCE and wx types so the rules can be tested alone
//  (tests/machregister/testAllocationProfiles.C); the Launcher only shows them.
#ifndef ALLOCATIONPROFILES_HH
#define ALLOCATIONPROFILES_HH

#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace AllocationProfiles
{
    inline const char *defaultName() { return "Default"; }

    struct Profile
    {
        std::string name;
        std::string account;
        std::string rundir;
        std::string scratchdir;
        std::string queue;      //  empty: keep the queue the Launcher shows
    };

    //  '|' and tabs separate fields on disk, so they cannot be kept.
    inline std::string clean(const std::string& s)
    {
        std::string r = s;
        for (size_t i = 0; i < r.size(); i++)
            if (r[i] == '|' || r[i] == '\t' || r[i] == '\n' || r[i] == '\r')
                r[i] = ' ';
        size_t a = r.find_first_not_of(' ');
        if (a == std::string::npos)
            return "";
        return r.substr(a, r.find_last_not_of(' ') - a + 1);
    }

    inline std::string lower(std::string s)
    {
        for (size_t i = 0; i < s.size(); i++)
            s[i] = (char)std::tolower((unsigned char)s[i]);
        return s;
    }

    inline bool isDefault(const std::string& name)
    {
        std::string n = clean(name);
        return n.empty() || lower(n) == lower(defaultName());
    }

    //  A name the user may give a new profile: not empty, not "Default".
    inline bool validName(const std::string& name)
    {
        return !isDefault(name);
    }

    //  The profiles other than Default, per machine, and the one each
    //  machine used last.
    class Store
    {
    public:
        void parse(const std::string& text)
        {
            std::istringstream in(text);
            std::string line;
            while (std::getline(in, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;
                std::vector<std::string> t = split(line, '\t');
                if (t.size() < 3)
                    continue;               //  not ours: skip, never fail
                if (t[0] == "profile")
                {
                    std::vector<std::string> f = split(t[2], '|');
                    f.resize(5);
                    Profile p;
                    p.name = f[0]; p.account = f[1]; p.rundir = f[2];
                    p.scratchdir = f[3]; p.queue = f[4];
                    put(t[1], p);
                }
                else if (t[0] == "last")
                    setLast(t[1], t[2]);
            }
        }

        //  Empty when there is nothing to keep.
        std::string serialise() const
        {
            std::string out;
            for (size_t i = 0; i < p_items.size(); i++)
            {
                const Item& it = p_items[i];
                out += "profile\t" + it.machine + "\t" + it.p.name + "|"
                    + it.p.account + "|" + it.p.rundir + "|"
                    + it.p.scratchdir + "|" + it.p.queue + "\n";
            }
            for (size_t i = 0; i < p_last.size(); i++)
                out += "last\t" + p_last[i].first + "\t" + p_last[i].second + "\n";
            if (!out.empty())
                out = "### DO NOT EDIT ### - File automatically generated!\n" + out;
            return out;
        }

        //  Default first, then the stored profiles in the order they were made.
        std::vector<std::string> names(const std::string& machine) const
        {
            std::vector<std::string> n(1, defaultName());
            for (size_t i = 0; i < p_items.size(); i++)
                if (p_items[i].machine == machine)
                    n.push_back(p_items[i].p.name);
            return n;
        }

        //  False for Default (the machine's own settings) and unknown names.
        bool find(const std::string& machine, const std::string& name,
                  Profile& out) const
        {
            int i = index(machine, name);
            if (i < 0)
                return false;
            out = p_items[i].p;
            return true;
        }

        //  Adds or replaces. False for Default and for an unusable name.
        bool put(const std::string& machine, Profile p)
        {
            p.name = clean(p.name);
            if (!validName(p.name))
                return false;
            p.account = clean(p.account);
            p.rundir = clean(p.rundir);
            p.scratchdir = clean(p.scratchdir);
            p.queue = clean(p.queue);
            int i = index(machine, p.name);
            if (i >= 0)
                p_items[i].p = p;
            else
            {
                Item it; it.machine = machine; it.p = p;
                p_items.push_back(it);
            }
            return true;
        }

        bool remove(const std::string& machine, const std::string& name)
        {
            int i = index(machine, name);
            if (i < 0)
                return false;
            p_items.erase(p_items.begin() + i);
            if (lower(last(machine)) == lower(clean(name)))
                setLast(machine, "");
            return true;
        }

        std::string last(const std::string& machine) const
        {
            for (size_t i = 0; i < p_last.size(); i++)
                if (p_last[i].first == machine)
                    return p_last[i].second;
            return "";
        }

        void setLast(const std::string& machine, const std::string& name)
        {
            std::string n = isDefault(name) ? "" : clean(name);
            for (size_t i = 0; i < p_last.size(); i++)
                if (p_last[i].first == machine)
                {
                    if (n.empty())
                        p_last.erase(p_last.begin() + i);
                    else
                        p_last[i].second = n;
                    return;
                }
            if (!n.empty())
                p_last.push_back(std::make_pair(machine, n));
        }

        //  Which profile the Launcher opens on. A calculation that was launched
        //  with a profile opens on it, one launched without opens on Default; with
        //  no calculation the machine's last-used profile is taken. A name that no
        //  longer exists falls back to Default.
        std::string choose(const std::string& machine, const std::string& stored,
                           bool haveCalc) const
        {
            std::string want = haveCalc ? stored : last(machine);
            if (isDefault(want))
                return defaultName();
            int i = index(machine, want);
            return i < 0 ? std::string(defaultName()) : p_items[i].p.name;
        }

    private:
        struct Item { std::string machine; Profile p; };
        std::vector<Item> p_items;
        std::vector<std::pair<std::string, std::string> > p_last;

        int index(const std::string& machine, const std::string& name) const
        {
            std::string n = lower(clean(name));
            for (size_t i = 0; i < p_items.size(); i++)
                if (p_items[i].machine == machine && lower(p_items[i].p.name) == n)
                    return (int)i;
            return -1;
        }

        static std::vector<std::string> split(const std::string& s, char d)
        {
            std::vector<std::string> r;
            size_t a = 0;
            for (;;)
            {
                size_t b = s.find(d, a);
                if (b == std::string::npos)
                {
                    r.push_back(s.substr(a));
                    return r;
                }
                r.push_back(s.substr(a, b - a));
                a = b + 1;
            }
        }
    };
}

#endif
