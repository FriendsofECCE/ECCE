/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#include <iostream>
using std::cerr;
using std::endl;

#include "util/NullPointerException.H"

#include "dsm/EDSIFactory.H"

#include "wxgui/ewxListBox.H"
#include "wxgui/ewxTextCtrl.H"

#include "CalcMgr.H"
#include "WxFind.H"


/**
 * Default constructor which should never be used.
 */
WxFind::WxFind( )
{

}


/**
 * Constructor.
 * All the default parameters are defined in WxFindGUI class.
 */
WxFind::WxFind( wxWindow* parent, 
                wxWindowID id,
                const wxString& caption,
                const wxPoint& pos,
                const wxSize& size,
                long style ):
  WxFindGUI(parent, id, caption, pos, size, style)
{
  FindWindowById(ID_FIND_BUTTON_GO, this)->Enable(false);
}



/**
 * Destructor.
 * Does nothing.  Parent window destroys.
 */
WxFind::~WxFind( )
{
  // do nothing, parent window uses Destroy()
}



/**
 * Used to set the value of where (which directory) the search will begin.
 */
void WxFind::setFrom(const string& value)
{
  p_from->SetValue(_T(value));
}



/**
 * Sets the context panel to whichever item is highlighted in the listbox.
 * Also changes the selection in the tree control to the appropriate item as
 * well.
 */
void WxFind::OnFindButtonGoClick( wxCommandEvent& event )
{
  // get the wxString form of the url selected
  wxString selection = p_results->GetString(p_results->GetSelection());

  if (selection == "") {
    FindWindowById(ID_FIND_BUTTON_GO, this)->Enable(false);
    return;
  }
  
  
  // convert wxString to EcceURL
  EcceURL url(selection.c_str());

  // parent is always CalcMgr
  CalcMgr *calcmgr = dynamic_cast<CalcMgr*>(GetParent());
  NULLPOINTEREXCEPTION(calcmgr, "could not cast from wxWindow to CalcMgr");

  calcmgr->findNode(url);
}



/**
 * Recursively searches through data server and builds list of urls with the 
 * desired substring found.
 */
void WxFind::OnFindButtonFindClick( wxCommandEvent& event )
{
  // declare the vector to hold url search results
  vector<EcceURL> urls;

  // clear the list control, reenable GO button
  p_results->Clear();
  FindWindowById(ID_FIND_BUTTON_GO, this)->Enable(false);

  // get the url to search for descendants from
  EcceURL url = p_from->GetValue().c_str();

  // get the resource from the url
  Resource *resource;
  resource = EDSIFactory::getResource(url);

  // check to see if factory returned a non-null pointer
  if (resource == (Resource*)0) {
    p_results->Append(_T("ERROR: Resource does not exist at given URL"));
  } else {
    // do the actual search
    string key, substr;
    key = "DAV:creationdate";
    substr = p_find->GetValue().c_str();
    urls = resource->descendantSearch(key, substr);
    
    if (urls.empty()) {
      p_results->Append(_T("No results found"));
    } else {
      vector<EcceURL>::iterator itor;
      for (itor = urls.begin(); itor != urls.end(); itor++) {
        wxString str(itor->toString().c_str());
        p_results->Append(str);
      }
    }
  }
}



/**
 * Closes the dialog.
 */
void WxFind::OnFindButtonCloseClick( wxCommandEvent& event )
{
  Show(false);
}



/**
 * Opens the standard help HTML.
 */
void WxFind::OnFindButtonHelpClick( wxCommandEvent& event )
{

}


void WxFind::OnFindResultsSelected( wxCommandEvent& event )
{
  if (p_results->GetString(p_results->GetSelection()) != "No results found")
    FindWindowById(ID_FIND_BUTTON_GO, this)->Enable(true);
}
