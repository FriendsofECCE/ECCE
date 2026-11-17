#!/usr/bin/env python3
"""Combined Relaxed Scan CSV/PNG and script controls, with isolated stored scans."""
import argparse
import csv
import io
import os
import re
from pathlib import Path
import shutil
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path[:0] = [str(HERE), str(REPO / 'tests/launch')]
import builder_overlay_test as bo
import harness

SOURCE = REPO / 'data/client/examples/computational/h2-dissociation/orca-h2_rhf_scan/stored/h2_rhf_scan'
SCALE = 2625.4996


def fixture(path, indices, degrees=False, structures=True):
    """Keep three real scan points and the stored calculation's structure/setup."""
    path.mkdir(parents=True)
    shutil.copy(SOURCE / '.ecce-meta', path)
    shutil.copytree(SOURCE / 'Parameters', path / 'Parameters')
    props = path / 'Props'
    props.mkdir()
    values = []
    for key in ('SCANCOORD', 'SCANENERGY'):
        root = ET.parse(SOURCE / 'Props' / key).getroot()
        data = [float(v) for v in ' '.join(root.itertext()).split()]
        chosen = [data[i] for i in indices]
        values.append(chosen)
        root.text = ' '.join('%.12g' % v for v in chosen)
        root.set('rows', str(len(chosen)))
        if degrees and key == 'SCANCOORD':
            root.set('units', 'Degree')
        ET.ElementTree(root).write(props / key, encoding='utf-8')
    if structures:
        root = ET.parse(SOURCE / 'Props/SCANGEOM').getroot()
        steps = list(root)
        for child in list(root):
            root.remove(child)
        for i, n in enumerate(indices):
            step = steps[n]
            step.set('number', str(i + 1))
            root.append(step)
        ET.ElementTree(root).write(props / 'SCANGEOM', encoding='utf-8')
    return values


def blocks(path):
    result = []
    current = []
    for line in path.read_text().splitlines():
        if line.startswith('# ECCE relaxed scan'):
            if current:
                result.append(list(csv.DictReader(io.StringIO('\n'.join(current)))))
            current = []
        elif line and not line.startswith('#'):
            current.append(line)
    if current:
        result.append(list(csv.DictReader(io.StringIO('\n'.join(current)))))
    return result


