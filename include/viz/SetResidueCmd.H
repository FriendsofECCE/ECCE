/**
 * @file
 * Edits one property of a residue (name, PDB number or chain), as the
 * Residue Table's cells do.
 */
#ifndef SETRESIDUECMD_HH
#define SETRESIDUECMD_HH

#include "util/Command.H"

class SetResidueCmd : public Command
{
   public:
      SetResidueCmd(const string& name, Receiver * receiver, const string& sender="");
      ~SetResidueCmd();

      virtual bool execute();
      virtual void undo();

   protected:
      void init();
      bool apply(int resnum, const string& prop, const string& value, string *old);

      string p_sender;
      int p_resnum;
      string p_prop;
      string p_old;
};
#endif
