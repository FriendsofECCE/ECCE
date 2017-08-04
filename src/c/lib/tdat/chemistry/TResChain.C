///////////////////////////////////////////////////////////////////////////////
//
//   The TResChain provides a simple method of keeping track of which chain
//   labels are currently being used by a fragment and for obtaining a new,
//   unique chain label. This functionality is necessary for handling any
//   system with residue.
//
///////////////////////////////////////////////////////////////////////////////
#include "tdat/TResChain.H"

#include <vector>

using std::vector;

///////////////////////////////////////////////////////////////////////////////
//  Destructor
///////////////////////////////////////////////////////////////////////////////
TResChain::~TResChain()
{
}
///////////////////////////////////////////////////////////////////////////////
//  Constructor
///////////////////////////////////////////////////////////////////////////////
TResChain::TResChain()
{

// Create list of standard symbols and set their corresponding usage to
// false.

  p_chainLabels.clear();
  p_chainUsage.clear();

  int i;
  for (i=0; i<26; i++) {
    p_chainLabels.push_back('A'+i);
    p_chainUsage.push_back(false);
  }
  for (i=0; i<9; i++) {
    p_chainLabels.push_back('1'+i);
    p_chainUsage.push_back(false);
  }
  for (i=0; i<26; i++) {
    p_chainLabels.push_back('a'+i);
    p_chainUsage.push_back(false);
  }
}
///////////////////////////////////////////////////////////////////////////////
//  Assign a TResChain from another TResChain
///////////////////////////////////////////////////////////////////////////////
TResChain& TResChain::operator=(const TResChain& rchain)
{
  int isize = rchain.p_chainUsage.size();
  for (int i=0; i<isize; i++) {
    p_chainUsage[i] = rchain.p_chainUsage[i];
    p_chainLabels[i] = rchain.p_chainLabels[i];
  }
  return *this;
}
///////////////////////////////////////////////////////////////////////////////
//  Clear information in TResChain and reset it.
///////////////////////////////////////////////////////////////////////////////
void TResChain::clearChainID(void)
{

// Clear information in label and usage vectors and reset labels to standard
// list

  p_chainLabels.clear();
  p_chainUsage.clear();

  int i;
  for (i=0; i<26; i++) {
    p_chainLabels.push_back('A'+i);
    p_chainUsage.push_back(false);
  }
  for (i=0; i<9; i++) {
    p_chainLabels.push_back('1'+i);
    p_chainUsage.push_back(false);
  }
  for (i=0; i<26; i++) {
    p_chainLabels.push_back('a'+i);
    p_chainUsage.push_back(false);
  }
}
///////////////////////////////////////////////////////////////////////////////
//  Clear information in TResChain and reset it.
///////////////////////////////////////////////////////////////////////////////
char TResChain::getNewChainID(void)
{

  // Return a chain label that is not currently in use. If all of them are
  // in use, then recycle the labels and return 'A'.

  int isize = p_chainLabels.size();
  for (int i=0; i<isize; i++) {
    if (!p_chainUsage[i]) {
      p_chainUsage[i] = true;
      return p_chainLabels[i];
    }
  }
  clearChainID();
  addChainID('A');
  return 'A';
}
///////////////////////////////////////////////////////////////////////////////
//  Clear information in TResChain and reset it.
///////////////////////////////////////////////////////////////////////////////
void TResChain::addChainID(char chainID)
{
  if (chainID == ' ') return;
  int isize = p_chainLabels.size();
  for (int i=0; i<isize; i++) {
    if (p_chainLabels[i] == chainID) {
      p_chainUsage[i] = true;
      return;
    }
  }
  p_chainLabels.push_back(chainID);
  p_chainUsage.push_back(true);
  return;
}
