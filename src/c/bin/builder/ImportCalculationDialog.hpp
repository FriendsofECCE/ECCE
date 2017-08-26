#ifndef IMPORTCALCULATIONDIALOG_H_
#define IMPORTCALCULATIONDIALOG_H_

#include "OpenCalculationDialog.hpp"

/**
 * Customized dialog for finding a URL of an Experiment.
 */
class ImportCalculationDialog : public OpenCalculationDialog
{
  public:
    ImportCalculationDialog();
    ImportCalculationDialog(wxWindow *parent);
    virtual ~ImportCalculationDialog();

    bool Create(wxWindow *parent);
};

#endif // IMPORTCALCULATIONDIALOG_H_
