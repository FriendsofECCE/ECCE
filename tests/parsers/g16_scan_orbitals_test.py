#!/usr/bin/env python3
"""Real Gaussian 16 RHF/STO-3G water bond scan: per-point orbital tables (#549).

The fixture is the unedited log of the deck ai.gauss16 writes for a scan
(Pop=(full,Always)), three points and nine optimisation steps, with its
fort.7.  The reference energies come from the "Alpha occ./virt. eigenvalues"
lines, which the parser under test does not read.  An older scan log, run
without Pop=Always, must give no point tables.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
import eccejobmonitor_sim as sim

FIXTURES = HERE / 'fixtures/gaussian-16'
FIXTURE = FIXTURES / 'h2o_scan_mos.log'
LEGACY = FIXTURES / 'h2o2_dihedral_scan.log'
PARSERS = REPO / 'scripts/parsers'
ARGS = ('.', 'Geometry', 'SCF', 'RHF', '0')


def records(output=FIXTURE):
    """Replay stream rules and File= rules, as the monitor and job store do."""
    desc = sim.read_desc(str(PARSERS / 'gaussian-16.desc'))
    for entry in desc.table.values():
        assert entry.script != 'gaussian-16.scanmos', 'scangeom runs it'
    assert not desc.collisions or all('g16.g16out' not in (a.rules.get('file'), b.rules.get('file'))
                                      for a, b in desc.collisions), desc.collisions
    out = {}
    def collect(text):
        for rec in sim.parse_parser_output(text):
            out.setdefault(rec['key'], []).append(rec)
    with tempfile.TemporaryDirectory(prefix='g16-scan-') as work:
        result = sim.replay(desc, str(output))
        for block in result.blocks:
            if block.delivered:
                text, err, rc = sim.run_parser(str(PARSERS), block.entry, block,
                                               parse_args=ARGS, workdir=work)
                assert rc == 0, err
                collect(text)
        for entry in desc.table.values():
            name = entry.rules.get('file')
            if name == 'g16.g16out':
                text = Path(output).read_text()
            elif name == 'fort.7' and output == FIXTURE:
                text = FIXTURE.with_suffix('.fort7').read_text()
            else:
                continue
            proc = subprocess.run(['perl', str(PARSERS / entry.script)] + list(ARGS),
                                  input=text, capture_output=True, text=True, cwd=work,
                                  env={'ECCE_HOME': str(REPO), 'PATH': '/usr/bin:/bin'})
            assert proc.returncode == 0, proc.stderr
            collect(proc.stdout)
    return out


def values(rec):
    return [float(x) for x in rec['flat']['values'].split()]


def printed_energies(path=FIXTURE):
    """Per point: the eigenvalue lines of the last population analysis before
    its "Optimization completed" (occupied ones first)."""
    points = []
    for chunk in path.read_text().split('Optimization completed')[:-1]:
        last = chunk.split('Population analysis using')[-1]
        occ = [float(x) for l in re.findall(r'Alpha  occ\. eigenvalues --(.*)', last)
               for x in re.findall(r'-?\d+\.\d{5}', l)]
        virt = [float(x) for l in re.findall(r'Alpha virt\. eigenvalues --(.*)', last)
                for x in re.findall(r'-?\d+\.\d{5}', l)]
        points.append((occ, virt))
    return points


def main():
    rec = records()
    for key in ('PTORB', 'PTMO', 'SCANGEOM'):
        assert len(rec.get(key, [])) == 3, '%s: want three tables, got %d' % (key, len(rec.get(key, [])))
    for k, (occ, virt) in enumerate(printed_energies()):
        assert (len(occ), len(virt)) == (5, 2), (k, occ, virt)
        got = values(rec['PTORB'][k])
        assert rec['PTORB'][k]['flat']['size'].split() == ['1', '7', '2']
        assert rec['PTMO'][k]['flat']['size'].split() == ['1', '7', '7']
        assert got[::2] == occ + virt, (k, got, occ, virt)
        assert got[1::2] == [2.] * 5 + [0.] * 2
    assert values(rec['PTORB'][0]) != values(rec['PTORB'][-1])
    # The last point's coefficients are the fort.7 set, to the log's 5 decimals.
    last, final = values(rec['PTMO'][-1]), values(rec['MO'][-1])
    assert len(last) == len(final) == 49
    assert max(abs(a - b) for a, b in zip(last, final)) <= 5.1e-6
    old = records(LEGACY)
    assert old.get('SCANGEOM'), 'legacy fixture is a relaxed scan'
    assert 'PTMO' not in old and 'PTORB' not in old
    print('PASS: three point tables match independently read eigenvalues and the final '
          'fort.7 set; a scan log without Pop=Always gives none')
    return 0


if __name__ == '__main__':
    sys.exit(main())
