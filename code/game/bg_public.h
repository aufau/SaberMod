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

// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#ifndef __BG_PUBLIC_H__
#define __BG_PUBLIC_H__

#include "bg_weapons.h"
#include "anims.h"
#include "bg_net.h"

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION			"SaberMod"
#define GAMEVERSION_C		S_COLOR_BRAND "S" S_COLOR_WHITE "aber" S_COLOR_BRAND "M" S_COLOR_WHITE "od"
#define AUTHOR				"fau"

#define	DEFAULT_GRAVITY		800
#define	GIB_HEALTH			(-40)
#define ARMOR_PROTECTION		0.5f // Shields only stop 50% of armor-piercing dmg
#define ARMOR_REDUCTION_FACTOR	0.5f // Certain damage doesn't take off armor as efficiently

#define	JUMP_VELOCITY		225//270

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	SCORE_NOT_PRESENT	(-9999)	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define DEFAULT_MINS_2		(-24)
#define DEFAULT_MAXS_2		40
#define CROUCH_MAXS_2		16
#define	STANDARD_VIEWHEIGHT_OFFSET (-4)

#define	MINS_Z				(-24)
#define	DEFAULT_VIEWHEIGHT	(DEFAULT_MAXS_2+STANDARD_VIEWHEIGHT_OFFSET)//26
#define CROUCH_VIEWHEIGHT	(CROUCH_MAXS_2+STANDARD_VIEWHEIGHT_OFFSET)//12
#define	DEAD_VIEWHEIGHT		(-16)

#define MAX_CLIENT_SCORE_SEND 20

#define SHIELD_HALFTHICKNESS	4

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

// Each teamvote string takes 2 indices
#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25
#define CS_ROUND				26		// could use SendScoreboardMessageToAllClients instead
#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present

#define CS_CLIENT_JEDIMASTER	28		// current jedi master
#define CS_CLIENT_DUELWINNER	29		// current duel round winner - needed for printing at top of scoreboard
#define CS_CLIENT_DUELISTS		30		// client numbers for both current duelists. Needed for a number of client-side things.

#define CS_MODES				31		// list of votable modes for UI

// these are also in be_aas_def.h - argh (rjr)
#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)
/*
Ghoul2 Insert Start
*/
#define CS_CHARSKINS 			(CS_PLAYERS+MAX_CLIENTS)
/*
Ghoul2 Insert End
*/
#define CS_LOCATIONS			(CS_CHARSKINS+MAX_CHARSKINS)
#define CS_PARTICLES			(CS_LOCATIONS+MAX_LOCATIONS) // unused
#define CS_EFFECTS				(CS_PARTICLES+MAX_LOCATIONS)
#define	CS_LIGHT_STYLES			(CS_EFFECTS + MAX_FX)
#define CS_STRING_PACKAGES		(CS_LIGHT_STYLES + (MAX_LIGHT_STYLES*3))

#define CS_NEW					(CS_STRING_PACKAGES + MAX_STRING_PACKAGES)
#define CS_INGAME_MOTD			(CS_NEW + 0)
#define CS_READY				(CS_NEW + 1)
#define CS_UNPAUSE				(CS_NEW + 2)

#define CS_MAPS					(CS_NEW + 32)

#define CS_MAX					(CS_MAPS + MAX_CS_MAPS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

#define MAX_MODES				128
#define MAX_SERVER_MAPS			128

typedef enum {
	G2_MODELPART_INVALID = -1,
	G2_MODELPART_HEAD = 10,
	G2_MODELPART_WAIST,
	G2_MODELPART_LARM,
	G2_MODELPART_RARM,
	G2_MODELPART_RHAND,
	G2_MODELPART_LLEG,
	G2_MODELPART_RLEG
} g2ModelParts_t;

#define G2_MODEL_PART	50

typedef enum {
	GT_FFA,				// free for all
	GT_HOLOCRON,		// holocron ffa
	GT_JEDIMASTER,		// jedi master
	GT_TOURNAMENT,		// one on one tournament
	GT_SINGLE_PLAYER,	// single player ffa

	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_SAGA,			// saga
	GT_CTF,				// capture the flag
	GT_CTY,
	GT_REDROVER,		// slain join your team
	GT_CLANARENA,		// round-based tffa
	GT_MAX_GAME_TYPE
} gametype_t;

#define GT_Flag(x) ((x) == GT_CTF || (x) == GT_CTY )
#define GT_Team(x) ((x) >= GT_TEAM)
#define GT_Round(x) ((x) >= GT_REDROVER)
#define GT_Valid(x) (0 <= (x) && (x) < GT_MAX_GAME_TYPE)

