/**
 * @file
 *
 *
 */
#ifndef CALCDROPHANDLER_H
#define CALCDROPHANDLER_H

#include <string>
  using std::string;

/**
 * Interface for windows want to support the standard Ecce drop site.
 */
class CalcDropHandler
{    
   public:
      virtual void processDrop(const string& url) = 0;

};

#endif
