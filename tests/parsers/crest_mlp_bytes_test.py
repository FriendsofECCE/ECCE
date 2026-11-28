#!/usr/bin/env python3
"""CREST/MLP blocks and malformed-byte sites against original Perl goldens."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from eccejobmonitor_sim import read_desc, replay

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
SCRIPTS = REPO / 'scripts/parsers'
FIX = HERE / 'fixtures/crest_mlp'
GOLDEN = HERE / 'expected/crest-mlp-bytes.json'
TOKENS = [('nbsp', b'\xc2\xa0'), ('digit', b'\xd9\xa3'), ('word', b'\xc3\x85'),
          ('overflow', b'*****'), ('prefix', b'3x'), ('empty', b''), ('stray', b'\xff')]


def pipe_cases():
    for stem in ('crest', 'mlp_energy', 'mlp_md', 'mlp_vib'):
        family = 'crest' if stem == 'crest' else 'mlp'
        desc = read_desc(str(SCRIPTS / (family + '.desc')))
        for suffix in ('', '_bytes'):
            result = replay(desc, str(FIX / (stem + suffix + '.out')))
            assert result.runaway is None, stem
            for entry in desc.live_entries():
                for i, block in enumerate(result.delivered_for(entry)):
                    # The replayer replaces FF on decode; restore the original
                    # byte so the actual parser, rather than the test, reads it.
                    data = block.text.encode('utf-8').replace(b'\xef\xbf\xbd', b'\xff')
                    name = '{}{}-{}-{}'.format(stem, suffix, entry.script, i)
                    yield dict(name=name, script=entry.script, data=data,
                               args=['.', 'Energy', 'SE', 'MACE', '0'])


def site_cases():
    sites = [
        ('crest.conformers', 'relative', b'energy: -1.0 relative: @\nH 0.0 0.0 0.0\n'),
        ('crest.conformers', 'energy-word', b'energy: @ relative: 1.0\nH 0.0 0.0 0.0\n'),
        ('crest.conformers', 'leading', b'@H 0.0 0.0 0.0\n'),
        ('crest.conformers', 'symbol', b'@ 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'energy', b'energy: @\nH 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'time', b'time: @\nH 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'cell', b'cell: @ 0 0 0 1 0 0 0 1\nH 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'cell-space', b'cell: 1@0 0 0 1 0 0 0 1\nH 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'leading', b'@H 0.0 0.0 0.0\n'),
        ('mlp.geomtrace', 'symbol', b'@ 0.0 0.0 0.0\n'),
        ('mlp.final', 'energy', b'energy: @\n'),
        ('mlp.final', 'forces-marker', b'energy: -1.0\n@forces:\nH 1.0 2.0 3.0\n'),
        ('mlp.final', 'stress-marker', b'energy: -1.0\n@stress\n1.0 2.0 3.0\n4.0 5.0 6.0\n7.0 8.0 9.0\n'),
        ('mlp.final', 'force-leading', b'energy: -1.0\nforces:\n@H 1.0 2.0 3.0\n'),
        ('mlp.final', 'symbol', b'energy: -1.0\nforces:\n@ 1.0 2.0 3.0\n'),
        ('mlp.final', 'stress-leading', b'energy: -1.0\nstress\n@1.0 2.0 3.0\n4.0 5.0 6.0\n7.0 8.0 9.0\n'),
        ('mlp.vib', 'nmodes', b'ECCE-MLP-VIB @ 1\nmode 1 2.0\n1.0 2.0 3.0\n'),
        ('mlp.vib', 'natoms', b'ECCE-MLP-VIB 1 @\nmode 1 2.0\n1.0 2.0 3.0\n'),
        ('mlp.vib', 'mode-index', b'ECCE-MLP-VIB 1 1\nmode @ 2.0\n1.0 2.0 3.0\n'),
        ('mlp.vib', 'frequency', b'ECCE-MLP-VIB 1 1\nmode 1 @\n1.0 2.0 3.0\n'),
        ('mlp.vib', 'leading', b'ECCE-MLP-VIB 1 1\nmode 1 2.0\n@1.0 2.0 3.0\n'),
    ]
    for script in ('crest.conformers', 'mlp.geomtrace', 'mlp.final', 'mlp.vib'):
        for i in range(3):
            row = [b'1.0', b'2.0', b'3.0']
            row[i] = b'@'
            body = b' '.join(row) + b'\n'
            if script == 'mlp.final':
                sites.append((script, 'stress-number-' + str(i),
                              b'energy: -1.0\nstress\n' + body + b'4.0 5.0 6.0\n7.0 8.0 9.0\n'))
                body = b'energy: -1.0\nforces:\nH ' + body
            elif script == 'mlp.vib':
                body = b'ECCE-MLP-VIB 1 1\nmode 1 2.0\n' + body
            else:
                body = b'H ' + body
            sites.append((script, 'number-' + str(i), body))
    for key in ('Model', 'Version', 'Licence'):
        prefix = b'' if key == 'Model' else b'Model: clean\n'
        sites.append(('mlp.version', key, prefix + key.encode('ascii') + b': @\n'))
        sites.append(('mlp.version', key + '-space', prefix + key.encode('ascii') + b':@clean\n'))
    for script, site, template in sites:
        for token, value in TOKENS:
            yield dict(name='site-{}-{}-{}'.format(script, site, token),
                       script=script, data=template.replace(b'@', value), args=[])
    for script in ('crest.launchpp', 'mlp.launchpp'):
        for name, args in [('normal', ['-p', 'postParams']), ('version', ['-v']),
                           ('merged', ['-pfoo', '-v']), ('unknown', ['-z']),
                           ('stop', ['--', '-v']), ('missing', ['-p'])]:
            yield dict(name=script+'-'+name, script=script, data=b'\xff\n', args=args,
                       files={'postParams': b'numProcs: 2\ninputFile: deck\n'})


def import_cases():
    clean = (FIX / 'crest.out').read_bytes()
    deck = (FIX / 'crest.crestin').read_bytes()
    for label, files, args, link in [
        ('fallback', {'job.out': clean}, ['job.out'], False),
        ('deck', {'job.out': clean, 'crest.crestin': deck}, ['job.out'], False),
        ('source', {'job.out': clean, 'source/crest.crestin': deck}, ['job.out', 'source/'], False),
        ('link', {'source/job.out': clean, 'source/crest.crestin': deck}, ['job.out'], True),
        ('invalid', {'job.out': b'\xff\n'}, ['job.out'], False),
        ('no-ensemble', {'job.out': b'Conformer-Rotamer Ensemble Sampling Tool\n'}, ['job.out'], False),
        ('empty-ensemble', {'job.out': b'Conformer-Rotamer Ensemble Sampling Tool\nECCE-CREST-FRAME 1 0\nenergy: 0\nECCE-CREST-ENDFRAME\n'}, ['job.out'], False),
        ('help', {}, ['-h'], False), ('version', {}, ['-v'], False),
        ('arity', {}, [], False), ('missing-file', {}, ['missing.out'], False),
    ]:
        yield dict(name='crest.expt-'+label, script='crest.expt', data=b'',
                   args=args, files=files, link=link)
    intro = b'Conformer-Rotamer Ensemble Sampling Tool\nVersion 3.0.2\n Command line input:\n $ crest '
    frame = b'\nECCE-CREST-FRAME 1 1\nenergy: -1.0 relative: 0.0\nH 0.0 0.0 0.0\nECCE-CREST-ENDFRAME\n'
    for site, template in [
        ('version', b'Version @\n'), ('version-word', b'Version 3@\n'), ('header', b'@Conformer-Rotamer Ensemble Sampling Tool\n'),
        ('command-space', b' Command line input:\n @$ crest --gfnff\n'),
        ('gfnff-boundary', b' Command line input:\n $ crest --gfnff@\n'),
        ('gfn-composite', b' Command line input:\n $ crest --gfn2//@gfnff\n'),
    ]:
        for token, value in TOKENS:
            mutant = template.replace(b'@', value)
            if site in ('version', 'version-word'):
                data = intro.replace(b'Version 3.0.2\n', mutant) + frame
            elif site == 'header':
                data = intro.replace(b'Conformer-Rotamer Ensemble Sampling Tool\n', mutant) + frame
            else:
                data = b'Conformer-Rotamer Ensemble Sampling Tool\n' + mutant + frame
            files = {'job.out': data}
            yield dict(name='crest.expt-'+site+'-'+token, script='crest.expt', data=b'', args=['job.out'], files=files)
    for flag in ('--chrg', '--uhf', '--ewin', '--nsolv', '--alpb', '--gbsa',
                 '--protonate', '--tautomerise', '--tautomerize', '--qcg', '--noreftopo', '--notopo'):
        boundary = flag in ('--protonate', '--tautomerise', '--tautomerize', '--qcg', '--noreftopo', '--notopo')
        for token, value in TOKENS + [('valid', b'2.5' if flag=='--ewin' else b'2')] + ([('water', b'h2o'), ('hexane', b'n-hexane')] if flag == '--gbsa' else []):
            call = (b'--qcg ' if flag == '--nsolv' else b'') + flag.encode('ascii') + (b'' if boundary else b' ') + value
            files = {'job.out': intro + call + frame, 'crest.crestin': deck}
            yield dict(name='crest.expt-'+flag[2:]+'-'+token, script='crest.expt', data=b'', args=['job.out'], files=files)
    for site, template in [
        ('deck-count', b'crest\n@\ncomment\nH 0 0 0\n'),
        ('deck-symbol', b'crest\n1\ncomment\n@ 0 0 0\n'),
        ('deck-leading', b'crest\n1\ncomment\n@H 0 0 0\n'),
        ('solvent-block', b'crest\n@$solvent\n1\nsolvent\nO 0 0 0\n$end\n1\nsolute\nH 0 0 0\n'),
        ('solvent-end', b'crest\n$solvent\n1\nsolvent\nO 0 0 0\n@$end\n1\nsolute\nH 0 0 0\n'),
    ] + [('deck-number-'+str(i), b'crest\n1\ncomment\nH '+b' '.join(b'@' if j==i else b'0' for j in range(3))+b'\n') for i in range(3)]:
        for token, value in TOKENS:
            files = {'job.out': intro + b'--protonate' + frame,
                     'crest.crestin': template.replace(b'@', value)}
            yield dict(name='crest.expt-'+site+'-'+token, script='crest.expt', data=b'', args=['job.out'], files=files)
    for site, template in [('frame-space', b'ECCE-CREST-FRAME@1 1'),
                           ('frame-index', b'ECCE-CREST-FRAME @ 1'),
                           ('frame-count', b'ECCE-CREST-FRAME 1 @'),
                           ('frame-leading', b'@H 0.0 0.0 0.0'),
                           ('frame-symbol', b'@ 0.0 0.0 0.0')]+[
                               ('frame-number-'+str(i), b'H '+b' '.join(b'@' if j==i else b'0.0' for j in range(3))) for i in range(3)]:
        old = b'ECCE-CREST-FRAME 1 1' if site.startswith('frame-') and site in ('frame-space','frame-index','frame-count') else b'H 0.0 0.0 0.0'
        for token, value in TOKENS:
            files = {'job.out': intro + frame.replace(old, template.replace(b'@', value))}
            yield dict(name='crest.expt-'+site+'-'+token, script='crest.expt', data=b'', args=['job.out'], files=files)


def execute(case, scripts, interpreter, artifacts=None):
    with tempfile.TemporaryDirectory(prefix='ecce-crest-mlp-') as work:
        work = Path(work)
        for name, data in case.get('files', {}).items():
            path = work / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        if case.get('link'):
            (work / 'job.out').symlink_to(work / 'source/job.out')
        script = str(scripts / case['script'])
        if case['name'] == 'crest.expt-missing-file':
            # Equal relative script names keep the legacy die diagnostic
            # comparable without normalising any bytes of its stderr.
            (work / 'parsers').symlink_to(scripts, target_is_directory=True)
            script = 'parsers/' + case['script']
        proc = subprocess.run([interpreter, script] + case['args'],
                              input=case['data'], stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, cwd=str(work), timeout=30)
        files = {str(p.relative_to(work)): p.read_bytes() for p in sorted(work.rglob('*')) if p.is_file()}
        result = dict(stdout=proc.stdout.hex(), stderr=proc.stderr.hex(), rc=proc.returncode,
                      files={name: data.hex() for name, data in files.items()})
        if artifacts:
            dest = artifacts / case['name']
            dest.mkdir(parents=True, exist_ok=True)
            for name, data in dict(stdout=proc.stdout, stderr=proc.stderr,
                                   rc=str(proc.returncode).encode('ascii') + b'\n', **{'stdin': case['data']}).items():
                (dest / name).write_bytes(data)
            for name, data in files.items():
                p = dest / 'files' / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_bytes(data)
        return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--capture-perl', type=Path)
    ap.add_argument('--artifacts', type=Path)
    ap.add_argument('--output', type=Path, default=GOLDEN)
    args = ap.parse_args()
    cases = list(pipe_cases()) + list(site_cases()) + list(import_cases())
    expected = {} if args.capture_perl else json.loads(GOLDEN.read_text(encoding='utf-8'))
    actual = {}
    for case in cases:
        if os.name == 'nt' and (case.get('link') or case['name'] == 'crest.expt-missing-file'):
            print('SKIP {}: symbolic links unavailable'.format(case['name']))
            continue
        result = execute(case, args.capture_perl or SCRIPTS,
                         'perl' if args.capture_perl else sys.executable, args.artifacts)
        actual[case['name']] = result
        if not args.capture_perl:
            assert result == expected[case['name']], '{}: stdout/stderr/rc/files differ\nactual={!r}\nexpected={!r}'.format(case['name'], result, expected[case['name']])
        print('PASS {}: stdout/stderr/rc/files'.format(case['name']))
    for stem in ('crest', 'mlp_energy', 'mlp_md', 'mlp_vib'):
        clean = {k[len(stem):]: v for k,v in actual.items() if k.startswith(stem+'-')}
        decoy = {k[len(stem+'_bytes'):]: v for k,v in actual.items() if k.startswith(stem+'_bytes-')}
        assert clean and clean == decoy, stem + ': decoys changed clean values'
        print('PASS {}: clean and byte-decoy records identical'.format(stem))
    if args.capture_perl:
        args.output.write_text(json.dumps(actual, indent=2, sort_keys=True)+'\n', encoding='utf-8')
    print('PASS {} cases'.format(len(actual)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
