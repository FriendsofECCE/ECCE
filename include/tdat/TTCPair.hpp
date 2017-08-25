///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TCPair.H
//
//
// CLASS SYNOPSIS:
//    Class TagCountPair Implements a Center Counting Class
//    It Manages a Single Pair = <Tag, Count>
//
// EXPORTED TYPES:
//    Class TagCountPair
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class Fragment
//    Class Formula
//      
// DESCRIPTION:
//    This class is nothing more than a simple wrapper for a structure
//    with two data members: a string tag and an unsigned integer
//    count.  This class adds comparison operators so that lists of
//    these objects can be searched and sorted.
//
// EXAMPLES:
//    testFragment.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TTCPAIR_HH
#define TTCPAIR_HH

// system includes
#include <string>
using std::string;

// library includes
// application includes

class TTagCountPair {
public:

  // Constructors
    TTagCountPair();
    virtual ~TTagCountPair();

  // Operators
    bool operator==(const TTagCountPair& tagcount) const;
    bool operator!=(const TTagCountPair& tagcount) const;
    bool operator<(const TTagCountPair& tagcount) const;

    int compareTo(const TTagCountPair& tcpair) const;

  // Public Data Members
    string tag;
    int    count;
};

#endif // TTCPair_H
