#!/usr/bin/env python3
"""CPK circle silhouettes, PNG export, and a 6400-atom GROMACS orbit (#880).

Run with --out DIR to retain PNGs and metrics; --baseline DIR compares frame
medians with an earlier installed build.  Slurm allocation belongs to caller.
"""
import argparse
import json
import math
import os
import re
import shutil
import subprocess
import tempfile

import apps
import isolate
import xdisplay

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BOX = os.path.join(ROOT, 'data/client/examples/genchem/gases-liquids-solids/'
                   'gromacs-decane-opls/1-dynamics/start.gro')


def circle(path):
    from PIL import Image
    image = Image.open(path).convert('RGB')
    bg = image.getpixel((0, 0))
    w, h = image.size
    mask = {(x, y) for y in range(h) for x in range(w)
            if image.getpixel((x, y)) != bg}
    edge = [(x, y) for x, y in mask if any((x+dx, y+dy) not in mask
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))]
    if not edge:
        raise AssertionError('empty sphere image: ' + path)
    cx = (min(x for x, y in edge) + max(x for x, y in edge)) / 2
    cy = (min(y for x, y in edge) + max(y for x, y in edge)) / 2
    radii = [math.hypot(x-cx, y-cy) for x, y in edge]
    # A rasterized circle's inner boundary varies by up to one pixel.
    # The outermost boundary estimates the analytic radius without fitting
    # away the inward bowing of polygon edges.
    return {'radius_px': max(radii), 'max_deviation_px': max(radii)-min(radii)}


