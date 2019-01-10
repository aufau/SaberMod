/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2019 Witold Pilat <witold.pilat@gmail.com>

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

#ifndef __BG_NET_H
#define __BG_NET_H

// bg_net.h -- entityState_t and playerState_t with correct types

#include "bg_weapons.h"

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_FLOAT,		// float with no gravity in general direction of velocity (intended for gripping)
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION,	// no movement or status bar
	PM_HARMLESS		// can't use weapons, items, force powers
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_RAISING,
	WEAPON_DROPPING,
	WEAPON_FIRING,
	WEAPON_CHARGING,
	WEAPON_CHARGING_ALT,
	WEAPON_IDLE, //lowered		// NOTENOTE Added with saber
} weaponstate_t;

typedef enum {
	HANDEXTEND_NONE = 0,
	HANDEXTEND_FORCEPUSH,
	HANDEXTEND_FORCEPULL,
	HANDEXTEND_FORCEGRIP,
	HANDEXTEND_SABERPULL,
	HANDEXTEND_CHOKE, //use handextend priorities to choke someone being gripped
	HANDEXTEND_WEAPONREADY,
	HANDEXTEND_DODGE,
	HANDEXTEND_KNOCKDOWN,
	HANDEXTEND_DUELCHALLENGE,
	HANDEXTEND_TAUNT
} forceHandAnims_t;

// Okay, here lies the much-dreaded Pat-created FSM movement chart...  Heretic II strikes again!
// Why am I inflicting this on you?  Well, it's better than hardcoded states.
// Ideally this will be replaced with an external file or more sophisticated move-picker
// once the game gets out of prototype stage.

// rww - Moved all this to bg_public so that we can access the saberMoveData stuff on the cgame
// which is currently used for determining if a saber trail should be rendered in a given frame

typedef enum {
	// Totally invalid
	LS_INVALID	= -1,
	// Invalid, or saber not armed
	LS_NONE		= 0,

	// General movements with saber
	LS_READY,
	LS_DRAW,
	LS_PUTAWAY,

	// Attacks
	LS_A_TL2BR,//4
	LS_A_L2R,
	LS_A_BL2TR,
	LS_A_BR2TL,
	LS_A_R2L,
	LS_A_TR2BL,
	LS_A_T2B,
	LS_A_BACKSTAB,
	LS_A_BACK,
	LS_A_BACK_CR,
	LS_A_LUNGE,
	LS_A_JUMP_T__B_,
	LS_A_FLIP_STAB,
	LS_A_FLIP_SLASH,

	//starts
	LS_S_TL2BR,//26
	LS_S_L2R,
	LS_S_BL2TR,//# Start of attack chaining to SLASH LR2UL
	LS_S_BR2TL,//# Start of attack chaining to SLASH LR2UL
	LS_S_R2L,
	LS_S_TR2BL,
	LS_S_T2B,

	//returns
	LS_R_TL2BR,//33
	LS_R_L2R,
	LS_R_BL2TR,
	LS_R_BR2TL,
	LS_R_R2L,
	LS_R_TR2BL,
	LS_R_T2B,

	//transitions
	LS_T1_BR__R,//40
	LS_T1_BR_TR,
	LS_T1_BR_T_,
	LS_T1_BR_TL,
	LS_T1_BR__L,
	LS_T1_BR_BL,
	LS_T1__R_BR,//46
	LS_T1__R_TR,
	LS_T1__R_T_,
	LS_T1__R_TL,
	LS_T1__R__L,
	LS_T1__R_BL,
	LS_T1_TR_BR,//52
	LS_T1_TR__R,
	LS_T1_TR_T_,
	LS_T1_TR_TL,
	LS_T1_TR__L,
	LS_T1_TR_BL,
	LS_T1_T__BR,//58
	LS_T1_T___R,
	LS_T1_T__TR,
	LS_T1_T__TL,
	LS_T1_T___L,
	LS_T1_T__BL,
	LS_T1_TL_BR,//64
	LS_T1_TL__R,
	LS_T1_TL_TR,
	LS_T1_TL_T_,
	LS_T1_TL__L,
	LS_T1_TL_BL,
	LS_T1__L_BR,//70
	LS_T1__L__R,
	LS_T1__L_TR,
	LS_T1__L_T_,
	LS_T1__L_TL,
	LS_T1__L_BL,
	LS_T1_BL_BR,//76
	LS_T1_BL__R,
	LS_T1_BL_TR,
	LS_T1_BL_T_,
	LS_T1_BL_TL,
	LS_T1_BL__L,

	//Bounces
	LS_B1_BR,
	LS_B1__R,
	LS_B1_TR,
	LS_B1_T_,
	LS_B1_TL,
	LS_B1__L,
	LS_B1_BL,

	//Deflected attacks
	LS_D1_BR,
	LS_D1__R,
	LS_D1_TR,
	LS_D1_T_,
	LS_D1_TL,
	LS_D1__L,
	LS_D1_BL,
	LS_D1_B_,

	//Reflected attacks
	LS_V1_BR,
	LS_V1__R,
	LS_V1_TR,
	LS_V1_T_,
	LS_V1_TL,
	LS_V1__L,
	LS_V1_BL,
	LS_V1_B_,

	// Broken parries
	LS_H1_T_,//
	LS_H1_TR,
	LS_H1_TL,
	LS_H1_BR,
	LS_H1_B_,
	LS_H1_BL,

	// Knockaways
	LS_K1_T_,//
	LS_K1_TR,
	LS_K1_TL,
	LS_K1_BR,
	LS_K1_BL,

	// Parries
	LS_PARRY_UP,//
	LS_PARRY_UR,
	LS_PARRY_UL,
	LS_PARRY_LR,
	LS_PARRY_LL,

	// Projectile Reflections
	LS_REFLECT_UP,//
	LS_REFLECT_UR,
	LS_REFLECT_UL,
	LS_REFLECT_LR,
	LS_REFLECT_LL,

	LS_MOVE_MAX//
} saberMoveName_t;

