/**
 * @file
 */
#ifndef _GROMACSPROFILE_H_
#define _GROMACSPROFILE_H_

#include "wx/wx.h"
#include "wx/notebook.h"

/**
 * Small helpers the MD panels use to show only the controls that mean
 * something for GROMACS.  The panels are NWChem's; a control with no GROMACS
 * counterpart is hidden rather than left to do nothing, and hiding is undone
 * when the window is next used for an NWChem task.
 */
class GromacsProfile
{
   public:
      /** Show or hide a window. */
      static void show(wxWindow *w, bool show);

      /** Show or hide every item of the sizer row that holds the window. */
      static void showRow(wxWindow *w, bool show);

      /** Show or hide every child of root except one (and its children). */
      static void showAllBut(wxWindow *root, wxWindow *keep, bool show);

      /** Set the label of a label-bearing window (text, check box...). */
      static void label(wxWindow *w, const wxString& text);

      /**
       * Give the first window under root whose label is from the label to
       * (labels of controls that have no id cannot be found any other way).
       */
      static void relabel(wxWindow *root, const wxString& from,
                          const wxString& to);

      /** Show or hide the first static text under root that reads label. */
      static void showByLabel(wxWindow *root, const wxString& label,
                              bool show);

      /**
       * Put a notebook page in or out of the notebook; index is where it
       * belongs among the pages that are in.
       */
      static void showTab(wxNotebook *nb, wxWindow *page,
                          const wxString& title, int index, bool show);

      /** Lay the panel out again after controls came or went. */
      static void relayout(wxWindow *w);
};

#endif
