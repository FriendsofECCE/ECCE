//  Memory limits are entered/labeled in GB in the GUI (Andy, 2026-09-07),
//  but the .Q file's memLimit is documented and written in MB (README.Q,
//  siteconfig/example-slurm.Q) -- convert at the widget, never change what
//  gets stored. 1 GB = 1000 MB, the same decimal factor ai.orca uses for
//  %maxcore (see GUIValues.C).
#ifndef MEMORYUNITS_H
#define MEMORYUNITS_H

namespace MemoryUnits
{
    const unsigned int MB_PER_GB = 1000;

    //  A spin control is integer GB. Round to the nearest GB, but never
    //  round a real nonzero limit down to 0 ("no limit").
    inline unsigned int mbToGB(unsigned int mb)
    {
        if (mb == 0)
            return 0;

        unsigned int gb = (mb + MB_PER_GB / 2) / MB_PER_GB;
        return (gb == 0) ? 1 : gb;
    }

    inline unsigned int gbToMB(unsigned int gb)
    {
        return gb * MB_PER_GB;
    }
}

#endif
