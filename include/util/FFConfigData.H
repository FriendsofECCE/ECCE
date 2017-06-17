/**
 * @file
 *
 */

#ifndef FFCONFIGDATA_H_
#define FFCONFIGDATA_H_

#include <string>
  using std::string;
#include <vector>
  using std::vector;

class FFConfigData
{
  public:
    static vector<string> getFFFiles();
    static vector<string> getSegDirs();

  private:
    static vector<string> readFor(string tag);
};

#endif // FFCONFIGDATA_H_

