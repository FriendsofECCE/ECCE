/**
 * @file
 *
 *
 */
#ifndef  HYDROGENBONDSCMD_H
#define  HYDROGENBONDSCMD_H

#include <map>
  using std::map;
  using std::less;

#include "util/Command.H"


class HydrogenBondsCmd : public Command
{
   public:
      HydrogenBondsCmd(const string& name, Receiver * receiver);
      virtual ~HydrogenBondsCmd();

      virtual bool execute() throw(EcceException);
      virtual bool isUndoable() const;
      virtual void undo();

   protected:
      virtual void init();

      bool isDonorAtom(const string& symbol) const;
      float getDonorDistance(const string& symbol) const;
      float determineCellSize() const;

      map<string, float, less<string> > p_donors;


};

#endif
