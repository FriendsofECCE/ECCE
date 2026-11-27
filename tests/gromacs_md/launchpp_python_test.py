#!/usr/bin/env python3
"""Compare the Python launch slice with frozen Perl scripts and live Perl tables."""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
PARSERS = REPO / 'scripts/parsers'
sys.path.insert(0, str(PARSERS))
import gmx_ions
import grompp_warn


def perl_json(code, data=None):
    r = subprocess.run(['perl', '-I' + str(PARSERS), '-MGmxIons', '-MGromppWarn', '-MJSON::PP',
                        '-e', code], input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert r.returncode == 0, r.stderr
    return json.loads(r.stdout)


def tables():
    anchor = perl_json('print encode_json([\\@GmxIons::FAMILIES, \\@GromppWarn::TABLE, '
                       '\\%GromppWarn::MDPKEY, $GromppWarn::CHARGE_LIMIT, '
                       '[map { $_->{re} } @GromppWarn::EXPLAIN]])')
    assert anchor == [gmx_ions.FAMILIES, grompp_warn.TABLE, grompp_warn.MDPKEY,
                      grompp_warn.CHARGE_LIMIT, grompp_warn.EXPLAIN]
    # Exercise every explanation's text, including the charge and tau branches.
    texts = [e['re'] for e in grompp_warn.TABLE] + [
        'constraints on all bonds', 'plain Coulomb cut-off',
        'center of mass motion in the presence of position restraints',
        'tau-t (0.01) should be at least 20 times larger than nsttcouple*dt (0.04)',
        'tau-p (0.5) should be at least 10 times larger than nstpcouple*dt (0.2)',
        'should be at least 10 times larger than nsttcouple', 'unknown warning']
    warnings = [dict(text=t, charge=q) for t in texts for q in (None, '-1.5', '0', '0.00001', '2.75')]
    anchor = perl_json('local $/; my $w = decode_json(<STDIN>); '
                       'print encode_json([map { GromppWarn::explain($_) } @$w]);',
                       json.dumps(warnings).encode())
    # The Perl module returns UTF-8 bytes in an otherwise byte-oriented scalar.
    anchor = [s.encode('latin-1').decode('utf-8') if s is not None else None for s in anchor]
    assert anchor == [grompp_warn.explain(w) for w in warnings]
    messages = [k + ' ' + k.replace('-', '_') + ' (' for k in grompp_warn.MDPKEY]
    anchor = perl_json('local $/; my $a = decode_json(<STDIN>); '
                       'print encode_json([map { GromppWarn::translate($_) } @$a]);',
                       json.dumps(messages).encode())
    assert anchor == [grompp_warn.translate(m) for m in messages]
    # Each harmless rule must actually match its own representative message.
    lines = ['non-matching atom names', 'Berendsen thermostat does not generate',
             'GROMOS force fields have been parametrized with a physically incorrect',
             'Ewald electrostatics in a system with net charge', 'unknown warning']
    for q in ('-2', '0.000001', '0'):
        for intended in (0, 1):
            out = 'System has non-zero total charge: ' + q + '\n\n' + ''.join(
                'WARNING %d [file run.mdp]:\n  %s\n\n' % (i + 1, t) for i, t in enumerate(lines))
            anchor = perl_json('local $/; print encode_json([GromppWarn::parse(<STDIN>, %d)]);'
                               % intended, out.encode())
            found = grompp_warn.parse(out, intended)
            # JSON::PP exposes Perl's incidental numeric coercion after abs().
            for warning in anchor + found:
                if warning['charge'] is not None:
                    warning['charge'] = float(warning['charge'])
            assert anchor == found
    for family in gmx_ions.FAMILIES:
        ff = family['re'][1:] + '.ff'
        for el, ion in family['ions'].items():
            for res in ('UNK', '', ion, 'XYZ'):
                text = ('HETATM%5d %-4s %3s %s%4d    %8.3f%8.3f%8.3f  1.00  0.00          %2s\n'
                        % (1, el + '1', res, ' ', 1, 0., 0., 0., el))
                anchor = perl_json('local $/; my $a = decode_json(<STDIN>); '
                                   r'my ($t, @n) = GmxIons::rewrite(@$a); print encode_json([$t, \@n]);',
                                   json.dumps([text, ff]).encode())
                actual = gmx_ions.rewrite(text, ff)
                assert anchor == list(actual)
    print('PASS: all ion families, warning rows, patterns, reasons, keys and explanation messages equal Perl')


PERL_RUN = ('use File::Temp (); no warnings "redefine"; '
            '*File::Temp::tempdir = sub { my $d = "$ENV{TMPDIR}/fixed"; mkdir $d; return $d; }; '
            'my $s = shift @ARGV; $0 = $s; do $s; die $@ if $@;')
PYTHON_RUN = '''import os, runpy, shutil, sys, tempfile
class Fixed:
    def __init__(self, **kw): self.name = os.path.join(os.environ['TMPDIR'], 'fixed')
    def __enter__(self): os.mkdir(self.name); return self.name
    def __exit__(self, *args): shutil.rmtree(self.name)
tempfile.TemporaryDirectory = Fixed
sys.argv = sys.argv[1:]
sys.path.insert(0, os.path.dirname(sys.argv[0]))
runpy.run_path(sys.argv[0], run_name='__main__')
'''


class Comparison:
    def __init__(self, root, evidence):
        self.root, self.evidence, self.count = root, evidence, 0

    def compare(self, name, script, files, args, path=None, expected=None):
        work = self.root / 'stage'
        snapshots = []
        for language in ('perl', 'python'):
            work.mkdir()
            tmp = work / 'tmp'
            tmp.mkdir()
            for f, content in files.items():
                target = work / f
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(content if isinstance(content, bytes) else content.encode())
            env = dict(os.environ, TMPDIR=str(tmp), PERL5LIB=str(PARSERS),
                       GMX_NO_QUOTES='1', GMX_MAXBACKUP='-1', OMP_NUM_THREADS='1')
            if path is not None:
                env['PATH'] = path
            # Identical scratch paths let raw failure diagnostics be compared without filtering.
            command = ([shutil.which('perl'), '-e', PERL_RUN, str(HERE / 'perl' / script)] if language == 'perl' else
                       [sys.executable, '-c', PYTHON_RUN, str(PARSERS / script)]) + args
            result = subprocess.run(command, cwd=str(work), env=env,
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90)
            scratch = tmp / 'fixed'
            if scratch.exists():
                shutil.rmtree(str(scratch))
            snapshot = {str(p.relative_to(work)): p.read_bytes()
                        for p in work.rglob('*') if p.is_file()}
            snapshot.update(stdout=result.stdout, stderr=result.stderr,
                            exitcode=(str(result.returncode) + '\n').encode())
            snapshots.append(snapshot)
            shutil.rmtree(str(work))
        assert snapshots[0] == snapshots[1], '{} differs: {}'.format(name, [
            (k, snapshots[0].get(k), snapshots[1].get(k)) for k in set(snapshots[0]) | set(snapshots[1]) if snapshots[0].get(k) != snapshots[1].get(k)])
        if expected is not None:
            assert int(snapshots[1]['exitcode']) == expected, (name, snapshots[1])
        if self.evidence:
            for lang, snapshot in zip(('perl', 'python'), snapshots):
                directory = self.evidence / name / lang
                directory.mkdir(parents=True, exist_ok=True)
                for f, content in snapshot.items():
                    target = directory / f
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(content)
            for f in sorted(snapshots[0]):
                paths = [str(self.evidence / name / lang / f) for lang in ('perl', 'python')]
                for command in (['cmp'] + paths, ['sha256sum'] + paths):
                    print('$ ' + ' '.join(shlex.quote(p) for p in command), flush=True)
                    subprocess.run(command, check=True)
        self.count += 1
        print('PASS: {} stdout, stderr, exit code and all staged files byte-identical'.format(name), flush=True)


def mkndx(c):
    gro = (HERE / 'fixtures/conf.gro').read_bytes()
    top = (HERE / 'fixtures/topol.top').read_bytes()
    base = {'conf.gro': gro, 'topol.top': top}
    for name, mdp, change, args, code in [
        ('none', '; no groups\n', {}, [], 0),
        ('residue-union', '; ecce-group Water = SOL SOL\n', {}, [], 0),
        ('molecule', '; ecce-group Water = mol:SOL\n', {}, ['topol.top'], 0),
        ('mixed-pull', '; ecce-group Water = SOL mol:SOL\npull-group1-name = Water\n'
         'pull-group2-name = Water\npull-group2-pbcatom = 7\n', {}, ['topol.top'], 0),
        ('missing-residue', '; ecce-group Bad = XYZ\n', {}, [], 25),
        ('no-topology', '; ecce-group Bad = mol:SOL\n', {}, [], 25),
        ('missing-topology-file', '; ecce-group Bad = mol:SOL\n', {'topol.top': None}, ['topol.top'], 2),
        ('missing-molecule', '; ecce-group Bad = mol:ZZZ\n', {}, ['topol.top'], 255),
        ('atom-count', '; ecce-group Bad = SOL\n', {'conf.gro': 'title\n2\n'}, [], 255),
        ('empty-atoms', '; ecce-group Bad = mol:SOL\n', {'topol.top': '[ molecules ]\nSOL 466\n'}, ['topol.top'], 255),
        ('top-count', '; ecce-group Bad = mol:SOL\n', {'topol.top': top.replace(b'SOL   466', b'SOL   1')}, ['topol.top'], 255),
        ('missing-gro', '; ecce-group Bad = SOL\n', {'conf.gro': None}, [], 2),
        ('stray-byte', b'; \xff\xfe junk\n; ecce-group Water = SOL\npull-group1-name = Water\n', {}, [], 0),
    ]:
        files = dict(base, **{'run.mdp': mdp})
        files.update(change)
        files = {k: v for k, v in files.items() if v is not None}
        c.compare('mkndx-' + name, 'gromacs.mkndx', files,
                  ['conf.gro', 'run.mdp', 'out.ndx'] + args, expected=code)
    c.compare('mkndx-usage', 'gromacs.mkndx', {}, [], expected=255)
    c.compare('mkndx-missing-mdp', 'gromacs.mkndx', {}, ['conf.gro', 'run.mdp', 'out.ndx'], expected=2)
    c.compare('mkndx-output-directory', 'gromacs.mkndx', dict(base, **{
        'run.mdp': '; ecce-group Water = SOL\n'}),
        ['conf.gro', 'run.mdp', 'missing/out.ndx'], expected=2)
    # A local include, flattened include and both preprocessor branches.
    for define in ('', 'define = -DHEAVY_H\n'):
        files = {'conf.gro': gro, 'run.mdp': define + '; ecce-group Water = mol:SOL\n',
                 'topol.top': '#include "local/water.itp"\n[ molecules ]\nSOL 466\n',
                 'local__SLASH__water.itp': '[ moleculetype ]\nSOL 2\n[ atoms ]\n'
                 '#ifndef HEAVY_H\n1\n2\n3\n#else\n1\n2\n3\n#endif\n'}
        c.compare('mkndx-conditional-' + ('on' if define else 'off'), 'gromacs.mkndx', files,
                  ['conf.gro', 'run.mdp', 'out.ndx', 'topol.top'], expected=0)


def launches(c):
    fixtures = HERE / 'fixtures'
    base = {'conf.gro': (fixtures / 'conf.gro').read_bytes(),
            'topol.top': (fixtures / 'topol.top').read_bytes(),
            'gromacs.mdp': 'integrator = steep\nnsteps = 0\ncoulombtype = pme\n',
            'postParams': 'inputFile: gromacs.mdp\n'}
    c.compare('launch-version', 'gromacs.launchpp', {}, ['-v'], expected=0)
    c.compare('launch-no-param', 'gromacs.launchpp', {}, ['-p', 'postParams'], expected=2)
    c.compare('launch-missing-inputs', 'gromacs.launchpp', {'postParams': ''}, ['-p', 'postParams'], expected=1)
    c.compare('launch-tpr', 'gromacs.launchpp', {'postParams': '', 'run.gmxtpr': b'tpr'}, ['-p', 'postParams'], expected=0)
    c.compare('launch-no-gmx', 'gromacs.launchpp', base, ['-p', 'postParams'], path='', expected=0)
    c.compare('launch-groups-no-gmx', 'gromacs.launchpp', dict(base, **{
        'gromacs.mdp': '; ecce-group Water = SOL\n'}), ['-p', 'postParams'], path='', expected=0)
    c.compare('launch-user-index', 'gromacs.launchpp', dict(base, **{
        'user.ndx': '[ System ]\n1\n', 'ecce_groups.ndx': 'stale'}),
        ['-p', 'postParams'], path='', expected=0)
    c.compare('launch-invalid-index', 'gromacs.launchpp', dict(base, **{
        'gromacs.mdp': '; ecce-group Bad = X\n'}), ['-p', 'postParams'], expected=1)
    if not shutil.which('gmx'):
        print('SKIP: real GROMACS comparisons (gmx unavailable)')
        return
    c.compare('launch-water', 'gromacs.launchpp', base, ['-p', 'postParams'], expected=0)
    c.compare('launch-stray-byte', 'gromacs.launchpp', dict(base, **{
        'postParams': b'inputFile: gromacs.mdp\n\xff junk\n',
        'topol.top': base['topol.top'] + b'; \xff\xfe junk\n',
        'conf.gro': b'\xff\xfe' + base['conf.gro'],
        'gromacs.mdp': b'; \xff\xfe junk\n; ecce-group Water = SOL\npull-group1-name = Water\n'
                       b'integrator = steep\nnsteps = 0\ncoulombtype = pme\n'}), ['-p', 'postParams'], expected=0)
    for suffix, files in [
        ('error', dict(base, **{'gromacs.mdp': 'integrator = invalid\n'})),
        ('minff', {'postParams': 'inputFile: prepare.gmxprep\n',
                   'prepare.gmxprep': 'SOURCE=minff\n',
                   'conf.gmxmin': base['conf.gro'], 'topol.gmxmin': base['topol.top']}),
        ('missing-minff', {'postParams': 'inputFile: prepare.gmxprep\n',
                           'prepare.gmxprep': 'SOURCE=minff\n'}),
    ]:
        c.compare('launch-' + suffix, 'gromacs.launchpp', files, ['-p', 'postParams'],
                  expected=1 if suffix == 'missing-minff' else 0)
    # The real warning fixtures use the same stored water calculation.
    sys.path.insert(0, str(HERE))
    import grompp_warn_test as warnings
    for name, charged, marked in [('names', False, False), ('charge', True, False), ('intended', True, True)]:
        d = Path(warnings.system(str(c.root), name, charged, marked))
        files = {p.name: p.read_bytes() for p in d.iterdir()}
        c.compare('launch-warning-' + name, 'gromacs.launchpp', files, ['-p', 'postParams'], expected=0)
    # Client example: stored Trp-cage Prepare topology with Optimize control/coordinates.
    example = REPO / 'data/client/examples/computational/gromacs/gromacs-trpcage'
    files = {'postParams': 'inputFile: gromacs.mdp\n',
             'gromacs.mdp': (example / '2-optimize/optimize.mdp').read_bytes(),
             'conf.gro': (example / '1-prepare/ecceSession_ecceMdPrepare.gro').read_bytes(),
             'topol.top': (example / '1-prepare/ecceSession.gmxtop').read_bytes()}
    files.update({p.name: p.read_bytes() for p in (example / '1-prepare').glob('*.itp')})
    c.compare('launch-client-trpcage', 'gromacs.launchpp', files, ['-p', 'postParams'], expected=0)
    for ff in ('amber99sb-ildn', 'oplsaa', 'charmm27', 'gromos54a7', 'gromos53a6'):
        if not Path('/usr/share/gromacs/top', ff + '.ff').is_dir():
            print('SKIP: force field ' + ff)
            continue
        files = {'postParams': 'inputFile: prepare.gmxprep\n',
                 'prepare.gmxprep': 'FF=' + ff + '\nWATER=tip3p\n',
                 'ion.gmxpdb': b'REMARK \xff\xfe junk\n' + (fixtures / 'na_unk.pdb').read_bytes()}
        c.compare('launch-ion-' + ff, 'gromacs.launchpp', files, ['-p', 'postParams'], expected=0)
    for name in ('1l2y.pdb', '1l2y_broken.pdb'):
        c.compare('launch-prepare-' + name, 'gromacs.launchpp', {
            'postParams': 'inputFile: prepare.gmxprep\n', 'prepare.gmxprep': 'FF=amber99sb-ildn\n',
            'protein.gmxpdb': (fixtures / name).read_bytes()}, ['-p', 'postParams'], expected=0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-dir', type=Path)
    args = parser.parse_args()
    tables()
    with tempfile.TemporaryDirectory(prefix='.gmxlaunch-test-', dir=str(REPO)) as tmp:
        comparison = Comparison(Path(tmp), args.evidence_dir)
        mkndx(comparison)
        launches(comparison)
        print('PASS: %d Perl/Python script comparisons' % comparison.count)
    return 0


if __name__ == '__main__':
    sys.exit(main())
