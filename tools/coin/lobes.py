#!/usr/bin/env python3
"""Is the lobe nearer the camera drawn on top?  (#166, transparency modes)

    tools/coin/lobes.py [outdir] [--nrender] [--builds coin,vendored]

For each system (synthetic pi field on benzene/crco6/water, real MOs of
calc-water-vib and orca-crco6) and camera angle 0/45/90 degrees, renders
  opaque both / pos only / neg only   (depth-tested: the oracle for which
                                       lobe is nearer at each pixel)
  blended both / pos only / neg only  (the mode under test)
and, at pixels where both lobes are visible, asks which draw order the blended
pixel matches: the single-lobe blended renders give each lobe's colour, from
which both over-orderings are predicted.  Agreement = fraction of overlap
pixels where the matching order is the one the opaque depth test gives.
Results: <outdir>/lobes.txt.  Private Xvfb :170-:179 only.
"""
import os, sys, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
import compare

args = [a for a in sys.argv[1:] if not a.startswith("--")]
OUT = os.path.abspath(args[0] if args else os.path.join(ROOT, "build-coin-compare", "lobes"))
BUILDS = {"vendored": os.path.join(ROOT, "build-cmake"), "coin": os.path.join(ROOT, "build-coin")}
if "--builds" in sys.argv:
    want = sys.argv[sys.argv.index("--builds") + 1].split(",")
    BUILDS = {k: v for k, v in BUILDS.items() if k in want}
SYS = ["benzene", "crco6", "water"]
ANGLES = [0, 45, 90]
# key -> scene lines that put the renderer in the mode under test
MODES = {
    "std": ["transparency SORTED_OBJECT_BLEND"],       # what ECCE requests
    "sd": ["transparency SCREEN_DOOR"],                # what the Builder uses for MO lobes
}
for n in (2, 4, 6, 8, 12, 16):
    MODES["p%d" % n] = ["transparency SORTED_LAYERS_BLEND", "layerpasses %d" % n]
COIN_ONLY = lambda k: k.startswith("p")
if os.environ.get("LOBE_MODES"):
    MODES = {k: v for k, v in MODES.items() if k in os.environ["LOBE_MODES"].split(",")}


def sceneText(stem, setup, modes):
    L = ["labels none", "style Ball And Stick", "viewall"]
    L += setup
    L += ["snap %s-base" % stem]
    for mk, cmds in modes.items():
        for ai, a in enumerate(ANGLES):
            if ai:
                L.append("rotate %d" % (a - ANGLES[ai - 1]))
            t = "%s-%s-a%03d" % (stem, mk, a)
            L += ["transparency SCREEN_DOOR", "isolobe none 0", "snap %s-none" % t,
                  "isolobe pos 0", "snap %s-pop" % t, "isolobe neg 0", "snap %s-nop" % t,
                  "isolobe both 0", "snap %s-bop" % t] + cmds + [
                  "isolobe none 0.5", "snap %s-nonebl" % t,
                  "isolobe pos 0.5", "snap %s-pbl" % t, "isolobe neg 0.5", "snap %s-nbl" % t,
                  "isolobe both 0.5", "snap %s-bbl" % t, "exportiso %s" % t]
        L.append("rotate %d" % -ANGLES[-1])
    return "\n".join(L) + "\n"


def render(name, b):
    import xdisplay
    os.environ["ECCE_COIN_ALPHA"] = "1"
    d = os.path.join(OUT, "raw", name)
    os.makedirs(d, exist_ok=True)
    for f in ("FAILED",):
        if os.path.exists(os.path.join(d, f)):
            os.remove(os.path.join(d, f))
    modes = {k: v for k, v in MODES.items() if name == "coin" or not COIN_ONLY(k)}
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT,
               FL_FONT_PATH=os.path.join(ROOT, "data", "client", "fonts") + "/",
               ECCE_REALUSERHOME=os.path.join(d, "state"))
    os.makedirs(env["ECCE_REALUSERHOME"], exist_ok=True)
    cache = lambda k: [l.split("=", 1)[1].strip() for l in open(b + "/CMakeCache.txt") if l.startswith(k + ":")][0]
    home, wrappers = cache("ECCE_HOME_DIR"), cache("ECCE_WRAPPER_DESTINATION")
    assert open(home + "/bin/builder", "rb").read() == open(b + "/builder", "rb").read(), "stale install"
    os.environ["ECCE_TEST_HOME"], os.environ["ECCE_TEST_WRAPPERS"] = home, wrappers
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    with xdisplay.Display() as disp:
        env["DISPLAY"] = ":%d" % disp.number
        for s in SYS:
            script = os.path.join(d, s + ".scene")
            open(script, "w").write(sceneText(s, ["isotest"], modes))
            r = subprocess.run([os.path.join(b, "viewer-scenes"), d, script, s],
                               env=env, capture_output=True, text=True, timeout=900)
            if r.returncode != 0:
                sys.exit("viewer-scenes %s %s: %s" % (name, s, (r.stdout + r.stderr)[-600:]))
    scenedir = os.path.join(d, "scenes")
    os.makedirs(scenedir, exist_ok=True)
    compare.SCENES = scenedir
    for cname, source, mo in (("calc-water-vib", compare.CALCS[0][1], "mo 5 0.05 40"),
                              ("orca-crco6", compare.CALCS[1][1], "mo 54 0.05 40")):
        stem = "calc-" + {"calc-water-vib": "water", "orca-crco6": "crco6"}[cname] + "-lobes"
        open(os.path.join(scenedir, stem + ".scene"), "w").write(sceneText(stem, [mo], modes))
        err = compare.renderCalc(cname, source, stem + ".scene", d)
        if err:
            sys.exit("%s %s: %s" % (name, cname, err))


