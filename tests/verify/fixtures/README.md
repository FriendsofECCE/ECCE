Every deck here except one was **captured from a real run**, not written
for the suite -- which is the point.  A checker validated against decks
invented by the same person who wrote the checker will agree with itself
and tell you nothing.  These are files ECCE actually generated, and the
working ones are files the codes actually accepted.

  g16-water.g16in         Gaussian 16, basis by name (Gen block removed)
  g16-gen-basis.g16in     the same calculation with its real Gen basis
  g16-ozone-sci.g16in     coordinates in exponent form (-2.26132e-16);
                          an earlier checker called this "no geometry"
  orca-ethane.orcain      ORCA, ethane, ! route and * xyz block
  nwchem-ammonia.nw       NWChem, geometry and task directives
  mopac-methane.mopin     MOPAC, keyword line then two title lines

  g16-garbled-basis.g16in THE REAL FAILURE of 2026-09-25: the Gen basis
                          block emerged as six bytes of binary (#146).
                          Gaussian read them as an atomic symbol and
                          took a segmentation violation -- after the job
                          had reached a queue.
  g16-gen-no-basis.g16in  /GEN promised, nothing following it

  g16-no-geometry.g16in   no coordinates and nothing to excuse it
  g16-geom-check.g16in    no coordinates because Geom=Check says so,
                          which must NOT read as a fault
  g16-no-route.g16in      no route card
  nwchem-ammonia-no-task.nw  NWChem reads it and does nothing
  orca-no-route.orcain    no ! line

The exception is `qe-silicon.qein`, which is hand-written: this tree has
no captured Quantum ESPRESSO deck, because QE support is experimental
and has never been run in anger.  Treat it as an assertion about the
namelist shape only -- it is worth exactly as much as the documentation
it was written from, which is the reason the checker judges nothing
about QE beyond whether the three namelists are present.
