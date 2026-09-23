"""
Checks on siteconfig/, the hand-maintained files that decide which machines
exist and how their jobs get submitted.

Nothing here needs a display, a dialog or a build -- it is pure file
consistency, in the same spirit as resources.py.  The checks exist because
every failure mode below is silent or fatal-at-runtime rather than caught
when the file is edited:

  * RefMachine::parseFile() splits on tabs and asserts EE_FATAL on an empty
    field, so a machine line with a space where a tab should be takes the
    whole application down on startup, not just that machine;
  * QueueMgr's loader does EE_RT_ASSERT(found, EE_FATAL, name +
    "|queueMgrName: not found!"), so naming a machine in `Queues:` without
    giving it a queueMgrName is fatal -- and naming one that is not
    registered at all is what example-slurm.Q already warns about;
  * JobParser::importJob() stops at the FIRST code whose verifypattern
    matches the user's file, iterating in directory-scan order, so two codes
    sharing a pattern means imports land on whichever the filesystem
    happened to return first.  That was live between NWChem and MetaDyn.
"""

import os
import re

import codes as CODES

SITECONFIG = os.path.join(CODES.REPO, "siteconfig")
CAP = os.path.join(CODES.REPO, "data", "client", "cap")

#  name  fullname  vendor  model  processor  procs:nodes  remShell  codes  options
MACHINE_FIELDS = 9


def _lines(path):
    """Yield (lineno, text) for content lines, matching LineReader's '#'."""
    if not os.path.exists(path):
        return
    with open(path) as handle:
        for num, raw in enumerate(handle, 1):
            text = raw.split("#", 1)[0].rstrip("\n")
            if text.strip():
                yield num, text


def machines():
    """Machine name -> its fields, from siteconfig/Machines."""
    found = {}
    for num, text in _lines(os.path.join(SITECONFIG, "Machines")):
        fields = text.split("\t")
        found[fields[0]] = (num, fields)
    return found


def dataServerPaths(report):
    """Filesystem paths in siteconfig/DataServers must resolve.

    The <ForceField> block names NWChem's AMBER parameter files and segment
    directories, and it still pointed at
    $ECCE_HOME/nwchem/usr.local.lib.nwchem/ -- a path that existed only
    when ECCE SHIPPED ITS OWN NWCHEM.  Nothing reports a missing force
    field; MD setup simply reads from a directory that is not there.

    CMakeLists.txt rewrites this file at configure time (the PNNL hostname
    and now these paths too), so the GENERATED copy is what ships and what
    is checked.  Skipped when there is no build directory, and skipped for
    paths under /usr/share/nwchem when nwchem is not installed -- it is
    only Recommends, and its absence is a packaging choice rather than a
    stale path.
    """
    generated = os.path.join(CODES.REPO, "build-cmake", "siteconfig-local",
                             "DataServers")
    if not os.path.exists(generated):
        return
    with open(generated) as handle:
        text = handle.read()

    nwchemInstalled = os.path.isdir("/usr/share/nwchem")
    for match in re.finditer(r"<(?:ParamFile|SegmentDir)>([^<]+)<", text):
        path = match.group(1).strip()
        if path.startswith("$ECCE_HOME"):
            report("siteconfig/DataServers",
                   "%r is still an $ECCE_HOME path.\n"
                   "      Anything under $ECCE_HOME/nwchem/ is residue from "
                   "when ECCE bundled its own NWChem and does not exist in "
                   "this package -- the force field silently reads from a "
                   "missing directory." % path)
            continue
        if path.startswith("/usr/share/nwchem") and not nwchemInstalled:
            continue
        if not os.path.exists(path):
            report("siteconfig/DataServers",
                   "%r does not exist." % path)


