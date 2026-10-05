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
#include <string.h>
#include <algorithm>

namespace {

void flipRows(vector<unsigned char>& rgb, int w, int h)
{
  const size_t row = (size_t)w * 3;
  for (int y = 0; y < h / 2; y++)
    std::swap_ranges(rgb.begin() + y * row, rgb.begin() + (y + 1) * row,
                     rgb.begin() + (size_t)(h - 1 - y) * row);
}

// Offscreen renderer output (Coin writeToRGB): big-endian header, one plane
// per channel, rows bottom-up, verbatim or RLE.  Returns top-down RGB.
void decodeSGI(const vector<unsigned char>& f, const string& name,
               int& w, int& h, vector<unsigned char>& rgb)
{
  const string bad = "Image " + name + " is a damaged SGI image";
  if (f.size() < 512) throw EcceException(bad, WHERE);
  int storage = f[2], bpc = f[3];
  int dim = (f[4] << 8) | f[5];
  w = (f[6] << 8) | f[7];
  h = (f[8] << 8) | f[9];
  int z = (f[10] << 8) | f[11];
  if (dim < 2) h = 1;
  if (dim < 3) z = 1;
  if (bpc != 1) {
    throw EcceException("Image " + name + " has " + char('0' + bpc) +
                        " bytes per channel; only 8-bit SGI images are "
                        "supported", WHERE);
  }
  if (w <= 0 || h <= 0 || (z != 1 && z != 3 && z != 4) || storage > 1) {
    throw EcceException(bad, WHERE);
  }
  const int chans = z >= 3 ? 3 : 1;   // alpha is dropped

  vector<unsigned char> planes((size_t)w * h * chans);
  vector<unsigned char> row(w);
  for (int c = 0; c < chans; c++) {
    for (int y = 0; y < h; y++) {
      size_t idx = (size_t)c * h + y;
      if (storage == 0) {
        size_t off = 512 + idx * w;
        if (off + w > f.size()) throw EcceException(bad, WHERE);
        memcpy(&row[0], &f[off], w);
      } else {
        size_t t = 512 + idx * 4;
        if (t + 4 > f.size()) throw EcceException(bad, WHERE);
        size_t off = ((size_t)f[t] << 24) | (f[t+1] << 16) | (f[t+2] << 8) | f[t+3];
        int x = 0;
        while (true) {
          if (off >= f.size()) throw EcceException(bad, WHERE);
          int b = f[off++];
          int n = b & 0x7f;
          if (n == 0) break;
          if (x + n > w) throw EcceException(bad, WHERE);
          if (b & 0x80) {
            if (off + n > f.size()) throw EcceException(bad, WHERE);
            memcpy(&row[x], &f[off], n);
            off += n;
          } else {
            if (off >= f.size()) throw EcceException(bad, WHERE);
            memset(&row[x], f[off++], n);
          }
          x += n;
        }
        if (x != w) throw EcceException(bad, WHERE);
      }
      // y = 0 is the bottom scanline
      memcpy(&planes[((size_t)c * h + (h - 1 - y)) * w], &row[0], w);
    }
  }

  rgb.resize((size_t)w * h * 3);
  for (size_t i = 0; i < (size_t)w * h; i++)
    for (int k = 0; k < 3; k++)
      rgb[i * 3 + k] = planes[(size_t)(chans == 3 ? k : 0) * w * h + i];
}

}  // namespace



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

  if (depth != 8) {
    throw BadValueException("Unsupported image depth", WHERE);
  }

  vector<unsigned char> bytes;
  {
    FILE *fp = fopen(inFile.c_str(), "rb");
    if (!fp) throw EcceException("Image " + inFile + " is missing", WHERE);
    unsigned char buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0) bytes.insert(bytes.end(), buf, buf + n);
    fclose(fp);
  }

  int w = width, h = height;
  vector<unsigned char> rgb;
  if (w > 0 && h > 0 && bytes.size() == (size_t)w * h * 3) {
    // The vendored viewer writes bare bottom-up rows; an SGI file always
    // has a 512 byte header, so the size cannot be confused with this.
    rgb = bytes;
    flipRows(rgb, w, h);
  } else if (bytes.size() >= 2 && bytes[0] == 0x01 && bytes[1] == 0xDA) {
    decodeSGI(bytes, inFile, w, h, rgb);
  } else {
    throw EcceException("Image " + inFile + " is not an SGI image or "
                        "raw RGB of the expected size", WHERE);
  }

  // wxImage takes ownership of data.
  unsigned char *data = (unsigned char*)malloc(rgb.size());
  memcpy(data, &rgb[0], rgb.size());
  wxImage image(w, h, data, false);

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
