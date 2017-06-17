Title "##annotation##"

Start  ##title##

echo

Memory ##MemorySize## mw

##Charge##

##chemsys##

ecce_print ##parseFile##

##basis##

dft
  mult ##Multiplicity##
  ##DFTDirect##
  vectors ##InitialGuess##
  XC ##XCFunctionals##
  XC ##ExchangeAndCorrelation##
  grid ##GridDensity##
  grid ##GridAngularDensity##
  grid ##GridPartition##
  grid ##GridRadial##
  ##DFTNoDIIS##
  convergence energy ##SCF.ConvergenceEnergy.Value##
  convergence density ##SCF.ConvergenceDensity.Value##
  convergence gradient ##SCF.ConvergenceGradient.Value##
  convergence ##DFTAlgorithm##
  iterations ##SCF.ConvergenceIterations##
  convergence levloff ##NewLevelShiftCrossover##
  convergence lshift ##LevelShiftSize##
  convergence damp ##DampingValue##
  ##DFTCoulombScreenExp##
  mulliken
end

nwpw
  ##NWPWSimulationCell##
  ##NWPW.InitialGuess##
  ##SCF.ConvergenceAlgorithm##
  mult ##Multiplicity##
  ##NWPW.SpinType##
  vectors ##InitialGuess##
  xc ##NWPW.XCFunctionals##
  cutoff ##NWPW.Cutoff##
  ewald_rcut ##NWPW.EwaldRcut##
  ewald_ncut ##NWPW.EwaldNcut##
  ##NWPWDimensions2##
  mapping ##NWPW.mapping##
  ##NWPWTolerances2##
  mulliken ##NWPW.Mulliken##
  ##NWPW_CPMD##
end

scf
  ##SCFTheory##
  ##SCFDirect##
  nopen ##Nopen##
  vectors ##InitialGuess##
  thresh ##SCF.ConvergenceGradient.Value##
  maxiter ##SCF.ConvergenceIterations##
  ##SCFLevel##
  ##NRSwitch##
  tol2e ##TwoElectronCutoffExponent##    # default is 1.e-7  LARGE!!
end

mp2
  ##UseFrozenCoreValue##
  ##MP.UseTightConvergence##
end

ccsd
  ##CCThresh##
  maxiter ##CC.ConvergenceIterations##
  ##UseFrozenCoreValue##
end

##Property##

##ESPfit##

##GeomConstraints##

##Optimizer##

##Frequency##

##Cosmo##

##Tasks##