def check(report):
    machs = machines()
    dataServerPaths(report)

    # (a) every machine line must have all nine tab-separated fields
    for name, (num, fields) in sorted(machs.items()):
        if len(fields) != MACHINE_FIELDS:
            report("siteconfig/Machines:%d" % num,
                   "%r has %d tab-separated fields, expected %d.\n"
                   "      RefMachine::parseFile() asserts EE_FATAL on an "
                   "empty field, so this aborts the application on startup "
                   "rather than just omitting the machine. The usual cause "
                   "is a space where a tab should be."
                   % (name, len(fields), MACHINE_FIELDS))
            continue
        if not re.match(r"^\d+:\d+$", fields[5]):
            report("siteconfig/Machines:%d" % num,
                   "%r has processor field %r, expected \"procs:nodes\"."
                   % (name, fields[5]))

    # (b) Queues: must name registered machines, and give each a manager
    queuePath = os.path.join(SITECONFIG, "Queues")
    listed, managers, prefFiles = [], {}, {}
    for num, text in _lines(queuePath):
        if text.startswith("Queues:"):
            listed.extend(text.split(":", 1)[1].split())
        else:
            match = re.match(r"^\s*(\S+)\|(\w+):\s*(\S+)", text)
            if match:
                name, key, value = match.groups()
                if key == "queueMgrName":
                    managers[name] = (num, value)
                elif key == "prefFile":
                    prefFiles[name] = (num, value)

    knownManagers = set()
    for _, text in _lines(os.path.join(SITECONFIG, "QueueManagers")):
        if text.startswith("QueueManagers:"):
            knownManagers.update(
                t for t in text.split(":", 1)[1].replace("\\", " ").split())
        else:
            match = re.match(r"^\s*(\w[\w.-]*)\|", text)
            if match:
                knownManagers.add(match.group(1))

    for name in listed:
        if name not in machs:
            report("siteconfig/Queues",
                   "%r is in the Queues list but is not registered in "
                   "siteconfig/Machines.\n"
                   "      A queue entry naming an unregistered machine is a "
                   "FATAL error at startup, not a warning." % name)
        if name not in managers:
            report("siteconfig/Queues",
                   "%r has no queueMgrName.\n"
                   "      QueueMgr does EE_RT_ASSERT(found, EE_FATAL) on "
                   "this, so it aborts rather than defaulting." % name)

    for name, (num, manager) in sorted(managers.items()):
        if name not in listed:
            report("siteconfig/Queues:%d" % num,
                   "%r is configured but missing from the Queues: list, so "
                   "it is never loaded." % name)
        if knownManagers and manager not in knownManagers:
            report("siteconfig/Queues:%d" % num,
                   "%r uses queue manager %r, which siteconfig/QueueManagers "
                   "does not define." % (name, manager))
        #  QueueMgr skips the prefFile entirely for "Shell"
        if manager != "Shell":
            if name not in prefFiles:
                report("siteconfig/Queues:%d" % num,
                       "%r uses %r but has no prefFile; only \"Shell\" may "
                       "omit one." % (name, manager))
            else:
                pnum, pref = prefFiles[name]
                if not os.path.exists(os.path.join(SITECONFIG, pref)):
                    report("siteconfig/Queues:%d" % pnum,
                           "%r names prefFile %r, which does not exist in "
                           "siteconfig/." % (name, pref))

    # (c) a machine flagged noRemoteAccess must still give every code it
    #     claims an executable path, or gensub dies at submit-script time
    for name, (num, fields) in sorted(machs.items()):
        config = os.path.join(SITECONFIG, "CONFIG." + name)
        if not os.path.exists(config):
            continue
        body = open(config).read()
        if not re.search(r"^\s*noRemoteAccess\s*:\s*(true|yes)", body,
                         re.I | re.M):
            continue
        declared = [c for c in fields[7].split(":") if c]
        for code in declared:
            if not re.search(r"^\s*%s\s*:" % re.escape(code), body, re.M):
                report("siteconfig/CONFIG." + name,
                       "machine offers %s but the config gives it no "
                       "executable path.\n"
                       "      gensub dies with \"Path for %s not found\" "
                       "even though a noRemoteAccess machine never runs it."
                       % (code, code.lower()))

    # (d) no two codes may share an import verifypattern
    patterns = {}
    for entry in sorted(os.listdir(CAP)):
        if not entry.endswith(".edml"):
            continue
        body = open(os.path.join(CAP, entry)).read()
        if "<Importer>" not in body:
            continue
        for pattern in set(re.findall(r'verifypattern="([^"]*)"', body)):
            if pattern:
                patterns.setdefault(pattern, set()).add(entry)
    for pattern, owners in sorted(patterns.items()):
        if len(owners) > 1:
            report("data/client/cap",
                   "verifypattern %r is claimed by %s.\n"
                   "      JobParser::importJob() stops at the FIRST code "
                   "whose pattern matches, iterating in directory-scan "
                   "order, so an import lands on whichever the filesystem "
                   "returns first." % (pattern, ", ".join(sorted(owners))))
