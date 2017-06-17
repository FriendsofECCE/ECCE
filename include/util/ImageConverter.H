/**
 * @file 
 *
 *
 */
#ifndef IMAGECONVERTER
#define IMAGECONVERTER

// system includes
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

      void convert(const string& inFile, const string& outFile,
                   const int& width, const int& height,
                   const int& depth, const bool& rmInFile) 
         throw (BadValueException);


   protected:
      void initialize();

      static bool p_initialized;

      static string p_mogrifyCheck; 
      static string p_mogrifyCommand; 
      static string p_convertCommand; 
      static string p_command;

      static vector<string> p_formats;
};

#endif
