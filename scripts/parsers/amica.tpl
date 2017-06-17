&INT
  GROUP ##Symmetry##
  LIBRARY ./
  ##chemsys##

&SCF
  OCC ##ShellOccupation##
  A ##CouplingA##
  B ##CouplingB##
  ##OpenShellOcc##
  CONV ##EnergyConvergence##

&PSI
  ##ELEC##
  SPIN ##ChemSys.Multiplicity##
  STATE ##State##
  SYM ##Irrep##
  ##PsiInteract##
  ##Asym##
  FRZ ##FrozenCore##
  DBL ##DoublyOcc##
  ACT ##ActiveOrbs##
  OCCMIN ##OccMin##
  OCCMAX ##OccMax##

&MCSCF
  NITER ##Iter##
  SHIFT ##Shift##
  WNORM ##Wnorm##
  FCILEV ##Fcilev##
  FREEZE ##Freeze##

&CI
  ##ACPF##
  ##ACPF2##
  ##AQCC##
  ##CEPA0##
  ##CI##
  ##CPF##
  ##R12ACPF##
  ##R12ACPF2##
  ##R12AQCC##
  ##R12CEPA0##
  ##R12CI##
  R12ORB ##R12Orbs##
  ELEC   ##NELEC##
  R12SHELL ##R12shell##
  NITER ##Iterations##
  CONV ##EnergyConvergence##
  ##NOHESS##
  ##DENSITY##
  ##PROPER##
  NVCIMX ##NVCIMX##

&CC
  ##R12_CC## 
  ##Direct##
  ##CC##
  FRZC ##FROZEN##
  OCC  ##OCCUPIED##
  FRZV ##FRZVIRT##
  CONV_SCF ##HF_Convergence##
  CONV_CC  ##CC_Convergence##
  ##DETAILED##

&MISC
  ##DIATOM##
  ##BOND##
  LOOP ##DIST##
  ##ANGSTROM## 
  ##NORUN##
  MEMORY ##MEMORY##
  ##CARTESIAN##
