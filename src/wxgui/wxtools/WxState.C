/**
 * @file
 *
 *
 */
#include <iostream>
using std::cout;
using std::endl;

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxStateGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/brush.h"

#include <iostream>
using namespace std;

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxColor.H"

#include "wxgui/WxState.H"


vector<string>   WxState::p_defaultColors;
vector<wxBrush*> WxState::p_brushes[2];
vector<wxPen*>   WxState::p_pens[2];


/**
 * Constructor.
 * State set to CREATED by default.
 */
WxState::WxState( ) : WxStateGUI()
{
  SetBackgroundColour(GetParent()->GetBackgroundColour());
  initializeColors();
  p_currentState = ResourceDescriptor::STATE_CREATED;
  p_useSystemColor = false;
}



/**
 * Constructor.
 * State set to CREATED by default.
 */
WxState::WxState( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
  : WxStateGUI(parent, id, pos, size, style)
{
  SetBackgroundColour(GetParent()->GetBackgroundColour());
  initializeColors();
  p_currentState = ResourceDescriptor::STATE_CREATED;
  p_useSystemColor = false;
}


WxState::~WxState()
{
}

/**
 * Initialize colors, brushes and pens.  
 * System colors are loaded followed by user colors.  This is only done
 * if not colors have been set yet.
 */
void WxState::initializeColors()
{
  if (p_defaultColors.size() == 0) {

    // Some hardwired defaults just in case
    p_defaultColors.push_back("#FF0000");  // ILLEGAL
    p_defaultColors.push_back("#9999CC");  // CREATED
    p_defaultColors.push_back("#0000FF");  // READY
    p_defaultColors.push_back("#3399CC");  // SUBMITTED
    p_defaultColors.push_back("#33CC33");  // RUNNING
    p_defaultColors.push_back("#FFFFCC");  // COMPLETE
    p_defaultColors.push_back("#3D3D3D");  // KILLED
    p_defaultColors.push_back("#CC3333");  // UNSUCCESSFUL
    p_defaultColors.push_back("#FF0000");  // FAILED
    p_defaultColors.push_back("#FFFFCC");  // LOADED
    p_defaultColors.push_back("#FF0000");  // SYSTEM
    p_defaultColors.push_back("#000000");  // LAST

    createBrushesAndPens();

    resetToSystemDefault();

    resetToUserDefault();
  }
}


/**
 * Force reload of user preference values.
 * Same as resetToUserDefault() with clearer name.
 * Need to call this in response to preference change event.
 * Caller must call refresh.
 */
void WxState::resetUserPreferences()
{
  resetToUserDefault();
}



void WxState::createBrushesAndPens()
{
  // Create two sets of pens and brushes
  // pens[0] using user colors and pens[1] using system colors
  for (size_t i=0; i<2; i++) {
    for (size_t idx=0; idx<p_defaultColors.size(); idx++) {
      if (idx == ResourceDescriptor::STATE_LOADED) {
        p_brushes[i].push_back(new wxBrush(ewxColor(p_defaultColors[idx]),
                                                    wxTRANSPARENT));
        p_pens[i].push_back(new wxPen(ewxColor(p_defaultColors[idx])));
      } else {
        p_brushes[i].push_back(new wxBrush(ewxColor(p_defaultColors[idx])));
        p_pens[i].push_back(new wxPen(ewxColor(p_defaultColors[idx])));
      }
    }
  }
}


/**
 * Clean up the static brushes and pens that we cache.
 */
void WxState::finalize()
{
  int idx;
  int i;
  for (i=0; i<2; i++) {
    for (idx=p_brushes[i].size()-1; idx>=0; idx--) {
      delete p_brushes[i][idx];
      delete p_pens[i][idx];
    }
    p_brushes[i].clear();
    p_pens[i].clear();
  }
}


/**
 * Special verion to set property from dialogblocks generated code.
 */
void WxState::setRunState(int state)
{
  setRunState((ResourceDescriptor::RUNSTATE) state);
}


/**
 * Change the current RUNSTATE.  
 * The widget is automatically refreshed.
 */
void WxState::setRunState(ResourceDescriptor::RUNSTATE state, bool useSystemColor)
{
  p_currentState = state;
  p_useSystemColor = useSystemColor;
  refresh();
}


/**
 * Get the current ResourceDescriptor::RUNSTATE of this instance.
 */
ResourceDescriptor::RUNSTATE WxState::getRunState() const
{
  return p_currentState;
}



/**
 * Redraw the state symbol using the objects current state.
 * The shapes are predefined but colors are user settable.
 *
 * When drawn, the widget is first cleared to its background color
 * which is controlled by default, by the ewxStyledWindow class but
 * can be overridden with a call to SetBackgroundColour().
 */
void WxState::refresh()
{
  wxPaintDC dc(this);
  dc.SetBackground(GetBackgroundColour());
  draw(dc, p_currentState, p_useSystemColor);
}


/**
 * Real drawing function which draw state icons to wxPaintDC passed in.
 *
 * The reason to have this static function is that the wxGrid in CalcMgr
 * won't take WxState which is essentially a wxPanel. On the other hand
 * wxDC is allowed to be passed in and drawn the state icon on it.
 */
void WxState::draw(wxDC & dc, ResourceDescriptor::RUNSTATE currentState,
                   bool useSystemColor, const wxRect& rect)
{
  initializeColors();

  int x, y, width, height;
  if (rect.IsEmpty()) {
    x = 0;
    y = 0;
    dc.GetSize(&width, &height);
  } else {
    x = rect.GetX();
    y = rect.GetY();
    width = rect.GetWidth();
    height = rect.GetHeight();
  }
  
  // Clear existing drawing within the given rectangle (or dc)
  dc.SetBrush(dc.GetBackground().GetColour());
  dc.SetPen(dc.GetBackground().GetColour());
  dc.DrawRectangle(x, y, width, height);

  // Now set the brush and pen for the current state
  dc.SetBrush( *p_brushes[useSystemColor?1:0][currentState]);
  dc.SetPen( *p_pens[useSystemColor?1:0][currentState]);

  switch (currentState) {
  case ResourceDescriptor::STATE_CREATED:
  case ResourceDescriptor::STATE_READY:
    {
      wxPoint tri[3] = {
              wxPoint(x, height + y), 
			        wxPoint(width + x, height + y), 
			        wxPoint(width/2 + x, y)
      };
      dc.DrawPolygon(3, tri);
      break;
    }
  case ResourceDescriptor::STATE_SUBMITTED:
  case ResourceDescriptor::STATE_RUNNING:
    dc.DrawCircle(width/2 + x, height/2 + y,
                  width < height ? width/2 : height/2);
    break;
  case ResourceDescriptor::STATE_COMPLETED:
  case ResourceDescriptor::STATE_LOADED:
    dc.DrawRectangle(x, y, width, height);
    break;
  case ResourceDescriptor::STATE_UNSUCCESSFUL:
  case ResourceDescriptor::STATE_FAILED:
  case ResourceDescriptor::STATE_SYSTEM_FAILURE:
    {
      float offset = width*0.15;
      wxPoint diamond[4] = {
              wxPoint(width/2 + x, y), 
              wxPoint((int)(width-offset/2.0) + x, height/2 + y),
              wxPoint(width/2 + x, height-(int)(offset/2.0)+y), 
              wxPoint((int)(offset/2.0) + x, height/2 + y)
      };
      dc.DrawPolygon(4, diamond);
      break;
    }
  case ResourceDescriptor::STATE_KILLED:
    {
      float offset = width*0.15;
      wxPoint coffin[6] = {
              wxPoint(width/2 - 2 + x, y),
              wxPoint(width/2 + 2 + x, y),
              wxPoint((int)(width-offset/2.0) + x, (int)(height/3.0) + y),
              wxPoint((int)(width/2.0 + 2.0) + x, height + y), 
              wxPoint((int)(width/2.0 - 2.0) + x, height + y),
              wxPoint((int)(offset/2.0) + x, (int)(height/3.0) + y)
      };
      dc.DrawPolygon(6, coffin);
      break;
    }
  case ResourceDescriptor::STATE_ILLEGAL:
    // a stop sign
    dc.DrawRoundedRectangle(0, 0, width, height, 5);
  default:
    break;
  }
}


/**
 * Reset all state colors to the Ecce default colors.
 * NOTE!!!! This involves reloading the system pref file for each instance 
 * of this class.
 *
 * Caller must call refresh to update display of individual instances.
 */
void WxState::resetToSystemDefault()
{
  Preferences sysPrefs(PrefLabels::GLOBALPREFFILE, true);
  resetFromPreferences(sysPrefs);

  // Set system default color brushes and pens, do it for once
  for (size_t idx=0; idx<p_defaultColors.size(); idx++) {
    p_brushes[1][idx]->SetColour(ewxColor(p_defaultColors[idx]));
    p_pens[1][idx]->SetColour(ewxColor(p_defaultColors[idx]));
  }

}


/**
 * Reset all state colors to the user preferred colors.
 * NOTE!!!! This involves reloading the system pref file for each instance 
 * of this class.
 *
 * Caller must call refresh to update display of individual instances.
 */
void WxState::resetToUserDefault()
{
  Preferences sysPrefs(PrefLabels::GLOBALPREFFILE);
  resetFromPreferences(sysPrefs);
}



/**
 * Reloads the user state color strings and recreates the brushes and pens.
 */
void WxState::resetFromPreferences(const Preferences& sysPrefs)
{
  int i = ResourceDescriptor::STATE_CREATED;
  for (; i < ResourceDescriptor::NUMBER_OF_STATES; i++) {
    sysPrefs.getString(getPrefString(i), p_defaultColors[i]);
    p_brushes[0][i]->SetColour(ewxColor(p_defaultColors[i]));
    p_pens[0][i]->SetColour(ewxColor(p_defaultColors[i]));
  }
}



/**
 * Respond to a paint event - redraw the symbol.
 */
void WxState::OnPaint( wxPaintEvent& event )
{
  refresh();
}


bool WxState::canBeReset()
{
  return p_pens[0][p_currentState]->GetColour() != 
    p_pens[1][p_currentState]->GetColour();
}


string WxState::getPrefString()
{
  return getPrefString(p_currentState);
}


string WxState::getName(ResourceDescriptor::RUNSTATE state, bool capital)
{
  switch (state) {
  case ResourceDescriptor::STATE_CREATED:
    return (capital?"Created":"created");
  case ResourceDescriptor::STATE_READY:
    return (capital?"Ready":"ready");
  case ResourceDescriptor::STATE_SUBMITTED:
    return (capital?"Submitted":"submitted");
  case ResourceDescriptor::STATE_RUNNING:
    return (capital?"Running":"running");
  case ResourceDescriptor::STATE_COMPLETED:
    return (capital?"Completed":"completed");
  case ResourceDescriptor::STATE_LOADED:
    return (capital?"Imported":"imported");
  case ResourceDescriptor::STATE_KILLED:
    return (capital?"Killed":"killed");
  case ResourceDescriptor::STATE_UNSUCCESSFUL:
    return (capital?"Incomplete":"incomplete");
  case ResourceDescriptor::STATE_FAILED:
    return (capital?"Monitor Error":"monitor error");
  case ResourceDescriptor::STATE_SYSTEM_FAILURE:
    return (capital?"System Error":"system error");
  case ResourceDescriptor::STATE_ILLEGAL:
  case ResourceDescriptor::NUMBER_OF_STATES:
    return (capital?"Illegal State":"illegal state");
  }
  return "";
}


string WxState::getName(bool capital)
{
  return getName(p_currentState, capital);
}


string WxState::getPrefString(int state)
{
  switch (state) {
  case ResourceDescriptor::STATE_CREATED:
    return PrefLabels::stateCreated;
  case ResourceDescriptor::STATE_READY:
    return PrefLabels::stateReady;
  case ResourceDescriptor::STATE_SUBMITTED:
    return PrefLabels::stateSubmitted;
  case ResourceDescriptor::STATE_RUNNING:
    return PrefLabels::stateRunning;
  case ResourceDescriptor::STATE_COMPLETED:
    return PrefLabels::stateCompleted;
  case ResourceDescriptor::STATE_LOADED:
    return PrefLabels::stateLoaded;
  case ResourceDescriptor::STATE_FAILED:
    return PrefLabels::stateFailed;
  case ResourceDescriptor::STATE_KILLED:
    return PrefLabels::stateKilled;
  case ResourceDescriptor::STATE_UNSUCCESSFUL:
    return PrefLabels::stateUnsuccessful;
  case ResourceDescriptor::STATE_SYSTEM_FAILURE:
    return PrefLabels::stateSystem;
  }
  return "";
}


string WxState::getColorString()
{
  return p_defaultColors[p_currentState];
}