extern const char * const gametypeLong[GT_MAX_GAME_TYPE];
extern const char * const gametypeShort[GT_MAX_GAME_TYPE];

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

extern const vec3_t WP_MuzzlePoint[WP_NUM_WEAPONS];

extern const forcePowers_t forcePowerSorted[NUM_FORCE_POWERS];

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
} animation_t;

extern qboolean			BGPAFtextLoaded;
extern animation_t		bgGlobalAnimations[MAX_TOTALANIMATIONS];

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define	ANIM_TOGGLEBIT		2048		// Maximum number of animation sequences is 2048 (0-2047).  12th bit is the toggle

#define ANIM(x) ((animNumber_t)((x) & ~ANIM_TOGGLEBIT))

typedef enum {
	FORCE_MASTERY_UNINITIATED,
	FORCE_MASTERY_INITIATE,
	FORCE_MASTERY_PADAWAN,
	FORCE_MASTERY_JEDI,
	FORCE_MASTERY_JEDI_GUARDIAN,
	FORCE_MASTERY_JEDI_ADEPT,
	FORCE_MASTERY_JEDI_KNIGHT,
	FORCE_MASTERY_JEDI_MASTER,
	NUM_FORCE_MASTERY_LEVELS
} forceMastery_t;
extern const char * const forceMasteryLevels[NUM_FORCE_MASTERY_LEVELS];
extern const int forceMasteryPoints[NUM_FORCE_MASTERY_LEVELS];

extern int bgForcePowerCost[NUM_FORCE_POWERS][NUM_FORCE_POWER_LEVELS];

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define PMF_ROLLING			4
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_RESPAWNED		512		// clear after attack and jump buttons come up
#define	PMF_USE_ITEM_HELD	1024
#define PMF_UPDATE_ANIM		2048	// The server updated the animation, the pmove should set the ghoul2 anim to match.
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboard
#define PMF_STILL			16384	// player doesn't press direction arrows

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;

	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	noKick;				// players can't kick each another
	qboolean	noYDFA;				// disable pmove ydfa (attack and then jump)
	qboolean	newPmove;			// new, FPS-independed pmove (not complete)
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something

	int			framecount;

	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];

	int			useEvent;

	vec3_t		mins, maxs;			// bounding box size

	int			watertype;
	int			waterlevel;

	int			gametype;

	const animation_t	*animations;

	float		xyspeed;

	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );

	playerState_t	*bgClients[MAX_CLIENTS];
	int			checkDuelLoss;
} pmove_t;

extern	pmove_t		*pm;

#define SETANIM_TORSO 1
#define SETANIM_LEGS  2
#define SETANIM_BOTH  (SETANIM_TORSO|SETANIM_LEGS)

#define SETANIM_FLAG_NORMAL		0//Only set if timer is 0
#define SETANIM_FLAG_OVERRIDE	1//Override previous
#define SETANIM_FLAG_HOLD		2//Set the new timer
#define SETANIM_FLAG_RESTART	4//Allow restarting the anim if playing the same one (weapon fires)
#define SETANIM_FLAG_HOLDLESS	8//Set the new timer


// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void PM_UpdateViewAngles2( vec3_t viewangles, int *delta_angles, const usercmd_t *cmd );
void PM_SetDeltaAngles(int *delta_angles, const vec3_t angle, const usercmd_t *ucmd);

void Pmove (pmove_t *pmove);

//===================================================================================


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	STAT_HEALTH,
	STAT_HOLDABLE_ITEM,
	STAT_HOLDABLE_ITEMS,
	STAT_PERSISTANT_POWERUP,
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,
	STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,				// unused
	STAT_MAX_HEALTH,				// health / armor limit, changable by handicap
	STAT_MAX
} statIndex_t;

q_static_assert(STAT_MAX <= MAX_STATS);

// player_state->persistant[] indexes
// these fields are the only part of player_state that isn't
// cleared on respawn
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_KILLS,						// how many enemies you killed
	PERS_KILLED,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
	PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
	PERS_DEFEND_COUNT,				// defend awards
	PERS_ASSIST_COUNT,				// assist awards
	PERS_GAUNTLET_FRAG_COUNT,		// kills with the guantlet
	PERS_CAPTURES,					// captures
	PERS_MAX
} persEnum_t;

q_static_assert(PERS_MAX <= MAX_PERSISTANT);

// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define	EF_BOUNCE_SHRAPNEL	0x00000002		// special shrapnel flag
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes

