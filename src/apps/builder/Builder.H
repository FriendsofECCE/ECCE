#ifndef _BUILDER_H_
#define _BUILDER_H_

#include <map>
  using std::map;
#include <set>
  using std::set;
#include <vector>
  using std::vector;

#include <wx/spinctrl.h> // for wxSpinEvent
#include "inv/draggers/SoDragger.H"
#include "inv/nodes/SoEventCallback.H" 

#include <wx/ewxaui/framemanager.H>

#include "util/CancelException.H"
#include "util/CommandManager.H"
#include "util/Event.H"
  using ecce::Event;
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Listener.H"
  using ecce::Listener;

#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxSlider.H"
#include "wxgui/WxCalcImportClient.H"

#include "wxviz/MouseEventListener.H"
#include "wxviz/MotionListener.H"
#include "wxviz/WxVizTool.H"
#include "wxviz/WxVizToolFW.H"

#include "BuilderGUI.H"

class AtomMeasureAngle;
class AtomMeasureDist;
class AtomMeasureTorsion;
class ContextHistory;
class ContextPanel;
class ewxButton;
class ewxBitmapButton;
class ewxTextCtrl;
class Fragment;
class IPropCalculation;
class Job;
class MiniPerTab;
class PropertyPanel;
class SFile;
class SGContainer;
class SGContainerManager;
class SGViewer;
class StructLib;
class ViewerEvtHandler;
class wxToolBarToolBase;

