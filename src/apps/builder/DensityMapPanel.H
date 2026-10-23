#ifndef DENSITYMAPPANEL__H
#define DENSITYMAPPANEL__H

#include <memory>
#include <string>

#include "tdat/DensityPlane.H"
#include "PropertyPanel.H"

class wxChoice;
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
 * Plane: the molecular plane (best fit, offered for a planar or linear
 * molecule), a plane through three chosen atoms, or a plane through two
 * atoms standing perpendicular to the molecular plane -- where a pi
 * orbital of a planar molecule has its lobes.
 */
class DensityMapPanel : public PropertyPanel
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
      virtual bool prefersFloating() const { return true; }
      virtual wxSize preferredFloatingSize() const { return wxSize(760, 760); }
      virtual int openingHeight() const { return 560; }

      /**
       * What the controls do, for a script or a test.  `orbital` 0 is the
       * total density; plane 0 molecular, 1 three atoms, 2 axial (atoms
       * 1-based as labelled).  False if the plane or orbital is refused.
       */
      bool setView(int orbital, bool beta, int plane, int a, int b, int c);

      /** The picture, drawn the way the panel draws it. */
      bool savePng(const wxString& path, int width, int height);
      bool exportCsv(const wxString& path);

      const DensityPlane::Map& map() const { return p_map; }

   protected:
      virtual void initialize();

   private:
      void build();
      void compute();
      void syncControls();
      bool evaluate(int orbital, bool beta, DensityPlane::PlaneKind kind,
                    int a, int b, int c, std::string& why);
      void onChanged();

      std::unique_ptr<DensityPlane::Evaluator> p_eval;
      DensityPlane::Map p_map;
      bool p_haveMap;
      bool p_busy;

      wxChoice *p_show, *p_spin, *p_plane, *p_atomA, *p_atomB, *p_atomC;
      wxSpinCtrl *p_mo;
      wxStaticText *p_status;
      DensityMapCanvas *p_canvas;
};

#endif
