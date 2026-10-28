//  How much of an allocation is used (#613): reads the output of a machine's
//  allocation command (HPC2N/NAISS `projinfo`, Slurm `sshare`, or anything
//  else) into one line for Tools > Allocations, and keeps the answers for 15
//  minutes.
//
//  Header-only and free of ECCE and wx types so the rules can be tested alone
//  (tests/machregister/testAllocationLeft.C). Wrong numbers are worse than no
//  number, so output that is not recognised gives no summary; the output line
//  that names the account is kept separately for a tooltip.
#ifndef ALLOCATIONLEFT_HH
#define ALLOCATIONLEFT_HH

#include <cctype>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace AllocationLeft
{
    const long CACHE_SECONDS = 15 * 60;

    //  The account goes into a command line.
    inline bool validAccount(const std::string& a)
    {
        if (a.empty() || a.size() > 64)
            return false;
        for (size_t i = 0; i < a.size(); i++)
            if (!std::isalnum((unsigned char)a[i]) && a[i] != '.' && a[i] != '_'
                && a[i] != '-')
                return false;
        return true;
    }

    inline std::string substitute(std::string command, const std::string& account)
    {
        const std::string token = "##account_no##";
        for (size_t at; (at = command.find(token)) != std::string::npos; )
            command.replace(at, token.size(), account);
        return command;
    }

    inline std::string lower(std::string s)
    {
        for (size_t i = 0; i < s.size(); i++)
            s[i] = (char)std::tolower((unsigned char)s[i]);
        return s;
    }

    inline std::string trim(const std::string& s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        if (a == std::string::npos)
            return "";
        return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
    }

    //  12000 -> "12 000"
    inline std::string grouped(const std::string& digits)
    {
        std::string r;
        size_t n = digits.size();
        for (size_t i = 0; i < n; i++)
        {
            r += digits[i];
            if (i + 1 < n && (n - i - 1) % 3 == 0)
                r += ' ';
        }
        return r;
    }

    inline std::vector<std::string> lines(const std::string& text)
    {
        std::vector<std::string> v;
        std::istringstream in(text);
        std::string l;
        while (std::getline(in, l))
            v.push_back(l);
        return v;
    }

    //  One "kebnekaise:  5000 CPUhours/month, used 4.9% the past 30 days" line
    //  of projinfo's Allocations list.
    struct Rate
    {
        std::string resource, amount, kind, used;   // kind: CPU, GPU ...
    };

    inline bool parseRate(const std::string& line, Rate& r)
    {
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            return false;
        r.resource = trim(line.substr(0, colon));
        std::istringstream in(line.substr(colon + 1));
        std::string num, unit, used, pct;
        in >> num >> unit;
        if (num.empty() || unit.empty())
            return false;
        for (size_t i = 0; i < num.size(); i++)
            if (!std::isdigit((unsigned char)num[i]))
                return false;
        size_t h = unit.find("hours/month");
        if (h == std::string::npos || h == 0)
            return false;
        in >> used >> pct;
        if (used != "used")
            return false;
        if (pct.empty() || pct[pct.size() - 1] != '%')
            return false;
        pct.erase(pct.size() - 1);
        char *end = 0;
        std::strtod(pct.c_str(), &end);
        if (end == pct.c_str() || *end != 0)
            return false;
        r.amount = num;
        r.kind = unit.substr(0, h);
        r.used = pct;
        return true;
    }

    //  The project's block of `projinfo` output as one line, "" when the
    //  account has no block or the block has no recognised allocation. With
    //  several CPU allocations the one named resource is taken.
    inline std::string projinfo(const std::string& output, const std::string& account,
                                const std::string& resource)
    {
        const std::string head = "information for project " + lower(account) + ":";
        std::vector<std::string> v = lines(output);
        size_t i = 0;
        for (; i < v.size(); i++)
            if (lower(trim(v[i])) == head)
                break;
        if (i == v.size())
            return "";

        std::vector<Rate> rates;
        bool inAlloc = false;
        for (i++; i < v.size(); i++)
        {
            std::string t = trim(v[i]);
            std::string lt = lower(t);
            if (lt.compare(0, 24, "information for project ") == 0 || lt.compare(0, 5, "note:") == 0)
                break;
            if (lt == "allocations:")
            {
                inAlloc = true;
                continue;
            }
            if (inAlloc && lt.compare(0, 6, "usage ") == 0)
                inAlloc = false;
            Rate r;
            if (inAlloc && parseRate(t, r))
                rates.push_back(r);
        }

        const Rate *cpu = 0;
        int cpuCount = 0;
        for (size_t k = 0; k < rates.size(); k++)
            if (rates[k].kind == "CPU")
            {
                cpuCount++;
                if (cpuCount == 1 || (!resource.empty() && rates[k].resource == resource))
                    cpu = &rates[k];
            }
        if (cpuCount > 1 && (resource.empty() || !cpu || cpu->resource != resource))
            return "";                  // which one is ours is not known
        if (!cpu)
            return "";

        std::string out = cpu->used + " % of " + grouped(cpu->amount) + " CPU-h/month used (past 30 days)";
        for (size_t k = 0; k < rates.size(); k++)
            if (rates[k].kind != "CPU")
                out += "; " + rates[k].used + " % of " + grouped(rates[k].amount) + " " +
                       rates[k].kind + "-h/month";
        return out;
    }

    //  The account as a whole word, not as part of a longer account name.
    inline bool hasWord(const std::string& line, const std::string& word)
    {
        for (size_t at = line.find(word); at != std::string::npos;
             at = line.find(word, at + 1))
        {
            size_t end = at + word.size();
            bool before = at > 0 && (std::isalnum((unsigned char)line[at - 1]) ||
                                     line[at - 1] == '.' || line[at - 1] == '_' ||
                                     line[at - 1] == '-');
            bool after = end < line.size() && (std::isalnum((unsigned char)line[end]) ||
                                               line[end] == '.' || line[end] == '_' ||
                                               line[end] == '-');
            if (!before && !after)
                return true;
        }
        return false;
    }

    //  Slurm's default: the account's own usage, one "account|user|RawUsage"
    //  line per association. `sshare -l` has no fixed column layout.
    inline const char *slurmCommand()
    {
        return "sshare -n -P -A ##account_no## -o Account,User,RawUsage";
    }

    //  CPU-seconds of the account's own line (empty User), as
    //  "1 234 CPU-h used (no allocation size known)"; "" if not in that format.
    inline std::string sshare(const std::string& output, const std::string& account)
    {
        const std::string a = lower(account);
        std::vector<std::string> v = lines(output);
        for (size_t i = 0; i < v.size(); i++)
        {
            std::vector<std::string> f;
            std::string t = trim(v[i]), cur;
            for (size_t k = 0; k <= t.size(); k++)
            {
                if (k == t.size() || t[k] == '|')
                {
                    f.push_back(trim(cur));
                    cur.clear();
                }
                else
                    cur += t[k];
            }
            if (f.size() != 3 || lower(f[0]) != a || !f[1].empty() || f[2].empty())
                continue;
            for (size_t k = 0; k < f[2].size(); k++)
                if (!std::isdigit((unsigned char)f[2][k]))
                    return "";
            const double h = std::atof(f[2].c_str()) / 3600.0;
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.0f", h);
            return grouped(buf) + " CPU-h used (no allocation size known)";
        }
        return "";
    }

    //  What is shown for the command's output: the recognised summary, else "".
    inline std::string summarize(const std::string& output, const std::string& account,
                                 const std::string& resource = "")
    {
        if (!validAccount(account))
            return "";
        std::string s = projinfo(output, account, resource);
        return s.empty() ? sshare(output, account) : s;
    }

    //  For a tooltip: the first output line that names the account (120
    //  characters at most), spaces squeezed, else "".
    inline std::string rawLine(const std::string& output, const std::string& account)
    {
        if (!validAccount(account))
            return "";
        std::string a = lower(account);
        std::vector<std::string> v = lines(output);
        for (size_t i = 0; i < v.size(); i++)
        {
            std::string t = trim(v[i]);
            if (hasWord(lower(t), a))
            {
                std::string r;
                bool space = false;
                for (size_t k = 0; k < t.size() && r.size() < 120; k++)
                {
                    bool sp = std::isspace((unsigned char)t[k]) != 0;
                    if (sp && space)
                        continue;
                    r += sp ? ' ' : t[k];
                    space = sp;
                }
                return r;
            }
        }
        return "";
    }

    //  The cache keeps both in one text: summary, a separator, raw line.
    inline std::string encode(const std::string& output, const std::string& account)
    {
        std::string used = summarize(output, account);
        return used + "\x1f" + (used.empty() ? rawLine(output, account) : "");
    }

    inline void decode(const std::string& text, std::string& used, std::string& raw)
    {
        size_t at = text.find('\x1f');
        used = text.substr(0, at);
        raw = at == std::string::npos ? "" : text.substr(at + 1);
    }

    inline std::string checkedAt(long when)
    {
        time_t t = (time_t)when;
        struct tm tmv;
#ifdef _WIN32
        localtime_s(&tmv, &t);
#else
        localtime_r(&t, &tmv);
#endif
        char buf[16];
        std::snprintf(buf, sizeof buf, "%02d:%02d", tmv.tm_hour, tmv.tm_min);
        return buf;
    }

    //  Answers per machine and account. A failed or empty answer is kept too
    //  (text ""), so a cluster that is down is not tried at every click.
    class Cache
    {
    public:
        struct Entry { long when; std::string text; };

        static std::string key(const std::string& machine, const std::string& account)
        {
            return machine + "/" + account;
        }

        bool find(const std::string& k, Entry& e) const
        {
            std::map<std::string, Entry>::const_iterator i = p_items.find(k);
            if (i == p_items.end())
                return false;
            e = i->second;
            return true;
        }

        bool fresh(const std::string& k, long now) const
        {
            Entry e;
            return find(k, e) && now >= e.when && now - e.when < CACHE_SECONDS;
        }

        void put(const std::string& k, long when, const std::string& text)
        {
            Entry e; e.when = when; e.text = text;
            p_items[k] = e;
        }

        void parse(const std::string& text)
        {
            std::vector<std::string> v = lines(text);
            for (size_t i = 0; i < v.size(); i++)
            {
                size_t a = v[i].find('\t');
                size_t b = a == std::string::npos ? a : v[i].find('\t', a + 1);
                if (b == std::string::npos)
                    continue;
                long when = std::atol(v[i].substr(a + 1, b - a - 1).c_str());
                if (when > 0)
                    put(v[i].substr(0, a), when, v[i].substr(b + 1));
            }
        }

        std::string serialise() const
        {
            std::string out;
            for (std::map<std::string, Entry>::const_iterator i = p_items.begin();
                 i != p_items.end(); ++i)
            {
                std::string t = i->second.text;
                for (size_t k = 0; k < t.size(); k++)
                    if (t[k] == '\t' || t[k] == '\n' || t[k] == '\r')
                        t[k] = ' ';
                out += i->first + "\t" + std::to_string(i->second.when) + "\t" + t + "\n";
            }
            return out;
        }

    private:
        std::map<std::string, Entry> p_items;
    };
}

#endif
