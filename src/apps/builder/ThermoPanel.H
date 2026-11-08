#ifndef THERMOPANEL__H
#define THERMOPANEL__H

#include <string>
#include <wx/string.h>
#include "PropertyPanel.H"
#include "util/ThermoRun.H"

class wxChoice;
class wxGrid;
class wxStaticText;
class wxTextCtrl;

/**
 * Thermochemistry (#692) for a frequency calculation: H, S and G recomputed
 * by ecce-thermo from the stored frequencies with a chosen low-frequency
 * treatment, temperature and standard state, next to the values the code
 * itself printed (which are left as they are).
 */
class ThermoPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(ThermoPanel)

   public:
      ThermoPanel();
      ThermoPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ThermoPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ThermoPanel");
      virtual ~ThermoPanel();

      virtual int minimumHeight() const { return 220; }
      // Leave room below the property list for wrapped controls and the grid.
      virtual int openingHeight() const { return 300; }

      /** Test hook: set the controls (treatment by ecce-thermo name). */
      bool setChoice(const std::string& treatment, double kelvin, bool oneMolar);
      /** What the table shows: recomputed H, S, G; false if there are none. */
      bool shown(ThermoValue *v) const;
      std::string errorText() const { return p_error; }

   protected:
      void fill();
      void rewrap();
      void OnChange(wxCommandEvent& event);

   private:
      wxChoice *p_treatment, *p_state;
      wxTextCtrl *p_temp;
      wxStaticText *p_text;
      wxString p_note;
      int p_wrapWidth = 0;
      wxGrid *p_grid;
      std::string p_error;
      bool p_have;
      ThermoValue p_value;
};

#endif
