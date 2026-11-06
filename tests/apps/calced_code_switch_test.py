#!/usr/bin/env python3
"""Switch every Calculation Editor code button and keep using the editor (#785)."""

import os
import re
import shutil
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps
import fixture
import run_tests
import symmetry_find_test as sym

CODES = ("xTB", "CREST", "ORCA", "ECCE-QM", "Gaussian-16", "CP2K",
         "QuantumESPRESSO", "NWChem", "MOPAC", "MLP")


def codeResources():
    """Include any additional registered code buttons in this installation."""
    config = "ResourceDescriptorRxn.xml"
    data = os.path.join(apps.INSTALL, "data", "client")
    resources = {}
    for resource in ET.parse(os.path.join(data, "config", config)).iter("ResourceType"):
        if resource.findtext("FactoryCategory") != "EsCalculation":
            continue
        application = resource.get("applicationType")
        # CodeFactory discovers code names from EDML filenames; reaction
        # resources can reuse another code's descriptor without a code button.
        if not os.path.isfile(os.path.join(data, "cap", application + ".edml")):
            continue
        descriptor = resource.findtext("DescriptorFile")
        if not descriptor:
            continue
        cap = ET.parse(os.path.join(data, "cap", descriptor))
        if cap.findtext(".//InputGenerator") and cap.findtext(".//Template"):
            resources[resource.get("applicationType")] = resource.get("name")
    return resources


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "codeswitch-localdata")
        project = os.path.join(folder, "users", "local", "qm")
        os.makedirs(project)
        shutil.copy(os.path.join(REPO, "tools", "screenshots", "data",
                                 "project.ecce-meta"), project + "/.ecce-meta")
        calc = os.path.join(project, "o2")
        shutil.copytree(os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks"),
                        calc, symlinks=True)
        meta = os.path.join(calc, ".ecce-meta")
        with open(meta) as handle:
            text = handle.read().replace("ecce:state\t\tComplete", "ecce:state\t\tReady")
        with open(meta, "w") as handle:
            handle.write(text)
        os.environ["ECCE_LOCAL_DATA"] = folder
        os.environ["ECCE_TEST_DIALOG_CLOSE"] = "1"
        os.environ["ECCE_TEST_DIALOG_ANSWER"] = "yes"
        restore = fixture.settleUpgradeNotices()
        try:
            resources = codeResources()
            codes = list(CODES) + sorted(set(resources) - set(CODES)) + ["xTB", "ECCE-QM"]
            steps = ["wait 9000"]
            for code in codes:
                steps += ["code " + code, "wait 5000", "code-state",
                          "button save", "wait 2000", "code-state"]
            steps += ["quit"]
            script = os.path.join(fixture.stateDir(), "codeswitch.script")
            with open(script, "w") as handle:
                handle.write("\n".join(steps) + "\n")
            result = apps.run(display, "calced", args=("-context", "file://" + calc + "/"),
                              windowTimeout=60, settle=300,
                              env={"ECCE_CALCED_SCRIPT": script, "ECCE_FEEDBACK_STDERR": "1"})
            log = result.log or ""
            sym.check(result.returncode == 0, "editor exits without a signal", log)
            if verbose:
                print(log)
            for code in dict.fromkeys(codes):
                sym.check("ECCE_CALCED_SCRIPT: code %s: ok" % code in log,
                          code + ": code button completed", run_tests._tail(log, 30))
            states = re.findall(r"CALCED: code=(\S+) application=(\S+) resource=(\S+) builder=(-?\d+)", log)
            sym.check(len(states) == 2 * len(codes),
                      "editor responds before and after every save", run_tests._tail(log, 30))
            if len(states) == 2 * len(codes):
                for index, code in enumerate(codes):
                    before, after = states[2 * index:2 * index + 2]
                    sym.check(before[0] == code and after[:3] == (code, code, resources.get(code)),
                              code + ": selected code and saved descriptor agree",
                              str((before, after)))
            sym.check("FAIL:" not in log and "exception:" not in log,
                      "all scripted actions succeeded", run_tests._tail(log, 30))
            sym.check("ECCE_CALCED_SCRIPT: quit" in log,
                      "editor exits normally", run_tests._tail(log, 30))
        finally:
            restore()
            for key in ("ECCE_LOCAL_DATA", "ECCE_TEST_DIALOG_CLOSE", "ECCE_TEST_DIALOG_ANSWER"):
                os.environ.pop(key, None)
        if sym.failures:
            results.fail("calced-code-switch", "; ".join(sym.failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
