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

// g_local.h -- local definitions for game module

#include "q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include "mvapi.h"

//==================================================================

#define BODY_QUEUE_SIZE		8

#define INFINITE			1000000
#define Q3_INFINITE			16777216

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000
#define	ROUND_INTERMISSION_DELAY_TIME	4000

// gentity->flags
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client

#define ANIMENT_SPAWNER //allow animent spawners

// These are public and shouldn't be changed
#define LOG_GAME			0x00000001
#define LOG_CONNECT			0x00000002
#define LOG_BEGIN			0x00000004
#define LOG_USERINFO		0x00000008
#define LOG_RENAME			0x00000010
#define LOG_SPAWN			0x00000020
#define LOG_PRIVATE_DUEL	0x00000040
#define LOG_KILL			0x00000080
#define LOG_SAY				0x00000100
#define LOG_SAY_TEAM		0x00000200
#define LOG_TELL			0x00000400
#define LOG_VTELL			0x00000800
#define LOG_ITEM			0x00001000
#define LOG_FLAG			0x00002000
#define LOG_WEAPON_STATS	0x00004000
#define LOG_GAME_STATS		0x00008000
#define LOG_AUSTRIAN		0x00010000
#define LOG_VOTE			0x00020000
#define LOG_REFEREE			0x00040000

#define LOG_DEFAULT			41943

#define MAX_LOGFILES		4

// Never remove/rearrange items for backwards compatibility
// DOCME in g_allowVote description
typedef enum {
	CV_INVALID,
	CV_FIRST,
	CV_MAP_RESTART = CV_FIRST,
	CV_NEXTMAP,
	CV_MAP,
	CV_GAMETYPE,
	CV_KICK,
	CV_SHUFFLE,
	CV_DOWARMUP,
	CV_TIMELIMIT,
	CV_FRAGLIMIT,
	CV_ROUNDLIMIT,
	CV_TEAMSIZE,
	CV_REMOVE,
	CV_WK,
	CV_MODE,
	CV_MATCH,
	CV_CAPTURELIMIT,
	CV_POLL,
	CV_REFEREE,
	CV_ABORT,
	CV_MAX
} voteCmd_t;

q_static_assert(CV_MAX <= 32);

typedef enum {
	CTV_INVALID,
	CTV_FIRST,
	CTV_LEADER = CTV_FIRST,
	CTV_FORFEIT,
	CTV_MAX
} teamVoteCmd_t;

q_static_assert(CTV_MAX <= 32);

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1
} moverState_t;

typedef enum //# material_e
{
	MAT_METAL = 0,
	MAT_GLASS,
	MAT_ELECTRICAL,// (sparks)
	MAT_ORGANIC,// (not implemented)
	MAT_BORG,//borg chunks
	MAT_STASIS,//stasis chunks
	MAT_GLASS_METAL,//mixed chunk type
	NUM_MATERIALS
} material_t;

#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"

typedef enum
{
	HL_NONE = 0,
	HL_FOOT_RT,
	HL_FOOT_LT,
	HL_LEG_RT,
	HL_LEG_LT,
	HL_WAIST,
	HL_BACK_RT,
	HL_BACK_LT,
	HL_BACK,
	HL_CHEST_RT,
	HL_CHEST_LT,
	HL_CHEST,
	HL_ARM_RT,
	HL_ARM_LT,
	HL_HAND_RT,
	HL_HAND_LT,
	HL_HEAD,
	HL_GENERIC1,
	HL_GENERIC2,
	HL_GENERIC3,
	HL_GENERIC4,
	HL_GENERIC5,
	HL_GENERIC6,
	HL_MAX
} hitLoc_t;

enum {
	KICK_NOEFFECT,
	KICK_BASEJK,
	KICK_NODAMAGE,
	KICK_LEAGUE,
};

//============================================================================

extern void *precachedKyle;
extern void *g2SaberInstance;

extern qboolean gEscaping;
extern int gEscapeTime;

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t	s;				// communicated by server to clients
	entityShared_t	r;				// shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;			// NULL if not a client

	qboolean	inuse;

	const char	*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	team_t		teamnodmg;			// set in QuakeEd

	char		*roffname;			// set in QuakeEd
	char		*rofftarget;		// set in QuakeEd

	int			objective;
	int			side;

	int			passThroughNum;		// set to index to pass through (+1) for missiles

	int			aimDebounceTime;
	int			painDebounceTime;
	int			attackDebounceTime;
	team_t		noDamageTeam;

	int			roffid;				// if roffname != NULL then set on spawn

	qboolean	neverFree;			// if true, FreeEntity will only unlink
									// bodyque uses this

	int			flags;				// FL_* variables

	const char	*model;
	const char	*model2;
	int			freetime;			// level.time when the object was freed

	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
	qboolean	unlinkAfterEvent;

	qboolean	physicsObject;		// if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects,
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*parent;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
	vec3_t		pos1, pos2;

	char		*message;

	int			timestamp;		// body queue sinking, etc

	float		angle;			// set in editor, -1 = up, -2 = down
	char		*target;
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;
	float		mass;
	int			setTime;

//Think Functions
	int			nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t mod);

	int			pain_debounce_time;
	int			fly_sound_debounce_time;	// wind tunnel
	int			last_move_time;

