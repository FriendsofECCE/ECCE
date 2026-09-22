"""
Static check: shell commands run with an INHERITED working directory must
not redirect output to a bare filename.

The bug this exists for (2026-09-22, all codes stuck at "submitted"):

    clientCmd += " > eccejobmaster.log 2>&1 &";
    int status = system((char*)clientCmd.c_str());

``system()`` inherits the calling process's cwd, and for every ECCE GUI
app that cwd is ``$ECCE_HOME/bin`` -- the gateway launches them with
``cd $ECCE_HOME/bin && ./<app>``, which is also why the ``./eccejobmaster``
on that same command line resolves.  In a packaged install that directory
is root-owned, so the redirect could not be created, the shell exited
non-zero, and job launching failed for every code.

It had previously been ``> /dev/null 2>&1``, which is writable from
anywhere -- that is exactly what masked the assumption for years.

WHY THIS IS NARROW ON PURPOSE.  Plenty of commands in this tree redirect
to relative paths perfectly legitimately: ``JobParser`` builds
``... >> parseInDebug`` but calls ``localconn.cd(dir->path())`` first, so
its working directory is chosen rather than inherited.  Flagging those
would make the check noise, and a noisy check gets ignored.

So the signal is deliberately specific: a redirect whose target is a BARE
LITERAL FILENAME -- no directory separator, not /dev/null, not a variable
-- in a file that also calls ``system()``.  A bare literal is the case
where the author cannot have been thinking about the working directory at
all, because nothing in the expression mentions one.
"""

import os
import re

import codes as CODES

SRC = os.path.join(CODES.REPO, "src")

#  " > name.log"  /  " >> name.log"  /  " >& name.log"   as a C++ string
#  literal, where name.log contains no "/" and is not a device.
_REDIRECT = re.compile(r'"\s*>{1,2}&?\s*([A-Za-z0-9_.\-]+)\s*(?:2>&1)?\s*&?\s*"')
_ALLOWED = ("/dev/null",)


def _usesSystem(text):
    return re.search(r'\bsystem\s*\(', text) is not None


def check(report):
    for root, dirs, files in os.walk(SRC):
        dirs[:] = [d for d in dirs if d not in (".git",)]
        for name in sorted(files):
            if not name.endswith((".C", ".H")):
                continue
            path = os.path.join(root, name)
            try:
                text = open(path, encoding="utf-8", errors="replace").read()
            except OSError:
                continue
            if not _usesSystem(text):
                continue
            for num, line in enumerate(text.split("\n"), 1):
                stripped = line.strip()
                if stripped.startswith("//") or stripped.startswith("*"):
                    continue
                for m in _REDIRECT.finditer(line):
                    target = m.group(1)
                    if target in _ALLOWED or "/" in target:
                        continue
                    #  A bare word with no extension is usually not a
                    #  filename (e.g. a shell operator caught by accident).
                    if "." not in target:
                        continue
                    report("%s:%d" % (os.path.relpath(path, CODES.REPO), num),
                           "redirects to the bare filename %r in a file that "
                           "calls system().\n"
                           "      system() inherits the process's working "
                           "directory, which for a gateway-launched app is "
                           "$ECCE_HOME/bin -- root-owned in a packaged "
                           "install, so the redirect fails and the whole "
                           "command fails with it.\n"
                           "      Use an absolute path into a directory "
                           "known writable, or redirect to /dev/null if the "
                           "output is genuinely unwanted." % target)
