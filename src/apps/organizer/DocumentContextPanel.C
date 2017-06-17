/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#include "util/TempStorage.H"
#include "util/SFile.H"
#include "util/StringConverter.H"

#include "dsm/EcceDAVClient.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "DocumentContextPanel.H"

class CalcMgr;

/**
 * Constructor.
 */
DocumentContextPanel::DocumentContextPanel(CalcMgr * calcMgr,
                                           WxResourceTreeItemData *itemData, 
                                           wxWindow *parent, 
                                           wxWindowID id,
                                           const wxPoint& pos,
                                           const wxSize& size, 
                                           long style):
  DocumentContextPanelGUI(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  p_textDisplay->SetValidator(wxDefaultValidator);
  p_textDisplay->SetFont(ewxStyledWindow::getMonoSpaceFont());

  p_itemData = itemData;

  refresh();
}



/**
 * Destructor.
 */
DocumentContextPanel::~DocumentContextPanel()
{
  p_itemData = 0;
}


/**
 * Refresh function to reload image without recreate the panel
 */
void DocumentContextPanel::refresh()
{
  p_textDisplay->Show(false);
  // Get file size in K
  int size = p_itemData->getResource()->getSize() / 1024;
  // Get size limit in K
  int sizeLimit = 10240;
  char * sizeLimitStr = getenv("ECCE_FILESIZE_WARNING");
  if (sizeLimitStr)
    StringConverter::toInt(sizeLimitStr, sizeLimit);

  bool showContent = true;
  if (size > sizeLimit) {
    string msg = "The large size of this file (" + 
      StringConverter::toString(size) + "K)\n"
      "may result in a long wait before it is loaded.\n\n"
      "Do you still want to view the file?";
    ewxMessageDialog dlg(this, msg, "View Large File?",
                         wxOK|wxCANCEL|wxICON_QUESTION, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_CANCEL) {
      showContent = false;
      p_textDisplay->SetValue("The file is too large to be displayed in the Organizer.\n"
                              "Please download the file to local disk if you wish to view it.\n");
    }
  }

  if (showContent) {
    SFile *content = TempStorage::getTempFile();
    content = p_itemData->getResource()->getDocument(content);
    if (content != 0) {
      p_textDisplay->LoadFile(content->path());
      content->remove();
      delete content;
    }
    else {
      p_textDisplay->SetValue("The file could not be displayed in the Organizer.\n");
    }
  }

  p_textDisplay->Show(true);
}
