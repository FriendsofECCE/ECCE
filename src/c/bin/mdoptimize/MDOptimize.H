#ifndef _MDOPTIMIZE_H_
#define _MDOPTIMIZE_H_


#include "wxgui/MDEdBase.H"
#include "wxgui/WxJMSListener.H"

class JMSMessage;
class EcceException;

class InteractionPanel;
class OptimizePanel;
class ConstraintPanel;
class ThermodynamicsPanel;
class ControlPanel;
class FilesPanel;
class NWChemMDModel;


/*!
 * MDOptimizeGUI class declaration
 */

class MDOptimize: public MDEdBase, public WxJMSListener
{    

   public:
      /// Constructors
      MDOptimize( );
      MDOptimize( wxWindow* parent, wxWindowID id = -1,
	  const wxString& caption = "ECCE NWChem MD Optimize",
	  const wxPoint& pos = wxDefaultPosition,
	  const wxSize& size = wxDefaultSize,
	  long style = wxDEFAULT_FRAME_STYLE);


      // TaskApp virtuals
      virtual string getTitle() const;
      virtual void setContext(const string& url) throw (EcceException);
      void resetPanel();

   protected:

      void initializeModel();
      virtual void createPanels();
      virtual void refreshGUI();
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


      virtual string getAppName() const;


   private:
      InteractionPanel *p_interaction;
      OptimizePanel *p_optimize;
      ConstraintPanel *p_constraint;
      ThermodynamicsPanel *p_thermo;
      ControlPanel *p_control;
      FilesPanel *p_files;

};

#endif
