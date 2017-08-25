#ifndef SUMMMARYITERATOR_HH
#define SUMMMARYITERATOR_HH
/////////////////////////////////////////////////////////////////////////////
// Class: SummaryIterator
//
// Synopsis: Iterates over JCode summary field descriptions.
//
// Description:
/////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
  using std::vector;
  using std::string;

class JCode;

class SummaryIterator 
{

  public:
    virtual ~SummaryIterator();

    bool next(string& topLabel, bool& toggleSet, bool& noDefault,
              vector<string>& keys, vector<string>& labels);

  protected:

    SummaryIterator();
    SummaryIterator(const DOMNodeList& els);
    SummaryIterator(const SummaryIterator& rhs);

  private:
    DOMNodeList *p_elements;
    int p_nextItem;

    friend SummaryIterator *JCode::getSummaryIterator(const string& kind) const;

};

#endif 
