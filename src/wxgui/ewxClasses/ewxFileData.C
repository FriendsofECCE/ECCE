/**
 * @file
 *
 * @author Jeff Daily
 *
 */

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ewxFileData.H"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_FILEDLG

#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "util/ResourceUtils.H"
#include "util/TDateTime.H"
#include "wxgui/ewxFileData.H"
#include "wxgui/WxResourceImageList.H"

/**
 *
 */
ewxFileData::ewxFileData() :
  wxFileData()
{
  init();
}


/**
 * Full copy constructor
 */
ewxFileData::ewxFileData( const ewxFileData& fileData )
{
  Copy(fileData);
}


/**
 *
 */
ewxFileData::ewxFileData( Resource * resource ) :
  wxFileData()
{
  init();
  p_resource = resource;
  ReadData();
}


/**
 * make a full copy of the other ewxFileData
 */
void ewxFileData::Copy( const ewxFileData &other )
{
  wxFileData::Copy(other);
  p_resource = other.p_resource;
}


/**
 * (re)read the extra data about the file from the system
 */
void ewxFileData::ReadData()
{
  m_fileName    = p_resource->getName();
  m_filePath    = p_resource->getURL().toString();
  m_size        = p_resource->getSize();
  m_dateTime    = wxDateTime(p_resource->getModifiedDate()->toSeconds());
  m_permissions = wxT("");
  m_image       = WxResourceImageList::getImageIndex(p_resource);

  switch (p_resource->getResourceType()) {
    case ResourceDescriptor::RT_COLLECTION:
      m_type = wxFileData::is_dir;
      break;
    case ResourceDescriptor::RT_VIRTUAL_DOCUMENT:
      switch (p_resource->getContentType()) {
        case ResourceDescriptor::CT_SESSION:
          m_type = wxFileData::is_dir;
          break;
        default:
          if (Resource::isHideInternal())
            m_type = wxFileData::is_file;
          else
            m_type = wxFileData::is_dir;
          break;
      }
      break;
    default:
      m_type = wxFileData::is_file;
      break;
  }
}


/**
 * Set the path + name and name of the item
 * TODO
 */
void ewxFileData::SetNewName( const wxString &filePath,
                              const wxString &fileName )
{
  // do nothing
}


/**
 * Get the time as a formatted string
 */
wxString ewxFileData::GetModificationTime() const
{
  return p_resource->getModifiedDate()->toString();
}


/**
 * operators
 */
ewxFileData& ewxFileData::operator = (const ewxFileData& fd)
{
  Copy(fd);
  return *this;
}


/**
 *
 */
void ewxFileData::init()
{
  p_resource = NULL;
}

#endif // wxUSE_FILEDLG

