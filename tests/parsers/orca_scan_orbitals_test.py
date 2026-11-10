#!/usr/bin/env python3
"""Real ORCA 6.1.1 HF/STO-3G H2 scan: selected-point tables (#549).

The fixture keeps the geometry, orbital and scan-result sections verbatim;
SCF iterations, populations, timings and the program banner are trimmed.
The companion input and allxyz are from the same one-core run.
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

FIXTURE = HERE / 'fixtures/orca/h2_scan_mos.out'
PARSERS = REPO / 'scripts/parsers'


def records(output=FIXTURE, legacy=False):
    """Replay stream rules and File= rules, as the monitor and job store do."""
    desc = sim.read_desc(str(PARSERS / 'orca.desc'))
    for entry in desc.table.values():
        if entry.script == 'orca.scanmos':
            assert all(e.script == entry.script for e in desc.entries
                       if e.keys[0] == entry.keys[0]), 'scan script dispatch collides'
    out = {}
    def collect(text):
        for rec in sim.parse_parser_output(text):
            out.setdefault(rec['key'], []).append(rec)
    with tempfile.TemporaryDirectory(prefix='orca-scan-') as work:
        result = sim.replay(desc, str(output))
        for block in result.blocks:
            if block.delivered:
                text, err, rc = sim.run_parser(str(PARSERS), block.entry, block,
                    parse_args=('.', 'Geometry', 'Hartree Fock', 'RHF', '0'), workdir=work)
                assert rc == 0, err
                collect(text)
        for entry in desc.table.values():
            if 'file' not in entry.rules:
                continue
            name = entry.rules['file']
            if name == 'orca.orcain.allxyz':
                text = FIXTURE.with_suffix('.allxyz').read_text()
            elif name == 'orca.orcaout':
                text = Path(output).read_text()
                if legacy:
                    # Older/minimal output: only the final point's MOs survive.
                    blocks = list(re.finditer(r'^MOLECULAR ORBITALS.*?MULLIKEN POPULATION ANALYSIS[^\n]*', text, re.M | re.S))
                    for b in reversed(blocks[:-1]):
                        text = text[:b.start()] + text[b.end():]
            else:
                continue
            proc = subprocess.run(['perl', str(PARSERS / entry.script), '.', 'Geometry',
                                   'Hartree Fock', 'RHF', '0'], input=text,
                                  capture_output=True, text=True, cwd=work)
            assert proc.returncode == 0, proc.stderr
            collect(proc.stdout)
    return out


def values(rec):
    return [float(x) for x in rec['flat']['values'].split()]


def printed_energies():
    """Independent reference: ORBITAL ENERGIES, not the coefficient parser."""
    points = re.split(r'RELAXED SURFACE SCAN STEP\s+\d+', FIXTURE.read_text())[1:]
    return [[float(e) for e in re.findall(
        r'^\s*\d+\s+\d+\.\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s*$',
        p.split('ORBITAL ENERGIES')[-1].split('MOLECULAR ORBITALS')[0], re.M)]
        for p in points]


def main():
    rec = records()
    for key in ('PTORB', 'PTMO', 'SCANGEOM'):
        assert len(rec.get(key, [])) == 3, '%s: want three tables, got %d' % (key, len(rec.get(key, [])))
    for k, expected in enumerate(printed_energies()):
        assert len(expected) == 2
        got = values(rec['PTORB'][k])
        assert rec['PTORB'][k]['flat']['size'].split() == ['1', '2', '2']
        assert rec['PTMO'][k]['flat']['size'].split() == ['1', '2', '2']
        assert all(abs(a-b) <= 5.1e-6 for a,b in zip(got[::2], expected)), (k, got, expected)
        assert got[1::2] == [2., 0.]
    assert values(rec['PTORB'][0]) != values(rec['PTORB'][-1])
    assert values(rec['PTMO'][-1]) == values(rec['MO'][-1])
    assert values(rec['PTORB'][-1])[::2] == values(rec['ORBENG'][-1])
    old = records(legacy=True)
    assert 'PTMO' not in old and 'PTORB' not in old
    assert values(old['MO'][-1]) == values(rec['MO'][-1])
    print('PASS: three point tables match independently read printed energies; final-only output keeps final MOs')
    return 0


if __name__ == '__main__':
    sys.exit(main())
