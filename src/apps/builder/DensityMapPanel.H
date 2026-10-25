#ifndef DENSITYMAPPANEL__H
#define DENSITYMAPPANEL__H

#include <memory>
#include <string>
#include <vector>

#include "tdat/DensityPlane.H"
#include "util/Listener.H"
#include "VizPropertyPanel.H"

class wxButton;
class wxChoice;
class wxRadioBox;
class wxTextCtrl;
class wxBoxSizer;
class wxSlider;
class wxSpinCtrl;
class wxStaticText;
class wxDC;
class DensityMapCanvas;

/**
 * The electron density, or one molecular orbital, drawn as contour lines
 * on a plane through the molecule (#215).  The numbers come from
 * DensityPlane, from the stored MO coefficients and basis, so it works
 * for every code whose MOs ECCE reads; this class only gathers the
 * options and draws.
 *
 * The plane is a point and a normal, set by atoms picked in the 3-D view
 * (three, or two for "through this bond, perpendicular to the molecular
 * plane"; the molecular plane is the first guess) or by the current view
 * ("Use this view", stored as a plane), plus an offset along the normal.
 * The plane is drawn in the viewer while the panel is shown.  The orbital
 * follows the MOs panel's selection.
 */
class DensityMapPanel : public VizPropertyPanel, public ecce::Listener
{
   DECLARE_DYNAMIC_CLASS(DensityMapPanel)

   public:
      DensityMapPanel();
      DensityMapPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Density Map");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="Density Map");
      virtual ~DensityMapPanel();

      virtual bool isRelevant(IPropCalculation *propCalc) const;
      //  The MOs panel owns the orbital surface in the viewer; this one
      //  only adds its plane, and must not take the focus from it.
      virtual bool drawsInViewer() const { return false; }
      virtual int openingHeight() const { return 560; }
      virtual bool Show(bool show=true);

      /**
       * What the controls do, for a script or a test.  `orbital` 0 is the
       * total density; plane 0 molecular, 1 three atoms, 2 two atoms
       * perpendicular to the molecular plane, 3 the stored view (atoms
       * 1-based as labelled).  `sum` shows the sum over the orbitals
       * degenerate with `orbital`.  False if the plane or orbital is refused.
       */
      bool setView(int orbital, bool beta, int plane, int a, int b, int c,
                   bool sum = false);

      /** As picking atoms in the 3-D view does: 2 or 3 atoms, 1-based. */
      bool pickAtoms(const std::vector<int>& atoms);
      /** The offset slider, A; `drag` computes the coarse grid only. */
      bool setOffset(double angstrom, bool drag = false);
      /** "Use this view": the screen axes in the stored frame. */
      bool useView(const double right[3], const double up[3]);
      bool useCurrentView();
      /** A click on plane entry N (0-3): resets the offset, picks defaults. */
      bool choosePlane(int n);
      /** Show or hide the plane controls (radio box, atoms, view); remembered. */
      void setPlaneControlsOpen(bool open);
      bool planeControlsOpen() const { return p_planeOpen; }
      /** Zoom the map about plane point (u, v), A; factor > 1 zooms in.  The
       *  window is recomputed at the same contour levels. */
      bool zoomAt(double u, double v, double factor);
      /** Back to the whole plane. */
      void resetZoom();
      bool zoomed() const { return p_zoomed; }
      /** Plane coordinates of atom N (1-based) in the current map. */
      bool atomPoint(int n, double& u, double& v) const;
      /** Where each control sits at a given panel width, and which are clipped. */
      std::string layoutReport(int width);
      /** The MOs panel's selection (1-based, alpha or beta). */
      void followMo(int mo, bool beta);

      /** The picture, drawn the way the panel draws it. */
      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);

      /** "key value" lines for headless checks. */
      std::string state() const;

      const DensityPlane::Map& map() const { return p_map; }

      virtual void eventMCB(const ecce::Event& event);

   protected:
      virtual void initialize();

   private:
      void build();
      void compute(bool coarse = false);
      void syncControls();
      bool currentPlane(DensityPlane::Plane& plane, std::string& why);
      void onChanged();
      void onPicked();
      void drawPlaneInViewer();
      void removePlaneFromViewer();
      void updateOffsetLabel();
      double offsetAngstrom() const;
      void newPlane();                 // offset back to 0
      void fillAtomLists();
      void chooseBondedB();
      void onControl(wxCommandEvent& event);
      void commitOffsetField();
      void ensurePlane();
      void setCaption();
      void relayout();
      std::string zoomKey(const DensityPlane::Plane& plane, int what,
                          int orbital, bool beta) const;

      bool p_zoomed;
      double p_zoomWin[4], p_fullWin[4], p_levelTop;
      std::string p_zoomKey;
      wxButton *p_zoomReset;
      std::string p_statusText;
      std::unique_ptr<DensityPlane::Evaluator> p_eval;
      DensityPlane::Map p_map;
      DensityPlane::Plane p_viewPlane;
      bool p_haveMap, p_haveView, p_busy, p_inViewer, p_dragging;

      wxChoice *p_show, *p_spin, *p_atomA, *p_atomB, *p_atomC;
      wxRadioBox *p_plane;
      wxTextCtrl *p_offsetField;
      wxButton *p_resetOffset;
      wxStaticText *p_labelA, *p_labelB, *p_labelC, *p_caption;
      wxBoxSizer *p_atomRow, *p_viewRow;
      wxButton *p_planeToggle;
      bool p_planeOpen, p_planeAuto;
      wxSpinCtrl *p_mo;
      wxSlider *p_offset;
      wxStaticText *p_status;
      wxButton *p_useView;
      DensityMapCanvas *p_canvas;
};

#endif
