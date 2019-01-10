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

#include "g_local.h"
#include "bg_version.h"

level_locals_t	level;
qboolean		g_mvapi;

typedef struct {
	vmCvar_t	*vmCvar;
	const char	*cvarName;
	const char	*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
	qboolean	teamShader;        // track and if changed, update shader state
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];
mvsharedEntity_t mv_entities[MAX_GENTITIES];

vmCvar_t	g_trueJedi;

vmCvar_t	g_gametype;
vmCvar_t	g_MaxHolocronCarry;
vmCvar_t	g_ff_objectives;
vmCvar_t	g_autoMapCycle;
vmCvar_t	g_dmflags;
vmCvar_t	g_maxForceRank;
vmCvar_t	g_forceBasedTeams;
vmCvar_t	g_privateDuel;
vmCvar_t	g_saberLocking;
vmCvar_t	g_saberLockFactor;
vmCvar_t	g_saberTraceSaberFirst;

#ifdef G2_COLLISION_ENABLED
vmCvar_t	g_saberGhoul2Collision;
#endif
vmCvar_t	g_saberAlwaysBoxTrace;
vmCvar_t	g_saberBoxTraceSize;

vmCvar_t	g_slowmoDuelEnd;

vmCvar_t	g_saberDamageScale;

vmCvar_t	g_useWhileThrowing;

vmCvar_t	g_forceRegenTime;
vmCvar_t	g_spawnInvulnerability;
vmCvar_t	g_forcePowerDisable;
vmCvar_t	g_weaponDisable;
vmCvar_t	g_duelWeaponDisable;
vmCvar_t	g_allowDuelSuicide;
vmCvar_t	g_fraglimitVoteCorrection;
vmCvar_t	g_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_saberInterpolate;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_friendlySaber;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_teamsize;
vmCvar_t    g_teamsizeMin;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_forcerespawn;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_adaptRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_synchronousClients;
vmCvar_t	g_warmup;
vmCvar_t	g_doWarmup;
vmCvar_t	g_restarted;
vmCvar_t	g_log[MAX_LOGFILES]; // Don't forget to update gameCvarTable
vmCvar_t	g_logFilter[MAX_LOGFILES];
vmCvar_t	g_consoleFilter;
vmCvar_t	g_logSync;
vmCvar_t	g_statLog;
vmCvar_t	g_statLogFile;
vmCvar_t	g_blood;
vmCvar_t	g_podiumDist;
vmCvar_t	g_podiumDrop;
vmCvar_t	g_allowVote;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_debugForward;
vmCvar_t	g_debugRight;
vmCvar_t	g_debugUp;
vmCvar_t	g_smoothClients;
vmCvar_t	g_pmove_fixed;
vmCvar_t	g_pmove_msec;
vmCvar_t	g_listEntity;
vmCvar_t	g_redteam;
vmCvar_t	g_blueteam;
vmCvar_t	g_singlePlayer;
vmCvar_t	g_dismember;
vmCvar_t	g_forceDodge;
vmCvar_t	g_timeouttospec;

vmCvar_t	g_saberDmgVelocityScale;
vmCvar_t	g_saberDmgDelay_Idle;
vmCvar_t	g_saberDmgDelay_Wound;

vmCvar_t	g_saberDebugPrint;

vmCvar_t	g_checkSpawnEntities;

vmCvar_t    g_damagePlums;
vmCvar_t	g_mode;
vmCvar_t	g_modeIdleTime;
vmCvar_t	g_modeDefault;
vmCvar_t	g_modeDefaultMap;
vmCvar_t	g_restrictChat;
vmCvar_t	g_restrictSpectator;
vmCvar_t	g_spawnItems;
vmCvar_t	g_spawnShield;
vmCvar_t	g_spawnWeapons;
vmCvar_t	g_roundlimit;
vmCvar_t	g_roundWarmup;
vmCvar_t	g_kickMethod;
vmCvar_t	g_infiniteAmmo;
vmCvar_t	g_instagib;
vmCvar_t	g_voteCooldown;
vmCvar_t	g_unlagged;
vmCvar_t	g_unlaggedMaxPing;
vmCvar_t	g_ingameMotd;
vmCvar_t	g_macroscan;
vmCvar_t	g_timeoutLimit;
vmCvar_t	g_requireClientside;
vmCvar_t	g_allowRefVote;
vmCvar_t	g_antiWarp;
vmCvar_t	g_antiWarpTime;
vmCvar_t	g_spSkill;
vmCvar_t	g_pushableItems;
vmCvar_t	g_refereePassword;


// bk001129 - made static to avoid aliasing
static cvarTable_t gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ NULL, "gamename", GAME_VERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_MaxHolocronCarry, "g_MaxHolocronCarry", "3", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },

	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_teamsize, "teamsize", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue  },
	{ &g_teamsizeMin, "g_teamsizeMin", "2", CVAR_ARCHIVE , 0, qfalse  },

	// change anytime vars
	{ &g_ff_objectives, "g_ff_objectives", "0", /*CVAR_SERVERINFO |*/  CVAR_NORESTART, 0, qtrue },

	{ &g_trueJedi, "g_jediVmerc", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qtrue },

	{ &g_autoMapCycle, "g_autoMapCycle", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

	{ &g_maxForceRank, "g_maxForceRank", "6", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_forceBasedTeams, "g_forceBasedTeams", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_privateDuel, "g_privateDuel", "1", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_saberLocking, "g_saberLocking", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberLockFactor, "g_saberLockFactor", "6", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberTraceSaberFirst, "g_saberTraceSaberFirst", "1", CVAR_ARCHIVE, 0, qtrue  },

#ifdef G2_COLLISION_ENABLED
	{ &g_saberGhoul2Collision, "g_saberGhoul2Collision", "0", 0, 0, qtrue  },
#endif
	{ &g_saberAlwaysBoxTrace, "g_saberAlwaysBoxTrace", "0", 0, 0, qtrue  },
	{ &g_saberBoxTraceSize, "g_saberBoxTraceSize", "2", 0, 0, qtrue  },

	{ &g_slowmoDuelEnd, "g_slowmoDuelEnd", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_saberDamageScale, "g_saberDamageScale", "1", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_useWhileThrowing, "g_useWhileThrowing", "1", 0, 0, qtrue  },

	{ &g_forceRegenTime, "g_forceRegenTime", "200", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },

	{ &g_spawnInvulnerability, "g_spawnInvulnerability", "3000", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_forcePowerDisable, "g_forcePowerDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_weaponDisable, "g_weaponDisable", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },
	{ &g_duelWeaponDisable, "g_duelWeaponDisable", "1", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH, 0, qtrue  },

	{ &g_allowDuelSuicide, "g_allowDuelSuicide", "0", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimitVoteCorrection, "g_fraglimitVoteCorrection", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

	{ &g_saberInterpolate, "g_saberInterpolate", "1", CVAR_ARCHIVE, 0, qtrue },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_friendlySaber, "g_friendlySaber", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmup, "g_warmup", "5", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_doWarmup, "g_doWarmup", "1", 0, 0, qtrue  },

	{ &g_log[0], "g_log1", "games.log", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_log[1], "g_log2", "", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_log[2], "g_log3", "", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_log[3], "g_log4", "", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse  },
	{ &g_logFilter[0], "g_logFilter1", "173015", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logFilter[1], "g_logFilter2", "", CVAR_ARCHIVE, 0, qfalse },
	{ &g_logFilter[2], "g_logFilter3", "", CVAR_ARCHIVE, 0, qfalse },
	{ &g_logFilter[3], "g_logFilter4", "", CVAR_ARCHIVE, 0, qfalse },
	{ &g_consoleFilter, "g_consoleFilter", "41943", CVAR_ARCHIVE, 0, qfalse },

	{ &g_logSync, "g_logSync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_statLog, "g_statLog", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_statLogFile, "g_statLogFile", "statlog.log", CVAR_ARCHIVE, 0, qfalse },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "250", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue },
	{ &g_adaptRespawn, "g_adaptrespawn", "1", 0, 0, qtrue  },		// Make weapons respawn faster with a lot of players.
	{ &g_forcerespawn, "g_forcerespawn", "60", 0, 0, qtrue },		// One minute force respawn.  Give a player enough time to reallocate force.
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },
//	{ &g_blood, "com_blood", "1", 0, 0, qfalse },

//	{ &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
//	{ &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

#if 0
	{ &g_debugForward, "g_debugForward", "0", 0, 0, qfalse },
	{ &g_debugRight, "g_debugRight", "0", 0, 0, qfalse },
	{ &g_debugUp, "g_debugUp", "0", 0, 0, qfalse },
#endif

	{ &g_redteam, "g_redteam", "Empire", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue },
	{ &g_blueteam, "g_blueteam", "Rebellion", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO , 0, qtrue, qtrue  },
	{ &g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse, qfalse  },

	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &g_pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
	{ &g_pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

	{ &g_dismember, "g_dismember", "100", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_forceDodge, "g_forceDodge", "1", 0, 0, qtrue  },

	{ &g_timeouttospec, "g_timeouttospec", "70", CVAR_ARCHIVE, 0, qfalse },

	{ &g_saberDmgVelocityScale, "g_saberDmgVelocityScale", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Idle, "g_saberDmgDelay_Idle", "350", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_saberDmgDelay_Wound, "g_saberDmgDelay_Wound", "0", CVAR_ARCHIVE, 0, qtrue  },

	{ &g_saberDebugPrint, "g_saberDebugPrint", "0", CVAR_CHEAT, 0, qfalse  },

	{ &g_checkSpawnEntities, "g_checkSpawnEntities", "0", CVAR_TEMP, 0, qfalse  },

	{ &g_damagePlums, "g_damagePlums", "1", CVAR_ARCHIVE , 0, qtrue  },
	{ &g_modeIdleTime, "g_modeIdleTime", "0", CVAR_ARCHIVE , 0, qfalse  },
	{ &g_modeDefault, "g_modeDefault", "", CVAR_ARCHIVE , 0, qfalse  },
	{ &g_modeDefaultMap, "g_modeDefaultMap", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_restrictChat, "g_restrictChat", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_restrictSpectator, "g_restrictSpectator", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_spawnItems, "g_spawnItems", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_spawnShield, "g_spawnShield", "25", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_spawnWeapons, "g_spawnWeapons", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_roundlimit, "roundlimit", "5", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_roundWarmup, "g_roundWarmup", "10", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_kickMethod, "g_kickMethod", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_infiniteAmmo, "g_infiniteAmmo", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_instagib, "g_instagib", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_voteCooldown, "g_voteCooldown", "3", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_unlagged, "g_unlagged", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_unlaggedMaxPing, "g_unlaggedMaxPing", "200", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_ingameMotd, "g_ingameMotd", "none", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_macroscan, "g_macroscan", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue },
	{ &g_timeoutLimit, "g_timeoutLimit", "2", CVAR_ARCHIVE, 0, qfalse },
	{ &g_requireClientside, "g_requireClientside", "0", CVAR_ARCHIVE, 0, qtrue },
	{ &g_allowRefVote, "g_allowRefVote", "-1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_antiWarp, "g_antiWarp", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_antiWarpTime, "g_antiWarpTime", "1000", CVAR_ARCHIVE, 0, qtrue },
	{ &g_spSkill, "g_spSkill", "2", CVAR_ARCHIVE, 0, qfalse },
	{ &g_pushableItems, "g_pushableItems", "0", CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_refereePassword, "g_refereePassword", "", CVAR_ARCHIVE, 0, qfalse },
};

void G_InitGame					( int levelTime, int randomSeed, int restart );
int  MVAPI_Init					( int apilevel );
void MVAPI_AfterInit			( int levelTime, int randomSeed, int restart );
void G_RunFrame					( int levelTime );
void G_ShutdownGame				( int restart );
void CheckExitRules				( void );
void G_ROFF_NotetrackCallback	( gentity_t *cent, const char *notetrack);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
	static int	initArgs[3];
	int			requestedMVAPI;


	switch ( command ) {
	case GAME_INIT:
		requestedMVAPI = MVAPI_Init(arg11);
		if (requestedMVAPI) {
			initArgs[0] = arg0;
			initArgs[1] = arg1;
			initArgs[2] = arg2;
		} else {
			G_InitGame( arg0, arg1, arg2 );
		}
		return requestedMVAPI;
	case MVAPI_AFTER_INIT:
		MVAPI_AfterInit(initArgs[0], initArgs[1], initArgs[2]);
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, (qboolean)arg1, (qboolean)arg2 );
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );
	case GAME_ROFF_NOTETRACK_CALLBACK:
		G_ROFF_NotetrackCallback( &g_entities[arg0], (const char *)arg1 );
	}

	return -1;
}

