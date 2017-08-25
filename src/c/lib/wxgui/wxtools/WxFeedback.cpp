/**
 * @file
 *
 *
 */
#include <iostream>
 using std::cout;
 using std::cerr;
 using std::endl;

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxFeedback.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/bitmap.h"  // for wxNullBitmap

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"
#include "util/Color.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/PropertyTask.H"
#include "dsm/EDSIFactory.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxMenu.H"
#include "wxgui/WxState.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/DnDCalcDrop.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxColor.H"


WxFeedback* WxFeedback::p_feedback = (WxFeedback*)0;

/**
 * Default Constructor.
 */
WxFeedback::WxFeedback( ) : WxFeedbackGUI()
{
}


/**
 * Constructor.
 * Status is set to NA.
 *
 * Connect your application to the save icon by calling setSaveHandler.
 * Connect your application to the drop icon by calling setDropHandler.
 */
WxFeedback::WxFeedback( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        long style )
  : WxFeedbackGUI(parent, id, pos, size, style)
{
   p_saveHandler = 0;
   p_editStatus = NA;
   p_stateIcon = new WxState(p_statePanel);
   p_beepWarn = p_beepError = p_confirmResets = true;
   if (p_feedback == (WxFeedback*)0) {
     p_feedback = this;
   }
   readPreferences();

   p_textMenu = new ewxMenu;
   p_textMenu->Append(wxID_CLEAR, "Clear", "", wxITEM_NORMAL);
   Connect(wxID_CLEAR,
           wxEVT_COMMAND_MENU_SELECTED,
           wxCommandEventHandler(WxFeedback::OnClearText));

   p_textMenu->AppendSeparator();
   p_textMenu->Append(wxID_HOME, "Show Gateway", "", wxITEM_NORMAL);
   Connect(wxID_HOME,
           wxEVT_COMMAND_MENU_SELECTED,
           wxCommandEventHandler(WxFeedback::OnShowGateway));

   p_textEvtHandler = new FeedbackTextEvtHandler();
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
   if (text) {
     text->SetValidator(wxDefaultValidator);
     text->PushEventHandler(p_textEvtHandler);
   }

   p_timer = new wxTimer(this);
   Connect(wxID_ANY,
           wxEVT_TIMER,
           wxTimerEventHandler(WxFeedback::FlashOff));
}


/**
 * Destructor.
 */
WxFeedback::~WxFeedback()
{
  delete p_textMenu;
}

int WxFeedback::getSaveId() const
{
   return ID_SAVE;
}


void WxFeedback::FlashOn(const char* color)
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
  ewxColor bkg(color);
  wxString saved = text->GetValue();
  text->Clear();
  text->SetBackgroundColour(bkg);
  text->SetValue(saved);

  p_timer->Start(500, wxTIMER_ONE_SHOT);
}


/**
 * Event handling function for timer.
 *
 * Restores the read-only background color for the ewxLogTextCtrl.
 */
void WxFeedback::FlashOff(wxTimerEvent& WXUNUSED(event))
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
  wxString saved = text->GetValue();
  text->Clear();
  text->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());
  text->SetValue(saved);
}


/**
 * Reload system preferences.
 * Example preferences include state color, and warning/error beep/flashing
 */
void WxFeedback::readPreferences()
{
   Preferences userPrefs(PrefLabels::GLOBALPREFFILE);
   userPrefs.getBool(PrefLabels::ERRORBEEP,p_beepError);
   userPrefs.getBool(PrefLabels::WARNINGBEEP,p_beepWarn);

   Preferences resetPrefs("Organizer");
   resetPrefs.getBool("Organizer.ConfirmResets", p_confirmResets);

   // support update of color changes for state icon
   p_stateIcon->resetUserPreferences();
   p_stateIcon->refresh();
}



/**
 * Sets up the drop site and adds a handler.
 * You must call this to get the drop site to be functional.
 * Currently only URL drops are supported.
 */
void WxFeedback::setDropHandler(CalcDropHandler *handler)
{

   wxBitmapButton *btn = 
      ((wxBitmapButton*)FindWindow( ID_BITMAPBUTTON_DROPICON));
   btn->SetDropTarget(new DndCalcDrop(btn, handler));
}


/**
 * Sets up the save icon handler.
 * You must call this to get save events.  I was unable to force the events
 * to propagate with the custom widget we have.  The custom widget provides
 * style that we could not get with current version of wx/gtk
 */
void WxFeedback::setSaveHandler(FeedbackSaveHandler *handler)
{
   p_saveHandler = handler;
}

/**
 * Set the associated task in order to carry out the reset state operation
 */
