#ifndef _SOLVATEED_H_
#define _SOLVATEED_H_

#include <string>
  using std::string;

#include "wx/wx.h"

#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"

#include "wxgui/CalcDropHandler.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/WxDavAuth.H"

#include "SolvateEdGUI.H"

class EcceTool;
class Fragment;
class ICalculation;

class SolvateEd: public SolvateEdGUI,
              public WxDavAuth,
              public JMSPublisher,
              public FeedbackSaveHandler,
              public CalcDropHandler
{
  DECLARE_CLASS( SolvateEd )
  DECLARE_EVENT_TABLE()

  public:
    SolvateEd( );
    SolvateEd( wxWindow* parent, wxWindowID id = SYMBOL_SOLVATEEDGUI_IDNAME,
            const wxString& caption = SYMBOL_SOLVATEEDGUI_TITLE,
            const wxPoint& pos = SYMBOL_SOLVATEEDGUI_POSITION,
            const wxSize& size = SYMBOL_SOLVATEEDGUI_SIZE,
            long style = SYMBOL_SOLVATEEDGUI_STYLE );
    ~SolvateEd( );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SOLVATEEDGUI_IDNAME,
                 const wxString& caption = SYMBOL_SOLVATEEDGUI_TITLE,
                 const wxPoint& pos = SYMBOL_SOLVATEEDGUI_POSITION,
                 const wxSize& size = SYMBOL_SOLVATEEDGUI_SIZE,
                 long style = SYMBOL_SOLVATEEDGUI_STYLE );

    void setContext(const string& url);
    string getContext() const;
    void setEditStatus(WxFeedback::EditStatus status);
    Fragment *getFrag() const;
    WxFeedback *getFeedback();

    // FeedbackSaveHandler virtuals
    virtual void processSave();

    // CalcDropHandler virtuals
    virtual void processDrop(const string& url);

    // Event handlers
    virtual void OnToolClick(wxCommandEvent &event);
    virtual void OnCloseWindow( wxCloseEvent& event );
    virtual void OnSaveClick( wxCommandEvent& event );
    virtual void OnTextctrlSlvRadEnter( wxCommandEvent& event );
    virtual void OnTextctrlSlvMinNxEnter( wxCommandEvent& event );
    virtual void OnTextctrlSlvMinNyEnter( wxCommandEvent& event );
    virtual void OnTextctrlSlvMinNzEnter( wxCommandEvent& event );
    virtual void OnExitClick( wxCommandEvent& event );
    virtual void OnHelpClick( wxCommandEvent& event );
    virtual void OnMenuFeedbackClick( wxCommandEvent& event );
    virtual void OnButtonSolvateEdSolvateClick( wxCommandEvent& event );
    virtual void OnButtonSolvateEdClearSlvClick( wxCommandEvent& event );

    // Calc/Project change message/event callbacks (MCB)
    void moveMCB(wxCommandEvent& event);
    void deleteMCB(wxCommandEvent& event);
    void stateMCB(wxCommandEvent& event);
    void subjectMCB(wxCommandEvent& event);

    // Sub-dialogs need to be able to call this
    void enableSave(const bool& enable=true);

  protected:
    void CreateControls();
    void replaceBuilderButton();
    void fixBuilderButtonId();
    void createSolventChoices();

    void doSetContext();
    void freeContext();

    void requestBuilder(const string& actionTopic="");

    // refresh & enable/disable & show/hide indicated field sets
    void updateChemSysFields();
    
    // refresh indicated field values
    void refreshChemSysFields();
    void refreshChemSysThumb();
    void refreshSystemDims(bool show);

    // enable/disable indicated field sets
    void enableChemSysFields();

    // show/hide indicated field sets
    void showAllFields();
    void showChemSysFields();

    void urlSubjectNotify() const;
    void setRunState(ResourceDescriptor::RUNSTATE state) const;

    wxWindowID doClose(const bool& allowCancel=true);
    void doSave();

    void startApp(int id, int force, const string& url) const;
    void startApp(const string& app, int force, const string& url) const;
    
  private:
    WxFeedback *p_feedback;
    EcceTool   *p_builderTool;

    ICalculation *p_iCalc;
    Fragment     *p_frag;
    Fragment     *p_fullFrag;

    string p_context;

    double p_scaleRadius;
    int    p_nx, p_ny, p_nz;
};

#endif // _SOLVATEED_H_
