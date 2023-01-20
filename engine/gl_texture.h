// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2022-2023 André Guilherme
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

#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include "gl_utils.h"

extern int                  curtexture;
extern int                  cursprite;
extern int                    curtrans;
extern int                  curgfx;

extern unsigned short* texturewidth;
extern unsigned short* textureheight;
extern unsigned int** textureptr;
extern int                  t_start;
extern int                  t_end;
extern int                  swx_start;
extern int                  numtextures;
extern unsigned short* texturetranslation;
extern unsigned short* palettetranslation;

extern int                  g_start;
extern int                  g_end;
extern int                  numgfx;
extern unsigned int* gfxptr;
extern unsigned short* gfxwidth;
extern unsigned short* gfxorigwidth;
extern unsigned short* gfxheight;
extern unsigned short* gfxorigheight;

extern int                  s_start;
extern int                  s_end;
extern unsigned int** spriteptr;
extern int                  numsprtex;
extern unsigned short* spritewidth;
extern float* spriteoffset;
extern float* spritetopoffset;
extern unsigned short* spriteheight;

void        GL_InitTextures(void);
void        GL_UnloadTexture(unsigned int* texture);
void        GL_SetTextureUnit(int unit, boolean enable);
void        GL_SetTextureMode(int mode);
void        GL_SetEnvColor(float* param);
void        GL_BindWorldTexture(int texnum, int* width, int* height);
void        GL_BindSpriteTexture(int spritenum, int pal);
int         GL_BindGfxTexture(const char* name, boolean alpha);
int         GL_PadTextureDims(int size);
void        GL_SetNewPalette(int id, unsigned char palID);
void        GL_DumpTextures(void);
void        GL_BindDummyTexture(void);
void        GL_UpdateEnvTexture(unsigned int color);
void        GL_BindEnvTexture(void);
unsigned int    GL_ScreenToTexture(void);
#endif