/*
=================
MVAPI_Init
=================
*/
int MVAPI_Init( int apilevel )
{
	if (!trap_Cvar_VariableIntegerValue("mv_apienabled")) {
		G_Printf("MVAPI is not supported at all or has been disabled.\n");
		G_Printf("You need at least JK2MV " MV_MIN_VERSION ".\n");
		return 0;
	}

	if (apilevel < MV_APILEVEL) {
		G_Printf("MVAPI level %i not supported.\n", MV_APILEVEL);
		G_Printf("You need at least JK2MV " MV_MIN_VERSION ".\n");
		return 0;
	}

	g_mvapi = qtrue;

	G_Printf("Using MVAPI level %i (%i supported).\n", MV_APILEVEL, apilevel);
	return MV_APILEVEL;
}

/*
=================
MVAPI_AfterInit
=================
*/
void MVAPI_AfterInit( int levelTime, int randomSeed, int restart )
{
	// disable jk2mv fixes
	trap_MVAPI_ControlFixes( MVFIX_CHARGEJUMP | MVFIX_SPEEDHACK | MVFIX_SABERSTEALING );

	memset(mv_entities, 0, sizeof(mv_entities));

	G_InitGame(levelTime, randomSeed, restart);

	trap_MVAPI_LocateGameData(mv_entities, level.num_entities, sizeof(mvsharedEntity_t));
}

void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Print( text );
}

Q_NORETURN void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

void G_SendServerCommand( int clientNum, const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsnprintf(text, sizeof(text), fmt, argptr);
	va_end(argptr);

	trap_SendServerCommand(clientNum, text);
}

