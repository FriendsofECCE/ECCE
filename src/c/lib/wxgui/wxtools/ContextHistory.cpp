#include <algorithm>
  using std::find;

#include "wx/menu.h"

#include "wxgui/ContextHistory.H"


ContextHistory::ContextHistory(size_t maxFiles, wxWindowID idBase,
                               bool useNumbers)
  : wxFileHistory(maxFiles, idBase),
    p_context(""),
    p_useNumbers(useNumbers)
{
}


ContextHistory::~ContextHistory()
{
}


/**
 * Sets context based on existing wxMenuItem ID.
 * Looks up label of given menu item ID and forwards to SetContext(wxString).
 */
bool ContextHistory::SetContext(const wxWindowID& id)
{
  return SetContext(GetHistoryFile(id - GetBaseId()));
}


/**
 * Adds and sets context to menu, or just sets context if it already exists.
 * @param context to add
 * @return false if not able (reached max size)
 */
bool ContextHistory::SetContext(const wxString& context)
{
  if (context.IsEmpty() || context.IsSameAs("")) {
    return false; // bail
  }

  p_context = context;

  // Check we don't already have this file
  for (size_t i = 0; i < m_fileHistoryN; i++)
  {
    if ( m_fileHistory[i] && ( context == m_fileHistory[i] ) )
    {
      // we do have it, so return
      RefreshNames();
      return true;
    }
  }

  // We don't have it, so add it to the list
  AddFileToHistory(context);

  RefreshNames();
  return true;
}


/**
 * Removes context from menu, if it exists.
 * @param context to remove
 * @return false if not able (did not exists in list)
 */
bool ContextHistory::RemoveContext(const wxString& context)
{
  // Check if we have this context
  for (size_t i = 0; i < m_fileHistoryN; i++)
  {
    if ( m_fileHistory[i] && ( context == m_fileHistory[i] ) )
    {
      // we do have it, so remove it
      RemoveFileFromHistory(i);
      RefreshNames();
      return true;
    }
  }
  return false;
}


bool ContextHistory::RenameContext(const wxString& oldName,
                                   const wxString& newName)
{
  // Check if we have this context
  for (size_t i = 0; i < m_fileHistoryN; i++)
  {
    if ( m_fileHistory[i] && ( oldName == m_fileHistory[i] ) )
    {
      // we do have it, so rename it
      m_fileHistory[i] = newName.char_str();
      RefreshNames();
      return true;
    }
  }
  return false;
}


void ContextHistory::RefreshNames()
{
  wxList::compatibility_iterator node = m_fileMenus.GetFirst();
  while ( node )
  {
    wxMenu* menu = (wxMenu*) node->GetData();

    // refresh filenames
    wxString buf;
    for ( size_t j = 0; j < m_fileHistoryN; j++ )
    {
      if (m_fileHistory[j] == p_context) {
        if (p_useNumbers)
          buf.Printf("&%zu *%s", j + 1, m_fileHistory[j]);
        else
          buf.Printf("*%s", m_fileHistory[j]);
      } else {
        if (p_useNumbers)
          buf.Printf("&%zu %s", j + 1, m_fileHistory[j]);
        else
          buf.Printf("%s", m_fileHistory[j]);
      }
      menu->SetLabel(GetBaseId() + j, buf);
    }

    node = node->GetNext();
  }
}

