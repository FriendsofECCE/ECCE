/*
 *
 *  Copyright (C) 2000 Silicon Graphics, Inc.  All Rights Reserved. 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  Further, this software is distributed without any warranty that it is
 *  free of the rightful claim of any third person regarding infringement
 *  or the like.  Any license provided herein, whether implied or
 *  otherwise, applies only to this software file.  Patent licenses, if
 *  any, provided herein do not apply to combinations of this program with
 *  other software, or any other product whatsoever.
 * 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact information: Silicon Graphics, Inc., 1600 Amphitheatre Pkwy,
 *  Mountain View, CA  94043, or:
 * 
 *  http://www.sgi.com 
 * 
 *  For further information regarding this notice, see: 
 * 
 *  http://oss.sgi.com/projects/GenInfo/NoticeExplan/
 *
 */

//  -*- C++ -*-

/*
 * Copyright (C) 1990,91   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22148 $
 |
 |   Description:
 |	This file #includes files that define ALL standard Inventor
 |	node classes. It is provided as a convenience.
 |
 |   Author(s)		: Paul S. Strauss, Thad Beier
 |
 |   Notes:
 |	Try not to include this file; use only those specific ones
 |	that you actually need.
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_NODES_
#define  _SO_NODES_

#include "inv/nodes/SoAnnotation.H"
#include "inv/nodes/SoArray.H"
#include "inv/nodes/SoAsciiText.H"
#include "inv/nodes/SoBaseColor.H"
#include "inv/nodes/SoBlinker.H"
#include "inv/nodes/SoCallback.H"
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoClipPlane.H"
#include "inv/nodes/SoColorIndex.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoCoordinate4.H"
#include "inv/nodes/SoCube.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoDirectionalLight.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoEnvironment.H"
#include "inv/nodes/SoEventCallback.H"
#include "inv/nodes/SoFaceSet.H"
#include "inv/nodes/SoFile.H"
#include "inv/nodes/SoFont.H"
#include "inv/nodes/SoFontStyle.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoIndexedFaceSet.H"
#include "inv/nodes/SoIndexedLineSet.H"
#include "inv/nodes/SoIndexedNurbsCurve.H"
#include "inv/nodes/SoIndexedNurbsSurface.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"
#include "inv/nodes/SoIndexedShape.H"
#include "inv/nodes/SoInfo.H"
#include "inv/nodes/SoLabel.H"
#include "inv/nodes/SoLevelOfDetail.H"
#include "inv/nodes/SoLOD.H"
#include "inv/nodes/SoLocateHighlight.H"
#include "inv/nodes/SoLight.H"
#include "inv/nodes/SoLightModel.H"
#include "inv/nodes/SoLineSet.H"
#include "inv/nodes/SoLinearProfile.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/nodes/SoMatrixTransform.H"
#include "inv/nodes/SoMultipleCopy.H"
#include "inv/nodes/SoNonIndexedShape.H"
#include "inv/nodes/SoNormal.H"
#include "inv/nodes/SoNormalBinding.H"
#include "inv/nodes/SoNurbsCurve.H"
#include "inv/nodes/SoNurbsProfile.H"
#include "inv/nodes/SoNurbsSurface.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoPackedColor.H"
#include "inv/nodes/SoPathSwitch.H"
#include "inv/nodes/SoPendulum.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoPickStyle.H"
#include "inv/nodes/SoPointLight.H"
#include "inv/nodes/SoPointSet.H"
#include "inv/nodes/SoProfile.H"
#include "inv/nodes/SoProfileCoordinate2.H"
#include "inv/nodes/SoProfileCoordinate3.H"
#include "inv/nodes/SoQuadMesh.H"
#include "inv/nodes/SoResetTransform.H"
#include "inv/nodes/SoRotation.H"
#include "inv/nodes/SoRotationXYZ.H"
#include "inv/nodes/SoRotor.H"
#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoShape.H"
#include "inv/nodes/SoShapeHints.H"
#include "inv/nodes/SoShuttle.H"
#include "inv/nodes/SoSphere.H"
#include "inv/nodes/SoSpotLight.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoText2.H"
#include "inv/nodes/SoText3.H"
#include "inv/nodes/SoTexture2.H"
#include "inv/nodes/SoTexture2Transform.H"
#include "inv/nodes/SoTextureCoordinate2.H"
#include "inv/nodes/SoTextureCoordinateBinding.H"
#include "inv/nodes/SoTextureCoordinateFunction.H"
#include "inv/nodes/SoTextureCoordinateDefault.H"
#include "inv/nodes/SoTextureCoordinateEnvironment.H"
#include "inv/nodes/SoTextureCoordinatePlane.H"
#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoTransformSeparator.H"
#include "inv/nodes/SoTransformation.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTriangleStripSet.H"
#include "inv/nodes/SoUnits.H"
#include "inv/nodes/SoVertexShape.H"
#include "inv/nodes/SoVertexProperty.H"
#include "inv/nodes/SoWWWAnchor.H"
#include "inv/nodes/SoWWWInline.H"

#endif /* _SO_NODES_ */
