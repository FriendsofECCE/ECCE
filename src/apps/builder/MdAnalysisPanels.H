#ifndef MDANALYSISPANELS__H
#define MDANALYSISPANELS__H

#include <string>
#include <vector>

#include "PropertyPanel.H"
#include "util/MdAnalysis.H"

class wxButton;
class wxCheckBox;
class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;

/**
 * Shared part of the Speed Distribution and Displacement panels of a
 * GROMACS dynamics run: the analysis comes from the run's MDSPEED / MDMSD
 * property (include/util/MdAnalysis.H does the numbers, the panel draws), and
 * another calculation of the same folder can be drawn over it.
 */
class MdAnalysisPanel : public PropertyPanel
{
   DECLARE_ABSTRACT_CLASS(MdAnalysisPanel)

   public:
      MdAnalysisPanel();
      virtual ~MdAnalysisPanel();

      /** Draw the calculation of this name over this one; "" removes it.
          False if no calculation of the folder has the data. */
      bool compareWith(const std::string& name);
      const std::string& compareName() const { return p_compareName; }
      bool exportCsv(const wxString& path);
      int curves() const { return p_curveTotal; }
      /** The text the panel shows above its plot. */
      wxString summary() const;
      XYPlotCanvas *plot() const { return p_plot; }

   protected:
      bool build(IPropCalculation *calculation, wxWindow *parent, wxWindowID id,
                 const wxPoint& pos, const wxSize& size, long style,
                 const wxString& name);
      virtual const char *propertyKey() const = 0;
      virtual const char *exportName() const = 0;
      virtual void loadRecords(const std::vector<std::string>& own,
                               const std::vector<std::string>& other) = 0;
      virtual void redraw() = 0;
      virtual std::string csvText() = 0;
      /** A control row beside the buttons (fit range ...); may add nothing. */
      virtual void addControls(wxSizer *bar) { (void)bar; }

      std::vector<std::string> recordsOf(IPropCalculation *calc) const;
      std::string calcName() const;
      void refill();

      XYPlotCanvas *p_plot;
      wxStaticText *p_text;
      std::string p_compareName;
      int p_curveTotal;

   private:
      std::vector<IPropCalculation*> comparable() const;
      void onCompare(wxCommandEvent&);
      void onExport(wxCommandEvent&);
      std::vector<std::string> p_otherRecords;
      wxButton *p_remove;
};

/** Histogram of atomic speeds per kind of atom with its Maxwell-Boltzmann curve. */
class SpeedPanel : public MdAnalysisPanel
{
   DECLARE_DYNAMIC_CLASS(SpeedPanel)

   public:
      SpeedPanel();
      SpeedPanel(IPropCalculation *calculation, wxWindow *parent,
                 wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
                 const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
                 const wxString& name="SpeedPanel");
      virtual bool Create(IPropCalculation *calculation, wxWindow *parent,
                 wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
                 const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
                 const wxString& name="SpeedPanel");
      const std::vector<SpeedCurve>& own() const { return p_own; }

   protected:
      virtual const char *propertyKey() const { return "MDSPEED"; }
      virtual const char *exportName() const { return "speeds"; }
      virtual void loadRecords(const std::vector<std::string>& own,
                               const std::vector<std::string>& other);
      virtual void redraw();
      virtual std::string csvText();

   private:
      std::vector<SpeedCurve> p_own, p_other;
};

/** MSD of the molecules against lag time, with the Einstein fit for D. */
class MsdPanel : public MdAnalysisPanel
{
   DECLARE_DYNAMIC_CLASS(MsdPanel)

   public:
      MsdPanel();
      MsdPanel(IPropCalculation *calculation, wxWindow *parent,
               wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
               const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
               const wxString& name="MsdPanel");
      virtual bool Create(IPropCalculation *calculation, wxWindow *parent,
               wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
               const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
               const wxString& name="MsdPanel");
      const std::vector<MsdCurve>& own() const { return p_own; }

      /** The window (ps) of the Einstein fit; from >= to is the default
          25% to 100% of the lag times. */
      void setFit(double from, double to);
      void setLogLog(bool on);
      /** D of the i-th molecule type of this calculation in 10^-5 cm^2/s; -1 if none. */
      double diffusion(int i) const;

   protected:
      virtual const char *propertyKey() const { return "MDMSD"; }
      virtual const char *exportName() const { return "msd"; }
      virtual void loadRecords(const std::vector<std::string>& own,
                               const std::vector<std::string>& other);
      virtual void redraw();
      virtual std::string csvText();
      virtual void addControls(wxSizer *bar);

   private:
      void onFit(wxCommandEvent&);
      void onLogLog(wxCommandEvent&);
      std::vector<MsdCurve> p_own, p_other;
      double p_from, p_to;
      wxTextCtrl *p_fromBox, *p_toBox;
      wxCheckBox *p_log;
      wxString p_summary;
};

#endif