/*
================
G_GametypeForString

Returns GT_MAX_GAME_TYPE for unrecognized gametype
================
*/
gametype_t G_GametypeForString( const char *s ) {
	int			i;

	if ( Q_IsInteger( s ) )
	{
		i = atoi( s );
	}
	else
	{
		for (i = 0; i < GT_MAX_GAME_TYPE; i++) {
			if ( !Q_stricmp( s, gametypeLong[i] ) )
				break;
			if ( !Q_stricmp( s, gametypeShort[i] ) )
				break;
		}
	}

	if ( GT_Valid( i ) )
		return (gametype_t) i;
	else
		return GT_MAX_GAME_TYPE;
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=MAX_CLIENTS, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

	G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders( void ) {
#if 0
	char string[1024];
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, level.time);
	AddRemap("textures/ctf2/redteam02", string, level.time);
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, level.time);
	AddRemap("textures/ctf2/blueteam02", string, level.time);
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
#endif
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	const cvarTable_t *end = gameCvarTable + ARRAY_LEN( gameCvarTable );
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	for ( cv = gameCvarTable ; cv < end ; cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	// initialize with g_defaultMode string. Main server config should
	// always execute default mode cfg
	trap_Cvar_Register( &g_mode, "g_mode", g_modeDefault.string, CVAR_ROM );

	trap_Cvar_Set( "gamename", GAME_VERSION );
	trap_Cvar_Set( "gamedate", __DATE__ );

	if (remapped) {
		G_RemapTeamShaders();
	}

	level.warmupModificationCount = g_warmup.modificationCount;
	level.dmflagsModificationCount = g_dmflags.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	const cvarTable_t *end = gameCvarTable + ARRAY_LEN( gameCvarTable );
	cvarTable_t *cv;
	qboolean remapped = qfalse;

	// don't announce changed cvars
	if ( level.restarting )
		return;

	for ( cv = gameCvarTable ; cv < end ; cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"",
						cv->cvarName, cv->vmCvar->string ) );
				}

				if (cv->teamShader) {
					remapped = qtrue;
				}
			}
		}
	}

	if (level.dmflagsModificationCount != g_dmflags.modificationCount) {
		level.dmflagsModificationCount = g_dmflags.modificationCount;;
		G_UpdateCollisionMap();
	}

	weaponData[WP_DISRUPTOR].fireTime = (g_dmflags.integer & DF_CJK) ? 1000 : 600;

	trap_Cvar_Update( &g_mode );

	if (remapped) {
		G_RemapTeamShaders();
	}
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	char	serverinfo[MAX_INFO_STRING];
	int		i;

	G_StaticCheck();

	B_InitAlloc(); //make sure everything is clean

	G_Printf ("------- Game Initialization -------\n");
	G_Printf ("gamename: %s\n", GAMEVERSION);
	G_Printf ("gamedate: %s\n", __DATE__);

	id_srand( randomSeed );
	srand( randomSeed );

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	level.time = levelTime;
	level.startTime = levelTime;
	level.idleTime = levelTime;
	level.snapnum = 1;
	level.duelist1 = -1;
	level.duelist2 = -1;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	// set gametype
	level.gametype = G_GametypeForString( g_gametype.string );
	if ( level.gametype == GT_MAX_GAME_TYPE ) {
		G_Printf( "Unrecognized g_gametype %s, defaulting to %s\n", g_gametype.string, gametypeShort[GT_FFA] );
		level.gametype = GT_FFA;
	}
	// set numeric value for engine hacks
	trap_Cvar_Set( "g_gametype", va( "%d", level.gametype ) );

	G_UpdateCollisionMap();

	//trap_SP_RegisterServer("mp_svgame");

	for ( i = 0; i < MAX_LOGFILES; i++ ) {
		if ( g_log[i].string[0] ) {
			if ( g_logSync.integer ) {
				trap_FS_FOpenFile( g_log[i].string, &level.logFile[i], FS_APPEND_SYNC );
			} else {
				trap_FS_FOpenFile( g_log[i].string, &level.logFile[i], FS_APPEND );
			}
			if ( !level.logFile[i] ) {
				G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log[i].string );
			}
		}
	}

	trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

	G_LogPrintf( LOG_GAME, "------------------------------------------------------------\n" );
	G_LogPrintf( LOG_GAME, "InitGame: %s\n", serverinfo );

	G_LogWeaponInit();

	G_InitWorldSession();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = CLAMP( 0, MAX_CLIENTS, g_maxclients.integer );
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	for ( i=0 ; i<level.maxclients ; i++ ) {
		// set client fields on player ents
		g_entities[i].client = level.clients + i;
		// Initialize sortedClients
		level.sortedClients[i] = i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// initialize saga mode before spawning entities so we know
	// if we should remove any saga-related entities on spawn
	InitSagaMode();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if( GT_Team(level.gametype) ) {
		G_CheckTeamItems();
	}
	else if ( level.gametype == GT_JEDIMASTER )
	{
		trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );
	}

	trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("-1|-1") );
	trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("-1") );
	level.duelist1 = -1;
	level.duelist2 = -1;

	SaveRegisteredItems();

	G_Printf ("-----------------------------------\n");

	if( level.gametype == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	if ( level.gametype == GT_TOURNAMENT )
	{
		G_LogPrintf( LOG_GAME, "Duel Tournament Begun: kill limit %d, win limit: %d\n",
			g_fraglimit.integer, g_roundlimit.integer );
	}
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	int	i;

	G_Printf ("==== ShutdownGame ====\n");

	G_LogWeaponOutput();

	G_LogPrintf( LOG_GAME, "ShutdownGame:\n" );
	G_LogPrintf( LOG_GAME, "------------------------------------------------------------\n" );

	for ( i = 0; i < MAX_LOGFILES; i++ ) {
		if ( level.logFile[i] ) {
			trap_FS_FCloseFile( level.logFile[i] );
		}
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap_ROFF_Clean();

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc
}



//===================================================================

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link

Q_NORETURN void QDECL Com_Error ( errorParm_t level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

#endif

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
//	if ( level.intermissiontime ) {
//		return;
//	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], TEAM_FREE );
}

/*
=======================
AddTournamentQueue

Add client to end of tournament queue
=======================
*/

void AddTournamentQueue(gclient_t *client)
{
	int index;
	gclient_t *curclient;

	for(index = 0; index < level.maxclients; index++)
	{
		curclient = &level.clients[index];

		if(curclient->pers.connected != CON_DISCONNECTED)
		{
			if(curclient == client)
				curclient->sess.spectatorNum = 0;
			else if(curclient->sess.sessionTeam == TEAM_SPECTATOR)
				curclient->sess.spectatorNum++;
		}
	}
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], TEAM_SPECTATOR );
}

void RemoveDuelDrawLoser(void)
{
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[ level.sortedClients[0] ].pers.connected != CON_CONNECTED )
	{
		return;
	}
	if ( level.clients[ level.sortedClients[1] ].pers.connected != CON_CONNECTED )
	{
		return;
	}

	clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
	clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];

	if (clFirst > clSec)
	{
		clFailure = 1;
	}
	else if (clSec > clFirst)
	{
		clFailure = 0;
	}
	else
	{
		clFailure = 2;
	}

	if (clFailure != 2)
	{
		SetTeam( &g_entities[ level.sortedClients[clFailure] ], TEAM_SPECTATOR );
	}
	else
	{ //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[ level.sortedClients[1] ], TEAM_SPECTATOR );
	}
}

#ifdef UNUSED
/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[ clientNum ], TEAM_SPECTATOR );
}
#endif

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	if (level.clients[level.sortedClients[0]].pers.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].pers.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
	{
		int clFirst = level.clients[ level.sortedClients[0] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[0] ].ps.stats[STAT_ARMOR];
		int clSec = level.clients[ level.sortedClients[1] ].ps.stats[STAT_HEALTH] + level.clients[ level.sortedClients[1] ].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if (clFirst > clSec)
		{
			clFailure = 1;
			clSuccess = 0;
		}
		else if (clSec > clFirst)
		{
			clFailure = 0;
			clSuccess = 1;
		}
		else
		{
			clFailure = 2;
			clSuccess = 2;
		}

		if (clFailure != 2)
		{
			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUpdateConfigString( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUpdateConfigString( clientNum );
		}
		else
		{
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[ clientNum ].sess.wins++;
			ClientUpdateConfigString( clientNum );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[ clientNum ].sess.losses++;
			ClientUpdateConfigString( clientNum );
		}
	}
	else
	{
		clientNum = level.sortedClients[0];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.wins++;
			ClientUpdateConfigString( clientNum );

			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED ) {
			level.clients[ clientNum ].sess.losses++;
			ClientUpdateConfigString( clientNum );
		}
	}
}

typedef int icmp_t(const int *a, const int *b);

