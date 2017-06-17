///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Store a list of chain IDs that are currently in use or that may
//    used in the future.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TRESCHAIN_HH
#define TRESCHAIN_HH

#include <vector>

  using std::vector;


class TResChain
{

public:

// Constructor

  TResChain();

// Destructor

  ~TResChain();

// Operators

   TResChain& operator=(const TResChain& resChain);

// Methods

   char getNewChainID(void);
   void addChainID(char chainID);
   void clearChainID(void);

private:

   vector<char> p_chainLabels;
   vector<bool> p_chainUsage;
};
#endif
