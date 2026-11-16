#!/usr/bin/env python3
"""ORCA EPR tables: real monitor boundaries, selected nuclei and partial output."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
import eccejobmonitor_sim as sim

FIXTURE = HERE / "fixtures/orca/nh2_epr.out"
PARSERS = REPO / "scripts/parsers"


def parse(text):
    run = subprocess.run(["perl", str(PARSERS / "orca.epr")], input=text,
                         text=True, capture_output=True, check=True)
    if run.stderr:
        raise AssertionError(run.stderr)
    return {r["key"]: r["flat"] for r in sim.parse_parser_output(run.stdout)}


def records():
    replay = sim.replay(sim.read_desc(str(PARSERS / "orca.desc")), str(FIXTURE))
    assert replay.runaway is None, replay.runaway
    result = {}
    for block in replay.blocks:
        if block.delivered and block.entry.script == "orca.epr":
            result.update(parse(''.join(block.lines)))
    return result


class EprTest(unittest.TestCase):
    def test_real_output(self):
        rec = records()
        self.assertEqual(set(rec), {"EPRG", "EPRA", "EPRQ"})
        self.assertEqual(rec["EPRG"]["values"].split(),
                         ['2.0021656', '2.0033156', '2.0069052', '2.0041288'])
        self.assertEqual(rec["EPRA"]["rowlabels"].split(), ['0-N', '1-H', '2-H'])
        self.assertEqual(rec["EPRA"]["units"], 'MHz')
        a = [float(v) for v in rec["EPRA"]["values"].split()]
        self.assertEqual(a, [80.6615, 82.7827, 203.4865, 122.3102,
                            -28.7800, -114.0087, -164.1407, -102.3098,
                            -28.7800, -114.0087, -164.1407, -102.3098])
        for offset in (0, 4, 8):
            self.assertAlmostEqual(sum(a[offset:offset+3])/3, a[offset+3], places=4)
        self.assertEqual(rec["EPRQ"]["rowlabels"], '0-N')
        self.assertEqual(rec["EPRQ"]["units"], 'MHz')
        cq, q1, q2, q3 = map(float, rec["EPRQ"]["values"].split())
        self.assertAlmostEqual(cq, -4.099864)
        self.assertAlmostEqual(q1, 0.618834497126)
        self.assertAlmostEqual(q2, 1.431097502874)
        self.assertAlmostEqual(q3, -2.049932)
        self.assertAlmostEqual(q1+q2+q3, 0)

    def test_optional_sections_and_selected_nuclei(self):
        text = FIXTURE.read_text()
        g, nuclei = text.split('ELECTRIC AND MAGNETIC HYPERFINE STRUCTURE', 1)
        self.assertEqual(set(parse(g)), {'EPRG'})
        self.assertEqual(set(parse(nuclei)), {'EPRA', 'EPRQ'})
        # Retain just the last H nucleus, with its original (sparse) index.
        last = nuclei[nuclei.index(' Nucleus   2H'):]
        rec = parse(last)
        self.assertEqual(set(rec), {'EPRA'})
        self.assertEqual(rec['EPRA']['rowlabels'], '2-H')
        self.assertEqual(rec['EPRA']['size'], '1 4')
        # EFG-only output must not require an A(Tot) line.
        q_only = re.sub(r'^.*A\(Tot\).*$\n?', '', nuclei, flags=re.M)
        self.assertEqual(set(parse(q_only)), {'EPRQ'})

    def test_partial_output(self):
        text = FIXTURE.read_text()
        g_end = text.index('EPR g-tensor done in')
        self.assertEqual(parse(text[:g_end]), {})
        a_start = text.index('ELECTRIC AND MAGNETIC HYPERFINE STRUCTURE')
        a_end = text.index('Hyperfine and quadrupole coupling calculation done in')
        for cut in [text.index('A(Tot)'), text.index('eta '), a_end, a_end+20]:
            self.assertEqual(parse(text[a_start:cut]), {})

    def test_no_epr(self):
        self.assertEqual(parse((HERE / 'fixtures/orca/h2o_optfreq.out').read_text()), {})

    def test_property_registry_and_store(self):
        from propinterp_test import buildDriver
        text = FIXTURE.read_text()
        stream = subprocess.run(['perl', str(PARSERS / 'orca.epr')],
                                input=text, text=True, capture_output=True,
                                check=True).stdout
        with tempfile.TemporaryDirectory(prefix='epr-store-') as work:
            driver = os.path.join(work, 'propinterp')
            self.assertTrue(buildDriver(driver), 'build the application libraries first')
            run = subprocess.run([driver, work], input=stream, text=True,
                                 capture_output=True, check=True,
                                 env=dict(os.environ, ECCE_HOME=str(REPO)))
            for key, rows in [('EPRG', 1), ('EPRA', 3), ('EPRQ', 1)]:
                self.assertIn(f'{key} rows {rows} columns 4 values {rows*4}', run.stdout)
                self.assertTrue((Path(work) / key).is_file())


if __name__ == '__main__':
    unittest.main()