//Health and damage fields
	int			health;
	qboolean	takedamage;
	material_t	material;

	int			damage;
	int			dflags;
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	meansOfDeath_t	methodOfDeath;
	meansOfDeath_t	splashMethodOfDeath;

	int			count;
	int			bounceCount;
	qboolean	alt_fire;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team

	int			watertype;
	int			waterlevel;

	int			noise_index;

	// timing variables
	float		wait;
	float		random;
	int			delay;

	//rww - these values were created when we needed to keep track of things like this on the server
	//more, but now only clients really have a server g2 instance (though it is possible to create a
	//server-side g2 instance for a non-client). As with their client and shared counterparts, these
	//values have become somewhat generic values which are used out of convenience for a number of value
	//placeholders.
	int			boltpoint1;
	int			boltpoint2;
	int			boltpoint3;
	int			boltpoint4;

	int			time1;
	int			time2;

	int			bolt_Head;
	int			bolt_LArm;
	int			bolt_RArm;
	int			bolt_LLeg;
	int			bolt_RLeg;
	int			bolt_Waist;
	int			bolt_Motion;

	qboolean	isSaberEntity;

	int			damageRedirect; //if entity takes damage, redirect to..
	int			damageRedirectTo; //this entity number

	gitem_t		*item;			// for bonus items

	qboolean	freeOnStop;

	// 1. Number of a client who should be blamed for this entity
	// 2. ENTITYNUM_WORLD for entities that should be always broadcasted
	// 3. ENTITYNUM_NONE for entities that don't belong to anyone but
	// can be ommited by dueling players
	int			blameEntityNum;
	unsigned	dimension;
};

#define DAMAGEREDIRECT_HEAD		1
#define DAMAGEREDIRECT_RLEG		2
#define DAMAGEREDIRECT_LLEG		3

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef enum {
	VOTE_NONE,
	VOTE_NO,
	VOTE_YES,
	VOTE_MAX,
} vote_t;

typedef union {
	byte		b[4];
	unsigned	ui;
} qipv4_t;

typedef struct ucmdStat_s {
	int		serverTime;
	int		thinkTime;
} ucmdStat_t;

#define CMD_MASK 1023

typedef struct {
	playerTeamStateState_t	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	int			lasthurtcarrier;
	int			lastreturnedflag;
	int			flagsince;
	int			lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow two top scoring clients
#define	FOLLOW_FIRST	(-1)
#define	FOLLOW_SECOND	(-2)
#define FOLLOW_TEAM		(-3)

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t				sessionTeam;
	int					spectatorNum;		// for determining next-in-line to play
	spectatorState_t	spectatorState;
	int					spectatorClient;	// for chasecam and follow mode
	int					wins, losses;		// tournament stats
	forcePowers_t		selectedFP;			// check against this, if doesn't match value in playerstate then update userinfo
	forceLevel_t		saberLevel;			// similar to above method, but for current saber attack level
	qboolean			setForce;			// set to true once player is given the chance to set force powers
	qboolean			teamLeader;			// true when this client is a team leader
	qboolean			motdSeen;
	qboolean			referee;
} clientSession_t;

//
#define	MAX_VOTE_COUNT		3

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t	connected;
	usercmd_t	cmd;				// we would lose angles if not persistant
	qboolean	initialSpawn;		// the first spawn should be at a cool location
	int			enterTime;			// level.time the client entered the game
	playerTeamState_t teamState;	// status in teamplay games
//	int			voteCount;			// to prevent people from constantly calling votes
//	int			teamVoteCount;		// to prevent people from constantly calling votes
	int			totalDamageTakenFromEnemies;
	int			totalDamageDealtToEnemies;
	int			totalDamageTakenFromAllies;
	int			totalDamageDealtToAllies;
	qboolean	registered;
	int			accuracy_shots;		// total number of shots
	int			accuracy_hits;		// total number of hits
	vote_t		vote;
	vote_t		teamVote;
	qboolean	ready;
	int			timeouts;			// number of timeouts called

	int			persistant[MAX_PERSISTANT];	// persistant fields for current
											// player. ps.persistant is updated
											// in ClientEndFrame
} clientPersistant_t;

// client data parsed from userinfo
typedef struct {
	qboolean	localClient;		// true if "ip" info key is "localhost"
	qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean	pmoveFixed;			//
	char		netname[MAX_NETNAME];
	char		model[MAX_QPATH];
	int			color1;				// lightsaber color
	int			color2;
	int			skill;				// 0 = human, 1-5 = bot
	int			teamTask;			// 0 = none, 1 = offence, 2 = defence
	int			maxHealth;			// for handicapping
	qboolean	teamInfo;			// send team overlay updates?
	qboolean	privateDuel;		// based on cg_privateDuel userinfo
} clientUserinfo_t;

