/**
 * @file
 *
 *
 */
#ifndef _MDENERGY_H_
#define _MDENERGY_H_


#include "util/IOException.H"

#include "wxgui/MDEdBase.H"
#include "wxgui/WxJMSListener.H"



/**
 * MDEnergy is a GUI for setting up NWChem MD Energy calculations.
 */
class MDEnergy: public MDEdBase, public WxJMSListener
{    

   public:
      /// Constructors
      MDEnergy( );
      MDEnergy( wxWindow* parent, wxWindowID id = -1, const wxString& caption = "ECCE NWChem MD Energy", const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE );


      // TaskApp virtuals
      virtual string getTitle() const;
      virtual void setContext(const string& url) throw (EcceException);
      void resetPanel();

   protected:

      virtual void initializeModel();
      virtual void constructor();

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

      virtual void createPanels();
      virtual void refreshGUI();
      virtual string getAppName() const;

      InteractionPanel *p_interaction;
      ConstraintPanel *p_constraint;
      ControlPanel *p_control;
      FilesPanel *p_files;



};

#endif
