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

#include "../game/q_shared.h"
#include "tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
#include "../game/mvapi.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define NULL_HANDLE			((qhandle_t) 0)
#define NULL_SOUND			((sfxHandle_t) 0)
#define NULL_FX				((fxHandle_t) 0)

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000

#define SABER_LENGTH_MAX	40

// Zoom vars
#define	ZOOM_TIME			150		// not currently used?
#define MAX_ZOOM_FOV		3.0f
#define ZOOM_IN_TIME		1500.0f
#define ZOOM_OUT_TIME		100.0f
#define ZOOM_START_PERCENT	0.3f
#define MAX_FOV				160

#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#define NUM_FONT_BIG	1
#define NUM_FONT_SMALL	2
#define NUM_FONT_CHUNKY	3

#define	NUM_CROSSHAIRS		('j' - 'a' + 1)

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#define	DEFAULT_MODEL			"kyle"
#define	DEFAULT_TEAM_MODEL		"kyle"

#define DEFAULT_FORCEPOWERS		"5-1-000000000000000000"
//"rank-side-heal.lev.speed.push.pull.tele.grip.lightning.rage.protect.absorb.teamheal.teamforce.drain.see"

#define DEFAULT_REDTEAM_NAME		"Empire"
#define DEFAULT_BLUETEAM_NAME		"Rebellion"

#define DEFAULT_CENTERTIME		3

#define CAMERA_MIN_FPS		15

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_BORG,

	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	float		yawSwingDif;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	const animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact

	float		animationSpeed;		// scale the animation speed
	float		animationTorsoSpeed;

	qboolean	torsoYawing;
} lerpFrame_t;


typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;
} playerEntity_t;

//=================================================

typedef enum {
	TFP_REGEN,
	TFP_HEAL,
	TFP_DRAIN,
	TFP_ABSORB
} teamForcePower_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	vec3_t			damageAngles;
	int				damageTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;

	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;

	void			*ghoul2;
	int				weapon;

	void			*ghoul2weapon; //rww - pointer to ghoul2 instance of the current 3rd person weapon

	vec3_t			modelScale;
	float			radius;
	int				boltInfo;

	//sometimes used as a bolt index, but these values are also used as generic values for clientside entities
	//at times
	int				bolt1;
	int				bolt2;
	int				bolt3;
	int				bolt4;

	float			saberLength;
	int				saberExtendTime;

	int				rootBone;
	int				torsoBolt;

	vec3_t			turAngles;

	int				isATST;
	int				atstFootClang;
	int				atstSwinging;

	refEntity_t		frame_minus1;
	refEntity_t		frame_minus2;

	int				frame_minus1_refreshed;
	int				frame_minus2_refreshed;

	refEntity_t		frame_hold;
	int				frame_hold_time;
	int				frame_hold_refreshed;

	refEntity_t		grip_arm;

	int				trickAlpha;
	int				trickAlphaTime;

	int				teamPowerEffectTime;
	teamForcePower_t	teamPowerType;

	vec3_t			mins;
	vec3_t			maxs;
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FADE_SCALE_MODEL, // currently only for Demp2 shock sphere
	LE_FRAGMENT,
	LE_PUFF,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
	LE_OLINE,
	LE_SHOWREFENTITY,
	LE_LINE
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001,			// do not scale size over time
	LEF_TUMBLE			= 0x0002,			// tumble over time, used for ejecting shells
	LEF_FADE_RGB		= 0x0004,			// explicitly fade
	LEF_NO_RANDOM_ROTATE= 0x0008			// MakeExplosion adds random rotate which could be bad in some cases
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect
	int				bounceSound;		// optional sound index to play upon bounce

	float			alpha;
	float			dalpha;

	int				forceAlpha;

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	union {
		struct {
			float radius;
			float dradius;
			vec3_t startRGB;
			vec3_t dRGB;
		} sprite;
		struct {
			float width;
			float dwidth;
			float length;
			float dlength;
			vec3_t startRGB;
			vec3_t dRGB;
		} trail;
		struct {
			float width;
			float dwidth;
			// Below are bezier specific.
			vec3_t			control1;				// initial position of control points
			vec3_t			control2;
			vec3_t			control1_velocity;		// initial velocity of control points
			vec3_t			control2_velocity;
			vec3_t			control1_acceleration;	// constant acceleration of control points
			vec3_t			control2_acceleration;
		} line;
		struct {
			float width;
			float dwidth;
			float width2;
			float dwidth2;
			vec3_t startRGB;
			vec3_t dRGB;
		} line2;
		struct {
			float width;
			float dwidth;
			float width2;
			float dwidth2;
			float height;
			float dheight;
		} cylinder;
		struct {
			float width;
			float dwidth;
		} electricity;
		struct
		{
			// fight the power! open and close brackets in the same column!
			float radius;
			float dradius;
			qboolean (*thinkFn)(struct localEntity_s *le);
			vec3_t	dir;	// magnitude is 1, but this is oldpos - newpos right before the
							//particle is sent to the renderer
			// may want to add something like particle::localEntity_s *le (for the particle's think fn)
		} particle;
		struct
		{
			qboolean	dontDie;
			vec3_t		dir;
			float		variance;
			int			delay;
			int			nextthink;
			qboolean	(*thinkFn)(struct localEntity_s *le);
			int			data1;
			int			data2;
		} spawner;
		struct
		{
			float radius;
		} fragment;
	} data;

	refEntity_t		refEntity;
} localEntity_t;

//======================================================================


typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean		dead;
	int				team;
	int				kills;
	int				deaths;
	int				netDamage;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	32

typedef struct
{
	// Actual trail stuff
	int		inAction;	// controls whether should we even consider starting one
	int		duration;	// how long each trail seg stays in existence
	int		lastTime;	// time a saber segement was last stored
	vec3_t	base;
	vec3_t	tip;

	vec3_t	dualbase;
	vec3_t	dualtip;

	// Marks stuff
	qboolean	haveOldPos[2];
	vec3_t		oldPos[2];
	vec3_t		oldNormal[2];	// store this in case we don't have a connect-the-dots situation
							//	..then we'll need the normal to project a mark blob onto the impact point
} saberTrail_t;

