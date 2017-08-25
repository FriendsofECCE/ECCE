#ifndef CLEANPARAMETERS_H_
#define CLEANPARAMETERS_H_

#include "wxgui/ewxDialog.H"

class ewxSpinCtrl;
class ewxTextCtrl;

class CleanParameters : public ewxDialog
{
  public:
    CleanParameters(wxWindow *parent);
    bool Create(wxWindow *parent);

    virtual void EndModal(int retCode);

    int getMaxIterations() const;
    double getConvergence() const;
    double getForceCutoff() const;

    void saveSettings();
    void restoreSettings();

  protected:
    wxSizer * createUnitLabel(const wxString& unit, const wxString& range);

  private:
    ewxSpinCtrl *p_maxIterations;
    ewxTextCtrl *p_convergence;
    ewxTextCtrl *p_forceCutoff;

    static const int DEFAULT_MAX_ITERATIONS;
    static const double DEFAULT_CONVERGENCE;
    static const double DEFAULT_FORCE_CUTOFF;
};

#endif // CLEANPARAMETERS_H_
