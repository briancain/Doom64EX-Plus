// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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
//
//
// DESCRIPTION:
//    DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//    plus functions to determine game mode (shareware, registered),
//    parse command line parameters, configure game parameters (turbo),
//    and call the startup functions.
//
//-----------------------------------------------------------------------------

#ifdef _WIN32
#include <io.h>
#endif

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_englsh.h"
#include "sounds.h"
#include "m_shift.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "f_finale.h"
#include "m_misc.h"
#include "m_menu.h"
#include "i_system.h"
#include "g_game.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "p_setup.h"
#include "d_main.h"
#include "con_console.h"
#include "d_devstat.h"
#include "r_local.h"
#include "r_wipe.h"
#include "g_controls.h"
#include "g_demo.h"
#include "p_saveg.h"
#include "gl_draw.h"
#include "deh_main.h"
#include "net_client.h"

//
// D_DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, and I_StartTic
//
void D_DoomLoop(void);

static int      pagetic;
static int      screenalpha;
static int      screenalphatext;
static int      creditstage;
static int      creditscreenstage;

int        InWindow;
int        InWindowBorderless;
boolean        setWindow = true;
int             validcount = 1;
boolean        windowpause = false;
boolean        devparm = false;    // started game with -devparm
boolean        nomonsters = false;    // checkparm of -nomonsters
boolean        respawnparm = false;    // checkparm of -respawn
boolean        respawnitem = false;    // checkparm of -respawnitem
boolean        fastparm = false;    // checkparm of -fast
boolean        BusyDisk = false;
boolean        nolights = false;
skill_t         startskill;
int             startmap;
boolean        autostart = false;
FILE* debugfile = NULL;
//char          wadfile[1024];              // primary wad file
char            mapdir[1024];               // directory of development maps
char            basedefault[1024];          // default file
boolean        rundemo4 = false;    // run demo lump #4?
int             gameflags = 0;
int             compatflags = 0;

void D_CheckNetGame(void);
void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t* cmd);

#define STRPAUSED    "Paused"

CVAR_EXTERNAL(sv_nomonsters);
CVAR_EXTERNAL(sv_fastmonsters);
CVAR_EXTERNAL(sv_respawnitems);
CVAR_EXTERNAL(sv_respawn);
CVAR_EXTERNAL(sv_skill);

//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
event_t            events[MAXEVENTS];
int                eventhead = 0;
int                eventtail = 0;

//
// D_PostEvent
// Called by the I/O functions when input is detected
//

void D_PostEvent(event_t* ev) {
	events[eventhead] = *ev;
	eventhead = (++eventhead) & (MAXEVENTS - 1);
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//

void D_ProcessEvents(void) {
	event_t* ev;

	for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1)) {
		ev = &events[eventtail];

		// 20120404 villsa - don't do console inputs for demo playbacks
		if (!demoplayback) {
			if (CON_Responder(ev)) {
				continue;    // console ate the event
			}
		}

		if (devparm && !netgame) {
			if (D_DevKeyResponder(ev)) {
				continue;    // dev keys ate the event
			}
		}

		if (M_Responder(ev)) {
			continue;    // menu ate the event
		}

		G_Responder(ev);
	}
}

//
// D_IncValidCount
//

void D_IncValidCount(void) {
	validcount++;
}

//
// D_MiniLoop
//

CVAR_EXTERNAL(i_interpolateframes);

extern boolean renderinframe;
extern int      gametime;
extern int      skiptics;

int             frameon = 0;
int             frametics[4];
int             frameskip[4];
int             oldnettics = 0;

int GetLowTic(void);
boolean PlayersInGame(void);

static void D_DrawInterface(void) {
	if (menuactive) {
		M_Drawer();
	}

	CON_Draw();

	if (devparm) {
		D_DeveloperDisplay();
	}

	BusyDisk = false;

	// draw pause pic
	if (paused) {
		Draw_BigText(-1, 64, WHITE, STRPAUSED);
	}
}

static void D_FinishDraw(void) {
	// send out any new accumulation
	NetUpdate();

	// normal update
	I_FinishUpdate();

	if (i_interpolateframes.value) {
		I_EndDisplay();
	}
}

