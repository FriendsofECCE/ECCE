#!/usr/bin/env python3
"""Every command AGENTS.md names under "Commands" exists in the build, and
the skill folder points at AGENTS.md (#759).  usage: agents_md_test.py SRC BUILD"""
import os, re, sys

src, build = sys.argv[1], sys.argv[2]
text = open(os.path.join(src, "docs", "AGENTS.md")).read()
section = text.split("\n## Commands\n", 1)[1].split("\n## ", 1)[0]
names = re.findall(r"^### (ecce-[a-z-]+)$", section, re.M)
bad = 0
if len(names) < 5:
    print("expected the five commands under '## Commands', found", names)
    bad += 1
for n in names:
    if n == "ecce-contacts":
        ok = os.path.exists(os.path.join(src, "scripts", "ecce_contacts.py"))
    else:
        ok = os.path.exists(os.path.join(build, "wrappers", n))
    print(("ok      " if ok else "MISSING ") + n)
    bad += not ok
# the usage line under each heading starts with the command itself
for n in names:
    if not re.search(r"^    " + n + r"\b", section, re.M):
        print("no usage line for", n)
        bad += 1
skill = open(os.path.join(src, "docs", "ai", "skills", "ecce", "SKILL.md")).read()
if not skill.startswith("---\nname: ecce\n") or "AGENTS.md" not in skill:
    print("SKILL.md frontmatter or pointer missing")
    bad += 1
sys.exit(1 if bad else 0)
