#ifndef PULLANALYSIS__H
#define PULLANALYSIS__H

#include <cstddef>
#include <string>
#include <vector>

/**
 * A GROMACS pull run (pullx.xvg and pullf.xvg joined): time in ps, the pull
 * coordinate (COM distance) in nm and the force in kJ/mol/nm.  No wx.
 */
struct PullSeries
{
   std::vector<double> time, dist, force;
   size_t size() const { return time.size(); }
};

struct PullSummary
{
   size_t points = 0;
   double rate = 0;       ///< least-squares slope of dist against time, nm/ps
   double meanForce = 0;
   double peakForce = 0;  ///< the force of largest magnitude, with its sign
   double peakTime = 0;
   double peakDist = 0;
};

PullSummary pullSummary(const PullSeries& s);

/** CSV: '#' header lines, then time_ps,distance_nm,force_kJ/mol/nm.  C locale. */
std::string pullCsv(const PullSeries& s, const std::string& calculation);

#endif
