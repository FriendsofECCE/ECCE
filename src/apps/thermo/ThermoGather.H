#ifndef THERMOGATHER_HH
#define THERMOGATHER_HH

#include <string>
#include <vector>

#include "tdat/thermo/ThermoChem.H"

/**
 * Fills the physical quantities computeThermo() needs from a stored
 * calculation (its TE, VIBFREQ, PNTGRP, multiplicity and fragment geometry).
 * symTolerance (angstrom) is the looser point-group search used for the
 * near-symmetry warning; 0 skips it.
 * Returns false and sets `error` when the calculation cannot be read.
 */
bool gatherThermoInput(const std::string& url, ThermoInput& in,
                       std::string& name, std::vector<std::string>& notes,
                       std::string& error, double symTolerance = 0.05);

/**
 * Code, method, basis set, solvent, functional and dispersion of a
 * calculation as one line, for deciding whether two calculations may be
 * compared (never across methods).  Empty when the calculation cannot be read.
 */
std::string gatherMethodKey(const std::string& url);

#endif
