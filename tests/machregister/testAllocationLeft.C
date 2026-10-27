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
    check(s == "Information for project p1:",
          "two CPU allocations, no resource named: not guessed, the heading line", s);
    s = summarize(two, "p1", "b");
    check(s == "2.0 % of 200 CPU-h/month used (past 30 days)",
          "two CPU allocations: the named resource", s);

    //  Not projinfo: the line that names the account, else nothing
    std::string sshare = "Account User RawShares\n root 1\n  naiss-2   0.5  1000   12345\n";
    s = summarize(sshare, "naiss-2");
    check(s == "naiss-2 0.5 1000 12345", "another tool: the account's line, spaces squeezed", s);
    check(summarize(sshare, "naiss-9").empty(), "another tool, account not in it: nothing");
    s = summarize("garbled\nInformation for project p1:\nAllocations:\n  x: lots of CPUhours\n", "p1");
    check(s == "Information for project p1:", "an unreadable block falls back to its heading line", s);

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
