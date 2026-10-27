//  Driver for include/dsm/AllocationProfiles.H (#541): parsing and writing the
//  MachProfiles file, which profile the Launcher opens on, and what is refused.
//  Header-only, no ECCE libraries. Run via tests/machregister/profiles_test.py.
#include <cstdio>
#include <string>
#include "../../include/dsm/AllocationProfiles.H"

using namespace AllocationProfiles;

static int failures = 0;

static void check(bool ok, const char *what)
{
    printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        failures++;
}

static Profile mk(const char *n, const char *a, const char *r, const char *s,
                  const char *q = "")
{
    Profile p;
    p.name = n; p.account = a; p.rundir = r; p.scratchdir = s; p.queue = q;
    return p;
}

int main()
{
    Store s;
    check(s.serialise().empty(), "nothing stored writes nothing");
    check(s.names("keb").size() == 1 && s.names("keb")[0] == "Default",
          "a machine with no stored profile has only Default");

    check(s.put("keb", mk("Course", "edu-1", "/proj/course/run", "/scratch/c")),
          "a profile is added");
    check(s.put("keb", mk("Project", "naiss-2", "/proj/p/run", "/scratch/p", "batch")),
          "a second profile is added");
    check(s.put("other", mk("Course", "x", "/x", "")),
          "the same name on another machine is a different profile");
    check(!s.put("keb", mk("Default", "a", "b", "c")), "Default cannot be stored");
    check(!s.put("keb", mk("default", "a", "b", "c")), "default in any case cannot be stored");
    check(!s.put("keb", mk("  ", "a", "b", "c")), "an empty name is refused");

    std::vector<std::string> n = s.names("keb");
    check(n.size() == 3 && n[0] == "Default" && n[1] == "Course" && n[2] == "Project",
          "names: Default first, then in the order made");

    Profile p;
    check(s.find("keb", "project", p) && p.account == "naiss-2" && p.queue == "batch",
          "a name is found in any case");
    check(!s.find("keb", "Default", p), "Default is not stored here");
    check(!s.find("other", "Project", p), "profiles belong to their machine");

    check(s.put("keb", mk("Course", "edu-9", "/new", "")), "a profile is replaced");
    check(s.find("keb", "Course", p) && p.account == "edu-9" && p.scratchdir.empty()
          && s.names("keb").size() == 3, "replaced in place, not duplicated");

    check(s.put("keb", mk(" Odd|name ", "a|b", "/r\t/s", "q")), "unsafe characters are cleaned");
    check(s.find("keb", "Odd name", p) && p.account == "a b" && p.rundir == "/r /s",
          "a '|' or tab in a name or field becomes a space");
    s.remove("keb", "Odd name");

    s.setLast("keb", "Project");

    std::string text = s.serialise();
    Store t;
    t.parse(text);
    check(t.serialise() == text, "writing, reading and writing again gives the same file");
    check(t.find("keb", "Project", p) && p.account == "naiss-2" && p.rundir == "/proj/p/run"
          && p.scratchdir == "/scratch/p" && p.queue == "batch",
          "every field survives a round trip");
    check(t.last("keb") == "Project" && t.last("other") == "", "the last-used profile survives");

    Store junk;
    junk.parse("### header\nnonsense\n\nprofile\tkeb\nfuture\tkeb\tx\nprofile\tkeb\tA|b\n");
    check(junk.names("keb").size() == 2 && junk.find("keb", "A", p) && p.rundir.empty(),
          "unknown or short lines are skipped, a short profile line is padded");

    //  Which profile opens
    check(t.choose("keb", "", false) == "Project", "no calculation: the last used");
    check(t.choose("keb", "Course", true) == "Course", "a calculation opens on its own profile");
    check(t.choose("keb", "", true) == "Default",
          "a calculation launched without a profile opens on Default, not the last used");
    check(t.choose("keb", "Gone", true) == "Default", "a deleted profile falls back to Default");
    check(t.choose("keb", "COURSE", true) == "Course", "the stored name is matched in any case");
    check(t.choose("fresh", "", false) == "Default", "a machine never used with profiles");

    //  Deleting
    check(t.remove("keb", "Project"), "a profile is deleted");
    check(!t.remove("keb", "Default") && !t.remove("keb", "Project"),
          "Default and unknown names cannot be deleted");
    check(t.last("keb").empty() && t.choose("keb", "", false) == "Default",
          "deleting the last-used profile clears it");
    t.remove("keb", "Course");
    t.remove("other", "Course");
    check(t.serialise().empty(), "with every profile deleted nothing is stored");

    if (failures == 0)
        printf("PASS  allocation profile rules\n");
    else
        printf("FAIL  %d allocation profile check(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