void WxFeedback::setContextURL(const EcceURL& url)
{
  p_task = dynamic_cast<PropertyTask*>(EDSIFactory::getResource(url));
  if (p_task != 0) {
    FlatBitmapButton *reset = ((FlatBitmapButton*)
                                FindWindow(ID_RESETSTATE));
    reset->SetBitmap(ewxBitmap("calcreset.xpm", wxBITMAP_TYPE_XPM));
  }
}

/**
 * Shows the label in the context label area (between state and edit icons)
 */
void WxFeedback::setContextLabel(const string& label)
{
   ewxStaticText *text =
       ((ewxStaticText*)FindWindow(wxID_STATIC_FEEDBACK_CONTEXT));
   text->SetLabel(label.c_str());
   p_topLineSizer->Layout();
}


/**
 * Set the edit status.
 * This results in the appropriate icon being displayed except for
 * NA which displays no icon.
 *
 * There is no explicit connection between the runstate and the
 * editstate.
 */
void WxFeedback::setEditStatus(WxFeedback::EditStatus status)
{

   p_editStatus = status;

   FlatBitmapButton *save = (FlatBitmapButton*) FindWindow( getSaveId());

   // For now using the same bitmap for disabled and normal states
   if (status == READONLY) {
      save->SetBitmap(ewxBitmap("lock.xpm",wxBITMAP_TYPE_XPM));
      save->Show(true);
      save->showBorder(false);
   } else if (status == MODIFIED) {
      save->SetBitmap(ewxBitmap("save.xpm",wxBITMAP_TYPE_XPM));
      save->Show(true);
      save->showBorder(true);
   } else {
      // Clear it -both SAVE and LOCK states
      //save->SetBitmap(ewxBitmap("edit_x.xpm",wxBITMAP_TYPE_XPM));
      save->Show(false);
   }
   //save->showBorder(false);
   p_topLineSizer->Layout();
}


/**
 * Get the edit status.
 */
WxFeedback::EditStatus WxFeedback::getEditStatus() const
{
   return p_editStatus;
}



/**
 * Add to the message area.  
 * @param message text to add
 * @param severity - @see WxFeedback::Severity
 * @param clear clears existing text first if true
 * @param flush immediately displays message by flushing event queue
 */
void WxFeedback::setMessage(const string& message, 
      const WxFeedback::Severity& severity,
      const bool& clear, const bool& flush)
{
   // Magic function that flushes the wxWidgets event queue
   // This is needed in certain instances when the message area is updated,
   // but control is not returned to the event loop.
   // The Launcher message area during a launch is one instance of this.
   // Doing this generically though can lead to odd behavior like duplicate
   // java messages being processed on the event queue so use a flag to
   // determine when it is needed.
   if (flush) {
      wxSafeYield();
   }

   if (clear) {
      clearMessage();
   }
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
   // SetValue doesn't work. operator<< sortof works
   //text->SetValue(message.c_str());
   if (severity == WxFeedback::ERROR) 
     (*text) << "ERROR: ";
   else if (severity == WxFeedback::WARNING) 
     (*text) << "WARNING: ";
   else if (severity == WxFeedback::SYSTEM) 
     (*text) << "SYSTEM: ";

   (*text) << message.c_str();

   if (text->GetLastPosition() != 0) {
      (*text) << "\n";
   }

   if ((p_beepWarn && (severity == WxFeedback::WARNING)) ||
       (p_beepError && (severity==ERROR || severity==SYSTEM))) {
      wxBell();
   }

   if (severity == WxFeedback::WARNING) {
      FlashOn(Color::WARNING);
   } else if (severity==ERROR || severity==SYSTEM) {
      FlashOn(Color::ERROR);
   }
}


/**
 * Clears the message area.
 */
void WxFeedback::clearMessage()
{
   ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
   text->SetValue("");
}


/**
 * Sets the runstate.
 * The state icon will be redrawn to reflect the changed state.
 */
void WxFeedback::setRunState(ResourceDescriptor::RUNSTATE state)
{
   p_stateIcon->setRunState(state);

   FlatBitmapButton *reset = ((FlatBitmapButton*)
                               FindWindow(ID_RESETSTATE));
   if (state == ResourceDescriptor::STATE_COMPLETED ||
       state == ResourceDescriptor::STATE_KILLED ||
       state == ResourceDescriptor::STATE_UNSUCCESSFUL ||
       state == ResourceDescriptor::STATE_FAILED ||
       state == ResourceDescriptor::STATE_SYSTEM_FAILURE) {
     // show the reset state button
    reset->Show();
   } else {
     // hide the reset state button
    reset->Hide();
   }
   p_topLineSizer->Layout();
}


ResourceDescriptor::RUNSTATE WxFeedback::getRunState() const
{
   return p_stateIcon->getRunState();
}


/**
 * Hides the entire top line sizer recursively.
 */
