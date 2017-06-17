/**
 * @file
 *
 *
 */
#include "util/ImageConverter.H"
#include "util/SFile.H"
#include "util/STLUtil.H"
#include "util/CommandWrapper.H"

#include <iostream>
  using std::cout;
  using std::endl;


bool   ImageConverter::p_initialized = false;

string ImageConverter::p_mogrifyCheck = "mogrify --version";
string ImageConverter::p_mogrifyCommand = "cp <inFile> <outFile>; mogrify -flip -size <width>x<height> -depth <depth> -format <outFormat> rgb:<outFile>";
string ImageConverter::p_convertCommand = "convert -flip -size <width>x<height> -depth <depth> <inFile> <outFile>";

string ImageConverter::p_command;
vector<string> ImageConverter::p_formats;


ImageConverter::ImageConverter()
{
  initialize();
}

ImageConverter::~ImageConverter()
{
}


void ImageConverter::initialize()
{
  if (p_initialized == false) {
    p_initialized = true;

    // 1. mogrify, 2. convert
    string cmd = "csh -c \"" + p_mogrifyCheck + " >& /dev/null\"";
    int status = system(cmd.c_str());
    status = status >> 8;
    if (status == 0) {
      p_command = p_mogrifyCommand;
    } else {
      // fallback to ImageMagick convert, whether it is available or not
      p_command = p_convertCommand;
    }

    p_formats.push_back("JPEG");
    p_formats.push_back("PNG");
    p_formats.push_back("GIF");
    p_formats.push_back("TIFF");
    p_formats.push_back("Postscript");
  }
}


void ImageConverter::convert(const string& inFile,
                             const string& outFile,
                             const int& width, const int& height,
                             const int& depth, const bool& rmInFile)
   throw (BadValueException)
{
  // Get the image type and check to make sure it is supported
  SFile file(outFile);
  string outFormat = file.extension();
  STLUtil::toLower(outFormat);

  if (outFormat!="gif" && outFormat!="rgb" && outFormat!="ps" &&
      outFormat!="tif" && outFormat!="tiff" && outFormat!="jpeg" &&
      outFormat!="jpg" && outFormat!="png" && outFormat!="mpeg") {
    throw BadValueException(("Unknown extension on " + outFile + 
                             ".  Please use the standard file extension for "
                             "the desired image format!").c_str(), WHERE);
  }

  char buf[16];
  sprintf(buf, "%d", width);
  string widthstr = buf;
  sprintf(buf, "%d", height);
  string heightstr = buf;
  sprintf(buf, "%d", depth);
  string depthstr = buf;

  // Perform substitutions for file names and format
  string cmd = p_command;
  STLUtil::substitute(cmd, "<inFile>", inFile);
  STLUtil::substitute(cmd, "<outFile>", outFile);
  STLUtil::substitute(cmd, "<outFormat>", outFormat);
  STLUtil::substitute(cmd, "<width>", widthstr);
  STLUtil::substitute(cmd, "<height>", heightstr);
  STLUtil::substitute(cmd, "<depth>", depthstr);

  if (rmInFile)
    cmd += "; /bin/rm -f " + inFile;

  CommandWrapper wrapper(cmd);
  wrapper.execute();
}


/**
 * Provides list of formats that can be converted TO.
 */
vector<string> ImageConverter::outputFormats()
{
  return p_formats;
}

