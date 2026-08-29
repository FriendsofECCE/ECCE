# ECCE

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā") is a graphical user interface, scientific visualization toolkit,
and data management framework for setting up, running, and analyzing
computational chemistry calculations.

As PNNL/EMSL stopped supporting ECCE, we forked the source code and
maintain it here.

**Status: heading toward a first alpha/beta release.** This fork is a
from-scratch modernization of the original codebase — not a rewrite. The
same ~1200-file C++ application, same chemistry logic, ported forward onto
current tools:

* Build system: CMake/CPack, replacing the old `build_ecce` recursive-make
  workflow. Produces a single `.deb` package.
* GUI toolkit: wxWidgets 3.2 (was 2.8), on GTK3.
* Language/runtime: C++17 (was C++14-ish/pre-C++11-in-places), Python 3
  (was Python 2) for the helper GUI scripts.
* Messaging: the JMS gateway now runs on the Debian-packaged ActiveMQ
  broker instead of a bundled 2008-era jar.
* Packaging: no bundled third-party sources build from scratch anymore —
  wxWidgets, Xerces-C, Mesa/OpenGL, Apache, and ActiveMQ all come from
  Debian's own repositories.
* Target platform: Debian 13 ("trixie"). Other distributions may work but
  aren't currently tested.

See [`GETTING_STARTED.md`](GETTING_STARTED.md) for build, install, and
first-login instructions. `CLAUDE.md` and `ECCE_modernization_status.md`
carry the detailed, dated history of the modernization work itself (root
causes, fixes, what's still open) if you want the full story behind any
particular change.

## Branches and releases

`main` is the only active branch — build and file PRs against it. Older
branches (`develop`, `stable`, `master`, `make`) have been consolidated
into `main` and preserved as `archive/*` for history; there's no reason to
branch from or compare against them going forward. Releases are marked
with tags, not separate release branches.

## General features

* Building molecular models.
* A graphical user interface to a broad range of electronic structure
  theory types. Supported codes include NWChem, GAMESS-UK, Gaussian 03,
  Gaussian 09, Gaussian 16, and Amica; other codes can be registered based
  on user requirements.
* A graphical user interface for basis set selection.
* Remote submission of calculations to Unix/Linux workstations, Linux
  clusters, and supercomputers, via PBS, LSF, NQE/NQS, LoadLeveler, and
  Maui Scheduler queue management.
* Three-dimensional visualization and graphical display of molecular data
  and properties, both while jobs are running and after completion.
  Molecular orbitals and vibrational frequencies are among the properties
  displayed.
* Importing results from NWChem and Gaussian calculations run outside of
  the ECCE environment.

## Contributing

Issues and pull requests are welcome — see the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues). If you hit
a crash or a build failure, a lot of prior investigation may already be
recorded in `CLAUDE.md`; worth a search before filing something new.

## License

See [`LICENSE`](LICENSE).
