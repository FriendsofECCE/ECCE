#!/usr/bin/env python3
"""PNGs of the Gaussian 16 Runtype Details dialog for the two QST run types
(#419), with both pickers filled and the QST3 tick off / on.

    tests/look/qst_dialog_shot.py OUTDIR

Same mechanism as codereg_shot.py; the dialog is handed a candidate list
(ECCE_QST_CANDIDATES) and a restore file with the picks, as CalcEd does.
"""
import os
import socket
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import codereg_shot as cs  # noqa: E402

ROOT = cs.ROOT
CANDS = [("hcn", "file:///home/user/proj/hcn", "optimised"),
         ("hnc", "file:///home/user/proj/hnc", "optimised"),
         ("hcn-hnc-ts", "file:///home/user/proj/hcn-hnc-ts", "input")]


# codereg_shot's INNER grabs the root window, which is black here; keep the
# dialog up instead and photograph it by window id from outside.
INNER = (cs.INNER.split("    r = self.GetScreenRect()")[0] + "    end = time.time() + 15\n    while time.time() < end:\n        wx.GetApp().Yield()\n        time.sleep(0.05)\n"
         + "templates.EcceFrame.Finalize = finalize\n"
         + cs.INNER.split("templates.EcceFrame.Finalize = finalize\n")[1])


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
    import xdisplay
    work = tempfile.mkdtemp(prefix="qst-shot")
    os.makedirs(os.path.join(work, ".ECCE"))
    with open(os.path.join(work, "cands"), "w") as f:
        for c in CANDS:
            f.write("\t".join(c) + "\n")
    sink = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sink.bind(("127.0.0.1", 0))
    port = str(sink.getsockname()[1])
    status = 0
    with xdisplay.Display(screen="1400x1000x24") as xv:
        time.sleep(1)
        for tag, run, guess in (
                ("qst2", "Transition State (QST)", "0"),
                ("qst3", "Transition State (QST)", "1"),
                ("qst2-freq", "Transition State (QST) + Frequencies", "0")):
            png = os.path.join(out, "dialog-%s.png" % tag)
            restore = os.path.join(work, "restore.in")
            with open(restore, "w") as f:
                f.write("ES.Runtype.GeomOpt.QSTReactant|%s||1|1|menu_input\n"
                        "ES.Runtype.GeomOpt.QSTProduct|%s||1|1|menu_input\n"
                        "ES.Runtype.GeomOpt.QSTGuess|%s||1|1|boolean\n"
                        "END_GUIValues\n" % (CANDS[0][1], CANDS[1][1], guess))
            argv = [restore, port, "GUIValues", "Writable", "DebugOff", "SCF",
                    "RHF", run, "shot", "0", "C1", "14", "1", "0", "5", "5", "6"]
            env = dict(os.environ, DISPLAY=xv.name, GTK_THEME="Adwaita",
                       ECCE_REALUSERHOME=work, HOME=work, CODEREG_DIR=cs.os.path.join(ROOT, "scripts/codereg"),
                       SHOT_OUT=png, ECCE_QST_CANDIDATES=os.path.join(work, "cands"),
                       SHOT_SCRIPT=os.path.join(ROOT, "scripts/codereg/ged16runtype.py"),
                       PYTHONDONTWRITEBYTECODE="1", SHOT_FLASH="", GDK_BACKEND="x11", NO_AT_BRIDGE="1")
            env.pop("WAYLAND_DISPLAY", None)
            known = set(w for w, _ in xv.windows())
            proc = subprocess.Popen([sys.executable, "-c", INNER] + argv, env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            end = time.time() + 60
            while time.time() < end and proc.poll() is None:
                new = [w for w, _ in xv.windows() if w not in known]
                if new:
                    time.sleep(5)
                    subprocess.run(["import", "-display", xv.name, "-window",
                                    new[0], png])
                    break
                time.sleep(0.5)
            proc.kill()
            r = proc
            if not os.path.exists(png):
                print("FAILED %s rc=%s\n%s" % (tag, r.returncode,
                                           r.stdout.read().decode()[-1500:]))
                status = 1
            else:
                print(png)
    return status


if __name__ == "__main__":
    sys.exit(main())