def verify(path, curves, mode, kcal=False):
    got = blocks(path)
    assert len(got) == len(curves), (path, len(got), len(curves))
    for rows, (x, e, first) in zip(got, curves):
        assert len(rows) == len(x), (path, rows)
        zero = min(e) if mode == 'lowest' else e[x.index(max(x))] if mode == 'largest' else first
        for row, coordinate, energy in zip(rows, x, e):
            assert abs(float(row['coordinate_angstrom']) - coordinate) < 1e-8
            assert abs(float(row['energy_hartree']) - energy) < 1e-9
            key = 'relative_energy_' + ('kcal/mol' if kcal else 'kJ/mol')
            assert abs(float(row[key]) - (energy - zero) * (627.5095 if kcal else SCALE)) < 1e-7
    harness.say('PASS: %s (%s)' % (path.name, mode))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', default=str(REPO / 'build-cmake'))
    ap.add_argument('--png', help='Keep combined plot exports here')
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ())
    if not shutil.which('Xvfb') or not SOURCE.is_dir():
        harness.skip('Xvfb or stored scan fixture missing')
    import xdisplay
    s = harness.Session(build, 'scancompare', {}, local=True)
    display = None
    try:
        for exe in bo.GUI:
            harness.link(os.path.join(build, exe), os.path.join(s.home, 'bin', exe))
        wrappers = Path(s.state) / 'wrappers'
        shutil.copytree(Path(build) / 'wrappers', wrappers)
        for entry in wrappers.iterdir():
            entry.chmod(0o755)
        if not s.services(True):
            raise AssertionError('services did not start')
        display = xdisplay.Display().__enter__()
        env = s.env({'DISPLAY': display.name, 'GTK_THEME': 'Adwaita',
                     'PATH': str(wrappers) + os.pathsep + s.env()['PATH']})
        project = Path(s.localData()) / 'users/local/comparisons'
        project.mkdir(parents=True)
        shutil.copy(bo.PROJECT_META, project / '.ecce-meta')
        for folder in ('inward', 'outward', 'other'):
            (project / folder).mkdir()
            shutil.copy(bo.PROJECT_META, project / folder / '.ecce-meta')
        own = project / 'inward/scan'
        other = project / 'outward/scan'
        unmatched = project / 'other/unmatched'
        a = fixture(own, [20, 21, 22])
        b = fixture(other, [20, 19, 18])
        c = fixture(unmatched, [25, 26, 27])
        invalid = project / 'other/angle'
        fixture(invalid, [20, 19, 18], degrees=True)
        missing = project / 'other/no-structures'
        fixture(missing, [20, 19, 18], structures=False)
        out = Path(s.state)
        png = Path(args.png).resolve() if args.png else out / 'png'
        png.mkdir(parents=True, exist_ok=True)
        lines = ['wait 6000', 'background 1 1 1',
                 'scancompare %s %s' % (other.as_uri(), unmatched),
                 'scancsv %s/default.csv' % out]
        for join in (False, True):
            lines.append('scanjoin ' + ('on' if join else 'off'))
            for mode in ('lowest', 'largest', 'first'):
                lines += ['scanzero ' + mode,
                          'scancsv %s/%s-%s.csv' % (out, 'join' if join else 'separate', mode)]
        lines += ['scancsv %s/kcal.csv kcal' % out,
                  'scanlayout multiples', 'scanpng %s/multiples.png 1000 900' % png,
                  'scanlayout overlay', 'scanpng %s/overlay.png 1000 560' % png,
                  'scancompare %s' % invalid.as_uri(), 'scancsv %s/after-invalid.csv' % out,
                  'scancompare %s' % missing.as_uri(), 'scancsv %s/missing.csv' % out,
                  'scancompare none', 'scanjoin off', 'scanzero lowest',
                  'scancsv %s/clear.csv' % out, 'scanclick 1',
                  'scanview graph', 'scanview table', 'scanrow 1', 'scanview graph', 'quit']
        text, _ = bo.runScript(s, display, env, str(wrappers), ['-context', own.as_uri()],
                               lines, str(png), 'combined',
                               {'ECCE_PANEL_MODE': 'detail', 'ECCE_OPEN_PANEL': 'Relaxed Scan'})
        (png / 'builder.log').write_text(text)
        assert 'segmentation' not in text.lower(), text[-3000:]
        assert 'scancsv points=3 ok' in text, text[-3000:]
        separate = [(x, e, e[0]) for x, e in (a, b, c)]
        pairs = sorted(zip(a[0] + b[0][1:], a[1] + b[1][1:]))
        def ordered(curve):
            x, e, first = curve
            points = sorted(zip(x, e))
            return ([p[0] for p in points], [p[1] for p in points], first)
        joined = [([p[0] for p in pairs], [p[1] for p in pairs], a[1][0]), ordered(separate[2])]
        verify(out / 'default.csv', separate, 'lowest')
        for mode in ('lowest', 'largest', 'first'):
            verify(out / ('separate-%s.csv' % mode), separate, mode)
            verify(out / ('join-%s.csv' % mode), joined, mode)
        verify(out / 'kcal.csv', joined, 'first', kcal=True)
        verify(out / 'after-invalid.csv', joined, 'first')
        assert 'compatible coordinate' in text, text[-3000:]
        verify(out / 'missing.csv', [ordered(p) for p in separate[:2]], 'first')
        verify(out / 'clear.csv', separate[:1], 'lowest')
        assert text.count('BUILDER: scanpng saved') == 2, text[-3000:]
        assert all((png / n).read_bytes().startswith(b'\x89PNG') for n in ('multiples.png', 'overlay.png'))
        assert 'scanclick 1 frame=1' in text, text[-3000:]
        views = re.findall(r'BUILDER: scanview table=(\d) plotShown=(\d) gridShown=(\d)', text)
        assert views == [('0', '1', '0'), ('1', '0', '1'), ('0', '1', '0')], views
        assert 'scanrow 1 point=1 row=1 marker=1' in text, text[-3000:]
        harness.say('PASS: Graph/Table visibility and table-to-structure selection')
        harness.say('PASS: cross-folder URLs/paths, duplicate labels, PNG layouts, incompatible scan refused, missing structures kept separate, clear and point selection')
        return 0
    except (AssertionError, FileNotFoundError) as exc:
        harness.say('FAIL: ' + str(exc))
        return 1
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()


if __name__ == '__main__':
    sys.exit(main())
