/**
 * @file
 *
 */

#ifndef _EWX_GENERICFILEDIALOG_H_
#define _EWX_GENERICFILEDIALOG_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ewxGenericFileDialog.H"
#endif

#include "wx/wx.h"
#include "wx/generic/filedlgg.h"

#include "util/EcceURL.H"

class ewxChoice;
class ewxFileCtrl;
class wxChoice;

class ewxGenericFileDialog : public wxGenericFileDialog
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
    void OnHome( wxCommandEvent &event );
    void onServerChoice( wxCommandEvent &event );

    virtual void HandleAction( const wxString &fn );
    virtual void UpdateControls();
    virtual void EndModal(int retCode);

  protected:
    void doSetServerChoice( int serverIndex );
    void goToHomeDir();

    ewxChoice        * p_serverChoice;
    vector<string>     p_mountDir;
    vector<string>     p_lastDir;
    boolGlobalHelper   local;
    boolGlobalHelper   ignoreChanges;

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

