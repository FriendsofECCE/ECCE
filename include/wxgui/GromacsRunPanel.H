/**
 * @file
 */
#ifndef _GROMACSRUNPANEL_H_
#define _GROMACSRUNPANEL_H_

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "wxgui/ewxPanel.H"

class MDEdBase;
class ewxButton;
class ewxCheckBox;
class ewxChoice;
class ewxStaticText;
class ewxTextCtrl;

/**
 * The "Run settings" page of a GROMACS task editor: the .mdp settings that
 * have no counterpart on NWChem's pages.  Defines and position restraints,
 * groups made from residue names, the barostat, and free .mdp lines.  The
 * values live in the Interaction model (GromacsRun) and so are saved with
 * the task; this panel only gathers them.
 */
class GromacsRunPanel : public ewxPanel
{
   public:
      GromacsRunPanel(MDEdBase *owner, wxWindow *parent);
      virtual ~GromacsRunPanel();

      /** Show the model's values; hides what the task cannot use. */
      void refresh();

      /** Re-read the chains of the topology (after the Inputs page changed). */
      void refreshChains();

      /** False once the task is submitted. */
      void setEditable(bool editable);

      /** What the page holds, in words: for the test hook. */
      string summary();

      /**
       * Set a field as the user would (through its widget and event):
       * defines, posres, posresname, chain:<id>, groups, comgroups, tcgroups,
       * barostat, pcoupltype, nstpcouple, extra, or "fill".
       */
      string setField(const string& name, const string& value);

   protected:
      void OnChanged(wxCommandEvent& event);
      void OnChain(wxCommandEvent& event);
      void OnFill(wxCommandEvent& event);
      void OnFillMolecules(wxCommandEvent& event);

   private:
      void store();
      void setChain(wxObject *box);
      string chainSummary();
      void warn();
      void fillGroups(bool molecules);

      MDEdBase      *p_owner;
      bool           p_loading;
      bool           p_editable;
      ewxTextCtrl   *p_defines;
      ewxCheckBox   *p_posres;
      ewxTextCtrl   *p_posresName;
      wxWindow      *p_defsBox;
      wxSizer       *p_chainRow;
      vector<string> p_chainIds;
      vector<ewxCheckBox*> p_chainBoxes;
      wxWindow      *p_groupBox;
      ewxTextCtrl   *p_groups;
      ewxChoice     *p_comGroups;
      ewxChoice     *p_tcGroups;
      wxWindow      *p_pressureBox;
      ewxChoice     *p_barostat;
      ewxChoice     *p_pcoupltype;
      ewxTextCtrl   *p_nstpcouple;
      ewxTextCtrl   *p_extra;
      wxPanel       *p_pullBox;
      ewxCheckBox   *p_pull;
      ewxTextCtrl   *p_pullGroup1, *p_pullGroup2, *p_pullRate, *p_pullK;
      ewxChoice     *p_pullGeom, *p_pullAxis;
      ewxCheckBox   *p_pullDim[3];
      ewxStaticText *p_warning;
};

#endif
