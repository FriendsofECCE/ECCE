#ifndef _GMXPREPARE_H_
#define _GMXPREPARE_H_

#include "wxgui/MDEdBase.H"
#include "wxgui/WxJMSListener.H"

class JMSMessage;
class GromacsPrepareModel;
class GromacsPreparePanel;

/**
 * The editor of a GROMACS Prepare task.  It borrows MDEdBase's window,
 * messaging and test hook but not its model: the task's model is a
 * GromacsPrepareModel, and the input it writes is gromacs.gmxprep (through
 * md.gmxprep), not an .mdp.
 */
class GmxPrepare: public MDEdBase, public WxJMSListener
{
   public:
      GmxPrepare( );
      GmxPrepare( wxWindow* parent, wxWindowID id = -1,
                  const wxString& caption = "ECCE GROMACS MD Prepare",
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxDEFAULT_FRAME_STYLE);

      virtual string getTitle() const;
      virtual void setContext(const string& url);
      virtual void saveModelAndInputFile(MdTask *task);
      virtual void generateInputFile(MdTask *task);
      virtual void updateDisabledState(ResourceDescriptor::RUNSTATE state,
                                       bool forceMsg=false);
      void resetPanel();

   protected:
      virtual void initializeModel();
      virtual void createPanels();
      virtual void refreshGUI();
      virtual void constructor();
      virtual string getAppName() const;
      virtual string runTestCommand(const string& line);
      virtual void OnButtonResetaClick( wxCommandEvent& event );

      void initializeMessaging();
      void invokeMCB(JMSMessage& msg);
      void quitMCB(JMSMessage& msg);
      void identifyMCB(JMSMessage& msg);
      void stateMCB(JMSMessage& msg);
      void statePrefsMCB(JMSMessage& msg);
      void prefsMCB(JMSMessage& msg);
      void renameMCB(JMSMessage& msg);
      void removeMCB(JMSMessage& msg);
      void molSelectionMCB(JMSMessage& msg);
      void authMCB(JMSMessage& msg);

   private:
      GromacsPrepareModel *prepModel() const;
      GromacsPreparePanel *p_panel;
};

#endif
