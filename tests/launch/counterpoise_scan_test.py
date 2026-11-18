#!/usr/bin/env python3
"""Stored water-dimer relaxed scan -> one CP job -> Builder table/CSV/PNG (#1091)."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path[:0] = [str(HERE), str(REPO / 'tests/apps')]
import counterpoise_test as cp
import ecceqm_scan_panel_test as scan
import ecceqm_test as q
import builder_overlay_test as bo
import harness
import scan_panel_test as sp
import scan_compare_test as compare
import xdisplay


def vector(path, key, values, unit):
    root = ET.Element('vector', name=key, rows=str(len(values)), units=unit,
                      rowLabel='Scan Point')
    root.text = ' '.join('%.17g' % v for v in values)
    ET.ElementTree(root).write(path, encoding='utf-8')


def values(calc, key):
    return [float(v) for v in ' '.join(ET.parse(calc / 'Props' / key).getroot().itertext()).split()]


def input_checks(work):
    frames = []
    for shift in (0., .2, .4):
        frames.append(' '.join('%.10f %.10f %.10f' % (x + (shift if i >= 3 else 0), y, z)
                      for i, (_, x, y, z) in enumerate(cp.ATOMS)))
    for code in ('ecceqm', 'orca'):
        cp.writeFiles(str(work), code, code)
        with (work / (code + '.param')).open('a') as out:
            out.write('ES.Runtype.CPScanGeometry: ' + ';'.join(frames) + '\n')
        deck = cp.generate(str(work), code, code)
        assert deck and deck.count('# ecce-run ') == 14
        parts = cp.splitDeck(deck)
        assert len(parts) == 15
        assert 'point-000003-complex' in parts
        for point in (1, 2, 3):
            prefix = 'point-%06d-' % point
            ghost = parts[prefix + 'fragment-1']
            assert len(re.findall(r' ghost$' if code == 'ecceqm' else r'^\s*[A-Z][a-z]?:\s', ghost, re.M)) == 3
        assert parts['complex'] != parts['point-000003-complex']
    block = ['begin counterpoise', 'points 3', 'complex -152.6500']
    for point in (3, 1, 2):
        prefix = 'point-%06d-' % point
        if point != 1:
            block.append(prefix + 'complex -152.6500')
        for part, value in (('fragment-1', -76.325), ('fragment-2', -76.324),
                            ('fragment-1-own', -76.324), ('fragment-2-own', -76.3235)):
            block.append(prefix + part + ' ' + str(value))
    block.append('end counterpoise')
    def parse(lines):
        return scan.parsed(subprocess.run(['perl', str(REPO / 'scripts/parsers/counterpoise.parse')],
                           input='\n'.join(lines) + '\n', text=True, capture_output=True, check=True).stdout)
    parsed = parse(block)
    assert len(parsed['SCANCP'][0]['values'][0].split()) == 3
    assert not parse([line for line in block if 'point-000002-fragment-2-own' not in line])
    assert not parse([line.replace('point-000002-fragment-2 -76.324', 'point-000002-fragment-2 NaN') for line in block])
    harness.say('PASS: ECCE-QM/ORCA indexed decks and ordered complete-only scan parser')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', default=str(REPO / 'build-cmake'))
    ap.add_argument('--png')
    ap.add_argument('--offline', action='store_true')
    args = ap.parse_args()
    import tempfile
    cache = Path(os.environ.get('XDG_CACHE_HOME', REPO / '.test-cache'))
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='cpscan-input-', dir=cache) as work:
        input_checks(Path(work))
    if args.offline:
        return 0
    build = Path(args.build).resolve()
    harness.prerequisites(str(build), ())
    qm = build / 'src/qm/ecce-qm'
    if not qm.is_file() or not shutil.which('Xvfb'):
        harness.skip('ecce-qm or Xvfb missing')
    s = harness.Session(str(build), 'cpscan', {'ecce-qm': str(qm)}, local=True)
    display = None
    try:
        for exe in bo.GUI:
            harness.link(str(build / exe), os.path.join(s.home, 'bin', exe))
        harness.link(str(qm), os.path.join(s.home, 'bin', 'ecce-qm'))
        wrappers = Path(s.state) / 'wrappers'
        shutil.copytree(build / 'wrappers', wrappers)
        for entry in wrappers.iterdir():
            entry.chmod(0o755)
        assert s.services(True), 'services did not start'
        work = Path(s.state) / 'work'
        work.mkdir()
        cp.writeFiles(str(work), 'scan', 'ecceqm', counterpoise=False)
        param = work / 'scan.param'
        param.write_text(param.read_text().replace('Category: DFT', 'Category: SCF').replace(
            'Theory: DFT', 'Theory: HF').replace('ES.Theory.ECCEQM.Dispersion: 1\n', ''))
        deck = cp.generate(str(work), 'scan', 'ecceqm')
        assert deck
        deck = deck.replace('runtype energy', 'runtype optimize') + 'scan\nbond 1 4 2.8 3.2 3\nend\n'
        inp = work / 'scan.qmin'
        inp.write_text(deck)
        env = s.env({'OMP_NUM_THREADS': '2'})
        output = q.importFromOutput(env, str(work), 'scan', str(qm), str(inp))
        text = Path(output).read_text()
        tables = re.findall(r'^begin scan_table\b.*?^end scan_table\n', text, re.S | re.M)
        frames = re.findall(r'^begin scan_geometry\b.*?^end scan_geometry\n', text, re.S | re.M)
        assert tables and len(frames) == 3, text[-3000:]
        def parsed(block):
            return scan.parsed(subprocess.run(['perl', str(REPO / 'scripts/parsers/ecceqm.parse')],
                               input=block, capture_output=True, text=True, check=True).stdout)
        table = parsed(tables[-1])
        coord = [float(v) for v in table['SCANCOORD'][0]['values'][0].split()]
        energy = [float(v) for v in table['SCANENERGY'][0]['values'][0].split()]
        geometry = [[[float(v) for v in row.split()] for row in parsed(frame)['SCANGEOM'][0]['values']]
                    for frame in frames]
        assert len(coord) == len(energy) == 3
        jobs = Path(s.state) / 'jobs'
        jobs.mkdir(exist_ok=True)
        rc, out = s.driver('create', s.userUrl(), 'water-scan', q.RESOURCE, str(inp),
                           'ecceqm.qmin', 'localhost', str(jobs), s.user())
        assert rc == 0, out
        url = out.strip().splitlines()[-1]
        rc, out = s.driver('setup', url, str(work), Path(output).name)
        assert rc == 0, out
        # This is an isolated stored fixture from the engine's relaxed scan,
        # not a hand-crafted energy curve. Launch below exercises the CP job.
        calc = Path(url.removeprefix('file://').rstrip('/'))
        meta = calc / '.ecce-meta'
        meta.write_text(re.sub(r'(:state\t\t)[^\n]+', r'\g<1>Complete', meta.read_text()))
        molecule = calc / 'Parameters/chemsys.mvm'
        fragment_data = '\nfragment_states: 2\n1 0 1\n2 0 1\natom_fragment_list:\n1\n1\n1\n2\n2\n2\n'
        vector(calc / 'Props/SCANCOORD', 'SCANCOORD', coord, 'Angstrom')
        vector(calc / 'Props/SCANENERGY', 'SCANENERGY', energy, 'Hartree')
        (calc / 'Props/SCANGEOM').write_text(sp.tsvector(geometry))
        before = {key: (calc / 'Props' / key).read_bytes() for key in ('SCANCOORD', 'SCANENERGY', 'SCANGEOM')}
        display = xdisplay.Display().__enter__()
        env = s.env({'DISPLAY': display.name, 'GTK_THEME': 'Adwaita',
                     'PATH': str(wrappers) + os.pathsep + s.env()['PATH']})
        png = Path(args.png).resolve() if args.png else Path(s.state) / 'png'
        png.mkdir(parents=True, exist_ok=True)
        def builder(lines, tag):
            log, _ = bo.runScript(s, display, env, str(wrappers), ['-context', url],
                       ['wait 4000', 'background 1 1 1'] + lines + ['quit'], str(png), tag,
                       {'ECCE_OPEN_PANEL': 'Relaxed Scan'})
            (png / (tag + '.log')).write_text(log)
            assert 'segmentation' not in log.lower(), log
            return log
        refused = builder(['scan cpsingle'], 'no-fragments')
        assert 'at least two fragments' in refused, refused[-3000:]
        molecule.write_text(molecule.read_text() + fragment_data)
        log = builder(['scan cpcolumn', 'scan cpsingle', 'scan cpsingle'], 'prepare')
        match = re.search(r'BUILDER: scan cpsingle url=(\S+)', log)
        assert match, log[-5000:]
        cpurl = match[1]
        assert 'already has a counterpoise calculation' in log, log[-3000:]
        child = Path(cpurl.removeprefix('file://').rstrip('/'))
        cpdeck = (child / 'Inputs/ecceqm.qmin').read_text()
        assert cpdeck.count('# ecce-run ') == 14, cpdeck[:2000]
        assert 'runtype optimize' not in cpdeck and '\nscan\n' not in cpdeck
        assert s.driver('launch', cpurl, timeout=300)[0] == 0
        assert s.waitState(cpurl, seconds=600) == 'completed'
        corrected, raw, bsse = (values(child, key) for key in ('SCANCP', 'SCANRAW', 'SCANBSSE'))
        assert len(corrected) == len(raw) == len(bsse) == 3
        assert all(c > r and b > 0 and abs(c-r-b) < 1e-8 for c, r, b in zip(corrected, raw, bsse))
        # Recompute each point with the existing scalar EINTCP parser.
        result = next((child / 'Outputs').glob('*.qmout')).read_text()
        block = result.split('begin counterpoise\n')[-1].split('end counterpoise')[0]
        runs = [{} for _ in coord]
        for name, val in re.findall(r'^(complex|point-\d+-\S+)\s+([-+\d.eE]+)$', block, re.M):
            if name == 'complex':
                runs[0]['complex'] = float(val)
            else:
                _, point, part = name.split('-', 2)
                runs[int(point)-1][part] = float(val)
        for i, point in enumerate(runs):
            scalar, _ = cp.parseBlock(list(point.items()))
            assert abs(corrected[i] - scalar['EINTCP']) < 1e-9
        csv = Path(s.state) / 'cp.csv'
        kcal = Path(s.state) / 'cp-kcal.csv'
        log = builder(['scan cpcolumn', 'scancsv ' + str(csv), 'scancsv ' + str(kcal) + ' kcal',
                       'scanpng ' + str(png / 'counterpoise.png') + ' 1000 560'], 'result')
        rows = re.findall(r'^([1-3]) (\S+) (\S+) (\S+)$', log, re.M)
        assert len(rows) == 3, log[-5000:]
        for row, c, r in zip(rows, corrected, raw):
            assert abs(float(row[3])-c) < 1e-9 and abs(float(row[2])-r) < 1e-9
        for path, scale in ((csv, 2625.4996), (kcal, 627.5095)):
            blocks = compare.blocks(path)
            assert len(blocks) == 2
            for block, energies in zip(blocks, (raw, corrected)):
                assert len(block) == 3
                for i, (row, value) in enumerate(zip(block, energies)):
                    assert abs(float(row['energy_hartree']) - value) < 1e-9
                    assert abs(float(row['scan_energy_hartree']) - energy[i]) < 1e-9
                    assert abs(float(row['cp_energy_hartree']) - corrected[i]) < 1e-9
                    key = 'interaction_energy_' + ('kcal/mol' if path == kcal else 'kJ/mol')
                    assert abs(float(row[key]) - value * scale) < 1e-7
        assert (png / 'counterpoise.png').read_bytes().startswith(b'\x89PNG')
        assert all((calc / 'Props' / key).read_bytes() == val for key, val in before.items())
        harness.say('PASS: one calculation, 15 decks in one launch, three SCANCP values equal per-point EINTCP')
        harness.say('PASS: CP interaction energies above raw, persisted table, two-curve CSV/PNG, original scan unchanged')
        return 0
    except (AssertionError, OSError, subprocess.SubprocessError) as exc:
        import traceback
        traceback.print_exc()
        harness.say('FAIL: ' + str(exc))
        return 1
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()


if __name__ == '__main__':
    sys.exit(main())