COSTMODES = [("SCREEN_DOOR", ["transparency SCREEN_DOOR"]),
             ("DELAYED_ADD", ["transparency DELAYED_ADD"]),
             ("SORTED_OBJECT_BLEND", ["transparency SORTED_OBJECT_BLEND"]),
             ("SORTED_LAYERS_BLEND", ["transparency SORTED_LAYERS_BLEND", "layerpasses 6"])]   # coin only


def costScene(stem, setup, vendored):
    L = ["labels none", "style Ball And Stick", "viewall"] + setup
    for mk, cmds in COSTMODES:
        if vendored:
            if mk == "SORTED_LAYERS_BLEND":
                continue
        L += cmds + ["isolobe both 0.5", "timeframes %s-%s %d" % (stem, mk, FRAMES)]
    return "\n".join(L) + "\n"


def cost(name, b):
    import xdisplay
    os.environ["ECCE_COIN_ALPHA"] = "1"
    d = os.path.join(OUT, "cost", name)
    os.makedirs(d, exist_ok=True)
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE="1", ECCE_HOME=ROOT,
               FL_FONT_PATH=os.path.join(ROOT, "data", "client", "fonts") + "/",
               ECCE_REALUSERHOME=os.path.join(d, "state"))
    os.makedirs(env["ECCE_REALUSERHOME"], exist_ok=True)
    os.environ.setdefault("ECCE_TEST_XDISPLAYS", "170-179")
    with xdisplay.Display() as disp:
        env["DISPLAY"] = ":%d" % disp.number
        for s in SYS + ["waterbox"]:
            script = os.path.join(d, s + ".scene")
            open(script, "w").write(costScene(s, ["isotest"], name == "vendored"))
            r = subprocess.run([os.path.join(b, "viewer-scenes"), d, script, s],
                               env=env, capture_output=True, text=True, timeout=1800)
            if r.returncode != 0:
                sys.exit("cost %s %s: %s" % (name, s, (r.stdout + r.stderr)[-600:]))


