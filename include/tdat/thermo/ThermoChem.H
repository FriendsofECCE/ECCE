#ifndef THERMOCHEM_HH
#define THERMOCHEM_HH

#include <string>
#include <vector>

/**
 * Ideal-gas / solution thermochemistry recomputed from a calculation's
 * stored frequencies, electronic energy, mass, moments of inertia and
 * multiplicity (#692).  No wx and no data layer: the caller supplies the
 * physical quantities, so the same code serves the CLI, the GUI panel and
 * the rate constants (#330).  ORCA's own printed thermochemistry for its
 * Grimme quasi-RRHO (w0 = 100 cm-1) is the oracle: see tests/thermo.
 */
struct ThermoInput
{
   double electronicEnergy = 0.0;        ///< E_el, hartree
   std::vector<double> frequencies;      ///< real vibrations only, cm-1
   double mass = 0.0;                    ///< molecular mass, amu
   double rotConstants[3] = {0, 0, 0};   ///< principal rotational constants, cm-1
   bool linear = false;                  ///< linear: rotConstants[0] is the one
   int symmetryNumber = 1;               ///< rotational symmetry number sigma
   int multiplicity = 1;                 ///< 2S+1 (singlet = 1)
};

enum ThermoTreatment {
   THERMO_RRHO = 0,           ///< plain rigid-rotor / harmonic-oscillator
   THERMO_GRIMME_QRRHO,       ///< Grimme quasi-RRHO entropy, w0 in [50,200]
   THERMO_TRUHLAR_RAISE,      ///< raise-to-100 cm-1 (ZPE, thermal energy and entropy)
   THERMO_HEADGORDON_QRRHO    ///< Head-Gordon quasi-RRHO enthalpy
};

enum ThermoStandardState {
   THERMO_GAS_1ATM = 0,       ///< ideal gas at 1 atm
   THERMO_SOLN_1M             ///< 1 M standard state
};

struct ThermoResult
{
   bool ok = false;
   std::vector<std::string> notes;   ///< imaginary modes skipped, and the like
   double zpe = 0.0;                 ///< hartree
   double thermalVib = 0.0;          ///< hartree, thermal vibrational energy
   double thermalRot = 0.0;          ///< hartree
   double thermalTrans = 0.0;        ///< hartree
   double H = 0.0;                   ///< hartree
   double S = 0.0;                   ///< J/mol/K
   double G = 0.0;                   ///< hartree
   double sElec = 0.0, sTrans = 0.0, sRot = 0.0, sVib = 0.0;  ///< J/mol/K
};

/** H, S, G for one molecule at temperature T (K). */
ThermoResult computeThermo(const ThermoInput& in, double T,
                           ThermoStandardState ss,
                           ThermoTreatment tr, double w0 = 100.0);

/**
 * The rotational symmetry number for a point-group label as stored
 * (PNTGRP), case-insensitive; 1 when the group is not one we know.
 */
int symmetryNumberForPointGroup(const std::string& pointGroup);

/**
 * The real vibrations from a stored VIBFREQ list, which also holds the
 * near-zero translation/rotation modes.  Those are dropped and imaginary
 * modes skipped, each with a note.  The 1 cm-1 cutoff matches ORCA's own
 * "Cut-Off Frequency" and is far below the soft modes this work is about.
 */
std::vector<double> realVibrations(const std::vector<double>& storedFreqs,
                                   std::vector<std::string>& notes);

/**
 * Parse one low-frequency treatment name as the CLI/panel spells it
 * (case-insensitive): "rrho", "truhlar", "head-gordon" (or "headgordon"),
 * "qrrho-grimme:<w0>" or "qrrho:<w0>".  Sets t and w0; returns false and
 * sets err when the name or its w0 is not recognised.
 */
bool parseTreatmentName(const std::string& name, ThermoTreatment& t,
                        double& w0, std::string& err);

const char* treatmentName(ThermoTreatment t);
const char* standardStateName(ThermoStandardState s);

#endif
