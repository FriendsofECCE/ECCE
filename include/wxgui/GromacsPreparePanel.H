/**
 * @file
 */
#ifndef _GROMACSPREPAREPANEL_H_
#define _GROMACSPREPAREPANEL_H_

#include <string>
#include <utility>
#include <vector>
using std::vector;
using std::string;

#include "wxgui/ewxPanel.H"

class MDEdBase;
class MdTask;
class GromacsPrepareModel;
class ewxButton;
class ewxCheckBox;
class ewxChoice;
class ewxStaticText;
class ewxTextCtrl;
class wxStaticBox;
class wxSizer;
class wxWindow;

/**
 * The page of the GROMACS Prepare editor: what pdb2gmx, editconf, solvate
 * and genion are asked to do.  The values live in a GromacsPrepareModel; the
 * source structure, when it is a file and not the Builder's, is stored in
 * the task's inputs as soon as it is chosen (like the Inputs page of the
 * other GROMACS editors) and is not part of the model.
 */
class GromacsPreparePanel : public ewxPanel
{
   public:
      GromacsPreparePanel(MDEdBase *owner, wxWindow *parent);
      virtual ~GromacsPreparePanel();

      void setModel(GromacsPrepareModel *model);
      void setContext(const string& url);
      void setEditable(bool editable);

      /** Show the model's values. */
      void refreshGUI();

      /** False, with the reason, while a field holds something unusable. */
      bool validate(string& why) const;

      /** The attached source PDB's name, empty for the Builder's structure. */
      string sourceFile() const;

      /** Set a field as a person would (test hook); false with a reason. */
      bool testSet(const string& field, const string& value, string& message);

      /** The built mineral's cell vectors in nm, c with the water layer. */
      bool cellVectors(double v[3][3]) const;

      /** The cut-off warning for the built cell; empty when it holds (test hook too). */
      string cellWarning() const;

      /** Where the unit cell's Browse... starts (also a test hook). */
      string cellStartDir() const;

      /** The size the page needs in its most demanding state (all sources). */
      wxSize bestSizeAllSources();

      /** What each box asks for (test hook). */
      string layoutReport() const;

      /** The fields in words (test hook). */
      string summary() const;

      /** The force field choice, entries joined by "|" (test hook). */
      string forceFieldChoices() const;

   protected:
      void OnForceField(wxCommandEvent& event);
      void OnImportForceField(wxCommandEvent& event);
      void OnWater(wxCommandEvent& event);
      void OnStructure(wxCommandEvent& event);
      void OnBrowse(wxCommandEvent& event);
      void OnCheck(wxCommandEvent& event);
      void OnBox(wxCommandEvent& event);
      void OnIon(wxCommandEvent& event);
      void OnNumber(wxCommandEvent& event);
      void OnOtherForceField(wxCommandEvent& event);
      void OnSource(wxCommandEvent& event);
      void fitToSource();
      void OnMinffText(wxCommandEvent& event);
      void OnMinffChoice(wxCommandEvent& event);
      void OnMineralForceField(wxCommandEvent& event);
      void OnMinffCell(wxCommandEvent& event);
      void OnMinffBuild(wxCommandEvent& event);

   private:
      MdTask *task() const;
      void changed();
      void fillWaters(const string& preferred);
      void fillForceFields();
      int otherIndex() const;
      int ffIndex(const string& id) const;
      string ffId(int index) const;
      void stageForceField(bool force);
      bool importForceField(const string& path, int clash, string& message);
      bool chooseFile();
      bool attachPdb(const string& path, string& message);
      void detachPdb();
      void showStructure();
      void showFields();
      bool isMinff() const;
      bool isClayff() const;
      void fillIons();
      void showIonNote();
      void markBuildNeeded(bool needed);
      void minffStale();
      void showCellWarning();
      void detachMineral();
      bool buildMineral(bool withDialog, string& message);
      bool storeMineral(const string& dir, string& message);
      void showMineral();
      void relayout();

      MDEdBase *p_owner;
      GromacsPrepareModel *p_model;
      string p_url;
      bool p_editable;
      bool p_refreshing;
      string p_badDistance, p_badConcentration, p_badRmin;
      string p_buildWarnings;   // minff_topology.py warnings of the last build
      string p_cellPath;        // the unit cell chosen for MINFF, this session
      bool p_built;
      bool p_staging;
      int p_testClash;          // test hook: 1 replace, 2 cancel on a name clash
      vector<std::pair<string, string> > p_ffItems;   // id, label of the choice
      vector<wxWindow*> p_proteinOnly, p_minffOnly, p_clayffOnly;
      vector<wxWindow*> p_minffFfOnly;   // the MINFF parameter choices, off with ClayFF
      wxStaticBox *p_ffBox, *p_waterBox, *p_mineralBox;
      ewxStaticText *p_distLabel, *p_ionNote, *p_summaryLabel, *p_cellLabel;
      ewxTextCtrl *p_mrep[3], *p_msubs, *p_mmineral, *p_mseed;
      string repeat() const;
      void setRepeat(const string& value);
      ewxChoice *p_mvariant, *p_mangle, *p_mions, *p_mff, *p_mbend;
      ewxCheckBox *p_mrigid;
      ewxButton *p_mcellBtn, *p_mbuild;
      wxPanel *p_mbuildFrame;
      wxSizer *p_leftSizer, *p_rightSizer, *p_minffSizer;

      ewxChoice *p_source;
      ewxChoice *p_structure;
      ewxButton *p_browse;
      ewxStaticText *p_file;
      ewxChoice *p_ff;
      ewxButton *p_ffImport;
      ewxTextCtrl *p_otherFf;
      ewxChoice *p_water;
      ewxCheckBox *p_ignh, *p_neut;
      ewxCheckBox *p_merge;
      ewxChoice *p_box;
      ewxTextCtrl *p_dist;
      ewxTextCtrl *p_conc;
      ewxTextCtrl *p_rmin;
      ewxChoice *p_pion;
      ewxChoice *p_nion;
};

#endif