int D_MiniLoop(void (*start)(void), void (*stop)(void),
	void (*draw)(void), boolean(*tick)(void)) {
	int action = gameaction = ga_nothing;

	if (start) {
		start();
	}

	while (!action) {
		int i = 0;
		int lowtic = 0;
		int entertic = 0;
		int oldentertics = 0;
		int realtics = 0;
		int availabletics = 0;
		int counts = 0;

		windowpause = (menuactive ? true : false);

		// process one or more tics

		// get real tics
		entertic = I_GetTime() / ticdup;
		realtics = entertic - oldentertics;
		oldentertics = entertic;

		if (i_interpolateframes.value) {
			renderinframe = true;

			if (I_StartDisplay()) {
				if (draw && !action) {
					draw();
				}
				D_DrawInterface();
				D_FinishDraw();
			}

			renderinframe = false;
		}

		// get available ticks

		NetUpdate();
		lowtic = GetLowTic();

		availabletics = lowtic - gametic / ticdup;

		// decide how many tics to run

		if (net_cl_new_sync) {
			counts = availabletics;
		}
		else {
			if (realtics < availabletics - 1) {
				counts = realtics + 1;
			}
			else if (realtics < availabletics) {
				counts = realtics;
			}
			else {
				counts = availabletics;
			}

			if (counts < 1) {
				counts = 1;
			}

			frameon++;

			if (!demoplayback) {
				int keyplayer = -1;

				// ideally maketic should be 1 - 3 tics above lowtic
				// if we are consistantly slower, speed up time

				for (i = 0; i < MAXPLAYERS; i++) {
					if (playeringame[i]) {
						keyplayer = i;
						break;
					}
				}

				if (keyplayer < 0) { // If there are no players, we can never advance anyway
					goto drawframe;
				}

				if (consoleplayer == keyplayer) {
					// the key player does not adapt
				}
				else {
					if (maketic <= nettics[keyplayer]) {
						gametime--;
						// I_Printf ("-");
					}

					frameskip[frameon & 3] = (oldnettics > nettics[keyplayer]);
					oldnettics = maketic;

					if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3]) {
						skiptics = 1;
						// I_Printf ("+");
					}
				}
			}
		}

		if (counts < 1) {
			counts = 1;
		}

		// wait for new tics if needed

		while (!PlayersInGame() || lowtic < gametic / ticdup + counts) {
			NetUpdate();
			lowtic = GetLowTic();

			if (lowtic < gametic / ticdup) {
				I_Error("D_MiniLoop: lowtic < gametic");
			}

			if (i_interpolateframes.value) {
				renderinframe = true;

				if (I_StartDisplay()) {
					if (draw && !action) {
						draw();
					}
					D_DrawInterface();
					D_FinishDraw();
				}

				renderinframe = false;
			}

			// Don't stay in this loop forever.  The menu is still running,
			// so return to update the screen

			if (I_GetTime() / ticdup - entertic > 0) {
				goto drawframe;
			}

			I_Sleep(1);
		}

		// run the count * ticdup dics
		while (counts--) {
			for (i = 0; i < ticdup; i++) {
				// check that there are players in the game.  if not, we cannot
				// run a tic.

				if (!PlayersInGame()) {
					break;
				}

				if (gametic / ticdup > lowtic) {
					I_Error("gametic>lowtic");
				}

				if (i_interpolateframes.value) {
					I_GetTime_SaveMS();
				}

				G_Ticker();

				if (tick) {
					action = tick();
				}

				if (gameaction != ga_nothing) {
					action = gameaction;
				}

				gametic++;

				// modify command for duplicated tics
				if (i != ticdup - 1) {
					ticcmd_t* cmd;
					int buf;
					int j;

					buf = (gametic / ticdup) % BACKUPTICS;
					for (j = 0; j < MAXPLAYERS; j++) {
						cmd = &netcmds[j][buf];
						cmd->chatchar = 0;
						if (cmd->buttons & BT_SPECIAL) {
							cmd->buttons = 0;
						}
					}
				}
			}

			NetUpdate();   // check for new console commands
		}

	drawframe:

		S_UpdateSounds();

		// Update display, next frame, with current state.
		if (i_interpolateframes.value) {
			if (!I_StartDisplay()) {
				goto freealloc;
			}
		}

		if (draw && !action) {
			draw();
		}
		D_DrawInterface();
		D_FinishDraw();

	freealloc:

		// force garbage collection
		Z_FreeAlloca();
	}

	gamestate = GS_NONE;

	if (stop) {
		stop();
	}

	return action;
}

//
// Title_Drawer
//

static void Title_Drawer(void) {
	GL_ClearView(0xFF000000);
	Draw_GfxImage(58, 50, "TITLE", WHITEALPHA(0x64), true);
}

