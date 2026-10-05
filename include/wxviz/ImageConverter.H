/**
 * @file
 *
 * Converts the raw RGB dump written by RenderFileCmd (bottom-up 8-bit
 * rows, no header) into an image file using wxImage, so that no external
 * tool (ImageMagick) is needed.  It lives in wxviz rather than util
 * because util has no wx dependency.
 */
#ifndef IMAGECONVERTER
#define IMAGECONVERTER

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include "util/EcceException.H"
#include "util/BadValueException.H"

class ImageConverter
{
   public:
      ImageConverter();
      ~ImageConverter();

      vector<string> outputFormats();

      /** Format comes from the extension of outFile; throws EcceException. */
      void convert(const string& inFile, const string& outFile,
                   const int& width, const int& height,
                   const int& depth, const bool& rmInFile);
};

#endif
