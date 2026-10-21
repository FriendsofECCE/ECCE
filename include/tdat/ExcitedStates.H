/**
 * @file
 *
 * The excited states of a TD-DFT / CIS calculation as the UV-vis Spectrum
 * panel shows them (#383).  No wx: the panel gives it the four properties
 * EXCIENERGY, EXCIOSC, EXCISYM and EXCITRANS and gets back a VibSpectrum
 * set up for an energy or wavelength axis, a transition list per state and
 * the CSV text.
 */

#ifndef EXCITEDSTATES_HH
#define EXCITEDSTATES_HH

#include <string>
#include <vector>

#include "tdat/VibSpectrum.H"

/** One orbital pair of a state, e.g. from "8a" to "9a", with its coefficient. */
struct ExcitedTransition
{
    std::string from, to;
    double      coefficient;
};

struct ExcitedState
{
    double      ev;             // excitation energy
    double      f;              // oscillator strength
    std::string label;          // "Singlet-A2", "Triplet", "" if unknown
    std::vector<ExcitedTransition> transitions;
};

class ExcitedStates
{
  public:
    /** "8>9:0.70503,7>10:-0.18944" (EXCITRANS); "-" or "" is none. */
    static std::vector<ExcitedTransition> parseTransitions(const std::string& text);

    /** Wavelength in nm of an energy in eV, 0 for a non-positive energy. */
    static double nm(double ev);

    /** Replaces the states.  The vectors may be shorter than `ev`. */
    void set(const std::vector<double>& ev, const std::vector<double>& f,
             const std::vector<std::string>& labels,
             const std::vector<std::string>& transitions);

    int count() const { return (int)p_states.size(); }
    const ExcitedState& state(int i) const { return p_states[i]; }

    /** The transitions of state i for display, largest coefficient first:
        "8 -> 9   0.705". */
    std::vector<std::string> transitionLines(int i) const;

    /** The spectrum model drawn against `unit` (X_EV or X_NM). */
    VibSpectrum spectrum(SpectrumXUnit unit) const;

  private:
    std::vector<ExcitedState> p_states;
};

/**
 * The spectrum as CSV: '#' header lines, one row per state (energy in eV,
 * wavelength in nm, oscillator strength, label, transitions as
 * from>to:coefficient joined by ';'), a blank line, then the broadened curve
 * on a grid of `step` in the axis unit from lo to hi (none when the width is
 * 0).  The curve is f per eV, evaluated in energy whatever the axis.
 * Numbers use the C locale.
 */
std::string excitedSpectrumCsv(const ExcitedStates& states,
                               const VibSpectrum& spectrum,
                               const SpectrumCsvInfo& info,
                               double lo, double hi, double step);

#endif
