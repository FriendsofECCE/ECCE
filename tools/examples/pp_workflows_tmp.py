import os, runpy, sys
os.makedirs(os.path.expanduser("~/.cache"), exist_ok=True)
sys.argv = ["preparse.py", "workflows"]
runpy.run_path("tools/examples/preparse.py", run_name="__main__")
