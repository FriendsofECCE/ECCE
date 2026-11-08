#!/usr/bin/env python3
"""Reaction rates in the GUI (#872), headless.

The SN2 workflow example carries the keys reactant, ts and product (workflow.txt
"!key" lines), so its Rate box must show what `ecce-rate --json` prints for the
same calculations, at 298.15 K and at a T typed into the box; the Details
table's CSV must equal the CLI's temperature rows.  The Reaction overview with
`reactant -> [ts] -> product` must list the same k, draw an energy profile with
the barrier dG of ecce-rate and no label touching a line or another label (also
with two [ts] reactions); a reaction without [ts] must not get rate columns.
ECCE_RATE_SHOTS=<dir> keeps the PNGs.  Same install and isolation as
examples_workflow_test.py; exit 77 (skip) without an install or Xvfb.
"""
import csv
import io
import json
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
import profile_layout  # noqa: E402
import xdisplay   # noqa: E402

failures = []


def check(what, cond, why=""):
    print(("ok    " if cond else "FAIL  ") + what + ("" if cond else "\n      " + str(why)[:600]))
    if not cond:
        failures.append(what)
    return cond


def cli(args):
    env = dict(os.environ, ECCE_HOME=apps.INSTALL)
    r = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-rate")] + args + ["--json"],
                       env=env, capture_output=True, text=True)
    return json.loads(r.stdout) if r.returncode == 0 else None


def sci(v):
    """The GUI's number form: 7.971 × 10⁷, plain digits between 0.01 and 10⁴."""
    if v == 0 or 0.01 <= abs(v) < 1e4:
        return "%.4g" % v
    m, e = ("%.3e" % v).split("e")
    return "%s × 10%s" % (m, str(int(e)).translate(str.maketrans("-0123456789", "⁻⁰¹²³⁴⁵⁶⁷⁸⁹")))