// client data that stays across reconnections.
typedef struct {
	qipv4_t		ip;
	int			qport;
	int			switchTeamTime;		// time the player switched teams
} clientProfile_t;

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t	pers;
	clientSession_t		sess;
	clientUserinfo_t	info;
	clientProfile_t		prof;

	int			invulnerableTimer;

	forceLevel_t	saberCycleQueue;

	qboolean	noclip;

	int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	vec3_t		oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation
	qboolean	damage_fromWorld;	// if true, don't use the damage_from vector

	int			damageBoxHandle_Head; //entity number of head damage box
	int			damageBoxHandle_RLeg; //entity number of right leg damage box
	int			damageBoxHandle_LLeg; //entity number of left leg damage box

	int			accurateCount;		// for "impressive" reward sound

	//
	int			lastkilled_client;	// last client that this client killed
	int			lasthurt_client;	// last client that damaged this client
	int			lasthurt_mod;		// type of damage the client did

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			inactivityTime;		// kick players when time > this + g_inactivity
	qboolean	inactivityWarning;	// qtrue if the five seoond warning has been given
	int			rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int			airOutTime;			// time the players needs to breathe

	int			lastKillTime;		// for multiple kill rewards
	int			readyTime;			// last time ready command was issued

	qboolean	fireHeld;			// used for hook
	gentity_t	*hook;				// grapple hook if out

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

	char		*areabits;

	void		*ghoul2;		// In parallel with the centity, there is a corresponding ghoul2 model for players.
								// This is an instance that is maintained on the server side that is used for
								// determining saber position and per-poly collision

	vec3_t		lastSaberTip;		//position of saber tip last update
	vec3_t		lastSaberBase;		//position of saber base last update

	vec3_t		lastSaberDir_Always; //every getboltmatrix, set to saber dir
	vec3_t		lastSaberBase_Always; //every getboltmatrix, set to saber base
	int			lastSaberStorageTime; //server time that the above two values were updated (for making sure they aren't out of date)

	qboolean	hasCurrentPosition;	//are lastSaberTip and lastSaberBase valid?

	int			dangerTime;		// level.time when last attack occured

	int			forcePowerSoundDebounce; //if > level.time, don't do certain sound events again (drain sound, absorb sound, etc)

	qboolean	fjDidJump;
	qboolean	duelStarted;

	ucmdStat_t	cmdStats[CMD_MASK + 1];
	int			cmdIndex;
};


//
// this structure is cleared as each map is entered
//
#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gclient_s	*clients;		// [maxclients]

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;		// current number, <= MAX_GENTITIES

	int			warmupTime;			// restart match at this time

	fileHandle_t	logFile[MAX_LOGFILES];

	// store latched cvars here that we want to get at often
	int			maxclients;
	gametype_t	gametype;

	int			framenum;
	int			snapnum;				// snapshot currently being prepared
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			startTime;				// level.time the map was started
	int			unpauseTime;			// unpause after this time

	int			teamScores[TEAM_NUM_TEAMS];
	int			lastTeamLocationTime;		// last time of client team location update

	qboolean	newSession;				// don't use any old session data, because
										// we changed gametype

	qboolean	restarted;				// waiting for a map_restart to fire

	int			numConnectedClients;
	int			numNonSpectatorClients;	// includes connecting clients
	int			numPlayingClients;		// connected, non-spectators
	int			sortedClients[MAX_CLIENTS];		// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators

	int			snd_fry;				// sound index for standing in lava

	int			warmupModificationCount;	// for detecting if g_warmup is changed
	int			dmflagsModificationCount;

	// voting state
	char		voteString[MAX_STRING_CHARS];
	char		voteDisplayString[MAX_STRING_CHARS];
	int			voteTime;				// level.time vote was called
	int			voteExecuteTime;		// time the vote is executed
	int			voteYes;
	int			voteNo;
	int			numVotingClients;		// set by CalculateRanks
	int			voteCooldown;			// when voteClient may call a new vote
	vote_t		voteReferee;

	voteCmd_t	voteCmd;			// current vote
	int			voteArg;				// vote argument for CheckVote
	int			voteClient;				// client who called current/last vote

	// team voting state
	teamVoteCmd_t	teamVoteCmd[2];		// current vote
	int			teamVoteArg[2];
	char		teamVoteString[2][MAX_STRING_CHARS];
	int			teamVoteTime[2];		// level.time vote was called
	int			teamVoteYes[2];
	int			teamVoteNo[2];
	int			numteamVotingClients[2];// set by CalculateRanks

	// spawn variables
	qboolean	spawning;				// the G_Spawn*() functions are valid
	int			numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int			intermissionQueued;		// intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int			intermissiontime;		// time the intermission was started
	char		*changemap;
	qboolean	readyToExit;			// at least one client wants to exit
	int			exitTime;
	vec3_t		intermission_origin;	// also used for spectator spawns
	vec3_t		intermission_angle;

	int			roundQueued;			// new round was qualified, but we're
										// doing a g_roundWarmup sec countdown
	team_t		forfeitTeam;			// Team wants to forfeit the match

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list
	int			bodyQueIndex;			// dead bodies
	gentity_t	*bodyQue[BODY_QUEUE_SIZE];
	int			portalSequence;

	qboolean	teamLock[TEAM_NUM_TEAMS];
	int			round;
	int			idleTime;
	qboolean	restarting;				// server is about to restart

	char		queuedCmd[1024];
	int			queuedCmdSnap;

	int			timeoutClient;

	int			duelist1;
	int			duelist2;
	qboolean	duelExit;
} level_locals_t;

//
// g_spawn.c
//
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );
qboolean	G_IsSpawnEntity( const gentity_t *ent );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gclient_t *client, int oldTeam );
qboolean ValidateTeam( int ignoreClientNum, team_t team );
qboolean SetTeam( gentity_t *ent, team_t team );
qboolean SetTeamSpec( gentity_t *ent, team_t team, spectatorState_t specState, int specClient );
void Cmd_FollowCycle_f( gentity_t *ent, int dir );
void Cmd_SmartFollowCycle_f( gentity_t *ent );
void Cmd_SaberAttackCycle_f(gentity_t *ent);
int G_ItemUsable(playerState_t *ps, holdable_t forcedUse);
void Cmd_ToggleSaber_f(gentity_t *ent);
void Cmd_EngageDuel_f(gentity_t *ent);
char *ConcatArgs( int start );

