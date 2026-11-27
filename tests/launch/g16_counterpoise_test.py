#!/usr/bin/env python3
"""Gaussian 16 native counterpoise decks and stored water-dimer log (#1091).

--offline checks decks and replays the fixture through the monitor descriptor.
--run-g16 EXE runs the generated B3LYP/def2-SVP deck and writes a trimmed fixture.
Run under the site's compute scheduler when using --run-g16.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
PARSERS = REPO / 'scripts/parsers'
sys.path.insert(0, str(HERE))
import counterpoise_test as cp
sys.path.insert(0, str(REPO / 'tests/parsers'))
from eccejobmonitor_sim import read_desc, replay, run_parser, parse_parser_output

FIXTURE = REPO / 'tests/data/g16_water_dimer_counterpoise.txt'


def generate(work, enabled=True, ids=(1, 2), states=((0, 1), (0, 1)),
             runtype="Energy", own=False):
    cp.writeFiles(str(work), 'dimer', 'gauss16', counterpoise=enabled)
    param = work / 'dimer.param'
    # This fixture exercises B3LYP without a dispersion option.
    param.write_text(
        'title: Water dimer\nannotation: Water dimer counterpoise\nMemorySize: 1\n'
        'Category: DFT\nTheory: RDFT\nRunType: Energy\nCharge: 0\n'
        'ChemSys.Multiplicity: 1\nES.Theory.DFT.XCFunctionals: B3LYP (hybrid)\n'
        + ('ES.Runtype.Counterpoise: 1\n' if enabled else '')
        + ('ES.Runtype.CounterpoiseOwn: 1\n' if own else ''))
    param.write_text(param.read_text().replace('RunType: Energy', 'RunType: ' + runtype))
    frag = work / 'dimer.frag'
    if enabled:
        s = frag.read_text().split('fragment_states:')[0]
        s += 'fragment_states: 2\n' + ''.join('%d %d %d\n' % (i, *st)
                                                   for i, st in zip(ids, states))
        s += 'atom_fragment_list:\n' + ''.join('%d\n' % i for i in (ids[0],)*3 + (ids[1],)*3)
        frag.write_text(s)
    env = dict(os.environ, ECCE_HOME=str(REPO))
    with (work / 'dimer.gbs').open() as inp:
        basis = subprocess.run([sys.executable, str(PARSERS / 'std2Gaussian-16')],
                               stdin=inp, capture_output=True, env=env, check=True)
    (work / 'dimer.basis').write_bytes(basis.stdout)
    shutil.copy(PARSERS / 'g16.tpl', work / 'dimer.tpl')
    result = subprocess.run(['perl', str(PARSERS / 'ai.gauss16'), '-p', '-f', '-b',
                             '-n', 'dimer', '-t', 'dimer.tpl'],
                            cwd=work, env=env, capture_output=True, text=True)
    return result, (work / 'dimer.tpl').read_text()


def parser_check(work, path=FIXTURE):
    desc = read_desc(str(PARSERS / 'gaussian-16.desc'))
    result = replay(desc, str(path))
    assert not result.ambiguous, result.ambiguous
    records = {}
    for entry in desc.live_entries():
        if entry.script != 'gaussian-16.counterpoise':
            continue
        blocks = result.delivered_for(entry)
        assert len(blocks) == 1, 'counterpoise summary must reach its parser'
        out, err, rc = run_parser(str(PARSERS), entry, blocks[0],
                                  ('.', 'Energy', 'DFT', 'B3LYP', '0'), workdir=str(work))
        assert rc == 0, err
        records = {r['key']: r['flat'] for r in parse_parser_output(out)}
    assert set(records) == {'EINTCP', 'EINTRAW', 'EBSSE', 'CPTABLE'}, records
    log = path.read_text()
    raw, corrected = map(float, re.search(
        r'complexation energy =\s*([-\d.]+) kcal/mole \(raw\)\s*'
        r'complexation energy =\s*([-\d.]+) kcal/mole \(corrected\)', log).groups())
    bsse = float(re.search(r'BSSE energy =\s*([-\d.]+)', log).group(1))
    total = float(re.search(r'Counterpoise corrected energy =\s*([-\d.]+)', log).group(1))
    fragments = float(re.search(r'sum of fragments =\s*([-\d.]+)', log).group(1))
    for key, want, tol in [('EINTRAW', total - fragments - bsse, 1e-10),
                           ('EINTCP', total - fragments, 1e-10),
                           ('EBSSE', bsse, 1e-10)]:
        assert records[key]['units'] == 'Hartree'
        assert abs(float(records[key]['values']) - want) < tol, key
    assert abs(float(records['EINTRAW']['values']) * 627.5094740631 - raw) < 0.0051
    assert abs(float(records['EINTCP']['values']) * 627.5094740631 - corrected) < 0.0051
    assert 'kJ/mol' in records['CPTABLE']['values'] and 'kcal/mol' in records['CPTABLE']['values']
    incomplete = subprocess.run(['perl', str(PARSERS / 'gaussian-16.counterpoise')],
                                input=log[:log.index('complexation energy')],
                                capture_output=True, text=True, check=True)
    assert not incomplete.stdout, 'an incomplete summary must not publish properties'
    print('PASS Gaussian 16 descriptor/parser: EINTCP, EINTRAW, EBSSE, CPTABLE')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--offline', action='store_true')
    ap.add_argument('--run-g16', metavar='EXE')
    args = ap.parse_args()
    cache = REPO / '.test-cache'
    cache.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='g16-cp-', dir=cache) as temp:
        work = Path(temp)
        result, deck = generate(work)
        assert result.returncode == 0, result.stderr
        assert 'Counterpoise=2' in deck and '# ecce-run' not in deck
        assert re.search(r'^0 1 0 1 0 1(?:\s*!.*)?$', deck, re.M), deck
        for f in (1, 2):
            assert len(re.findall(r'^\s*[OH]\(Fragment=%d\)' % f, deck, re.M)) == 3
        if args.run_g16:
            exe = Path(args.run_g16).resolve()
            env = dict(os.environ, GAUSS_EXEDIR=str(exe.parent),
                       g16root=str(exe.parent.parent), GAUSS_SCRDIR=str(work), HOME=str(work))
            with (work / 'full.log').open('w') as out:
                run = subprocess.run([str(exe)], input='%nprocshared=2\n' + deck,
                                     text=True, stdout=out, stderr=subprocess.STDOUT,
                                     cwd=work, env=env)
            log = (work / 'full.log').read_text()
            assert run.returncode == 0 and 'Normal termination' in log, log[-3000:]
            parser_check(work, work / 'full.log')
            # Keep the generated input and native summary for offline parser checks.
            summary = log.index(' Counterpoise corrected energy')
            FIXTURE.parent.mkdir(exist_ok=True)
            trimmed = deck + '\n' + log[summary:log.index(' Leave Link', summary)]
            FIXTURE.write_text('\n'.join(line.rstrip() for line in trimmed.splitlines()) + '\n')
            print('PASS real Gaussian 16 water-dimer B3LYP/def2-SVP run')
        result, with_own = generate(work, own=True)
        assert result.returncode == 0 and with_own == deck
        result, _ = generate(work, runtype='Gradient')
        assert result.returncode != 0 and 'for Energy runs' in result.stderr
        result, plain = generate(work, enabled=False)
        assert result.returncode == 0 and 'Counterpoise=' not in plain and '(Fragment=' not in plain
        result, ionic = generate(work, ids=(7, 42), states=((1, 2), (-1, 2)))
        assert result.returncode == 0, result.stderr
        assert re.search(r'^0 1 1 2 -1 2(?:\s*!.*)?$', ionic, re.M)
        assert '(Fragment=1)' in ionic and '(Fragment=2)' in ionic and '(Fragment=42)' not in ionic
        result, _ = generate(work, states=((1, 2), (0, 1)))
        assert result.returncode != 0 and 'charges add up' in result.stderr
        generate(work, enabled=False)
        with (work / 'dimer.param').open('a') as out:
            out.write('ES.Runtype.Counterpoise: 1\n')
        result = subprocess.run(['perl', str(PARSERS / 'ai.gauss16'), '-p', '-f', '-b',
                                 '-n', 'dimer', '-t', 'dimer.tpl'], cwd=work,
                                env=dict(os.environ, ECCE_HOME=str(REPO)), capture_output=True, text=True)
        assert result.returncode != 0 and 'at least two fragments' in result.stderr
        print('PASS Gaussian 16 decks: fragment tags/states, sparse IDs, disabled option, validation')
        parser_check(work)


if __name__ == '__main__':
    main()
