/**
 * @file
 *
 *
 */
#include <iostream>

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/confbase.h"

#if defined(__WXGTK__)
extern "C" {
  #include <gtk/gtk.h>
  #include <gdk/gdk.h>
  #ifdef GDK_WINDOWING_X11
    #include <gdk/gdkx.h>
  #endif
}
#endif
#include <cstdlib>
#include <cstdio>

#include "wxgui/ewxBitmap.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/UnitFactory.H"
#include "util/UnitConverter.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxScrolledWindow.H"
#include "wx/display.h"
#include "wx/statline.h"
#include "wx/glcanvas.h"
#include "wx/aui/framemanager.h"
#include <vector>
#include <map>
#include <algorithm>
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/NumericValidatorBase.H"
#include "wxgui/WxFeedback.H"


void ewxRaiseWindow(wxWindow *win)
{
  if (win == NULL) return;
  const char *how = "wxWindow::Raise";
#if defined(__WXGTK__)
  GtkWidget *gw = (GtkWidget*)win->GetHandle();
  GdkWindow *gdkw = gw ? gtk_widget_get_window(gw) : NULL;
  if (gdkw != NULL && GTK_IS_WINDOW(gw)) {
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_window_get_display(gdkw))) {
      gtk_window_present_with_time(GTK_WINDOW(gw),
                                   gdk_x11_get_server_time(gdkw));
      how = "gtk_window_present_with_time";
    } else
#endif
    {
      gtk_window_present(GTK_WINDOW(gw));
      how = "gtk_window_present";
    }
  } else {
    win->Raise();
  }
#else
  win->Raise();
#endif
  if (getenv("ECCE_DEBUG_RAISE"))
    fprintf(stderr, "ewxRaiseWindow: %s\n", how);
}


/**
 * For window win, save the position and optionally the size of the
 * window in the config file.
 * No checking is done to ensure that the window is a shell.
 */
bool ewxWindowUtils::setToolIcon(wxTopLevelWindow *win,
                                 const std::string& toolName)
{
  if (win == (wxTopLevelWindow*)0) return false;

  ResourceDescriptor& descriptor =
      ResourceDescriptor::getResourceDescriptor();
  ResourceTool *tool = descriptor.getTool(toolName);
  if (tool == (ResourceTool*)0) return false;

  std::string icon = tool->getIcon();
  if (icon.empty()) return false;

  win->SetIcon(ewxBitmap::icon(icon));
  return true;
}


void ewxWindowUtils::saveWindowSettings(wxWindow *win,
                                        ewxConfig * config,
                                        bool saveSize)
{
  int x=0, y=0;
  win->GetPosition(&x, &y);

  config->Write("/Window/X", x);
  config->Write("/Window/Y", y);

  if (saveSize) {
    int width=0, height=0;
    win->GetSize(&width, &height);
    config->Write("/Window/Height", height);
    config->Write("/Window/Width", width);
  }
  config->Flush();
}


/**
 * For window win, restore the position and optionally the size of the
 * window in the config file.
 * No checking is done to ensure that the window is a shell.
 */
void ewxWindowUtils::restoreWindowSettings(wxWindow *win,
                                           ewxConfig *config,
                                           bool restoreSize)
{
  int x, y;

  config->Read("/Window/X", &x, wxDefaultCoord);
  config->Read("/Window/Y", &y, wxDefaultCoord);

  if (restoreSize) {
    int width=wxDefaultCoord;
    int height=wxDefaultCoord;
    config->Read("/Window/Height", &height, wxDefaultCoord);
    config->Read("/Window/Width", &width, wxDefaultCoord);
    win->SetSize(x, y, width, height, wxSIZE_AUTO);
  } else {
    // GitHub #67: passing -1/wxDefaultCoord for width/height here does
    // NOT mean "leave the size alone" under the default wxSIZE_AUTO
    // flag -- it means "recompute via best-size", which re-triggers
    // Layout() on whatever sizer the caller just built in its own
    // constructor (e.g. Gateway::CreateControls()'s toolbar sizer).
    // On at least one real dual-monitor GTK3 setup that reentrant
    // Layout()/DoSetSize pass computes an invalid (<=0) height and GTK
    // asserts ("gtk_window_resize: assertion 'height > 0' failed"),
    // leaving the window's real height clobbered even though the
    // caller had already set a correct one. When we're not restoring
    // size, don't touch it at all -- just reposition.
    win->Move(x, y);
  }
}


/**
 * @deprecated Should use the ewxConfig version from now on
 * For window win, save the position and optionally the size of the
 * window in the preference file.
 * No checking is done to ensure that the window is a shell.
 */