int G_ClientNumberFromString (const char *s, const char **errorMsg);

//
// g_items.c
//
void ItemUse_Binoculars(gentity_t *ent);
void ItemUse_Shield(gentity_t *ent);
void ItemUse_Sentry(gentity_t *ent);
void ItemUse_Seeker(gentity_t *ent);
void ItemUse_MedPack(gentity_t *ent);

void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
void PrecacheItem (gitem_t *it);
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity, int blameEntityNum );
void SetRespawn (gentity_t *ent, float delay);
void G_SpawnItem (gentity_t *ent, gitem_t *item);
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon (gentity_t *ent);
int ArmorIndex (gentity_t *ent);
void	Add_Ammo (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace);

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

qboolean	G_HoldableDisabled( holdable_t holdable );

//
// g_utils.c
//
int G_ModelIndex( const char *name );
int		G_SoundIndex( const char *name );
int		G_EffectIndex( const char *name );
void	G_TeamCommand( team_t team, char *cmd );
void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
int		G_RadiusList ( const vec3_t origin, float radius,	gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES]);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);
void	G_SetAngles( gentity_t *ent, const vec3_t angles );

void	G_InitGentity( gentity_t *e, int blameEntityNum );
gentity_t	*G_Spawn ( int blameEntityNum );
gentity_t *G_TempEntity( const vec3_t origin, entity_event_t event, int blameEntityNum );
gentity_t	*G_PlayEffect(effectTypes_t fxID, const vec3_t org, const vec3_t ang, int blameEntityNum);
gentity_t *G_ScreenShake(const vec3_t org, gentity_t *target, float intensity, int duration, qboolean global);
void	G_MuteSound( int entnum, soundChannel_t channel );
void	G_Sound( gentity_t *ent, int channel, int soundIndex );
void	G_SoundAtLoc( vec3_t loc, soundChannel_t channel, int soundIndex, int blameEntityNum );
void	G_EntitySound( gentity_t *ent, soundChannel_t channel, int soundIndex );
void	TryUse( gentity_t *ent );
void	G_SendG2KillQueue(void);
void	G_KillG2Queue(int entNum);
void	G_FreeEntity( gentity_t *ed );
qboolean	G_EntitiesFree( void );
void	G_FreeUnusedEntities( void );

void	G_TouchTriggers (gentity_t *ent);
void	G_TouchSolids (gentity_t *ent);
int		G_PointEntityContents(const vec3_t point, int passEntityNum);


//
// g_object.c
//

extern void G_RunObject			( gentity_t *ent );


float	*tv (float x, float y, float z);
char	*vtos( const vec3_t v );

void G_AddPredictableEvent( gentity_t *ent, entity_event_t event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, const vec3_t origin );
void AddRemap(const char *oldShader, const char *newShader, int timeOffset);
const char *BuildShaderStateConfig(void);
/*
Ghoul2 Insert Start
*/
int G_SkinIndex( const char *name );

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
int			trap_G2API_AddBolt(void *ghoul2, int modelIndex, const char *boneName);
void		trap_G2API_SetBoltInfo(void *ghoul2, int modelIndex, int boltInfo);

int			trap_G2API_CopyGhoul2Instance(void *g2From, void *g2To, int modelIndex);
void		trap_G2API_CopySpecificGhoul2Model(void *g2From, int modelFrom, void *g2To, int modelTo);
void		trap_G2API_DuplicateGhoul2Instance(void *g2From, void **g2To);
qboolean	trap_G2API_HasGhoul2ModelOnIndex(void *ghlInfo, int modelIndex);
qboolean	trap_G2API_RemoveGhoul2Model(void *ghlInfo, int modelIndex);
void		trap_G2API_CleanGhoul2Models(void **ghoul2Ptr);
void		trap_G2API_CollisionDetect ( CollisionRecord_t *collRecMap, void* ghoul2, const vec3_t angles, const vec3_t position,
								int frameNumber, int entNum, const vec3_t rayStart, const vec3_t rayEnd, const vec3_t scale, int traceFlags, int useLod, float fRadius );

qboolean	trap_G2API_SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags,
								const int up, const int right, const int forward, qhandle_t *modelList,
								int blendTime , int currentTime );
void		trap_G2API_GetGLAName(void *ghoul2, int modelIndex, char *fillBuf);
qboolean	trap_G2API_SetBoneAnim(void *ghoul2, const int modelIndex, const char *boneName, const int startFrame, const int endFrame,
							  const int flags, const float animSpeed, const int currentTime, const float setFrame , const int blendTime );
/*
Ghoul2 Insert End
*/

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, vec3_t origin);
void G_Damage (gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t direction, const vec3_t point, int damage, int dflags, meansOfDeath_t mod);
qboolean G_RadiusDamage (vec3_t origin, gentity_t *attacker, int damage, int radius, gentity_t *ignore, meansOfDeath_t mod);
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath );
void TossClientWeapon(gentity_t *self, const vec3_t direction, float speed);
void TossClientItems( gentity_t *self );
void TossClientCubes( gentity_t *self );
void ExplodeDeath( gentity_t *self );
void G_CheckForDismemberment(gentity_t *ent, vec3_t point, int damage, animNumber_t deathAnim);
hitLoc_t G_GetHitLocation(gentity_t *target, vec3_t ppoint);
extern int gGAvoidDismember;


