#include <vector>
using std::vector;

#include <unordered_set>
   using std::unordered_set;
// Deliberately NOT "using std::hash" here (unlike unordered_set/vector
// above) - this header gets included by files that also pull in the old
// SGI <ext/hash_map> extension and declare their own unrelated
// "using __gnu_cxx::hash", which conflicts with std::hash at global scope
// if both are in play. Fully-qualify std::hash below instead.


   struct sbequint {
      bool operator()(const int s1, const int s2) const {
         return s1 == s2;
      }
   };


/**
 * Implementation of a bit vector concept as a hash.  Set values will
 * be in the hash.  Unset values are not so by default all values are
 * false.
 *
 * Excells where RWBitVec is too memory intensive.
 */
class SparseBits 
{

   public:
      SparseBits();
      /**
       * Constructor - for backward compatability
       */
      SparseBits(int bucketSize, bool set=false);
      SparseBits(const SparseBits& rhs);

      ~SparseBits();

      void setBitTo(int index, bool value);
      void setBit(int index);
      void clearBit(int index);
      bool testBit(int index) const;
      int  firstTrue() const;
      int  firstFalse() const;

      void getSetBits(vector<int>& setbits) const;

      void resize(int size);

      /**
       * Computes the number of bits that are set to true.
       */
      int  sum() const;

      unsigned int computeHashValue() const;

      int size() const;

      /**
       * Logical or.
       * Vectors are assumed to be of the same size.
       */
      SparseBits operator|(const SparseBits& rhs);
      SparseBits& operator^=(const SparseBits& rhs);
      SparseBits& operator|=(const SparseBits& rhs);

      bool operator[](int index) const;


      /**
       * Basically operator== without incurring copies.
       */
      bool isEqual(const SparseBits& rhs);

   protected:

      unordered_set<int, std::hash<int>, sbequint> p_set;


   private:


};

typedef SparseBits RWBitVec;