void ewxWindowUtils::saveWindowSettings(wxWindow *win, 
                                         const string& prefix, 
                                         Preferences & prefs, 
                                         bool saveSize)
{
  int x=0, y=0;
  win->GetPosition(& x, & y);

  prefs.setInt(prefix+".X", x);
  prefs.setInt(prefix+".Y", y);

  if (saveSize) {
    int width=0, height=0;
    win->GetSize(& width, & height);
    prefs.setInt(prefix+".Height",height);
    prefs.setInt(prefix+".Width",width);
  }
  prefs.saveFile();
}


/**
 * @deprecated Should use the ewxConfig version from now on
 * For window win, restore the position and optionally the size of the
 * window in the preference file.
 * No checking is done to ensure that the window is a shell.
 */
bool ewxWindowUtils::restoreWindowSettings(wxWindow *win,
                                      const string& prefix, 
                                      Preferences & prefs, 
                                      bool restoreSize)
{
   bool ret = true;

   int x=-1 ,y=-1;

   prefs.getInt(prefix+".X",x);
   prefs.getInt(prefix+".Y",y);
   if (x < 0)
      x = -1;
   if (y < 0)
      y = -1;

   if (restoreSize) {
      int width=-1 ,height=-1;
      prefs.getInt(prefix+".Height",height);
      prefs.getInt(prefix+".Width",width);
      if (height < 0)
         height = -1;
      if (width < 0)
         width = -1;
      win->SetSize(x, y, width, height);
   } else {
      // GitHub #67: see the ewxConfig overload above for the full
      // explanation -- SetSize(x, y, -1, -1) is not a no-op for size
      // under wx3.2/GTK3's default wxSIZE_AUTO, it recomputes and can
      // clobber the caller's already-correct size via a reentrant
      // Layout() pass. Reposition only.
      win->Move(x, y);
   }
   return ret;
}


/**
 * Finds the top window for 'win'
 */
wxWindow *ewxWindowUtils::getTopWindow(wxWindow *win)
{
   wxWindow *ret = 0;
   wxWindow *parent = win->GetParent();
   if (win->IsTopLevel() || parent == 0) {
     ret = win;
   } else {
     ret = ewxWindowUtils::getTopWindow(parent);
   }
   return ret;
}


/**
 * Recursively find all text controls that have ewxUnitHelper objects assigned
 * and change the unit family.
 */
void ewxWindowUtils::setUnitFamily(wxWindow *top, const string& family)
{
   wxWindowList children = top->GetChildren();
   wxWindowList::compatibility_iterator node = children.GetFirst();
   UnitFactory& uf = UnitFactory::getInstance();

   while (node) {
      wxWindow *child = (wxWindow*)node->GetData();
      ewxTextCtrl *text = dynamic_cast<ewxTextCtrl*>(child);
      if (text) {
         ewxUnitHelper *helper = text->getUnitHelper();
         NumericValidatorBase *validator =
                dynamic_cast<NumericValidatorBase*>(text->GetValidator());
         if (helper) {
            // Get UnitFamily for our new family
            if (family != helper->getFamily()) {
               UnitFamily& newFamily = uf.getUnitFamily(family);
               UnitFamily& curFamily = uf.getUnitFamily(helper->getFamily());
               string to = newFamily.get(helper->getUnitClass());
               string from = curFamily.get(helper->getUnitClass());

               // Get converter & convert
               UnitConverter& uconv = uf.getUnitConverter(helper->getUnitClass());
               uconv.setBaseUnits(from);
               string value = uconv.convertTo(text->GetValue().ToStdString(), to);
               text->SetValue(value.c_str());

               // Now do range limits
               wxValidator *v = text->GetValidator();
               if (v) {
                  ewxNumericValidator *nv = 
                       dynamic_cast<ewxNumericValidator*>(v);
                  if (nv) {
                     double min, max;
                     if (nv->isUsingHardRange()) {
                        nv->getHardRange(min,max);
                        min = uconv.convertTo(min, to);
                        max = uconv.convertTo(max, to);
                        if (nv->isIntRange()) {
                           nv->setHardRange(static_cast<int>(min),
                                            static_cast<int>(max));
                        } else {
                           nv->setHardRange(min, max);
                        }
                     }
                     if (nv->isUsingSoftRange()) {
                        nv->getSoftRange(min,max);
                        min = uconv.convertTo(min, to);
                        max = uconv.convertTo(max, to);
                        if (nv->isIntRange()) {
                           nv->setSoftRange(static_cast<int>(min),
                                            static_cast<int>(max));
                        } else {
                           nv->setSoftRange(min, max);
                        }
                     }
                  }
               }
               // set labe if any
               ewxStaticText *label = helper->getLabel();
               if (label) {
                  label->SetLabel(to.c_str());
               }

               // Finally update to the new family
               helper->setFamily(family);
            }
         } else if (validator) {
           validator->SetFamily(family);
         }
      } else {
         // Recursive call to handle children
         // TODO only for selective widgets
         setUnitFamily(child, family);
      }
      // Calling layout on EVERY child is the only way I could get the
      // sizing to work right.  I tried to call it just on the unit label and
      // the text field.
      child->Layout();

      node = node->GetNext();
   }


}



