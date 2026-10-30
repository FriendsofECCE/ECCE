import runpy, sys
sys.argv = ["preparse.py", "workflows"]
runpy.run_path("tools/examples/preparse.py", run_name="__main__")
