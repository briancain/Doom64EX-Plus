// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------


#ifndef __D_EVENT__
#define __D_EVENT__


#include "doomtype.h"


//
// Event handling.
//

// Input event types.
typedef enum {
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_mousedown,
    ev_mouseup,
    ev_gamepad,
    ev_gamepaddown,
    ev_gamepadup,
} evtype_t;

// Event structure.
typedef struct {
    evtype_t    type;
    int         data1;        // keys / mouse/joystick buttons
    float       data2;        // mouse/joystick x move
    float       data3;        // mouse/joystick y move
    int         data4;        // misc data
} event_t;


typedef enum {
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_exitdemo,
    ga_completed,
    ga_victory,
    ga_finale,
    ga_screenshot,
    ga_warplevel,
    ga_warpquick,
    ga_title
} gameaction_t;


//
// Button/action code definitions.
//
typedef enum {
    // Press "Fire".
    BT_ATTACK        = 1,
    // Use button, to open doors, activate switches.
    BT_USE           = 2,

    // Flag: game events, not really buttons.
    BT_SPECIAL       = 0x80,
    BT_SPECIALMASK   = 3,

    // Flag, weapon change pending.
    // If true, the next 3 bits hold weapon num.
    BT_CHANGE        = 4,
    // The 3bit weapon mask and shift, convenience.
    BT_WEAPONMASK    = 0x78,
    BT_WEAPONSHIFT   = 3,

    // Pause the game.
    BTS_PAUSE        = 1,
    // Save the game at each console.
    BTS_SAVEGAME     = 2,

    // Savegame slot numbers
    //  occupy the second byte of buttons.
    BTS_SAVEMASK     = (4+8+16),
    BTS_SAVESHIFT    = 2,
} buttoncode_t;

typedef enum {
    BT2_JUMP         = 1,
    BT2_NEXTWEAP     = 2,
    BT2_PREVWEAP     = 4,
    BT2_CENTER       = 8
} buttoncode2_t;




//
// GLOBAL VARIABLES
//

#define MAXEVENTS        64

extern  event_t events[MAXEVENTS];
extern  int     eventhead;
extern    int     eventtail;

extern  gameaction_t    gameaction;

#endif
