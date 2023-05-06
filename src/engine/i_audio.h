// Emacs style mode select   -*- C++ -*-
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

#ifndef __I_AUDIO_H__
#define __I_AUDIO_H__

#include <fmod.h>
#include <fmod_common.h>
#include "m_fixed.h"

// 20120107 bkw: Linux users can change the default FluidSynth backend here:
#ifndef _WIN32
#define DEFAULT_FLUID_DRIVER "sndio"

// 20120203 villsa: add default for windows
#else
#define DEFAULT_FLUID_DRIVER "dsound"

#endif

typedef struct {
    fixed_t x;
    fixed_t y;
    fixed_t z;
} sndsrc_t;

// FMOD Studio
struct Sound {
    FMOD_SYSTEM* fmod_studio_system;
};

struct Sound sound;
extern FMOD_SOUND* fmod_studio_sound[93];
extern FMOD_CHANNEL* fmod_studio_channel;
extern FMOD_RESULT   fmod_studio_result;
extern FMOD_CHANNELGROUP* master;
extern FMOD_VECTOR fmod_vec_position;
extern FMOD_VECTOR fmod_vec_velocity;
extern FMOD_VECTOR fmod_vec_forward;
extern FMOD_VECTOR fmod_vec_up;

int I_GetMaxChannels(void);
int I_GetVoiceCount(void);
sndsrc_t* I_GetSoundSource(int c);

int FMOD_StartSound(int sfx_id);
int FMOD_StopSound(void);
int FMOD_StartMusic(int mus_id);
int FMOD_StartSFXLoop(int sfx_id);
int FMOD_StopSFXLoop(void);

void I_InitSequencer(void);
void I_ShutdownSound(void);
void I_UpdateChannel(int c, int volume, int pan);
void I_RemoveSoundSource(int c);
void I_SetMusicVolume(float volume);
void I_SetSoundVolume(float volume);
void I_ResetSound(void);
void I_PauseSound(void);
void I_ResumeSound(void);
void I_SetGain(float db);
void I_StopSound(sndsrc_t* origin, int sfx_id);
void I_StartMusic(int mus_id);
void I_StartSound(int sfx_id, sndsrc_t* origin, int volume, int pan, int reverb);

#endif // __I_AUDIO_H__