typedef struct {
	qboolean		infoValid;

	char			name[MAX_QPATH];
	team_t			team;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	int				frame;

	vec3_t			color1;
	vec3_t			color2;

	saber_colors_t	icolor1;

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
	int				armor;
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
//	char			headModelName[MAX_QPATH];
//	char			headSkinName[MAX_QPATH];
	char			forcePowers[MAX_QPATH];
	char			redTeam[MAX_TEAMNAME];
	char			blueTeam[MAX_TEAMNAME];

	char			teamName[MAX_TEAMNAME];

	qboolean		deferred;

	qboolean		newAnims;		// true if using the new mission pack animations
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model

	qhandle_t		legsModel;
	qhandle_t		legsSkin;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;

	//qhandle_t		headModel;
	//qhandle_t		headSkin;

	qboolean		ATST;

	void			*ghoul2Model;

	qhandle_t		modelIcon;

	qhandle_t		bolt_rhand;
	qhandle_t		bolt_lhand;

	qhandle_t		bolt_head;

	qhandle_t		bolt_motion;

	qhandle_t		bolt_llumbar;

	saberTrail_t	saberTrail;
	int				saberHitWallSoundDebounceTime;

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];

	int				legsAnim;
	int				torsoAnim;

	qboolean		referee;
} clientInfo_t;


// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;		// this is the pickup model
	qhandle_t		viewModel;			// this is the in-view model used by the player
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose
	sfxHandle_t		firingSound;
	sfxHandle_t		chargeSound;
	fxHandle_t		muzzleEffect;
	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;
	sfxHandle_t		missileHitSound;

	sfxHandle_t		altFlashSound[4];
	sfxHandle_t		altFiringSound;
	sfxHandle_t		altChargeSound;
	fxHandle_t		altMuzzleEffect;
	qhandle_t		altMissileModel;
	sfxHandle_t		altMissileSound;
	void			(*altMissileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			altMissileDlight;
	vec3_t			altMissileDlightColor;
	int				altMissileRenderfx;
	sfxHandle_t		altMissileHitSound;

	sfxHandle_t		readySound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
/*
Ghoul2 Insert Start
*/
	void			*g2Models[MAX_ITEM_MODELS];
	float			radius[MAX_ITEM_MODELS];
/*
Ghoul2 Insert End
*/
} itemInfo_t;


typedef struct {
	int				itemNum;
} powerupInfo_t;


#define MAX_SKULLTRAIL		10

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;


#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

typedef enum {
	SPECMODE_FOLLOW,		// standard following mode
	SPECMODE_FREEANGLES,	// spectator controls absolute camera angles
	SPECMODE_MAX
} specMode_t;

typedef struct {
	qboolean	following;			// if we're following another player
	specMode_t	mode;
	int			delta_angles[3];	// delta angles for free angles mode
	int			thirdPersonRange;	// precalculated third person range
} specData_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16

typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;

	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
//	snapshot_t	activeSnapshots[2];

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)
	float		predictedTimeFrac;	// frameInterpolation * (next->commandTime - prev->commandTime)

	qboolean	mMapChange;

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime
	int			serverTime;		// cl.serverTime
	int			time;			// time since an initialization of CGame module
								// used for renderer, effects and ghoul2 submodules
	int			oldServerTime;	// server time at last frame

	int			gameTime;		// serverTime stopped during pauses
	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	int			seekTime;			// seek to given serverTime
	qboolean	fastSeek;			// don't draw any intermediate frames
	char		savedmaxfps[16];	// save com_maxfps value

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;
	int			predictionBaseTime;		// serverTime of snapshot predictedPlayerState is based on
	vec3_t		predictedPlayerOrigin;	// predicted origin unaffected by BG_AdjustPositionForMover

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	weapon_t	weaponSelect;

	forcePowers_t	forceSelect;		// FP_NONE or selectable force (see FP_Selectable macro)
	holdable_t		itemSelect;

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	float		constrictValue;
	float		constrict;
	int			doConstrict;

	qboolean	hasFallVector;
	vec3_t		fallVector;

	// following spectator mode data
	specData_t	spec;

	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	char		killerName[MAX_NETNAME];
#ifdef MISSIONPACK
	char			spectatorList[MAX_STRING_CHARS];		// list of names
	int				spectatorLen;												// length of list
	float			spectatorWidth;											// width in device units
	int				spectatorTime;											// next time to offset
	int				spectatorPaintX;										// current paint x
	int				spectatorPaintX2;										// current paint x
	int				spectatorOffset;										// current offset from start
	int				spectatorPaintLen; 									// current offset from start

	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];
#endif
	// centerprinting
	int			centerPrintTime;
	float		centerPrintMsec;
	int			centerPrintY;
	char		centerPrint[MAX_INFO_STRING];
	int			centerPrintLines;
	qboolean	centerPrintLock;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// kill timers for carnage reward
	int			lastKillTime;

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;
#ifdef JK2AWARDS
	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];
#endif
	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];
#ifdef MISSIONPACK
	// for voice chat buffer
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;
#endif
	// warmup countdown
	int			warmup;
	int			warmupCount;

	//==========================

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	int			damageTime;
	float		damageX, damageY, damageValue;

	// view movement
	int			v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int     	nextOrbitTime;

	//qboolean cameraMode;		// if rendering from a loaded camera
	int			loadLCARSStage;


	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	// HUD stuff
	int				HUDTickFlashTime;
	qboolean		HUDArmorFlag;
	qboolean		HUDHealthFlag;
	qboolean		iconHUDActive;
	float			iconHUDPercent;
	int				iconSelectTime;
	int				invenSelectTime;
	int				forceSelectTime;

	vec3_t			lastFPFlashPoint;

	// saber clash flare
	int				saberFlashTime;
	vec3_t			saberFlashPos;

	qboolean		queueMacroscan;
/*
Ghoul2 Insert Start
*/
	int				testModel;
	// had to be moved so we wouldn't wipe these out with the memset - these have STL in them and shouldn't be cleared that way
	snapshot_t	activeSnapshots[2];
/*
Ghoul2 Insert End
*/

	char				sharedBuffer[MAX_CG_SHARED_BUFFER_SIZE];
} cg_t;

#define MAX_TICS	14

typedef struct forceTicPos_s
{
	const int		x;
	const int		y;
	const int		width;
	const int		height;
	const char		*file;
	qhandle_t		tic;
} forceTicPos_t;
extern forceTicPos_t forceTicPos[];
extern forceTicPos_t ammoTicPos[];

