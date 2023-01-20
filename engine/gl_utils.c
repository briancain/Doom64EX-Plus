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
// DESCRIPTION:
//		It stores some OpenGL utils here:
// 
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Inlined OpenGL-exclusive functions
//
//-----------------------------------------------------------------------------


#ifdef __APPLE__
#include <math.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "con_console.h"
#include "i_system.h"
#include "gl_utils.h"

#define MAXINDICES  0x10000

unsigned short statindice = 0;

static unsigned short indicecnt = 0;
static unsigned short drawIndices[MAXINDICES];

void glGetVersion(int major, int minor)
{
	OGL_WINDOW_HINT(OGL_MAJOR_VERSION, major);
	OGL_WINDOW_HINT(OGL_MINOR_VERSION, minor);
}

void glDestroyWindow(OGL_DEFS)
{
	if (Window)
	{
		OGL_DESTROY_WINDOW(Window);
		Window = NULL;
	}
}

//
// glLogError
//

#ifdef USE_DEBUG_GLFUNCS
void glLogError(const char* message, const char* file, int line) {
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		char str[64];

		switch (err) {
		case GL_INVALID_ENUM:
			strcpy(str, "INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			strcpy(str, "INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			strcpy(str, "INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			strcpy(str, "STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			strcpy(str, "STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			strcpy(str, "OUT_OF_MEMORY");
			break;
		default:
			sprintf(str, "0x%x", err);
			break;
		}

		I_Printf("\nGL ERROR (%s) on gl function: %s (file = %s, line = %i)\n\n", str, message, file, line);
		I_Sleep(1);
	}
}
#endif

//
// glSetVertex
//

static vtx_t* gl_prevptr = NULL;

void glSetVertex(vtx_t* vtx) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glSetVertex(vtx=0x%p)\n", vtx);
#endif

	// 20120623 villsa - avoid redundant calls by checking for
	// the previous pointer that was set
	if (gl_prevptr == vtx) {
		return;
	}

	glTexCoordPointer(2, GL_FLOAT, sizeof(vtx_t), &vtx->tu);
	glVertexPointer(3, GL_FLOAT, sizeof(vtx_t), vtx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vtx_t), &vtx->r);

	gl_prevptr = vtx;
}

//
// glTriangle
//

void glTriangle(int v0, int v1, int v2) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glTriangle(v0=%i, v1=%i, v2=%i)\n", v0, v1, v2);
#endif
	if (indicecnt + 3 >= MAXINDICES) {
		I_Error("Triangle indice overflow");
	}

	drawIndices[indicecnt++] = v0;
	drawIndices[indicecnt++] = v1;
	drawIndices[indicecnt++] = v2;
}

//
// glDrawGeometry
//

void glDrawGeometry(int count, vtx_t* vtx) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glDrawGeometry(count=0x%x, vtx=0x%p)\n", count, vtx);
#endif

	glDrawElements(GL_TRIANGLES, indicecnt, GL_UNSIGNED_SHORT, drawIndices);


	if (devparm) {
		statindice += indicecnt;
	}

	indicecnt = 0;
}

//
// glViewFrustum
//

void glViewFrustum(int width, int height, float fovy, float znear) {
	float left;
	float right;
	float bottom;
	float top;
	float aspect;
	float m[16];

#ifdef LOG_GLFUNC_CALLS
	I_Printf("glViewFrustum(width=%i, height=%i, fovy=%f, znear=%f)\n", width, height, fovy, znear);
#endif

	aspect = (float)width / (float)height;
	top = znear * (float)tan((double)fovy * M_PI / 360.0f);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;

	m[0] = (2 * znear) / (right - left);
	m[4] = 0;
	m[8] = (right + left) / (right - left);
	m[12] = 0;

	m[1] = 0;
	m[5] = (2 * znear) / (top - bottom);
	m[9] = (top + bottom) / (top - bottom);
	m[13] = 0;

	m[2] = 0;
	m[6] = 0;
	m[10] = -1;
	m[14] = -2 * znear;

	m[3] = 0;
	m[7] = 0;
	m[11] = -1;
	m[15] = 0;

	glMultMatrixf(m);
}

//
// glSetVertexColor
//

void glSetVertexColor(vtx_t* v, unsigned int color, unsigned short count) {
	int i = 0;
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glSetVertexColor(v=0x%p, c=0x%x, count=0x%x)\n", v, c, count);
#endif
	for (i = 0; i < count; i++) {
		*(unsigned int*)&v[i].r = color;
	}
}

//
// glGetColorf
//

void glGetColorf(unsigned int color, float* argb) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glGetColorf(color=0x%x, argb=0x%p)\n", color, argb);
#endif
	argb[3] = (float)((color >> 24) & 0xff) / 255.0f;
	argb[2] = (float)((color >> 16) & 0xff) / 255.0f;
	argb[1] = (float)((color >> 8) & 0xff) / 255.0f;
	argb[0] = (float)(color & 0xff) / 255.0f;
}