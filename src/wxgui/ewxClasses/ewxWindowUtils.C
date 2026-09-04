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

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#endif

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/UnitFactory.H"
#include "util/UnitConverter.H"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/NumericValidatorBase.H"
#include "wxgui/WxFeedback.H"


/**
 * For window win, save the position and optionally the size of the
 * window in the config file.
 * No checking is done to ensure that the window is a shell.
 */
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