//
// Title_Ticker
//

static int Title_Ticker(void) {
	if (mainmenuactive) {
		if ((gametic - pagetic) >= (TICRATE * 30)) {
			return 1;
		}
	}
	else {
		if (gametic != pagetic) {
			pagetic = gametic;
		}
	}

	return 0;
}

//
// Title_Start
//

static void Title_Start(void) {
	gameaction = ga_nothing;
	pagetic = gametic;
	usergame = false;   // no save / end game here
	paused = false;
	allowclearmenu = false;

	S_StartMusic(mus_title);
	M_StartMainMenu();
}

//
// Title_Stop
//

static void Title_Stop(void) {
	mainmenuactive = false;
	menuactive = false;
	allowmenu = false;
	allowclearmenu = true;

	WIPE_FadeScreen(8);
	S_StopMusic();
}

//
// Legal_Start
//

CVAR_EXTERNAL(p_regionmode);

static char* legalpic = "USLEGAL";
static int legal_x = 32;
static int legal_y = 72;

static void Legal_Start(void) {
	int pllump;
	int jllump;

	pllump = W_CheckNumForName("PLLEGAL");
	jllump = W_CheckNumForName("JPLEGAL");

	if (pllump == -1 && jllump == -1) {
		return;
	}

	if (p_regionmode.value >= 2 && jllump >= 0) {
		legalpic = "JPLEGAL";
		legal_x = 35;
		legal_y = 45;
	}
	else if (p_regionmode.value >= 2 && jllump == -1) {
		CON_CvarSetValue(p_regionmode.name, 1);
	}

	if (p_regionmode.value == 1 && pllump >= 0) {
		legalpic = "PLLEGAL";
		legal_x = 35;
		legal_y = 50;
	}
	else if (p_regionmode.value == 1 && pllump == -1) {
		CON_CvarSetValue(p_regionmode.name, 0);
	}
}

//
// Legal_Drawer
//

static void Legal_Drawer(void) {
	GL_ClearView(0xFF000000);
	Draw_GfxImage(legal_x, legal_y, legalpic, WHITE, true);
}

//
// Legal_Ticker
//

static int Legal_Ticker(void) {
	if ((gametic - pagetic) >= (TICRATE * 5)) {
		WIPE_FadeScreen(6);
		return 1;
	}

	return 0;
}

//
// Credits_Drawer
//

static void Credits_Drawer(void) {
	GL_ClearView(0xFF000000);

	switch (creditscreenstage) {
	case 0:
		Draw_GfxImage(72, 24, "IDCRED1",
			D_RGBA(255, 255, 255, (byte)screenalpha), true);

		Draw_GfxImage(40, 40, "IDCRED2",
			D_RGBA(255, 255, 255, (byte)screenalphatext), true);
		break;

	case 1:
		Draw_GfxImage(16, 80, "WMSCRED1",
			D_RGBA(255, 255, 255, (byte)screenalpha), true);

		Draw_GfxImage(32, 24, "WMSCRED2",
			D_RGBA(255, 255, 255, (byte)screenalphatext), true);
		break;

	case 2:
		Draw_GfxImage(64, 30, "EVIL",
			D_RGBA(255, 255, 255, (byte)screenalpha), true);
		break;
	}
}

//
// Credits_Ticker
//

static int Credits_Ticker(void) {
	switch (creditstage) {
	case 0:
		if (screenalpha < 0xff) {
			screenalpha = MIN(screenalpha + 8, 0xff);
		}
		else {
			creditstage = 1;
		}
		break;

	case 1:
		if (screenalphatext < 0xff) {
			screenalphatext = MIN(screenalphatext + 8, 0xff);
		}
		else {
			creditstage = 2;
		}
		break;

	case 2:
		if ((gametic - pagetic) >= (TICRATE * 6)) {
			screenalpha = MAX(screenalpha - 8, 0);
			screenalphatext = MAX(screenalphatext - 8, 0);

			if (screenalpha <= 0) {
				creditstage = 3;
				creditscreenstage++;
			}
		}
		break;

	case 3:
		if (creditscreenstage >= 3) {
			return 1;
		}

		screenalpha = 0;
		screenalphatext = 0;
		creditstage = 0;
		pagetic = gametic;

		break;
	}

	return 0;
}

//
// Credits_Start
//