typedef enum {
	ZOOM_NONE,
	ZOOM_DISRUPTOR,
	ZOOM_BINOCULARS,
} zoomMode_t;

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s {
	int				commandTime;	// cmd->serverTime of last executed command
	pmtype_t		pm_type;
	int				bobCycle;		// for view bobbing and footstep generation
	int				pm_flags;		// ducked, jump_held, etc
	int				pm_time;

	vec3_t			origin;
	vec3_t			velocity;
	int				weaponTime;
	int				weaponChargeTime;
	int				weaponChargeSubtractTime;
	int				gravity;
	int				speed;
	int				basespeed; //used in prediction to know base server g_speed value when modifying speed between updates
	int				delta_angles[3];	// add to command angles to get view direction
										// changed by spawns, rotating objects, and teleporters

	int				useTime;

	int				groundEntityNum;// ENTITYNUM_NONE = in air

	int				legsTimer;		// don't change low priority animations until this runs out
	int				legsAnim;		// mask off ANIM_TOGGLEBIT

	int				torsoTimer;		// don't change low priority animations until this runs out
	int				torsoAnim;		// mask off ANIM_TOGGLEBIT

	int				movementDir;	// a number 0 to 7 that represents the reletive angle
									// of movement to the view angle (axial and diagonals)
									// when at rest, the value will remain unchanged
									// used to twist the legs during strafing

	int				eFlags;			// copied to entityState_t->eFlags

	int				eventSequence;	// pmove generated events
	int				events[MAX_PS_EVENTS];
	int				eventParms[MAX_PS_EVENTS];

	int				externalEvent;	// events set on player from another source
	int				externalEventParm;
	int				externalEventTime;

	int				clientNum;		// ranges from 0 to MAX_CLIENTS-1
	weapon_t		weapon;			// copied to entityState_t->weapon
	weaponstate_t	weaponstate;

	vec3_t			viewangles;		// for fixed views
	int				viewheight;

	// damage feedback
	int				damageEvent;	// when it changes, latch the other parms
	int				damageYaw;
	int				damagePitch;
	int				damageCount;
	int				damageType;

	int				painTime;		// used for both game and client side to process the pain twitch - NOT sent across the network
	int				painDirection;	// NOT sent across the network
	float			yawAngle;		// NOT sent across the network
	qboolean		yawing;			// NOT sent across the network
	float			pitchAngle;		// NOT sent across the network
	qboolean		pitching;		// NOT sent across the network

	int				stats[MAX_STATS];
	int				persistant[MAX_PERSISTANT];	// stats that aren't cleared on death
	int				powerups[MAX_POWERUPS];	// level.time that the powerup runs out
	int				ammo[MAX_WEAPONS];

	int				generic1;
	int				loopSound;
	int				jumppad_ent;	// jumppad entity hit this frame

	// not communicated over the net at all
	int				ping;			// server to game info for scoreboard
	int				pmove_framecount;	// FIXME: don't transmit over the network
	int				jumppad_frame;
	int				entityEventSequence;

	int				lastOnGround;	//last time you were on the ground

	qboolean		saberInFlight;
	qboolean		saberActive;

	saberMoveName_t		saberMove;
	saberBlockType_t	saberBlocking;
	saberBlockedType_t	saberBlocked;

	int				saberLockTime;
	int				saberLockEnemy;
	int				saberLockFrame; //since we don't actually have the ability to get the current anim frame
	int				saberLockHits; //every x number of buttons hits, allow one push forward in a saber lock (server only)
	qboolean		saberLockAdvance; //do an advance (sent across net as 1 bit)

	int				saberEntityNum;
	float			saberEntityDist;
	int				saberEntityState;
	int				saberThrowDelay;
	qboolean		saberCanThrow;
	int				saberDidThrowTime;
	int				saberDamageDebounceTime;		// unused
	int				saberHitWallSoundDebounceTime;
	int				saberEventFlags;

	int				rocketLockIndex;
	int				rocketLastValidTime;
	int				rocketLockTime;
	int				rocketTargetTime;

	int				emplacedIndex;
	int				emplacedTime;

	qboolean		isJediMaster;
	qboolean		forceRestricted;
	qboolean		trueJedi;
	qboolean		trueNonJedi;
	int				saberIndex;

	int				genericEnemyIndex;
	int				droneFireTime;
	int				droneExistTime;

	int				activeForcePass;

	qboolean		hasDetPackPlanted; //better than taking up an eFlag isn't it?

	int				holocronsCarried[NUM_FORCE_POWERS];
	int				holocronCantTouch;
	int				holocronCantTouchTime; //for keeping track of the last holocron that just popped out of me (if any)
	int				holocronBits;

	animNumber_t	legsAnimExecute;
	animNumber_t	torsoAnimExecute;
	int				fullAnimExecute;		// unused

	int				electrifyTime;

	int				saberAttackSequence;
	int				saberIdleWound;
	int				saberAttackWound;
	int				saberBlockTime;

	int				otherKiller;
	int				otherKillerTime;
	int				otherKillerDebounceTime;

	forcedata_t		fd;
	qboolean		forceJumpFlip;
	forceHandAnims_t	forceHandExtend;
	int				forceHandExtendTime;

	int				forceRageDrainTime;

	int				forceDodgeAnim;
	qboolean		quickerGetup;

	int				groundTime;		// time when first left ground; unused

	int				footstepTime;

	int				otherSoundTime;
	float			otherSoundLen;

	int				forceGripMoveInterval;
	pmtype_t		forceGripChangeMovetype;

	int				forceKickFlip;

	int				duelIndex;
	int				duelTime;
	qboolean		duelInProgress;

	int				saberAttackChainCount;

	qboolean		saberHolstered;

	qboolean		usingATST;
	qboolean		atstAltFire;
	int				holdMoveTime;

	int				forceAllowDeactivateTime;

	// zoom key
	zoomMode_t		zoomMode;
	int				zoomTime;
	qboolean		zoomLocked;
	int				zoomFov;		// unused, was float
	int				zoomLockTime;

	int				fallingToDeath;

	int				useDelay;

	qboolean		inAirAnim;

	qboolean		dualBlade;

	vec3_t			lastHitLoc;
} playerState_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s {
	int				number;			// entity index
	int				eType;			// entityType_t
	int				eFlags;

	trajectory_t	pos;	// for calculating position
	trajectory_t	apos;	// for calculating angles

	int				time;
	int				time2;

	vec3_t			origin;
	vec3_t			origin2;

	vec3_t			angles;
	vec3_t			angles2;

	//rww - these were originally because we shared g2 info client and server side. Now they
	//just get used as generic values everywhere.
	int				bolt1;
	int				bolt2;

	//rww - this is necessary for determining player visibility during a jedi mindtrick
	int				trickedentindex; //0-15
	int				trickedentindex2; //16-32
	int				trickedentindex3; //33-48
	int				trickedentindex4; //49-64

	float			speed;

	int				fireflag;

	int				genericenemyindex;

	int				activeForcePass;

	int				emplacedOwner;

	int				otherEntityNum;	// shotgun sources, etc
	int				otherEntityNum2;

	int				groundEntityNum;	// -1 = in air

	int				constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int				loopSound;		// constantly loop this sound

	int				modelGhoul2;
	int				g2radius;
	int				modelindex;
	int				modelindex2;
	int				clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int				frame;

	qboolean		saberInFlight;
	int				saberEntityNum;
	saberMoveName_t	saberMove;
	int				forcePowersActive;

	qboolean		isJediMaster;

	int				solid;			// for client side prediction, trap_linkentity sets this properly

	int				event;			// impulse events -- muzzle flashes, footsteps, etc
	int				eventParm;

	// so crosshair knows what it's looking at
	int				owner;
	int				teamowner;
	qboolean		shouldtarget;

	// for players
	int				powerups;		// bit flags
	int				weapon;			// determines weapon and flash model, etc
	int				legsAnim;		// mask off ANIM_TOGGLEBIT
	int				torsoAnim;		// mask off ANIM_TOGGLEBIT

	int				forceFrame;		//if non-zero, force the anim frame

	int				generic1;
} entityState_t;

#endif // __BG_NET_H
