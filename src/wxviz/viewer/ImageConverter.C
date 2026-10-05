/**
 * @file
 */
#include "wxviz/ImageConverter.H"
#include "util/SFile.H"
#include "util/STLUtil.H"

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/image.h"

#include <stdio.h>
#include <stdlib.h>


ImageConverter::ImageConverter()
{
  // Without this the JPEG/PNG/GIF/TIFF handlers are missing in a process
  // that has no ewxApp (headless thumbnail rendering); repeat calls are cheap.
  wxInitAllImageHandlers();
}

ImageConverter::~ImageConverter()
{
}


void ImageConverter::convert(const string& inFile,
                             const string& outFile,
                             const int& width, const int& height,
                             const int& depth, const bool& rmInFile)
{
  SFile file(outFile);
  string ext = file.extension();
  STLUtil::toLower(ext);

  wxBitmapType type;
  if (ext == "jpg" || ext == "jpeg")      type = wxBITMAP_TYPE_JPEG;
  else if (ext == "png")                  type = wxBITMAP_TYPE_PNG;
  else if (ext == "gif")                  type = wxBITMAP_TYPE_GIF;
  else if (ext == "tif" || ext == "tiff") type = wxBITMAP_TYPE_TIFF;
  else if (ext == "bmp")                  type = wxBITMAP_TYPE_BMP;
  else {
    // wx cannot write Postscript; the viewer writes that itself.
    throw BadValueException(("Unsupported image format for " + outFile +
                             ".  Use .jpg, .png, .gif, .tif or .bmp.").c_str(),
                            WHERE);
  }

  if (depth != 8 || width <= 0 || height <= 0) {
    throw BadValueException("Unsupported raw image size or depth", WHERE);
  }

  size_t count = (size_t)width * height * 3;
  unsigned char *data = (unsigned char*)malloc(count);
  FILE *fp = fopen(inFile.c_str(), "rb");
  size_t got = fp ? fread(data, 1, count, fp) : 0;
  if (fp) fclose(fp);
  if (got != count) {
    free(data);
    throw EcceException("Raw image " + inFile + " is missing or too short",
                        WHERE);
  }

  // wxImage takes ownership of data.  The renderer writes rows bottom-up.
  wxImage image(width, height, data, false);
  image = image.Mirror(false);

  bool ok = image.SaveFile(wxString::FromUTF8(outFile.c_str()), type);
  if (rmInFile) remove(inFile.c_str());
  if (!ok) {
    throw EcceException("Could not write image " + outFile, WHERE);
  }
}


/**
 * Provides list of formats that can be converted TO.
 */
vector<string> ImageConverter::outputFormats()
{
  vector<string> formats;
  formats.push_back("JPEG");
  formats.push_back("PNG");
  formats.push_back("GIF");
  formats.push_back("TIFF");
  return formats;
}
