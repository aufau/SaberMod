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

// bg_local.h -- local definitions for the bg (both games) files

#define	MIN_WALK_NORMAL	0.7f		// can't walk on very steep slopes

#define	STEPSIZE		18

#define	TIMER_LAND		130
#define	TIMER_GESTURE	(34*66+50)

#define	OVERCLIP		1.001f

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vec3_t		forward, right, up;
	float		frametime;

	int			msec;

	qboolean	walking;
	qboolean	groundPlane;
	trace_t		groundTrace;

	float		impactSpeed;

	vec3_t		previous_origin;
	vec3_t		previous_velocity;
	int			previous_waterlevel;

	int			seed;
} pml_t;

extern	pml_t		pml;

extern	int		c_pmove;

extern const int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];

// Had to add these here because there was no file access within the BG right now.
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );

//PM anim utility functions:
qboolean PM_SaberInParry( saberMoveName_t move );
qboolean PM_SaberInKnockaway( saberMoveName_t move );
qboolean PM_SaberInReflect( saberMoveName_t move );
qboolean PM_SaberInStart( saberMoveName_t move );
qboolean PM_InSaberAnim( animNumber_t anim );
qboolean PM_InKnockDown( playerState_t *ps );
qboolean PM_PainAnim( animNumber_t anim );
qboolean PM_JumpingAnim( animNumber_t anim );
qboolean PM_LandingAnim( animNumber_t anim );
qboolean PM_SpinningAnim( animNumber_t anim );
qboolean PM_InOnGroundAnim ( animNumber_t anim );
qboolean PM_InRollComplete( playerState_t *ps, animNumber_t anim );
int PM_AnimLength( animNumber_t anim );

animNumber_t PM_GetSaberStance(void);
float PM_GroundDistance(void);
qboolean PM_SomeoneInFront(trace_t *tr);
saberMoveName_t PM_SaberFlipOverAttackMove(trace_t *tr);
saberMoveName_t PM_SaberJumpAttackMove( void );

void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out, float overbounce );
void PM_AddTouchEnt( int entityNum );
void PM_AddEvent( entity_event_t newEvent );

qboolean	PM_SlideMove( qboolean gravity );
void		PM_StepSlideMove( qboolean gravity );

void BG_CycleInven(playerState_t *ps, int direction);

void PM_StartTorsoAnim( animNumber_t anim );
void PM_ContinueLegsAnim( animNumber_t anim );
void PM_ForceLegsAnim( animNumber_t anim );

void PM_BeginWeaponChange( int weapon );
void PM_FinishWeaponChange( void );

void PM_SetAnim(int setAnimParts, animNumber_t anim, unsigned setAnimFlags, int blendTime);

void PM_WeaponLightsaber(void);
void PM_SetSaberMove( saberMoveName_t newMove);

void PM_SetForceJumpZStart(float value);
