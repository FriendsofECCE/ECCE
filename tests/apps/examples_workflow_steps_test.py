#!/usr/bin/env python3
"""Every step and link of every shipped workflow.txt arrives (#954).

    tests/apps/examples_workflow_steps_test.py

The Workflows example set is imported twice: fresh, and as the update of an
older copy (the set project marked with an older example-set version, one
topic missing, as when a release adds a workflow), and of a copy whose
session lost the step most links start from (opened again, and at a
plain start of the same version).  After each, every
calculation a workflow.txt names must be a step of its session and every
link it lists must be there; a step that fails to import must leave a
message, never silence. Synthetic topics and sets reuse calculation names
to check that links, Hessians and Rate keys stay in their own workflow.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77
SET = os.path.join(HERE, "..", "..", "data", "client", "examples", "workflows")

failures = []


def check(what, cond, why, log=""):
    if cond:
        print("ok    " + what)
        return
    failures.append(what)
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def calc_of(topic_dir, folder):
    """The calculation name, as CalcMgr's collectExamples finds it."""
    if folder.startswith("gromacs-"):
        return folder[8:]
    files = os.listdir(os.path.join(topic_dir, folder))
    for ext in (".qmout", ".eprint", ".pwout", ".out"):
        names = sorted(f for f in files
                       if os.path.splitext(f[:-3] if f.endswith(".gz") else f)[1]
                       == ext and f != "g98.out")
        if names:
            name = names[0][:-3] if names[0].endswith(".gz") else names[0]
            return os.path.splitext(name)[0]
    return None


def expected():
    """{tree topic name: (set of calcs, set of "from>to:link")}"""
    topics = {}
    for row in open(os.path.join(SET, "set.txt")).read().splitlines()[2:]:
        folder, _, title = row.partition("|")
        tdir = os.path.join(SET, folder)
        if not os.path.exists(os.path.join(tdir, "workflow.txt")):
            continue
        calcs, links = set(), set()
        for line in open(os.path.join(tdir, "workflow.txt")):
            words = line.split()
            if not words or words[0].startswith("#") or words[0] == "!key":
                continue
            to = calc_of(tdir, words[0])
            calcs.add(to)
            for tok in words[2:]:
                tok = tok[:-5] if tok.endswith("+hess") else tok
                src, _, point = tok.partition("@")
                src, _, role = src.partition("=")
                name = (role or "geometry") + ("." + point if point else "")
                links.add("%s>%s:%s" % (calc_of(tdir, src), to, name))
        topics[title.replace(" ", "_")] = (calcs, links)
    return topics


def import_and_check(display, data, work, label, opened=True):
    root = os.path.join(data, "users", "local", "Examples", "Workflows")
    topics = expected()
    commands = (["examples workflows"] if opened else []) + [
        "workflowinfo file://%s/%s" % (root, t) for t in sorted(topics)]
    script = os.path.join(work, "commands")
    with open(script, "w") as handle:
        handle.write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=90,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_NO_EXAMPLE_IMPORT": ""})
    log = result.log or ""
    if result.crashed:
        return check(label, False, "CRASHED (%s)" % result.signalName, log)
    made = re.findall(r"ECCE_TEST_ORGANIZER: examples.*?: (.*)$", log, re.M)
    check("%s: the set opens without a message of failure" % label,
          not opened or (bool(made) and made[0].startswith("ok ")),
          "answered %r" % made, log)
    infos = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: (.*)$", log,
                       re.M)
    for (topic, (calcs, links)), info in zip(sorted(topics.items()),
                                              infos + [""] * len(topics)):
        if not info.startswith("ok "):
            check("%s: %s" % (label, topic), False, "workflowinfo %r" % info,
                  log)
            continue
        steps = {e.split()[0] for e in info[3:].split(" links:")[0].split("|")
                 if len(e.split()) >= 4}
        got = set(info.split(" links:")[1].split())
        check("%s: %s has every step of workflow.txt" % (label, topic),
              steps == calcs, "steps %s, expected %s"
              % (sorted(steps), sorted(calcs)))
        check("%s: %s has every link of workflow.txt" % (label, topic),
              got == links, "links %s, expected %s"
              % (sorted(got), sorted(links)))


def drop_source_step(data):
    """The state of a copy whose import lost one step: the step most links
    start from is gone, its session keeps the others and no link."""
    root = os.path.join(data, "users", "local", "Examples", "Workflows")
    topic = sorted(expected())[0]
    calcs, links = expected()[topic]
    starts = [l.split(">")[0] for l in links]
    lost = max(sorted(calcs), key=starts.count)
    shutil.rmtree(os.path.join(root, topic, lost))
    lb = os.path.join(root, topic, "linkbase.xml")
    text = open(lb).read()
    head = text[:text.index("<Task")]
    tasks = re.findall(r"<Task .*?</Task>", text, re.S)
    with open(lb, "w") as handle:
        handle.write(head + "\n\n".join(
            t for t in tasks if 'xlink:href="%s"' % lost not in t) +
            "\n\n</Session>\n")


