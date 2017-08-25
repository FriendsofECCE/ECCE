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

#ifndef __CHEMPDBIMPORTER__
#define __CHEMPDBIMPORTER__


#include "inv/ChemKit/ChemFileImporter.H"

class ChemData;
class ChemAssociatedData;

class CHEMKIT_DLL_API ChemPDBImporter: public ChemFileImporter
{
public:
	ChemPDBImporter();
	~ChemPDBImporter();

	virtual bool openFile(const char* filename, ChemData* chemdata, ChemAssociatedData* chemassociateddata) const;

private:
	void parseATOMLine (char* s, short &atnum, float &x, float &y, float &z, float &therm, char *Label) const;
};

#endif