typedef struct cgscreffects_s
{
	float		FOV;
	float		FOV2;

	float		shake_intensity;
	int			shake_duration;
	int			shake_start;

	float		music_volume_multiplier;
	int			music_volume_time;
	qboolean	music_volume_set;
} cgscreffects_t;

extern cgscreffects_t cgScreenEffects;

void CGCam_Shake( float intensity, int duration );
void CGCam_SetMusicMult( float multiplier, int duration );

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	whiteShader;

	qhandle_t	loadBarLED;
	qhandle_t	loadBarLEDCap;
	qhandle_t	loadBarLEDSurround;

	qhandle_t	bryarFrontFlash;
	qhandle_t	greenFrontFlash;
	qhandle_t	lightningFlash;
	qhandle_t	redLine;
	qhandle_t	whiteline2;
	qhandle_t	smokeTrail;

	qhandle_t	itemHoloModel;
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	flagShader[4];

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

	qhandle_t	lightningShader;

	qhandle_t	redSaberGlowShader;
	qhandle_t	redSaberCoreShader;
	qhandle_t	orangeSaberGlowShader;
	qhandle_t	orangeSaberCoreShader;
	qhandle_t	yellowSaberGlowShader;
	qhandle_t	yellowSaberCoreShader;
	qhandle_t	greenSaberGlowShader;
	qhandle_t	greenSaberCoreShader;
	qhandle_t	blueSaberGlowShader;
	qhandle_t	blueSaberCoreShader;
	qhandle_t	purpleSaberGlowShader;
	qhandle_t	purpleSaberCoreShader;
	qhandle_t	saberBlurShader;

	qhandle_t	yellowDroppedSaberShader;

	qhandle_t	rivetMarkShader;
	qhandle_t	saberFlareShader;
	qhandle_t	saberDamageGlowShader;
	qhandle_t	forcePushShader;

	qhandle_t	teamRedShader;
	qhandle_t	teamBlueShader;

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;

	qhandle_t	smokePuffShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;

	qhandle_t	numberShaders[11];
	qhandle_t	smallnumberShaders[11];
	qhandle_t	chunkyNumberShaders[11];

	qhandle_t	electricBodyShader;
	qhandle_t	electricBody2Shader;

	qhandle_t	shadowMarkShader;

	// Portable shield shaders
	qhandle_t	redDmgShieldShader;
	qhandle_t	redPortaShieldShader;
	qhandle_t	blueDmgShieldShader;
	qhandle_t	bluePortaShieldShader;

	//glass shard shader
	qhandle_t	glassShardShader;

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// Pain view shader
	qhandle_t	viewPainShader;
	qhandle_t	viewPainShader_Shields;
	qhandle_t	viewPainShader_ShieldsAndHealth;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	hastePuffShader;

	qhandle_t	itemRespawningPlaceholder;
	qhandle_t	itemRespawningRezOut;

	qhandle_t	playerShieldDamage;
	qhandle_t	forceSightBubble;
	qhandle_t	forceShell;
	qhandle_t	sightShell;

	qhandle_t	lightEnglightDisableShader;
	qhandle_t	darkEnglightDisableShader;

	// Disruptor zoom graphics
	qhandle_t	disruptorMask;
	qhandle_t	disruptorInsert;
	qhandle_t	disruptorLight;
	qhandle_t	disruptorInsertTick;
	qhandle_t	disruptorChargeShader;

	// Binocular graphics
	qhandle_t	binocularCircle;
	qhandle_t	binocularMask;
	qhandle_t	binocularArrow;
	qhandle_t	binocularTri;
	qhandle_t	binocularStatic;
	qhandle_t	binocularOverlay;

	// weapon effect models
	qhandle_t	lightningExplosionModel;

	// explosion assets
	qhandle_t	explosionModel;
	qhandle_t	surfaceExplosionShader;

	qhandle_t	solidWhite;

	qhandle_t	heartShader;

	// All the player shells
	qhandle_t	ysaliredShader;
	qhandle_t	ysaliblueShader;
	qhandle_t	ysalimariShader;
	qhandle_t	boonShader;
	qhandle_t	endarkenmentShader;
	qhandle_t	enlightenmentShader;
	qhandle_t	invulnerabilityShader;

#ifdef JK2AWARDS
	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;
#endif

	// sounds
	sfxHandle_t	selectSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];

	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;

	sfxHandle_t crackleSound;

	sfxHandle_t	grenadeBounce1;
	sfxHandle_t	grenadeBounce2;

	sfxHandle_t teamHealSound;
	sfxHandle_t teamRegenSound;

	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

#ifdef JK2AWARDS
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
#endif

	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;

	sfxHandle_t rollSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t deploySeeker;
	sfxHandle_t medkitSound;

	// teamplay sounds
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;
#ifdef JK2AWARDS
	sfxHandle_t	assistSound;
	sfxHandle_t captureSound;
	sfxHandle_t defendSound;
#endif
	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
	sfxHandle_t	redTookFlagSound;
	sfxHandle_t blueTookFlagSound;

	sfxHandle_t redYsalReturnedSound;
	sfxHandle_t blueYsalReturnedSound;
	sfxHandle_t	redTookYsalSound;
	sfxHandle_t blueTookYsalSound;

	sfxHandle_t	drainSound;

	//music blips
	sfxHandle_t	happyMusic;
	sfxHandle_t dramaticFailure;

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
#ifdef MISSIONPACK
	// new stuff
	qhandle_t patrolShader;
	qhandle_t assaultShader;
	qhandle_t campShader;
	qhandle_t followShader;
	qhandle_t defendShader;
	qhandle_t teamLeaderShader;
	qhandle_t retrieveShader;
	qhandle_t escortShader;

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;
#endif
	qhandle_t halfShieldModel;
	qhandle_t halfShieldShader;

	qhandle_t demp2Shell;
	qhandle_t demp2ShellShader;

	//weapon icons
	qhandle_t weaponIcons[WP_NUM_WEAPONS];
	qhandle_t weaponIcons_NA[WP_NUM_WEAPONS];

	//holdable inventory item icons
	qhandle_t invenIcons[HI_NUM_HOLDABLE];

	//force power icons
	qhandle_t forcePowerIcons[NUM_FORCE_POWERS];

	qhandle_t rageRecShader;

	//other HUD parts
	qhandle_t HUDLeftFrame;
	qhandle_t HUDArmor1;
	qhandle_t HUDArmor2;
	qhandle_t HUDHealth;
	qhandle_t HUDHealthTic;
	qhandle_t HUDArmorTic;
	qhandle_t HUDLeftStatic;
	qhandle_t HUDLeft;

	qhandle_t	HUDSaberStyle1;
	qhandle_t	HUDSaberStyle2;
	qhandle_t	HUDSaberStyle3;

	qhandle_t	HUDRightFrame;
	qhandle_t	HUDInnerRight;

	int			currentBackground;
	qhandle_t	weaponIconBackground;
	qhandle_t	weaponProngsOff;
	qhandle_t	weaponProngsOn;
	qhandle_t	forceIconBackground;
	qhandle_t	forceProngsOn;
	qhandle_t	inventoryIconBackground;
	qhandle_t	inventoryProngsOn;

	qhandle_t	HUDInnerLeft;

	sfxHandle_t	holocronPickup;

	qhandle_t	mpiRFlagXShader;
	qhandle_t	mpiBFlagXShader;
	qhandle_t	mpiRFlagYSShader;
	qhandle_t	mpiBFlagYSShader;
	qhandle_t	mpiRFlagShader;
	qhandle_t	mpiBFlagShader;

	// Zoom
	sfxHandle_t	zoomStart;
	sfxHandle_t	zoomLoop;
	sfxHandle_t	zoomEnd;
	sfxHandle_t	disruptorZoomLoop;
