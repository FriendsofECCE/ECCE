#!/usr/bin/env python3
"""Render synthetic atomic spectra with the real wx canvas, isolated under Xvfb.

Run inside a Slurm allocation. Optional argument: directory for PNGs/listings.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

import xdisplay

REPO = Path(__file__).resolve().parents[2]


def main():
    output = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else REPO / 'build-cmake/aufbau-layout'
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=REPO / 'build-cmake', prefix='aufbau-layout-') as work:
        env = dict(os.environ, HOME=work, ECCE_REALUSERHOME=work,
                   XDG_CACHE_HOME=work, GTK_THEME='Adwaita', GDK_BACKEND='x11')
        env.pop('WAYLAND_DISPLAY', None)
        binary = str(Path(work) / 'layout')
        flags = shlex.split(subprocess.check_output(['wx-config', '--cxxflags', '--libs'], text=True))
        subprocess.run(['c++', '-std=c++17', '-I'+str(REPO / 'include'),
                        '-I'+str(REPO / 'src/apps/builder'),
                        str(REPO / 'tests/builder/aufbau_layout_test.C'),
                        '-o', binary] + flags, check=True, env=env)
        with xdisplay.Display() as display:
            subprocess.run([binary, str(output)], check=True, env=display.env(env), timeout=120)
    failures = []
    for path in sorted(output.glob('*.txt')):
        scaled = path.stem.endswith('-scale')
        name, dimensions = path.stem.removesuffix('-scale').split('-')
        width, height = map(int, dimensions.split('x'))
        text = path.read_text()
        rects = []
        for line in text.splitlines():
            m = re.match(r'rect (\S+) (-?\d+) (-?\d+) (\d+) (\d+) (.*)', line)
            if m:
                rects.append((m[1], *map(int, m.group(2, 3, 4, 5)), m[6]))
        problems = []
        layout = re.search(r'boxes size=(\d+) wraps=(\d+) core=(\S+)', text)
        if not scaled and (not layout or int(layout[1]) < 18):
            problems.append('missing or unreadable boxes')
        if layout:
            if width == 900 and (int(layout[2]) or layout[3] != 'none'):
                problems.append('wide panel wraps or folds a core')
            if name in ('Xe', 'reordered') and width == 500 and (int(layout[2]) == 0 or layout[3] != 'none'):
                problems.append('heavy case must wrap without folding at 500x400')
            if name in ('Xe', 'reordered') and width == 320 and layout[3] == 'none':
                problems.append('small heavy case must fold a noble-gas core')
        for i, r in enumerate(rects):
            if not (r[1] >= 0 and r[2] >= 0 and r[1]+r[3] <= width and r[2]+r[4] <= height):
                problems.append('outside panel: '+str(r))
            for q in rects[i+1:]:
                if max(r[1], q[1]) < min(r[1]+r[3], q[1]+q[3]) and max(r[2], q[2]) < min(r[2]+r[4], q[2]+q[4]):
                    problems.append('overlap: '+str((r, q)))
        boxes = [r for r in rects if r[0] == 'box']
        for heading in (r for r in rects if r[0] == 'text' and r[5].startswith('n = ')):
            shell_boxes = [r for r in boxes if r[5].startswith(heading[5][-1])]
            if not shell_boxes or heading[2] <= max(r[2]+r[4] for r in shell_boxes):
                problems.append('heading is not at column foot')
        hits = re.findall(r'^hit (\d+) (\d+)$', text, re.M)
        levels = [r for r in rects if r[0] == ('level' if scaled else 'box')]
        if len(hits) != len(levels) or any(a != b for a, b in hits):
            problems.append('orbital hit targets differ')
        ys = {r[5]: r[2] for r in boxes}
        ranks = re.findall(r'^shell (\S+)', text, re.M)
        ordered = [ys[label] for label in ranks if label in ys]
        if any(a <= b for a, b in zip(ordered, ordered[1:])):
            problems.append('energy ranks do not increase upwards')
        if layout and layout[3] == 'none':
            if len(boxes) != len(re.findall(r'^  orbital ', text, re.M)):
                problems.append('an unfolded orbital is missing')
        if scaled:
            if not any(r[5] == 'Energy (eV)' and r[2] < min(q[2] for q in levels) for r in rects):
                problems.append('axis title is not above the axis')
            core_labels = re.findall(r'^shell (\S+) core=1', text, re.M)
            core_ys = [next(r[2] for r in levels if r[5] == label) for label in core_labels]
            if any(a <= b for a, b in zip(core_ys, core_ys[1:])):
                problems.append('core order is inverted')
            values = [r for r in rects if r[0] == 'text' and r[5].endswith(' eV')]
            if (width >= 500 and not values) or any(not re.fullmatch(r'−?\d+\.\d eV', r[5]) for r in values):
                problems.append('missing or incorrectly formatted energy values')
            if width >= 500 and not any(r[5] == '−3200.0 eV' for r in values):
                problems.append('core energy value is missing')
            if width >= 500 and sum(r[5] == '−300.0 eV' for r in values) != 1:
                problems.append('degenerate core value is missing or repeated')
            if name == 'O' and any(r[5] == '−250.0 eV' for r in values):
                problems.append('crowded split-level value was not omitted')
            first_x = {r[5]: min(q[1] for q in levels if q[5] == r[5]) for r in levels}
            for angular in ('s', 'p', 'd'):
                if len({x for label, x in first_x.items() if label.endswith(angular)}) > 1:
                    problems.append('core and valence columns differ')
            if width == 900:
                starts = sorted({r[1] for r in levels if r[5] in ('1s', '2p', '3d')})
                if any(b-a > 270 for a, b in zip(starts, starts[1:])):
                    problems.append('angular columns spread too far apart')
            print(('FAIL ' if problems else 'PASS ')+path.stem+': '+('; '.join(problems) if problems else 'core order, values, group spacing, hits, no overlaps'))
            failures.extend(problems)
            continue
        config_lines = []
        for r in rects:
            if r[0] == 'text' and r[5] == 'not to scale':
                break
            if r[0] == 'text':
                config_lines.append(r[5])
        drawn_config = ' '.join(config_lines).translate(str.maketrans('⁰¹²³⁴⁵⁶⁷⁸⁹', '0123456789'))
        expected_config = re.search(r'^config (.*)', text, re.M)[1]
        if drawn_config != expected_config:
            problems.append('drawn configuration is incomplete')
        if name in ('Xe', 'reordered'):
            config = re.search(r'^config (.*)', text, re.M)[1]
            if '1s2' not in config or '5p6' not in config:
                problems.append('full configuration missing')
        print(('FAIL ' if problems else 'PASS ')+path.stem+': '+('; '.join(problems) if problems else 'fit, ranks, headings, hits, no overlaps'))
        failures.extend(problems)
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
