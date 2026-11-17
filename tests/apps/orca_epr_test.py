#!/usr/bin/env python3
"""ORCA EPR results appear as labelled tables in the Builder's Properties list."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / 'tests/launch'))
import builder_overlay_test as bo
import harness
import scan_panel_test as sp
import xdisplay
sys.path.insert(0, str(REPO / 'tests/parsers'))
from propinterp_test import buildDriver


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', default=str(REPO / 'build-cmake'))
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ('Xvfb',))
    s = harness.Session(build, 'orcaepr', {}, local=True)
    display = None
    try:
        for exe in bo.GUI:
            harness.link(os.path.join(build, exe), os.path.join(s.home, 'bin', exe))
        wrappers = os.path.join(s.state, 'wrappers')
        shutil.copytree(os.path.join(build, 'wrappers'), wrappers)
        for entry in os.listdir(wrappers):
            os.chmod(os.path.join(wrappers, entry), 0o755)
        if not s.services(True):
            raise AssertionError('test services did not start')
        display = xdisplay.Display().__enter__()
        env = s.env({'DISPLAY': display.name,
                     'PATH': wrappers + os.pathsep + s.env()['PATH']})
        project = Path(s.localData()) / 'users/local/epr'
        project.mkdir(parents=True)
        shutil.copy(bo.PROJECT_META, project / '.ecce-meta')
        calc = project / 'nh2'
        shutil.copytree(sp.BASE, calc)
        meta = calc / '.ecce-meta'
        meta.write_text(meta.read_text().replace('Gaussian-16', 'ORCA')
                        .replace('GeoVib', 'Energy'))
        fragment = calc / 'Parameters/chemsys.mvm'
        fragment.write_text('title: nh2\ntype: molecule\nnum_atoms: 3\n'
                            'atom_info: symbol cart\natom_list:\n'
                            'N 0 0 0\nH 0.8 0 0.6\nH -0.8 0 0.6\n'
                            'attr_list:\n0 1 0 0 0\n0 1 0 0 0\n0 1 0 0 0\n'
                            'atom_type_list:\n7 1\n1 1\n1 1\n'
                            'num_bonds: 2\nbond_list:\n1 2 1.0 1\n1 3 1.0 1\n')
        props = calc / 'Props'
        for path in props.iterdir():
            path.unlink()
        driver = str(Path(s.state) / 'propinterp')
        assert buildDriver(driver), 'build the application libraries first'
        stream = subprocess.run(['perl', str(REPO / 'scripts/parsers/orca.epr')],
                    input=(REPO / 'tests/parsers/fixtures/orca/nh2_epr.out').read_text(),
                    text=True, capture_output=True, check=True).stdout
        subprocess.run([driver, str(props)], input=stream, text=True,
                       capture_output=True, check=True,
                       env=dict(env, ECCE_HOME=str(REPO)))
        names = ['EPR g-tensor', 'EPR Hyperfine Couplings', 'EPR Quadrupole Couplings']
        lines = ['wait 6000', 'background 1 1 1', 'plist']
        for name in names:
            lines += ['list ' + name, 'wait 1000', 'tablecopy ' + name]
        lines += ['quit']
        log, _ = bo.runScript(s, display, env, wrappers,
                    ['-context', 'file://' + str(calc) + '/'], lines,
                    s.state, 'epr-tables', {'ECCE_PANEL_MODE': 'detail'})
        for name in names:
            assert 'Magnetic resonance/' + name in log, log[-6000:]
        for bad in ['no table in panel', 'unknown command', 'segmentation',
                    'not in the properties list']:
            assert bad not in log.lower(), log[-6000:]
        copies = [line for line in log.splitlines() if 'BUILDER: tablecopy ' in line]
        for label in ['0-N', '1-H', '2-H']:
            assert any(label in line for line in copies), log[-6000:]
        values = [float(value) for line in copies for value in line.split()
                  if re.fullmatch(r'[+-]?\d+\.\d+', value)]
        for value in [2.0041288, 122.3102, -102.3098, -4.099864]:
            assert any(abs(value - shown) < 1e-6 for shown in values), log[-6000:]
        print('PASS: three EPR Properties tables, nucleus labels and values')
        return 0
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()


if __name__ == '__main__':
    sys.exit(main())
