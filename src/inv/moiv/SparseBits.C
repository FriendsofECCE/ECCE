#include <iostream>
using namespace std;

#include "inv/ChemKit/SparseBits.H"


/**
 * Constructor.
 * The internal cache is constructed with bucket size 100.
 * This choice is totally arbitrary.
 */
SparseBits::SparseBits() 
{
   //p_set.resize(100);
}



/**
 * Constructor - This constructor for backwards compatability only.  
 * Bucket size not set, default value not meaninful.
 * @param bucketsize - not used
 * @param set - not used
 */
SparseBits::SparseBits(int bucketsize, bool /*set*/) 
{
   //p_set.resize(bucketsize);
}



/**
 * Copy Constructor.
 * @param rhs - object to be copied.
 */
SparseBits::SparseBits(const SparseBits& rhs) 
{
   // Use the same bucket size
   p_set.resize(rhs.p_set.bucket_count());

   // Anything in rhs must be true so set everything in this that is set
   // in rhs.
   hash_set<int, hash<int>, sbequint>::const_iterator it = rhs.p_set.begin();
   while (it != rhs.p_set.end()) {
      setBit(*it);
      it++;
   }
}



/**
 * Destructor.
 */
SparseBits::~SparseBits()
{
   //cout << "Destroy " << p_set.size() << " " << p_set.bucket_count() << endl;
   p_set.clear();
}



/**
 * Reset size of hash.
 * This exists for compatibility with a typical dense vector.
 * @param size - new bucket size.
 */
void SparseBits::resize(int size)
{
   // Do nothing now.  Maybe later determine if bucket size needs to
   // be improved.
   //p_set.resize(size);
}

/**
 *  Set or clear bit at index.
 *  This method simplifies code where you may need an if statement to
 *  decide whether to call setBit or clearBit.  
 *  operator[] cannot do the job since it cannot return a reference.
 */
void SparseBits::setBitTo(int index, bool value)
{
  // For speed, repeat code in setBit/testBit rather than call the methods
  if (value == true) {
    p_set.insert(index);
  } else {
    p_set.erase(p_set.find(index));
  }
}


/**
 * Get number of set bits.
 */
int SparseBits::size() const
{
   return p_set.size();
}



/**
 * Set the bit to true.
 * @param index - the bit to set.
 */
void SparseBits::setBit(int index)
{
   p_set.insert(index);
}



/**
 * Clear bit by removing corresponding index from set.
 * @param index - bit to be cleared.
 */
void SparseBits::clearBit(int index)
{
   p_set.erase(p_set.find(index));
}



/**
 * Return true if index is in set, otherwise false
 * @param index - bit to test
 */
bool SparseBits::testBit(int index) const
{
   return (p_set.find(index) != p_set.end());
}



/**
 * Determine index of first bit that is set to true.
 * Not really sure what the logic is here....
 * @return the bit index or -1 if none are set.
 */
int SparseBits::firstTrue() const
{
   int ret = -1;
   hash_set<int, hash<int>, sbequint>::const_iterator it = p_set.begin();
   while (it != p_set.end()) {
      if (*it < ret) {
         ret = *it;
      }
      it++;
   }
   return ret;
}



/**
 * Determ the index of the first bit that is not set.
 * @return bit index or -1 if none are false
 */
int SparseBits::firstFalse() const
{
   int ret = -1;

   // Yes this is an infinite loop - break when we find a missing value
   for (int i=0; ; i++) {
      if (p_set.find(i) != p_set.end()) {
         ret = i;
         break;
      }
   }

   return ret;
}



/**
 * Compute a hash value for this vector.
 * For now, using address.
 */
unsigned int SparseBits::computeHashValue() const
{
#ifdef __x86_64
   // GDB 2/2/12  On 64-bit systems mask off upper 4 bytes
   // to get a 4-byte hash index
   unsigned long addr = (unsigned long)this;
   unsigned int ret = (unsigned int) (addr & 0x00000000FFFFFFFF);
#else
   unsigned int ret = (unsigned int) this;
#endif
   return ret;
}



/**
 * Return the number of set bits.
 */
int SparseBits::sum() const
{
   return p_set.size();
}



/**
 * Load up the indices of all set bits into the setbits param.
 * For sparse bit vectors, it can be prohibitively expensive to
 * iterate over bits and call testBit so this provides an alternative
 * perhaps much faster method.
 */
void SparseBits::getSetBits(vector<int>& setbits) const
{
   setbits.clear();
   hash_set<int, hash<int>, sbequint>::const_iterator it = p_set.begin();
   while (it != p_set.end()) {
      setbits.push_back(*it);
      it++;
   }
}



/**
 * Determine if this SparseBits class is identical to rhs.
 */
bool SparseBits::isEqual(const SparseBits& rhs)
{
   //cout << "start isEqual " << endl;
   bool ret = false;
   if (p_set.size() == rhs.p_set.size()) {
      // Same size but same values?
#ifdef builtinequal
      //ret = (p_set == rhs.p_set);
#else
      hash_set<int, hash<int>, sbequint>::const_iterator it = p_set.begin();
      while (it != p_set.end()) {
         if ( ! rhs.testBit(*it) ) {
            break;
         }
         it++;
      }
      // If we made it through all slots without a mismatch, they are equal
      if (it == p_set.end()) {
         ret = true;
      }
#endif
   }
   //cout << "end isEqual " << ret << endl;
   return ret;
}


/**
 */
SparseBits SparseBits::operator|(const SparseBits& rhs)
{
   SparseBits ret(*this);
   hash_set<int, hash<int>, sbequint>::const_iterator it = rhs.p_set.begin();
   while (it != rhs.p_set.end()) {
      ret.setBit(*it);
      it++;
   }

   return ret;
}

/**
 * Or the two sparse bits.
 * This is a merge of the two hashes.
 */
SparseBits& SparseBits::operator|=(const SparseBits& rhs)
{
   if (this != &rhs) {
      hash_set<int, hash<int>, sbequint>::const_iterator it = rhs.p_set.begin();
      while (it != p_set.end()) {
         setBit(*it);
         it++;
      }
   }

   return *this;
}


/**
 */
SparseBits& SparseBits::operator^=(const SparseBits& rhs)
{
   hash_set<int, hash<int>, sbequint>::const_iterator it = rhs.p_set.begin();
   while (it != rhs.p_set.end()) {
      bool set = (*it == (true ^ testBit(*it)));
      if (set) {
         setBit(*it);
      } else {
         clearBit(*it);
      }
      it++;
   }

   return *this;
}



/**
 * Return value at specified index.
 * Unlike most operator[] implementations, a reference cannot be returned
 * since the data may not exist so this method cannot be used to modify
 * data.
 */
bool SparseBits::operator[](int index) const
{
   return this->testBit(index);
}

