/**
 * @file
 *
 * The Rate box of a workflow (#872): the rate constant of the step from the
 * reactant to the TS (and product), for workflows whose steps carry the keys
 * reactant, ts and product.  The numbers are `ecce-rate --json`'s (RateRun.H);
 * this file chooses the steps and words the result.  No wx.
 */

#ifndef RATEBOX_HH
#define RATEBOX_HH

#include <string>
#include <utility>
#include <vector>

#include "KeyData.H"
#include "util/RateRun.H"

struct RateBoxResult {
  bool applicable = false;   ///< the workflow has a reactant and a TS step
  bool ready = false;        ///< ecce-rate answered
  std::string level;         ///< "qrrho-grimme:100, 1 M standard state"
  std::string note;          ///< what is awaited, ecce-rate's refusal, or its notes
  std::vector<std::pair<std::string, std::string> > rows;   ///< label, value
  RateResult data;
  std::string text() const;  ///< the box as plain lines
};

/// The reactant, TS and product calculations of the steps; false when the
/// workflow has no reactant or no TS step.
bool rateRequest(const std::vector<KeyStep>& steps, RateRequest& req, std::string& waiting);

RateBoxResult rateBox(const std::vector<KeyStep>& steps, const std::string& temperature,
                      const std::string& treatment = "qrrho-grimme:100",
                      const std::string& standardState = "1M");

/// A number as 1.234e-05.
std::string rateNum(double v);

/// The T-range table and the fits as text and as CSV.
std::string rateDetailsText(const RateResult& r);
std::string rateDetailsCsv(const RateResult& r);

#endif