//doesn't do anything
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite

#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles

#define	EF_BOUNCE_HALF		0x00000020		// for missiles

//doesn't do anything
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite

#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define EF_ALT_FIRING		0x00000200		// for alt-fires, mostly for lightning guns though
#define	EF_MOVER_STOP		0x00000400		// will push otherwise

//doesn't do anything
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite

#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote

//next 4 don't actually do anything
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_AWARDS			0x00078848		// sum of all awards for clearing

#define EF_TEAMVOTED		0x00080000		// already cast a team vote
#define EF_SEEKERDRONE		0x00100000		// show seeker drone floating around head
#define EF_MISSILE_STICK	0x00200000		// missiles that stick to the wall.
#define EF_ITEMPLACEHOLDER	0x00400000		// item effect
#define EF_SOUNDTRACKER		0x00800000		// sound position needs to be updated in relation to another entity
#define EF_DROPPEDWEAPON	0x01000000		// it's a dropped weapon
#define EF_DISINTEGRATION	0x02000000		// being disintegrated by the disruptor
#define EF_INVULNERABLE		0x04000000		// just spawned in or whatever, so is protected



typedef enum {
	EFFECT_NONE = 0,
	EFFECT_SMOKE,
	EFFECT_EXPLOSION,
	EFFECT_EXPLOSION_PAS,
	EFFECT_SPARK_EXPLOSION,
	EFFECT_EXPLOSION_TRIPMINE,
	EFFECT_EXPLOSION_DETPACK,
	EFFECT_EXPLOSION_FLECHETTE,
	EFFECT_STUNHIT,
	EFFECT_EXPLOSION_DEMP2ALT,
	EFFECT_MAX
} effectTypes_t;

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,

	PW_QUAD,
	PW_BATTLESUIT,
	PW_HASTE,
	//PW_INVIS, //rww - removed
	//PW_REGEN, //rww - removed
	//PW_FLIGHT, //rww - removed

	PW_REDFLAG,
	PW_BLUEFLAG,
	PW_NEUTRALFLAG,

	PW_SHIELDHIT,

	//PW_SCOUT, //rww - removed
	//PW_GUARD, //rww - removed
	//PW_DOUBLER, //rww - removed
	//PW_AMMOREGEN, //rww - removed
	PW_SPEEDBURST,
	PW_DISINT_4,
	PW_SPEED,
	PW_FORCE_LIGHTNING,
	PW_FORCE_ENLIGHTENED_LIGHT,
	PW_FORCE_ENLIGHTENED_DARK,
	PW_FORCE_BOON,
	PW_YSALAMIRI,

	PW_NUM_POWERUPS

} powerup_t;

q_static_assert(PW_NUM_POWERUPS < 31);

typedef enum {
	HI_NONE,

	HI_SEEKER,
	HI_SHIELD,
	HI_MEDPAC,
	HI_DATAPAD,
	HI_BINOCULARS,
	HI_SENTRY_GUN,

	HI_NUM_HOLDABLE
} holdable_t;

// HI_NONE breaks item selector cycle in cgame
#define LEGAL_ITEMS ((1 << HI_SEEKER) | (1 << HI_SHIELD) | (1 << HI_MEDPAC) | (1 << HI_SENTRY_GUN) | (1 << HI_BINOCULARS))

typedef enum {
	CTFMESSAGE_FRAGGED_FLAG_CARRIER,
	CTFMESSAGE_FLAG_RETURNED,
	CTFMESSAGE_PLAYER_RETURNED_FLAG,
	CTFMESSAGE_PLAYER_CAPTURED_FLAG,
	CTFMESSAGE_PLAYER_GOT_FLAG
} ctfMsg_t;

// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum
{
	PDSOUND_NONE,
	PDSOUND_PROTECTHIT,
	PDSOUND_PROTECT,
	PDSOUND_ABSORBHIT,
	PDSOUND_ABSORB,
	PDSOUND_FORCEJUMP,
	PDSOUND_FORCEGRIP
} pdSounds_t;

