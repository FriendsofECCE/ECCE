#ifndef _EWXSELECTABLETEXT_H
#define _EWXSELECTABLETEXT_H

#include <wx/textctrl.h>
#include "dsm/LaunchInfoRows.H"

/**
 * A value shown like a label that can still be selected with the mouse and
 * copied (#408): a read-only, borderless text field on the parent's
 * background, as wide as its text.
 */
class ewxSelectableText : public wxTextCtrl
{
  public:
    ewxSelectableText(wxWindow* parent, const wxString& text,
                      long align = wxTE_LEFT);
    void SetLabel(const wxString& text) override;

  private:
    void fit();
};

/**
 * A selectable value: on GTK the native label with selection switched on
 * (it keeps the label's look; a text field cannot take the theme's panel
 * colour there), elsewhere an ewxSelectableText.
 */
wxWindow *ewxSelectableValue(wxWindow* parent, const wxString& text,
                             long align = wxTE_LEFT);

/** Lets an existing label's text be selected (GTK; elsewhere unchanged). */
void ewxMakeSelectable(wxWindow *label);

/** Whether `w` lets its text be selected (for tests). */
bool ewxIsSelectable(wxWindow *w);

/**
 * Makes the right-hand column of a two-column grid body wrap within the
 * window's width (#490), breaking inside a word (at '/' and '_' in paths)
 * only when it does not fit. GTK only; elsewhere the labels stay on one
 * line. Call after the rows are added; the wrap follows size events.
 */
void ewxWrapValueColumn(wxWindow *body);

/**
 * The Launch Info rows (#648) as two independent columns side by side, each
 * a label and a value per row, labels left-aligned. A row that is not in
 * `cols` does not exist. Labels are named "launch:<label>" and values
 * "launchvalue:<label>" so a test can find them. Values wrap to the width
 * that ewxFitLaunchInfo() works out; call it from the owner's size event and
 * after a Layout().
 */
wxSizer *ewxLaunchInfoBlock(wxWindow *parent,
                            const LaunchInfoRows::Columns& cols);

/** Wraps the values of every ewxLaunchInfoBlock below `body`'s sizer. */
void ewxFitLaunchInfo(wxWindow *body);

#endif