/*
============
isort

Insertion sort for int arrays. It's a stable sorting algorithm; i.e.,
it doesn't change the order of records with equal keys. Use for small
or nearly sorted data.
============
*/
static void isort( int *a, int n, icmp_t cmp )
{
	int		i, j;
	int		temp;

	for (i = 1; i < n; i++) {
		temp = a[i];
		j = i - 1;
		while (j >= 0 && cmp(&a[j], &temp) > 0) {
			a[j + 1] = a[j];
			j--;
		}
		a[j + 1] = temp;
	}
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const int *a, const int *b ) {
	gclient_t	*ca, *cb;
	int			ta, tb;

	ca = &level.clients[*a];
	cb = &level.clients[*b];

	ta = (int)(ca->pers.connected == CON_DISCONNECTED);
	tb = (int)(cb->pers.connected == CON_DISCONNECTED);
	if (ta || tb) {
		return ta - tb;
	}
	// equivalent to:
	// if ( ta && tb ) return 0;
	// if ( ta )       return 1;
	// if ( tb )       return -1;

	// sort special clients last
	ta = (int)(ca->sess.spectatorState == SPECTATOR_SCOREBOARD);
	tb = (int)(cb->sess.spectatorState == SPECTATOR_SCOREBOARD);
	if (ta || tb) {
		return ta - tb;
	}

	// then connecting clients
	ta = (int)(ca->pers.connected == CON_CONNECTING);
	tb = (int)(cb->pers.connected == CON_CONNECTING);
	if (ta || tb) {
		return ta - tb;
	}

	// then spectators
	ta = (int)(ca->sess.sessionTeam == TEAM_SPECTATOR);
	tb = (int)(cb->sess.sessionTeam == TEAM_SPECTATOR);
	if (ta && tb) {
		ta = (int)(ca->sess.spectatorClient < 0);
		tb = (int)(cb->sess.spectatorClient < 0);

		// dedicated followers are last (GT_TOURNAMENT)
		if (ta && !tb)	return 1;
		if (!ta && tb)	return -1;

		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if (ta) {
		return 1;
	}
	if (tb) {
		return -1;
	}

	// then sort by score
	if ( ca->pers.persistant[PERS_SCORE]
		> cb->pers.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->pers.persistant[PERS_SCORE]
		< cb->pers.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	int		voteCount[VOTE_MAX] = { 0 };
	int		teamVoteCount[2][VOTE_MAX] = { { 0 } };
	// int		preNumSpec = level.numNonSpectatorClients;
	// int		nonSpecIndex = -1;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots
	level.numteamVotingClients[0] = level.numteamVotingClients[1] = 0;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
				level.numNonSpectatorClients++;
				// nonSpecIndex = i;

				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						level.numVotingClients++;
						voteCount[level.clients[i].pers.vote]++;
						if ( level.clients[i].sess.sessionTeam == TEAM_RED ) {
							level.numteamVotingClients[0]++;
							teamVoteCount[0][level.clients[i].pers.teamVote]++;
						} else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE ) {
							level.numteamVotingClients[1]++;
							teamVoteCount[1][level.clients[i].pers.teamVote]++;
						}
					}
				}
			}
		}
	}

	trap_SetConfigstring( CS_VOTE_YES, va("%i", voteCount[VOTE_YES] ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", voteCount[VOTE_NO] ) );
	level.voteYes = voteCount[VOTE_YES];
	level.voteNo = voteCount[VOTE_NO];

	for ( i = 0; i < 2; i++ ) {
		trap_SetConfigstring( CS_TEAMVOTE_YES + i, va("%i", teamVoteCount[i][VOTE_YES] ) );
		trap_SetConfigstring( CS_TEAMVOTE_NO + i, va("%i", teamVoteCount[i][VOTE_NO] ) );
		level.teamVoteYes[i] = teamVoteCount[i][VOTE_YES];
		level.teamVoteNo[i] = teamVoteCount[i][VOTE_NO];
	}

	if (!g_warmup.integer)
	{
		level.warmupTime = 0;
	}

	/*
	if (level.numNonSpectatorClients == 2 && preNumSpec < 2 && nonSpecIndex != -1 && level.gametype == GT_TOURNAMENT && !level.warmupTime)
	{
		gentity_t *currentWinner = G_GetDuelWinner(&level.clients[nonSpecIndex]);

		if (currentWinner && currentWinner->client)
		{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
			currentWinner->client->info.netname, G_GetStripEdString("SVINGAME", "VERSUS"), level.clients[nonSpecIndex].info.netname));
		}
	}
	*/
	//NOTE: for now not doing this either. May use later if appropriate.

	// Use a stable sorting algorithm to keep the previous order of
	// tied clients
	isort(level.sortedClients, level.maxclients, SortRanks);

	if (level.numPlayingClients >= 1) {
		level.follow1 = level.follow2 = level.sortedClients[0];
	}
	if (level.numPlayingClients >= 2) {
		level.follow2 = level.sortedClients[1];
	}

	// set the rank value for all clients that are connected and not spectators
	if ( GT_Team(level.gametype) && level.gametype != GT_REDROVER ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->pers.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->pers.persistant[PERS_RANK] = 0;
			} else {
				cl->pers.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->pers.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].pers.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].pers.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].pers.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( level.gametype == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].pers.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( level.gametype == GT_REDROVER ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", TeamCount( -1, TEAM_RED, qtrue ) ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", TeamCount( -1, TEAM_BLUE, qtrue ) ) );
	} else if ( GT_Team(level.gametype) ) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].pers.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].pers.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].pers.persistant[PERS_SCORE] ) );
		}

		if (level.gametype != GT_TOURNAMENT)
		{ //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 )
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, va("%i", level.sortedClients[0] ) );
			}
			else
			{
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || level.gametype == GT_TOURNAMENT ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW &&
		ent->client->sess.spectatorClient != FOLLOW_FIRST &&
		ent->client->sess.spectatorClient != FOLLOW_SECOND )
	{
		StopFollowing( ent );
	}


	// move to the spot
	VectorCopy( level.intermission_origin, ent->s.origin );
	VectorCopy( level.intermission_origin, ent->client->ps.origin );
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_INVISIBLE;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;

	ent->client->ps.fallingToDeath = qfalse;
	// G_MuteSound(ent->s.number, CHAN_VOICE);
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint ( NULL, vec3_origin, level.intermission_origin, level.intermission_angle );
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

qboolean DuelLimitHit(void);

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( level.gametype == GT_TOURNAMENT ) {
		trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

		AdjustTournamentScores();
		if (DuelLimitHit())
		{
			level.duelExit = qtrue;
		}
		else
		{
			level.duelExit = qfalse;
		}
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

#if 0
	if (g_singlePlayer.integer) {
		trap_Cvar_Set("ui_singlePlayerActive", "0");
		UpdateTournamentInfo();
	}
#endif

	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
		if (client->health <= 0) {
			respawn(client);
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}

void BeginRound( void )
{
	level.startTime = level.time;
	trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));
	trap_SetConfigstring(CS_WARMUP, "");
	trap_SendServerCommand( -1, va("cp \"%s\"", G_GetStripEdString("SVINGAME", "BEGIN_DUEL")) );
}

/*
==================
Parity

Calculates parity of 1s in binary representation of i
==================
*/
static qboolean Parity( int i )
{
	qboolean parity = qtrue;

	while (i) {
		parity = (qboolean)!parity;
		i &= i - 1;
	}

	return parity;
}

/*
==================
Shuffle

Shuffle players according to score
==================
*/
static void Shuffle( void )
{
	gentity_t	*ent;
	int			clientNum;
	team_t		newTeam;
	int			i;

	for ( i = 0 ; i < level.numNonSpectatorClients ; i++ ) {
		clientNum = level.sortedClients[i];
		ent = g_entities + clientNum;
		newTeam = Parity(i) ? TEAM_RED : TEAM_BLUE;

		ent->client->sess.sessionTeam = newTeam;
		ent->client->sess.teamLeader = qfalse;
	}

	CheckTeamLeader( TEAM_RED );
	CheckTeamLeader( TEAM_BLUE );

	for ( i = 0 ; i < level.numNonSpectatorClients ; i++ ) {
		clientNum = level.sortedClients[i];
		ent = g_entities + clientNum;

		respawn(ent);
		ClientUpdateConfigString(clientNum);
	}

	CalculateRanks();
}

static void NextRound( void )
{
	char	warmup[8];
	int		i;

	level.roundQueued = level.time + (g_roundWarmup.integer - 1) * 1000;
	// repeat the round in case of draw
	level.round = level.teamScores[TEAM_RED] + level.teamScores[TEAM_BLUE] + 1;
	trap_SetConfigstring(CS_ROUND, va("%i", level.round));
	trap_GetConfigstring(CS_WARMUP, warmup, sizeof(warmup));
	if ( warmup[0] == '\0' ) {
		trap_SetConfigstring(CS_WARMUP, va("%i", level.roundQueued));
	}

	if ( level.gametype == GT_REDROVER ) {
		Shuffle(); // calls CheckExitRules
	} else {
		gentity_t	*ent;

		for ( i = 0; i < level.maxclients; i++ ) {
			ent = g_entities + i;

			if ( !ent->inuse )
				continue;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
				continue;

			if ( ent->client->sess.spectatorState == SPECTATOR_NOT )
				respawn( ent );
			else
				SetTeam( ent, ent->client->sess.sessionTeam );
		}
	}

	// clean up dead bodies
	for ( i = 0; i < BODY_QUEUE_SIZE ; i++ ) {
		trap_UnlinkEntity( level.bodyQue[i] );
		level.bodyQue[i]->physicsObject = qfalse;
	}
}

