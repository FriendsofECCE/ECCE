#!/usr/bin/env python3
"""Only the #1088 section of one_process_test.py (for runners that pass no
environment, e.g. tools/offload apptest)."""
import os
import sys

os.environ["ECCE_1088_ONLY"] = "1"
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import one_process_test  # noqa: E402

if __name__ == "__main__":
    sys.exit(one_process_test.main())
