/**
 * @file
 * @author Ken Swanson
 *
 *  Utility to allow registration functions for calculation servers.
 *  Supports adding/editing/deleting of registered Machine instances.
 *  Usually this is invoked from the Gateway, but can be invoked from
 *  the command line via "ecce -admin", which edits the site files instead
 *  of the user's own.
 *
 *  One machine at a time is edited in a tabbed frame.  What the form holds
 *  is tracked by a MachineConfigDraft (the CONFIG.<machine> keys, the
 *  Machines line and the queues) so that "unsaved" is a comparison with what
 *  was loaded, not a flag.
 */

#ifndef WXMACHINEREGISTER_HH
#define WXMACHINEREGISTER_HH

#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>
    using std::string;
    using std::vector;

#include "wx/frame.h"
#include "wx/msgdlg.h"

#include "tdat/MachineConfigDraft.H"
#include "tdat/RefMachine.H"

#include "util/Preferences.H"

#include "wxgui/ewxFrame.H"

class wxPanel;
class wxSpinCtrlDouble;
class wxListCtrl;
class wxListEvent;
class wxCollapsiblePane;
class wxNotebook;
class wxStaticText;
class ewxButton;
class ewxCheckBox;
class ewxChoice;
class ewxScrolledWindow;
class ewxSpinCtrl;
class ewxStaticText;
class ewxTextCtrl;
class MachRegScript;

class WxMachineRegister: public ewxFrame
{
    public:
        WxMachineRegister(wxWindow* parent, const bool admin = false);
        virtual ~WxMachineRegister();

        /// Select a machine in the list; asks about unsaved changes first.
        bool selectMachine(string refName);

        /// Open on a tab by name (machine, connection, codes, job, queues).
        bool showPage(const string& name);

    private:
        friend class MachRegScript;

        struct Row { string name, from; };

        //  One CONFIG key on the Connection tab: label, control, source tag
        //  and the [...] menu.  The control is named after the key.
        enum CfgKind { CfgText, CfgShell, CfgCheck, CfgCheckYes, CfgTri };
        struct CfgRow {
            string key, label;
            CfgKind kind;
            wxWindow *ctrl;
            wxStaticText *tag, *name;
            ewxButton *menu;
            CfgRow() : kind(CfgText), ctrl(NULL), tag(NULL), name(NULL),
                       menu(NULL) {}
        };

        //  Construction
        void createControls();
        wxWindow* createMachinePage(wxWindow* parent);
        wxWindow* createConnectionPage(wxWindow* parent);
        wxWindow* createCodesPage(wxWindow* parent);
        wxWindow* createJobScriptPage(wxWindow* parent);
        wxWindow* createQueuesPage(wxWindow* parent);
        ewxScrolledWindow* newPage(wxWindow* parent, wxSizer* sizer);
        void loadQueueManagerList();
        void addCfgRow(wxWindow* page, wxFlexGridSizer* grid, const string& key,
                       const string& label, CfgKind kind, const string& tip);
        void addHeading(wxWindow* page, wxSizer* sizer, const string& text);

        //  Event handlers
        void onClose(wxCloseEvent& event);
        void onListSelected(wxListEvent& event);
        void onFullNameText(wxCommandEvent& event);
        void onFieldChanged(wxCommandEvent& event);
        void onQueueChoice(wxCommandEvent& event);
        void onQueueApply(wxCommandEvent& event);
        void onQueueRemove(wxCommandEvent& event);
        void onQueueClear(wxCommandEvent& event);
        void onSave(wxCommandEvent& event);
        void onDelete(wxCommandEvent& event);
        void onNew(wxCommandEvent& event);
        void onCloseButton(wxCommandEvent& event);
        void onHelp(wxCommandEvent& event);

        //  The machine list
        void loadMachinesList();
        int findRow(const string& name) const;
        bool fromSite() const;

