#ifndef SUBMITHEADERCHECK_H
#define SUBMITHEADERCHECK_H

#include <cctype>
#include <map>
#include <string>
#include <vector>

// A queue-manager block in a machine's CONFIG replaces ECCE's whole default
// submit header (gensub merges blocks whole, never line by line), so a
// one-line "Slurm: #SBATCH --mem-per-cpu=2500M" leaves the job without
// --partition, --time and --account.  This says so, for Register Machines
// and the launch.  Header-only so a test can use it without the libraries.
namespace SubmitHeaderCheck
{
    inline std::string lower(const std::string& s)
    {
        std::string r = s;
        for (size_t i = 0; i < r.size(); i++)
            r[i] = (char)std::tolower((unsigned char)r[i]);
        return r;
    }

    // True when some #SBATCH line sets the option under its long or short name.
    inline bool slurmSets(const std::string& block, const char* longOpt,
                          const char* shortOpt)
    {
        size_t pos = 0;
        while (pos <= block.size())
        {
            size_t eol = block.find('\n', pos);
            if (eol == std::string::npos)
                eol = block.size();
            std::string line = block.substr(pos, eol - pos);
            pos = eol + 1;
            size_t b = line.find_first_not_of(" \t\r");
            if (b == std::string::npos || line.compare(b, 7, "#SBATCH") != 0)
                continue;
            line.erase(0, b + 7);
            for (size_t i = 0; i < line.size(); i++)
                if (line[i] == '=') line[i] = ' ';
            size_t at = 0;
            while ((at = line.find_first_not_of(" \t\r", at)) != std::string::npos)
            {
                size_t end = line.find_first_of(" \t\r", at);
                std::string tok = line.substr(at, end == std::string::npos
                                                      ? end : end - at);
                if (tok == longOpt || tok == shortOpt)
                    return true;
                if (end == std::string::npos)
                    break;
                at = end;
            }
        }
        return false;
    }

    // The account a generated submit script charges the job to, "" when its
    // directives name none: #SBATCH --account/-A, #PBS -A, #BSUB -P.  Read
    // from the script after gensub has substituted, so it is the account the
    // scheduler sees whether it came from the launcher, the machine's CONFIG
    // block or a queue default.  An unsubstituted ##token## or $variable is
    // not an account.
    inline std::string submitAccount(const std::string& script)
    {
        static const struct { const char* tag; const char* a; const char* b; } kinds[] = {
            { "#SBATCH", "--account", "-A" },
            { "#PBS",    "-A",        "-A" },
            { "#BSUB",   "-P",        "-P" } };
        size_t pos = 0;
        while (pos <= script.size())
        {
            size_t eol = script.find('\n', pos);
            if (eol == std::string::npos)
                eol = script.size();
            std::string line = script.substr(pos, eol - pos);
            pos = eol + 1;
            size_t b = line.find_first_not_of(" \t\r");
            if (b == std::string::npos)
                continue;
            for (size_t k = 0; k < 3; k++)
            {
                size_t n = std::string(kinds[k].tag).size();
                if (line.compare(b, n, kinds[k].tag) != 0)
                    continue;
                line.erase(0, b + n);
                for (size_t i = 0; i < line.size(); i++)
                    if (line[i] == '=') line[i] = ' ';
                size_t at = 0;
                while ((at = line.find_first_not_of(" \t\r", at)) != std::string::npos)
                {
                    size_t end = line.find_first_of(" \t\r", at);
                    std::string tok = line.substr(at, end == std::string::npos
                                                          ? end : end - at);
                    if ((tok == kinds[k].a || tok == kinds[k].b) &&
                        end != std::string::npos)
                    {
                        size_t v = line.find_first_not_of(" \t\r", end);
                        if (v != std::string::npos)
                        {
                            size_t ve = line.find_first_of(" \t\r", v);
                            std::string val = line.substr(v, ve == std::string::npos
                                                                ? ve : ve - v);
                            if (!val.empty() && val[0] != '#' && val[0] != '$' &&
                                val[0] != '-')
                                return val;
                        }
                    }
                    if (end == std::string::npos)
                        break;
                    at = end;
                }
                break;
            }
        }
        return "";
    }

    // cfg: the machine's effective CONFIG (RefMachine::config, lower-case
    // keys).  qmgr: the machine's queue manager.  needAccount: the machine
    // uses allocation accounts.  Returns "" when there is nothing to warn
    // about: a machine without a block for its queue manager uses ECCE's
    // default header, which is complete.
    inline std::string warning(const std::map<std::string, std::string>& cfg,
                               const std::string& qmgr, bool needAccount)
    {
        std::string key = lower(qmgr);
        std::map<std::string, std::string>::const_iterator it = cfg.find(key);
        if (key.empty() || key == "none" || key == "shell" || it == cfg.end())
            return "";
        const std::string& block = it->second;

        std::vector<std::string> missing;
        if (key == "slurm")
        {
            if (!slurmSets(block, "--partition", "-p"))
                missing.push_back("--partition");
            if (!slurmSets(block, "--time", "-t"))
                missing.push_back("--time");
            if (!slurmSets(block, "--account", "-A"))
                missing.push_back("--account");
        }
        else if (needAccount && block.find("$account") == std::string::npos &&
                 block.find("${account}") == std::string::npos)
            missing.push_back("$account");
        if (missing.empty())
            return "";

        std::string list;
        for (size_t i = 0; i < missing.size(); i++)
            list += (i ? ", " : "") + missing[i];
        std::string msg = "This machine's CONFIG has a " + qmgr + ": entry. "
            "It replaces ECCE's whole default submit header, it does not add "
            "to it, and it has no " + list + " line, so the job is submitted "
            "without " + (missing.size() > 1 ? "them" : "it") + ".";
        if (needAccount && key == "slurm")
            msg += " This machine uses allocation accounts.";
        msg += " Write the full header in the entry, or remove it to use the "
               "default.";
        return msg;
    }
}

#endif
