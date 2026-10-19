#!/usr/bin/env python3
"""Every tool a resource type enables has a ToolPropertyView for its content type.

The Organizer's summary panel looks the view up by tool and content type
(ResourceDescriptor::getToolPropertyViews); a missing one took the
Organizer down when a GROMACS Equilibrate task was selected, because
ecceMdEquilibrate had no views.  Checks both resource descriptors.

    tests/gromacs_md/tool_views_test.py
"""
import os
import sys
import xml.etree.ElementTree as ET

REPO = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
FILES = ("ResourceDescriptor.xml", "ResourceDescriptorRxn.xml")


def gaps(path):
    root = ET.parse(path).getroot()
    views = set((v.get("tool"), v.get("contentType") or "")
                for v in root.iter("ToolPropertyView"))
    found = []
    for rt in root.iter("ResourceType"):
        ct = rt.get("contentType") or ""
        tools = rt.find("EnabledTools")
        for t in (tools if tools is not None else []):
            if t.tag == "Tool" and (t.get("name"), ct) not in views:
                found.append("%s: %s enables %s, no view for content type %r"
                             % (os.path.basename(path), rt.get("name"),
                                t.get("name"), ct))
    return found


def main():
    found = []
    for name in FILES:
        found += gaps(os.path.join(REPO, "data", "client", "config", name))
    for f in found:
        print("FAIL  " + f)
    if not found:
        print("PASS  every enabled tool has a view (%s)" % ", ".join(FILES))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
