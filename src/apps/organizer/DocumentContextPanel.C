/**
 * @file
 *
 * @author Jeff Daily
 *
 */


#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "wxgui/ewxPanel.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "DocumentContextPanel.H"
#include "TextFileReader.H"
#include <memory>

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
  std::unique_ptr<EDSI> source(EDSIFactory::getEDSI(p_itemData->getUrl()));
  TextFileReader reader;
  wxString preview;
  if (!source || !reader.tail(*source))
    preview = "Unable to read file. " + wxString::FromUTF8(reader.error);
  else if (reader.binary)
    preview = wxString::Format("Binary file: %lu bytes. Double-click or choose View for details.", reader.size);
  else {
    preview = "Double-click or choose View for Load more, Find, Follow and Open in editor.\n\n";
    wxString text = wxString::FromUTF8(reader.text.data(), reader.text.size());
    if (text.empty() && !reader.text.empty())
      text = wxString(reader.text.data(), wxConvISO8859_1, reader.text.size());
    preview += text;
  }
  p_textDisplay->ChangeValue(preview);
  p_textDisplay->ShowPosition(p_textDisplay->GetLastPosition());
}
