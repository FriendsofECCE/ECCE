"""Locate the CMake-built fixture helper in the install under test."""

import os

import apps


def path():
    name = "resourceTest.exe" if os.name == "nt" else "resourceTest"
    helper = os.path.join(apps.INSTALL, "libexec", "tests", name)
    if not os.path.isfile(helper):
        raise FileNotFoundError(
            "Missing app-test helper %s; build and install the tree first" % helper)
    return helper
