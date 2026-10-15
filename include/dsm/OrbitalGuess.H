/**
 * @file
 * Carrying a finished calculation's final orbitals into another one as its
 * initial guess.  Free of the GUI, so the Organizer, the tests and a
 * future second code share one path.
 */
#ifndef ORBITALGUESS_HH
#define ORBITALGUESS_HH

#include <string>

class TaskJob;

namespace OrbitalGuess {

/** True when the code of `source` writes an orbital file that a later
    calculation of the same code can read, and `source` has kept it.
    Otherwise `why` says which of the two is missing. */
bool available(const TaskJob *source, std::string &why);

/** Copy the orbitals of `source` into `target` as its input guess and
    record where they came from (calculation name, basis set name).
    `target` is normally a fresh duplicate of `source`. */
bool attach(TaskJob *target, TaskJob *source, std::string &why);

/** The stored record, "" when `calc` has no guess attached. */
std::string sourceName(const TaskJob *calc);
std::string sourceBasis(const TaskJob *calc);

}

#endif