/**
 * Load global preferences and update accordingly.
 * This method will
 *   <li>find ID_FEEDBACK  and set beep preferences</li>
 *   <li>call setUnitFamily for TaskApp implementations.  This works only
 *   if the top frame can be cast to a TaskApp</li>
 */
void ewxWindowUtils::processGlobalPreferenceChange(wxWindow *top)
{
   string unitFamily = DEFAULT_UNIT_FAMILY;

   Preferences * eccePref = Preferences::getGlobalPref(true);
   eccePref->getString(PrefLabels::UNITFAMILY, unitFamily);
   ewxWindowUtils::setUnitFamily(top, unitFamily);
}


/**
 *  Displays a wxMessageDialog to confirm that the user would like to exit
 *  an application or tool.  Returns true if confirmed; false otherwise.
 *
 *  K.Swanson  2005 October 11
 */
bool ewxWindowUtils::confirmExit(wxWindow *owner)
{
    ewxMessageDialog *dlg;
    string prompt, title;
    long style;
    int result;

    prompt = "Do you really want to exit?";
    title = "Confirm Exit";
    style = wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT;

    dlg = new ewxMessageDialog(owner, prompt, title, style);
    result = dlg->ShowModal();

    delete dlg;

    return (result == wxID_YES);
}


void ewxWindowUtils::setCustomDisabledStyle(wxWindow *win, bool enabled)
{
   // Don't process feedback windows - never disable
   // This may be a bad way to do this since a simple ewx class
   // has to know about WxFeedback
   WxFeedback *feedback = dynamic_cast<WxFeedback*>(win);
   if (feedback) {
      return;
   }

   ewxStyledWindow *swin = dynamic_cast<ewxStyledWindow*>(win);
   if (swin) {
      swin->setCustomDisabledStyle(enabled);
   } 

   wxWindowList children = win->GetChildren();
   wxWindowList::compatibility_iterator node = children.GetFirst();

   while (node) {
      wxWindow *child = (wxWindow*)node->GetData();

      // Recursive call to handle children
      ewxWindowUtils::setCustomDisabledStyle(child, enabled);

      node = node->GetNext();
   }


}




//  ---- small screens (#189) ------------------------------------------------

namespace {

//  What a window manager adds above the client area and a panel may take
//  below it.  The decoration cannot be told from the menu bar in
//  GetSize() - GetClientSize(), nor measured before the window is mapped, so
//  this is always reserved on top of what is measured.
const int TITLE_RESERVE = 36;

wxRect displayArea(wxWindow *win)
{
  int idx = wxDisplay::GetFromWindow(win);
  return wxDisplay((unsigned)(idx == wxNOT_FOUND ? 0 : idx)).GetClientArea();
}


void collectWindows(wxSizer *sizer, std::vector<wxWindow*>& out)
{
  for (wxSizerItemList::compatibility_iterator n =
         sizer->GetChildren().GetFirst(); n; n = n->GetNext()) {
    wxSizerItem *item = n->GetData();
    if (item->IsWindow()) {
      out.push_back(item->GetWindow());
    } else if (item->IsSizer()) {
      wxSizer *child = item->GetSizer();
      wxStaticBoxSizer *box = dynamic_cast<wxStaticBoxSizer*>(child);
      if (box != NULL)
        out.push_back(box->GetStaticBox());
      collectWindows(child, out);
    }
  }
}


bool isButtonRow(wxSizerItem *item)
{
  std::vector<wxWindow*> windows;
  if (item->IsWindow())
    windows.push_back(item->GetWindow());
  else if (item->IsSizer())
    collectWindows(item->GetSizer(), windows);

  bool buttons = false;
  for (size_t i = 0; i < windows.size(); i++) {
    if (dynamic_cast<wxAnyButton*>(windows[i]) != NULL)
      buttons = true;
    else if (dynamic_cast<wxStaticLine*>(windows[i]) == NULL &&
             dynamic_cast<wxStaticText*>(windows[i]) == NULL)
      return false;
  }
  return buttons;
}


//  A vertical sizer's closing row of buttons, with the few things the app
//  puts under it (a rule, a status line), as a sizer of their own.  NULL
//  when there is no such row; the content is then scrolled whole.
wxSizer *detachButtonTail(wxSizer *sizer)
{
  wxBoxSizer *box = dynamic_cast<wxBoxSizer*>(sizer);
  if (box == NULL || box->GetOrientation() != wxVERTICAL)
    return NULL;

  std::vector<wxSizerItem*> items;
  for (wxSizerItemList::compatibility_iterator n =
         box->GetChildren().GetFirst(); n; n = n->GetNext())
    items.push_back(n->GetData());

  const size_t TAIL = 4;
  size_t first = items.size();
  for (size_t i = items.size(); i-- > 0 && items.size() - i <= TAIL; ) {
    if (isButtonRow(items[i])) {
      first = i;
      break;
    }
  }
  //  Never the whole content: something has to scroll.
  if (first >= items.size() || first == 0)
    return NULL;

  wxBoxSizer *tail = new wxBoxSizer(wxVERTICAL);
  for (size_t i = first; i < items.size(); i++) {
    wxSizerItem *it = items[i];
    if (it->IsWindow()) {
      wxWindow *w = it->GetWindow();
      int prop = it->GetProportion(), flag = it->GetFlag(), border = it->GetBorder();
      box->Detach(w);
      tail->Add(w, prop, flag, border);
    } else if (it->IsSizer()) {
      wxSizer *c = it->GetSizer();
      int prop = it->GetProportion(), flag = it->GetFlag(), border = it->GetBorder();
      box->Detach(c);
      tail->Add(c, prop, flag, border);
    } else {
      wxSize sz = it->GetSize();
      box->Detach((int)first);
      tail->Add(sz.x, sz.y);
    }
  }
  return tail;
}


//  Scrolls the window's content.  Its natural size is read from the content
//  sizer (a scrolled window's own best size is the size it was created
//  with) and clamped to what is left of the display above the fixed row.
class FitScroller : public ewxScrolledWindow
{
public:
  FitScroller(wxTopLevelWindow *top)
    : ewxScrolledWindow(top, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                        wxHSCROLL|wxVSCROLL|wxNO_BORDER|wxTAB_TRAVERSAL),
      p_top(top), p_row(NULL)
  {
    SetScrollRate(10, 10);
  }

