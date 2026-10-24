///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: JobProgress.H
//
// DESCRIPTION:
//    A running job's progress snapshot (#284) and what the Organizer's
//    Progress box shows of it.  eccejobmonitor builds the snapshot from a
//    code's [PROGRESS] lines, eccejobstore relays it as ecce_url_progress
//    and stores the latest as the calculation's ecce:progress.  Text, one
//    line each:
//
//      # seq=7 sent=1760000000.123          header (monitor's clock)
//      opt step=4 energy=-76.4 maxgrad=1.2e-03/3e-04 dE=...
//      opt~ energy=-76.1,-76.3,-76.4        history: one value per record
//      scf cycle=12 dE=-3.1e-06/1e-10 grad=2.0e-04/1e-07
//      freq point=14/54 eta=120
//
//    A value "a/b" is a quantity and its target.  No wx here, so the
//    rules can be tested without a display (tests/progress).
///////////////////////////////////////////////////////////////////////////////
#ifndef JOBPROGRESS_HH
#define JOBPROGRESS_HH

#include <string>
#include <utility>
#include <vector>

class JobProgress
{
public:
  struct Stage {
    std::string name;
    std::vector<std::pair<std::string, std::string> > fields;
    std::string historyKey;
    std::vector<double> history;

    // "" when absent.
    std::string field(const std::string& key) const;
  };

  // One convergence criterion: value against target.
  struct Criterion {
    std::string label;    // "max force"
    std::string value;    // "1.2e-03 / 3e-04"
    int met;              // 1 met, 0 not met
  };

  // What the box draws for one stage: a title and lines on the left, a
  // plot or a bar beside them.
  struct Block {
    std::string title;
    std::vector<std::string> lines;
    std::vector<Criterion> criteria;
    std::vector<double> plot;   // empty: no plot
    bool plotLog = false;       // plot is log10 of |value|
    std::string plotLabel;
    // x axis of the plot: the last point sits at plotXLast, the axis runs
    // 0..plotXEnd (the run's total when known, so the curve's end shows how
    // far it has got). plotXUnit is "ps", "step", "cycle", ... .
    double plotXLast = 0;
    double plotXEnd = 0;
    std::string plotXUnit;
    bool bar = false;
    double fraction = 0;        // of the bar, 0..1
  };

  // False for empty or unreadable text (nothing to show).
  bool parse(const std::string& text);

  bool empty() const { return p_stages.empty(); }
  int seq() const { return p_seq; }
  // When the job store relayed it (its clock, seconds since 1970); 0 if
  // not known.
  double at() const { return p_at; }
  const std::vector<Stage>& stages() const { return p_stages; }
  const Stage* stage(const std::string& name) const;

  // While the job runs.  `age`: seconds since the snapshot arrived, for
  // the time left of a frequency run.
  // `finished`: the job has ended, so no time left is shown.
  std::vector<Block> running(double age, bool finished = false) const;

  // After it: one or two lines.  `completed` is false for a job that
  // stopped (failed, killed): then the lines say where it got to.
  std::vector<std::string> summary(bool completed) const;

  // One short line for a table cell while the job runs, e.g. "Freq 14/54",
  // "Opt step 4 - SCF 10" or "SCF 7 - dE 4e-5" (UTF-8 middle dot, Delta,
  // minus sign).  The stage that came last of opt/freq is the current
  // one.  Empty without progress.
  std::string shortLine() const;

  // "a/b" -> a, b.  False when a is not a number; hasTarget false
  // without "/b".
  static bool split(const std::string& v, double& value, double& target,
                    bool& hasTarget);

  // "updated 3 s ago", "updated 2 min ago".
  static std::string updatedText(double seconds);

  // Out of contact (#284), the table cell and the folded box: "no contact
  // for 17 min" after a warning sign (UTF-8).
  static std::string noContactText(double seconds);
  // "14:02 (17 min ago)": when the job was last heard from, local time.
  static std::string lastHeardText(double when, double now);

  // The running mean and standard deviation the monitor keeps for a
  // dynamics stage (avg= sd= n=); false without them.
  bool average(const std::string& stage, double& avg, double& sd,
               int *n = 0) const;

  // The stage's title: "Optimisation", "SCF", "Frequencies", or its name.
  static std::string title(const std::string& stage);

private:
  Block stageBlock(const Stage& st, double age, bool finished) const;
  void plotAxis(const Stage& st, const std::string& rec,
                const std::string& recName, Block& b) const;

  int p_seq = 0;
  double p_at = 0;
  std::vector<Stage> p_stages;
};

#endif
