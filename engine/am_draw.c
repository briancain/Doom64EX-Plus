// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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
// DESCRIPTION: Automap rendering code
//
//-----------------------------------------------------------------------------

#include "r_lights.h"
#include "m_fixed.h"
#include "tables.h"
#include "doomstat.h"
#include "z_zone.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "am_map.h"
#include "am_draw.h"
#include "m_cheat.h"
#include "r_sky.h"
#include "p_local.h"
#include "r_clipper.h"
#include "r_drawlist.h"
#include "gl_utils.h"

extern fixed_t automappanx;
extern fixed_t automappany;
extern unsigned char amModeCycle;

static angle_t am_viewangle;

CVAR_EXTERNAL(am_fulldraw);
CVAR_EXTERNAL(am_ssect);

//
// AM_BeginDraw
//

void AM_BeginDraw(angle_t view, fixed_t x, fixed_t y) {
	am_viewangle = view;

	glDepthRange(0.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewFrustum(video_width, video_height, 45.0f, 0.1f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glTranslatef(-F2D3D(automappanx), -F2D3D(automappany), 0);
	glRotatef(-(float)TRUEANGLES(am_viewangle), 0.0f, 0.0f, 1.0f);
	glTranslatef(-F2D3D(x), -F2D3D(y), 0);

	drawlist[DLT_AMAP].index = 0;

	R_FrustrumSetup();
	curtexture = cursprite = curgfx = -1;
}

//
// AM_EndDraw
//

void AM_EndDraw(void) {
	glPopMatrix();
	glDepthRange(0.0f, 1.0f);

	GL_SetDefaultCombiner();
}

//
// DL_ProcessAutomap
//

static float am_drawscale = 0.0f;
static boolean DL_ProcessAutomap(vtxlist_t* vl, int* drawcount) {
	leaf_t* leaf;
	unsigned int color;
	fixed_t tx;
	fixed_t ty;
	vtx_t* v;
	int j;
	int count;
	subsector_t* sub;

	sub = (subsector_t*)vl->data;
	leaf = &leafs[sub->leaf];
	count = *drawcount;

	for (j = 0; j < sub->numleafs - 2; j++) {
		glTriangle(count, count + 1 + j, count + 2 + j);
	}

	tx = (leaf->vertex->x >> 6) & ~(FRACUNIT - 1);
	ty = (leaf->vertex->y >> 6) & ~(FRACUNIT - 1);

	//
	// setup RGB data
	//
	if (am_ssect.value) {
		w3sint64_t num = sub - subsectors;
		color = D_RGBA(
			(num * 0x3f) & 0xff,
			(num * 0xf) & 0xff,
			(num * 0x7) & 0xff,
			0xff
		);

		if (color == 0xff000000) {
			color = D_RGBA(0x80, 0x80, 0x80, 0xff);
		}
	}
	else {
		if (nolights) {
			color = WHITE;
		}
		else {
			light_t* light;

			light = &lights[sub->sector->colors[LIGHT_FLOOR]];
			color = D_RGBA(
				light->active_r,
				light->active_g,
				light->active_b,
				0xff
			);
		}
	}

	if (am_overlay.value) {
		color -= D_RGBA(0, 0, 0, 0xBF);
	}

	v = &drawVertex[count];

	glSetVertexColor(v, color, sub->numleafs);

	//
	// setup vertex data
	//
	for (j = 0; j < sub->numleafs; j++) {
		vertex_t* vertex;

		vertex = leafs[sub->leaf + j].vertex;

		v[j].x = F2D3D(vertex->x);
		v[j].y = F2D3D(vertex->y);
		v[j].z = -(am_drawscale * 2);

		v[j].tu = F2D3D((vertex->x >> 6) - tx);
		v[j].tv = -F2D3D((vertex->y >> 6) - ty);

		count++;
	}

	*drawcount = count;

	return true;
}

//
// AM_DrawLeafs
//

void AM_DrawLeafs(float scale) {
	subsector_t* sub;
	drawlist_t* am_drawlist;
	int i;
	int j;

	am_drawlist = &drawlist[DLT_AMAP];

	for (i = 0; i < numsubsectors; i++) {
		sub = &subsectors[i];

		//
		// don't add sky flats
		//
		if (sub->sector->floorpic == skyflatnum) {
			continue;
		}

		//
		// must be mapped
		//
		if (segs[sub->firstline].linedef->flags & ML_MAPPED || amCheating) {
			//
			// add to draw list if visible
			//
			if (!(sub->sector->flags & MS_HIDESSECTOR) || am_fulldraw.value) {
				vtxlist_t* list;
				vtx_t* v = &drawVertex[0];

				for (j = 0; j < sub->numleafs; j++) {
					vertex_t* vertex;

					vertex = leafs[sub->leaf + j].vertex;

					v[j].x = F2D3D(vertex->x);
					v[j].y = F2D3D(vertex->y);
					v[j].z = -(scale * 2);
				}

				if (!R_FrustrumTestVertex(v, sub->numleafs)) {
					continue;
				}

				list = DL_AddVertexList(am_drawlist);
				list->data = (subsector_t*)sub;
				list->callback = NULL;
				list->texid = sub->sector->floorpic;
			}
		}
	}

	am_drawscale = scale;

	//
	// process draw list
	//
	DL_BeginDrawList(!am_ssect.value && 1);

	if (!nolights) {
		GL_SetTextureMode(GL_MODULATE);
	}
	else {
		GL_SetTextureMode(GL_REPLACE);
	}

	DL_ProcessDrawList(DLT_AMAP, DL_ProcessAutomap);
}

//
// AM_DrawLine
//

void AM_DrawLine(int x1, int x2, int y1, int y2, float scale, unsigned int c) {
	vtx_t v[2];

	v[0].x = F2D3D(x1);
	v[0].z = F2D3D(y1);
	v[1].x = F2D3D(x2);
	v[1].z = F2D3D(y2);

	v[0].y = v[1].y = (scale * 2);

	glSetVertexColor(v, c, 2);

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4ub(v[0].r, v[0].g, v[0].b, v[0].a);
	glVertex3f(v[0].x, v[0].z, -v[0].y);
	glColor4ub(v[1].r, v[1].g, v[1].b, v[1].a);
	glVertex3f(v[1].x, v[1].z, -v[1].y);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}

//
// AM_DrawTriangle
//

void AM_DrawTriangle(mobj_t* mobj, float scale, boolean solid, unsigned char r, unsigned char g, unsigned char b) {
	vtx_t tri[3];
	fixed_t x;
	fixed_t y;
	angle_t angle;

	if (mobj->flags & (MF_NOSECTOR | MF_RENDERLASER)) {
		return;
	}
	else if (mobj->state == (state_t*)S_NULL) {
		return;
	}

	x = mobj->x;
	y = mobj->y;
	angle = mobj->angle;

	tri[0].z = tri[1].z = tri[2].z = -(scale * 2);
	tri[0].tu = tri[1].tu = tri[2].tu = 0.0f;
	tri[0].tv = tri[1].tv = tri[2].tv = 0.0f;

	tri[0].x = F2D3D((finecosine[(angle) >> ANGLETOFINESHIFT] << 5) + x);
	tri[0].y = F2D3D((finesine[(angle) >> ANGLETOFINESHIFT] << 5) + y);

	tri[1].x = F2D3D((finecosine[(angle + 0xA0000000) >> ANGLETOFINESHIFT] << 5) + x);
	tri[1].y = F2D3D((finesine[(angle + 0xA0000000) >> ANGLETOFINESHIFT] << 5) + y);

	tri[2].x = F2D3D((finecosine[(angle + 0x60000000) >> ANGLETOFINESHIFT] << 5) + x);
	tri[2].y = F2D3D((finesine[(angle + 0x60000000) >> ANGLETOFINESHIFT] << 5) + y);

	tri[0].r = tri[1].r = tri[2].r = r;
	tri[0].g = tri[1].g = tri[2].g = g;
	tri[0].b = tri[1].b = tri[2].b = b;
	tri[0].a = tri[1].a = tri[2].a = 0xff;

	if (!R_FrustrumTestVertex(tri, 3)) {
		return;
	}

	glPolygonMode(GL_FRONT_AND_BACK, (solid == 1) ? GL_LINE : GL_FILL);

	glSetVertex(tri);
	glTriangle(0, 1, 2);
	glDisable(GL_TEXTURE_2D);
	glDrawGeometry(3, tri);
	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (devparm) {
		vertCount += 3;
	}
}

//
// AM_DrawSprite
//

void AM_DrawSprite(mobj_t* thing, float scale) {
	spritedef_t* sprdef;
	spriteframe_t* sprframe;
	fixed_t x;
	fixed_t y;
	float flip = 0.0f;
	float width;
	float height;
	int rot = 0;
	unsigned int c;
	unsigned char alpha;
	float scalefactor;
	float fz;
	float tx;
	float ty;
	float dx1;
	float dx2;
	float dy1;
	float dy2;
	float dcos;
	float dsin;
	vtx_t vtx[4];

	if (thing->flags & (MF_NOSECTOR | MF_RENDERLASER)) {
		return;
	}
	else if (thing->state == (state_t*)S_NULL) {
		return;
	}

	if (!thing->sprite) {
		return;
	}
	else {
		//
		// setup sprite data
		//
		sprdef = &spriteinfo[thing->sprite];
		sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

		if (sprframe->rotate) {
			rot = ((am_viewangle - thing->angle) + ANG90 + (unsigned)(ANG45 / 2) * 9) >> 29;
		}

		//
		// keys and artifacts are scaled when zooming out
		//
		if (thing->type >= MT_ITEM_BLUECARDKEY && thing->type <= MT_ITEM_ARTIFACT3) {
			scalefactor = scale / 200.0f;

			if (scalefactor >= 5.0f) {
				scalefactor = 5.0f;
			}
		}
		else {
			scalefactor = 1.0f;
		}

		width = ((float)spritewidth[sprframe->lump[rot]] * scalefactor);
		height = ((float)spriteheight[sprframe->lump[rot]] * scalefactor);

		if (sprframe->flip[rot]) {
			flip = 1.0f;
		}
		else {
			flip = 0.0f;
		}
	}

	x = thing->x;
	y = thing->y;

	//
	// get vertex data and rotate the plane
	//
	tx = F2D3D(x);
	ty = F2D3D(y);
	fz = -(scale * 2);
	dx1 = -(width / 2.0f);
	dx2 = dx1 + width;
	dy1 = -(height / 2.0f);
	dy2 = dy1 + height;
	dcos = F2D3D(finecosine[(am_viewangle + ANG90) >> ANGLETOFINESHIFT]);
	dsin = F2D3D(finesine[(am_viewangle + ANG90) >> ANGLETOFINESHIFT]);
	glSetVertex(vtx);

	vtx[0].x = tx - ((dx2 * dsin) + (dy1 * dcos));
	vtx[0].y = ty + ((dx2 * dcos) - (dy1 * dsin));
	vtx[0].z = fz;
	vtx[0].tu = flip;
	vtx[0].tv = 0.0f;
	vtx[1].x = tx - ((dx2 * dsin) + (dy2 * dcos));
	vtx[1].y = ty + ((dx2 * dcos) - (dy2 * dsin));
	vtx[1].z = fz;
	vtx[1].tu = flip;
	vtx[1].tv = 1.0f;
	vtx[2].x = tx - ((dx1 * dsin) + (dy2 * dcos));
	vtx[2].y = ty + ((dx1 * dcos) - (dy2 * dsin));
	vtx[2].z = fz;
	vtx[2].tu = 1 - flip;
	vtx[2].tv = 1.0f;
	vtx[3].x = tx - ((dx1 * dsin) + (dy1 * dcos));
	vtx[3].y = ty + ((dx1 * dcos) - (dy1 * dsin));
	vtx[3].z = fz;
	vtx[3].tu = 1 - flip;
	vtx[3].tv = 0.0f;

	GL_BindSpriteTexture(sprframe->lump[rot], thing->info->palette);
	GL_SetState(GLSTATE_BLEND, 1);

	alpha = (thing->alpha * (am_overlay.value ? 96 : 0xff)) / 0xff;

	//
	// show as full white in non-textured mode or if the mobj is an item
	//
	if ((thing->frame & FF_FULLBRIGHT) || nolights || thing->flags & MF_SPECIAL || amModeCycle) {
		c = D_RGBA(255, 255, 255, alpha);
	}
	else {
		c = R_GetSectorLight(alpha, thing->subsector->sector->colors[LIGHT_THING]);
	}

	glSetVertexColor(vtx, c, 4);

	//
	// do the drawing
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTriangle(2, 1, 0);
	glTriangle(2, 0, 3);
	glDrawGeometry(4, vtx);

	GL_SetState(GLSTATE_BLEND, 0);
}