/**
 * @file
 *
 *
 */
#ifndef WXFFDATACONFIG_H_
#define WXFFDATACONFIG_H_


#include "util/IOException.H"

#include "wxgui/WxFFDataConfigGUI.H"


class EDSIServerCentral;


/**
 * 
 */
class WxFFDataConfig : public WxFFDataConfigGUI
{

  public:

    WxFFDataConfig(wxWindow * parent);
    virtual ~WxFFDataConfig();

    string         getFFName();
    vector<string> getFFNames();
    vector<string> getFFPaths();
    vector<string> getSegPaths();

    vector<string> getFFPaths(string ffName);
    vector<string> getSegPaths(string ffName);

    void loadPreferences(const bool& usePrefs=true) throw (IOException);
    void savePreferences();

  protected:

    void savePreferences(Preferences& prefs);

    void doChoiceForceFieldSelected(string selection);
    void doButtonRemoveClick(int id);
    void doButtonUpClick(int id);
    void doButtonDownClick(int id);

    void OnCloseWindow( wxCloseEvent& event );
    void OnChoiceForceFieldSelected( wxCommandEvent& event );
    void OnButtonFfBrowseClick( wxCommandEvent& event );
    void OnButtonFfRemoveClick( wxCommandEvent& event );
    void OnButtonFfUpClick( wxCommandEvent& event );
    void OnButtonFfDownClick( wxCommandEvent& event );
    void OnButtonSegBrowseClick( wxCommandEvent& event );
    void OnButtonSegRemoveClick( wxCommandEvent& event );
    void OnButtonSegUpClick( wxCommandEvent& event );
    void OnButtonSegDownClick( wxCommandEvent& event );
    void OnButtonDefaultsClick( wxCommandEvent& event );
    void OnCloseClick( wxCommandEvent& event );
    void OnHelpClick( wxCommandEvent& event );

  private:

    string validate(const string& name, const vector<string>& paths);

    map<string, vector<string>, less<string> > p_ffPaths;
    map<string, vector<string>, less<string> > p_segPaths;

    static string p_appName;

};

#endif // WXFFDATACONFIG_H_