def run(display, data, work, shots):
    root = os.path.join(data, "users", "local", "Examples", "Workflows", "SN2_Cl-_and_CH3Br")
    wf = "file://" + root
    re_, ts, pr = (os.path.join(root, n) for n in ("sn2_reactant", "sn2_qst3", "sn2_product"))
    png = os.path.join(shots, "rate-box.png")
    csv1 = os.path.join(work, "details.csv")
    rx = os.path.join(work, "reactions.txt")
    with open(rx, "w") as f:
        f.write("file://%s -> [file://%s] -> file://%s\nfile://%s -> file://%s\n" % (re_, ts, pr, re_, pr))
    png2 = os.path.join(shots, "reaction-overview-ts.png")
    csv2 = os.path.join(work, "overview.csv")
    rx2 = os.path.join(work, "reactions2.txt")
    with open(rx2, "w") as f:
        f.write("file://%s -> [file://%s] -> file://%s\nfile://%s -> [file://%s] -> file://%s\n"
                % (re_, ts, pr, pr, ts, re_))
    png3 = os.path.join(shots, "reaction-overview-two-ts.png")
    # Bare names in the Test > Reaction overview folder (#885): both reactions
    # of its seven calculations draw a profile, a missing name is refused.
    tdir = os.path.join(data, "users", "local", "Examples", "Test", "Reaction_overview")
    rx3 = os.path.join(work, "reactions3.txt")
    with open(rx3, "w") as f:
        f.write("# base file://%s\nreactant_complex -> [sn2_ts] -> product_complex\n"
                "butadiene, ethylene -> [diels_alder_ts] -> cyclohexene\n" % tdir)
    shot3 = os.path.join(os.environ.get("ECCE_RATE_SHOTS") or work, "overview-two-ts.png")
    shot4 = os.path.join(os.environ.get("ECCE_RATE_SHOTS") or work, "900-impl.png")
    rx4 = os.path.join(work, "reactions4.txt")
    with open(rx4, "w") as f:
        f.write("# base file://%s\nbutadiene, ethylene -> [sn2_qts3] -> cyclohexene\n" % tdir)
    commands = ["examples workflows", "examples test",
                "workflowrate %s png=%s csv=%s range=250:400:25" % (wf, png, csv1),
                "workflowrate %s T=310" % wf,
                "reactionoverview %s qrrho-grimme:100 %s %s 298.15" % (rx, png2, csv2),
                "reactionoverview %s qrrho-grimme:100 %s - 298.15" % (rx2, png3),
                "reactionoverview %s qrrho-grimme:100 - - 298.15 %s" % (rx3, shot3),
                "reactionoverview %s qrrho-grimme:100" % rx4,
                "reactionoverviewui %s qrrho-grimme:100 %s add=2*sn2_reactant->sn2_product add=sn2_reactant->sn2_product remove=1"
                % (wf, shot4)]
    script = os.path.join(work, "commands")
    open(script, "w").write("\n".join(commands) + "\n")
    result = apps.run(display, "organizer", windowTimeout=60, settle=120,
                      env={"ECCE_TEST_ORGANIZER": script, "ECCE_NO_EXAMPLE_IMPORT": ""})
    log = result.log or ""
    if result.crashed:
        return check("the Organizer ran", False, "CRASHED (%s)\n%s" % (result.signalName, log[-1500:]))

    def answers(prefix):
        return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix), log, re.M)

    rate = answers("workflowrate")
    if not check("two workflowrate answers", len(rate) == 2 and all(a.startswith("ok ") for a in rate),
                 "%r\n%s" % (rate, log[-1500:])):
        return
    ref = cli([re_, ts, pr, "--T", "298.15"])
    ref310 = cli([re_, ts, pr, "--T", "310"])
    check("ecce-rate runs on the example calculations", ref is not None and ref310 is not None)
    if ref is None or ref310 is None:
        return
    row, row310 = ref["temperatures"][0], ref310["temperatures"][0]

    def box(text):
        d = {}
        for l in text[3:].split("|"):
            if ": " in l:
                k, v = l.split(": ", 1)
                d[k] = v
        return d
    b = box(rate[0])
    check("Rate box: dG at 298.15 K", any("298.15 K" in k and v.startswith("%.2f kJ/mol" % row["dG_act"])
                                         for k, v in b.items()), b)
    check("Rate box: k TST equals ecce-rate", b.get("k (TST)", "").startswith(sci(row["k"]["none"])), b)
    check("Rate box: Wigner and Eckart k equal ecce-rate",
          any(k.startswith("κ Wigner") and v.startswith("k " + sci(row["k"]["wigner"])) for k, v in b.items()) and
          any(k.startswith("κ Eckart") and v.startswith("k " + sci(row["k"]["eckart"])) for k, v in b.items()), b)
    check("Rate box: product given, so reverse k and K",
          b.get("k reverse (Eckart)", "").startswith(sci(row["k_reverse"]["eckart"])) and
          b.get("K (equilibrium)", "") == sci(row["K_eq"]), b)
    check("Rate box: treatment and standard state shown", "qrrho-grimme:100, 1 M standard state" in rate[0], rate[0])
    b2 = box(rate[1])
    check("Rate box at T = 310 K equals ecce-rate",
          b2.get("k (TST)", "").startswith(sci(row310["k"]["none"])), b2)
    got = answers("workflowrate")[0]
    check("details CSV written", "csv saved" in got and "png saved" in got, got[-200:])
    full = cli([re_, ts, pr, "--T", "250:400:25"])
    try:
        rows = list(csv.DictReader(open(csv1).read().split("\n\n")[0].splitlines()))
        ok = len(rows) == len(full["temperatures"]) and all(
            abs(float(r["k_eckart"]) / t["k"]["eckart"] - 1) < 1e-4 and
            abs(float(r["dG_act_kJ_mol"]) - t["dG_act"]) < 1e-3
            for r, t in zip(rows, full["temperatures"]))
    except Exception as e:  # noqa
        ok, rows = False, str(e)
    check("details CSV equals the CLI's 250-400 K rows", ok, rows)
    check("Rate box PNG exists", os.path.exists(png) and os.path.getsize(png) > 5000, png)

    ui = answers("reactionoverviewui")
    if check("picker overview answered", len(ui) == 1 and ui[0].startswith("ok"), ui):
        u = ui[0].split(" | ")
        l0 = [l[7:] for l in u if l.startswith("lines0 ")]
        l1 = [l[6:] for l in u if l.startswith("lines ")]
        check("auto-fill: reactant -> [ts] -> product from the workflow's roles",
              l0 == ["sn2_reactant -> [sn2_qst3] -> sn2_product"], l0)
        t0 = [l for l in u if l.startswith("table0 reaction 1 ")]
        check("auto-fill: dE -9.61 kJ/mol", t0 and abs(float(t0[0].split()[3]) + 9.61) < 0.01, t0)
        check("two adds then remove row 1 leave the unbalanced and the balanced reaction",
              l1 == ["2*sn2_reactant -> sn2_product", "sn2_reactant -> sn2_product"], l1)
        uh = "\n".join(l[5:] for l in u if l.startswith("html "))
        check("an unbalanced reaction is one warning line in the formatted details",
              "Not balanced" in uh and "#a02020" in uh, uh[:300])
        bal = [l[8:] for l in u if l.startswith("balance ")]
        check("an unbalanced reaction is refused with the atoms of both sides",
              len(bal) == 2 and bal[0].startswith("1 Not balanced: left ") and ", right " in bal[0]
              and bal[1] == "2 balanced", bal)
        t1 = [l for l in u if l.startswith("table reaction ")]
        check("only the balanced reaction has energies (dE -9.61 kJ/mol)",
              len(t1) == 1 and t1[0].startswith("table reaction 2 ") and abs(float(t1[0].split()[3]) + 9.61) < 0.01, t1)
        check("TS choices: only calculations with one imaginary frequency enabled",
              "ts sn2_qst3:1:1" in u and "ts sn2_reactant:0:0" in u, [l for l in u if l.startswith("ts ")])
        check("dialog picture 900-impl.png written", os.path.exists(shot4) and os.path.getsize(shot4) > 5000, shot4)

    ov = answers("reactionoverview ")
    if len(ov) == 4:
        refused = ov.pop()
        names = ov.pop().split(" | ")
        check("bare names: two profiles, the first two reactions",
              any(l.startswith("profile 1 1 ts ") for l in names) and
              any(l.startswith("profile 2 2 ts ") for l in names), names[:20])
        check("bare names: an unknown calculation is refused by name",
              "no calculation 'sn2_qts3' in this folder" in refused, refused[:600])
    if not check("overview answered", len(ov) == 2 and all(o.startswith("ok") for o in ov),
                 "%r\n%s" % (ov, log[-1200:])):
        return
    lines = ov[0].split(" | ")
    prof = {l.split()[3]: l.split()[4:] for l in lines if l.startswith("profile 1 1 ")}
    check("overview: one energy profile, for reaction 1 only",
          len(prof) == 3 and not any(l.startswith("profile 2 ") for l in lines), lines[:20])
    check("overview profile: TS dG is the barrier dG of ecce-rate",
          "ts" in prof and abs(float(prof["ts"][1]) - row["dG_act"]) < 1e-2, prof)
    check("overview profile: reactants at zero",
          prof.get("reactant") == ["0.000", "0.000"], prof)
    click = " ".join(lines[lines.index("Click 1:") + 1:]) if "Click 1:" in lines else ""
    check("the copyable text carries the profile numbers",
          "Energy profile:" in click and "%.2f" % row["dG_act"] in click, click[:600])
    bad = profile_layout.overlaps(lines)
    check("SN2 profile: no label touches a line or another label", not bad, "; ".join(bad[:8]))
    two = ov[1].split(" | ")
    lay = dict(kv.split("=", 1) for l in two if l.startswith("layout: ") for kv in l[8:].split())
    h = lambda k: int(lay.get(k, "0x0").split("x")[1])
    check("two [ts] reactions: the chart and the choice are shown, chart taller than 50 px",
          lay.get("plotshown") == "1" and lay.get("choiceshown") == "1" and h("plot") > 50 and h("choice") > 0, lay)
    check("the G treatment choice is enabled", lay.get("treatmentenabled") == "1", lay)
    check("the details page is enabled and has room",
          lay.get("detailenabled") == "1" and h("detail") >= 100, lay)
    html2 = "\n".join(l[5:] for l in two if l.startswith("html "))
    check("formatted details: the G treatment in words, not the internal key",
          "Grimme quasi-RRHO" in html2 and "qrrho-grimme" not in html2, html2[:300])
    check("formatted details: tables, tunnelling and superscripts",
          "<table" in html2 and "<h4>Reaction 1:" in html2 and "Tunnelling" in html2 and "<sup>" in html2, html2[:600])
    refs = html2[html2.find("<h4>References"):] if "<h4>References" in html2 else ""
    check("References name Eyring, Wigner, Eckart and Grimme for the TS reaction",
          all(n in refs for n in ("Eyring", "Wigner", "Eckart", "Grimme")) and "Truhlar" not in refs, refs[:500])
    check("dialog screenshot written", os.path.getsize(shot3) > 5000 if os.path.exists(shot3) else False, shot3)
    check("two [ts] reactions: two profiles",
          any(l.startswith("profile 2 2 ts ") for l in two) and
          len([l for l in two if l.startswith("layout 2 size")]) == 1, two[:20])
    bad = profile_layout.overlaps(two)
    check("two profiles: no label touches a line or another label", not bad, "; ".join(bad[:8]))
    check("two profiles: PNG written", "png saved" in two and os.path.getsize(png3) > 5000, png3)
    i = lines.index("Rates:") if "Rates:" in lines else -1
    rt = " ".join(lines[i + 1:]) if i >= 0 else ""
    check("overview lists the same k (TST, Wigner, Eckart)",
          sci(row["k"]["none"]) in rt and sci(row["k"]["wigner"]) in rt and sci(row["k"]["eckart"]) in rt, rt)
    check("overview lists the kappas", "kappa Wigner %.3f" % row["kappa"]["wigner"] in rt, rt)
    check("the reaction without [ts] has no rate lines", not re.search(r"\b2: dE", rt), rt)
    try:
        crows = list(csv.DictReader(open(csv2)))
        ok = (len(crows) == 2 and abs(float(crows[0]["k_eckart"]) / row["k"]["eckart"] - 1) < 1e-3 and
              crows[1]["k_tst"] == "" and "dG_act_kJ_mol" in crows[0])
    except Exception as e:  # noqa
        ok, crows = False, str(e)
    check("overview CSV has the rate columns, empty for the plain reaction", ok, crows)
    check("overview PNG exists", os.path.exists(png2) and os.path.getsize(png2) > 3000, png2)


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="rate-gui-", dir=os.environ.get("ECCE_REALUSERHOME"))
    shots = os.environ.get("ECCE_RATE_SHOTS") or os.path.join(work, "shots")
    os.makedirs(shots, exist_ok=True)
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work, shots)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