#ifndef MISSIONPACK
	// Fonts
	qhandle_t	qhSmallFont;
	qhandle_t	qhMediumFont;
	qhandle_t	qhBigFont;
#endif
	// Other SaberMod
	qhandle_t	download;
	qhandle_t	missing;
	qhandle_t	crosshairArrow;
} cgMedia_t;


// Stored FX handles
//--------------------
typedef struct
{
	// BRYAR PISTOL
	fxHandle_t	bryarShotEffect;
	fxHandle_t	bryarPowerupShotEffect;
	fxHandle_t	bryarWallImpactEffect;
	fxHandle_t	bryarWallImpactEffect2;
	fxHandle_t	bryarWallImpactEffect3;
	fxHandle_t	bryarFleshImpactEffect;
	fxHandle_t	bryarDroidImpactEffect;

	// BLASTER
	fxHandle_t  blasterShotEffect;
	fxHandle_t  blasterWallImpactEffect;
	fxHandle_t  blasterFleshImpactEffect;
	fxHandle_t  blasterDroidImpactEffect;

	// DISRUPTOR
	fxHandle_t  disruptorRingsEffect;
	fxHandle_t  disruptorProjectileEffect;
	fxHandle_t  disruptorWallImpactEffect;
	fxHandle_t  disruptorFleshImpactEffect;
	fxHandle_t  disruptorAltMissEffect;
	fxHandle_t  disruptorAltHitEffect;

	// BOWCASTER
	fxHandle_t	bowcasterShotEffect;
	fxHandle_t	bowcasterImpactEffect;

	// REPEATER
	fxHandle_t  repeaterProjectileEffect;
	fxHandle_t  repeaterAltProjectileEffect;
	fxHandle_t  repeaterWallImpactEffect;
	fxHandle_t  repeaterFleshImpactEffect;
	fxHandle_t  repeaterAltWallImpactEffect;

	// DEMP2
	fxHandle_t  demp2ProjectileEffect;
	fxHandle_t  demp2WallImpactEffect;
	fxHandle_t  demp2FleshImpactEffect;

	// FLECHETTE
	fxHandle_t	flechetteShotEffect;
	fxHandle_t	flechetteAltShotEffect;
	fxHandle_t	flechetteWallImpactEffect;
	fxHandle_t	flechetteFleshImpactEffect;

	// ROCKET
	fxHandle_t  rocketShotEffect;
	fxHandle_t  rocketExplosionEffect;

	// THERMAL
	fxHandle_t	thermalExplosionEffect;
	fxHandle_t	thermalShockwaveEffect;

	// TRIPMINE
	fxHandle_t	tripmineLaserFX;

	//FORCE
	fxHandle_t forceLightning;
	fxHandle_t forceLightningWide;

	fxHandle_t forceDrain;
	fxHandle_t forceDrainWide;
	fxHandle_t forceDrained;

	//TURRET
	fxHandle_t turretShotEffect;

} cgEffects_t;


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenWidth;		// virtual screen width (originally 640)
	float			screenXFactor;		// 640 / screenWidth (for calculations)
	float			screenXFactorInv;	// screenWidth / 640

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t		gametype;
	int				dmflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				roundlimit;
	int				maxclients;
	qboolean		needpass;
	qboolean		jediVmerc;
	int				wDisable;
	int				fDisable;
	qboolean		privateDuel;
	qboolean		instagib;
	qboolean		macroscan;

	char			mapname[MAX_QPATH];
	char			redTeam[MAX_QPATH];
	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];
	qboolean		voteMapMissing;

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

	int				scores1, scores2;		// from configstrings
	int				round;
	int				jediMaster;
	int				duelWinner;
	int				duelist1;
	int				duelist2;
	flagStatus_t	redflag, blueflag;		// flag status from configstrings
	flagStatus_t	flagStatus;

	int				readyClients;
	int				unpauseTime;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];
	fxHandle_t		gameEffects[MAX_FX];
/*
Ghoul2 Insert Start
*/
	qhandle_t		skins[MAX_CHARSKINS];

/*
Ghoul2 Insert End
*/
	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];
#ifdef MISSIONPACK
	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	menuDef_t *capturedItem;
	qhandle_t activeCursor;

	// orders
	int currentOrder;
	qboolean orderPending;
	int orderTime;
	int currentVoiceClient;
	int acceptOrderTime;
	int acceptTask;
	int acceptLeader;
	char acceptVoice[MAX_NAME_LENGTH];
#endif // MISSIONPACK
	// media
	cgMedia_t		media;

	// effects
	cgEffects_t		effects;

	// precomputed
	vec4_t			crosshairColor;
} cgs_t;

//==============================================================================

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
extern	itemInfo_t		cg_items[MAX_ITEMS];
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];
extern	int				cg_mvapi;

