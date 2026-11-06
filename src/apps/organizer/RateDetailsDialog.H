#ifndef RATEDETAILSDIALOG_HH
#define RATEDETAILSDIALOG_HH

#include <string>

#include <wx/dialog.h>

#include "util/RateRun.H"

class wxTextCtrl;

/**
 * The Rate box's Details... window (#872): ecce-rate over a temperature range,
 * the table, the Arrhenius and Eyring fits, and Export CSV.
 */
class RateDetailsDialog : public wxDialog
{
public:
  RateDetailsDialog(wxWindow *parent, const RateRequest& req);

private:
  void update();
  void exportCsv();

  RateRequest p_req;
  RateResult p_result;
  wxTextCtrl *p_low, *p_high, *p_step, *p_text;
};

#endif
