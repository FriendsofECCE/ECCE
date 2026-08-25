/**
 * @file
 *
 * wx2.8's wxGenericFileDialog (the composite, all-platforms file dialog
 * built from a wxFileListCtrl + text/choice controls) no longer exists in
 * wx3.2 - modern wxFileDialog is backed by native platform dialogs instead,
 * and there's no public class left to subclass for a custom, composited
 * dialog like this one.
 *
 * ECCE's ewxGenericFileDialog is not just cosmetic: it lets a user pick
 * which EDSI server (local filesystem, or one of the configured remote
 * compute resources) to browse via p_serverChoice, so this dialog now owns
 * its own layout and widgets directly instead of inheriting them from a
 * base-class implementation, building on wxFileDialogBase (which is still
 * present in wx3.2 and gives us m_dir/m_fileName/m_path/m_wildCard/
 * m_filterIndex/HasFdFlag()/AppendExtension()) the same way wx2.8's own
 * wxGenericFileDialog did internally.
 */

#ifndef _EWX_GENERICFILEDIALOG_H_
#define _EWX_GENERICFILEDIALOG_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ewxGenericFileDialog.H"
#endif

#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/filedlg.h"

#include "util/EcceURL.H"

class ewxChoice;
class ewxFileCtrl;
class wxChoice;

class ewxGenericFileDialog : public wxFileDialogBase
{
  class boolGlobalHelper {
    public:
      boolGlobalHelper() { otherValue = &myValue; }
      void setOtherBool(bool * other) { *(otherValue = other) = myValue; }
      operator bool() { return myValue; }
      bool operator =(bool value) { return myValue = *otherValue = value; }
    private:
      bool   myValue;
      bool * otherValue;
  };

  public:
    ewxGenericFileDialog();

    ewxGenericFileDialog(wxWindow * parent,
                        const wxString& message = wxFileSelectorPromptStr,
                        const EcceURL& defaultUrl = EcceURL(),
                        const wxString& wildCard = wxFileSelectorDefaultWildcardStr,
                        long style = 0,
                        const wxPoint& pos = wxDefaultPosition,
                        bool bypassGenericImpl = false );

    bool Create( wxWindow *parent,
                 const wxString& message = wxFileSelectorPromptStr,
                 const wxString& defaultDir = wxEmptyString,
                 const wxString& defaultFile = wxEmptyString,
                 const wxString& wildCard = wxFileSelectorDefaultWildcardStr,
                 long style = 0,
                 const wxPoint& pos = wxDefaultPosition,
                 bool bypassGenericImpl = false );

    virtual ~ewxGenericFileDialog();

    virtual void SetPath(const wxString& path);
    virtual void SetWildcard(const wxString& wildCard);
    void setServerChoice( int serverIndex );
    void setServerChoice( string dir );

    virtual wxString getType() const;
    virtual wxString getExt() const;
    virtual wxString getSelectedWildcardLabel() const;
    virtual vector<wxString> getSelectedWildcard() const;

    void OnSelected( wxListEvent &event );
    void OnActivated( wxListEvent &event );
    void OnHome( wxCommandEvent &event );
    void OnNewDir( wxCommandEvent &event );
    void OnUpDir( wxCommandEvent &event );
    void OnOk( wxCommandEvent &event );
    void OnTextEnter( wxCommandEvent &event );
    void onServerChoice( wxCommandEvent &event );

    virtual void HandleAction( const wxString &fn );
    virtual void UpdateControls();
    virtual void EndModal(int retCode);

  protected:
    void doSetServerChoice( int serverIndex );
    void goToHomeDir();
    void createControls( bool bypassGenericImpl );

    ewxChoice        * p_serverChoice;
    vector<string>     p_mountDir;
    vector<string>     p_lastDir;
    boolGlobalHelper   local;
    boolGlobalHelper   ignoreChanges;

    // Widgets this dialog now owns and lays out itself (these stand in for
    // the protected members ewxGenericFileDialog's subclasses used to
    // inherit from wxGenericFileDialog - kept under the same names so
    // SaveExperimentAsDialog/ExportTableDialog/OpenCalculationDialog don't
    // need to change how they reach them).
    ewxFileCtrl       * m_list;
    wxChoice          * m_choice;      // wildcard filter chooser
    wxTextCtrl        * m_text;        // filename entry
    wxStaticText      * m_static;      // current directory label
    wxButton          * m_upDirButton;
    wxString            m_filterExtension;

    static long          ms_lastViewStyle;
    static bool          ms_lastShowHidden;

    void init();
    virtual void initServerChoices();
    void cleanupChoices(wxChoice * choice);
    void clearChoices(wxChoice * choice);
    bool fileExists(wxString filename);
    bool dirExists(wxString filename);
    virtual void saveSettings();
    virtual void restoreSettings();

  private:
    DECLARE_DYNAMIC_CLASS(ewxGenericFileDialog)
    DECLARE_EVENT_TABLE()

};

#endif // _EWX_GENERICFILEDIALOG_H_
