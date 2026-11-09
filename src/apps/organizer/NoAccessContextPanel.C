/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#include "NoAccessContextPanel.H"
#include "dsm/Resource.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "util/Filesystem.H"
#include <system_error>

namespace {

// Where a local folder of this name is now, if it was only moved down into
// a sibling folder (the usual move made outside ECCE); empty if not found.
std::string movedTo(const EcceURL& url)
{
  namespace fs = ecce::fs;
  std::error_code ec;
  fs::path gone(url.getPath());
  fs::path name = gone.filename();
  if (name.empty()) name = gone.parent_path().filename();
  fs::path parent = fs::path(url.getPath()).parent_path();
  for (fs::directory_iterator it(parent, ec), end; !ec && it != end;
       it.increment(ec)) {
    fs::path candidate = it->path() / name;
    if (it->is_directory(ec) && fs::exists(candidate, ec))
      return candidate.string();
  }
  return "";
}

}


/**
 * Constructor with default parameters.
 */
NoAccessContextPanel::NoAccessContextPanel( CalcMgr * calcMgr,
                                            WxResourceTreeItemData *itemData,
                                            wxWindow *parent,
                                            wxWindowID id,
                                            const wxPoint& pos,
                                            const wxSize& size,
                                            long style ):
  NoAccessContextPanelGUI(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  // cache the tree item pointer
  p_itemData = itemData;

  Resource *res = itemData->getResource();
  wxString text;
  switch (res->accessFailure()) {
  case Resource::ACCESS_MISSING: {
    EcceURL url = res->getURL();
    text = _("This item is no longer on disk; it was moved, renamed or removed\noutside this window:\n") + wxString::FromUTF8(url.getPath().c_str());
    std::string now = url.isLocal() ? movedTo(url) : std::string();
    if (!now.empty())
      text += _("\nA folder of the same name is now at\n") + wxString::FromUTF8(now.c_str());
    text += _("\nClick the refresh button on the tool bar to update the tree.");
    break;
  }
  case Resource::ACCESS_LOGIN:
    text = _("The data server did not accept the login for this project.\nTo re-enter the password, click on the refresh button\non the tool bar.");
    break;
  default:
    text = _("This item could not be read.");
    if (!res->accessMessage().empty())
      text += wxString("\n") + wxString::FromUTF8(res->accessMessage().c_str());
    break;
  }
  if (p_message) {
    p_message->SetLabel(text);
    // The sizer item was sized for the generated label; size it for this one.
    p_message->InvalidateBestSize();
    p_message->SetMinSize(p_message->GetBestSize());
    Layout();
  }
}

NoAccessContextPanel::~NoAccessContextPanel( )
{
  p_itemData = 0;
}


void NoAccessContextPanel::refresh()
{}

