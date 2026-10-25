#ifndef MDTRAJECTORY__H
#define MDTRAJECTORY__H

#include <cstdio>
#include <string>
#include <vector>

/**
 * One frame of a GROMACS trajectory: positions in nm, velocities in nm/ps,
 * time in ps, box vectors (rows) in nm.  No wx.
 */
struct MdFrame
{
   int natoms = 0;
   long step = 0;
   double time = 0;
   float box[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
   std::vector<float> x;  ///< 3*natoms, empty if the frame has none
   std::vector<float> v;  ///< 3*natoms, empty if the frame has none
};

/**
 * Sequential reader for the .trr (positions and velocities, single or double
 * precision) and .xtc (compressed positions) files of mdrun.  The format is
 * told from the file's magic number, not its name.
 */
class MdTrajectoryReader
{
   public:
      MdTrajectoryReader();
      ~MdTrajectoryReader();

      bool open(const std::string& path, std::string& error);
      /** The next frame; false at the end of the file or on a damaged frame
          (error is empty at a clean end). */
      bool next(MdFrame& frame, std::string& error);
      bool isXtc() const { return p_xtc; }

   private:
      bool nextTrr(MdFrame& frame, std::string& error);
      bool nextXtc(MdFrame& frame, std::string& error);
      FILE *p_file;
      bool p_xtc;
};

#endif
