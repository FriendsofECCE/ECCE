#ifndef RESIDUEINDEXPROMPT_H_
#define RESIDUEINDEXPROMPT_H_

#include "wxviz/ResidueIndexPromptGUI.H"
#include "wxviz/WxVizTool.H"

class ResidueIndexPrompt : public ResidueIndexPromptGUI,
                           public WxVizTool
{
  public:
    ResidueIndexPrompt( );
    ResidueIndexPrompt( wxWindow* parent,
            wxWindowID id = SYMBOL_RESIDUEINDEXPROMPTGUI_IDNAME,
            const wxString& caption = SYMBOL_RESIDUEINDEXPROMPTGUI_TITLE,
            const wxPoint& pos = SYMBOL_RESIDUEINDEXPROMPTGUI_POSITION,
            const wxSize& size = SYMBOL_RESIDUEINDEXPROMPTGUI_SIZE,
            long style = SYMBOL_RESIDUEINDEXPROMPTGUI_STYLE );
    bool Create( wxWindow* parent,
            wxWindowID id = SYMBOL_RESIDUEINDEXPROMPTGUI_IDNAME,
            const wxString& caption = SYMBOL_RESIDUEINDEXPROMPTGUI_TITLE,
            const wxPoint& pos = SYMBOL_RESIDUEINDEXPROMPTGUI_POSITION,
            const wxSize& size = SYMBOL_RESIDUEINDEXPROMPTGUI_SIZE,
            long style = SYMBOL_RESIDUEINDEXPROMPTGUI_STYLE );
    ~ResidueIndexPrompt();

    bool isSelectResidue() const;
    bool isDeleteResidue() const;
    bool isInsertResidue() const;
    long getResidueIndex() const;
    string getResidueName() const;

    void setSelectResidue();
    void setInsertResidue();

    bool Show(const bool show);
    int ShowModal();

  protected:
    bool isRadioSelected(wxWindowID id) const;
    void loadChoices();

    void OnRadiobuttonRespromptSelectSelected( wxCommandEvent& event );
    void OnChoiceResSelectSelected( wxCommandEvent& event );
    void OnRadiobuttonRespromptNewSelected( wxCommandEvent& event );
    void OnChoiceResInsertSelected( wxCommandEvent& event );
    void OnRadiobuttonRespromptDeleteSelected( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    void doRadioSelected(wxWindowID id);
    void doClose(int retCode);

};

#endif // RESIDUEINDEXPROMPT_H_
