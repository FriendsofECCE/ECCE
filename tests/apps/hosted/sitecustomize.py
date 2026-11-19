"""ECCE_TEST_HOSTED=1: start tools as tabs of the Organizer (#983).

On PYTHONPATH (tests/apps/hosted_suite.py puts it there), this patches
subprocess.Popen in every Python process of a test run: a command whose
program is the ecce-builder, ecce-calced or ecce-launcher wrapper runs
tests/apps/hosted_shim.py instead, which starts the Organizer in
one-program mode and opens that tool as a tab on the same URL.  The
tool's script hooks (ECCE_BUILDER_SCRIPT, ECCE_VIEWER_SCENE,
ECCE_CALCED_SCRIPT, ...) are environment variables, so the hosted tool
reads them from the Organizer's environment unchanged.

Without ECCE_TEST_HOSTED=1 nothing is patched.
"""
import os
import subprocess
import sys

HOSTED = {"ecce-builder": "Builder", "ecce-calced": "CalculationEditor",
          "ecce-launcher": "Launcher"}
SHIM = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(
    __file__))), "hosted_shim.py")

if os.environ.get("ECCE_TEST_HOSTED") == "1":
    _init = subprocess.Popen.__init__

    def _hostedInit(self, args, *rest, **kw):
        if isinstance(args, (list, tuple)) and args:
            # A program run under strace or gdb keeps its place in the list.
            for i, word in enumerate(args):
                base = os.path.basename(str(word))
                if base in HOSTED:
                    args = (list(args[:i]) +
                            [sys.executable, SHIM, HOSTED[base], str(word)] +
                            list(args[i + 1:]))
                    break
        _init(self, args, *rest, **kw)

    subprocess.Popen.__init__ = _hostedInit