static void Credits_Start(void) {
	screenalpha = 0;
	screenalphatext = 0;
	creditstage = 0;
	creditscreenstage = 0;
	pagetic = gametic;
	allowmenu = false;
	menuactive = false;
	usergame = false;   // no save / end game here
	paused = false;
	gamestate = GS_SKIPPABLE;
}

//
// D_SplashScreen
//

static void D_SplashScreen(void) {
	int skip = 0;

	if (gameaction || netgame) {
		return;
	}

	screenalpha = 0xff;
	allowmenu = false;
	menuactive = false;

	gamestate = GS_SKIPPABLE;
	pagetic = gametic;
	gameaction = ga_nothing;

	skip = D_MiniLoop(Legal_Start, NULL, Legal_Drawer, Legal_Ticker);

	if (skip != ga_title) {
		G_RunTitleMap();
		gameaction = ga_title;
	}
}

//
// D_DoomLoop
// Main game loop
//

void D_DoomLoop(void) {
	int exit;

	if (netgame) {
		gameaction = ga_newgame;
	}

	exit = gameaction;

	while (1) {
		exit = D_MiniLoop(Title_Start, Title_Stop, Title_Drawer, Title_Ticker);

		if (exit == ga_newgame || exit == ga_loadgame) {
			G_RunGame();
		}
		else {
			D_MiniLoop(Credits_Start, NULL, Credits_Drawer, Credits_Ticker);

			if (gameaction == ga_title) {
				continue;
			}
			if (exit != ga_exitdemo)
			{
				iwadDemo = true;
				exit = D_RunDemo("DEMO1LMP", sk_medium, 3);
			}

			iwadDemo = true;
			if (exit != ga_exitdemo)
			{
				exit = D_RunDemo("DEMO2LMP", sk_medium, 9);
			}

			iwadDemo = true;
			if (exit != ga_exitdemo)
			{
				exit = D_RunDemo("DEMO3LMP", sk_medium, 17);
			}

			if (rundemo4) {
				iwadDemo = true;
				if (exit != ga_exitdemo)
				{
					exit = D_RunDemo("DEMO4LMP", sk_medium, 32);
				}

				G_RunTitleMap();
				continue;
			}
		}
	}
}

//      print title for every printed line
char title[128];

//
// Find a Response File
//

#define MAXARGVS 100

