/**
 * @file
 *
 *
 */
#ifndef _MDDYNAMICS_H_
#define _MDDYNAMICS_H_

#include "util/IOException.H"

#include "wxgui/MDEdBase.H"
#include "wxgui/WxJMSListener.H"

class InteractionPanel;
class ConstraintPanel;
class DynamicsPanel;
class ThermodynamicsPanel;
class ControlPanel;
class FilesPanel;
class NWChemMDModel;


/*!
 * MDDynamics class declaration
 */

class MDDynamics: public MDEdBase, public WxJMSListener
{    

   public:
      /// Constructors
      MDDynamics( );
      MDDynamics( wxWindow* parent,
	          wxWindowID id = -1,
		  const wxString& caption = "ECCE NWChem MD Dynamics",
		  const wxPoint& pos = wxDefaultPosition,
		  const wxSize& size = wxDefaultSize,
		  long style = wxDEFAULT_FRAME_STYLE );


      // TaskApp virtuals
      virtual string getTitle() const;

      void setContext(const string& url) throw (EcceException);
      void resetPanel();

      /**
       * Overwrite this function from MDEdBase class
       */
      void processSave();
      void notifyChange();

   protected:


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

      virtual void constructor();
      virtual void createPanels();
      virtual void initializeModel();
      virtual void refreshGUI();
      virtual string getAppName() const;

      InteractionPanel *p_interaction;
      DynamicsPanel *p_dynamics;
      ThermodynamicsPanel *p_thermo;
      ConstraintPanel *p_constraint;
      ControlPanel *p_control;
      FilesPanel *p_files;


};

#endif
