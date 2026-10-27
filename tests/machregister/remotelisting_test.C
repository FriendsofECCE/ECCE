//  RemoteListing.H: ls -1Ap output, paths and the start directory.
#include <cstdio>
#include "../../src/apps/machregister/RemoteListing.H"
using namespace RemoteListing;

static int failures = 0;
static void eq(const std::string& what, const std::string& got,
               const std::string& want)
{
    if (got != want)
    {
        printf("FAIL  %s: got '%s', wanted '%s'\n", what.c_str(), got.c_str(),
               want.c_str());
        failures++;
    }
}

int main()
{
    Entries e = parse(".bashrc\nbin/\nmy dir/\norca\r\n\nnwchem\n");
    std::string d, f;
    for (size_t i = 0; i < e.dirs.size(); i++) d += e.dirs[i] + "|";
    for (size_t i = 0; i < e.files.size(); i++) f += e.files[i] + "|";
    eq("dirs", d, "bin|my dir|");
    eq("files", f, ".bashrc|orca|nwchem|");
    eq("empty", parse("").files.size() ? "x" : "ok", "ok");
    eq("slash only", parse("/\n").dirs.size() ? "x" : "ok", "ok");
    eq("parent", parent("/opt/orca/"), "/opt");
    eq("parent root", parent("/"), "/");
    eq("parent top", parent("/opt"), "/");
    eq("join", join("/", "opt"), "/opt");
    eq("join2", join("/opt//", "orca"), "/opt/orca");
    eq("normalize", normalize("/a/./b/../c/"), "/a/c");
    eq("start file", startDir("/opt/orca/orca", "/home/u"), "/opt/orca");
    eq("start dir", startDir("/opt/orca/", "/home/u"), "/opt/orca");
    eq("start relative", startDir("orca", "/home/u"), "/home/u");
    eq("start empty", startDir("", ""), "/");
    eq("quote", command("/a b/it's"), "ls -1Ap -- '/a b/it'\\''s'");
    printf(failures ? "FAILED\n" : "PASS\n");
    return failures ? 1 : 0;
}