void WxFeedback::hideTopLine()
{
  bool show = false;
  bool recursive = true;
  GetSizer()->Show(p_topLineSizer, show, recursive);
}

/**
 * Shows the entire top line sizer recursively.
 */
void WxFeedback::showTopLine()
{
  bool show = true;
  bool recursive = true;
  GetSizer()->Show(p_topLineSizer, show, recursive);
}


/**
 * Hides run state icon for apps that don't use it.
 */
void WxFeedback::hideStateIcon()
{
   p_stateIcon->Hide();
}


/**
 * Hides drop site icon for apps that don't use it.
 */
void WxFeedback::hideDropSite()
{
   ((wxBitmapButton*)FindWindow(ID_BITMAPBUTTON_DROPICON))->Hide();
   p_stateIcon->Hide();
}


/**
 * Hides EditState icon for apps that don't use it.
 */
void WxFeedback::hideEditState()
{
   FlatBitmapButton *save = (FlatBitmapButton*) FindWindow( getSaveId());
   save->Hide();
}


void WxFeedback::resetForRerun()
{
  bool doit = true;

  if (p_confirmResets) {
    string msg = "Resetting to rerun this task will remove all existing"
                 " output data from the previous run.  Continue?";

    ewxMessageDialog msgDlg(ewxWindowUtils::getTopWindow(this), msg,
                            "Reset Task?",
                            wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    p_task->resetForRerun(ResourceDescriptor::STATE_READY);
  }
}


/**
 * Handle reset state clicked
 */
void WxFeedback::onResetstateClicked( wxCommandEvent& event )
{
  resetForRerun();
  event.Skip();
}


/**
 * Handle save button clicked
 */
void WxFeedback::onSaveClicked( wxCommandEvent& event )
{
  if (p_saveHandler!=0 && p_editStatus==MODIFIED)
    p_saveHandler->processSave();
  event.Skip();
}


void WxFeedback::OnContextMenu( wxContextMenuEvent& event )
{
  wxPoint pt = event.GetPosition();

  if (event.GetClientObject()) { // Right click event inside text area
    if ( pt.x == -1 && pt.y == -1 ) { //(this is how MSW indicates it)
      pt = wxPoint(20, 20);
    }
    else {                // event was generated by mouse, use supplied coords
      pt = ScreenToClient(pt);
    }
    PopupMenu(p_textMenu, pt);
  }
  else  // Right click event outside text area
    ;
}


void WxFeedback::OnClearText(wxCommandEvent& event)
{
  ewxTextCtrl *text = ((ewxTextCtrl*)FindWindow(ID_TEXTCTRL_FEEDBACK_MSG));
  if (text) {
    text->Clear();
  }
}

void WxFeedback::OnShowGateway(wxCommandEvent& event)
{
  JMSPublisher publisher("Feedback");
  JMSMessage *msg = publisher.newMessage();
  publisher.publish("ecce_gateway_raise",*msg);
  delete msg;
}


/**
 * Return instance of WxFeedback widget, if it exists
 */
WxFeedback* WxFeedback::getFeedback(wxWindow *win)
{
  // Navigate to top of current window
  wxWindow *topWin = ewxWindowUtils::getTopWindow(win);

  // Recurse to see if there is a WxFeedback instance on this window
  WxFeedback *ret = findFeedback(topWin);

  // If none found, fallback to the static one that would be on
  // a different window in the same app
  if (ret == (WxFeedback*)0)
    ret = p_feedback;

  return ret;
}

WxFeedback* WxFeedback::findFeedback(wxWindow *win)
{
   WxFeedback *feedback = dynamic_cast<WxFeedback*>(win);
   if (feedback != (WxFeedback*)0) {
      return feedback;
   }

   wxWindowList children = win->GetChildren();
   wxWindowList::compatibility_iterator node = children.GetFirst();

   while (feedback==(WxFeedback*)0 && node) {
      wxWindow *child = (wxWindow*)node->GetData();

      // Recursive call to navigate children
      feedback = findFeedback(child);

      node = node->GetNext();
   }

   return feedback;
}


FeedbackTextEvtHandler::FeedbackTextEvtHandler()
{
  p_clientData = new wxClientData;
  Connect(wxEVT_CONTEXT_MENU,
          wxContextMenuEventHandler(FeedbackTextEvtHandler::OnContextMenu));
}


FeedbackTextEvtHandler::~FeedbackTextEvtHandler()
{
  Disconnect(wxEVT_CONTEXT_MENU,
             wxContextMenuEventHandler(FeedbackTextEvtHandler::OnContextMenu));
  delete p_clientData;
  p_clientData = 0;
}


void FeedbackTextEvtHandler::OnContextMenu(wxContextMenuEvent & event)
{
  event.SetClientObject(p_clientData);
  event.Skip();
}