def costTable():
    lines = ["%-9s %-14s %-20s %s" % ("build", "system", "mode", "result")]
    for name in BUILDS:
        for s in SYS + ["waterbox"]:
            for mk, _ in COSTMODES:
                p = os.path.join(OUT, "cost", name, "%s-%s.txt" % (s, mk))
                if os.path.exists(p):
                    lines.append("%-9s %-14s %-20s %s" % (name, s, mk, open(p).read().strip()))
    open(os.path.join(OUT, "cost.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


def analyseSD(name, stem, mk, a):
    """Screen door: each pixel is a lobe colour or what is behind it, so classify
    the overlap pixels by which single-lobe stippled render they equal."""
    import numpy as np
    r = lambda t: compare.readPpm(os.path.join(OUT, "raw", name, "%s-%s-a%03d-%s.ppm" % (stem, mk, a, t))).astype(int)
    none, pop, nop, bop = r("none"), r("pop"), r("nop"), r("bop")
    nb, pbl, nbl, bbl = r("nonebl"), r("pbl"), r("nbl"), r("bbl")
    tol = 3
    ov = (np.abs(pop - none).max(axis=2) > tol) & (np.abs(nop - none).max(axis=2) > tol)
    pos_near = np.abs(bop - pop).sum(axis=2) < np.abs(bop - nop).sum(axis=2)
    ps = np.abs(pbl - nb).max(axis=2) > tol          # pos stipple passes here
    ns = np.abs(nbl - nb).max(axis=2) > tol
    isP = np.abs(bbl - pbl).sum(axis=2) <= 6
    isN = np.abs(bbl - nbl).sum(axis=2) <= 6
    both = ov & ps & ns & (np.abs(pbl - nbl).sum(axis=2) > 12)   # both lobes drawn here, colours distinguishable
    vp = both & isP & ~isN
    vn = both & isN & ~isP
    vis = int((vp | vn).sum())
    near_ok = int(((vp & pos_near) | (vn & ~pos_near)).sum())
    back = int(((vp & ~pos_near) | (vn & pos_near)).sum())
    bgpx = int((both & ~isP & ~isN).sum())
    return int(ov.sum()), int(both.sum()), int((ov & (ps != ns)).sum()), vis, near_ok, back, bgpx


def analyse(name, stem, mk, a):
    import numpy as np
    r = lambda t: compare.readPpm(os.path.join(OUT, "raw", name, "%s-%s-a%03d-%s.ppm" % (stem, mk, a, t))).astype(int)
    none, pop, nop, bop = r("none"), r("pop"), r("nop"), r("bop")
    nb, pbl, nbl, bbl = r("nonebl"), r("pbl"), r("nbl"), r("bbl")
    tol = 3
    pv = np.abs(pop - none).max(axis=2) > tol          # pos lobe visible
    nv = np.abs(nop - none).max(axis=2) > tol
    ov = pv & nv
    # opaque depth test: whichever single-lobe render the both-lobes render equals
    dp = np.abs(bop - pop).sum(axis=2)
    dn = np.abs(bop - nop).sum(axis=2)
    pos_near = dp < dn
    # blended single-lobe renders -> lobe colours -> predicted over-orderings
    Cp = nb + 2 * (pbl - nb)
    Cn = nb + 2 * (nbl - nb)
    pred_p = 0.5 * Cp + 0.5 * (0.5 * Cn + 0.5 * nb)
    pred_n = 0.5 * Cn + 0.5 * (0.5 * Cp + 0.5 * nb)
    ep = np.abs(bbl - pred_p).sum(axis=2)
    en = np.abs(bbl - pred_n).sum(axis=2)
    # only pixels where the two orderings are distinguishable
    sel = ov & (np.abs(pred_p - pred_n).sum(axis=2) > 12)
    n = int(sel.sum())
    pos_top = ep < en
    agree = int((sel & (pos_top == pos_near)).sum())
    return int(ov.sum()), n, agree, (round(100.0 * agree / n, 1) if n else None)


FRAMES = int(os.environ.get("COST_FRAMES", "30"))


def refCompare():
    """Per-pixel comparison with isoref.py's renderer-independent reference."""
    import numpy as np, isoref
    TOL = 24
    lines = ["%-9s %-5s %-15s %4s %8s %7s %8s | %7s %8s %8s | %9s %9s" % (
        "build", "mode", "system", "deg", "lobe_px", "diff_px", "mean_abs", "ovl_px", "dom~ref", "dom~near", "domagr_all", "(pct)")]
    tot = {}
    for name in BUILDS:
        for stem in SYS + ["calc-water-lobes", "calc-crco6-lobes"]:
            for mk in MODES:
                if name != "coin" and COIN_ONLY(mk):
                    continue
                for a in ANGLES:
                    base = os.path.join(OUT, "raw", name, "%s-%s-a%03d" % (stem, mk, a))
                    if not os.path.exists(base + "-iso.txt"):
                        continue
                    img = compare.readPpm(base + "-bbl.ppm").astype(int)
                    none = compare.readPpm(base + "-none.ppm").astype(int)
                    bg = none[0, 0]
                    ref, m, top = isoref.render(base + "-iso.txt", bg)
                    ref = ref.astype(int)
                    if "--save-ref" in sys.argv or True:
                        from PIL import Image
                        Image.fromarray(ref.astype(np.uint8)).save(base + "-ref.png")
                    atoms = np.abs(none - bg).max(axis=2) > 3
                    L = ((m != 0) | (np.abs(img - bg).max(axis=2) > 3)) & ~atoms
                    d = np.abs(img - ref).max(axis=2)
                    n = int(L.sum())
                    diffpx = int((L & (d > TOL)).sum())
                    mean = float(np.abs(img - ref).mean(axis=2)[L].mean()) if n else 0
                    dom = lambda x: (x[..., 0] - bg[0]) > (x[..., 1] - bg[1])
                    agree = dom(img) == dom(ref)
                    ov = (m == 3) & ~atoms
                    ag_all = int((L & agree).sum())
                    ag_ov = int((ov & agree).sum())
                    ag_top = int((ov & (dom(img) == (top == 0))).sum())
                    lines.append("%-9s %-5s %-15s %4d %8d %7d %8.2f | %7d %8d %8d | %9d %8.1f%%" % (
                        name, mk, stem, a, n, diffpx, mean, int(ov.sum()), ag_ov, ag_top, ag_all, 100.0 * ag_all / max(n, 1)))
                    t = tot.setdefault((name, mk), [0, 0, 0, 0, 0, 0.0, 0])
                    t[0] += n; t[1] += diffpx; t[2] += int(ov.sum()); t[3] += ag_ov; t[4] += ag_all; t[5] += mean * n; t[6] += ag_top
    lines.append("")
    for (name, mk), t in sorted(tot.items()):
        lines.append("TOTAL %-9s %-4s lobe px %7d, differing(>%d) %7d (%.1f%%), mean abs %.2f; overlap px %7d dominant colour: vs reference image %.1f%%, vs reference nearest lobe %.1f%%; all lobe px (vs ref image) %.1f%%" % (
            name, mk, t[0], TOL, t[1], 100.0 * t[1] / max(t[0], 1), t[5] / max(t[0], 1), t[2], 100.0 * t[3] / max(t[2], 1), 100.0 * t[6] / max(t[2], 1), 100.0 * t[4] / max(t[0], 1)))
    open(os.path.join(OUT, "refcompare.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


def sdTable():
    lines = ["%-9s %-15s %4s %7s %7s %7s %7s %7s %7s %7s" % ("build", "system", "deg", "overlap", "both", "maskdif", "visible", "nearer", "back", "other")]
    tot = {}
    for name in BUILDS:
        for stem in SYS + ["calc-water-lobes", "calc-crco6-lobes"]:
            for a in ANGLES:
                try:
                    o, b, md, v, ok, bk, ot = analyseSD(name, stem, "sd", a)
                except FileNotFoundError:
                    continue
                lines.append("%-9s %-15s %4d %7d %7d %7d %7d %7d %7d %7d" % (name, stem, a, o, b, md, v, ok, bk, ot))
                t = tot.setdefault(name, [0] * 7)
                for i, x in enumerate((o, b, md, v, ok, bk, ot)):
                    t[i] += x
    lines.append("")
    for name, t in tot.items():
        lines.append("TOTAL %-9s overlap %d, both-stipple-pass %d, mask-differs %d, visible lobe px %d: nearer %.1f%%, back %.1f%% (of both-pass %.1f%%), neither %d" % (
            name, t[0], t[1], t[2], t[3], 100.0 * t[4] / max(t[3], 1), 100.0 * t[5] / max(t[3], 1), 100.0 * t[5] / max(t[1], 1), t[6]))
    open(os.path.join(OUT, "sd.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


def main():
    if "--sd" in sys.argv:
        sdTable()
        return
    if "--ref" in sys.argv:
        refCompare()
        return
    if "--cost" in sys.argv:
        if os.environ.get("LOBE_CHILD"):
            for name, b in BUILDS.items():
                cost(name, b)
        else:
            for name in BUILDS:
                subprocess.run([sys.executable, __file__, OUT, "--builds", name, "--cost"],
                               env=dict(os.environ, LOBE_CHILD="1"), check=True)
            costTable()
        return
    if os.environ.get("LOBE_CHILD"):
        for name, b in BUILDS.items():
            render(name, b)
        return
    if "--nrender" not in sys.argv:
        for name in BUILDS:      # apps/fixture read ECCE_TEST_HOME at import: one process per build
            subprocess.run([sys.executable, __file__, OUT, "--builds", name],
                           env=dict(os.environ, LOBE_CHILD="1"), check=True)
    lines = ["%-9s %-15s %-5s %4s %8s %8s %8s %7s" % ("build", "system", "mode", "deg", "overlap", "distinct", "agree", "pct")]
    tot = {}
    stems = SYS + ["calc-water-lobes", "calc-crco6-lobes"]
    for name in BUILDS:
        for stem in stems:
            for mk in MODES:
                if name != "coin" and COIN_ONLY(mk):
                    continue
                for a in ANGLES:
                    try:
                        o, n, ag, pc = analyse(name, stem, mk, a)
                    except FileNotFoundError:
                        continue
                    lines.append("%-9s %-15s %-5s %4d %8d %8d %8d %7s" % (name, stem, mk, a, o, n, ag, pc))
                    t = tot.setdefault((name, mk), [0, 0])
                    t[0] += n
                    t[1] += ag
    lines.append("")
    for (name, mk), (n, ag) in sorted(tot.items()):
        lines.append("TOTAL %-9s %-5s distinct-overlap px %7d agree %7d = %.1f%%" % (name, mk, n, ag, 100.0 * ag / max(n, 1)))
    open(os.path.join(OUT, "lobes.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))


main()
