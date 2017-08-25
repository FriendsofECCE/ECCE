/**
 * @file
 * @author Ken Swanson
 *
 *  Utility to allow registration functions for calculation servers.
 *  Supports adding/editing/deleting of registered Machine instances.
 *  Usually this is invoked from the Gateway, but can be invoked from
 *  the command line via "ecce -admin".  In this mode, the use of the
 *  utility is intended for administrators, and a correspondingly more
 *  complete interface is displayed.
 *
 *
 */


#ifndef WXMACHINEREGISTER_HH
#define WXMACHINEREGISTER_HH

#define ID_TEXT_APPLICATIONS_ROOT 11100

#include <algorithm>
    using std::find;
#include <string>
    using std::string;
#include <vector>
    using std::vector;

#include "wx/frame.h"
#include "wx/msgdlg.h"

#include "tdat/RefMachine.H"

#include "util/Preferences.H"

//#include "wxgui/ewxWindowUtils.H"

#include "WxMachineRegisterGUI.H"

class ewxButton;
class ewxCheckBox;
class ewxChoice;
class ewxListBox;
class ewxSpinCtrl;
class ewxTextCtrl;

class WxMachineRegister: public WxMachineRegisterGUI
{
    public:
        /// Constructor, Destructor
        WxMachineRegister(      wxWindow* parent,
                      const bool admin = false,
                            wxWindowID id = SYMBOL_WXMACHINEREGISTERGUI_IDNAME,
                      const wxString& caption = SYMBOL_WXMACHINEREGISTERGUI_TITLE,
                      const wxPoint& pos = SYMBOL_WXMACHINEREGISTERGUI_POSITION,
                      const wxSize& size = SYMBOL_WXMACHINEREGISTERGUI_SIZE,
                       long style = SYMBOL_WXMACHINEREGISTERGUI_STYLE);
        virtual ~WxMachineRegister();

        //  Event Handlers
        void mainWindowCloseCB(wxCloseEvent& event);
        void machinesListBoxSelectedCB(wxCommandEvent& event);
        void machineFullNameUpdatedCB(wxCommandEvent& event);
        void machineRefNameUpdatedCB(wxCommandEvent& event);
        void queueChoiceboxSelectedCB(wxCommandEvent& event);
        void queueChangeButtonClickedCB(wxCommandEvent& event);
        void queueRemoveButtonClickedCB(wxCommandEvent& event);
        void queueClearButtonClickedCB(wxCommandEvent& event);
        void machineChangeButtonClickedCB(wxCommandEvent& event);
        void machineDeleteButtonClickedCB(wxCommandEvent& event);
        void formClearButtonClickedCB(wxCommandEvent& event);
        void formCloseButtonClickedCB(wxCommandEvent& event);
        void helpButtonClickedCB(wxCommandEvent& event);
        void machineVendorTextEnterCB(wxCommandEvent& event);

        bool selectMachine(string refName);



    protected:
        //  User interface components
        ewxTextCtrl         *p_machineFullNameText,
                            *p_machineRefNameText,
                            *p_machineVendorText,
                            *p_machineModelText,
                            *p_machineProcessorText,
                            *p_queueNameText;

        ewxSpinCtrl         *p_machineNumProcsSpin,
                            *p_machineNumNodesSpin,
                            *p_queueMinProcsSpin,
                            *p_queueMaxProcsSpin,
                            *p_queueMaxWallSpin,
                            *p_queueMaxMemorySpin,
                            *p_queueMinScratchSpin;

        ewxChoice           *p_queueManagerChoicebox,
                            *p_queuesChoicebox;

        ewxCheckBox         *p_queueAllctnAcctsCheckbox;

        ewxButton           *p_queueAcceptButton,
                            *p_queueRemoveButton,
                            *p_queueClearButton,
                            *p_machineChangeButton,
                            *p_machineDeleteButton,
                            *p_formClearButton,
                            *p_formCloseButton,
                            *p_helpButton;

        vector<ewxCheckBox*>    p_remshellsCheckboxes;
        vector<ewxTextCtrl*>    p_miscPathsText;
        vector<ewxTextCtrl*>    p_codePaths;       //  This is where the codes are being stored.

        ewxListBox          *p_machinesList;


    private:
        void initialize();
        void loadMachinesList();
        void refreshControls();

        void saveSettings(Preferences& prefs);
        void restoreSettings(Preferences& prefs);
        void loadQueueManagerList();

        void loadConfig(string refName);
        void loadQueues(string refName);
        void fillQueues();
        void showQueue(string refName = "");
        void removeQueue();
        vector<string> getConfigFileNames(string refName) const;
        void acceptChanges();
        bool verifyInput();
        int removeMachine();
        bool confirmRemove(string refName);
        bool hasMinimalInput();

        void clearForm();
        void clearQueues();
        string collectSettings() const;
        void displayMessage(string mesg);
        void redo(string refName);
        void reset();

        void notifyUpdate();
        bool confirmExit();

        //  Private Variables
        bool                    p_adminFlag;
        EcceMap                *p_config;
        vector<string>          p_qnames;
        vector<int>             p_minProcs;
        vector<int>             p_maxProcs;
        vector<int>             p_maxWall;
        vector<int>             p_maxMem;
        vector<int>             p_minScratch;
        vector<string>          p_codeNames;
        vector<string>          p_shellNames;

        RefMachine             *p_slctRgstn;
        bool                    p_inCtrlUpdate;

        string                  p_slctQueue;

};

#endif  // WXMACHINEREGISTER_HH