// damage flags
#define DAMAGE_NORMAL				0x00000000	// No flags set.
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and godmode have no effect
//JK2 flags
#define DAMAGE_EXTRA_KNOCKBACK		0x00000040	// add extra knockback to this damage
#define DAMAGE_DEATH_KNOCKBACK		0x00000080	// only does knockback on death of target
#define DAMAGE_IGNORE_TEAM			0x00000100	// damage is always done, regardless of teams
#define DAMAGE_NO_DAMAGE			0x00000200	// do no actual damage but react as if damage was taken
#define DAMAGE_HALF_ABSORB			0x00000400	// half shields, half health
#define DAMAGE_HALF_ARMOR_REDUCTION	0x00000800	// This damage doesn't whittle down armor as efficiently.

//
// g_missile.c
//
void G_ReflectMissile( gentity_t *ent, gentity_t *missile, const vec3_t forward );

void G_RunMissile( gentity_t *ent );

gentity_t *CreateMissile( vec3_t org, const vec3_t dir, float vel, int life,
							gentity_t *owner, qboolean altFire);
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout );
void G_ExplodeMissile( gentity_t *ent );

void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire );


//
// g_mover.c
//
#define SPF_BUTTON_USABLE		1
#define SPF_BUTTON_FPUSHABLE	2

void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );

//
// g_trigger.c
//
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
void ATST_ManageDamageBoxes(gentity_t *ent);
int G_PlayerBecomeATST(gentity_t *ent);
void G_CreateExampleAnimEnt(gentity_t *ent);


//
// g_weapon.c
//
void WP_FireTurretMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire, int damage, int velocity, meansOfDeath_t mod, gentity_t *ignore );
void WP_FireGenericBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire, int damage, int velocity, meansOfDeath_t mod );
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint ( const gentity_t *ent, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t muzzlePoint );
void SnapVectorTowards( vec3_t v, const vec3_t to );
qboolean CheckGauntletAttack( gentity_t *ent );


//
// g_client.c
//
int TeamCount( int ignoreClientNum, team_t team, qboolean dead );
int TeamLeader( team_t team );
team_t PickTeam( int ignoreClientNum );
void ResetClientState( gentity_t *self );
void SetClientViewAngle( gentity_t *ent, const vec3_t angle );
gentity_t *SelectSpawnPoint ( gentity_t *ent, const vec3_t avoidPoint, vec3_t origin, vec3_t angles );
void CopyToBodyQue( gentity_t *ent );
void respawn (gentity_t *ent);
void BeginIntermission (void);
void InitBodyQue (void);
void ClientSpawn( gentity_t *ent );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, meansOfDeath_t meansOfDeath);
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *ent, gentity_t *spot );

extern gentity_t *gJMSaberEnt;

//
// g_svcmds.c
//
qboolean	ConsoleCommand( void );
void G_ProcessIPBans(void);
qboolean G_FilterPacket (const char *from);
qipv4_t G_StringToIPv4(const char *s);
void G_CenterPrintPersistant( const char *str );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent, qboolean altFire );
void BlowDetpacks(gentity_t *ent);

//
// p_hud.c
//
void MoveClientToIntermission (gentity_t *ent);
void G_SetStats (gentity_t *ent);
void DeathmatchScoreboardMessage (gentity_t *ent);

//
// g_cmds.c
//

//
// g_pweapon.c
//


//
// g_main.c
//
extern vmCvar_t g_ff_objectives;
extern qboolean gDoSlowMoDuel;
extern int gSlowMoDuelTime;

void FindIntermissionPoint( void );
void SetLeader(team_t team, int client);
void CheckTeamLeader( team_t team );
void G_RunThink (gentity_t *ent);
void AddTournamentQueue(gclient_t *client);
void G_LogPrintf( int event, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
void SendScoreboardMessageToAllClients( void );
void QDECL G_Printf( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
Q_NORETURN void QDECL G_Error( const char *fmt, ... ) __attribute__ ((format (printf, 1, 2)));
void G_SendServerCommand( int clientNum, const char *fmt, ... ) __attribute__ ((format (printf, 2, 3)));
const char *G_GetStripEdString(const char *refSection, const char *refName);
gametype_t G_GametypeForString( const char *s );

//
// g_client.c
//
const char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUpdateConfigString( int clientNum );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum, qboolean allowTeamReset );
void ClientCommand( int clientNum );

//
// g_active.c
//
void G_CheckClientTimeouts	( gentity_t *ent );
void ClientThink			( int clientNum );
void ClientThink_real		( gentity_t *ent );
void ClientEndFrame			( gentity_t *ent );
void G_RunClient			( gentity_t *ent );
void G_Respawn				( gentity_t *ent );
void G_UpdateClientReadyFlags( void );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );

//
// g_mem.c
//
void *G_Alloc( int size );
void G_InitMemory( void );
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot, qboolean firstTime );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_arenas.c
//
void UpdateTournamentInfo( void );
//void SpawnModelsOnVictoryPads( void );
//void Svcmd_AbortPodium_f( void );

//
// g_bot.c
//
typedef int arena_t;
#define ARENA_INVALID (-1)

