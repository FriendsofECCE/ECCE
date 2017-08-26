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

//-*- C++ -*-

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
 |	element classes. It is provided as a convenience.
 |
 |   Author(s)		: Paul S. Strauss
 |
 |   Notes:
 |	Try not to include this file; use only those specific ones
 |	that you actually need.
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_ELEMENTS_
#define  _SO_ELEMENTS_

#include "inv/elements/SoBBoxModelMatrixElement.hpp"
#include "inv/elements/SoCacheElement.hpp"
#include "inv/elements/SoClipPlaneElement.hpp"
#include "inv/elements/SoComplexityElement.hpp"
#include "inv/elements/SoComplexityTypeElement.hpp"
#include "inv/elements/SoCoordinateElement.hpp"
#include "inv/elements/SoCreaseAngleElement.hpp"
#include "inv/elements/SoDrawStyleElement.hpp"
#include "inv/elements/SoFloatElement.hpp"
#include "inv/elements/SoFocalDistanceElement.hpp"
#include "inv/elements/SoFontNameElement.hpp"
#include "inv/elements/SoFontSizeElement.hpp"
#include "inv/elements/SoGLCacheContextElement.hpp"
#include "inv/elements/SoGLClipPlaneElement.hpp"
#include "inv/elements/SoGLCoordinateElement.hpp"
#include "inv/elements/SoGLDrawStyleElement.hpp"
#include "inv/elements/SoGLLazyElement.hpp"
#include "inv/elements/SoGLLightIdElement.hpp"
#include "inv/elements/SoGLLinePatternElement.hpp"
#include "inv/elements/SoGLLineWidthElement.hpp"
#include "inv/elements/SoGLModelMatrixElement.hpp"
#include "inv/elements/SoGLNormalElement.hpp"
#include "inv/elements/SoGLPointSizeElement.hpp"
#include "inv/elements/SoGLProjectionMatrixElement.hpp"
#include "inv/elements/SoGLRenderPassElement.hpp"
#include "inv/elements/SoGLShapeHintsElement.hpp"
#include "inv/elements/SoGLTextureCoordinateElement.hpp"
#include "inv/elements/SoGLTextureEnabledElement.hpp"
#include "inv/elements/SoGLTextureImageElement.hpp"
#include "inv/elements/SoGLTextureMatrixElement.hpp"
#include "inv/elements/SoGLUpdateAreaElement.hpp"
#include "inv/elements/SoGLViewingMatrixElement.hpp"
#include "inv/elements/SoGLViewportRegionElement.hpp"
#include "inv/elements/SoInt32Element.hpp"
#include "inv/elements/SoLightAttenuationElement.hpp"
#include "inv/elements/SoLinePatternElement.hpp"
#include "inv/elements/SoLineWidthElement.hpp"
#include "inv/elements/SoLocalBBoxMatrixElement.hpp"
#include "inv/elements/SoMaterialBindingElement.hpp"
#include "inv/elements/SoModelMatrixElement.hpp"
#include "inv/elements/SoNormalBindingElement.hpp"
#include "inv/elements/SoNormalElement.hpp"
#include "inv/elements/SoOverrideElement.hpp"
#include "inv/elements/SoPickRayElement.hpp"
#include "inv/elements/SoPickStyleElement.hpp"
#include "inv/elements/SoPointSizeElement.hpp"
#include "inv/elements/SoProfileCoordinateElement.hpp"
#include "inv/elements/SoProfileElement.hpp"
#include "inv/elements/SoProjectionMatrixElement.hpp"
#include "inv/elements/SoShapeHintsElement.hpp"
#include "inv/elements/SoShapeStyleElement.hpp"
#include "inv/elements/SoSwitchElement.hpp"
#include "inv/elements/SoTextureCoordinateBindingElement.hpp"
#include "inv/elements/SoTextureCoordinateElement.hpp"
#include "inv/elements/SoTextureImageElement.hpp"
#include "inv/elements/SoTextureMatrixElement.hpp"
#include "inv/elements/SoTextureQualityElement.hpp"
#include "inv/elements/SoUnitsElement.hpp"
#include "inv/elements/SoViewVolumeElement.hpp"
#include "inv/elements/SoViewingMatrixElement.hpp"
#include "inv/elements/SoViewportRegionElement.hpp"
#include "inv/elements/SoWindowElement.hpp"

#endif /* _SO_ELEMENTS_ */
