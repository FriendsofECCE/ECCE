/**
 * @file
 *
 *
 */
#ifndef _WXPDBPROMPT_H_
#define _WXPDBPROMPT_H_

#include <string>

#include "wxgui/WxPDBPromptGUI.H"
#include "wxgui/ewxStyledWindow.H"


/**
 * Implementation of WxPDBPromptGUI.
 * This class displays options for selecting which model, alternate
 * location, and chain to use from a PDB source file.
 */
class WxPDBPrompt : public WxPDBPromptGUI
{    

public:
    /// Constructors
    WxPDBPrompt( );
    WxPDBPrompt( wxWindow* parent, wxWindowID id = SYMBOL_WXPDBPROMPTGUI_IDNAME,
	             const wxString& caption = SYMBOL_WXPDBPROMPTGUI_TITLE,
		     const wxPoint& pos = SYMBOL_WXPDBPROMPTGUI_POSITION,
		     const wxSize& size = SYMBOL_WXPDBPROMPTGUI_SIZE,
		     long style = SYMBOL_WXPDBPROMPTGUI_STYLE );
    /* Accessors */
    int getPDBModel() const;
    string getAltLocation() const;
    string getChainID() const;

    /* Initialization methods */
    void setModelRange(int models);
    void setAltLocOptions(vector<string> opt);
    void setChainOptions(vector<string> opt);

    /* Call backs */

    virtual void OnButtonOkClick( wxCommandEvent& event );

    virtual void OnButtonCancelClick( wxCommandEvent& event );

    virtual void OnButtonHelpClick( wxCommandEvent& event );


private:

};

#endif // _WXPDBPROMPT_H_
