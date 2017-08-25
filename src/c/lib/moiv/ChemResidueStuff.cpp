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

#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "inv/ChemKit/ChemResidueStuff.H"

#define POINT_PLANE_DIST(PLANE,POINT) \
PLANE.getNormal().dot(POINT) - PLANE.getDistanceFromOrigin();

ChemResidueStuff::ChemResidueStuff()
{
	index = indices[0] = indices[1] = indices[2] = indices[3] = from = to = -1;
	isRibbonRenderable = isStickRenderable = caDistance = false;

	center.setValue(0,0,0);
	//halfsize=0.0f;
	segments=2;	
}
ChemResidueStuff::~ChemResidueStuff()
{
}

void ChemResidueStuff::renderAABB() const
{
	glPushMatrix();

	glTranslatef(center[0],center[1],center[2]);
	
	// sides
	glBegin (GL_LINE_STRIP);

	glVertex3f (-halfsize[0],halfsize[1],-halfsize[2]);
	glVertex3f (-halfsize[0],halfsize[1],halfsize[2]);
	glVertex3f (halfsize[0],halfsize[1],halfsize[2]);
	glVertex3f (halfsize[0],halfsize[1],-halfsize[2]);

	glVertex3f (-halfsize[0],halfsize[1],-halfsize[2]);

	glVertex3f (-halfsize[0],-halfsize[1],-halfsize[2]);
	glVertex3f (-halfsize[0],-halfsize[1],halfsize[2]);
	glVertex3f (halfsize[0],-halfsize[1],halfsize[2]);
	glVertex3f (halfsize[0],-halfsize[1],-halfsize[2]);

	glVertex3f (-halfsize[0],-halfsize[1],-halfsize[2]);

	glEnd();

	glBegin(GL_LINES);

	glVertex3f (halfsize[0],halfsize[1],-halfsize[2]);
	glVertex3f (halfsize[0],-halfsize[1],-halfsize[2]);

	glVertex3f (halfsize[0],halfsize[1],halfsize[2]);
	glVertex3f (halfsize[0],-halfsize[1],halfsize[2]);

	glVertex3f (-halfsize[0],halfsize[1],halfsize[2]);
	glVertex3f (-halfsize[0],-halfsize[1],halfsize[2]);

	glEnd();

	glPopMatrix();
}

bool ChemResidueStuff::overlapAABBFrustum(const SbPlane* planes) const
{
	int i,q;
	SbVec3f vmin,vmax;
	float distance;

	for (i=0;i<6;i++)
	{
		for(q=0;q<=2;q++)
		{
			if(planes[i].getNormal()[q]>=0.0f)
			{
				vmin[q]=-halfsize[q];
				vmax[q]=halfsize[q];
			}
			else
			{
				vmin[q]=halfsize[q];
				vmax[q]=-halfsize[q];
			}
		}
		distance = POINT_PLANE_DIST(planes[i],center+vmin);
		if(distance>=0.0f) //full inside
			continue;

		distance = POINT_PLANE_DIST(planes[i],center+vmax);
		if(distance<0.0f) // full outside
			return false;
	}
	
	return true;
}

// Ray box code based on the following paper:
// http://www.cs.utah.edu/~rmorley/pubs/box.pdf
bool ChemResidueStuff::intersectAABB(const SbLine &line, float &t0, float &t1) const
{

	/*
	int sign[3], const SbVec3f &inv_direction,
	inv_direction = Vector3(1/d.x(), 1/d.y(), 1/d.z());
	sign[0] = (inv_direction.x() < 0);
	sign[1] = (inv_direction.y() < 0);
	sign[2] = (inv_direction.z() < 0);

	const SbVec3f& parameters[2], float halfsize,

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (parameters[sign[0]][0] - r.origin[0]) * r.inv_direction[0];
	tmax = (parameters[1-sign[0]][0] - r.origin[0]) * r.inv_direction[0];
	tymin = (parameters[sign[1]][1] - r.origin[1]) * r.inv_direction[1];
	tymax = (parameters[1-sign[1]][1] - r.origin[1]) * r.inv_direction[1];
	if ( (tmin > tymax) || (tymin > tmax) )
	return false;
	if (tymin > tmin)
	tmin = tymin;
	if (tymax < tmax)
	tmax = tymax;
	tzmin = (parameters[r.sign[2]][2] - r.origin[2]) * r.inv_direction[2];
	tzmax = (parameters[1-r.sign[2]][2] - r.origin[2]) * r.inv_direction[2];
	if ( (tmin > tzmax) || (tzmin > tmax) )
	return false;
	if (tzmin > tmin)
	tmin = tzmin;
	if (tzmax < tmax)
	tmax = tzmax;
	return ( (tmin < t1) && (tmax > t0) );
	*/
	return true;
}

#undef POINT_PLANE_DIST
