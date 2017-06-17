/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#ifndef _EWX_FILEDATA_H_
#define _EWX_FILEDATA_H_

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ewxFileData.h"
#endif

#include "wx/wx.h"
#include "wx/generic/filedlgg.h"

class Resource;

/**
 * A class to hold the file info for the wxFileCtrl.
 * Abstracts whether using local filesystem or DAV.
 */
class ewxFileData : public wxFileData
{
public:
  ewxFileData();
  ewxFileData( const ewxFileData& fileData );
  ewxFileData( Resource * resource );

  void Copy( const ewxFileData &other );

  void ReadData();

  void SetNewName( const wxString &filePath, const wxString &fileName );

  wxString GetModificationTime() const;

  ewxFileData& operator = (const ewxFileData& fd);

protected:
  Resource * p_resource;

private:
  void init();
};

#endif // _EWX_FILEDATA_H_