static void FindResponseFile(void) {
	int i;

	for (i = 1; i < myargc; i++) {
		if (myargv[i][0] == '@') {
			FILE* handle;
			int     size;
			int     k;
			int     index;
			int     indexinfile;
			char* infile;
			char* file;
			char* moreargs[20];
			char* firstargv;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen(&myargv[i][1], "rb");
			if (!handle) {
				//                I_Warnf (IWARNMINOR, "\nNo such response file!");
				exit(1);
			}
			I_Printf("Found response file %s!\n", &myargv[i][1]);
			fseek(handle, 0, SEEK_END);
			size = ftell(handle);
			fseek(handle, 0, SEEK_SET);
			file = (char*)malloc(size);
			fread(file, size, 1, handle);
			fclose(handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (index = 0, k = i + 1; k < myargc; k++) {
				moreargs[index++] = myargv[k];
			}

			firstargv = myargv[0];
			myargv = (char**)malloc(sizeof(char*) * MAXARGVS);
			dmemset(myargv, 0, sizeof(char*) * MAXARGVS);
			myargv[0] = firstargv;

			infile = file;
			indexinfile = k = 0;
			indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
			do {
				myargv[indexinfile++] = infile + k;
				while (k < size &&
					((*(infile + k) >= ' ' + 1) && (*(infile + k) <= 'z'))) {
					k++;
				}
				*(infile + k) = 0;
				while (k < size &&
					((*(infile + k) <= ' ') || (*(infile + k) > 'z'))) {
					k++;
				}
			} while (k < size);

			for (k = 0; k < index; k++) {
				myargv[indexinfile++] = moreargs[k];
			}
			myargc = indexinfile;

			// DISPLAY ARGS
			for (k = 1; k < myargc; k++) {
				I_Printf("%d command-line args: %s\n", myargc, myargv[k]);
			}

			break;
		}
	}
}

//
// D_Init
//

static void D_Init(void) {
	int     p;

	FindResponseFile();

	nomonsters = M_CheckParm("-nomonsters");
	respawnparm = M_CheckParm("-respawn");
	respawnitem = M_CheckParm("-respawnitem");
	fastparm = M_CheckParm("-fast");

	if ((p = M_CheckParm("-setvars"))) {
		p++;

		while (p != myargc && myargv[p][0] != '-') {
			char* name;
			char* value;

			name = myargv[p++];
			value = myargv[p++];

			CON_CvarSet(name, value);
		}
	}

	if (M_CheckParm("-deathmatch")) {
		deathmatch = 1;
	}

	// get skill / episode / map from parms
	startskill = sk_medium;
	startmap = 1;
	autostart = false;

	p = M_CheckParm("-skill");
	if (p && p < myargc - 1) {
		startskill = myargv[p + 1][0] - '1';
		autostart = true;
		gameaction = ga_newgame;
	}

	p = M_CheckParm("-timer");
	if (p && p < myargc - 1 && deathmatch) {
		int     time;
		time = datoi(myargv[p + 1]);
		I_Printf("Levels will end after %d minute\n", time);

		if (time > 1) {
			I_Printf("s");
		}

		I_Printf(".\n");
	}

	p = M_CheckParm("-warp");
	if (p && p < myargc - 1) {
		autostart = true;
		startmap = datoi(myargv[p + 1]);
		gameaction = ga_newgame;
	}

	// set server cvars
	CON_CvarSetValue(sv_skill.name, (float)startskill);
	CON_CvarSetValue(sv_respawn.name, (float)respawnparm);
	CON_CvarSetValue(sv_respawnitems.name, (float)respawnitem);
	CON_CvarSetValue(sv_fastmonsters.name, (float)fastparm);
	CON_CvarSetValue(sv_nomonsters.name, (float)nomonsters);

	p = M_CheckParm("-loadgame");
	if (p && p < myargc - 1) {
		// sprintf(file, SAVEGAMENAME"%c.dsg",myargv[p+1][0]);
		G_LoadGame(P_GetSaveGameName(myargv[p + 1][0] - '0'));
		autostart = true; // 20120105 bkw: this was missing
	}

	if (M_CheckParm("-nogun")) {
		ShowGun = false;
	}
}

//
// D_CheckDemo
//

static int D_CheckDemo(void) {
	int p;

	// start the apropriate game based on parms
	p = M_CheckParm("-record");

	if (p && p < myargc - 1) {
		G_RecordDemo(myargv[p + 1]);
		return 1;
	}

	p = M_CheckParm("-playdemo");
	if (p && p < myargc - 1) {
		singledemo = true;              // quit after one demo
		G_PlayDemo(myargv[p + 1]);
		return 1;
	}

	return 0;
}

//
// D_DoomMain
//

void D_DoomMain(void) {
	devparm = M_CheckParm("-devparm");

	// init subsystems

	I_Printf("Z_Init: Init Zone Memory Allocator\n");
	Z_Init();

	I_Printf("CON_Init: Init Game Console\n");
	CON_Init();

	I_Printf("G_Init: Setting up game input and commands\n");
	G_Init();

	I_Printf("M_LoadDefaults: Loading game configuration\n");
	M_LoadDefaults();

	I_Printf("I_Init: Setting up machine state.\n");
	I_Init();

	I_Printf("D_Init: Init DOOM parameters\n");
	D_Init();

	I_Printf("W_Init: Init WADfiles.\n");
	W_Init();

	// Load Dehacked patches specified on the command line with -deh.
	// Note that there's a very careful and deliberate ordering to how
	// Dehacked patches are loaded. The order we use is:
	//  1. IWAD dehacked patches.
	//  2. Command line dehacked patches specified with -deh.
	//  3. PWAD dehacked patches in DEHACKED lumps.
	DEH_ParseCommandLine();

	I_Printf("M_Init: Init miscellaneous info.\n");
	M_Init();

	I_Printf("R_Init: Init DOOM refresh daemon.\n");
	R_Init();

	I_Printf("P_Init: Init Playloop state.\n");
	P_Init();

	I_Printf("NET_Init: Init network subsystem.\n");
	NET_Init();

	I_Printf("S_Init: Setting up sound.\n");
	S_Init();

	I_Printf("D_CheckNetGame: Checking network game status.\n");
	D_CheckNetGame();

	I_Printf("ST_Init: Init status bar.\n");
	ST_Init();

	I_Printf("GL_Init: Init OpenGL\n");
	GL_Init();

	// garbage collection
	Z_FreeAlloca();

	if (!D_CheckDemo()) {
		if (!autostart) {
			// start legal screen and title map stuff
			D_SplashScreen();
		}
		else {
			G_RunGame();
		}
	}

	D_DoomLoop();   // never returns
}