extern	vmCvar_t		cg_centertime;
extern	vmCvar_t		cg_runpitch;
extern	vmCvar_t		cg_runroll;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
//extern	vmCvar_t		cg_swingSpeed;
extern	vmCvar_t		cg_shadows;
extern	vmCvar_t		cg_drawTimer;
extern	vmCvar_t		cg_drawFPS;
extern	vmCvar_t		cg_drawSnapshot;
extern	vmCvar_t		cg_draw3dIcons;
extern	vmCvar_t		cg_drawIcons;
extern	vmCvar_t		cg_drawAmmoWarning;
extern	vmCvar_t		cg_drawCrosshair;
extern	vmCvar_t		cg_drawCrosshairNames;
extern	vmCvar_t		cg_drawScores;
extern	vmCvar_t		cg_dynamicCrosshair;
extern	vmCvar_t		cg_drawRewards;
extern	vmCvar_t		cg_drawTeamOverlay;
extern	vmCvar_t		cg_teamOverlayUserinfo;
extern	vmCvar_t		cg_crosshairX;
extern	vmCvar_t		cg_crosshairY;
extern	vmCvar_t		cg_crosshairSize;
extern	vmCvar_t		cg_crosshairHealth;
extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_draw2D;
extern	vmCvar_t		cg_animSpeed;
extern	vmCvar_t		cg_debugAnim;
extern	vmCvar_t		cg_debugPosition;
extern	vmCvar_t		cg_debugEvents;
extern	vmCvar_t		cg_errorDecay;
extern	vmCvar_t		cg_nopredict;
extern	vmCvar_t		cg_noPlayerAnims;
extern	vmCvar_t		cg_showmiss;
extern	vmCvar_t		cg_footsteps;
extern	vmCvar_t		cg_addMarks;
extern	vmCvar_t		cg_gun_frame;
extern	vmCvar_t		cg_gun_x;
extern	vmCvar_t		cg_gun_y;
extern	vmCvar_t		cg_gun_z;
extern	vmCvar_t		cg_drawGun;
extern	vmCvar_t		cg_viewsize;
extern	vmCvar_t		cg_tracerChance;
extern	vmCvar_t		cg_tracerWidth;
extern	vmCvar_t		cg_tracerLength;
extern	vmCvar_t		cg_autoswitch;
extern	vmCvar_t		cg_ignore;
extern	vmCvar_t		cg_simpleItems;
extern	vmCvar_t		cg_fov;

extern	vmCvar_t		cg_swingAngles;

extern	vmCvar_t		cg_oldPainSounds;

#ifdef G2_COLLISION_ENABLED
extern	vmCvar_t		cg_saberModelTraceEffect;
#endif

extern	vmCvar_t		cg_fpls;

extern	vmCvar_t		cg_saberDynamicMarks;
extern	vmCvar_t		cg_saberDynamicMarkTime;

extern	vmCvar_t		cg_saberContact;
extern	vmCvar_t		cg_saberTrail;

extern	vmCvar_t		cg_duelHeadAngles;

extern	vmCvar_t		cg_speedTrail;
extern	vmCvar_t		cg_auraShell;

extern	vmCvar_t		cg_animBlend;

extern	vmCvar_t		cg_dismember;

extern	vmCvar_t		cg_thirdPerson;
extern	vmCvar_t		cg_thirdPersonRange;
extern	vmCvar_t		cg_thirdPersonAngle;
extern	vmCvar_t		cg_thirdPersonPitchOffset;
extern	vmCvar_t		cg_thirdPersonVertOffset;
extern	vmCvar_t		cg_thirdPersonCameraDamp;
extern	vmCvar_t		cg_thirdPersonTargetDamp;

extern	vmCvar_t		cg_thirdPersonAlpha;
extern	vmCvar_t		cg_thirdPersonHorzOffset;

extern	vmCvar_t		cg_stereoSeparation;
extern	vmCvar_t		cg_lagometer;
extern	vmCvar_t		cg_drawEnemyInfo;
extern	vmCvar_t		cg_synchronousClients;
extern	vmCvar_t		cg_teamChatTime;
extern	vmCvar_t		cg_teamChatHeight;
extern	vmCvar_t		cg_stats;
extern	vmCvar_t 		cg_forceModel;
extern	vmCvar_t 		cg_buildScript;
extern	vmCvar_t		cg_paused;
extern	vmCvar_t		cg_blood;
extern	vmCvar_t		cg_predictItems;
extern	vmCvar_t		cg_deferPlayers;
extern	vmCvar_t		cg_drawFriend;
extern	vmCvar_t		cg_teamChatsOnly;
#ifdef MISSIONPACK
extern	vmCvar_t		cg_noVoiceChats;
extern	vmCvar_t		cg_noVoiceText;
#endif
extern  vmCvar_t		cg_damagePlums;
extern	vmCvar_t		cg_hudFiles;
extern	vmCvar_t		cg_smoothClients;
extern	vmCvar_t		cg_pmove_fixed;
extern	vmCvar_t		cg_pmove_msec;
extern	vmCvar_t		cg_cameraOrbit;
extern	vmCvar_t		cg_cameraOrbitDelay;
extern	vmCvar_t		cg_timescaleFadeEnd;
extern	vmCvar_t		cg_timescaleFadeSpeed;
extern	vmCvar_t		cg_timescale;
extern	vmCvar_t		cg_cameraMode;
extern  vmCvar_t		cg_smallFont;
extern  vmCvar_t		cg_bigFont;
extern	vmCvar_t		cg_noTaunt;
extern	vmCvar_t		cg_noProjectileTrail;
extern	vmCvar_t		cg_trueLightning;

extern	vmCvar_t		cg_redTeamName;
extern	vmCvar_t		cg_blueTeamName;
extern	vmCvar_t		cg_currentSelectedPlayer;
extern	vmCvar_t		cg_currentSelectedPlayerName;
extern	vmCvar_t		cg_enableDust;
extern	vmCvar_t		cg_enableBreath;
extern  vmCvar_t		cg_recordSPDemo;
extern  vmCvar_t		cg_recordSPDemoName;

extern	vmCvar_t		cg_chatBeep;
extern	vmCvar_t		cg_smoothCamera;
extern	vmCvar_t		cg_smoothCameraFPS;
extern	vmCvar_t		cg_crosshairColor;
extern	vmCvar_t		cg_darkenDeadBodies;
extern	vmCvar_t		cg_drawClock;
extern	vmCvar_t		cg_drawFollow;
extern	vmCvar_t		cg_drawSpectatorHints;
extern	vmCvar_t		cg_duelGlow;
extern	vmCvar_t		cg_fastSeek;
extern	vmCvar_t		cg_followKiller;
extern	vmCvar_t		cg_followPowerup;
extern	vmCvar_t		cg_privateDuel;
extern	vmCvar_t		cg_crosshairIndicators;
extern	vmCvar_t		cg_crosshairIndicatorsSpec;
extern	vmCvar_t		cg_widescreen;
extern	vmCvar_t		cg_fovAspectAdjust;

