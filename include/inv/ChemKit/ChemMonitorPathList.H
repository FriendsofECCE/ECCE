/*
 * ChemMonitorPathList.h
 *
 *	This file contains the definitions of a subclass of SbPList for
 *      holding pointers to ChemMonitorPath data.
 *
 * Copyright 1996, 1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * THE CONTENT OF THIS WORK CONTAINS CONFIDENTIAL AND PROPRIETARY
 * INFORMATION OF SILICON GRAPHICS, INC. ANY DUPLICATION, MODIFICATION,
 * DISTRIBUTION, OR DISCLOSURE IN ANY FORM, IN WHOLE, OR IN PART, IS STRICTLY
 * PROHIBITED WITHOUT THE PRIOR EXPRESS WRITTEN PERMISSION OF SILICON
 * GRAPHICS, INC.
 */
/**************************************************************************\
 *
 * OpenMOIV - C++ library for molecular visualization using Inventor.
 * Copyright (C) 2001-2003 Universitat Pompeu Fabra - Barcelona (Spain)
 * 
 * Developers: Interactive Technology Group (GTI)
 * Team: Josep Blat, Eduard Gonzalez, Sergi Gonzalez,
 *       Daniel Soto, Alejandro Ramirez, Oscar Civit.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details (see the file 
 * LICENSE.LGPL at the root directory).
 *
 * REMARK: This library is a derived product.
 *         You need also to accept all other applicable licenses.
 * 
 * Homepage: http://www.tecn.upf.es/openMOIV/
 * Contact:  openmoiv@upf.es
 *
\**************************************************************************/

#ident "$Revision: 22148 $"

#ifndef __CHEM_MONITORPATH_LIST_H__
#define __CHEM_MONITORPATH_LIST_H__

class ChemMonitor;
class ChemMonitorPath;
class ChemPath;

#include "inv/SbPList.H"
#include "inv/SoPath.H"

#include "inv/ChemKit/MFVec2i.H"
#include "inv/ChemKit/ChemkitBasic.H"

class CHEMKIT_DLL_API ChemMonitorPathList : public SbPList {
  public:
    ChemMonitorPathList()         : SbPList()     {}
    ChemMonitorPathList(int size) : SbPList(size) {}
    ~ChemMonitorPathList()        { truncate(0); }
    
    // Merges a chemPath with what is already in the list.
    // Returns whether the list changed or not.
    SbBool mergePath(ChemMonitorPath *chemPath, SbBool toggle);

    // Removes chemPath from the list.
    void removePath(ChemMonitorPath *chemPath);

    // Removes a chemPath specified by the index "which".
    void remove(int32_t which);

    // Removes all list entries beginning with "start".
    void truncate(int32_t start);

    // Finds the ChemMonitorPath in the ChemMonitorPathList which has the 
    // same SoPath as the argument ChemMonitorPath.  It returns the index
    // into the ChemMonitorPathList
    // or -1 if not found.
    int32_t findSoPath(const ChemPath *chemPath);
    int32_t findSoPath(const SoPath &path);
    
    void copy(const ChemMonitorPathList &l);

    ChemMonitorPathList &        operator=(const ChemMonitorPathList &l)
        { copy(l); return *this; }
    ChemMonitorPath *            operator [](int i) const
        { return ( (ChemMonitorPath *) ( (*(const SbPList *) this) [i] ) ); }

  private:
    SbBool mergePaths(ChemMonitorPath *listPath,
                      ChemMonitorPath *newPath,
                      ChemMonitor *chemMonitor, SbBool toggle);
    SbBool mergeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
                        int32_t end_indicator, int32_t numberOfItems,
                        SbBool toggle);
    void   removePath(ChemMonitorPath *listPath,
                      ChemMonitorPath *newPath,
                      ChemMonitor *chemMonitor);
    void   removeIndices(MFVec2i &listIndex, MFVec2i &newIndex,
                         int32_t end_indicator, int32_t numberOfItems);

};

#endif /* !__CHEM_MONITORPATH_LIST_H__ */
