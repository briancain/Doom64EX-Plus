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
//
// DESCRIPTION: System Interface
//
//-----------------------------------------------------------------------------

#ifdef __OpenBSD__
#include <SDL_timer.h>
#else
#include <SDL3/SDL_timer.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#else
#include <windows.h>
#include <direct.h>
#include <io.h>
#endif

#include <stdarg.h>
#include <sys/stat.h>
#include "doomstat.h"
#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_net.h"
#include "g_demo.h"
#include "d_main.h"
#include "con_console.h"
#include "z_zone.h"
#include "i_system.h"
#include "gl_draw.h"
#include "i_xinput.h"

#ifdef __APPLE__
#include "i_mac_audio.h"
#else
#include "i_audio.h"
#endif

#if defined(_WIN32) && defined(USE_XINPUT)
#include "i_xinput.h"
#endif

CVAR(i_interpolateframes, 1);
CVAR(v_accessibility, 0);

// Gibbon - hack from curl to deal with some crap
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#ifdef DOOM_UNIX_INSTALL
#define GetBasePath()	SDL_GetPrefPath("", "doom64ex-plus");
#elif !defined DOOM_UNIX_INSTALL || defined _WIN32 || !defined __ANDROID__
#define GetBasePath()	SDL_GetBasePath();
#elif defined __ANDROID__
#define GetBasePath()   SDL_AndroidGetInternalStoragePath();
#endif

ticcmd_t        emptycmd;

//
// I_uSleep
//

void I_Sleep(unsigned long usecs) {
#ifdef _WIN32
	Sleep((DWORD)usecs);
#else
	struct timespec tc;
	tc.tv_sec = usecs / 1000;
	tc.tv_nsec = (usecs % 1000) * 1000000;
	nanosleep(&tc, NULL);
#endif
}

static Uint64 basetime = 0;

//
// I_GetTimeNormal
//

static int I_GetTimeNormal(void) {
	Uint64 ticks;
	Uint64 tic_division = 1000;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	ticks -= basetime;

	return (ticks * TICRATE) / tic_division;
}

//
// I_GetTime_Error
//

static int I_GetTime_Error(void) {
	I_Error("I_GetTime_Error: GetTime() used before initialization");
	return 0;
}

//
// I_InitClockRate
//

void I_InitClockRate(void) {
	I_GetTime = I_GetTimeNormal;
}

//
// FRAME INTERPOLTATION
//

static Uint64 start_displaytime;
static Uint64 displaytime;
static boolean InDisplay = false;

boolean realframe = false;

fixed_t         rendertic_frac = 0;
Uint64			rendertic_start;
Uint64			rendertic_step;
Uint64			rendertic_next;
const float     rendertic_msec = 100 * TICRATE / 100000.0f;

//
// I_StartDisplay
//

boolean I_StartDisplay(void) {
	rendertic_frac = I_GetTimeFrac();

	if (InDisplay) {
		return false;
	}

	start_displaytime = SDL_GetTicks();
	InDisplay = true;

	return true;
}

//
// I_EndDisplay
//

void I_EndDisplay(void) {
	displaytime = SDL_GetTicks() - start_displaytime;
	InDisplay = false;
}

//
// I_GetTimeFrac
//

fixed_t I_GetTimeFrac(void) {
	Uint64 now;
	fixed_t frac;

	now = SDL_GetTicks();

	if (rendertic_step == 0) {
		return FRACUNIT;
	}
	else {
		frac = (fixed_t)((now - rendertic_start + displaytime) * FRACUNIT / rendertic_step);
		if (frac < 0) {
			frac = 0;
		}
		if (frac > FRACUNIT) {
			frac = FRACUNIT;
		}
		return frac;
	}
}

//
// I_GetTimeMS
//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void) {
	Uint64 ticks;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	return ticks - basetime;
}

//
// I_GetTime_SaveMS
//