        //  Loading and showing a machine
        void loadMachine(const string& refName);
        void showNewMachine();
        MachineConfigDraft* newDraft(const string& refName) const;
        void draftToControls();
        void clearQueues();
        void loadQueues(const string& refName);
        void fillQueues();
        void showQueue(const string& name = "");
        void showSiteBanner();
        string explain(const string& refName, string& err) const;
        CfgRow* cfgRow(const string& key);
        string cfgDefault(const CfgRow& r) const;
        string cfgHint(const CfgRow& r) const;
        string cfgCanon(const CfgRow& r, const string& raw) const;
        string cfgCtrlText(const CfgRow& r) const;
        void cfgToControl(const CfgRow& r);
        void cfgToControls();
        void cfgTags();
        void onCfgMenu(wxCommandEvent& event);
        void popupCfgMenu(const string& key, const string& shot,
                          std::function<void()> after = std::function<void()>());
        void cfgAction(const string& key, const string& action);
        string cfgSources(const CfgRow& r) const;

        //  Dirty tracking
        void syncDraft();
        void syncKey(MachineConfigDraft* draft, const string& key,
                     const string& text, bool cppOnly = false);
        void syncCfg(MachineConfigDraft* draft, const CfgRow& r);
        void syncKeys(MachineConfigDraft* draft);
        void updateDirty();
        bool isDirty();
        bool hasMinimalInput();
        bool queueFormDiffers() const;
        MachineConfigDraft::QueueRow queueFormRow() const;
        void updateQueueButtons();
        int confirmUnsaved(const string& discardLabel);
        bool resolveUnsaved(const string& discardLabel);

        //  Queues
        bool applyQueueForm();
        void removeQueue();

        //  Saving and deleting
        bool save();
        bool verifyInput();
        string collectSettings() const;
        bool writeConfig(const string& name, string& err);
        bool deleteMachine();
        string removalList(const string& refName) const;
        void redo(const string& refName);

        //  Dialogs; every one goes through ask()/displayMessage() so the
        //  test hook can answer them.
        int ask(const string& title, const string& message, const string& ext,
                long style, const string& yes, const string& no,
                const string& cancel);
        void displayMessage(const string& mesg);
        void notifyUpdate();

        //  Sizing (#187) and the locality note (#144)
        void growToFitSizer(bool fitWholeForm = false);
        wxSize maxClientSizeForDisplay();
        void keepOnScreen();
        void refreshLocality();
        void updateFooter();
        string editedBase() const;
        void saveSettings(Preferences& prefs);
        void restoreSettings(Preferences& prefs);

        //  Test hook support
        wxWindow* field(const string& name) const;
        void reg(const string& name, wxWindow* w);

        wxNotebook          *p_book;
        wxListCtrl          *p_list;
        wxPanel             *p_info;
        wxStaticText        *p_infoText;
        wxStaticText        *p_storeNote;   // names the files Save writes
        wxStaticText        *p_jobNote;
        vector<CfgRow>       p_cfgRows;
        wxWindow            *p_cfgPage;
        wxCollapsiblePane   *p_advanced;
        ewxStaticText       *p_localityText;
        ewxTextCtrl         *p_fullName, *p_refName, *p_vendor, *p_model,
                            *p_proc, *p_queueName;
        ewxSpinCtrl         *p_procs, *p_nodes, *p_qMinProcs, *p_qMaxProcs,
                            *p_qMaxMem, *p_qMinScratch;
        wxSpinCtrlDouble    *p_qMaxWall;   // hours; stored as minutes
        ewxChoice           *p_qmgrChoice, *p_queueChoice;
        ewxCheckBox         *p_allocAccts;
        ewxButton           *p_queueApply, *p_queueRemoveButton,
                            *p_queueClearButton, *p_saveButton,
                            *p_deleteButton, *p_newButton, *p_closeButton,
                            *p_helpButton;
        vector<ewxTextCtrl*> p_codePaths;
        vector<string>       p_codeNames;
        std::map<string, wxWindow*> p_fields;

        bool                 p_adminFlag;
        bool                 p_inCtrlUpdate;
        bool                 p_inListUpdate;
        bool                 p_closing;
        string               p_autoRefName;
        string               p_loadedName;   // "" for a new machine
        string               p_loadedFrom;   // yours, site, server
        vector<Row>          p_rows;
        RefMachine          *p_slctRgstn;

        MachineConfigDraft  *p_draft;
        vector<MachineConfigDraft::QueueRow> p_queues;
        MachineConfigDraft::QueueRow p_queueFormBase;

        //  Test hook only; empty otherwise.
        std::deque<int>      p_answers;
        bool                 p_scripted;
        string               p_lastMessage;

};

#endif  // WXMACHINEREGISTER_HH
