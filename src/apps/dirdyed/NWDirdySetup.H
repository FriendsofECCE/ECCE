/**
 * @file
 *
 *
 */

#ifndef NWDIRDYSETUP_HH
#define NWDIRDYSETUP_HH


#include "wx/wx.h"

#include "util/EcceException.H"
#include "util/IOException.H"
#include "util/EditListener.H"

#include "tdat/SpinMult.H"

#include "NWDirdySetupGUI.H"

class JMSMessage;
class WxJMSMessageDispatch;
class ewxProgressDialog;
class WxFeedback;

class EditEvent;

class NWDirdyModel;
class NWDirdy;

/**
 * GUI Implementation of NWChem DIRDYVTST editor.
 */

class NWDirdySetup: public NWDirdySetupGUI
{    

   public:

      NWDirdySetup(NWDirdyModel *model, NWDirdy *nwdirdy,
	    wxWindow* parent, wxWindowID id = SYMBOL_NWDIRDYSETUPGUI_IDNAME,
            const wxString& caption = SYMBOL_NWDIRDYSETUPGUI_TITLE,
            const wxPoint& pos = SYMBOL_NWDIRDYSETUPGUI_POSITION,
            const wxSize& size = SYMBOL_NWDIRDYSETUPGUI_SIZE,
            long style = SYMBOL_NWDIRDYSETUPGUI_STYLE );
      ~NWDirdySetup();

      void refreshGUI();
      void refreshBasisSetFields();

      virtual bool Show(bool show);

      virtual void setUnitFamily(const string& family);
      void setNWDirdyModel(NWDirdyModel *model);
      NWDirdyModel* getNWDirdyModel() const;

      void setSaveState(bool unsavedChanges);
      bool isUnsavedState();
      void setParent(NWDirdy *parent);
      void restrictTheories(const SpinMult::SpinMultEnum& mult,
                            const bool& initializeFlag);

      /*
       * NWDirdySetup event handler declarations
       */
      virtual void OnCloseWindow( wxCloseEvent& event );
      virtual void OnChoiceNwdrdysetThSelected( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetBasisClick( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetSpbasisClick( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetDtlsClick( wxCommandEvent& event );
      virtual void OnChoiceNwdrdysetSpthSelected( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetSpdtlsClick( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetSymClick( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetTolEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSepEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSstepEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSsvEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetShssEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSmssEnter( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetTstClick( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSlmEnter( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetSlpEnter( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetSgnClick( wxCommandEvent& event );
      virtual void OnChoiceNwdrdysetIntalgSelected( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetPrtClick( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetCloseClick( wxCommandEvent& event );
      virtual void OnButtonNwdrdysetResetClick( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetAutozClick( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetMemoryClick( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetMemoryEnter( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetDiskClick( wxCommandEvent& event );
      virtual void OnTextctrlNwdrdysetDiskEnter( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetExpcoeffClick( wxCommandEvent& event );
      virtual void OnCheckboxNwdrdysetSpexpcoeffClick( wxCommandEvent& event );

   protected:
      void initializeGUI();

      NWDirdyModel *p_model;
      NWDirdy *p_parent;
      WxFeedback *p_feedback;
};

#endif
