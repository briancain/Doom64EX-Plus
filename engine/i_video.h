// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
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

#ifndef __I_VIDEO_H__
#define __I_VIDEO_H__

#ifdef USE_IMGUI
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define CIMGUI_USE_OPENGL3
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <backend/imgui_impl_sdl.h>
#ifdef __LEGACYGL__
#include <backend/imgui_impl_opengl2.h>
#else
#include <backend/imgui_impl_opengl3.h>
#endif
#endif

#include "gl_utils.h"
#include "d_event.h"

////////////Video///////////////

extern SDL_Surface* screen;
extern SDL_Window* window;
extern const char version_date[];
#ifdef USE_IMGUI
extern ImGuiIO* io;
extern ImGuiContext* ctx;
#ifdef __LEGACYGL__
#define ImGuiImplGL ImGui_ImplOpenGL2_Init
#else
#define ImGuiImplGL ImGui_ImplOpenGL3_Init
#endif
#endif
void I_InitVideo(void);
void I_InitScreen(void);
void I_ShutdownVideo(void);
void V_RegisterCvars();
#ifdef USE_GLFW
void I_ResizeCallback(OGL_DEFS, int width, int height);
#endif

#endif