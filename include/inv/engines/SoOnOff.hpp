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
 * Copyright (C) 1990,91,92   Silicon Graphics, Inc.
 *
 _______________________________________________________________________
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 |
 |   $Revision: 22148 $
 |
 |   Description:
 |	This file contains the declaration of the OnOff engine
 |
 |   Classes:
 |	SoOnOff
 |
 |   Author(s)		: Ronen Barzel
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_ON_OFF_
#define  _SO_ON_OFF_

#include "inv/engines/SoSubEngine.H"
#include "inv/fields/SoSFTrigger.H"

// C-api: public=on,off,toggle,isOn,isOff
class SoOnOff : public SoEngine {

    SO_ENGINE_HEADER(SoOnOff);
    
  public:

    // Inputs
    SoSFTrigger	on;	// turns on
    SoSFTrigger	off;	// turns off
    SoSFTrigger	toggle;	// changes state
    
    SoEngineOutput isOn;	// (SoSFBool)
    SoEngineOutput isOff;	// (SoSFBool)
    
    // Constructor
    SoOnOff();
    
  SoINTERNAL public:

    static void initClass();

  protected:
    virtual void	inputChanged(SoField *whichInput);

  private:

    // Destructor
    ~SoOnOff();

    // Evaluation method
    virtual void evaluate();

    SbBool	state;
};

#endif  /* _SO_ON_OFF_ */
