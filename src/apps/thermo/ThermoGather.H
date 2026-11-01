#ifndef THERMOGATHER_HH
#define THERMOGATHER_HH

#include <string>
#include <vector>

#include "tdat/thermo/ThermoChem.H"

/**
 * Fills the physical quantities computeThermo() needs from a stored
 * calculation (its TE, VIBFREQ, PNTGRP, multiplicity and fragment geometry).
 * Returns false and sets `error` when the calculation cannot be read.
 */
bool gatherThermoInput(const std::string& url, ThermoInput& in,
                       std::string& name, std::vector<std::string>& notes,
                       std::string& error);

#endif