  void setRow(wxSizer *row) { p_row = row; }

protected:
  virtual wxSize DoGetBestSize() const
  {
    wxSize want = GetSizer() ? GetSizer()->GetMinSize() : wxSize(0, 0);
    wxSize cap = ewxWindowUtils::clientCapForDisplay(p_top);
    cap.y -= p_row ? p_row->GetMinSize().y : 0;

    const int bar = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    wxSize best = want;
    if (want.y > cap.y) {
      best.y = cap.y;
      best.x += bar;
    }
    if (best.x > cap.x) {
      best.x = cap.x;
      if (want.y <= cap.y)
        best.y = wxMin(want.y + bar, cap.y);
    }
    return best;
  }

private:
  wxTopLevelWindow *p_top;
  wxSizer *p_row;
};


FitScroller *findScroller(wxWindow *win)
{
  wxWindowList& kids = win->GetChildren();
  for (wxWindowList::compatibility_iterator n = kids.GetFirst(); n;
       n = n->GetNext()) {
    FitScroller *s = dynamic_cast<FitScroller*>(n->GetData());
    if (s != NULL)
      return s;
  }
  return NULL;
}


bool holdsGLCanvas(wxWindow *win)
{
  if (dynamic_cast<wxGLCanvas*>(win) != NULL)
    return true;
  wxWindowList& kids = win->GetChildren();
  for (wxWindowList::compatibility_iterator n = kids.GetFirst(); n;
       n = n->GetNext()) {
    if (!n->GetData()->IsTopLevel() && holdsGLCanvas(n->GetData()))
      return true;
  }
  return false;
}


//  A GL canvas must not be scrolled: macOS aborts when a GL surface lies
//  outside what the window allows, and a scroller sizes its content to its
//  virtual size.  An AUI frame's sizer is AUI's own, rebuilt on every
//  Update(), and its minimum size is no natural size.  Such a window is
//  only capped, and its sizer shrinks the canvas.
bool mustNotScroll(wxTopLevelWindow *win)
{
  return wxAuiManager::GetManager(win) != NULL || holdsGLCanvas(win);
}


//  Move the window's content, and the windows its sizer manages, into a
//  scroller; the fixed row's windows stay in the window.
FitScroller *wrapContent(wxTopLevelWindow *win, wxSizer *content,
                         wxSizer *fixedRow)
{
  std::vector<wxWindow*> keep;
  if (fixedRow != NULL)
    collectWindows(fixedRow, keep);

  std::vector<wxWindow*> move;
  collectWindows(content, move);

  FitScroller *scroller = new FitScroller(win);
  scroller->setRow(fixedRow);
  win->SetSizer(NULL, false);

  for (size_t i = 0; i < move.size(); i++) {
    if (move[i]->GetParent() == win &&
        std::find(keep.begin(), keep.end(), move[i]) == keep.end())
      move[i]->Reparent(scroller);
  }
  scroller->SetSizer(content);

  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  //  Proportion 1 for the part that may shrink: a proportion-0 item would
  //  keep its whole natural height whatever the window offers (#187).
  outer->Add(scroller, 1, wxEXPAND);
  if (fixedRow != NULL)
    outer->Add(fixedRow, 0, wxEXPAND);
  win->SetSizer(outer);
  return scroller;
}


//  A window can leave the display after it is shown: its content is loaded,
//  the toolkit places it, a preference restores a size.  Every size change
//  is checked, once the event is over, and put right.
struct Watch
{
  bool pending;
  int tries;
  Watch() : pending(false), tries(0) {}
};

std::map<wxTopLevelWindow*, Watch> g_watched;

bool outsideDisplay(wxTopLevelWindow *win)
{
  if (!win->IsShown() || win->IsMaximized() || win->IsFullScreen())
    return false;

  wxSize cap = ewxWindowUtils::clientCapForDisplay(win);
  wxSize client = win->GetClientSize();
  if (client.x > cap.x || client.y > cap.y)
    return true;

  wxRect area = displayArea(win);
  wxPoint pos = win->GetPosition();
  wxSize size = win->GetSize();
  return pos.x < area.x || pos.y < area.y ||
         pos.x + size.x > area.x + area.width ||
         pos.y + size.y + TITLE_RESERVE > area.y + area.height;
}


void watch(wxTopLevelWindow *win)
{
  if (g_watched.find(win) != g_watched.end())
    return;
  g_watched[win] = Watch();

  win->Bind(wxEVT_DESTROY, [win](wxWindowDestroyEvent& e) {
    if (e.GetEventObject() == win)
      g_watched.erase(win);
    e.Skip();
  });
  win->Bind(wxEVT_SIZE, [win](wxSizeEvent& e) {
    e.Skip();
    std::map<wxTopLevelWindow*, Watch>::iterator it = g_watched.find(win);
    if (it == g_watched.end() || it->second.pending)
      return;
    if (!outsideDisplay(win)) {
      it->second.tries = 0;
      return;
    }
    //  A window that will not obey (a minimum size larger than the display)
    //  must not be chased for ever.
    if (it->second.tries >= 3)
      return;
    it->second.pending = true;
    it->second.tries++;
    win->CallAfter([win]() {
      std::map<wxTopLevelWindow*, Watch>::iterator w = g_watched.find(win);
      if (w == g_watched.end())
        return;
      w->second.pending = false;
      ewxWindowUtils::fitToDisplay(win);
    });
  });
}

} // namespace


