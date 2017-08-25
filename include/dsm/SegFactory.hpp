///////////////////////////////////////////////////////////////////////////////
// Synopsis: SegFactory implements the interface for for creating Segment
//           objects from the appropriate input stream
///////////////////////////////////////////////////////////////////////////////
#ifndef SEGFACTORY_HH
#define SEGFACTORY_HH

#include <vector>
  using std::vector;

#include "dsm/FileEDSI.H"
#include "tdat/Segment.H"

class Residue;
class Fragment;

class SegFactory 
{
  public:

// Default constructor
    SegFactory(void);
// Destructor
    virtual ~SegFactory(void);

// Return a pointer to a segment corresponding to a particular residue name
// (and coupling)

     Segment * getSegment(Residue *residue, const Residue::Coupling coupling,
	                  Fragment *frag, string fftype = "Amber");

// Read and write segment (and fragment) files

     Segment * readSegment(FileEDSI & url, const string & segname);
     Segment * readFragment(FileEDSI & url, const string & segname);
     bool writeSegment(const Residue * residue, const FileEDSI & url,
                       const Fragment * frag, bool use46 = true);
     bool writeFragment(const Residue * residue, const FileEDSI & url,
                       const Fragment * frag, bool use46 = true);

// Add and delete items to EDSIList

     void addEDSI(const char* url);
     void deleteEDSI(const FileEDSI url);
     void clearEDSI();

  private:
     vector<Segment*> p_segmentList;
     vector<string> p_unknownSegmentNames;
     vector<FileEDSI> p_EDSIList;
};
#endif