def make_older(data):
    """The state an update starts from: the set project of an older
    example-set version, without its first workflow topic."""
    root = os.path.join(data, "users", "local", "Examples", "Workflows")
    first = sorted(expected())[0]
    shutil.rmtree(os.path.join(root, first))
    meta = os.path.join(root, ".ecce-meta")
    rows = open(meta).read().splitlines(True)
    with open(meta, "w") as handle:
        for r in rows:
            f = r.split("\t")
            if len(f) >= 4 and f[1].endswith(":exampleSet"):
                f[3] = "1\n"
            handle.write("\t".join(f))


def check_name_collisions(display, data, work):
    """Use a private examples overlay so the installed examples stay intact."""
    home = os.path.join(work, "collision-home")
    original = os.environ["ECCE_HOME"]
    for rel, excluded in (("", "data"), ("data", "client"),
                          ("data/client", "examples")):
        dest = os.path.join(home, rel)
        os.makedirs(dest, exist_ok=True)
        for name in os.listdir(os.path.join(original, rel)):
            if name != excluded:
                os.symlink(os.path.join(original, rel, name),
                           os.path.join(dest, name))
    examples = os.path.join(home, "data", "client", "examples")
    source = os.path.join(original, "data", "client", "examples", "workflows",
                          "aminopyridine-protonation")
    cases = (("collision-a", "Collision_A", "First", "geometry", True),
             ("collision-a", "Collision_A", "Second", "geometry.last", False),
             ("collision-b", "Collision_B", "Third", "geometry.max", False))
    for sid, title, topic, link, keyed in cases:
        folder = os.path.join(examples, sid, topic)
        os.makedirs(folder)
        manifest = os.path.join(examples, sid, "set.txt")
        if not os.path.exists(manifest):
            with open(manifest, "w") as handle:
                handle.write(title + "\nRepeated calculation names\n")
        with open(manifest, "a") as handle:
            handle.write(topic + "|" + topic + "\n")
        for calc in ("ap_amino_n", "ap_ring_n"):
            shutil.copytree(os.path.join(source, "orca-" + calc),
                            os.path.join(folder, "orca-" + calc))
        with open(os.path.join(folder, "workflow.txt"), "w") as handle:
            handle.write("orca-ap_amino_n\norca-ap_ring_n < orca-ap_amino_n" +
                         ("+hess" if keyed else "@" + link.split(".")[1]) + "\n")
            if keyed:
                handle.write("!key orca-ap_ring_n reactant thermo\n")
        # Attachment is a byte copy; no calculation is launched with this file.
        with open(os.path.join(folder, "orca-ap_amino_n", "ap_amino_n.hess"),
                  "w") as handle:
            handle.write("test Hessian attachment\n")
    root = os.path.join(data, "users", "local", "Examples")
    script = os.path.join(work, "collision-commands")
    with open(script, "w") as handle:
        handle.write("examples collision-a\nexamples collision-b\n")
        for _, title, topic, _, _ in cases:
            handle.write("workflowinfo file://%s/%s/%s\n" % (root, title, topic))
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script, "ECCE_HOME": home})
    log = result.log or ""
    answers = re.findall(r"ECCE_TEST_ORGANIZER: examples.*?: (.*)$", log, re.M)
    check("collision sets imported", not result.crashed and len(answers) == 2
          and all(a.startswith("ok ") for a in answers), repr(answers), log)
    infos = re.findall(r"ECCE_TEST_ORGANIZER: workflowinfo.*?: (.*)$", log, re.M)
    for (_, title, topic, link, keyed), info in zip(cases, infos + [""] * 3):
        links = info.partition(" links:")[2].split()
        check(topic + ": only its own link", info.startswith("ok ") and
              links == ["ap_amino_n>ap_ring_n:" + link], repr(info), log)
        step = os.path.join(root, title, topic, "ap_ring_n")
        props = {}
        meta = os.path.join(step, ".ecce-meta")
        if os.path.isfile(meta):
            for row in open(meta):
                fields = row.rstrip("\n").split("\t")
                if len(fields) >= 4 and fields[0] == ".":
                    props[fields[1].rsplit(":", 1)[-1]] = fields[3]
        keys = (props.get("recipe.key"), props.get("recipe.thermo"))
        check(topic + ": only its own Rate keys",
              keys == (("reactant", "1") if keyed else (None, None)), repr(keys))
        hessian = os.path.join(step, "Inputs", "inhess.hess")
        check(topic + ": only its own Hessian",
              os.path.isfile(hessian) == keyed, hessian)


def main():
    msg = apps.versionMismatch()
    if msg:
        print(msg)
        return SKIP
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="examples-steps-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            import_and_check(display, data, work, "fresh")
            if os.path.isdir(os.path.join(data, "users", "local", "Examples",
                                          "Workflows")):
                make_older(data)
                import_and_check(display, data, work, "update")
                drop_source_step(data)
                make_older(data)
                import_and_check(display, data, work, "update, step lost")
                # same version, set not opened: the start repairs it
                drop_source_step(data)
                import_and_check(display, data, work, "start, step lost",
                                 opened=False)
            check_name_collisions(display, data, work)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
