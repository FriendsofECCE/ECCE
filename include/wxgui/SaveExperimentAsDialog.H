#ifndef SAVEEXPERIMENTASDIALOG_H_
#define SAVEEXPERIMENTASDIALOG_H_

#include <string>
  using std::string;

#include "wxgui/ewxGenericFileDialog.H"

class Fragment;
class wxBitmapComboBox;

/**
 * Creates a new file based on either a chem file type or an ECCE type.
 * Also supports selection of an image type, but does not natively
 * support image rendering.  Call canSave to determine if the user
 * selected a type that this dialog can handle.  If not, the calling
 * app must determine the file type/ext and handle approperiately.
 */
class SaveExperimentAsDialog : public ewxGenericFileDialog
{
  DECLARE_EVENT_TABLE()
  DECLARE_CLASS(SaveExperimentAsDialog)

  public:
    SaveExperimentAsDialog();
    SaveExperimentAsDialog(wxWindow *parent);
    virtual ~SaveExperimentAsDialog();

    bool Create(wxWindow *parent);

    bool canSave() const;
    bool doSave(Fragment *fragment, string& context) const;
    void notifyCreate(const string& url) const;

    // ewxGenericFileDialog virtuals
    virtual wxString getType() const;
    virtual wxString getExt() const;

    virtual void EndModal(int retCode);

    void setSaveAsFilterIndex(int index);
    int getImageIndex() const { return p_imageIndex; }
    int getChemFileIndex() const { return p_chemFileIndex; }
    wxString getExts() const;

    // event handlers
    void OnChoiceSaveAsFilter(wxCommandEvent& event);

  protected:
    bool doSaveAsChemFileType(Fragment *fragment, string& context) const;
    bool doSaveAsChemistryTask(Fragment *fragment, string& context) const;
    ChemistryTask* doSaveMdStudy(Session *session) const;
    ChemistryTask* doSaveReactionStudy(Session *session) const;

    void DoSetSaveAsFilterIndex(int index);

    virtual void saveSettings();
    virtual void restoreSettings();

  private:
    wxBitmapComboBox *p_bitmapCombo;
    int p_imageIndex;
    int p_chemFileIndex;
    map<wxString, ResourceType*> p_nameToType;
};

#endif // SAVEEXPERIMENTASDIALOG_H_