def cell_circle(path):
    """Measure a full boundary-image sphere under orthographic projection."""
    import numpy as np
    from PIL import Image
    pixels = np.asarray(Image.open(path).convert('RGB'), dtype=float)
    h, w = pixels.shape[:2]
    # Hue locates the atom, but grey ambient light must remain in its rim.
    quarter = pixels[:h//2, :w//2]
    r, g, b = quarter.transpose(2, 0, 1)
    violet = (r > 1.2*g) & (b > 1.2*g)
    ys, xs = np.nonzero(violet)
    if len(xs) < 20:
        raise AssertionError('no boundary-image silhouette: ' + path)
    y0, y1 = max(0, ys.min()-20), min(h//2, ys.max()+21)
    x0, x1 = max(0, xs.min()-20), min(w//2, xs.max()+21)
    mask = np.any(quarter[y0:y1, x0:x1] != pixels[0, 0], axis=2)

    def interior(m):
        return (np.roll(m, 1, 0) & np.roll(m, -1, 0) &
                np.roll(m, 1, 1) & np.roll(m, -1, 1))

    # Remove one-pixel cell lines; both silhouettes receive the same inset.
    mask &= interior(mask)
    ys, xs = np.nonzero(mask & ~interior(mask))
    cx = (xs.min() + xs.max()) / 2
    cy = (ys.min() + ys.max()) / 2
    radii = np.hypot(xs-cx, ys-cy)
    return {'radius_px': float(radii.max()),
            'max_deviation_px': float(radii.max()-radii.min()),
            'samples': len(xs)}


def run(display, structure, out, scene, orthographic=False):
    os.makedirs(out)
    script = os.path.join(out, 'scene')
    with open(script, 'w') as f:
        f.write(scene)
    extra = {}
    if orthographic:
        controls = os.path.join(out, 'controls')
        with open(controls, 'w') as f:
            f.write('menu Render|Enable Orthographic\n')
        extra = {'ECCE_BUILDER_SCRIPT': controls, 'ECCE_VIEWER_SCENE_HOLD': '0'}
    result = apps.run(display, 'builder', args=('-context', 'file://' + structure), windowTimeout=120,
                      settle=180, env={'ECCE_VIEWER_SCENE': script,
                      'ECCE_VIEWER_SCENE_OUT': out,
                      'ECCE_TRANSPARENCY_FALLBACK_MS': '0',
                      'ECCE_TEST_XYZ_UNITS': 'angstrom',
                      'LP_NUM_THREADS': '2', 'LIBGL_ALWAYS_SOFTWARE': '1', **extra})
    with open(os.path.join(out, 'builder.log'), 'w') as f:
        f.write(result.log or '')
    first_shot = re.search(r'^snap (\S+)', scene, re.M)[1] + '.ppm'
    if (result.crashed or os.path.exists(os.path.join(out, 'FAILED'))
            or not os.path.exists(os.path.join(out, first_shot))):
        raise AssertionError('Builder scene failed: ' + out)


def analyze(out, baseline=None, bonds_only=False):
    from PIL import Image
    metrics = {}
    if not bonds_only:
        cell_log = open(os.path.join(out, 'cell/builder.log')).read()
        camera = re.search(r'SCENE camera orthographic:.* size ([0-9.e+-]+)', cell_log)
        assert camera and float(camera[1]) > 1, 'cell camera is not orthographic'
        for name in ('automatic', 'explicit50', 'explicit100'):
            path = os.path.join(out, 'small', name + '.ppm')
            metrics[name] = circle(path)
            Image.open(path).save(os.path.join(out, name + '.png'))
        Image.open(os.path.join(out, 'cell/automatic.ppm')).save(os.path.join(out, 'cell.png'))
        metrics['cell_export'] = cell_circle(os.path.join(out, 'cell/export.png'))
        metrics['export'] = circle(os.path.join(out, 'small/export.png'))
        assert metrics['automatic']['max_deviation_px'] < 1.8
        assert metrics['export']['max_deviation_px'] < 1.8
    styles = ('bs',) if bonds_only else ('cpk', 'bs')
    for name in styles:
        folder = 'large-bs' if name == 'bs' and os.path.isdir(os.path.join(out, 'large-bs')) else 'large'
        text = open(os.path.join(out, folder, name + '.txt')).read().split()
        metrics[name] = {text[i]: float(text[i+1]) for i in range(0, len(text), 2)}
        Image.open(os.path.join(out, folder, name + '.ppm')).save(
            os.path.join(out, 'large-' + name + '.png'))
    with open(os.path.join(out, 'quality-metrics.json'), 'w') as f:
        json.dump(metrics, f, indent=2)
    print(json.dumps(metrics, indent=2))
    if baseline:
        if not bonds_only:
            def info(name):
                return dict(line.strip().split(' ', 1) for line in
                            open(os.path.join(out, 'small', name + '.txt')))
            automatic, chosen = info('auto'), info('user50')
            cell_info = dict(line.strip().split(' ', 1) for line in
                             open(os.path.join(out, 'cell/cellinfo.txt')))
            large_info = dict(line.strip().split(' ', 1) for line in
                              open(os.path.join(out, 'large/largeinfo.txt')))
            assert cell_info['cell_images'] == '7'
            assert large_info['quality_atoms'] == '6400'
            assert automatic['sphere_auto'] == automatic['cylinder_auto'] == '1'
            assert chosen['sphrq'] == '50' and chosen['sphere_auto'] == '0'
            assert chosen['cylinder_auto'] == '1'
            assert chosen['stored_sphere'] == chosen['roundtrip_sphere'] == '50 0'
            assert chosen['stored_cylinder'] == chosen['roundtrip_cylinder'] == '50 1'
            assert metrics['explicit50']['max_deviation_px'] > metrics['automatic']['max_deviation_px']
            print('PASS explicit slider50 survives serialization and preferences; cylinder stays automatic')
            old_circle = cell_circle(os.path.join(baseline, 'cell/export.png'))
            cell_ratio = metrics['cell_export']['max_deviation_px'] / old_circle['max_deviation_px']
            print('boundary-image silhouette %.3f -> %.3f px; ratio %.3f' % (
                  old_circle['max_deviation_px'], metrics['cell_export']['max_deviation_px'], cell_ratio))
            assert cell_ratio < 0.8, 'boundary sphere facets did not improve clearly'
        old_metrics = 'quality-metrics.json' if os.path.exists(os.path.join(baseline, 'quality-metrics.json')) else 'metrics.json'
        before = json.load(open(os.path.join(baseline, old_metrics)))
        for name in styles:
            # Pixel comparisons need matching camera angles; a timed orbit
            # can stop early at its time cap.
            same_camera = name == 'cpk' or all(os.path.isdir(os.path.join(d, 'large-bs')) for d in (out, baseline))
            if same_camera:
                old = Image.open(os.path.join(baseline, 'large-' + name + '.png'))
                new = Image.open(os.path.join(out, 'large-' + name + '.png'))
                identical = old.size == new.size and old.tobytes() == new.tobytes()
                print('%s large-box pixels identical: %s' % (name, identical))
                assert identical, name + ' large-box geometry changed'
            else:
                print('BS pixel comparison omitted: camera angles differ after the CPK orbit')
            ratio = metrics[name]['median_ms'] / before[name]['median_ms']
            print('%s frame-time %.2f -> %.2f ms; ratio %.3f' % (
                  name, before[name]['median_ms'], metrics[name]['median_ms'], ratio))
            assert ratio <= 1.10, name + ' slowed by over 10%'
    print('PASS quality measurements' + (' and baseline comparison' if baseline else ' captured'))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True)
    parser.add_argument('--baseline')
    parser.add_argument('--analyze-only', action='store_true', help='reuse existing captures')
    parser.add_argument('--bonds-only', action='store_true', help='capture a fresh-camera bond benchmark')
    args = parser.parse_args()
    out = os.path.abspath(args.out)
    os.makedirs(out, exist_ok=True)
    if args.analyze_only:
        return analyze(out, args.baseline, args.bonds_only)
    # Keep service state inside the checkout so cleanup obeys worktree limits.
    os.environ.setdefault('XDG_CACHE_HOME', os.path.join(ROOT, '.test-cache'))
    settings = isolate.apply(apps.INSTALL)
    os.environ['HOME'] = settings['ECCE_REALUSERHOME']
    os.environ['ECCE_NO_REAP'] = '1'
    work = tempfile.mkdtemp(prefix='quality-', dir=settings['ECCE_REALUSERHOME'])
    os.environ['ECCE_LOCAL_DATA'] = os.path.join(work, 'data')
    os.makedirs(os.path.join(os.environ['ECCE_LOCAL_DATA'], 'users', 'local'))
    atom = os.path.join(work, 'atom.xyz')
    with open(atom, 'w') as f:
        f.write('1\nCPK sphere silhouette\nC 0 0 0\n')
    cell = os.path.join(work, 'cell.pdb')
    with open(cell, 'w') as f:
        f.write('CRYST1    8.000    8.000    8.000  90.00  90.00  90.00 P 1           1\n'
                'HETATM    1  NA  NA  A   1       0.000   0.000   0.000  1.00  0.00          NA\nEND\n')
    try:
        with xdisplay.Display() as display:
            subprocess.run([os.path.join(apps.INSTALL, 'bin/ecce-gateway-start')],
                           env=display.env(), timeout=180, check=True)
            try:
                if not args.bonds_only:
                    run(display, atom, os.path.join(out, 'small'),
                        'style CPK\nviewall\nzoom 0.9\nsnap automatic\nvizfile export 480\n'
                        'dsstyle info auto\ndsstyle slider sphrq 50\n'
                        'dsstyle info user50\n'
                        'stylerq 50 50 CPK\nsnap explicit50\n'
                        'stylerq 100 100 CPK\nsnap explicit100\n')
                    run(display, cell, os.path.join(out, 'cell'),
                        'windowsize 1200 900\npbcopen\nstyle CPK\nviewall\ncamera orthographic\n'
                        'dsstyle info cellinfo\nsnap automatic\nvizfile export 1920\n', orthographic=True)
                    run(display, BOX, os.path.join(out, 'large'),
                        'style CPK\nviewall\ndsstyle info largeinfo\nsnap cpk\ntimeframes cpk 80\n')
                run(display, BOX, os.path.join(out, 'large-bs'),
                    'style Ball And Stick\nviewall\nsnap bs\ntimeframes bs 80\n')
            finally:
                subprocess.run([os.path.join(apps.INSTALL, 'bin/ecce-gateway-stop')],
                               env=display.env(), timeout=120)
        return analyze(out, args.baseline, args.bonds_only)
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    raise SystemExit(main())
