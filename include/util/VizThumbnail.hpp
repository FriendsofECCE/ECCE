/**
 * @file
 *
 */

#ifndef VIZTHUMBNAIL_H_
#define VIZTHUMBNAIL_H_

#include <string>
using std::string;

#include <vector>
using std::vector;

class VizThumbnail
{
  public:
    static void thumbnail(const string& whoami, const string& calcurl,
                          int width = 64, int height = 64,
                          double r = 0.0, double g = 0.0, double b = 0.0);
    static void thumbnail(const string& whoami, const vector<string>& calcurls,
                          int width = 64, int height = 64,
                          double r = 0.0, double g = 0.0, double b = 0.0);
};

#endif // VIZTHUMBNAIL_H_