wxSize ewxWindowUtils::clientCapForDisplay(wxTopLevelWindow *win)
{
  wxRect area = displayArea(win);
  wxSize frame = win->GetSize() - win->GetClientSize();
  wxSize cap(area.width - frame.x,
             area.height - frame.y - TITLE_RESERVE);
  if (cap.x <= 0)
    cap.x = area.width;
  if (cap.y <= 0)
    cap.y = area.height;
  return cap;
}


void ewxWindowUtils::keepOnDisplay(wxTopLevelWindow *win)
{
  wxRect area = displayArea(win);
  wxPoint pos = win->GetPosition();
  wxSize size = win->GetSize();
  size.y += TITLE_RESERVE;

  int x = wxMax(area.x, wxMin(pos.x, area.x + area.width - size.x));
  int y = wxMax(area.y, wxMin(pos.y, area.y + area.height - size.y));
  if (x != pos.x || y != pos.y)
    win->Move(x, y);
}


void ewxWindowUtils::fitToDisplay(wxTopLevelWindow *win, wxSizer *fixedRow)
{
  if (win == NULL)
    return;

  watch(win);
  wxSize cap = clientCapForDisplay(win);
  FitScroller *scroller = findScroller(win);
  wxSizer *sizer = win->GetSizer();

  //  A minimum size the app set for a roomier screen is the content's
  //  natural size, and would keep the window off the display whatever is
  //  asked of it.
  wxSize least = win->GetMinClientSize();

  if (scroller == NULL && sizer != NULL && !mustNotScroll(win)) {
    wxSize natural = sizer->GetMinSize();
    natural.x = wxMax(natural.x, least.x);
    natural.y = wxMax(natural.y, least.y);
    if (natural.x > cap.x || natural.y > cap.y) {
      //  The content sizer is detached from the window here, so what it
      //  holds is split before it is wrapped.
      if (fixedRow == NULL)
        fixedRow = detachButtonTail(sizer);
      else
        sizer->Detach(fixedRow);
      sizer->SetMinSize(wxSize(
        least.x, wxMax(0, least.y - (fixedRow ? fixedRow->GetMinSize().y : 0))));
      scroller = wrapContent(win, sizer, fixedRow);
    }
  }

  if (least.x > cap.x || least.y > cap.y)
    win->SetMinClientSize(wxSize(least.x > cap.x ? cap.x : least.x,
                                 least.y > cap.y ? cap.y : least.y));

  wxSize client = win->GetClientSize();
  wxSize target(wxMin(client.x, cap.x), wxMin(client.y, cap.y));
  if (scroller != NULL) {
    scroller->InvalidateBestSize();
    win->GetSizer()->Layout();
    wxSize fit = win->GetSizer()->GetMinSize();
    win->SetMinClientSize(fit);
    target = wxSize(wxMin(wxMax(client.x, fit.x), cap.x),
                    wxMin(wxMax(client.y, fit.y), cap.y));
  }
  if (target != client)
    win->SetClientSize(target);

  win->Layout();
  if (scroller != NULL)
    scroller->FitInside();
  keepOnDisplay(win);
}


