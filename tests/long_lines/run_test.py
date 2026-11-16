"""Run a reader unit test with private state and a deadline for stuck streams."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
source = str(Path(__file__).resolve().parents[2])
with tempfile.TemporaryDirectory(prefix="ecce-long-lines-", dir=os.getcwd()) as work:
    env = os.environ.copy()
    env.update(HOME=work, ECCE_REALUSERHOME=work, ECCE_REALUSER="longlines",
               ECCE_HOME=source, XDG_CACHE_HOME=os.path.join(work, "cache"),
               ECCE_LOCAL_DATA=os.path.join(work, "data"))
    subprocess.run([binary], cwd=work, env=env, check=True, timeout=20)
