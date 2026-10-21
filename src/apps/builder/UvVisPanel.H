#ifndef UVVISPANEL__H
#define UVVISPANEL__H

#include "PropertyPanel.H"
#include "SpectrumCanvas.H"
#include "tdat/ExcitedStates.H"

class wxChoice;
class wxGrid;
class wxGridEvent;
class wxTextCtrl;

/**
 * The UV-vis spectrum of a TD-DFT / CIS calculation (#383): sticks at the
 * excitation energies with their oscillator strengths, a Gaussian
 * broadening whose width is in eV, an axis in nm or eV, a table of the
 * states, and the orbital transitions of the selected state.  What the
 * curve is lives in tdat/ExcitedStates.H and VibSpectrum.H; this only
 * gathers options and draws.
 */
class UvVisPanel : public PropertyPanel, public SpectrumClickTarget
{
  DECLARE_DYNAMIC_CLASS(UvVisPanel)

  public:
    UvVisPanel();
    UvVisPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="UvVisPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="UvVisPanel");
    virtual ~UvVisPanel();

    virtual int minimumHeight() const { return 420; }
    virtual int openingHeight() const { return 560; }
    virtual void refresh();

    /** A stick was clicked: select that state. */
    virtual void modeClicked(int mode);

    void selectState(int index);
    int  selectedState() const { return p_selected; }
    /** Width in eV (0: sticks only) and axis; the same as the controls. */
    void setWidth(double ev);
    void setAxis(SpectrumXUnit unit);
    /** The spectrum as CSV; false if the file cannot be written. */
    bool exportCsv(const wxString& path);

    const ExcitedStates& states() const { return p_states; }
    const SpectrumCanvas *canvas() const { return p_canvas; }

  protected:
    void load();
    void fillTable();
    void showTransitions(int row);
    void applyWidth();
    void exportDialog();
    void dumpIfRequested();
    void onCell(wxGridEvent& event);

  private:
    SpectrumCanvas *p_canvas;
    wxGrid *p_grid;
    wxTextCtrl *p_detail;
    wxTextCtrl *p_width;
    wxChoice *p_axis;
    ExcitedStates p_states;
    SpectrumXUnit p_unit;
    double p_fwhm;
    int p_selected;
    bool p_dumped;
};

#endif