typedef enum {
	EV_NONE,

	EV_CLIENTJOIN,

	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,

	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,

	EV_FALL,

	EV_JUMP_PAD,			// boing sound at origin, jump sound on player

	EV_PRIVATE_DUEL,

	EV_JUMP,
	EV_ROLL,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves

	EV_ITEM_PICKUP,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyone

	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	EV_ALT_FIRE,
	EV_SABER_ATTACK,
	EV_SABER_HIT,
	EV_SABER_BLOCK,
	EV_SABER_UNHOLSTER,
	EV_BECOME_JEDIMASTER,
	EV_DISRUPTOR_MAIN_SHOT,
	EV_DISRUPTOR_SNIPER_SHOT,
	EV_DISRUPTOR_SNIPER_MISS,
	EV_DISRUPTOR_HIT,
	EV_DISRUPTOR_ZOOMSOUND,

	EV_PREDEFSOUND,

	EV_TEAM_POWER,

	EV_SCREENSHAKE,

	EV_USE,			// +Use key

	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,

	EV_ITEMUSEFAIL,

	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,

	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex
	EV_MISSILE_STICK,		// eventParm will be the soundindex

	EV_PLAY_EFFECT,
	EV_PLAY_EFFECT_ID,

	EV_MUTE_SOUND,
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,
	EV_ENTITY_SOUND,

	EV_PLAY_ROFF,

	EV_GLASS_SHATTER,
	EV_DEBRIS,

	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_BULLET,				// otherEntity is the shooter

	EV_PAIN,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,

	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	//EV_POWERUP_REGEN,

	EV_FORCE_DRAINED,

	EV_GIB_PLAYER,			// gib a previously living player
	EV_SCOREPLUM,			// score plum

	EV_CTFMESSAGE,

	EV_SAGA_ROUNDOVER,
	EV_SAGA_OBJECTIVECOMPLETE,

	EV_DESTROY_GHOUL2_INSTANCE,

	EV_DESTROY_WEAPON_MODEL,

	EV_GIVE_NEW_RANK,
	EV_SET_FREE_SABER,
	EV_SET_FORCE_DISABLE,

	EV_WEAPON_CHARGE,
	EV_WEAPON_CHARGE_ALT,

	EV_SHIELD_HIT,

	EV_DEBUG_LINE,
	EV_TESTLINE,
	EV_STOPLOOPINGSOUND,
	EV_STARTLOOPINGSOUND,
	EV_TAUNT,
	EV_TAUNT_YES,
	EV_TAUNT_NO,
	EV_TAUNT_FOLLOWME,
	EV_TAUNT_GETFLAG,
	EV_TAUNT_GUARDBASE,
	EV_TAUNT_PATROL,

	EV_BODY_QUEUE_COPY,

	EV_MAX,
} entity_event_t;			// There is a maximum of 256 events (8 bits transmission, 2 high bits for uniqueness)


typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED
} global_team_sound_t;

typedef enum {
	CASE_NORMAL,
	CASE_UPPER,
	CASE_LOWER,
	CASE_MAX
} letterCase_t;

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,

	TEAM_NUM_TEAMS
} team_t;

// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		8

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE,
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;

// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_STUN_BATON,
	MOD_MELEE,
	MOD_SABER,
	MOD_BRYAR_PISTOL,
	MOD_BRYAR_PISTOL_ALT,
	MOD_BLASTER,
	MOD_DISRUPTOR,
	MOD_DISRUPTOR_SPLASH,
	MOD_DISRUPTOR_SNIPER,
	MOD_BOWCASTER,
	MOD_REPEATER,
	MOD_REPEATER_ALT,
	MOD_REPEATER_ALT_SPLASH,
	MOD_DEMP2,
	MOD_DEMP2_ALT,
	MOD_FLECHETTE,
	MOD_FLECHETTE_ALT_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_ROCKET_HOMING,
	MOD_ROCKET_HOMING_SPLASH,
	MOD_THERMAL,
	MOD_THERMAL_SPLASH,
	MOD_TRIP_MINE_SPLASH,
	MOD_TIMED_MINE_SPLASH,
	MOD_DET_PACK_SPLASH,
	MOD_FORCE_DARK,
	MOD_SENTRY,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_LEAVE,
	MOD_MAX
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	const char	*classname;	// spawning name
	const char	*pickup_sound;
	const char	*world_model[MAX_ITEM_MODELS];
	const char	*view_model;

	const char	*icon;
//	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

	const char	*precaches;		// string of all models and images this item will use
	const char	*sounds;		// string of all sounds this item will use
} const gitem_t;

// included in both the game dll and the client
extern	gitem_t		bg_itemlist[];
extern	const int	bg_numItems;

float vectoyaw( const vec3_t vec );

gitem_t	*BG_FindItem( const char *classname );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps );



#define SABER_BLOCK_DUR 150		// number of milliseconds a block animation should take.



// g_dmflags->integer flags
#define DF_NEW_PMOVE			1
#define DF_CJK					2
#define DF_TEAM_PASS			4
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32
#define	DF_NO_KICK				64
#define	DF_NO_YDFA				128

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)


