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

/*
 * Copyright (C) 1990, 1991   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22115 $
 |
 |   Description:
 |      This file #includes all of the node kit upgraders, to save
 |      time compiling and reduce the size of the library.
 |
 |   Author(s)          : Gavin Bell
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#include "SoV1AppearKit.inc"
#include "SoV1BaseKit.inc"
#include "SoV1CameraKit.inc"
#include "SoV1ConeKit.inc"
#include "SoV1CubeKit.inc"
#include "SoV1CylinderKit.inc"
#include "SoV1DirLtKit.inc"
#include "SoV1FaceSetKit.inc"
#include "SoV1GroupKit.inc"
#include "SoV1IFaceSetKit.inc"
#include "SoV1ILineSetKit.inc"
#include "SoV1INrbCurvKit.inc"
#include "SoV1INrbSurfKit.inc"
#include "SoV1ITMshSetKit.inc"
#include "SoV1LightKit.inc"
#include "SoV1LineSetKit.inc"
#include "SoV1NkCatalog.inc"
#include "SoV1NkParts.inc"
#include "SoV1NodeKit.inc"
#include "SoV1NurbCurvKit.inc"
#include "SoV1NurbSurfKit.inc"
#include "SoV1OrthoCamKit.inc"
#include "SoV1PerspCamKit.inc"
#include "SoV1PointLtKit.inc"
#include "SoV1PointSetKit.inc"
#include "SoV1QuadMeshKit.inc"
#include "SoV1SceneKit.inc"
#include "SoV1ShapeKit.inc"
#include "SoV1SphereKit.inc"
#include "SoV1SpotLtKit.inc"
#include "SoV1Text2Kit.inc"
#include "SoV1Text3Kit.inc"
#include "SoV1TriStripKit.inc"
#include "SoV1VtxShapeKit.inc"
#include "SoV1WrapperKit.inc"
