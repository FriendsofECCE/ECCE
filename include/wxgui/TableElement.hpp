/**
 *  @file
 *  @author Ken Swanson
 *
 *  Base class for elements to be displayed in the WxTableView.
 *
 *  This class cannot be instantiate directly, but must be extended to a concrete
 *  class.  Extending classes are required to implement details for the compareTo()
 *  and getValueAsString() methods.
 *
 *  It is expected that the compareTo() method return -1 if the value within the
 *  field of this element is less than that of the comparison element, +1 if it
 *  is greater than that of the comparison element, and 0 if the values are equal.
 *
 *  The getValueAsString() method is expected to return a string representation
 *  of the value within the indicated field.
 *
 */


#ifndef TABLEELEMENT_HH
#define TABLEELEMENT_HH

#include <string>
    using std::string;

class TableElement
{

     public:
         virtual int compareTo(TableElement *elmnt, int field) = 0;
         virtual string getValueAsString(int field) = 0;
         virtual int getImageIndex() = 0;

};

#endif  //  TABLEELEMENT_HH
