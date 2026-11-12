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

      virtual bool execute();
      virtual bool isUndoable() const;
      virtual void undo();

      // GROMACS gmx hbond default: D...A <= dist (Angstrom) and
      // H-D...A <= angle (degrees); shared by every command instance.
      static void setCriteria(double dist, double angle);
      static double maxDistance();
      static double maxAngle();
      // Bonds drawn by the last execute(), for scripts and tests.
      static int lastCount();
      // Cl-/Br- ions as acceptors (default on); off gives the N/O/F count
      // gmx hbond reports.
      static void setHalideAcceptors(bool on);

   protected:
      virtual void init();

   private:
      static double p_maxDist;
      static double p_maxAngle;
      static int p_lastCount;
      static bool p_halides;
};

#endif
