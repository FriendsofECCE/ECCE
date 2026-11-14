#!/usr/bin/env python3
"""Basis Set Tool content size, row growth and saved geometry (#1029)."""
import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apps
import fixture
import run_tests
import smallscreen_test as small
import xdisplay


def main():
    png = None
    if '--png' in sys.argv:
        index = sys.argv.index('--png')
        png = os.path.abspath(sys.argv[index + 1])
        del sys.argv[index:index + 2]
        os.makedirs(png, exist_ok=True)
    xdisplay.SCREEN = '1366x768x24'

    def checkApp(display, name, results, verbose=False):
        results.checks += 1
        def check(ok, message):
            print(('PASS ' if ok else 'FAIL ') + message, flush=True)
            if not ok:
                results.fail(name, message)

        with tempfile.TemporaryDirectory(prefix='basis-size-', dir=fixture.stateHome()) as data:
            target = os.path.join(data, 'users', 'local', 'Ar')
            shutil.copytree(os.path.join(HERE, '..', 'builder', 'radialdist-fixtures', 'ar'), target)
            command = os.path.join(data, 'commands')
            def send(text):
                with open(command, 'a') as handle:
                    handle.write(text + '\n')
                time.sleep(3 if text == 'close' else 1)
            env = {'ECCE_LOCAL_DATA': data, 'ECCE_TEST_BASISTOOL': command,
                   'ECCE_TEST_DIALOG_CLOSE': '1', 'ECCE_TEST_DIALOG_ANSWER': 'ok'}
            def inspect(d):
                send('info')
                if png:
                    small.opts.update(out=png, size=(1366, 768))
                    small.shoot(d, 'basis-tool-1366x768')
                send('rows 8')
                send('info')
                send('rows 1')
                send('resize 20 25 1100 720')
                send('info')
                send('close')
            result = apps.run(display, name, args=('-context', 'Ar'), env=env,
                              settle=3, inspect=inspect)
            print(run_tests._tail(result.log, 20))
            records = re.findall(r'BASISTOOL: frame=(\d+),(\d+),(\d+),(\d+) grid=(\d+) rows=(\d+) table=(\d+),(\d+) tablemin=(\d+),(\d+)', result.log)
            check(len(records) == 3 and not result.crashed, 'three geometry samples without a crash')
            if len(records) != 3:
                return
            records = [tuple(map(int, r)) for r in records]
            for x, y, w, h, grid, rows, tw, th, mw, mh in records:
                check(x + w <= 1366 and y + h + small.TITLE_ALLOWANCE <= 768,
                      'frame %dx%d at %d,%d fits 1366x768' % (w, h, x, y))
                check(tw >= mw and th >= mh, 'periodic table keeps its readable minimum')
            check(records[0][5] == 1 and 90 <= records[0][4] <= 130,
                  'Ar selections start compact (%d px)' % records[0][4])
            check(records[1][4] > records[0][4], 'selections grow with rows')
            saved = records[2][:4]
            open(command, 'w').close()
            def reopen(d):
                send('info')
                send('close')
            restored = apps.run(display, name, args=('-context', 'Ar'), env=env,
                                settle=3, inspect=reopen)
            print(run_tests._tail(restored.log, 10))
            match = re.search(r'BASISTOOL: frame=(\d+),(\d+),(\d+),(\d+)', restored.log)
            check(not restored.crashed and match is not None
                  and tuple(map(int, match.groups())) == saved,
                  'closing and reopening restores size and position')
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], '--app', 'basistool'] + sys.argv[1:]
    return run_tests.main()


if __name__ == '__main__':
    sys.exit(main())
