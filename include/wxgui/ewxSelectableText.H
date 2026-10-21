#ifndef _EWXSELECTABLETEXT_H
#define _EWXSELECTABLETEXT_H

#include <wx/textctrl.h>

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

#endif
