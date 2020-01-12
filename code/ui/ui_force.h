/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2020 Witold Pilat <witold.pilat@gmail.com>

This program is free software; you can redistribute it and/or modify it
under the terms and conditions of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
================================================================================
*/

#include "../qcommon/qfiles.h"

#define NUM_FORCE_STAR_IMAGES  9
#define FORCE_NONJEDI	0
#define FORCE_JEDI		1

extern int uiForceSide;
extern int uiJediNonJedi;
extern int uiForceRank;
extern int uiMaxRank;
extern int uiForceUsed;
extern int uiForceAvailable;
extern qboolean gTouchedForce;
extern qboolean uiForcePowersDisabled[NUM_FORCE_POWERS];
extern int uiForcePowersRank[NUM_FORCE_POWERS];
extern int uiForcePowerDarkLight[NUM_FORCE_POWERS];
extern int uiSaberColorShaders[NUM_SABER_COLORS];
// Dots above or equal to a given rank carry a certain color.
extern vmCvar_t	ui_freeSaber, ui_forcePowerDisable;

void UI_InitForceShaders(void);
void UI_ReadLegalForce(void);
void UI_DrawTotalForceStars(rectDef_t *rect, float scale, vec4_t color, int textStyle);
void UI_DrawForceStars(rectDef_t *rect, float scale, vec4_t color, int textStyle, int forceindex, int val, int min, int max) ;
void UI_UpdateClientForcePowers(const char *teamArg);
void UI_SaveForceTemplate();
void UI_UpdateForcePowers();
qboolean UI_SkinColor_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
qboolean UI_ForceSide_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
qboolean UI_JediNonJedi_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
qboolean UI_ForceMaxRank_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
qboolean UI_ForcePowerRank_HandleKey(int flags, float *special, int key, int num, int min, int max, int type);
extern void UI_ForceConfigHandle( int oldindex, int newindex );
