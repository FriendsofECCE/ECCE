/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#ifndef _EWX_FILECTRL_H_
#define _EWX_FILECTRL_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ewxFileCtrl.H"
#endif

#include "wx/wx.h"
#include "wx/generic/filedlgg.h"

/**
 *
 */
class ewxFileCtrl : public wxFileCtrl
{
public:
  ewxFileCtrl();
  ewxFileCtrl( wxWindow *win,
              wxWindowID id,
              const wxString &wild,
              bool showHidden,
              const wxPoint &pos = wxDefaultPosition,
              const wxSize &size = wxDefaultSize,
              long style = wxLC_LIST,
              const wxValidator &validator = wxDefaultValidator,
              const wxString &name = wxT("filelist") );
  virtual ~ewxFileCtrl();

  virtual void ShowHidden( bool show = true );

  virtual void UpdateFiles();
  virtual void MakeDir();
  virtual void GoToParentDir();
  virtual void GoToHomeDir();
  virtual void GoToDir( const wxString &dir );

  bool local;
  bool ignoreChanges;

protected:

private:
  DECLARE_DYNAMIC_CLASS(ewxFileCtrl)
};

#endif // _EWX_FILECTRL_H_

