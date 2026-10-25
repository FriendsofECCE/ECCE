#ifndef MDANALYSIS__H
#define MDANALYSIS__H

#include <string>
#include <vector>

#include "MdTopology.H"

/**
 * Analyses of a GROMACS trajectory for teaching: the distribution of atomic
 * speeds and the mean-square displacement with its diffusion coefficient.
 * No wx; the Builder's panels only draw what these return.
 */

/// Speeds (m/s) of one kind of atom, averaged over the frames.
struct SpeedCurve
{
   std::string label;          ///< element, or the atom type if no element has the mass
   double mass = 0;            ///< u
   double refT = 0;            ///< K, the run's reference temperature
   double vmax = 0;            ///< m/s; the histogram spans 0..vmax
   std::vector<double> density;///< per m/s, integrates to 1 over the bins
   double mean = 0, rms = 0;   ///< measured, m/s
   double samples = 0;
};

/// MSD of the centres of mass of one molecule type.
struct MsdCurve
{
   std::string label;          ///< molecule type name
   int molecules = 0;
   std::vector<double> time;   ///< ps (lag times)
   std::vector<double> msd;    ///< nm^2
};

struct DiffusionFit
{
   bool ok = false;
   double slope = 0;           ///< nm^2/ps
   double intercept = 0;       ///< nm^2
   double D = 0;               ///< nm^2/ps, slope/6
   double from = 0, to = 0;    ///< ps, the points actually used
   int points = 0;
};

/** Maxwell-Boltzmann speed density f(v) in 1/(m/s), v in m/s, mass in u. */
double mbDensity(double v, double massU, double T);
double mbMostProbable(double massU, double T);  ///< m/s
double mbMean(double massU, double T);
double mbRms(double massU, double T);

/** Least-squares line through the points with from <= t <= to (ps); the
    Einstein relation gives D = slope/6.  from >= to means 25% to 100% of the span. */
DiffusionFit fitDiffusion(const std::vector<double>& time,
                          const std::vector<double>& msd, double from, double to);

/** D in 10^-5 cm^2/s from nm^2/ps. */
inline double toE5cm2s(double nm2ps) { return nm2ps * 1000.0; }

/** Reference-temperature speed histograms per distinct atom mass; the
    frames need velocities.  nbins bins span 0 to 4.5 times the most
    probable speed of the lightest atom. */
bool speedDistribution(const std::string& trr, const MdTopology& top,
                       double refT, int nbins, std::vector<SpeedCurve>& out,
                       std::string& error);

/** MSD of molecule centres of mass, unwrapped through the periodic box,
    averaged over time origins; at most maxLags lag times, spaced evenly in
    log time.  Molecules are made whole around their first atom. */
bool displacement(const std::string& traj, const MdTopology& top,
                  int maxLags, std::vector<MsdCurve>& out, std::string& error);

/// The records of the parse file (one string without blanks per record),
/// key MDSPEED and MDMSD.
std::vector<std::string> encodeSpeed(const std::vector<SpeedCurve>& curves);
std::vector<std::string> encodeMsd(const std::vector<MsdCurve>& curves);
bool decodeSpeed(const std::vector<std::string>& records,
                 std::vector<SpeedCurve>& out);
bool decodeMsd(const std::vector<std::string>& records,
               std::vector<MsdCurve>& out);

#endif
