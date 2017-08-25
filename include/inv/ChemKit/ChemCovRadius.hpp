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

#ifndef __COVRADIUS__
#define __COVRADIUS__

static float covRadius[] = {
   0.00f,
   0.32f,1.60f,
   0.68f,0.35f,0.82f,0.77f,0.75f,0.73f,0.72f,1.12f,
   1.53f,1.43f,1.18f,1.10f,1.06f,1.02f,1.06f,1.54f,
   1.75f,1.81f,1.80f,1.64f,1.48f,1.50f,1.49f,1.47f,1.40f,1.44f,1.35f,
   1.38f,1.37f,1.40f,1.41f,1.37f,1.14f,1.37f,
   1.44f,2.09f,1.78f,1.82f,1.66f,1.61f,1.52f,1.46f,1.41f,1.38f,1.76f,
   1.70f,1.68f,1.66f,1.54f,1.50f,1.37f,1.42f,
   2.57f,2.24f,1.97f,1.98f,1.98f,1.97f,1.96f,1.94f,2.22f,1.93f,1.91f,1.91f,
   1.90f,1.88f,1.87f,2.09f,1.87f,1.75f,1.60f,1.57f,1.54f,1.51f,1.43f,1.43f,
   1.50f,1.54f,1.85f,2.00f,1.60f,1.71f,1.45f,1.74f,
   1.68f,1.68f,1.68f,1.98f,1.98f,1.76f,1.70f,1.70f,1.70f,1.70f,1.70f,1.70f,1.70f,
   1.70f,1.70f,1.70f,1.70f
};

enum stdNames{
   _GLY, _ALA, _VAL, _PHE, _PRO, _MET,
   _ILE, _LEU, _ASP, _GLU, _LYS, _ARG,
   _SER, _THR, _TYR, _HIS, _CYS, _ASN,
   _GLN, _TRP, _AAA, _TTT, _CCC, _GGG,
   _RESNUM
};

static const char* stdName3[_RESNUM] = { "GLY",
          "ALA", "VAL", "PHE", "PRO", "MET", "ILE", "LEU",
          "ASP", "GLU", "LYS", "ARG",
          "SER", "THR", "TYR", "HIS", "CYS", "ASN", "GLN", "TRP",
          "  A", "  T", "  C", "  G", };

#endif
