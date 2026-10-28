//  Driver for include/dsm/AllocationLeft.H (#613): reading projinfo output
//  (an anonymised copy of real Kebnekaise output) and the 15-minute cache.
//  Run via tests/machregister/profiles_test.py with the fixture path.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../../include/dsm/AllocationLeft.H"

using namespace AllocationLeft;

static int failures = 0;

static void check(bool ok, const char *what, const std::string& got = "")
{
    printf("%s  %s", ok ? "ok  " : "FAIL", what);
    if (!ok && !got.empty())
        printf("  got: %s", got.c_str());
    printf("\n");
    if (!ok)
        failures++;
}

int main(int argc, char **argv)
{
    std::ifstream f(argc > 1 ? argv[1] : "projinfo-kebnekaise.txt");
    std::stringstream all;
    all << f.rdbuf();
    std::string out = all.str();
    check(!out.empty(), "fixture read");

    std::string s = summarize(out, "hpc2n2026-351");
    check(s == "7.4 % of 30 000 CPU-h/month used (past 30 days)", "single-CPU project", s);

    s = summarize(out, "hpc2ncourses2026-002");
    check(s == "4.9 % of 5 000 CPU-h/month used (past 30 days); 3.1 % of 100 GPU-h/month",
          "project with a GPU allocation: both, compact", s);

    check(summarize(out, "HPC2N2026-351") == summarize(out, "hpc2n2026-351"),
          "the account is matched in any case");
    check(summarize(out, "hpc2n2026-999").empty(), "an unknown account shows nothing");
    check(summarize(out, "hpc2n2026-35").empty(), "a prefix of an account is not the account");
    check(summarize(out, "bad account;rm").empty(), "an unsafe account shows nothing");
    check(summarize("", "hpc2n2026-351").empty(), "no output shows nothing");

    std::string two = "Information for project p1:\nAllocations:\n"
        "    a:   100 CPUhours/month, used 1.0% the past 30 days\n"
        "    b:   200 CPUhours/month, used 2.0% the past 30 days\n";
    s = summarize(two, "p1");
    check(s.empty(), "two CPU allocations, no resource named: not guessed", s);
    s = summarize(two, "p1", "b");
    check(s == "2.0 % of 200 CPU-h/month used (past 30 days)",
          "two CPU allocations: the named resource", s);

    //  Not projinfo, not sshare's account line: no summary, the line for a tooltip
    std::string other = "Account User RawShares\n root 1\n  naiss-2   0.5  1000   12345\n";
    check(summarize(other, "naiss-2").empty(), "unrecognised output has no summary");
    s = rawLine(other, "naiss-2");
    check(s == "naiss-2 0.5 1000 12345", "its line for the tooltip, spaces squeezed", s);
    check(rawLine(other, "naiss-9").empty(), "account not in it: no line");
    std::string used, raw;
    decode(encode(other, "naiss-2"), used, raw);
    check(used.empty() && raw == "naiss-2 0.5 1000 12345", "cache text round trip, unrecognised", raw);

    //  Slurm default: sshare -n -P -A <acct> -o Account,User,RawUsage
    std::string sh = "naiss-2||4446000\nnaiss-2|alice|1800000\nnaiss-2|bob|2646000\n";
    s = summarize(sh, "naiss-2");
    check(s == "1 235 CPU-h used (no allocation size known)", "sshare: RawUsage seconds to CPU-h", s);
    check(summarize(sh, "NAISS-2") == s, "sshare: account in any case");
    check(summarize(sh, "naiss-3").empty(), "sshare: another account");
    check(summarize("naiss-2|alice|1800000\n", "naiss-2").empty(), "sshare: user line alone is not the account");
    check(summarize("naiss-2||12x\n", "naiss-2").empty(), "sshare: non-numeric usage");
    decode(encode(sh, "naiss-2"), used, raw);
    check(used == s && raw.empty(), "cache text round trip, recognised", used);
    check(substitute(slurmCommand(), "p1") == "sshare -n -P -A p1 -o Account,User,RawUsage", "slurm command");
    s = summarize("garbled\nInformation for project p1:\nAllocations:\n  x: lots of CPUhours\n", "p1");
    check(s.empty(), "an unreadable block has no summary", s);

    check(substitute("projinfo -p ##account_no##", "p1") == "projinfo -p p1", "command substitution");
    check(validAccount("hpc2n2026-351") && !validAccount("") && !validAccount("a b") &&
          !validAccount("a$b"), "account validity");

    Cache c;
    std::string k = Cache::key("kebnekaise", "p1");
    check(!c.fresh(k, 1000), "empty cache is not fresh");
    c.put(k, 1000, "x\ty");
    check(c.fresh(k, 1000 + 899) && !c.fresh(k, 1000 + 900), "fresh for 15 minutes");
    c.put(Cache::key("kebnekaise", "p2"), 5000, "");
    Cache d;
    d.parse(c.serialise());
    Cache::Entry e;
    check(d.find(k, e) && e.when == 1000 && e.text == "x y", "cache round trip");
    check(d.find(Cache::key("kebnekaise", "p2"), e) && e.text.empty(),
          "a failed answer is kept as empty");
    check(checkedAt(0).size() == 5, "checked time is HH:MM");

    if (failures)
        printf("FAIL  %d check(s)\n", failures);
    else
        printf("PASS  allocation left\n");
    return failures ? 1 : 0;
}
