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

#ifndef CHEMKIT_BASIC_H
#define CHEMKIT_BASIC_H

/* Precaution to avoid an error easily made by the application programmer. */
#ifdef CHEMKIT_DLL_API
#error Leave the internal CHEMKIT_DLL_API define alone.
#endif /* CHEMKIT_API */

/*
  On MSWindows platforms, one of these defines must always be set when
  building application programs:

   - "CHEMKIT_DLL", when the application programmer is using the
     library in the form of a dynamic link library (DLL)

   - "CHEMKIT_NOT_DLL", when the application programmer is using the
     library in the form of a static object library (LIB)
 */

#ifdef WIN32
	#ifdef _CHEMKIT_INTERNAL
		#ifdef _MAKE_CHEMKIT_DLL
			#define CHEMKIT_DLL_API __declspec(dllexport)
		#else
			#define CHEMKIT_DLL_API
		#endif
	#else
		#ifdef CHEMKIT_DLL
			#define CHEMKIT_DLL_API __declspec(dllimport)
		#else
			#ifndef CHEMKIT_NOT_DLL
				#error Define either CHEMKIT_DLL or CHEMKIT_NOT_DLL as appropriate for your linkage! See Chemkit2/ChemkitBasic.h for further instructions.
			#endif /* CHEMKIT_NOT_DLL */
		#endif
	#endif // _CHEMKIT_INTERNAL
#endif // WIN32

/* Empty define to avoid errors when _not_ compiling an MSWindows DLL. */
#ifndef CHEMKIT_DLL_API
	#define CHEMKIT_DLL_API
#endif /* !CHEMKIT_DLL_API */

#endif // ! CHEMKIT_BASIC_H
