#ifndef CLIPPLANEPANEL__H
#define CLIPPLANEPANEL__H

#include <functional>
#include <string>
#include <wx/panel.h>

class SGContainer;
class wxButton;
class wxCheckBox;
class wxChoice;
class wxSlider;
class wxSpinCtrl;
class wxSpinEvent;
class wxStaticText;

/**
 * A plane that removes everything on one side of it, so a structure is
 * shown cut open.  The plane is a normal plus an offset in Angstrom from a
 * reference point (the middle of the structure along that normal, or the
 * centre of the three atoms for an atom plane); the scene node itself is
 * SGContainer::setClipPlane.  The panel and the script hooks use the same
 * setters.
 */
class ClipPlanePanel : public wxPanel
{
   public:
      enum Direction { DIR_X, DIR_Y, DIR_Z, DIR_HKL, DIR_ATOMS };

      ClipPlanePanel(wxWindow *parent, std::function<SGContainer*()> getSG);

      void setOn(bool on);
      void setDirection(Direction dir);
      void setHKL(int h, int k, int l);
      //  Numbered from 1, as everywhere the user sees an atom number.
      bool setAtoms(int a, int b, int c);
      void setOffset(double angstrom);
      void flip();
      //  The structure changed (or a different one opened): re-apply.
      void refresh();

      bool isOn() const { return p_on; }
      std::string describe() const;
      //  The panel has no message line of its own for the script's sake.
      const std::string& problem() const { return p_problem; }

   private:
      bool plane(double n[3], double& ref, double& lo, double& hi);
      void apply();
      void syncControls();
      void OnOn(wxCommandEvent&);
      void OnDirection(wxCommandEvent&);
      void OnHKL(wxSpinEvent&);
      void OnSlider(wxCommandEvent&);
      void OnFlip(wxCommandEvent&);

      std::function<SGContainer*()> p_getSG;
      bool p_on = false, p_flip = false;
      Direction p_dir = DIR_X;
      int p_h = 1, p_k = 0, p_l = 0;
      int p_atoms[3] = {0, 0, 0};
      double p_offset = 0.0;
      std::string p_problem;

      wxCheckBox *p_onBox;
      wxChoice *p_dirChoice;
      wxSpinCtrl *p_hs, *p_ks, *p_ls;
      wxSlider *p_slider;
      wxStaticText *p_offsetText, *p_note;
      wxButton *p_flipButton;
};

#endif
