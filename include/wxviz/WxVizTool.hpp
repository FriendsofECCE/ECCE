#ifndef WXVIZTOOL_H
#define WXVIZTOOL_H

#include <string>
using std::string;
#include "wx/fileconf.h"

#include "util/Preferences.H"

#include "wxviz/WxVizToolFW.H"

class Residue;

/**
 * Interface that viz toolkits can inherit from to become part of the
 * Viz framework and gain access to standard viz objects and methods.
 * It is assumed that the Viz framework object will call ensure the object
 * is properly initialized for use.
 *
 * WxVizTools can access the main framework methods by doing
 * getFW()->somemethod().
 * @see WxVizToolFW.
 *
 */
class WxVizTool 
{
   public:
      WxVizTool();
      virtual ~WxVizTool();

      /* Establishes connection between toolkit and framework.
         This is done outside the constructor so as not to affect
         gui class constructors. But think of it as required for
         successful class creation. */
      virtual void connectToolKitFW(WxVizToolFW * fw);

      WxVizToolFW& getFW() const;

      /* common interface for initializing and saving preferences.  */
      virtual void restoreSettings(wxConfig * config);
      virtual void saveSettings(wxConfig * config);

      /* executes select which then propagates selection to the viewer
         and all toolkits. */
      virtual void selectCommand(const vector<int>& atoms,
                                 const vector<int>& bonds);

      /* executes select which then propagates selection to the viewer
         and all toolkits. */
      virtual void selectCommand(const vector<bool>& atoms,
                                 const vector<bool>& bonds);

      /* Set selection in viewer. */
      virtual void setSelection(const vector<int>& atoms, 
                                const vector<int>& bonds);

      /* Set selection in viewer using a bit mask. */
      virtual void setSelection(const vector<bool>& atoms, 
                                const vector<bool>& bonds);

      /* Selects all atoms and bonds in the specified residue. 
         extendSelect controls whether this adds to the current selection
         or replaces it. */
      virtual void selectResidue(const Residue& res, bool extendSelect);

      /* Selects all atoms and bonds in the specified residues (by index). 
         extendSelect controls whether this adds to the current selection
         or replaces it. */
      virtual void selectResidues(const vector<int>& resIdxs, bool extendSelect);

      virtual void invertSelection();

      virtual bool areLabelsOn() const;

      /* Refresh the UI.  Works around problem with AUI initialization */
      virtual void refresh() {;}


   protected:


      WxVizToolFW *p_vizfw;

};



#endif
