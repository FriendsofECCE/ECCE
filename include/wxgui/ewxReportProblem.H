#ifndef _EWXREPORTPROBLEM_H
#define _EWXREPORTPROBLEM_H

#include "wx/string.h"

class wxWindow;

/**
 * Help > Report a Problem...: packs the diagnostics a bug report needs
 * into ~/ecce-bug-<time>/ and ~/ecce-bug-<time>.zip, and says where they
 * are. Nothing is uploaded.
 */
namespace ewxReportProblem {

  /// Collect and zip; returns the archive path, or "" with error set.
  /// Keeps the event loop running until ecce-diagnose is done.
  wxString collect(wxString& error, wxWindow* parent = nullptr);

  /// collect(), then the dialog with "Open folder" / "Open issue page".
  void run(wxWindow* parent);
}

#endif
