# ECCE
As PNNL/EMSL have stopped supporting ECCE (see description below) we have forked the source code and put it on github.

Being new to Git we're currently trying to work according to this model: http://nvie.com/posts/a-successful-git-branching-model/

There are thus three (3) branches that are worth looking at. 
* master -- only bug fixes have been added. No new functionality relative to ECCE v7.0 from PNNL
* stable -- bug fixes + 'safe' functionality additions (ones that work and are objectively useful)
* develop -- anything goes. Contains added functionality as well as changes to default behaviour. Not everyone may appreciate this.

It's nice to name releases. Given the meaning of the latin word ECCE, we are currently naming release according to colours. (https://en.wikipedia.org/wiki/List_of_colors:_A%E2%80%93F)


Description from "http://ecce.emsl.pnl.gov/"

The Extensible Computational Chemistry Environment (ECCE, pronounced "etch-ā") provides a sophisticated graphical user interface, scientific visualization tools, and the underlying data management framework enabling scientists to efficiently set up calculations and store, retrieve, and analyze the rapidly growing volumes of data produced by computational chemistry studies.

General Features

* Support for building molecular models.
*  Graphical user interface to a broad range of electronic structure theory types. Supported codes currently include NWChem, GAMESS-UK, Gaussian 03™, Gaussian 98™, and Amica. Other codes are registered based on user requirements.
*  Graphical user interface for basis set selection.
*  Remote submission of calculations to UNIX and Linux workstations, Linux clusters, and supercomputers. Supported queue management systems include PBS™, LSF™, NQE/NQS™, LoadLeveler™ and Maui Scheduler.
*  Three-dimensional visualization and graphical display of molecular data properties while jobs are running and after completion. Molecular orbitals and vibrational frequencies are among the properties displayed.
*  Support for importing results from NWChem, Gaussian 94™, Gaussian 98™, and Gaussian 03™ calculations run outside of the ECCE environment.
*  The ECCE application software currently runs on Linux workstations and is written in C++ using the wxWidgets user interface toolkit and OpenGL graphics. A virtual machine distribution of ECCE allows it to run on PC Windows as well as Mac OS X hosts. Ongoing development will extend ECCE to build and run calculations on periodic systems.
