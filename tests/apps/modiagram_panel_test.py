#!/usr/bin/env python3
"""The MO diagram in the Builder, headless: clicks, tooltips, composition.

    tests/apps/modiagram_panel_test.py

Water (calc-orca-water-ccpvdz), with only the MO Diagram panel opened:

  * a click on a level draws that orbital although the MOs panel was never
    opened (its first-time initialisation is done by the click);
  * a level's tooltip text names the MO, its energy, irrep and occupancy and
    carries the MOs panel's composition line ("MO n, composition (Mulliken
    population): ..."); a degenerate set names every MO and says the
    composition is their mean.

ECCE-QM HF (#745): every correlation line ends on its molecular level's bar,
however long the construction label between them.

ECCE-QM O2 with localised orbitals (#745): with "Localised MO" chosen in the
MOs panel the diagram's status line says it shows the canonical MOs, and a
click on a level switches the list back to the canonical set.

Same installed tree and isolation as orca_mo_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else None


BENZENE = os.path.join(os.path.dirname(HERE), "mocomp", "fixtures", "benzene")
#  ECCE-QM ROKS O2 with localised orbitals stored (LOCMO*).
O2LOC = os.path.join(os.path.dirname(HERE), "ecceqm", "fixtures", "o2-roks-loc")


def makeBenzene(data):
    """Benzene (D6h, doubly degenerate e levels)."""
    return makeFrom(data, "benzene", BENZENE)


def makeFrom(data, name, source):
    """The water calculation's shell with <source>'s geometry, basis and
    stored properties put in."""
    BENZENE = source
    url = base.makeCalc(data, name)
    calc = url[len("file://"):].rstrip("/")
    for name in ("chemsys.mvm", "BasisSet.ecce_basisset"):
        shutil.copy(os.path.join(BENZENE, "Parameters", name),
                    os.path.join(calc, "Parameters", name))
    for name in os.listdir(os.path.join(calc, "Props")):
        if not name.startswith("."):
            os.unlink(os.path.join(calc, "Props", name))
    for name in os.listdir(os.path.join(BENZENE, "Props")):
        if not name.startswith("."):
            shutil.copy(os.path.join(BENZENE, "Props", name),
                        os.path.join(calc, "Props", name))
    return url


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="modiagram-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "water")
            out = os.path.join(work, "run")
            scene = "hold 3\n"
            for n in range(0, 8):
                scene += "modiagram tip%d tip %d\n" % (n, n)
                scene += "modiagram ctip%d ctip %d\n" % (n, n)
            scene += "modiagram click click 4\nmogrid grid\n"
            result = base.runBuilder(display, url, out, scene,
                                     panel="MO Diagram")
            log = result.log or ""
            failed = os.path.join(out, "FAILED")
            check(not result.crashed, "the Builder did not crash")
            check(not os.path.exists(failed), "the scene ran to the end: %s" % (
                open(failed).read() if os.path.exists(failed) else ""))
            click = text(out, "click") or ""
            check("grid yes" in click,
                  "a click draws the orbital without the MOs panel opened "
                  "first (%s)" % click.strip())
            check("cannot be computed" not in log,
                  "no 'cannot be computed' message")
            tips = [text(out, "ctip%d" % n) or "" for n in range(0, 8)]
            mtips = [text(out, "tip%d" % n) or "" for n in range(0, 8)]
            check(any("Made of:" in t and "Matches MO" in t for t in mtips),
                  "a model level's hover gives its fragment composition and its match")
            check(any("matches model" in t for t in tips),
                  "a calculation level's hover names its model partner")
            shown = [t for t in tips if t.strip()]
            check(len(shown) >= 3, "level tooltips exist (%d)" % len(shown))
            heads = [t.splitlines()[0] for t in shown]
            check(all(("MO " in h or "MOs " in h) and "E = " in h and
                      "occupancy" in h for h in heads),
                  "every tooltip head names MO, energy, occupancy")
            check(all("composition (Mulliken population):" in t for t in shown),
                  "every tooltip carries the MOs panel's composition line")
            check(any("O 2p" in t or "O 1s" in t for t in shown), "the composition names O with its shell (O 2p)")
            degenerate = [t for t in shown if t.startswith("MOs ")]
            print("tooltips:\n" + "\n".join(shown[:3]))
            if degenerate:
                check("mean over its orbitals" in degenerate[0],
                      "a degenerate set says its composition is a mean")
            url = makeBenzene(data)
            out = os.path.join(work, "benzene")
            scene = "hold 3\n"
            for n in range(0, 30):
                scene += "modiagram tip%d ctip %d\n" % (n, n)
            res = base.runBuilder(display, url, out, scene, panel="MO Diagram")
            tips = [text(out, "tip%d" % n) or "" for n in range(0, 30)]
            if not any(t.strip() for t in tips):
                print("no benzene tooltips; %s\n%s" % (
                    text(out, "FAILED"), "\n".join(
                        (res.log or "").splitlines()[-15:])))
            print("benzene tooltips:\n" + "\n".join(
                t.splitlines()[0] for t in tips if t.strip()))
            pairs = [t for t in tips if t.startswith("MOs ")]
            print("benzene, degenerate sets:\n" + "\n".join(pairs[:2]))
            check(len(pairs) >= 1, "benzene has a degenerate level in the diagram "
                  "(%d)" % len(pairs))
            check(all("mean over its orbitals" in t and
                      "composition (Mulliken population):" in t
                      for t in pairs),
                  "a degenerate set names its MOs and gives the mean "
                  "composition")
            #  #745: HF's construction labels are long; every correlation
            #  line must still end on its level's bar.
            hfdir = os.path.join(os.path.dirname(HERE), "modiagram", "fixtures", "ecceqm-hf")
            url = makeFrom(data, "hf", hfdir)
            out = os.path.join(work, "hf")
            res = base.runBuilder(display, url, out, "hold 3\nmodiagram links links 0\n",
                                  panel="MO Diagram")
            links = text(out, "links") or ""
            gaps = [int(l.split()[-1]) for l in links.splitlines() if " gap " in l]
            print("HF links:\n" + links.strip())
            check(any(" left " in l for l in links.splitlines()) and
                  any(" right " in l for l in links.splitlines()),
                  "HF draws correlation lines from both atoms (%d)" % len(gaps))
            check(gaps and max(gaps) <= 1,
                  "every HF correlation line reaches its level's bar (gaps %s)" % gaps)
            #  #827: labels carry the principal quantum number; the core
            #  level is drawn below the break on request.
            scene = ("hold 3\nmotable mot\nmodiagram coreon core 1\n"
                     "modiagram corehit corehits 0\n"
                     "modiagram coreoff core 0\nmodiagram tip0 tip 0\n")
            res = base.runBuilder(display, url, out, scene, panel="MO Diagram")
            mot = text(out, "mot") or ""
            rows = {}
            for l in mot.splitlines():
                if l.startswith("row "):
                    cells = [c.strip() for c in l[4:].split("|")]
                    rows[cells[0]] = l
            print("HF MOs table, MO 1 and 2:\n%s\n%s" % (rows.get("1"), rows.get("2")))
            check("F 1s" in rows.get("1", "") and "F2" not in rows.get("1", ""),
                  "HF MO 1 is labelled F 1s in the MOs table")
            check("F 2s" in rows.get("2", ""),
                  "HF MO 2 is labelled F 2s in the MOs table")
            on = text(out, "coreon") or ""
            off = text(out, "coreoff") or ""
            check("core F 1s" in on and "nonbonding" in on,
                  "Show core orbitals: an F 1s non-bonding level is drawn below the break (%r)" % on.strip())
            check(off.strip() == "", "Show core orbitals off: no core level")
            ch = text(out, "corehit") or ""
            check("corehit MO 1 |" in ch and "core" in ch.split("|", 1)[-1],
                  "core levels are clickable: a hit box for MO 1 with its tip (%r)" % ch.strip()[:200])
            check("F 2p" in (text(out, "tip0") or "") or "F 2s" in (text(out, "tip0") or ""),
                  "a level's tooltip names its shells with n")
            url = makeFrom(data, "ecceqm-water",
                           os.path.join(os.path.dirname(HERE), "modiagram", "fixtures", "ecceqm-water"))
            out = os.path.join(work, "water827")
            res = base.runBuilder(display, url, out, "hold 3\nmotable mot\n", panel="MO Diagram")
            mot = text(out, "mot") or ""
            check("H(1) 1s" in mot and "H(2) 1s" in mot and "O 1s" in mot,
                  "water: H(1) 1s, H(2) 1s and O 1s in the MOs table")
            #  #745: the diagram is canonical; with the localised set listed
            #  it says so, and a click on a level brings the canonical list back.
            url = base.makeCalc(data, "o2loc", source=O2LOC)
            out = os.path.join(work, "o2loc")
            scene = ("hold 3\nmodiagram s0 status 0\nproperty MOs\nhold 2\n"
                     "mofield Localised MO\nmodiagram s1 status 0\n"
                     "modiagram c click 3\nmodiagram s2 status 0\n")
            res = base.runBuilder(display, url, out, scene, panel="MO Diagram")
            if os.path.exists(os.path.join(out, "FAILED")):
                print("o2loc: %s\n%s" % (text(out, "FAILED"), "\n".join(
                    (res.log or "").splitlines()[-15:])))
            s0, s1, s2 = (text(out, n) or "" for n in ("s0", "s1", "s2"))
            print("status lines: %r %r %r" % (s0.strip(), s1.strip(), s2.strip()))
            check(s0.strip() == "status", "no status line with the canonical MOs listed")
            check("canonical MOs" in s1 and "localised" in s1,
                  "with Localised MO chosen the diagram says it shows the canonical MOs")
            check(s2.strip() == "status" and "grid yes" in (text(out, "c") or ""),
                  "a click on a level draws it and the MOs panel lists the canonical set again")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