qboolean DuelLimitHit(void)
{
	int i;
	gclient_t *cl;

	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		if ( g_roundlimit.integer && cl->sess.wins >= g_roundlimit.integer )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel (void) {
	int		i;
	gclient_t *cl;

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( level.gametype == GT_TOURNAMENT  ) {
		if (!DuelLimitHit())
		{
			if ( !level.restarted ) {
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
			return;
		}
	}

	trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	level.restarting = qtrue;
	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< level.maxclients ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->pers.persistant[PERS_SCORE] = 0;

		// Reset duel wins/losses
		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< level.maxclients ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
#define TIMESTAMP_LEN STRLEN("2016-04-13 13:37:00 ")
void G_LogPrintf( int event, const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	qtime_t		t;
	int			i;

	if ( g_dedicated.integer && (g_consoleFilter.integer & event)) {
		goto log;
	}

	for ( i = 0; i < MAX_LOGFILES; i++ ) {
		if ( level.logFile[i] && (g_logFilter[i].integer & event) ) {
			goto log;
		}
	}
	return;

 log:
	trap_RealTime( &t );

	Com_sprintf( string, sizeof(string), "%04i-%02i-%02i %02i:%02i:%02i ",
		1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec );

	va_start( argptr, fmt );
	vsnprintf( string + TIMESTAMP_LEN, sizeof(string) - TIMESTAMP_LEN, fmt, argptr );
	va_end( argptr );

	if ( g_dedicated.integer && (g_consoleFilter.integer & event)) {
		G_Printf( "%s", string + TIMESTAMP_LEN );
	}

	for ( i = 0; i < MAX_LOGFILES; i++ ) {
		if ( level.logFile[i] && (g_logFilter[i].integer & event) ) {
			trap_FS_Write( string, strlen( string ), level.logFile[i] );
		}
	}
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string ) {
	int				i;
	gclient_t		*cl;
	qboolean		won = qtrue;
	G_LogPrintf( LOG_GAME, "Exit: %s: %s\n", gametypeShort[level.gametype], string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	if ( GT_Team(level.gametype) && level.gametype != GT_REDROVER ) {
		G_LogPrintf( LOG_GAME, "Score: %i %i: %s %s\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
			BG_TeamName(TEAM_RED, CASE_NORMAL), BG_TeamName(TEAM_BLUE, CASE_NORMAL) );
	} else {
		for (i = 0; i < level.numPlayingClients; i++) {
			gclient_t	*client = level.clients + level.sortedClients[i];
			int 		rank = client->pers.persistant[PERS_RANK];

			if ((rank & ~RANK_TIED_FLAG) != 0) {
				break;
			}
			G_LogPrintf( LOG_GAME, "Winner: %i: %s\n",
				level.sortedClients[i], client->info.netname );
		}
	}

	G_LogStats();

	if (g_singlePlayer.integer) {
		if (level.gametype == GT_TOURNAMENT) {
			for (i=0 ; i < level.numPlayingClients ; i++) {
				cl = &level.clients[level.sortedClients[i]];


				if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->pers.persistant[PERS_RANK] == 0) {
					won = qfalse;
				}
			}
		} else if (GT_Flag(level.gametype)) {
			won = (qboolean)(level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]);
		}

		trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
}

static void LogRoundExit( team_t winner, const char *string )
{
	const char *param1;

	level.intermissionQueued = level.time;
	// update CS_SCORES1 and CS_SCORES2
	CalculateRanks(); // calls CheckExitRules!

	if ( winner == TEAM_SPECTATOR ) {
		param1 = "DRAW";
	} else {
		param1 = BG_TeamName( winner, CASE_UPPER );
	}

	G_LogPrintf( LOG_GAME, "RoundExit: %s: %s\n", param1, string );
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady;
	int			i;
	gclient_t	*cl;

    // see which players are ready
    ready = 0;
    notReady = 0;
    for (i=0 ; i< level.maxclients ; i++) {
        cl = level.clients + i;
        if ( cl->pers.connected != CON_CONNECTED ) {
            continue;
        }
        if ( g_entities[i].r.svFlags & SVF_BOT ) {
            continue;
        }

        if ( cl->pers.ready ) {
            ready++;
        } else {
            notReady++;
        }
    }

	if ( level.gametype == GT_TOURNAMENT && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) )
	{
		gDidDuelStuff = qtrue;

		G_LogPrintf( LOG_AUSTRIAN, "Duel Results:\n");
		//G_LogPrintf( LOG_AUSTRIAN, "Duel Time: %d\n", level.time );
		G_LogPrintf( LOG_AUSTRIAN, "winner: %s, score: %d, wins/losses: %d/%d\n",
			level.clients[level.sortedClients[0]].info.netname,
			level.clients[level.sortedClients[0]].pers.persistant[PERS_SCORE],
			level.clients[level.sortedClients[0]].sess.wins,
			level.clients[level.sortedClients[0]].sess.losses );
		G_LogPrintf( LOG_AUSTRIAN, "loser: %s, score: %d, wins/losses: %d/%d\n",
			level.clients[level.sortedClients[1]].info.netname,
			level.clients[level.sortedClients[1]].pers.persistant[PERS_SCORE],
			level.clients[level.sortedClients[1]].sess.wins,
			level.clients[level.sortedClients[1]].sess.losses );

		// if we are running a tournement map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if (!DuelLimitHit())
		{
			if (level.clients[level.sortedClients[0]].pers.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].pers.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
			{
				RemoveDuelDrawLoser();
			}
			else
			{
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			G_LogPrintf( LOG_AUSTRIAN, "Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
				level.clients[level.sortedClients[0]].info.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses,
				level.clients[level.sortedClients[1]].info.netname,
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses,
				g_fraglimit.integer );

			if (level.numPlayingClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

				level.duelist1 = level.sortedClients[0];
				level.duelist2 = level.sortedClients[1];
			}

			return;
		}

		G_LogPrintf( LOG_AUSTRIAN, "Duel Tournament Winner: %s wins/losses: %d/%d\n",
			level.clients[level.sortedClients[0]].info.netname,
			level.clients[level.sortedClients[0]].sess.wins,
			level.clients[level.sortedClients[0]].sess.losses );

		//this means we hit the duel limit so reset the wins/losses
		//but still push the loser to the back of the line, and retain the order for
		//the map change
		if (level.clients[level.sortedClients[0]].pers.persistant[PERS_SCORE] ==
			level.clients[level.sortedClients[1]].pers.persistant[PERS_SCORE] &&
			level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
			level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED)
		{
			RemoveDuelDrawLoser();
		}
		else
		{
			RemoveTournamentLoser();
		}

		AddTournamentPlayer();

		if (level.numPlayingClients >= 2)
		{
			trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			trap_SetConfigstring ( CS_CLIENT_DUELWINNER, "-1" );

			level.duelist1 = level.sortedClients[0];
			level.duelist2 = level.sortedClients[1];
		}
	}

	if (level.gametype == GT_TOURNAMENT && !level.duelExit)
	{ //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 )
		{ //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}
		return;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( GT_Team(level.gametype) ) {
		return (qboolean)(level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]);
	}

	a = level.clients[level.sortedClients[0]].pers.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].pers.persistant[PERS_SCORE];

	return (qboolean)(a == b);
}


/*
=============
GetStrongerTeam

Returns stronger team out of red and blue, or spectator on draw
=============
*/
static team_t GetRoundWinner( const char **explanation )
{
	static char	expl[128];
	team_t		winner;
	int			health[TEAM_NUM_TEAMS] = { 0 };
	int			count[TEAM_NUM_TEAMS] = { 0 };
	int			i;

	*explanation = expl;

	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t *client = &level.clients[i];

		if ( client->pers.connected == CON_CONNECTED &&
			client->sess.spectatorState == SPECTATOR_NOT &&
			client->ps.stats[STAT_HEALTH] > 0 &&
			client->ps.fallingToDeath == qfalse )
		{
			team_t team = client->sess.sessionTeam;
			count[team]++;
			health[team] += client->ps.stats[STAT_HEALTH];
			health[team] += client->ps.stats[STAT_ARMOR];
		}
	}

	winner = TEAM_SPECTATOR;

	if ( count[TEAM_RED] > count[TEAM_BLUE] ) {
		winner = TEAM_RED;
	} else if ( count[TEAM_BLUE] > count[TEAM_RED] ) {
		winner = TEAM_BLUE;
	}

	if ( winner != TEAM_SPECTATOR ) {
		if ( count[winner] == 1 ) {
			Com_sprintf( expl, sizeof( expl ), "%s%s" S_COLOR_WHITE
				" won the round (%d hp remaining)", BG_TeamColor( winner ),
				BG_TeamName( winner, CASE_NORMAL ), health[winner] );
		} else {
			Com_sprintf( expl, sizeof( expl ), "%s%s" S_COLOR_WHITE
				" won the round (%d player%s remaining)", BG_TeamColor( winner ),
				BG_TeamName( winner, CASE_NORMAL ), count[winner],
				count[winner] > 1 ? "s" : "" );
		}
		return winner;
	}

	if ( health[TEAM_RED] > health[TEAM_BLUE] ) {
		winner = TEAM_RED;
	} else if ( health[TEAM_BLUE] > health[TEAM_RED] ) {
		winner = TEAM_BLUE;
	}

	if ( winner != TEAM_SPECTATOR ) {
		Com_sprintf( expl, sizeof( expl ), "%s%s" S_COLOR_WHITE
			" won the round (%d hp remaining)", BG_TeamColor( winner ),
			BG_TeamName( winner, CASE_NORMAL ), health[winner] );
		return winner;
	}

	Q_strncpyz( expl, S_COLOR_YELLOW "Round draw" S_COLOR_WHITE, sizeof( expl ) );
	return winner;
}

/*
=================
G_QueueServerCommand

Queue server command to be sent in next snapshot
=================
*/
__attribute__ ((format (printf, 1, 2)))
static void G_QueueServerCommand( const char *fmt, ... ) {
	va_list	argptr;

	if ( level.queuedCmdSnap ) {
		// there is a queued message already, send it now
		trap_SendServerCommand( -1, level.queuedCmd );
	}

	va_start( argptr, fmt );
	vsnprintf( level.queuedCmd, sizeof( level.queuedCmd ), fmt, argptr );
	va_end( argptr );

	level.queuedCmdSnap = level.snapnum + 1;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;

	if (level.warmupTime) {
		return;
	}

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	}

	if (gDoSlowMoDuel)
	{ //don't go to intermission while in slow motion
		return;
	}

	if (gEscaping)
	{
		int i = 0;
		int numLiveClients = 0;

		while (i < MAX_CLIENTS)
		{
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0)
			{
				if (g_entities[i].client->sess.spectatorState == SPECTATOR_NOT)
				{
					numLiveClients++;
				}
			}

			i++;
		}
		if (gEscapeTime < level.time)
		{
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if (!numLiveClients)
		{
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		int time;

		if ( g_singlePlayer.integer ) {
			time = SP_INTERMISSION_DELAY_TIME;
		} else if ( GT_Round(level.gametype) ) {
			time = ROUND_INTERMISSION_DELAY_TIME;
		} else {
			time = INTERMISSION_DELAY_TIME;
		}
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			if (GT_Round(level.gametype)) {
				qboolean abort = qfalse;
				qboolean roundlimitHit = qfalse;

				if ( level.gametype == GT_REDROVER ) {
					if ( level.numPlayingClients < 2 )
						abort = qtrue;
					if ( g_roundlimit.integer > 0 && level.round >= g_roundlimit.integer ) {
						roundlimitHit = qtrue;
						trap_SendServerCommand( -1, "print \"Roundlimit hit.\n\"" );
					}
				} else {
					int	redCount = TeamCount( -1, TEAM_RED, qtrue );
					int	blueCount = TeamCount( -1, TEAM_BLUE, qtrue );

					if ( redCount == 0 || blueCount == 0 )
						abort = qtrue;
					if ( g_roundlimit.integer > 0 ) {
						if ( level.teamScores[TEAM_RED] >= g_roundlimit.integer ) {
							roundlimitHit = qtrue;
							trap_SendServerCommand( -1,	"print \"" S_COLOR_RED "Red"
								S_COLOR_WHITE " hit the round limit.\n\"" );
						} else if ( level.teamScores[TEAM_BLUE] >= g_roundlimit.integer ) {
							roundlimitHit = qtrue;
							trap_SendServerCommand( -1,	"print \"" S_COLOR_BLUE "Blue"
								S_COLOR_WHITE " hit the round limit.\n\"" );
						}
					}
				}

				if ( abort ) {
					trap_SendServerCommand( -1, "print \"Game aborted. Not enough players.\n\"" );
					LogExit("Game aborted. Not enough players.");
					G_PrintStats();
					BeginIntermission();
					return;
				} else if ( roundlimitHit ) {
					LogExit("Roundlimit hit.");
					G_PrintStats();
					BeginIntermission();
				} else {
					NextRound();
				}
			} else {
				G_PrintStats();
				BeginIntermission();
			}
		}
		return;
	}

	if ( level.roundQueued ) {
		if ( level.time - level.roundQueued >= g_roundWarmup.integer ) {
			level.roundQueued = 0;
			BeginRound();
		}
		return;
	}

	// check for sudden death
	if ( level.gametype != GT_TOURNAMENT || !g_timelimit.integer ) {
		// always wait for sudden death
		if ( !GT_Round(level.gametype) && ScoreIsTied() ) {
			return;
		}
	}

	if ( g_timelimit.integer ) {
		if ( level.time - level.startTime >= g_timelimit.integer*60000 ) {
			if ( GT_Round(level.gametype) ) {
				const char	*explanation;
				team_t		winner = GetRoundWinner( &explanation );

				AddTeamScore( level.intermission_origin, winner, 1 );
				if ( level.gametype == GT_REDROVER ) {
					trap_SendServerCommand( -1, va( "print \"%s.\n\"",
							G_GetStripEdString( "SVINGAME", "TIMELIMIT_HIT" ) ) );
				} else {
					trap_SendServerCommand( -1, va( "print \"%s. %s.\n\"",
							G_GetStripEdString( "SVINGAME", "TIMELIMIT_HIT" ), explanation ) );
				}
				LogRoundExit( winner, "Timelimit hit." );
			} else {
				trap_SendServerCommand( -1, va( "print \"%s.\n\"",
						G_GetStripEdString( "SVINGAME", "TIMELIMIT_HIT" ) ) );
				LogExit( "Timelimit hit." );
			}

			return;
		}
	}

	if ( GT_Round(level.gametype) ) {
		int redCount = TeamCount( -1, TEAM_RED, qfalse );
		int blueCount = TeamCount( -1, TEAM_BLUE, qfalse );

		// begin first round of the game
		if ( level.round == 0 ) {
			if ( redCount > 0 && blueCount > 0 )
				NextRound();
			return;
		} else if ( redCount == 0 || blueCount == 0 ) {
			const char	*explanation;
			team_t		winner = GetRoundWinner( &explanation );

			AddTeamScore( level.intermission_origin, winner, 1 );
			if ( level.gametype == GT_REDROVER ) {
				G_QueueServerCommand( "print \"Team eliminated.\n\"" );
			} else {
				G_QueueServerCommand( "print \"Team eliminated. %s.\n\"", explanation );
			}
			LogRoundExit( winner, "Team eliminated." );
			return;
		}
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( !GT_Flag(level.gametype) && !GT_Round(level.gametype) && g_fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			G_QueueServerCommand( "print \"" S_COLOR_RED "Red" S_COLOR_WHITE " %s\n\"",
				G_GetStripEdString( "SVINGAME", "HIT_THE_KILL_LIMIT" ) );
			LogExit( "Kill limit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			G_QueueServerCommand( "print \"" S_COLOR_BLUE "Blue" S_COLOR_WHITE " %s\n\"",
				G_GetStripEdString( "SVINGAME", "HIT_THE_KILL_LIMIT" ) );
			LogExit( "Kill limit hit." );
			return;
		}

		for ( i=0 ; i< level.maxclients ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( level.gametype == GT_TOURNAMENT && g_roundlimit.integer && cl->sess.wins >= g_roundlimit.integer )
			{
				LogExit( "Duel limit hit." );
				level.duelExit = qtrue;
				G_QueueServerCommand( "print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->info.netname );
				return;
			}

			if ( cl->pers.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Kill limit hit." );
				level.duelExit = qfalse;
				G_QueueServerCommand( "print \"%s" S_COLOR_WHITE " %s\n\"", cl->info.netname,
					G_GetStripEdString( "SVINGAME", "HIT_THE_KILL_LIMIT" ) );
				return;
			}
		}
	}

	if ( GT_Flag(level.gametype) && g_capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			G_QueueServerCommand( "print \"" S_COLOR_RED "Red" S_COLOR_WHITE " hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			G_QueueServerCommand( "print \"" S_COLOR_BLUE "Blue" S_COLOR_WHITE " hit the capturelimit.\n\"" );
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

static void CheckIdle( void ) {
	qboolean	defaultMode;

	defaultMode = (qboolean) !!Q_stricmp(g_mode.string, g_modeDefault.string);

	// reset to default mode if server is idle
	if ( level.numVotingClients == 0 && g_modeIdleTime.integer > 0 && defaultMode ) {
		if ( level.idleTime > 0 ) {
			if ( level.idleTime + g_modeIdleTime.integer * 60000 < level.time + 15000 ) {
				trap_SendServerCommand( -1, "print \"Server idle. Changing to default mode in 15 seconds...\n\"" );
				level.idleTime = - level.idleTime; // change sign after annoncement
			}
		} else {
			if ( - level.idleTime + g_modeIdleTime.integer * 60000 < level.time ) {
				trap_SendConsoleCommand( EXEC_APPEND, "mode default\n" );
				level.idleTime = level.time; // don't spam if it doesn't work
			}
		}
	} else {
		if ( level.idleTime < 0 ) // mode change has been announced already
			trap_SendServerCommand( -1, "print \"Mode change aborted.\n\"" );
		level.idleTime = level.time;
	}
}

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( level.gametype == GT_TOURNAMENT ) {

		// pull in a spectator if needed
		if ( level.numNonSpectatorClients < 2 ) {
			if ( !level.intermissionQueued && !level.intermissiontime )
				AddTournamentPlayer();

			if (level.numNonSpectatorClients >= 2)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				level.duelist1 = level.sortedClients[0];
				level.duelist2 = level.sortedClients[1];
			}
		}

		if (level.numPlayingClients >= 2)
		{
			if (level.duelist1 == -1 ||
				level.duelist2 == -1)
			{
				trap_SetConfigstring ( CS_CLIENT_DUELISTS, va("%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				level.duelist1 = level.sortedClients[0];
				level.duelist2 = level.sortedClients[1];

				G_LogPrintf( LOG_AUSTRIAN, "Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
					level.clients[level.sortedClients[0]].info.netname,
					level.clients[level.sortedClients[0]].sess.wins,
					level.clients[level.sortedClients[0]].sess.losses,
					level.clients[level.sortedClients[1]].info.netname,
					level.clients[level.sortedClients[1]].sess.wins,
					level.clients[level.sortedClients[1]].sess.losses,
					g_fraglimit.integer );

				//trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
			}
		}

		//rww - It seems we have decided there will be no warmup in duel.
		//if (!g_warmup.integer)
		{ //don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
#if 0
		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( LOG_GAME, "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				// fudge by -1 to account for extra delays
				level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;

				if (level.warmupTime < (level.time + 3000))
				{ //rww - this is an unpleasent hack to keep the level from resetting completely on the client (this happens when two map_restarts are issued rapidly)
					level.warmupTime = level.time + 3000;
				}
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
#endif
	} else if ( level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;
		int			ready;
		int			i;

		ready = 0;
		for ( i = 0; i < level.numPlayingClients; i++ ) {
			gentity_t *ent = &g_entities[level.sortedClients[i]];

			if (ent->r.svFlags & SVF_BOT) {
				ready++;
			} else {
				ready += ent->client->pers.ready;
			}
		}

		if ( ready < level.numPlayingClients ) {
			notEnough = qtrue;
		}

		if ( !notEnough && GT_Team(level.gametype) && level.gametype != GT_REDROVER ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qtrue );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qtrue );

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( LOG_GAME, "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

static void G_AnnouncePollResults( void ) {
	int	withdrew = level.numVotingClients - level.voteYes - level.voteNo;

	trap_SendServerCommand( -1, va( "cp \"%s\n\n"
			S_COLOR_WHITE "Yes: " S_COLOR_GREEN "%d   "
			S_COLOR_WHITE "No: "  S_COLOR_RED   "%d\n"
			S_COLOR_WHITE "Withdrew: " S_COLOR_CYAN "%d\"",
			level.voteDisplayString, level.voteYes, level.voteNo, withdrew ) );
	G_LogPrintf( LOG_VOTE, "Poll: %d %d %d: %s\n", level.voteYes, level.voteNo,
		withdrew, level.voteString );
	trap_SendServerCommand( -1, va("print \"Poll finished. Yes: %d, No: %d, Withdrew: %d\n\"",
			level.voteYes, level.voteNo, withdrew ) );
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		level.voteCooldown = level.time + g_voteCooldown.integer * 1000;

		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );

		if (level.voteCmd == CV_GAMETYPE)
		{
			if (level.gametype != (gametype_t)level.voteArg)
			{ //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap(level.voteArg, qtrue);

				if (nextMap && nextMap[0])
				{
					trap_SendConsoleCommand( EXEC_APPEND, va("map %s\n", nextMap ) );
				}

			}
			else
			{ //otherwise, just leave the map until a restart
				G_RefreshNextMap(level.voteArg, qfalse);
			}

			if (g_fraglimitVoteCorrection.integer)
			{ //This means to auto-correct fraglimit when voting to and from duel.
				int currentGT = level.gametype;
				int currentFL = g_fraglimit.integer;

				if (level.voteArg == GT_TOURNAMENT && currentGT != GT_TOURNAMENT)
				{
					if (currentFL > 3 || !currentFL)
					{ //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 3\n");
					}
				}
				else if (level.voteArg != GT_TOURNAMENT && currentGT == GT_TOURNAMENT)
				{
					if (currentFL && currentFL < 20)
					{ //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap_SendConsoleCommand(EXEC_APPEND, "fraglimit 20\n");
					}
				}
			}
		}
	}
	if ( !level.voteTime ) {
		return;
	}

	if (level.voteCmd == CV_KICK && level.clients[level.voteArg].pers.connected == CON_DISCONNECTED )
	{
		trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED")) );
	}
	else if ( level.time - level.voteTime >= VOTE_TIME )
	{
		if ( level.voteCmd == CV_POLL ) {
			G_AnnouncePollResults();
		} else {
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED")) );
		}
	}
	else if ( level.voteCmd == CV_POLL )
	{
		// assume question is formulated in such a way that owner votes "yes"
		if ( level.voteYes == 0 ) {
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED")) );
		} else if ( level.voteYes + level.voteNo == level.numVotingClients ) {
			G_AnnouncePollResults();
		} else {
			return;
		}
	}
	else
	{
		if ( level.voteReferee == VOTE_YES ) {
			trap_SendServerCommand( -1, va("print \"%s\n\"", "Vote passed by a referee decision.") );
			level.voteExecuteTime = level.time + 3000;
			G_LogPrintf( LOG_VOTE | LOG_REFEREE, "Referee: %d VotePassed: %d %d %d: %s\n",
				level.voteClient, level.voteCmd, level.voteYes, level.voteNo,
				level.voteDisplayString );
		} else if ( level.voteReferee == VOTE_NO ) {
			trap_SendServerCommand( -1, va("print \"%s\n\"", "Vote failed by a referee decision.") );
		} else if ( level.voteYes > level.numVotingClients/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEPASSED")) );
			level.voteExecuteTime = level.time + 3000;
			G_LogPrintf( LOG_VOTE, "VotePassed: %d %d %d: %s\n", level.voteCmd,
				level.voteYes, level.voteNo, level.voteDisplayString );
		} else if ( level.voteYes == 0 || level.voteNo >= level.numVotingClients/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEFAILED")) );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );

}

/*
==================
PrintTeam
==================
*/
static void PrintTeam(team_t team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(team_t team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s" S_COLOR_WHITE " is not connected\n\"", level.clients[client].info.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s" S_COLOR_WHITE " is not on the team anymore\n\"", level.clients[client].info.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUpdateConfigString(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUpdateConfigString( client );
	PrintTeam(team, va("print \"%s" S_COLOR_WHITE " %s\n\"", level.clients[client].info.netname, G_GetStripEdString("SVINGAME", "NEWTEAMLEADER")) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( team_t team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
static void CheckTeamVote( team_t team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEFAILED")) );
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEPASSED")) );
			//
			if ( !strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				int clientNum = atoi(level.teamVoteString[cs_offset] + 7);
				//set the team leader
				SetLeader(team, clientNum);
				G_LogPrintf( LOG_VOTE, "TeamVotePassed: %s %d %d %d: %s is the new %s team leader\n",
					BG_TeamName(team, CASE_UPPER), clientNum,
					level.teamVoteYes[cs_offset], level.teamVoteNo[cs_offset],
					level.clients[clientNum].info.netname, BG_TeamName(team, CASE_LOWER) );
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteYes[cs_offset] == 0 ||
			level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEFAILED")) );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );

}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int passwordMod = -1;
	static int motdMod = -1;

	if ( g_password.modificationCount != passwordMod ) {
		passwordMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}

	if ( g_ingameMotd.modificationCount != motdMod ) {
		if ( g_ingameMotd.string[0] && Q_stricmp( g_ingameMotd.string, "none" ) ) {
			char	motd[MAX_INFO_STRING];

			Q_strncpyz( motd, g_ingameMotd.string, sizeof( motd ) );
			trap_SetConfigstring( CS_INGAME_MOTD, Q_SanitizeStr( motd ) );
		} else {
			trap_SetConfigstring( CS_INGAME_MOTD, "" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	if (ent->nextthink <= 0) {
		return;
	}
	if (ent->nextthink > level.time) {
		return;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

void SlowMoDuelTimescale(void) {
	if (level.restarted)
	{
		char buf[128];
		float tFVal = 0;

		trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		trap_Cvar_Set("timescale", "1");
		if (tFVal == 1.0f)
		{
			gDoSlowMoDuel = qfalse;
		}
	}
	else
	{
		int		timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
		float	useDif = 0; //the difference to use when actually setting the timescale

		if (timeDif < 150)
		{
			trap_Cvar_Set("timescale", "0.1f");
		}
		else if (timeDif < 1150)
		{
			useDif = timeDif * 0.001f; //scale from 0.1 up to 1
			if (useDif < 0.1f)
			{
				useDif = 0.1f;
			}
			if (useDif > 1.0f)
			{
				useDif = 1.0f;
			}
			trap_Cvar_Set("timescale", va("%f", useDif));
		}
		else
		{
			char buf[128];
			float tFVal = 0;

			trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

			tFVal = atof(buf);

			trap_Cvar_Set("timescale", "1");
			if (timeDif > 1500 && tFVal == 1.0f)
			{
				gDoSlowMoDuel = qfalse;
			}
		}
	}
}

/*
================
G_RunPausedFrame
================
*/
qboolean G_RunPausedFrame( int levelTime ) {
	static int		pauseTime = 0;
	static int		unpauseTime = 0;
	static qboolean paused = qfalse;

	if (level.unpauseTime > levelTime)
	{
		if (!paused)
		{
			paused = qtrue;
			pauseTime = levelTime;
			G_Printf("Game paused\n");
		}

		if (unpauseTime != level.unpauseTime)
		{
			unpauseTime = level.unpauseTime;
			trap_SetConfigstring(CS_UNPAUSE, va("%d", unpauseTime));
		}
	}
	else
	{
		if (paused)
		{
			int		time = levelTime - pauseTime;
			int		i, j;

			paused = qfalse;
			trap_SetConfigstring(CS_UNPAUSE, "");
			G_Printf("Game unpaused\n");

#define ADJUST(x) if ((x) > 0) (x) += time;

			ADJUST(level.startTime)
			ADJUST(level.warmupTime)
			ADJUST(level.intermissiontime)
			ADJUST(level.roundQueued)
			ADJUST(level.intermissionQueued)
			ADJUST(level.exitTime)

			trap_SetConfigstring(CS_LEVEL_START_TIME, va("%i", level.startTime));
			trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));

			for (i = 0; i < level.maxclients; i++) {
				if (level.clients[i].pers.connected != CON_DISCONNECTED) {
					gentity_t		*ent = &g_entities[i];
					gclient_t		*client = ent->client;
					playerState_t	*ps = &client->ps;

					ADJUST(ent->s.constantLight)
					ADJUST(ent->s.emplacedOwner)

					ADJUST(ps->weaponChargeTime)
					ADJUST(ps->weaponChargeSubtractTime)
					ADJUST(ps->externalEventTime)
					ADJUST(ps->painTime)
					ADJUST(ps->lastOnGround)
					ADJUST(ps->saberLockTime)
					ADJUST(ps->saberDidThrowTime)
					ADJUST(ps->saberHitWallSoundDebounceTime)
					ADJUST(ps->rocketLastValidTime)
					ADJUST(ps->rocketLockTime)
					ADJUST(ps->rocketTargetTime)
					ADJUST(ps->emplacedTime)
					ADJUST(ps->droneFireTime)
					ADJUST(ps->droneExistTime)
					ADJUST(ps->holocronCantTouchTime)
					ADJUST(ps->electrifyTime)
					ADJUST(ps->saberBlockTime)
					ADJUST(ps->otherKillerTime)
					ADJUST(ps->otherKillerDebounceTime)
					ADJUST(ps->forceHandExtendTime)
					ADJUST(ps->forceRageDrainTime)
					ADJUST(ps->forceGripMoveInterval)
					ADJUST(ps->footstepTime)
					ADJUST(ps->otherSoundTime)
					ADJUST(ps->duelTime)
					ADJUST(ps->holdMoveTime)
					ADJUST(ps->forceAllowDeactivateTime)
					ADJUST(ps->zoomTime)
					ADJUST(ps->zoomLockTime)
					ADJUST(ps->useDelay)
					ADJUST(ps->fallingToDeath)
					ADJUST(ps->saberIdleWound)
					ADJUST(ps->saberAttackWound)
					ADJUST(ps->saberThrowDelay)

					for (j = 0; j < MAX_POWERUPS; j++) {
						ADJUST(ps->powerups[j])
					}

					for (j = 0; j < NUM_FORCE_POWERS; j++) {
						ADJUST(ps->holocronsCarried[j])
					}

					for (j = 0; j < NUM_FORCE_POWERS; j++) {
						ADJUST(ps->fd.forcePowerDebounce[j])
					}

					for (j = 0; j < NUM_FORCE_POWERS; j++) {
						ADJUST(ps->fd.forcePowerDuration[j])
					}

					ADJUST(ps->fd.forcePowerRegenDebounceTime)
					ADJUST(ps->fd.forceJumpAddTime)
					ADJUST(ps->fd.forceGripDamageDebounceTime)
					ADJUST(ps->fd.forceGripStarted)
					ADJUST(ps->fd.forceGripBeingGripped)
					ADJUST(ps->fd.forceGripUseTime)
					ADJUST(ps->fd.forceGripSoundTime)
					ADJUST(ps->fd.forceHealTime)
					ADJUST(ps->fd.forceRageRecoveryTime)
					ADJUST(ps->fd.forceDrainTime)

					ADJUST(client->invulnerableTimer)
					ADJUST(client->respawnTime)
					ADJUST(client->inactivityTime)
					ADJUST(client->rewardTime)
					ADJUST(client->airOutTime)
					ADJUST(client->lastKillTime)
					ADJUST(client->lastSaberStorageTime)
					ADJUST(client->dangerTime)
					ADJUST(client->forcePowerSoundDebounce)

					ADJUST(client->pers.teamState.lastfraggedcarrier)
					ADJUST(client->pers.teamState.lasthurtcarrier)
					ADJUST(client->pers.teamState.lastreturnedflag)
					ADJUST(client->pers.teamState.flagsince)
				}
			}

			for (i = 0; i < level.num_entities; i++) {
				if (g_entities[i].inuse) {
					gentity_t	*ent = &g_entities[i];

					if (ent->s.pos.trType != TR_STATIONARY) {
						ent->s.pos.trTime += time;
					}
					if (ent->s.apos.trType != TR_STATIONARY) {
						ent->s.apos.trTime += time;
					}
					// ADJUST(ent->s.time)
					// ADJUST(ent->s.time2)

					ADJUST(ent->nextthink)

					ADJUST(ent->aimDebounceTime)
					ADJUST(ent->painDebounceTime)
					ADJUST(ent->attackDebounceTime)
					ADJUST(ent->freetime)
					ADJUST(ent->eventTime)
					ADJUST(ent->timestamp)
					ADJUST(ent->setTime)
					ADJUST(ent->pain_debounce_time)
					ADJUST(ent->fly_sound_debounce_time)
					ADJUST(ent->last_move_time)
					ADJUST(ent->time1)
					ADJUST(ent->time2)

					// ent->s.genericenemyindex
					// ent->s.powerups
					// ent->bolt_RArm
					// ent->bolt_LArm
					// ent->bolt_LLeg
					// ent->bolt_Head

				}
			}
		}

		return qfalse;
	}

	// get any cvar changes
	G_UpdateCvars();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	return qtrue;
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/

void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
	int			msec;
	int 		start, end;

	if (gDoSlowMoDuel) {
		SlowMoDuelTimescale();
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	// send queued server command to all players now
	if ( level.snapnum == level.queuedCmdSnap ) {
		trap_SendServerCommand( -1, level.queuedCmd );
		level.queuedCmdSnap = 0;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	msec = level.time - level.previousTime;

	if (G_RunPausedFrame(levelTime)) {
		return;
	}

	// get any cvar changes
	G_UpdateCvars();

	//
	// go through all allocated objects
	//
	start = trap_Milliseconds();
	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		G_EntityCheckRep( ent );

		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{ //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else
				{
					G_FreeEntity( ent );
					continue;
				}
			} else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity( ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			G_RunMissile( ent );
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
			G_RunItem( ent );
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		if ( i < MAX_CLIENTS )
		{
			G_CheckClientTimeouts ( ent );

			if((!level.intermissiontime)&&!(ent->client->ps.pm_flags&PMF_FOLLOW) && ent->client->sess.spectatorState == SPECTATOR_NOT)
			{
				WP_ForcePowersUpdate(ent, &ent->client->pers.cmd );
				WP_SaberPositionUpdate(ent, &ent->client->pers.cmd);
			}
			G_RunClient( ent );
			continue;
		}

		G_RunThink( ent );
	}
end = trap_Milliseconds();

	trap_ROFF_UpdateEntities();

start = trap_Milliseconds();
	// perform final fixups on the players
	ent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
		G_EntityCheckRep( ent );
	}
end = trap_Milliseconds();

	// see if server has been idle
	CheckIdle();

	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	// garbage collection
	// G_FreeUnusedEntities();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();


	if (gQueueScoreMessage)
	{
		if (gQueueScoreMessageTime < level.time)
		{
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = qfalse;
		}
	}

	if ( g_unlagged.integer ) {
		G_BackupWorld();
	}

	level.snapnum++;

	// suppress unused-but-set warnings
	(void)start;
	(void)end;
	(void)msec;
}

const char *G_GetStripEdString(const char *refSection, const char *refName)
{
	/*
	static char text[1024];
	trap_SP_GetStringTextString(va("%s_%s", refSection, refName), text, sizeof(text));
	return text;
	*/

	//Well, it would've been lovely doing it the above way, but it would mean mixing
	//languages for the client depending on what the server is. So we'll mark this as
	//a striped reference with @@@ and send the refname to the client, and when it goes
	//to print it will get scanned for the striped reference indication and dealt with
	//properly.
	static char text[1024];
	Com_sprintf(text, sizeof(text), "@@@%s", refName);
	return text;
}

