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

#include "inv/nodes/SoAnnotation.hpp"
#include "inv/nodes/SoArray.hpp"
#include "inv/nodes/SoAsciiText.hpp"
#include "inv/nodes/SoBaseColor.hpp"
#include "inv/nodes/SoBlinker.hpp"
#include "inv/nodes/SoCallback.hpp"
#include "inv/nodes/SoCamera.hpp"
#include "inv/nodes/SoClipPlane.hpp"
#include "inv/nodes/SoColorIndex.hpp"
#include "inv/nodes/SoComplexity.hpp"
#include "inv/nodes/SoCone.hpp"
#include "inv/nodes/SoCoordinate3.hpp"
#include "inv/nodes/SoCoordinate4.hpp"
#include "inv/nodes/SoCube.hpp"
#include "inv/nodes/SoCylinder.hpp"
#include "inv/nodes/SoDirectionalLight.hpp"
#include "inv/nodes/SoDrawStyle.hpp"
#include "inv/nodes/SoEnvironment.hpp"
#include "inv/nodes/SoEventCallback.hpp"
#include "inv/nodes/SoFaceSet.hpp"
#include "inv/nodes/SoFile.hpp"
#include "inv/nodes/SoFont.hpp"
#include "inv/nodes/SoFontStyle.hpp"
#include "inv/nodes/SoGroup.hpp"
#include "inv/nodes/SoIndexedFaceSet.hpp"
#include "inv/nodes/SoIndexedLineSet.hpp"
#include "inv/nodes/SoIndexedNurbsCurve.hpp"
#include "inv/nodes/SoIndexedNurbsSurface.hpp"
#include "inv/nodes/SoIndexedTriangleStripSet.hpp"
#include "inv/nodes/SoIndexedShape.hpp"
#include "inv/nodes/SoInfo.hpp"
#include "inv/nodes/SoLabel.hpp"
#include "inv/nodes/SoLevelOfDetail.hpp"
#include "inv/nodes/SoLOD.hpp"
#include "inv/nodes/SoLocateHighlight.hpp"
#include "inv/nodes/SoLight.hpp"
#include "inv/nodes/SoLightModel.hpp"
#include "inv/nodes/SoLineSet.hpp"
#include "inv/nodes/SoLinearProfile.hpp"
#include "inv/nodes/SoMaterial.hpp"
#include "inv/nodes/SoMaterialBinding.hpp"
#include "inv/nodes/SoMatrixTransform.hpp"
#include "inv/nodes/SoMultipleCopy.hpp"
#include "inv/nodes/SoNonIndexedShape.hpp"
#include "inv/nodes/SoNormal.hpp"
#include "inv/nodes/SoNormalBinding.hpp"
#include "inv/nodes/SoNurbsCurve.hpp"
#include "inv/nodes/SoNurbsProfile.hpp"
#include "inv/nodes/SoNurbsSurface.hpp"
#include "inv/nodes/SoOrthographicCamera.hpp"
#include "inv/nodes/SoPackedColor.hpp"
#include "inv/nodes/SoPathSwitch.hpp"
#include "inv/nodes/SoPendulum.hpp"
#include "inv/nodes/SoPerspectiveCamera.hpp"
#include "inv/nodes/SoPickStyle.hpp"
#include "inv/nodes/SoPointLight.hpp"
#include "inv/nodes/SoPointSet.hpp"
#include "inv/nodes/SoProfile.hpp"
#include "inv/nodes/SoProfileCoordinate2.hpp"
#include "inv/nodes/SoProfileCoordinate3.hpp"
#include "inv/nodes/SoQuadMesh.hpp"
#include "inv/nodes/SoResetTransform.hpp"
#include "inv/nodes/SoRotation.hpp"
#include "inv/nodes/SoRotationXYZ.hpp"
#include "inv/nodes/SoRotor.hpp"
#include "inv/nodes/SoScale.hpp"
#include "inv/nodes/SoSeparator.hpp"
#include "inv/nodes/SoShape.hpp"
#include "inv/nodes/SoShapeHints.hpp"
#include "inv/nodes/SoShuttle.hpp"
#include "inv/nodes/SoSphere.hpp"
#include "inv/nodes/SoSpotLight.hpp"
#include "inv/nodes/SoSwitch.hpp"
#include "inv/nodes/SoText2.hpp"
#include "inv/nodes/SoText3.hpp"
#include "inv/nodes/SoTexture2.hpp"
#include "inv/nodes/SoTexture2Transform.hpp"
#include "inv/nodes/SoTextureCoordinate2.hpp"
#include "inv/nodes/SoTextureCoordinateBinding.hpp"
#include "inv/nodes/SoTextureCoordinateFunction.hpp"
#include "inv/nodes/SoTextureCoordinateDefault.hpp"
#include "inv/nodes/SoTextureCoordinateEnvironment.hpp"
#include "inv/nodes/SoTextureCoordinatePlane.hpp"
#include "inv/nodes/SoTransform.hpp"
#include "inv/nodes/SoTransformSeparator.hpp"
#include "inv/nodes/SoTransformation.hpp"
#include "inv/nodes/SoTranslation.hpp"
#include "inv/nodes/SoTriangleStripSet.hpp"
#include "inv/nodes/SoUnits.hpp"
#include "inv/nodes/SoVertexShape.hpp"
#include "inv/nodes/SoVertexProperty.hpp"
#include "inv/nodes/SoWWWAnchor.hpp"
#include "inv/nodes/SoWWWInline.hpp"

#endif /* _SO_NODES_ */