void G_InitBots( int restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_RemoveQueuedBotBegin( int clientNum );
qboolean G_BotConnect( int clientNum, qboolean restart );
void Svcmd_AddBot_f( void );
void Svcmd_BotList_f( void );
void BotInterbreedEndMatch( void );
qboolean G_DoesMapSupportGametype(const char *mapname, gametype_t gametype);
qboolean G_DoesArenaSupportGametype(arena_t arena, gametype_t gametype);
const char *G_RefreshNextMap(int gametype, qboolean forced);
arena_t G_GetArenaByMap( const char *map );
const char *G_GetArenaInfo( arena_t arena );


// w_force.c / w_saber.c
gentity_t *G_PreDefSound(vec3_t org, pdSounds_t pdSound, int blameEntityNum);
qboolean HasSetSaberOnly(void);
void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
void WP_SaberPositionUpdate( gentity_t *self, usercmd_t *ucmd );
int WP_SaberCanBlock(gentity_t *self, vec3_t point, int dflags, qboolean projectile, int attackStr);
void WP_SaberInitBladeData( gentity_t *ent );
void WP_InitForcePowers( gentity_t *ent );
void WP_SpawnInitForcePowers( gentity_t *ent );
void WP_ForcePowersUpdate( gentity_t *self, usercmd_t *ucmd );
int ForcePowerUsableOn(gentity_t *attacker, gentity_t *other, forcePowers_t forcePower);
void ForceHeal( gentity_t *self );
void ForceSpeed( gentity_t *self, int forceDuration );
void ForceRage( gentity_t *self );
void ForceGrip( gentity_t *self );
void ForceProtect( gentity_t *self );
void ForceAbsorb( gentity_t *self );
void ForceTeamHeal( gentity_t *self );
void ForceTeamForceReplenish( gentity_t *self );
void ForceSeeing( gentity_t *self );
void ForceThrow( gentity_t *self, qboolean pull );
void ForceTelepathy(gentity_t *self);
qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, hitLoc_t hitLoc );

// g_log.c
void QDECL G_LogWeaponPickup(int client, int weaponid);
void QDECL G_LogWeaponFire(int client, int weaponid);
void QDECL G_LogWeaponDamage(int client, int mod, int amount);
void QDECL G_LogWeaponKill(int client, int mod);
void QDECL G_LogWeaponDeath(int client, int weaponid);
void QDECL G_LogWeaponFrag(int attacker, int deadguy);
void QDECL G_LogWeaponPowerup(int client, int powerupid);
void QDECL G_LogWeaponItem(int client, int itemid);
void QDECL G_LogWeaponInit(void);
void QDECL G_LogWeaponOutput(void);
void QDECL G_LogExit( const char *string );
void QDECL G_ClearClientLog(int client);

// g_saga.c
void InitSagaMode(void);

// g_stats.c
void G_PrintStats(void);
void G_LogStats(void);

// g_dimensions.c
#define DEFAULT_DIMENSION	(1u << TEAM_FREE)
#define ALL_DIMENSIONS		0xffffffffu

void G_BlameForEntity( int blame, gentity_t *ent );
unsigned G_GetFreeDuelDimension(void);
unsigned G_EntitiesCollide(gentity_t *ent1, gentity_t *ent2);
void G_StartPrivateDuel(gentity_t *ent);
void G_StopPrivateDuel(gentity_t *ent);
void G_UpdateCollisionMap(void);

