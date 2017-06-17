/**
 *  @file
 *  @author Ken Swanson
 *
 *  Abstract class that defines the interface of a filter on the WxTableView.
 *
 *  Applications in which a filter is needed on the table view will need
 *  to define a concrete subclass of this class, and specify the required
 *  behavior of the filter by implementing the accept() method.
 *
 */


#ifndef TABLEVIEWFILTER_HH
#define TABLEVIEWFILTER_HH

#include "wxgui/TableElement.H"

class TableViewFilter
{

     public:
         virtual bool accept(TableElement *elmnt) = 0;

};

#endif  //  TABLEVIEWFILTER_HH
