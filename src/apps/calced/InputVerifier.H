/**
 * @file
 *
 * Ask scripts/parsers/verifyinput what it thinks of a generated input
 * deck, and give the answer back as something a panel can colour
 * (GitHub #148).
 *
 * WHY THE JUDGEMENT LIVES IN A SCRIPT AND NOT HERE.  What counts as a
 * broken deck changes with every code that gets integrated, and it is
 * learned from decks that failed -- which happens between releases, not
 * at them.  A rule kept in C++ ships when the next package ships; a
 * rule kept in scripts/parsers can be corrected on a running
 * installation by the person who just watched a job die, and is
 * testable without a GUI.  So this class runs the script, parses four
 * fields, and decides nothing.
 *
 * WHAT THE THREE LEVELS MEAN, because they are not severities.
 *
 *   BAD     Something was checked and is definitely wrong.  The deck
 *           will not run, or will not run as the user thinks.
 *   UNSURE  Something was looked at and could not be judged.  NOT a
 *           mild fault: it is the checker declining to guess, and it
 *           is the only honest answer for most of what a deck says.
 *   GOOD    Something was checked and is in order.
 *
 * The absence of BAD is not a statement that the calculation is
 * correct, and nothing in the UI may imply it is.  This checker knows
 * about the SHAPE of a deck.  It does not know whether the theory suits
 * the molecule, whether the basis is adequate, or whether a keyword
 * this version of the code accepts means what the user thinks.  This
 * tree has been wrong about keyword validity from reading manuals
 * repeatedly -- NWChem rejects its own documented dispersion spellings,
 * ORCA takes 6-31++G** but not 6-31++G -- which is exactly why none of
 * that is checked here and why UNSURE exists.
 */

#ifndef INPUTVERIFIER_HH
#define INPUTVERIFIER_HH

#include <string>
#include <vector>
using std::string;
using std::vector;


struct VerifyFinding
{
    enum Level { GOOD, UNSURE, BAD };

    Level  level;
    int    line;      ///< 1-based, or 0 when it is about the whole file
    string check;     ///< short name of the check, e.g. "geometry"
    string message;

    VerifyFinding() : level(GOOD), line(0) { }
};


class InputVerifier
{
  public:

    /**
     * Run the checker over a deck.
     *
     * @param text      the input file exactly as it would be submitted
     * @param codeName  the code, as the .edml names it
     * @param atoms     how many atoms the calculation has, or 0 to
     *                  leave the checker unable to compare -- pass 0
     *                  rather than a guess, since a wrong count here
     *                  produces a confident and false BAD
     * @param findings  filled in, in the order the checker reported
     * @param error     why nothing could be checked, when that happens
     *
     * @return false if the checker could not be run at all, which is
     *         NOT the same as a deck with faults in it and must not be
     *         shown as one.  A missing checker means the user learns
     *         nothing; it does not mean their deck is bad.
     */
    static bool run(const string& text,
                    const string& codeName,
                    int atoms,
                    vector<VerifyFinding>& findings,
                    string& error);

    /** The worst level present; GOOD for an empty list. */
    static VerifyFinding::Level worst(const vector<VerifyFinding>& findings);
};

#endif