extern qboolean (*G_Collide) (const gentity_t *ent1, const gentity_t *ent2);
extern void (*G_Trace) (trace_t *results, const vec3_t start, const vec3_t mins,
	const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
extern int (*G_EntitiesInBox) (const vec3_t mins, const vec3_t maxs,
	int *entityList, int maxcount, int entityNum);

// g_unlagged.c
void G_BackupWorld( void );
void G_RollbackWorld( int serverTime, int contents );
void G_RestoreWorld( void );

// g_debug.c
#if !defined(NDEBUG) && !defined(Q3_VM)
void G_StaticCheck(void);
void G_EntityStateCheckRep(const entityState_t *s);
void G_ClientCheckRep(const gclient_t *cl);
void G_EntityCheckRep(const gentity_t *ent);
#else
#define G_StaticCheck()
#define G_EntityStateCheckRep(x)
#define G_ClientCheckRep(x)
#define G_EntityCheckRep(x)
#endif

// g_referee.c
typedef struct {
	void	(*Printf)(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
	void	(*LogPrintf)(int event, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
} refCmdContext_t;

extern refCmdContext_t ref;

qboolean RefereeCommand(const char *cmd);

// ai_main.c

int		OrgVisible		( vec3_t org1, vec3_t org2, int ignore);
void	BotOrder		( gentity_t *ent, int clientnum, int ordernum);
int		InFieldOfVision	( vec3_t viewangles, float fov, vec3_t angles);

// ai_util.c
void B_InitAlloc(void);
void B_CleanupAlloc(void);

//bot settings
typedef struct bot_settings_s
{
	char personalityfile[MAX_QPATH];
	float skill;
	char team[16]; // TODO: replace with enum
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownClient( int client, int restart );
int BotAIStartFrame( int time );

#include "g_team.h" // teamplay specific stuff

extern	level_locals_t	level;
extern	qboolean		g_mvapi;
extern	gentity_t		g_entities[MAX_GENTITIES];
extern	mvsharedEntity_t mv_entities[MAX_GENTITIES];

#define	FOFS(x) ((size_t)&(((gentity_t *)0)->x))

extern	vmCvar_t	g_gametype;
extern	vmCvar_t	g_dedicated;
extern	vmCvar_t	g_cheats;
extern	vmCvar_t	g_maxclients;			// allow this many total, including spectators
extern  vmCvar_t	g_teamsize;
extern  vmCvar_t    g_teamsizeMin;
extern	vmCvar_t	g_restarted;

extern	vmCvar_t	g_trueJedi;

extern	vmCvar_t	g_autoMapCycle;
extern	vmCvar_t	g_dmflags;
extern	vmCvar_t	g_maxForceRank;
extern	vmCvar_t	g_forceBasedTeams;
extern	vmCvar_t	g_privateDuel;
extern	vmCvar_t	g_saberLocking;
extern	vmCvar_t	g_saberLockFactor;
extern	vmCvar_t	g_saberTraceSaberFirst;

#ifdef G2_COLLISION_ENABLED
extern	vmCvar_t	g_saberGhoul2Collision;
#endif
extern	vmCvar_t	g_saberAlwaysBoxTrace;
extern	vmCvar_t	g_saberBoxTraceSize;

extern	vmCvar_t	g_slowmoDuelEnd;

extern	vmCvar_t	g_saberDamageScale;

extern	vmCvar_t	g_useWhileThrowing;

extern	vmCvar_t	g_forceRegenTime;
extern	vmCvar_t	g_spawnInvulnerability;
extern	vmCvar_t	g_forcePowerDisable;
extern	vmCvar_t	g_weaponDisable;

extern	vmCvar_t	g_allowDuelSuicide;
extern	vmCvar_t	g_fraglimitVoteCorrection;

extern	vmCvar_t	g_duelWeaponDisable;
extern	vmCvar_t	g_fraglimit;
extern	vmCvar_t	g_timelimit;
extern	vmCvar_t	g_capturelimit;
extern	vmCvar_t	g_saberInterpolate;
extern	vmCvar_t	g_friendlyFire;
extern	vmCvar_t	g_friendlySaber;
extern	vmCvar_t	g_password;
extern	vmCvar_t	g_needpass;
extern	vmCvar_t	g_gravity;
extern	vmCvar_t	g_speed;
extern	vmCvar_t	g_knockback;
extern	vmCvar_t	g_quadfactor;
extern	vmCvar_t	g_forcerespawn;
extern	vmCvar_t	g_inactivity;
extern	vmCvar_t	g_debugMove;
extern	vmCvar_t	g_debugAlloc;
extern	vmCvar_t	g_debugDamage;
extern	vmCvar_t	g_weaponRespawn;
extern	vmCvar_t	g_weaponTeamRespawn;
extern	vmCvar_t	g_adaptRespawn;
extern	vmCvar_t	g_synchronousClients;
extern	vmCvar_t	g_motd;
extern	vmCvar_t	g_warmup;
extern	vmCvar_t	g_doWarmup;
extern	vmCvar_t	g_blood;
extern	vmCvar_t	g_allowVote;
extern	vmCvar_t	g_teamAutoJoin;
extern	vmCvar_t	g_teamForceBalance;
extern	vmCvar_t	g_banIPs;
extern	vmCvar_t	g_filterBan;
extern	vmCvar_t	g_debugForward;
extern	vmCvar_t	g_debugRight;
extern	vmCvar_t	g_debugUp;
extern	vmCvar_t	g_redteam;
extern	vmCvar_t	g_blueteam;
extern	vmCvar_t	g_smoothClients;
extern	vmCvar_t	g_pmove_fixed;
extern	vmCvar_t	g_pmove_msec;
extern	vmCvar_t	g_singlePlayer;
extern	vmCvar_t	g_dismember;
extern	vmCvar_t	g_forceDodge;
extern	vmCvar_t	g_timeouttospec;

extern	vmCvar_t	g_saberDmgVelocityScale;
extern	vmCvar_t	g_saberDmgDelay_Idle;
extern	vmCvar_t	g_saberDmgDelay_Wound;

extern	vmCvar_t	g_saberDebugPrint;

extern	vmCvar_t	g_checkSpawnEntities;

extern  vmCvar_t	g_damagePlums;
extern	vmCvar_t	g_mode;
extern	vmCvar_t	g_modeIdleTime;
extern	vmCvar_t	g_modeDefault;
extern	vmCvar_t	g_modeDefaultMap;
extern  vmCvar_t	g_restrictChat;
extern  vmCvar_t	g_restrictSpectator;
extern	vmCvar_t	g_spawnItems;
extern  vmCvar_t	g_spawnShield;
extern	vmCvar_t	g_spawnWeapons;
extern  vmCvar_t	g_kickMethod;
extern	vmCvar_t	g_infiniteAmmo;
extern	vmCvar_t	g_instagib;
extern	vmCvar_t	g_voteCooldown;
extern	vmCvar_t	g_unlagged;
extern	vmCvar_t	g_unlaggedMaxPing;
extern	vmCvar_t	g_timeoutDuration;
extern	vmCvar_t	g_timeoutLimit;
extern	vmCvar_t	g_requireClientside;
extern	vmCvar_t	g_allowRefVote;
extern	vmCvar_t	g_antiWarp;
extern	vmCvar_t	g_antiWarpTime;
extern	vmCvar_t	g_spSkill;
extern	vmCvar_t	g_pushableItems;
extern	vmCvar_t	g_refereePassword;
extern	vmCvar_t	g_allowTeamVote;
extern	vmCvar_t	g_vampiricDamage;
extern	vmCvar_t	g_removeUnreachableItems;

void	trap_Print( const char *fmt );
Q_NORETURN void	trap_Error( const char *fmt );
int		trap_Milliseconds( void );
int		trap_Argc( void );
void	trap_Argv( int n, char *buffer, int bufferLength );
void	trap_Args( char *buffer, int bufferLength );
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );
int		trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void	trap_SendConsoleCommand( cbufExec_t exec_when, const char *text );
void	trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void	trap_Cvar_Update( vmCvar_t *cvar );
void	trap_Cvar_Set( const char *var_name, const char *value );
int		trap_Cvar_VariableIntegerValue( const char *var_name );
float	trap_Cvar_VariableValue( const char *var_name );
void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void	trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient );
void	trap_DropClient( int clientNum, const char *reason );
void	trap_SendServerCommand( int clientNum, const char *text );
void	trap_SetConfigstring( int num, const char *string );
void	trap_GetConfigstring( int num, char *buffer, int bufferSize );
void	trap_GetUserinfo( int num, char *buffer, int bufferSize );
void	trap_SetUserinfo( int num, const char *buffer );
void	trap_GetServerinfo( char *buffer, int bufferSize );
void	trap_SetBrushModel( gentity_t *ent, const char *name );
void	trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int		trap_PointContents( const vec3_t point, int passEntityNum );
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 );
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void	trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
qboolean trap_AreasConnected( int area1, int area2 );
void	trap_LinkEntity( gentity_t *ent );
void	trap_UnlinkEntity( gentity_t *ent );
int		trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
int		trap_BotAllocateClient( void );
void	trap_BotFreeClient( int clientNum );
void	trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

#ifdef BOT_ZMALLOC
void	*trap_BotGetMemoryGame(int size);
void	trap_BotFreeMemoryGame(void *ptr);
#endif

int		trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void	trap_DebugPolygonDelete(int id);

int		trap_BotLibSetup( void );
int		trap_BotLibShutdown( void );
int		trap_BotLibVarSet(char *var_name, char *value);
int		trap_BotLibVarGet(char *var_name, char *value, int size);
int		trap_BotLibDefine(char *string);
int		trap_BotLibStartFrame(float time);
int		trap_BotLibLoadMap(const char *mapname);
int		trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int		trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int		trap_BotGetSnapshotEntity( int clientNum, int sequence );
int		trap_BotGetServerCommand(int clientNum, char *message, int size);
void	trap_BotUserCommand(int clientNum, usercmd_t *ucmd);

int		trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int		trap_AAS_AreaInfo( int areanum, void /* struct aas_areainfo_s */ *info );
void	trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int		trap_AAS_Initialized(void);
void	trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float	trap_AAS_Time(void);

int		trap_AAS_PointAreaNum(vec3_t point);
int		trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int		trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int		trap_AAS_PointContents(vec3_t point);
int		trap_AAS_NextBSPEntity(int ent);
int		trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int		trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int		trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int		trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int		trap_AAS_AreaReachability(int areanum);

int		trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int		trap_AAS_EnableRoutingArea( int areanum, int enable );
int		trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);

