#ifndef _WXCOLORSCHEMECHOOSER_H_
#define _WXCOLORSCHEMECHOOSER_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxColorSchemeChooser.C"
#endif

#include <vector>
using std::vector;

#include "wxgui/WxColorSchemeButton.H"


/*!
  WxColorSchemeChooser class declaration

  Example for using this class:

  WxColorSchemeChooser * chooser = new WxColorSchemeChooser(3, 2, 2, this);

  chooser->setButtonSize(100, 60);

  vector<string> colors1;
  colors1.push_back("orange");
  colors1.push_back("red");
  colors1.push_back("yellow");
  colors1.push_back("green");
  colors1.push_back("blue");
  chooser->addSchemeButton(colors1);

  vector<wxColour> colors2;
  colors2.push_back("red");
  colors2.push_back("green");
  colors2.push_back("blue");
  chooser->addSchemeButton(colors2);

  vector<string> colors3;
  colors3.push_back("white");
  colors3.push_back("black");
  chooser->addSchemeButton(colors3);

  chooser->addSchemeButton(colors1);

  chooser->getSizer()->SetVGap(20);

  chooser->setSelection(1);

  vertical_sizer->Add(chooser, 0, wxEXPAND, 0);

  When a selection occured, onSchemeSelected get called.
  It is possible to generate an event from that function so that the
  parent frame can get the event.
 */
class WxColorSchemeChooser: public ewxPanel
{
  DECLARE_DYNAMIC_CLASS( WxColorSchemeChooser )
  //  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  WxColorSchemeChooser( );
  WxColorSchemeChooser( int cols,
                        int vgap,
                        int hgap,
                        wxWindow* parent,
                        wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxNO_BORDER|wxTAB_TRAVERSAL );

  virtual void setButtonSize( int width, int height );

  virtual int addSchemeButton( const vector<wxColour> & colors );
  virtual int addSchemeButton( const vector<string> & colors );

  virtual int getSelection();
  virtual void setSelection( int index );
  virtual void setSelection(const vector<string>& colors);
  virtual void onSchemeSelected( int index );

  virtual wxFlexGridSizer * getSizer();
  virtual void reset();
  virtual vector<wxColour> getColors(int btn) const;
  virtual void setToolTip( int index, string tooltip );
  virtual void setToolTip( string tooltip );

  
protected:

  virtual WxColorSchemeButton * getSchemeButton( int index );

  int p_selection;
  int p_butWidth;
  int p_butHeight;
  string p_tooltip;
  vector<WxColorSchemeButton *> p_buttons;
  wxFlexGridSizer * p_topSizer;
};

#endif