class Builder: public BuilderGUI,
               public WxDavAuth,
               public JMSPublisher,
               public WxVizToolFW,
               public Listener,
               public MouseEventListener,
               public WxVizTool,
               public FeedbackSaveHandler,
               public MotionListener,
               public WxCalcImportClient
{    
    DECLARE_CLASS( Builder)
    DECLARE_EVENT_TABLE()

   public:

    enum WIDGET_ID {
      ID_TOOLMENU_ITEM = 3000,
      ID_TOOLBARMENU_ITEM = 3100,
      ID_CONTEXT_HISTORY_BASE = 3200,
      ID_PROPERTY_MENU_BASE = 3300,
      ID_TEXTCTRL_CMD = 3400,
      ID_SHOW_CMD,
      ID_MODE_TOOLBAR,
      ID_FILE_TOOLBAR,
      ID_STYLE_TOOLBAR,
      ID_VIEW_TOOLBAR,
      ID_MEASURE_TOOLBAR,
      ID_DRAGGER_TOOLBAR,
    
      ID_MODE_SELECT,
      ID_MODE_ROTATE,
      ID_MODE_TRANSLATE,
      ID_MODE_ZOOM,
      ID_MODE_ATOM,
      ID_MODE_BOND,
      ID_MODE_STRUCTLIB,
      ID_MODE_SHAPE,
    
      ID_TOOL_GOHOME,
      ID_TOOL_SETHOME,
      ID_TOOL_STYLE,
      ID_TOOL_TRANSLATEM,
    
      ID_ROT_X,
      ID_ROT_Y,
      ID_ROT_Z,
    
      ID_CHOICE_VIEWER
    };

    const static string NAME_TOOL_CONTEXT;
    const static string NAME_TOOL_BUILD;
    const static string NAME_TOOL_COORDINATES;
    const static string NAME_TOOL_SELECTION;
    const static string NAME_TOOL_ATOM_TABLE;
    const static string NAME_TOOL_RESIDUE_TABLE;
    const static string NAME_TOOL_SYMMETRY;
    const static string NAME_TOOL_DNA_BUILDER;
    const static string NAME_TOOL_PEPTIDE_BUILDER;
    const static string NAME_TOOL_LOG;
    const static string NAME_TOOL_COMMAND_LINE;
    const static string NAME_TOOL_STRUCTLIB;
    const static string NAME_TOOL_PBC;
    const static string NAME_TOOL_SLICER;

    const static string NAME_TOOLBAR_FILE;
    const static string NAME_TOOLBAR_MODE;
    const static string NAME_TOOLBAR_VIEW;
    const static string NAME_TOOLBAR_STYLE;
    const static string NAME_TOOLBAR_MANIPULATOR;
    const static string NAME_TOOLBAR_MEASURE;

    const static string NAME_MODE_SELECT;
    const static string NAME_MODE_ROTATE;
    const static string NAME_MODE_TRANSLATE;
    const static string NAME_MODE_ZOOM;
    const static string NAME_MODE_ATOM;
    const static string NAME_MODE_BOND;
    const static string NAME_MODE_STRUCTLIB;
    const static string NAME_MODE_SHAPE;
    const static string s_modeText[];

    const static string NAME_LAYOUT_PREFIX;
    const static string NAME_LAYOUT_DEFAULT;
    const static string NAME_LAYOUT_READONLY;
    const static string NAME_LAYOUT_STRUCTLIB;

      Builder( );
      Builder(wxWindow* parent, bool standalne = false,
              wxWindowID id = SYMBOL_BUILDERGUI_IDNAME,
              const wxString& caption = SYMBOL_BUILDERGUI_TITLE,
              const wxPoint& pos = SYMBOL_BUILDERGUI_POSITION,
              const wxSize& size = SYMBOL_BUILDERGUI_SIZE,
              long style = SYMBOL_BUILDERGUI_STYLE);
      bool Create( wxWindow* parent, bool standalne = false,
              wxWindowID id = SYMBOL_BUILDERGUI_IDNAME,
              const wxString& caption = SYMBOL_BUILDERGUI_TITLE,
              const wxPoint& pos = SYMBOL_BUILDERGUI_POSITION,
              const wxSize& size = SYMBOL_BUILDERGUI_SIZE,
              long style = SYMBOL_BUILDERGUI_STYLE);
      virtual ~Builder();

      // wxWindow virtuals
      virtual bool Show(bool show=true);

      // ewxTool virtuals
      virtual void setContext(const string& url, const bool& force=false);
      virtual void saveSettings();
      virtual void restoreSettings();

      void applySceneGraphSettings();

      // WxVizToolFW virtual functions
      virtual bool execute(Command *cmd, bool batch=false);
      virtual CommandManager&  getCommandManager();
      virtual SGContainer&     getSceneGraph();
      virtual SGViewer&        getViewer();
      virtual void showMessage(const string& msg, bool error=false);
      virtual void center();
      virtual bool isAutoCenter() const;
      virtual bool isSelectModeDrag() const;
      virtual bool areLabelsOn() const;

      // Listener virtuals
      virtual void eventMCB(const Event& event);

      // MouseEventListener virtuals
      virtual bool mouseEvent(wxMouseEvent *event);

      // FeedbackSaveHandler virtuals
      virtual void processSave();

      virtual bool writeFragmentToImageFile(SFile *file, wxString type,
                                            wxString ext);
      virtual bool readFragmentFromFile(SFile *file, wxString type,
                                        wxString ext);

      // WxCalcImportClient virtuals
      TaskJob *getContainer(const string& name);
      void importValidationComplete(TaskJob *ipc, bool status, string message);

      SGContainer*    getSG();
      CommandManager* getCM();

      void setStandalone(bool standalone);
      bool isStandalone();

      void showSelectionFromNWChemString(string selection);
      static void selectionChangeCB(void *  data, ChemSelection * sel);
      void processSelectionChange(ChemSelection *sel);
      void setMode(int modeId);
      int getMode();

      void deleteSelection();

      // Measure methods
      void viewPosToScreenPos(int& x, int& y);
      void lengthPopup(int x, int y, AtomMeasureDist *dist);
      void anglePopup(int x, int y, AtomMeasureAngle *angle);
      void torsionPopup(int x, int y, AtomMeasureTorsion *torsion);

      // Main window events
      virtual void mainWindowCloseCB( wxCloseEvent& event );
      virtual void OnSize( wxSizeEvent& event );
      virtual void OnKeyDown( wxKeyEvent& event );
      virtual void rotateTo(float x, float y, float z);

      // File menu events
      virtual void OnNewClick( wxCommandEvent& event );
      virtual void OnOpenClick( wxCommandEvent& event );
      virtual void OnImportChemsysClick( wxCommandEvent& event );
      virtual void OnImportCalcClick( wxCommandEvent& event );
      virtual void OnCloseClick( wxCommandEvent& event );
      virtual void OnSaveClick( wxCommandEvent& event );
      virtual void OnSaveasClick( wxCommandEvent& event );
      virtual void OnSaveThumbClick( wxCommandEvent& event );
      virtual void OnDumpsgClick( wxCommandEvent& event );
      virtual void quitMenuItemClickCB( wxCommandEvent& event );

      // Edit menu events
      virtual void OnUndoClick( wxCommandEvent& event );
      virtual void OnRedoClick( wxCommandEvent& event );
      virtual void OnCutClick( wxCommandEvent& event );
      virtual void OnCopyClick( wxCommandEvent& event );
      virtual void OnPasteClick( wxCommandEvent& event );
      virtual void OnClearClick( wxCommandEvent& event );
      virtual void OnDeleteClick( wxCommandEvent& event );
      virtual void OnSelectallClick( wxCommandEvent& event );
      virtual void OnSelectMoleculeClick( wxCommandEvent& event );
      virtual void OnSelectSolventClick( wxCommandEvent& event );
      virtual void OnSelectBackboneClick( wxCommandEvent& event );
      virtual void OnSelectSidechainsClick( wxCommandEvent& event );
      virtual void OnSelectReverseClick( wxCommandEvent& event );
      virtual void OnUnselectClick( wxCommandEvent& event );

      virtual void OnTextctrlCmdEnter( wxCommandEvent& event );

      virtual void OnRotX( wxSpinEvent& event );
      virtual void OnRotY( wxSpinEvent& event );
      virtual void OnRotZ( wxSpinEvent& event );
              void OnViewerChoice( wxCommandEvent& event );

      virtual void OnShowCmdClick( wxCommandEvent& event );
      virtual void OnToolMenuClick( wxCommandEvent& event );

      virtual void OnPaneClose(ewxAuiManagerEvent& event);
      virtual void OnPaneTakeFocus(ewxAuiManagerEvent& event);
      virtual void OnPaneAddFocus(ewxAuiManagerEvent& event);
      virtual void OnPaneOptions(ewxAuiManagerEvent& event);
      virtual void OnPaneOpen(ewxAuiManagerEvent& event);
      virtual void OnAuiUpdate(ewxAuiManagerEvent& event);

      // Mode events
      virtual void OnModePrevClick( wxCommandEvent& event );
      virtual void OnModeNextClick( wxCommandEvent& event );
      virtual void OnModeClick( wxCommandEvent& event );
      virtual void OnModeElementClick( wxCommandEvent& event );
      virtual void OnModeShapeClick( wxCommandEvent& event );
      virtual void OnModeBondClick( wxCommandEvent& event );

      // Context menu events
              void OnContextRadioClick( wxCommandEvent& event );
              void OnContextListClick( wxListEvent& event );

      // Property menu events
              void OnPropertyMenuClick( wxCommandEvent& event );

      // Help menu events
      virtual void helpSupportMenuitemClick( wxCommandEvent& event );
      virtual void helpBuilderMenuitemClick( wxCommandEvent& event );

      // Tool bar items
      virtual void OnToolStyleClick( wxCommandEvent& event );
      virtual void OnToolTranslatemClick( wxCommandEvent& event );
      virtual void OnToolGohomeClick( wxCommandEvent& event );
      virtual void OnToolSethomeClick( wxCommandEvent& event );
      virtual void OnToolOrientClick( wxCommandEvent& event );

      void pertabPrefsMCB();
      void urlRenameMCB(JMSMessage& msg);
      void urlRemoveMCB(JMSMessage& msg);
      void urlStateMCB(JMSMessage& msg);
      void propertyChangeMCB(JMSMessage& msg);

      void quit(const bool& allowCancel = true);

      static void moveStartCB(void *userData, SoDragger *dragger);
      static void moveEndCB(void *userData, SoDragger *dragger);
      static void moveCB(void *userData, SoDragger *dragger);

      static void rotateStartCB(void *userData, SoDragger *dragger);
      static void rotateEndCB(void *userData, SoDragger *dragger);
      static void rotateCB(void *userData, SoDragger *dragger);

      void interpretCommand(const string& command);

   protected:
      void createToolbar();
      void createMenus();
      void createMainPanel();
      void createRotators(wxWindow *parent);
      void createToolPanels();
      void createReadOnlyIds();
      bool handleCommandsIfLattice(Command *cmd);

      // MotionListener virtual
      void motionChanged(const MotionData& data);
      
      IPropCalculation* createCalculation(const string& url);

      Command * createCommand(const string& input) throw(EcceException);
      Command * createModeCommand(int modeId) throw(EcceException);

      void addToolPanel(wxWindow *panel, const string& name,
                        const bool& readOnlyDisabled=true,
                        const bool& alwaysFixed=true);
      void addToolBar(wxToolBar *toolbar, const string& name);
      void addPropertyPanel(PropertyPanel *panel, const string& name);
      void createPropertyPanel(const string& name);
      void removePropertyPanels(const string& context="");

      void toggleTool(int id, bool isChecked);
      bool getToolState(int id) const;
      int checkResidueEditing() throw(CancelException);

      bool createSolventSoluteStyles(SGFragment& frag);
      bool getPDBOptions(const string& filename, string& altLoc,
                         int& model, string& chain);

      void savePaneLayout(const wxString& layoutName="");
      void loadPaneLayout(const wxString& layoutName, const bool& update=true);
      void loadDefaultPaneLayout(const wxString& layoutName,
                                 const bool& update=true);

      // GUI refresh/update based on current context
      void updateUI();
      void updateSave();
      void updateResource();
      void updatePropertyMenus();
      void updateReadOnly(const bool& loadPaneFlag);
      void updateMenus();
      void updateUndoMenus();
      void updateBondIcon();
      void updateElementIcon();
      void updateShapeIcon();
      void updateForAnyEdit();
      void updateModeText();
      void updateAtomResidueText();
      void updateSelectionText();
      void updateViewerText();

      void reportError(const string& msg);
      void toggleModeButton(int toolId);

      void setUnsavedState(bool changed);
      void doSave();
      void doSaveAs(const bool& imagesOnly=false);
      void doSaveThumb();
      void doClose(const string& context="", const bool& autoOpen=true);

      bool isReadOnly() const;
      bool isReadOnly(IPropCalculation *calc) const;
      bool isDirty(IPropCalculation *calc=NULL);
      void setDirty(const bool& val, IPropCalculation *calc=NULL);
      bool isImport(IPropCalculation *calc=NULL);
      void setImport(const bool& val, IPropCalculation *calc=NULL);

      // JMS utils
      void notifySubject() const;

   private:
      SGContainerManager *p_sgMgr;
      SGViewer           *p_viewer;

      int                 p_currentMode;
      string              p_currentElement;
      map<string, string> p_currentShapes;
      double              p_currentBond;

      wxSpinCtrl * p_xrot;
      wxSpinCtrl * p_yrot;
      wxSpinCtrl * p_zrot;
      ewxButton  * p_viewerButton;
  
      int p_rotx;
      int p_roty;
      int p_rotz;

      /** buffer for cut/copy/paste. */
      Fragment *p_copyBuffer;

      wxToolBar *p_fileToolbar;
      wxToolBar *p_modeToolbar;
      wxToolBar *p_viewToolbar;
      wxToolBar *p_styleToolbar;
      wxToolBar *p_manipulatorToolbar;
      wxToolBar *p_measureToolbar;
      vector<wxToolBar*> p_toolbars;

      wxMenu *p_fileMenu;
      wxMenu *p_editMenu;
      wxMenu *p_optionsMenu;
      wxMenu *p_renderMenu;
      wxMenu *p_buildMenu;
      wxMenu *p_modeMenu;
      wxMenu *p_measureMenu;
      wxMenu *p_toolMenu;
      wxMenu *p_toolbarMenu;
      wxMenu *p_propertyMenu;
      wxMenu *p_contextMenu;
      ContextHistory *p_contextHistory;
      ContextPanel *p_contextPanel;
      vector<wxMenu*> p_menus;

      wxBoxSizer  *p_mainSizer;
      wxBoxSizer  *p_rotSizer;
      ewxTextCtrl *p_cmd;

      ViewerEvtHandler *p_viewerEvtHandler;
      ewxAuiManager p_mgr;

      int p_toolCount;
      int p_toolbarCount;

      MiniPerTab * p_pertab;

      StructLib * p_structLib;

      bool p_standalone;

      set<int> p_readOnlyDisabledIds;

      map<string, IPropCalculation*> p_calculations;
      IPropCalculation *p_calculation;
      map<string, CommandManager*> p_commandManagers;
      CommandManager *p_commandManager;
      map<string, bool> p_dirty;
      map<string, bool> p_import;

      ewxBitmap p_lockBitmap;
      ewxBitmap p_unlockBitmap;
      ewxBitmap p_orthoBitmap;
      ewxBitmap p_perspBitmap;
      ewxBitmapButton * p_centerLockButton;
      ewxBitmapButton * p_cameraButton;

      map<wxWindow*, wxString> p_propertyPanelInfo;
};

#endif