//  ---- clipping audit -------------------------------------------------

#include "wx/tglbtn.h"
#include "wx/spinctrl.h"
#include "wx/combobox.h"
#include "wx/combo.h"
#include "wx/choice.h"
#include "wx/radiobox.h"
#include "wx/statbox.h"
#include "wx/scrolwin.h"
#include "wx/textctrl.h"
#include "wx/button.h"
#include "wx/weakref.h"
#include "wx/timer.h"
#include "wx/image.h"
#include <set>

namespace {

//  Windows whose children are laid out by the window itself, or that
//  scroll: what lies outside them is not a clipped control.
bool scrollsOrIsCustom(wxWindow *w)
{
  return w->IsKindOf(wxCLASSINFO(wxScrolledWindow)) ||
         w->IsKindOf(wxCLASSINFO(wxScrolledCanvas)) ||
         w->IsKindOf(wxCLASSINFO(wxGLCanvas)) ||
         (w->GetClassInfo()->GetClassName() == wxString("wxGrid") ||
          w->GetClassInfo()->GetClassName() == wxString("wxListCtrl") ||
          w->GetClassInfo()->GetClassName() == wxString("wxTreeCtrl") ||
          w->GetClassInfo()->GetClassName() == wxString("wxHtmlWindow") ||
          w->GetClassInfo()->GetClassName() == wxString("wxGenericTreeCtrl") ||
          w->GetClassInfo()->GetClassName() == wxString("wxDataViewCtrl"));
}

wxRect screenRect(wxWindow *w)
{
  return wxRect(w->GetScreenPosition(), w->GetSize());
}

wxRect clientScreenRect(wxWindow *w)
{
  return wxRect(w->ClientToScreen(wxPoint(0, 0)), w->GetClientSize());
}

std::string describeWindow(wxWindow *w)
{
  wxString text = w->GetLabel();
  if (text.empty()) {
    wxTextCtrl *t = wxDynamicCast(w, wxTextCtrl);
    if (t) text = t->GetValue();
  }
  if (text.empty()) text = w->GetToolTipText();
  if (text.length() > 40) text = text.Left(40) + "...";
  text.Replace("\t", " ");
  text.Replace("\n", " ");
  wxString out = w->GetClassInfo()->GetClassName();
  if (!text.empty()) out << " \"" << text << "\"";
  wxWindow *p = w->GetParent();
  //  The nearest ancestor with something to call it by.
  for (int up = 0; p && up < 4; ++up, p = p->GetParent()) {
    wxString name = p->GetLabel();
    if (name.empty()) name = p->GetName();
    if (!name.empty() && name != "panel" && name != "frame" &&
        name != "dialog") {
      if (name.length() > 30) name = name.Left(30);
      name.Replace("\t", " ");
      name.Replace("\n", " ");
      out << " in " << p->GetClassInfo()->GetClassName() << " \"" << name
          << "\"";
      break;
    }
  }
  return out.ToStdString();
}

//  True if the bitmap draws something: any pixel with alpha, or, without
//  an alpha channel, any pixel unlike the corner.
bool bitmapShowsAnything(const wxBitmap& bmp)
{
  if (!bmp.IsOk()) return false;
  wxImage img = bmp.ConvertToImage();
  if (!img.IsOk()) return false;
  const int w = img.GetWidth(), h = img.GetHeight();
  const unsigned char *rgb = img.GetData();
  if (img.HasAlpha()) {
    const unsigned char *a = img.GetAlpha();
    for (int i = 0; i < w*h; ++i) if (a[i] > 32) return true;
    return false;
  }
  for (int i = 1; i < w*h; ++i) {
    if (abs(rgb[3*i] - rgb[0]) + abs(rgb[3*i+1] - rgb[1]) +
        abs(rgb[3*i+2] - rgb[2]) > 40) return true;
  }
  return false;
}

void auditWindow(wxWindow *w, wxWindow *top, std::vector<std::string>& out)
{
  auto add = [&](const char *kind, const std::string& detail) {
    wxRect r = screenRect(w);
    out.push_back(std::string(kind) + "\t" + describeWindow(w) + "\t" +
                  detail + "\t" +
                  wxString::Format("%d,%d,%d,%d", r.x, r.y, r.width,
                                   r.height).ToStdString());
  };

  const wxSize size = w->GetSize();
  if (w != top && size.x > 0 && size.y > 0) {
    //  A control that sticks out of any ancestor's client area is cut.
    wxRect me = screenRect(w);
    for (wxWindow *a = w->GetParent(); a; a = a->GetParent()) {
      if (scrollsOrIsCustom(a)) break;
      wxRect area = clientScreenRect(a);
      if (me.x < area.x - 1 || me.y < area.y - 1 ||
          me.GetRight() > area.GetRight() + 1 ||
          me.GetBottom() > area.GetBottom() + 1) {
        const int over = wxMax(wxMax(area.x - me.x, me.GetRight() - area.GetRight()),
                               wxMax(area.y - me.y, me.GetBottom() - area.GetBottom()));
        add("outside-parent",
            wxString::Format("%dx%d, %d px outside %s's %dx%d client area",
                             size.x, size.y, over,
                             (const char*) wxString(a->GetClassInfo()->GetClassName()).utf8_str(),
                             area.width, area.height).ToStdString());
        break;
      }
      if (a == top) break;
    }
  }

  //  Controls whose best size is the size of what they show.
  const bool fixedContent =
      w->IsKindOf(wxCLASSINFO(wxStaticText)) ||
      w->IsKindOf(wxCLASSINFO(wxButton)) ||
      w->IsKindOf(wxCLASSINFO(wxToggleButton)) ||
      w->IsKindOf(wxCLASSINFO(wxCheckBox)) ||
      w->IsKindOf(wxCLASSINFO(wxRadioButton)) ||
      w->IsKindOf(wxCLASSINFO(wxRadioBox)) ||
      w->IsKindOf(wxCLASSINFO(wxChoice)) ||
      w->IsKindOf(wxCLASSINFO(wxComboBox)) ||
      w->IsKindOf(wxCLASSINFO(wxComboCtrl)) ||
      w->IsKindOf(wxCLASSINFO(wxSpinCtrl)) ||
      w->IsKindOf(wxCLASSINFO(wxSpinCtrlDouble));
  wxStaticText *st = wxDynamicCast(w, wxStaticText);
  const bool ellipsized = st && (st->GetWindowStyleFlag() & wxST_ELLIPSIZE_MASK);
  if (fixedContent && !ellipsized && size.x > 0 && size.y > 0) {
    wxSize best = w->GetBestSize();
    if (size.x + 2 < best.x || size.y + 2 < best.y) {
      //  A wrapped label's best size is what it needs at the width it has,
      //  so a short one is cut, not wrapped.
      add("smaller-than-best",
          wxString::Format("is %dx%d, needs %dx%d", size.x, size.y, best.x,
                           best.y).ToStdString());
    }
  }

  wxAnyButton *btn = wxDynamicCast(w, wxAnyButton);
  if (btn) {
    wxBitmap bmp = btn->GetBitmap();
    const bool hasLabel = !btn->GetLabel().empty();
    const bool hasBitmap = bmp.IsOk();
    //  An empty button that is a colour swatch has its own background.
    const bool swatch = !hasLabel && !hasBitmap && w->GetParent() &&
        w->GetBackgroundColour() != w->GetParent()->GetBackgroundColour();
    if (!hasLabel && !hasBitmap && !swatch) {
      add("empty-button", "no label and no bitmap");
    } else if (!hasLabel && hasBitmap && !bitmapShowsAnything(bmp)) {
      add("blank-bitmap",
          wxString::Format("bitmap %dx%d draws nothing", bmp.GetWidth(),
                           bmp.GetHeight()).ToStdString());
    }
  }

  //  A single-line field narrower than what it holds.
  wxString value;
  int pad = 0;
  wxTextCtrl *tc = wxDynamicCast(w, wxTextCtrl);
  if (tc && !tc->IsMultiLine() && !(tc->GetWindowStyleFlag() & wxTE_PASSWORD)) {
    value = tc->GetValue();
    if (!value.empty()) {
      wxSize need = tc->GetSizeFromTextSize(
          tc->GetTextExtent(value).x);
      pad = need.x - tc->GetTextExtent(value).x;
    }
  }
  wxComboBox *cb = wxDynamicCast(w, wxComboBox);
  if (cb) { value = cb->GetValue(); pad = 36; }
  wxComboCtrl *cc = wxDynamicCast(w, wxComboCtrl);
  if (cc) { value = cc->GetValue(); pad = 36; }
  wxChoice *ch = wxDynamicCast(w, wxChoice);
  if (ch && ch->GetSelection() != wxNOT_FOUND) {
    value = ch->GetStringSelection(); pad = 36;
  }
  wxSpinCtrl *sp = wxDynamicCast(w, wxSpinCtrl);
  if (sp) { value = wxString::Format("%d", sp->GetValue()); pad = 36; }
  if (!value.empty()) {
    const int need = w->GetTextExtent(value).x + pad;
    if (size.x < need) {
      std::string v = value.ToStdString();
      if (v.size() > 30) v = v.substr(0, 30) + "...";
      add("text-wider-than-field",
          wxString::Format("field is %d px wide, value \"%s\" needs %d",
                           size.x, v.c_str(), need).ToStdString());
    }
  }
}

void walk(wxWindow *w, wxWindow *top, std::vector<std::string>& out)
{
  if (!w->IsShown()) return;
  if (w->IsKindOf(wxCLASSINFO(wxTopLevelWindow)) && w != top) return;
  if (w->IsShownOnScreen()) auditWindow(w, top, out);
  if (scrollsOrIsCustom(w)) return;
  //  A combo box's or spin control's inner text field is not a control of
  //  ours.
  if (w->IsKindOf(wxCLASSINFO(wxComboBox)) ||
      w->IsKindOf(wxCLASSINFO(wxComboCtrl)) ||
      w->IsKindOf(wxCLASSINFO(wxSpinCtrl)) ||
      w->IsKindOf(wxCLASSINFO(wxSpinCtrlDouble)) ||
      w->IsKindOf(wxCLASSINFO(wxRadioBox))) return;
  const wxWindowList& kids = w->GetChildren();
  for (wxWindowList::compatibility_iterator n = kids.GetFirst(); n;
       n = n->GetNext())
    walk(n->GetData(), top, out);
}

}  // namespace


