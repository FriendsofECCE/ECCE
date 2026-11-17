#!/usr/bin/env python3
"""File-type registrations of the packages (#1069): the sources parse and name
the same types, and nothing makes ECCE the default for .xyz/.pdb/.cif."""
import os
import plistlib
import sys
import xml.dom.minidom

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PK = os.path.join(ROOT, "packaging")
failures = []


def check(ok, what):
    print(("PASS  " if ok else "FAIL  ") + what)
    if not ok:
        failures.append(what)


def read(*p):
    with open(os.path.join(PK, *p), encoding="utf-8") as f:
        return f.read()


plist = plistlib.loads(read("macos", "Info.plist.in")
                       .replace("@VERSION@", "1").replace("@MINOS@", "11").encode())
exts = set(plist["CFBundleDocumentTypes"][0]["CFBundleTypeExtensions"])
check(exts == {"mvm", "xyz", "pdb", "cif"}, "Info.plist document types")
check(plist["CFBundleDocumentTypes"][0]["LSHandlerRank"] == "Alternate",
      "macOS: not the default handler")

xml.dom.minidom.parseString(read("desktop", "ecce-mvm.xml"))
check("*.mvm" in read("desktop", "ecce-mvm.xml"), "mime XML names .mvm")
desk = read("desktop", "ecce.desktop.in")
check("MimeType=" in desk and "application/x-ecce-mvm" in desk and " %F" in desk,
      ".desktop file: MimeType and %F")

msi = read("windows", "make-msi.ps1")
check("ECCE.Structure" in msi and "OpenWithProgids" in msi, "MSI: per-user ProgId")
for ext in ("xyz", "pdb", "cif"):
    check(('Classes\\.%s" ' % ext) not in msi
          and ("Classes\\.%s\\OpenWithProgids" % ext) in msi,
          "MSI: .%s only in Open with" % ext)
check('Root="HKLM"' not in msi, "MSI: file types are per user")
check("WScript.Arguments" in read("windows", "ecce.vbs"),
      "ecce.vbs passes the file name on")
sys.exit(1 if failures else 0)
