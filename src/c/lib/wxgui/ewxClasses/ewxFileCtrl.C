/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ewxFileCtrl.H"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_FILEDLG

#include "wx/tokenzr.h"

#include <iostream>
using std::cerr;
using std::endl;

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "wxgui/ewxFileCtrl.H"
#include "wxgui/ewxFileData.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxTextEntryDialog.H"

IMPLEMENT_DYNAMIC_CLASS(ewxFileCtrl,wxFileCtrl)

/**
 *
 */
ewxFileCtrl::ewxFileCtrl() :
  wxFileCtrl()
{
}


/**
 *
 */
ewxFileCtrl::ewxFileCtrl( wxWindow *win,
            wxWindowID id,
            const wxString &wild,
            bool showHidden,
            const wxPoint &pos,
            const wxSize &size,
            long style,
            const wxValidator &validator,
            const wxString &name ) :
  wxFileCtrl(win, id, wild, showHidden, pos, size, style, validator, name)
{
}


/**
 *
 */
ewxFileCtrl::~ewxFileCtrl()
{
}


/**
 *
 */
void ewxFileCtrl::ShowHidden( bool show )
{
  Resource::setHideInternal(!show);
  wxFileCtrl::ShowHidden(show);
}


/**
 *
 */
void ewxFileCtrl::UpdateFiles()
{
  // if local, do the standard filesystem implementation
  if (local) return wxFileCtrl::UpdateFiles();

  // don't do anything before ShowModal() call which sets m_dirName
  if (m_dirName == wxT("*")) return;

  wxBusyCursor bcur; // this may take a while...

  DeleteAllItems();

  wxListItem item;
  item.m_itemId = 0;
  item.m_col = 0;

  Resource * resource = EDSIFactory::getResource(EcceURL(m_dirName));

  if (!resource) {
    resource = EDSIFactory::getResource(EDSIServerCentral::getUserHome(
                                        EcceURL(m_dirName).getEcceRoot()));
    if (!resource) {
      cerr << "resource did not retrieve for: " << m_dirName;
      cerr << "\tnor did default home" << endl;
      return;
    }
  }

  vector<Resource *> * children = resource->getChildren();
  vector<Resource *>::iterator child;

  // Get the directories first (not matched against wildcards)
  for (child = children->begin(); child != children->end(); child++) {
    wxFileData *fd = new ewxFileData( *child );
    if (fd->IsDir()) {
      if (Add(fd, item) != -1)
        item.m_itemId++;
      else
        delete fd;
    } else {
      delete fd;
    }
  }

  // Tokenize the wildcard string, so we can handle more than 1 search pattern
  // in a wildcard.
  wxStringTokenizer tokenWild(m_wild, wxT(";"));
  while (tokenWild.HasMoreTokens()) {
    wxString wild = tokenWild.GetNextToken();
    for (child = children->begin(); child != children->end(); child++) {
      wxFileData *fd = new ewxFileData( *child );
      if (!fd->IsDir() && wxString((*child)->getName()).Matches(wild)) {
        if (Add(fd, item) != -1)
          item.m_itemId++;
        else
          delete fd;
      } else if (!fd->IsDir()
              && wxString((*child)->getDescriptor()->getName()).Matches(wild)){
        // matches "nwchem_es, amica_es, etc"
        if (Add(fd, item) != -1)
          item.m_itemId++;
        else
          delete fd;
      } else {
        delete fd;
      }
    }
  }

  SortItems(m_sort_field, m_sort_foward);
}


/**
 *
 */
void ewxFileCtrl::MakeDir()
{
  if (local) return wxFileCtrl::MakeDir();

  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType * resType = rd.getResourceType(ResourceDescriptor::RT_COLLECTION,
                                              ResourceDescriptor::CT_PROJECT,
                                              ResourceDescriptor::AT_UNDEFINED);
  string name;

  bool showInvalidMsg = false;
  string invalidMsg =  "Invalid name entered.  Please make sure the\n"
    "name only contains the following characters:\n" +
    resType->getNameRule() + "\n\n";

  while (true) {
    name = resType->getLabel();
    string infoStr = "Please enter the name for the new " + name + ":";
    if (showInvalidMsg)
      infoStr = invalidMsg + infoStr;
    ewxTextEntryDialog dialog(this, infoStr, "New " + name + " Name",
                              name, wxOK | wxCANCEL);
    if (dialog.ShowModal() == wxID_OK) {
      name = dialog.GetValue();
      if (resType->isValidResourceName(name))
          break;
      else
        showInvalidMsg = true;
    }
    else
      // Cancel clicked, return without doing anything
      return;
  }

  try {
    Resource * resource = EDSIFactory::getResource(EcceURL(m_dirName));
    Resource * newRes = resource->createChild(name, resType);

    if (newRes == 0)
      throw InvalidException("Null Pointer Resource", WHERE);

    wxFileData *fd = new ewxFileData(newRes);
    wxListItem item;
    item.m_itemId = 0;
    item.m_col = 0;
    long id = Add(fd, item);

    if (id != -1) {
      SortItems(m_sort_field, m_sort_foward);
      id = FindItem(0, (long)fd);
      EnsureVisible(id);
    } else {
      delete fd;
    }
  } catch (InvalidException ex) {
    ewxMessageDialog dlg(this, ex.what(),
                         "Unable to create " + name + " object!",
                         wxOK|wxICON_EXCLAMATION,
                         wxPoint(wxDefaultCoord, wxDefaultCoord));
    dlg.ShowModal();
  }
}


/**
 *
 */
void ewxFileCtrl::GoToParentDir()
{
  if (local) return wxFileCtrl::GoToParentDir();

  wxString fname(EcceURL(m_dirName).getFilePathTail());
  m_dirName = (EcceURL(m_dirName)).getParent().toString();
  UpdateFiles();

  long id = FindItem(0, fname);
  if (id != wxNOT_FOUND) {
    ignoreChanges = true;
    SetItemState(id, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    EnsureVisible(id);
    ignoreChanges = false;
  }
}


/**
 *
 */
void ewxFileCtrl::GoToHomeDir()
{
  if (local) return wxFileCtrl::GoToHomeDir();

  GoToDir(EDSIServerCentral::getUserHome(
          EcceURL(m_dirName.c_str()).getEcceRoot()).toString());
}


/**
 *
 */
void ewxFileCtrl::GoToDir( const wxString &dir )
{
  if (local) return wxFileCtrl::GoToDir(dir);

  m_dirName = dir;
  UpdateFiles();

  ignoreChanges = true;
  SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  ignoreChanges = false;

  EnsureVisible(0);
}

#endif // wxUSE_FILEDLG

