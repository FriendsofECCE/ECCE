/**
 * @file
 *
 *
 */
#ifndef _WXFEEDBACK_H_
#define _WXFEEDBACK_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "WxFeedback.C"
#endif

#include <string>
  using std::string;

class ewxMenu;
class WxState;
class PropertyTask;


#include "dsm/ResourceDescriptor.H"

#include "util/EcceURL.H"

#include "wxgui/WxFeedbackGUI.H"
#include "wxgui/CalcDropHandler.H"
#include "wxgui/FeedbackSaveHandler.H"


class FeedbackTextEvtHandler :public wxEvtHandler
{
public:
  FeedbackTextEvtHandler();
  ~FeedbackTextEvtHandler();
  void OnContextMenu(wxContextMenuEvent & event);
protected:

  wxClientData * p_clientData;

};



/**
 * Generic feedback panel that most tools will have a the bottom of their
 * main window. It includes a context summary label, a WxState icon,
 * a message area, an edit status icon, and a drop site.
 */
class WxFeedback : public WxFeedbackGUI
{
   public:
      /** Pertains to the edit status (lock, dirty star...) */
      enum EditStatus { READONLY, MODIFIED, EDIT, NA };

      /** Severity of error message - may have different appearances */
      enum Severity { INFO, WARNING, ERROR, SYSTEM };

      WxFeedback( );
      WxFeedback( wxWindow* parent,
	          wxWindowID id = SYMBOL_WXFEEDBACKGUI_IDNAME,
		  const wxPoint& pos = SYMBOL_WXFEEDBACKGUI_POSITION,
		  const wxSize& size = SYMBOL_WXFEEDBACKGUI_SIZE,
		  long style = SYMBOL_WXFEEDBACKGUI_STYLE );
      virtual ~WxFeedback( );

      static WxFeedback* getFeedback(wxWindow *win);
      static WxFeedback* findFeedback(wxWindow *win);

      void setDropHandler(CalcDropHandler *handler);
      void setSaveHandler(FeedbackSaveHandler *handler);
      
      void setContextURL(const EcceURL& url);

      void setContextLabel(const string& label);
      int getSaveId() const;

      void setEditStatus(WxFeedback::EditStatus status);
      WxFeedback::EditStatus getEditStatus() const;
      void setSaveHandler(wxObjectEventFunction function);

      void setMessage(const string& message, 
               const WxFeedback::Severity& severity=WARNING,
               const bool& clear=false, const bool& flush=false);
      void clearMessage();

      void setRunState(ResourceDescriptor::RUNSTATE state);
      ResourceDescriptor::RUNSTATE getRunState() const;

      void readPreferences();

      void hideEditState();
      void hideDropSite();
      void hideStateIcon();
      void hideTopLine();
      void showTopLine();

      void resetForRerun();

      virtual void onResetstateClicked( wxCommandEvent& event );

      virtual void onSaveClicked( wxCommandEvent& event );

      virtual void OnContextMenu( wxContextMenuEvent& event );

      virtual void OnClearText(wxCommandEvent& event);

      virtual void OnShowGateway(wxCommandEvent& event);

   protected:
      void FlashOn(const char* color);
      void FlashOff(wxTimerEvent& event);

      WxFeedback::EditStatus  p_editStatus;
      WxState                *p_stateIcon;
      FeedbackSaveHandler    *p_saveHandler;
      FeedbackTextEvtHandler *p_textEvtHandler;
      PropertyTask           *p_task;
      bool                    p_beepWarn;
      bool                    p_beepError;
      bool                    p_confirmResets;
      ewxMenu                *p_textMenu;
      static WxFeedback      *p_feedback;
      wxTimer                *p_timer;
};

#endif