extern	vmCvar_t		cg_ui_myteam;
extern	vmCvar_t		cg_com_maxfps;
/*
Ghoul2 Insert Start
*/

extern	vmCvar_t		cg_debugBB;

/*
Ghoul2 Insert End
*/

extern	vmCvar_t		cg_param1;
extern	vmCvar_t		cg_param2;

//
// cg_main.c
//

typedef enum
{
	CT_NONE,
	CT_BLACK,
	CT_RED,
	CT_GREEN,
	CT_BLUE,
	CT_YELLOW,
	CT_MAGENTA,
	CT_CYAN,
	CT_WHITE,
	CT_LTGREY,
	CT_MDGREY,
	CT_DKGREY,
	CT_DKGREY2,

	CT_VLTORANGE,
	CT_LTORANGE,
	CT_DKORANGE,
	CT_VDKORANGE,

	CT_VLTBLUE1,
	CT_LTBLUE1,
	CT_DKBLUE1,
	CT_VDKBLUE1,

	CT_VLTBLUE2,
	CT_LTBLUE2,
	CT_DKBLUE2,
	CT_VDKBLUE2,

	CT_VLTBROWN1,
	CT_LTBROWN1,
	CT_DKBROWN1,
	CT_VDKBROWN1,

	CT_VLTGOLD1,
	CT_LTGOLD1,
	CT_DKGOLD1,
	CT_VDKGOLD1,

	CT_VLTPURPLE1,
	CT_LTPURPLE1,
	CT_DKPURPLE1,
	CT_VDKPURPLE1,

	CT_VLTPURPLE2,
	CT_LTPURPLE2,
	CT_DKPURPLE2,
	CT_VDKPURPLE2,

	CT_VLTPURPLE3,
	CT_LTPURPLE3,
	CT_DKPURPLE3,
	CT_VDKPURPLE3,

	CT_VLTRED1,
	CT_LTRED1,
	CT_DKRED1,
	CT_VDKRED1,
	CT_VDKRED,
	CT_DKRED,

	CT_VLTAQUA,
	CT_LTAQUA,
	CT_DKAQUA,
	CT_VDKAQUA,

	CT_LTPINK,
	CT_DKPINK,
	CT_LTCYAN,
	CT_DKCYAN,
	CT_LTBLUE3,
	CT_BLUE3,
	CT_DKBLUE3,

	CT_HUD_GREEN,
	CT_HUD_RED,
	CT_ICON_BLUE,
	CT_NO_AMMO_RED,
	CT_HUD_ORANGE,

	CT_MAX
} ct_table_t;

extern const vec4_t colorTable[CT_MAX];

const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
Q_NORETURN void QDECL CG_Error( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));

void CG_StartMusic( qboolean bForceStart );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type);
void CG_SetScoreSelection(void *menu);
void CG_BuildSpectatorString(void);
void CG_NextInventory_f(void);
void CG_PrevInventory_f(void);
void CG_NextForcePower_f(void);
void CG_PrevForcePower_f(void);
void CG_WideScreenMode(qboolean on);

//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
void CG_ZoomDown_f( void );
void CG_ZoomUp_f( void );
void CG_AddBufferedSound( sfxHandle_t sfx);

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
/*
Ghoul2 Insert Start
*/

void CG_TestG2Model_f (void);
void CG_TestModelSurfaceOnOff_f(void);
void CG_ListModelSurfaces_f (void);
void CG_ListModelBones_f (void);
void CG_TestModelSetAnglespre_f(void);
void CG_TestModelSetAnglespost_f(void);
void CG_TestModelAnimate_f(void);
/*
Ghoul2 Insert End
*/

//
// cg_drawtools.c
//
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_DrawPicExt( float x, float y, float width, float height, float s1, float t1, float s2, float t2, qhandle_t hShader );
void CG_DrawRotatePic( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawRotatePic2( float x, float y, float width, float height,float angle, qhandle_t hShader );
void CG_DrawString( float x, float y, const char *string,
				   float charWidth, float charHeight, const float *modulate );

void CG_DrawNumField (float x, float y, int width, int value,int charWidth,int charHeight,int style,qboolean zeroFill);

void CG_DrawStringExt( float x, float y, const char *string, const float *setColor,
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );
void CG_DrawBigString( float x, float y, const char *s, float alpha );
void CG_DrawBigStringColor( float x, float y, const char *s, vec4_t color );
void CG_DrawSmallString( float x, float y, const char *s, float alpha );
void CG_DrawSmallStringColor( float x, float y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str );

float	*CG_FadeColor( int startMsec, int totalMsec );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );

void UI_DrawProportionalString( float x, float y, const char* str, int style, const vec4_t color );
void UI_DrawScaledProportionalString( float x, float y, const char* str, int style, const vec4_t color, float scale);
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);

//
// cg_draw.c, cg_newDraw.c
//
extern	int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers;

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( const char *str, int y );
void CG_PrintMotd_f( void );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, team_t team, qboolean force2D );
void CG_DrawTeamBackground( float x, float y, float w, float h, float alpha, team_t team );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle,int font);
void CG_Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, font_t iMenuFont);
float CG_Text_Width(const char *text, float scale, font_t iMenuFont);
float CG_Text_Height(const char *text, float scale, font_t iMenuFont);
qboolean CG_YourTeamHasFlag(void);
qboolean CG_OtherTeamHasFlag(void);

// cg_newDraw.c
void CG_SelectPrevPlayer(void);
void CG_SelectNextPlayer(void);
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(const char **args);
qboolean CG_DeferMenuScript(const char **args);
void CG_ShowResponseHead(void);
void CG_SetPrintString(int type, const char *p);
void CG_InitTeamChat(void);
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText(void);
const char *CG_GetKillerText(void);
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, const vec3_t origin, const vec3_t angles );
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_CheckOrderPending(void);
qhandle_t CG_StatusHandle(int task);



//
// cg_player.c
//
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, team_t team );
void CG_NewClientInfo( int clientNum, qboolean entitiesInitialized );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );
void CG_PlayerShieldHit(int entitynum, vec3_t dir, int amount);