void I_GetTime_SaveMS(void) {
	rendertic_start = SDL_GetTicks();
	rendertic_next = (unsigned int)((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
	rendertic_step = rendertic_next - rendertic_start;
}

//
// I_BaseTiccmd
//

ticcmd_t* I_BaseTiccmd(void) {
	return &emptycmd;
}

/**
 * @brief Get the user-writeable directory.
 *
 * Assume this is the only user-writeable directory on the system.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not found.
 */

char* I_GetUserDir(void) 
{
	return GetBasePath();
}

/*
 * @brief Get the directory which contains this program.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not 

 * @brief Find a regular file in the user-writeable directory.
 *
 * @return Fully-qualified path or NULL if not found.
 */

char* I_GetUserFile(char* file) {
	const char* path, * userdir;

	if (!(userdir = I_GetUserDir()))
		return NULL;

	path = malloc(512);

	snprintf(path, 511, "%s%s", userdir, file);

	return path;
}

/**
 * @brief Find a regular read-only data file.
 *
 * @return Fully-qualified path or NULL if not found.
 */

char* I_FindDataFile(char* file) {
	char* path = malloc(512);
	const char* dir;

	if (path == NULL) {
		return NULL;
	}

	if ((dir = SDL_GetBasePath())) {
		snprintf(path, 511, "%s%s", dir, file);
		if (I_FileExists(path)) {
			return path;
		}
	}

#if !defined(_WIN32)
#ifdef DOOM_UNIX_SYSTEM_DATADIR
	snprintf(path, 511, "%s/%s", DOOM_UNIX_SYSTEM_DATADIR, file);
	if (I_FileExists(path)) {
		return path;
	}
#endif

	snprintf(path, 511, "%s", file);
	if (I_FileExists(path)) {
		return path;
	}

	const char* homeDir = getenv("HOME");
	if (homeDir) {
		snprintf(path, 511, "%s/.steam/steam/steamapps/common/DOOM 64/%s", homeDir, file);
		if (I_FileExists(path)) {
			I_Printf("I_FindDataFile: Adding Steam Path %s\n", path);
			return path;
		}

		snprintf(path, 511, "%s/.local/share/Steam/steamapps/common/DOOM 64/%s", homeDir, file);
		if (I_FileExists(path)) {
			I_Printf("I_FindDataFile: Adding Steam Path %s\n", path);
			return path;
		}
	}

	snprintf(path, 511, "%s/GOG Games/DOOM 64/%s", homeDir, file);
	if (I_FileExists(path)) {
		I_Printf("I_FindDataFile: Adding GOG Path %s\n", path);
		return path;
	}

#elif defined(_WIN32)
	const char* steamPath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\DOOM 64";
	snprintf(path, 511, "%s\\%s", steamPath, file);
	if (I_FileExists(path)) {
		I_Printf("I_FindDataFile: Adding Steam Path %s\n", path);
		return path;
	}

	const char* gogPath = "C:\\Program Files (x86)\\GOG Galaxy\\Games\\DOOM 64";
	snprintf(path, 511, "%s\\%s", gogPath, file);
	if (I_FileExists(path)) {
		I_Printf("I_FindDataFile: Adding GOG Path %s\n", path);
		return path;
	}
#endif

	free(path);
	return NULL;
}

/**
 * @brief Checks to see if the given absolute path is a regular file.
 * @param path Absolute path to check.
 */

boolean I_FileExists(const char* path)
{
	struct stat st;
	return !stat(path, &st) && S_ISREG(st.st_mode);
}

//
// I_GetTime
// returns time in 1/70th second tics
// now 1/35th?
//

int (*I_GetTime)(void) = I_GetTime_Error;

//
// I_GetRandomTimeSeed
//

unsigned long I_GetRandomTimeSeed(void) {
	// not exactly random....
	return SDL_GetTicks();
}

//
// I_Init
//

void I_Init(void)
{
	I_InitJoystick();
	I_InitVideo();
	I_InitClockRate();
}

//
// I_Error
//

void I_Error(const char* string, ...) {
	char buff[1024];
	va_list    va;

	I_ShutdownSound();

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);

	fprintf(stderr, "Error - %s\n", buff);
	fflush(stderr);

	I_Printf("\n********* ERROR *********\n");
	I_Printf("%s", buff);

	if (usingGL) {
		while (1) {
			GL_ClearView(0xFF000000);
			Draw_Text(0, 0, WHITE, 1, 1, "Error - %s\n", buff);
			GL_SwapBuffers();

			if (I_ShutdownWait()) {
				break;
			}

			I_Sleep(1);
		}
	}
	else {
		I_ShutdownVideo();
	}
	exit(0);    // just in case...
}

void I_Warning(const char* string, ...) {
	char buff[1024];
	va_list    va;

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);

	fprintf(stderr, "Warning - %s\n", buff);
	fflush(stderr);

	I_Printf("\n********* WARNING *********\n");
	I_Printf("%s", buff);

	if (usingGL) {
		while (1) {
			GL_ClearView(0xFF000000);
			Draw_Text(0, 0, WHITE, 1, 1, "Warning - %s\n", buff);
			GL_SwapBuffers();

			I_Sleep(1);
		}
	}
}

//
// I_Quit
//

void I_Quit(void) {
	if (demorecording) {
		endDemo = true;
		G_CheckDemoStatus();
	}
	M_SaveDefaults();
	I_ShutdownSound();
	I_ShutdownVideo();

	exit(0);
}

//
// I_Printf
//

void I_Printf(const char* string, ...) {
	char buff[1024];
	va_list    va;

	dmemset(buff, 0, 1024);

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);
	printf("%s", buff);
	if (console_initialized) {
		CON_AddText(buff);
	}
}

//
// I_BeginRead
//

boolean    inshowbusy = false;

void I_BeginRead(void) {
	if (!devparm) {
		return;
	}

	if (inshowbusy) {
		return;
	}
	inshowbusy = true;
	inshowbusy = false;
	BusyDisk = true;
}

//
// I_RegisterCvars
//

#if defined(_WIN32) && defined(USE_XINPUT)
CVAR_EXTERNAL(i_rsticksensitivity);
CVAR_EXTERNAL(i_rstickthreshold);
CVAR_EXTERNAL(i_xinputscheme);
#endif

CVAR_EXTERNAL(i_gamma);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(v_accessibility);

void I_RegisterCvars(void) {
#if defined(_WIN32) && defined(USE_XINPUT)
	CON_CvarRegister(&i_rsticksensitivity);
	CON_CvarRegister(&i_rstickthreshold);
	CON_CvarRegister(&i_xinputscheme);
#endif
	CON_CvarRegister(&i_gamma);
	CON_CvarRegister(&i_brightness);
	CON_CvarRegister(&i_interpolateframes);
	CON_CvarRegister(&v_accessibility);
}