//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_SPECIAL,				// rww - force fields
	ET_HOLOCRON,			// rww - holocron icon displays
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,
	ET_BODY,

	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;

typedef enum {
	Q_NEGATIVE = -1, // make sure this enum is signed
	Q_BR,
	Q_R,
	Q_TR,
	Q_T,
	Q_TL,
	Q_L,
	Q_BL,
	Q_B,
	Q_NUM_QUADS
} saberQuadrant_t;

typedef struct
{
	const char *name;
	animNumber_t animToUse;
	saberQuadrant_t	startQuad;
	saberQuadrant_t	endQuad;
	unsigned animSetFlags;
	int blendTime;
	saberBlockType_t blocking;
	saberMoveName_t chain_idle;			// What move to call if the attack button is not pressed at the end of this anim
	saberMoveName_t chain_attack;		// What move to call if the attack button (and nothing else) is pressed
	int trailLength;
} saberMoveData_t;

extern const saberMoveData_t	saberMoveData[LS_MOVE_MAX];

const char	*BG_TeamName(team_t team, letterCase_t letterCase);
const char	*BG_TeamColor(team_t team);
team_t		BG_OtherTeam(team_t team);
team_t		BG_TeamFromString(const char *s);

qboolean BG_LegalizedForcePowers(char *powerOut, forceMastery_t maxRank, qboolean freeSaber, forceSide_t teamForce, gametype_t gametype, int fpDisabled);

//BG anim utility functions:
animNumber_t BG_Anim( int anim );
qboolean BG_InSpecialJump( animNumber_t anim );
qboolean BG_InSaberStandAnim( animNumber_t anim );
qboolean BG_DirectFlippingAnim( animNumber_t anim );
qboolean BG_SaberInAttack( saberMoveName_t move );
qboolean BG_SaberInSpecial( saberMoveName_t move );
qboolean BG_SaberInIdle( saberMoveName_t move );
qboolean BG_FlippingAnim( animNumber_t anim );
qboolean BG_SpinningSaberAnim( animNumber_t anim );
qboolean BG_SaberInSpecialAttack( animNumber_t anim );
saberMoveName_t BG_BrokenParryForAttack( saberMoveName_t move );
saberMoveName_t BG_BrokenParryForParry( saberMoveName_t move );
saberMoveName_t BG_KnockawayForParry( saberBlockedType_t move );
qboolean BG_InRoll( const playerState_t *ps, animNumber_t anim );
qboolean BG_InDeathAnim( animNumber_t anim );

void BG_SaberStartTransAnim( animNumber_t anim, float *animSpeed );

void BG_ForcePowerDrain( playerState_t *ps, forcePowers_t forcePower, int overrideAmt );

void	BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result );

void	BG_AddPredictableEventToPlayerstate( entity_event_t newEvent, int eventParm, playerState_t *ps );

void	BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap, qboolean extrapolate );

void	BG_AdjustPositionForMover( const vec3_t in, const entityState_t *mover, int fromTime, int toTime, vec3_t out );

qboolean	BG_PlayerTouchesItem( const playerState_t *ps, const entityState_t *item, int atTime );

qboolean	BG_ParseAnimationFile(const char *filename);

int BG_GetItemIndexByTag(int tag, itemType_t type);

qboolean BG_HasYsalamiri(int gametype, playerState_t *ps);
qboolean BG_CanUseFPNow(int gametype, playerState_t *ps, int time, forcePowers_t power);

void *BG_Alloc ( size_t size );
void *BG_AllocUnaligned ( size_t size );
void *BG_TempAlloc( size_t size );
void BG_TempFree( size_t size );
char *BG_StringAlloc ( const char *source );
qboolean BG_OutOfMemory ( void );

extern const animNumber_t WeaponReadyAnim[WP_NUM_WEAPONS];
extern const animNumber_t WeaponAttackAnim[WP_NUM_WEAPONS];

extern const forceSide_t forcePowerDarkLight[NUM_FORCE_POWERS];

#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192

// sent over the net as uint16_t. Compatible with old negative values
#define DEBRIS_SPECIALCASE_ROCK			0xffff // -1
#define DEBRIS_SPECIALCASE_CHUNKS		0xfffe // -2
#define DEBRIS_SPECIALCASE_WOOD			0xfffd // -3
#define DEBRIS_SPECIALCASE_GLASS		0xfffc // -4
#define DEBRIS_SPECIALCASE_MIN			0xfffc

#endif //__BG_PUBLIC_H__
