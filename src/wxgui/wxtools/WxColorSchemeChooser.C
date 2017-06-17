/**
 * @file
 *
 * Manage a group of color scheme buttons.
 *
 */
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxColorSchemeChooser.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/WxColorSchemeChooser.H"


IMPLEMENT_DYNAMIC_CLASS( WxColorSchemeChooser, ewxPanel )

/*
BEGIN_EVENT_TABLE( WxColorSchemeChooser, ewxPanel )

    EVT_LEFT_DOWN   ( WxColorSchemeChooser::OnMouseLeftDown )

END_EVENT_TABLE()
*/


/**
 * Constructors
 */
WxColorSchemeChooser::WxColorSchemeChooser( )
{
}


/**
 * Constructor.
 * Need to specify number of columns and gaps between rows and volumns first.
 */
WxColorSchemeChooser::WxColorSchemeChooser( int cols,
                                            int vgap,
                                            int hgap,
                                            wxWindow* parent,
                                            wxWindowID id,
                                            const wxPoint& pos,
                                            const wxSize& size,
                                            long style )
{
  ewxPanel::Create(parent, id, pos, size, style);

  p_topSizer = new wxFlexGridSizer(cols, vgap, hgap);
  SetSizer(p_topSizer);

  p_selection = -1;
  p_butWidth = 40;
  p_butHeight = 30;
  p_tooltip = "";
}


/**
 * Set the size of each button.
 */
void WxColorSchemeChooser::setButtonSize( int width, int height )
{
  p_butWidth = width;
  p_butHeight = height;
  
  if (p_buttons.size() != 0) {
    for (int i=0; i<p_buttons.size(); i++)
      p_buttons[i]->SetSize(p_butWidth, p_butHeight);
    p_topSizer->Layout();
  }
}


/**
 * Add one more button with the given color sets
 */
int WxColorSchemeChooser::addSchemeButton( const vector<wxColour> & colors )
{
  WxColorSchemeButton * button =
    new WxColorSchemeButton(this, p_buttons.size(), wxDefaultPosition,
                            wxSize(p_butWidth, p_butHeight));
  button->addColors(colors);
  if (p_tooltip != "")
    button->SetToolTip(p_tooltip);
  p_topSizer->Add(button, 0, wxALIGN_CENTER, 0);
  p_buttons.push_back(button);
  return p_buttons.size()-1;
}


/**
 * Add one more button with the given color sets
 */
int WxColorSchemeChooser::addSchemeButton( const vector<string> & colors )
{
  vector<wxColour> colBuffer;
  vector<string>::const_iterator colItor = colors.begin();
  for (; colItor != colors.end(); colItor++)
    colBuffer.push_back(wxColour(*colItor));
  return addSchemeButton(colBuffer);
}


/**
 * Get current selection
 */
int WxColorSchemeChooser::getSelection()
{
  return p_selection;
}


/**
 * Set current selection
 */
void WxColorSchemeChooser::setSelection( int index )
{
  // make sure index is less than total number of buttons
  if (index != p_selection) {
    if (p_selection >= 0)
      p_buttons[p_selection]->setSelected(false);
    p_buttons[index]->setSelected(true);
    p_selection = index;
  }
}

/**
 * Select a button by matching colors against the colors currently
 * defined for each button.  This is useful for serialized/deserialized
 * data sets.
 * Will select 0th button if there is no match.
 */
void WxColorSchemeChooser::setSelection( const vector<string>& colors)
{
  int matchBtn = 0; // A default in case we don't find one

  for (int idx=0; idx<p_buttons.size(); idx++) {
     WxColorSchemeButton *btn = p_buttons[idx];
     vector<wxColour> wxcolors = btn->getColors();
     if (wxcolors.size() == colors.size()) {
        bool allMatch = true;
        for (int jdx=0; jdx<colors.size(); jdx++) {
           wxColour bcolor = wxcolors[jdx];

           wxColour tmp(colors[jdx].c_str());
           // operator== doesn't seem to work
           if (bcolor.Red() != tmp.Red() || bcolor.Green() != tmp.Green() || bcolor.Blue() != tmp.Blue()) {
              allMatch = false;
              break;
           }
        }
        if (allMatch) {
           matchBtn = idx;
           break;
        }
     }
  }
  setSelection(matchBtn);
}


/**
 * Get the button in the chooser based on index
 */
WxColorSchemeButton * WxColorSchemeChooser::getSchemeButton( int index )
{
  return p_buttons[index];
}


/**
 * Event callback function when a button is selected.
 */
void WxColorSchemeChooser::onSchemeSelected( int index )
{
  setSelection(index);

  // @todo Put event code here to let parent frame knew about this selection
}


/**
 * Get the top level flexible grid sizer.
 */
wxFlexGridSizer * WxColorSchemeChooser::getSizer()
{
  return p_topSizer;
}


/**
 * Reset everything.
 * Buttons will be destroyed.
 */
void WxColorSchemeChooser::reset()
{
  for (int i=0; i<p_buttons.size(); i++) {
    p_topSizer->Remove(0);
  }
  p_buttons.clear();
  p_selection = -1;
  p_topSizer->Layout();
  p_tooltip = "";
}


/**
 * Get a button's colors
 */
vector<wxColour> WxColorSchemeChooser::getColors(int btn) const
{
   return p_buttons[btn]->getColors();
}


/**
 * Set tooltip for a single button in the chooser
 */
void WxColorSchemeChooser::setToolTip( int index, string tooltip )
{
  p_buttons[index]->SetToolTip(tooltip);
}


/**
 * Set tooltip for all the buttons in the chooser
 */
void WxColorSchemeChooser::setToolTip( string tooltip )
{
  p_tooltip = tooltip;
  for(int i=0; i<p_buttons.size(); i++)
    p_buttons[i]->SetToolTip(tooltip);
}