std::vector<std::string> ewxWindowUtils::clipFindings(wxWindow *top)
{
  std::vector<std::string> out;
  if (top) walk(top, top, out);
  return out;
}


void ewxWindowUtils::clipAuditReport(wxWindow *top, const std::string& tag)
{
  const char *path = getenv("ECCE_CLIP_AUDIT");
  if (!path || !top) return;
  std::vector<std::string> found = clipFindings(top);
  wxRect r = screenRect(top);
  std::string shot;
  const char *dir = getenv("ECCE_CLIP_SHOTS");
  if (dir) {
    std::string safe = tag;
    for (size_t i = 0; i < safe.size(); ++i)
      if (!isalnum((unsigned char)safe[i]) && safe[i] != '.' && safe[i] != '-')
        safe[i] = '_';
    shot = std::string(dir) + "/" + safe + ".png";
    //  Cheap and works on every Xvfb: the screen itself, not the window.
    wxString cmd;
    cmd << "import -window root '" << shot << "' 2>/dev/null";
    if (system(cmd.mb_str()) != 0) shot.clear();
  }
  FILE *f = fopen(path, "a");
  if (!f) return;
  fprintf(f, "WINDOW\t%s\t%d,%d,%d,%d\t%s\t%s\n", tag.c_str(), r.x, r.y,
          r.width, r.height, (const char*) wxString(top->GetClassInfo()->GetClassName()).utf8_str(),
          shot.c_str());
  for (size_t i = 0; i < found.size(); ++i)
    fprintf(f, "FINDING\t%s\t%s\n", tag.c_str(), found[i].c_str());
  fclose(f);
}


void ewxWindowUtils::scheduleClipAudit(wxTopLevelWindow *win)
{
  if (!getenv("ECCE_CLIP_AUDIT") || !win) return;
  static std::set<wxWindow*> seen;
  if (!seen.insert(win).second) return;
  wxWeakRef<wxTopLevelWindow> ref(win);
  wxTimer *timer = new wxTimer();   // lives until the process exits
  timer->Bind(wxEVT_TIMER, [ref](wxTimerEvent&) {
    if (!ref) return;
    wxString title = ref->GetTitle();
    if (title.empty()) title = ref->GetClassInfo()->GetClassName();
    ewxWindowUtils::clipAuditReport(
        ref.get(), (wxTheApp->GetAppName() + ":" + title).ToStdString());
  });
  timer->StartOnce(5000);
}
