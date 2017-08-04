#ifndef OPENCALCULATIONDIALOG_H_
#define OPENCALCULATIONDIALOG_H_

#include "wxgui/ewxGenericFileDialog.H"

/**
 * Customized dialog for finding a URL of an Experiment.
 */
class OpenCalculationDialog : public ewxGenericFileDialog
{
  public:
    OpenCalculationDialog();
    OpenCalculationDialog(wxWindow *parent);
    virtual ~OpenCalculationDialog();

    bool Create(wxWindow *parent);
};

#endif // OPENCALCULATIONDIALOG_H_
