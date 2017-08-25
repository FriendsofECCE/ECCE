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
 |   Classes:
 |	SoMouseButtonEvent
 |
 |   Author(s): David Mott, Gavin Bell
 |
 ______________  S I L I C O N   G R A P H I C S   I N C .  ____________
 _______________________________________________________________________
 */

#ifndef  _SO_MOUSE_BUTTON_EVENT_
#define  _SO_MOUSE_BUTTON_EVENT_

#include "inv/SbBasic.H"
#include "inv/events/SoButtonEvent.H"

// some convenience macros for determining if an event matches
// C-api: end

#define SO_MOUSE_PRESS_EVENT(EVENT,BUTTON) \
    (SoMouseButtonEvent::isButtonPressEvent(EVENT,SoMouseButtonEvent::BUTTON))

#define SO_MOUSE_RELEASE_EVENT(EVENT,BUTTON) \
    (SoMouseButtonEvent::isButtonReleaseEvent(EVENT,SoMouseButtonEvent::BUTTON))

// C-api: begin

// C-api: interpret #define SO_MOUSE_PRESS_EV(EVENT,BUTTON)
// C-api: interpret     (SoMouseEvIsBtnPressEv(EVENT,BUTTON))
// C-api: interpret #define SO_MOUSE_RELEASE_EV(EVENT,BUTTON)
// C-api: interpret     (SoMouseEvIsBtnReleaseEv(EVENT,BUTTON))


// C-api: prefix=SoMouseEv
class SoMouseButtonEvent : public SoButtonEvent {

    SO_EVENT_HEADER();
    
  public:
    enum Button {
	ANY = 0, 
	BUTTON1 = 1, 
	BUTTON2 = 2, 
	BUTTON3 = 3
    };
    
    // constructor
    SoMouseButtonEvent();
    virtual ~SoMouseButtonEvent();
    
    // set/get which button generated the event, either SO_MOUSE_BUTTON1, 
    // SO_MOUSE_BUTTON2, or SO_MOUSE_BUTTON3
    // C-api: name=setBtn
    void		setButton(SoMouseButtonEvent::Button b)  { button = b; }
    // C-api: name=getBtn
    SoMouseButtonEvent::Button	getButton() const	    { return button; }
    
    // convenience routines to see if an SoEvent is a press or release
    // of the passed mouse button
    // C-api: name=isBtnPressEv
    static SbBool	isButtonPressEvent(
			    const SoEvent *e,
			    SoMouseButtonEvent::Button whichButton);
			    
    // C-api: name=isBtnReleaseEv
    static SbBool	isButtonReleaseEvent(
			    const SoEvent *e,
			    SoMouseButtonEvent::Button whichButton);
    
  SoINTERNAL public:
    static void		initClass();
    
  private:
    Button   	    button;	// which button
};


#endif /* _SO_MOUSE_BUTTON_EVENT_ */
