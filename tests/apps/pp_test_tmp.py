import os, runpy, sys
os.makedirs(os.path.expanduser("~/.cache"), exist_ok=True)
runpy.run_path("tests/apps/examples_preparsed_test.py", run_name="__main__")
