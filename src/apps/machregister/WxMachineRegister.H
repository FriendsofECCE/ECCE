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
#include <map>
#include <string>
#include <utility>
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
class wxListBox;
class wxHyperlinkCtrl;
class wxListEvent;
class wxCollapsiblePane;
class wxBitmapButton;
class wxRadioButton;
class wxStaticBitmap;
class wxNotebook;
class wxTimer;
class wxDialog;
class wxButton;
class wxCheckBox;
class wxTextCtrl;
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
        //  and an undo button for the user's own value.
        enum CfgKind { CfgText, CfgShell, CfgCheck, CfgCheckYes, CfgTri };
        struct CfgRow {
            string key, label;
            CfgKind kind;
            wxWindow *ctrl;
            wxStaticText *tag, *name;
            wxWindow *undoBox;            // keeps the cell when undo is hidden
            wxBitmapButton *undo;
            bool gensubOnly;              // read by gensub, not by C++
            CfgRow() : kind(CfgText), ctrl(NULL), tag(NULL), name(NULL),
                       undoBox(NULL), undo(NULL), gensubOnly(false) {}
        };

        //  A block of text on the Job script tab: the inherited text shown
        //  read-only above, the user's own (replacing it) below.
        struct BlockRow {
            string id;                    // header, setup, wrapup
            string key;                   // the CONFIG key now shown
            wxStaticText *heading, *siteHead, *siteLabel, *yoursLabel, *tag, *note;
            wxTextCtrl *site, *user;
            wxCheckBox *none;
            wxButton *copy;
            wxWindow *undoBox;
            wxBitmapButton *undo;
            wxSizer *box;
            wxWindow *scroll;             // the tab's scrolled window
            BlockRow() : heading(NULL), siteHead(NULL), siteLabel(NULL), yoursLabel(NULL),
                         tag(NULL), note(NULL), site(NULL), user(NULL),
                         none(NULL), copy(NULL), undoBox(NULL), undo(NULL),
                         box(NULL), scroll(NULL) {}
        };

        //  A one-line setting of the code selected on the Codes tab (program,
        //  files to remove): its key is the code's name plus `suffix`.
        struct CodeLine {
            string id, suffix;
            wxTextCtrl *ctrl;
            wxStaticText *name, *tag;
            wxWindow *undoBox;
            wxBitmapButton *undo;
            CodeLine() : ctrl(NULL), name(NULL), tag(NULL), undoBox(NULL),
                         undo(NULL) {}
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
        void onCfgUndo(wxCommandEvent& event);
        void cfgUndo(const string& key);
        int undoCase(const string& key, bool cppOnly) const;
        string undoTip(int undoCase, const string& what) const;
        void undoKey(const string& key, bool revertOnly);
        void setAdvanced(bool on);
        void cfgTagInfo(const string& key, bool cppOnly,
                        MachineConfigDraft::Tag& tag, string& tip) const;
        void addBlock(wxWindow* page, wxSizer* sizer, const string& id,
                      const string& title, wxWindow* scroll = NULL);
        void relayout(const BlockRow& b);
        wxStaticText* addBoxHint(wxTextCtrl* t);
        void setBoxHint(wxTextCtrl* t, const string& text);
        void refreshHints();
        string blockExample(const BlockRow& b) const;
        string blockKey(const string& id) const;
        void addCodeLine(wxWindow* page, wxFlexGridSizer* grid,
                         const string& id, const string& label,
                         const string& suffix, const string& tip);
        CodeLine* codeLine(const string& id);
        string codeLineKey(const CodeLine& l) const;
        void codeLineToControl(CodeLine& l);
        void codeLinesToControls();
        void codeLinesTags();
        void codeLineUndo(const string& id);
        void syncCodeLines(MachineConfigDraft* draft);
        void fillCodeList();
        void sortCodeShown();
        void showCode();
        void selectCode(int index);
        bool selectCodeByName(const string& name);
        bool codeInUse(const string& name) const;
        void setCodeAdvanced(bool on);
        void trackFocus(wxTextCtrl* t);
        wxTextCtrl* insertTarget(string& where) const;
        bool insertWord(const string& word);
        BlockRow* block(const string& id);
        string headerKey() const;
        void blocksToControls();
        void blockToControl(BlockRow& b);
        void blocksTags();
        void syncBlocks(MachineConfigDraft* draft);
        void syncBlock(MachineConfigDraft* draft, const BlockRow& b);
        void blockCopySite(const string& id);
        void blockUndo(const string& id);
        void blockNone(const string& id);
        void blocksRetarget();
        void blocksCheckCsh();
        void editFile();
        void showWords();
        wxHyperlinkCtrl* wordsLink(wxWindow* page);
        string rawFileText() const;
        string rawFileCheck(const string& text, bool& errors) const;
        bool rawFileSave(const string& text, string& err);
        void rawFileClosed(bool saved);
        wxBitmapButton* makeUndo(wxWindow* page, wxWindow*& box);
        int jobsIndex() const;
        void jobsToRadios();
        void jobsLine();
        void jobsFromRadios();

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
        //  Test hook: the Advanced dialog, which is shown without a modal loop.
        bool rawDialogOpen() const { return p_rawDlg != NULL; }
        void reg(const string& name, wxWindow* w);

        wxNotebook          *p_book;
        wxListCtrl          *p_list;
        wxPanel             *p_info;
        wxStaticText        *p_infoText;
        wxStaticText        *p_storeNote;   // names the files Save writes
        wxStaticText        *p_jobNote;
        vector<CfgRow>       p_cfgRows;
        vector<BlockRow>     p_blocks;
        wxSizer             *p_condorGrid;
        wxWindow            *p_jobPage;
        wxTimer             *p_cshTimer;
        wxDialog            *p_wordsDlg;
        wxDialog            *p_rawDlg;     // Advanced: edit file
        wxCheckBox          *p_jobsCheck[3];   // [1] interactive, [2] files only
        wxStaticText        *p_jobsTag, *p_jobsMode;
        wxStaticBitmap      *p_jobsIcon;
        wxWindow            *p_jobsUndoBox;
        wxBitmapButton      *p_jobsUndo;
        wxWindow            *p_cfgPage;
        wxPanel             *p_advanced;
        wxButton            *p_advBtn;
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
        vector<CodeLine>     p_codeLines;
        vector<std::pair<wxTextCtrl*, wxStaticText*> > p_hints;
        vector<int>          p_codeShown;   // list row -> p_codeNames index
        int                  p_codeSel;
        wxListBox           *p_codeList;
        wxStaticText        *p_codeTitle;
        wxWindow            *p_codePage;
        wxPanel             *p_codeAdvanced;
        wxButton            *p_codeAdvBtn;
        wxTextCtrl          *p_lastText;    // the box last typed in
        wxListCtrl          *p_wordsList;
        wxButton            *p_wordsInsert;
        wxStaticText        *p_wordsWhere;
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
