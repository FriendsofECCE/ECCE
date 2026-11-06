#ifndef MOMENTSPANEL__H
#define MOMENTSPANEL__H


#include <string>
#include <vector>
#include "VizPropertyPanel.H"


class SliderCombo;
class wxGrid;


/**
 * A class for displaying dipole type moments.
 * It is customized to show a property with just 1 row corresponding to
 * X,Y,Z values.  
 * Subclass for a more generic display - see QuadPolePanel
 */
class ewxStaticText;
class ewxCheckBox;
class ewxChoice;
class ewxGrid;

class MomentsPanel : public VizPropertyPanel
{
   DECLARE_DYNAMIC_CLASS(MomentsPanel)
   DECLARE_EVENT_TABLE()

   public:

      static const wxWindowID ID_DIALOG_MOMENTS;
      static const wxWindowID ID_MOMENTS_SLIDER;
      static const wxWindowID ID_BUTTON_MOMENTS_VECCOLOR;
      static const wxWindowID ID_MOMENTS_GRID;

      MomentsPanel();
      MomentsPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Moments");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Moments");
      virtual ~MomentsPanel();

      // Bond dipoles (#849), also driven by the Builder script command
      // "bonddipoles": switch the display on or off (key: the charge
      // property, "" keeps the choice) and read the table back as text rows.
      bool hasBondDipoles() const { return p_bondCheck != NULL; }
      void setBondDipoles(bool on, const string& key = "");
      vector<vector<string> > bondDipoleRows() const { return p_bondRows; }

      virtual void receiveFocus();
      virtual void loseFocus();

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      virtual string getConfigKey();

      virtual void OnVecColorClick( wxCommandEvent& event);
      virtual void OnEndSliderMotion(wxScrollEvent& event);
      virtual void OnSliderTextEnter(wxCommandEvent& event);

      /* The report oriented table showing Moments. Could be dipole... */
      wxGrid *p_grid;  

      SliderCombo *p_slider;

      /* The dipole's note: magnitude, pole charge and separation. */
      ewxStaticText *p_dipoleNote = 0;

      ewxCheckBox *p_bondCheck = NULL;
      ewxChoice *p_bondChoice = NULL;
      ewxGrid *p_bondGrid = NULL;
      ewxStaticText *p_bondNote = NULL;
      vector<string> p_bondKeys;
      vector<vector<string> > p_bondRows;
      void fillBondTable();
      void OnBondToggle(wxCommandEvent& event);
      void OnBondChoice(wxCommandEvent& event);

      /* The first time, we want to let the command guess the scale
         but after that, stick with the user's choice. */
      bool p_guessScale;

      /* DIPOLE or MP2DIPOLE drawn by this class itself (not a subclass):
         the panel states the arrow's length, origin and direction. */
      bool isDipole();
      virtual bool drawsOverlay() const { return true; }
      virtual bool isPolarizability() const { return false; }
      // Quadrupole, octupole, hexadecapole: drawn as angular surfaces.
      virtual bool isMultipole() const { return false; }

};

#endif // MOMENTSPANEL_H