int		trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
										int type);
int		trap_AAS_Swimming(vec3_t origin);
int		trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);


void	trap_EA_Say(int client, const char *str);
void	trap_EA_SayTeam(int client, const char *str);
void	trap_EA_Command(int client, const char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, vec3_t dir, float speed);
void	trap_EA_View(int client, vec3_t viewangles);
void	trap_EA_Alt_Attack(int client);
void	trap_EA_ForcePower(int client);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);


int		trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min, float max);
int		trap_Characteristic_Integer(int character, int index);
int		trap_Characteristic_BInteger(int character, int index, int min, int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int		trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int		trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int		trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotNumInitialChats(int chatstate, char *type);
int		trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int		trap_StringContains(char *str1, char *str2, int casesensitive);
int		trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, unsigned long int context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, unsigned long int context);
int		trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void	trap_BotSetChatGender(int chatstate, gender_t gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int		trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int		trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int		trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int		trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int		trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int		trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int		trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int		trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int		trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int		trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int		trap_BotReachabilityArea(vec3_t origin, int testground);
int		trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int		trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int		trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int		trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo);
int		trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int		trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int		trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

int		trap_RealTime( qtime_t *qtime );
void	trap_SnapVector( float *v );

qboolean trap_SP_RegisterServer( const char *package );
int trap_SP_GetStringTextString(const char *text, char *buffer, int bufferLength);

qboolean	trap_ROFF_Clean( void );
void		trap_ROFF_UpdateEntities( void );
int			trap_ROFF_Cache( char *file );
qboolean	trap_ROFF_Play( int entID, int roffID, qboolean doTranslation );
qboolean	trap_ROFF_Purge_Ent( int entID );

qboolean	trap_MVAPI_SendConnectionlessPacket(const mvaddr_t *addr, const char *message);
qboolean	trap_MVAPI_LocateGameData(mvsharedEntity_t *mvEnts, int numGEntities, int sizeofmvsharedEntity_t);
qboolean	trap_MVAPI_GetConnectionlessPacket(mvaddr_t *addr, char *buf, unsigned int bufsize);
qboolean	trap_MVAPI_ControlFixes(int fixes);