//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask );
void CG_DuelTrace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );
void CG_ReattachLimb(centity_t *source);


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_ManualEntityRender(centity_t *cent);
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const char *tagName );

/*
Ghoul2 Insert Start
*/
void ScaleModelAxis(refEntity_t	*ent);
/*
Ghoul2 Insert End
*/

//
// cg_turret.c
//
void TurretClientRun(centity_t *ent);

//
// cg_weapons.c
//
void CG_GetClientWeaponMuzzleBoltPoint(int clIndex, vec3_t to);

void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );

void CG_RegisterWeapon( weapon_t weaponNum);
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent, qboolean altFire );
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType, qboolean altFire, int charge);
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum, qboolean altFire);

void CG_AddViewWeapon (const playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, const playerState_t *ps, centity_t *cent, int team, vec3_t newAngles, qboolean thirdPerson );
void CG_DrawWeaponSelect( void );
void CG_DrawIconBackground(void);

void CG_OutOfAmmoChange( weapon_t oldWeapon );	// should this be in pmove?

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
void	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float red, float green, float blue, float alpha,
					qboolean alphaFade,
					float radius, qboolean temporary );

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff( const vec3_t p,
				   const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   int duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_GlassShatter(int entnum, vec3_t dmgPt, vec3_t dmgDir, float dmgRadius, int maxShards);
void CG_CreateDebris(int entnum, const vec3_t org, const vec3_t mins, const vec3_t maxs, int debrissound, int debrismodel);
void CG_ScorePlum( int client, const vec3_t org, int score );
#ifdef UNUSED
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing );
void CG_Bleed( vec3_t origin, int entityNum );

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, int numframes, qhandle_t shader, int msec,
								qboolean isSprite, float scale, int flags );// Overloaded in single player

void CG_SurfaceExplosion( vec3_t origin, vec3_t normal, float radius, float shake_speed, qboolean smoke );
#endif
void CG_TestLine( const vec3_t start, const vec3_t end, int time, unsigned int color, int radius);

void CG_InitGlass( void );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );
void CG_ShutDownConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
#ifdef MISSIONPACK
void CG_LoadVoiceChats( void );
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );
#endif
void CG_UpdateConfigString( int num, qboolean init );

//
// cg_playerstate.c
//
int CG_IsMindTricked(int trickIndex1, int trickIndex2, int trickIndex3, int trickIndex4, int client);
void CG_Respawn( const playerState_t *ps );
void CG_TransitionPlayerState( const playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );


//
// cg_saga.c
//
void CG_InitSagaMode(void);
void CG_SagaRoundOver(centity_t *ent, int won);
void CG_SagaObjectiveCompleted(centity_t *ent, int won, int objectivenum);



//===============================================

// MVAPI

qboolean	trap_MVAPI_ControlFixes(int fixes);
void		trap_MVAPI_SetVirtualScreen(float w, float h);

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print( const char *fmt );

// abort the game
Q_NORETURN void		trap_Error( const char *fmt );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds( void );

// console variable interaction
void		trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void		trap_Cvar_Update( vmCvar_t *vmCvar );
void		trap_Cvar_Set( const char *var_name, const char *value );
void		trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc( void );
void		trap_Argv( int n, char *buffer, int bufferLength );
void		trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int			trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void		trap_FS_Read( void *buffer, int len, fileHandle_t f );
void		trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void		trap_FS_FCloseFile( fileHandle_t f );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand( const char *cmdName );
void		trap_RemoveCommand( const char *cmdName );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points,
			const vec3_t projection,
			int maxPoints, vec3_t *pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_MuteSound( int entityNum, int entchannel );
void		trap_S_StartSound( vec3_t origin, int entityNum, soundChannel_t entchannel, sfxHandle_t sfx );
void		trap_S_StopLoopingSound(int entityNum);

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, soundChannel_t channelNum );
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// repatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t	trap_S_RegisterSound( const char *sample);		// returns buzz if not found
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop, qboolean bReturnWithoutStarting);	// empty name stops music
void	trap_S_StopBackgroundTrack( void );


void		trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterFont( const char *fontName );
int			trap_R_Font_StrLenPixels(const char *text, const qhandle_t iFontIndex, const float scale);
int			trap_R_Font_StrLenChars(const char *text);
int			trap_R_Font_HeightPixels(const qhandle_t iFontIndex, const float scale);
void		trap_R_Font_DrawString(int ox, int oy, const char *text, const float *rgba, const int setIndex, int iCharLimit, const float scale);
qboolean	trap_Language_IsAsian(void);
qboolean	trap_Language_UsesSpaces(void);
unsigned	trap_AnyLanguage_ReadCharFromString( const char *psText, int *piAdvanceCount, qboolean *pbIsTrailingPunctuation/* = NULL*/ );


// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame,
					   float frac, const char *tagName );
// Does weird, barely controllable rotation behaviour
void	trap_R_DrawRotatePic( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
// rotates image around exact center point of passed in coords
void	trap_R_DrawRotatePic2( float x, float y, float w, float h,
			float s1, float t1, float s2, float t2,float a, qhandle_t hShader );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

void		trap_R_GetLightStyle(int style, color4ub_t color);
void		trap_R_SetLightStyle(int style, int color);

void		trap_R_GetBModelVerts(int bmodelIndex, vec3_t *verts, vec3_t normal );

void	trap_FX_AddLine( const vec3_t start, const vec3_t end, float size1, float size2, float sizeParm,
									float alpha1, float alpha2, float alphaParm,
									const vec3_t sRGB, const vec3_t eRGB, float rgbParm,
									int killTime, qhandle_t shader, int flags);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale, int fpSel, int invenSel );

void		trap_SetClientForceAngle(int time, vec3_t angle);
void		trap_SetClientTurnExtent(float turnAdd, float turnSub, int turnTime);

void trap_OpenUIMenu(int menuID);

// aids for VM testing
void		testPrintInt( char *string, int i );
void		testPrintFloat( char *string, float f );

int			trap_MemoryRemaining( void );
qboolean	trap_Key_IsDown( int keynum );
int			trap_Key_GetCatcher( void );
void		trap_Key_SetCatcher( int catcher );
int			trap_Key_GetKey( const char *binding );

void		BG_CycleInven(playerState_t *ps, int direction);
int			BG_ProperForceIndex(forcePowers_t power);
void		BG_CycleForce(playerState_t *ps, int direction);


typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t; // bk001201 - warning: useless keyword or type name in empty declaration


int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status trap_CIN_StopCinematic(int handle);
e_status trap_CIN_RunCinematic (int handle);
void trap_CIN_DrawCinematic (int handle);
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

int trap_RealTime(qtime_t *qtime);
void trap_SnapVector( float *v );

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken( char *buffer, int bufferSize );
qboolean	trap_R_inPVS( const vec3_t p1, const vec3_t p2 );

int	trap_FX_RegisterEffect(const char *file);
void trap_FX_PlaySimpleEffect( const char *file, vec3_t org );					// uses a default up axis
void trap_FX_PlayEffect( const char *file, vec3_t org, vec3_t fwd );		// builds arbitrary perp. right vector, does a cross product to define up
void trap_FX_PlayEntityEffect( const char *file, vec3_t org,
						vec3_t axis[3], const int boltInfo, const int entNum );
void trap_FX_PlaySimpleEffectID( int id, vec3_t org );					// uses a default up axis
void trap_FX_PlayEffectID( int id, vec3_t org, vec3_t fwd );		// builds arbitrary perp. right vector, does a cross product to define up
void trap_FX_PlayEntityEffectID( int id, vec3_t org,
						vec3_t axis[3], const int boltInfo, const int entNum );
void trap_FX_PlayBoltedEffectID( int id, sharedBoltInterface_t *fxObj );
void trap_FX_AddScheduledEffects( void );
int	trap_FX_InitSystem( void );	// called in CG_Init to purge the fx system.
qboolean trap_FX_FreeSystem( void );	// ditches all active effects;
void trap_FX_AdjustTime( int time, vec3_t vieworg, vec3_t viewaxis[3] );

void trap_FX_AddPoly( addpolyArgStruct_t *p );
void trap_FX_AddBezier( addbezierArgStruct_t *p );
void trap_FX_AddPrimitive( effectTrailArgStruct_t *p );
void trap_FX_AddSprite( addspriteArgStruct_t *p );

void trap_SP_Print(const unsigned ID, byte *Data);
int trap_SP_GetStringTextString(const char *text, char *buffer, int bufferLength);
qboolean trap_SP_Register(const char *file );

void		trap_CG_RegisterSharedMemory(char *memory);

qboolean	trap_ROFF_Clean( void );
void		trap_ROFF_UpdateEntities( void );
	int			trap_ROFF_Cache( char *file );
qboolean	trap_ROFF_Play( int entID, int roffID, qboolean doTranslation );
qboolean	trap_ROFF_Purge_Ent( int entID );

const char *CG_GetStripEdString(const char *refSection, const char *refName);

// cg_particles.c
#ifdef UNUSED
void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
extern qboolean		initparticles;
int CG_NewParticleArea ( int num );
#endif // UNUSED


/*
Ghoul2 Insert Start
*/
// CG specific API access
void		trap_G2_ListModelSurfaces(void *ghlInfo);
void		trap_G2_ListModelBones(void *ghlInfo, int frame);
void		trap_G2_SetGhoul2ModelIndexes(void *ghoul2, qhandle_t *modelList, qhandle_t *skinList);
qboolean	trap_G2_HaveWeGhoul2Models(void *ghoul2);
qboolean	trap_G2API_GetBoltMatrix(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale);
qboolean	trap_G2API_GetBoltMatrix_NoReconstruct(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale);
qboolean	trap_G2API_GetBoltMatrix_NoRecNoRot(void *ghoul2, const int modelIndex, const int boltIndex, mdxaBone_t *matrix,
								const vec3_t angles, const vec3_t position, const int frameNum, qhandle_t *modelList, vec3_t scale);
int			trap_G2API_InitGhoul2Model(void **ghoul2Ptr, const char *fileName, int modelIndex, qhandle_t customSkin,
						  qhandle_t customShader, int modelFlags, int lodBias);
void		trap_G2API_CollisionDetect( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles,
								const vec3_t position, int frameNumber, int entNum, const vec3_t rayStart, const vec3_t rayEnd,
								const vec3_t scale, int traceFlags, int useLod, float fRadius );

void		trap_G2API_GiveMeVectorFromMatrix(mdxaBone_t *boltMatrix, int flags, vec3_t vec);
int			trap_G2API_CopyGhoul2Instance(void *g2From, void *g2To, int modelIndex);
void		trap_G2API_CopySpecificGhoul2Model(void *g2From, int modelFrom, void *g2To, int modelTo);
void		trap_G2API_DuplicateGhoul2Instance(void *g2From, void **g2To);
qboolean	trap_G2API_HasGhoul2ModelOnIndex(void *ghlInfo, int modelIndex);
qboolean	trap_G2API_RemoveGhoul2Model(void *ghlInfo, int modelIndex);

int			trap_G2API_AddBolt(void *ghoul2, int modelIndex, const char *boneName);
//qboolean	trap_G2API_RemoveBolt(void *ghoul2, int index);
void		trap_G2API_SetBoltInfo(void *ghoul2, int modelIndex, int boltInfo);
void		trap_G2API_CleanGhoul2Models(void **ghoul2Ptr);
qboolean	trap_G2API_SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags,
								const int up, const int right, const int forward, qhandle_t *modelList,
								int blendTime , int currentTime );
void		trap_G2API_GetGLAName(void *ghoul2, int modelIndex, char *fillBuf);
qboolean	trap_G2API_SetBoneAnim(void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame,
							  const int flags, const float animSpeed, const int currentTime, const float setFrame , const int blendTime );

qboolean	trap_G2API_SetRootSurface(void *ghoul2, const int modelIndex, const char *surfaceName);
qboolean	trap_G2API_SetSurfaceOnOff(void *ghoul2, const char *surfaceName, const int flags);
qboolean	trap_G2API_SetNewOrigin(void *ghoul2, const int boltIndex);


void CG_SetGhoul2Info( refEntity_t *ent, centity_t *cent);
void CG_CreateBBRefEnts(entityState_t *s1, vec3_t origin );

void CG_InitG2Weapons(void);
void CG_ShutDownG2Weapons(void);
void CG_CopyG2WeaponInstance(int weaponNum, void *toGhoul2);
void CG_CheckPlayerG2Weapons(const playerState_t *ps, centity_t *cent);

extern void *g2WeaponInstances[MAX_WEAPONS];
/*
Ghoul2 Insert End
*/
