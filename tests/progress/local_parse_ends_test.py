#!/usr/bin/env python3
"""#1094: an import or re-parse job store never parks itself for a later
catch-up, never loops on silence and ends with the session.  A parked
import has no job id, so nothing could ever end it; the example GROMACS
imports were left running past Quit.  Structural: each exit path of
JobStore.C that would keep or park a store sends a local parse to fail().
Optional argument: the JobStore.C to check."""
import os
import re
import sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                   "src", "comm", "commxt", "JobStore.C")


def body(text, signature):
    i = text.find(signature + "\n{")
    if i < 0:
        return None
    depth, j = 0, text.index("{", i)
    for k in range(j, len(text)):
        depth += {"{": 1, "}": -1}.get(text[k], 0)
        if depth == 0:
            return text[j:k + 1]
    return None


def main():
    text = open(sys.argv[1] if len(sys.argv) > 1 else SRC).read()
    errors = []
    if not re.search(r"static bool localParse\(void\)\s*\{\s*return gReparse"
                     r" \|\| !cpImportDir\.empty\(\);", text):
        errors.append("localParse() must cover re-parse and import")
    for sig in ("void park(const char* name, const string& msg, int exitStatus)",
                "void reconnectAfterSilence(void)",
                "void ecceExitMCB(JMSMessage& msg)"):
        b = body(text, sig)
        if b is None:
            errors.append("missing " + sig)
            continue
        first = b.find("localParse()")
        if first < 0 or "fail(" not in b[first:first + 200]:
            errors.append(sig + ": a local parse must go to fail()")
            continue
        for kept in ("WaitingJobs::add", "exitMessaging", "restartSystem",
                     "exit(4)"):
            k = b.find(kept)
            if 0 <= k < first:
                errors.append(sig + ": " + kept + " before the local-parse check")
    f = body(text, "void fail(const char* name, const string& msg)")
    if f is None or "WaitingJobs::add" in f or "park(" in f:
        errors.append("fail() must end the store without parking it")
    for e in errors:
        print("FAIL:", e)
    if not errors:
        print("PASS: import/re-parse stores end instead of parking (#1094)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
