#include <vector>
using std::vector;

#if defined (__sun) || defined(__sgi)
#include <hash_set>
   using std::hash_set;
   using std::hash;
#endif
#ifdef __GNUC__
#include <ext/hash_set>
   using __gnu_cxx::hash_set;
   using __gnu_cxx::hash;
#endif


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

      hash_set<int, hash<int>, sbequint> p_set;


   private:


};

typedef SparseBits RWBitVec;

