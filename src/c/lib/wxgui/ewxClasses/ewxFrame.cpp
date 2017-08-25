#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Preferences.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxConfig.H"

/**
 * Default Constructor.
 */
ewxFrame::ewxFrame()
  : wxFrame(),
    ewxStyledWindow()
{
}


/**
 * Constructor.
 */
ewxFrame::ewxFrame(wxWindow* parent, wxWindowID id, const wxString& title,
                   const wxPoint& pos, const wxSize& size, long style,
                   const wxString& name)
  : wxFrame(),
    ewxStyledWindow()
{
   Create(parent, id, title, pos, size, style|wxWANTS_CHARS, name);
}


/**
 * Override this set all frames to ECCE via the default argument.
 * Just doing this with the constructor doesn't work because the
 * generated code uses the default constructor which sets the name 
 * to 'frame' up the chain.
 * Using SetName didn't seem to solve the problem - probably it happens
 * too late in the creation process.
 */
bool ewxFrame::Create(wxWindow* parent, wxWindowID id, const wxString& title,
                      const wxPoint& pos, const wxSize& size, long style,
                      const wxString& name)
{
   if (!wxFrame::Create(parent, id, title, pos, size, style, name)) {
     wxFAIL_MSG( wxT("ewxFrame creation failed") );
     return false;
   }

   PushEventHandler(new ewxHelpHandler(this));
   setStyles(this, false);

   return true;
}


ewxFrame::~ewxFrame()
{
  PopEventHandler(true);
}


bool ewxFrame::Show(bool show)
{
   setStyles(this, false);
   
   return (wxFrame::Show(show));
}


void ewxFrame::saveWindowSettings(ewxConfig * config, bool saveSize) 
{
  ewxWindowUtils::saveWindowSettings(this, config, saveSize);
}



void ewxFrame::restoreWindowSettings(ewxConfig * config, bool restoreSize) 
{
  ewxWindowUtils::restoreWindowSettings(this, config, restoreSize);
}



/**
 * @deprecated Should use the ewxConfig version from now on
 */
void ewxFrame::saveWindowSettings(const string& prefix, Preferences & prefs, bool saveSize) 
{
  ewxWindowUtils::saveWindowSettings(this, prefix, prefs, saveSize);
}



/**
 * @deprecated Should use the ewxConfig version from now on
 */
bool ewxFrame::restoreWindowSettings(const string& prefix, Preferences & prefs, bool restoreSize) 
{
   return
     ewxWindowUtils::restoreWindowSettings(this, prefix, prefs, restoreSize);
}


void ewxFrame::setUnitFamily(const string& family)
{
   ewxWindowUtils::setUnitFamily(this, family);
}

void ewxFrame::setFrameCustomDisabledStyle(bool enabled)
{
   ewxWindowUtils::setCustomDisabledStyle(this, enabled);
}

