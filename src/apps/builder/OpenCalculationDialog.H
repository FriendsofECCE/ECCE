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

    /** The file types the Builder opens, as a wx wildcard ("All Supported
        Types" first, then one entry per calculation type). */
    static wxString wildcard();
    /** The label of entry <index> of wildcard(). */
    static wxString wildcardLabel(int index);
};

#endif // OPENCALCULATIONDIALOG_H_
