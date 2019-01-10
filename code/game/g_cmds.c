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

#ifdef MISSIONPACK
#include "../../assets/ui/jk2mp/menudef.h"			// for the voice chats
#endif

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void BG_CycleInven(playerState_t *ps, int direction);
void BG_CycleForce(playerState_t *ps, int direction);

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[1400];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, dead, netDamage;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;

	if (numSorted > MAX_CLIENT_SCORE_SEND)
	{
		numSorted = MAX_CLIENT_SCORE_SEND;
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		if( cl->pers.accuracy_shots ) {
			accuracy = cl->pers.accuracy_hits * 100 / cl->pers.accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		// perfect = ( persistant[PERS_RANK] == 0 && persistant[PERS_KILLED] == 0 ) ? 1 : 0;
		dead = ( cl->sess.spectatorState != SPECTATOR_NOT || cl->ps.stats[STAT_HEALTH] <= 0 );

		netDamage = cl->pers.totalDamageDealtToEnemies;
		netDamage -= cl->pers.totalDamageTakenFromEnemies;
		netDamage /= 100; // Don't send exact data

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			level.sortedClients[i],
			cl->pers.persistant[PERS_SCORE],
			ping,
			(level.time - cl->pers.enterTime)/60000,
			scoreFlags,
			g_entities[level.sortedClients[i]].s.powerups,
			accuracy,
			cl->pers.persistant[PERS_KILLED],
			cl->pers.persistant[PERS_KILLS],
			netDamage,
//			persistant[PERS_IMPRESSIVE_COUNT],
//			persistant[PERS_EXCELLENT_COUNT],
//			persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->pers.persistant[PERS_DEFEND_COUNT],
			cl->pers.persistant[PERS_ASSIST_COUNT],
			dead,
//			perfect,
			cl->pers.persistant[PERS_CAPTURES]);
		j = strlen(entry);
		if (stringlength + j > 1022)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//still want to know the total # of clients
	i = level.numConnectedClients;

	trap_SendServerCommand( ent-g_entities, va("scores %i %i %i%s", i,
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
		string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}

/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	static char	line[MAX_STRING_CHARS];
	char		*cursor = line;
	int			remaining = MAX_STRING_CHARS;
	size_t		len;
	int			c = trap_Argc();
	int			i;

	// need at least 3 bytes for space, one character and '\0'
	for ( i = start ; i < c && remaining >= 3 ; i++ ) {
		if ( i > start ) {
			*cursor++ = ' ';
			remaining--;
		}
		trap_Argv( i, cursor, remaining );
		len = strlen( cursor );
		cursor += len;
		remaining -= len;
	}

	return line;
}

/*
G_ClientNumberFromPattern

Returns a client number of a matching player as described below
-1 when there are no matching players
-2 when there are multiple matching players

Description: There are 6 test cases: compare (5), compare
color-insensitive (4), compare color-case-insensitive (3), find
substring (2), find color-insensitive substring (1) and find
color-case-insensitive substring (0).

If for one of these test cases there is only a single player name that
passes it, it's a hit. There can be many hits so more precise (higher
in the numeration above) match takes precendence.

 5     4     3
  *<---*<---*
  ^    ^    ^
  |    |    |
  *<---*<---*
 2     1     0

Graph above shows relations between negated test cases (ie not
satisfying 1 implies that tested string doesn't satisfy 2 4 and 5). It
has been used for optimization.
*/

#define MatchedTwice(n) (matchedTwice & 1 << (n))
#define StoreMatch(n) matchedTwice |= matched & 1 << (n); matched |= 1 << (n); matches[n] = idnum

static int G_ClientNumberFromPattern ( const char *pattern ) {
	gclient_t	*cl;
	char		ciName[MAX_NETNAME];
	char		cciName[MAX_NETNAME];
	char		ciPattern[MAX_NETNAME];
	char		cciPatter[MAX_NETNAME];
	const char	*name;
	int			idnum = 0;
	int			matches[5];			// Element n stores last client number matched in test case n
	int			matched = 0;		// bit n stores 1 if at least one netname passed test case n
	int			matchedTwice = 0;	// bit n stores 1 if at least two netnames passed test case n

	Q_strncpyz(ciPattern, pattern, sizeof(ciPattern));
	Q_CleanStr(ciPattern);
	Q_strncpyz(cciPatter, ciPattern, sizeof(cciPatter));
	Q_strlwr(cciPatter);

	for ( cl = level.clients; idnum < level.maxclients; idnum++, cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		name = cl->info.netname;

		Q_strncpyz(ciName, name, sizeof(ciName));
		Q_CleanStr(ciName);
		Q_strncpyz(cciName, ciName, sizeof(cciName));
		Q_strlwr(cciName);

		if ( !MatchedTwice(0) ) {
			if ( strstr(cciName, cciPatter) ) {
				StoreMatch(0);
			} else {
				continue;
			}
		}
		if ( !MatchedTwice(1) ) {
			if ( strstr(ciName, ciPattern) ) {
				StoreMatch(1);
			} else {
				if ( !MatchedTwice(3) ) {
					if ( !strcmp(cciName, cciPatter) ) {
						StoreMatch(3);
					}
				}
				continue;
			}
		}
		if ( !MatchedTwice(2) ) {
			if ( strstr(name, pattern) ) {
				StoreMatch(2);
			} else {
				if ( !MatchedTwice(3) ) {
					if ( !strcmp(cciName, cciPatter) ) {
						StoreMatch(3);
					} else {
						continue;
					}
				}
				if ( !MatchedTwice(4) ) {
					if ( !strcmp(ciName, ciPattern) ) {
						StoreMatch(4);
					}
				}
				continue;
			}
		}
		if ( !MatchedTwice(3) ) {
			if ( !strcmp(cciName, cciPatter) ) {
				StoreMatch(3);
			} else {
				continue;
			}
		}
		if ( !MatchedTwice(4) ) {
			if ( !strcmp(ciName, ciPattern) ) {
				StoreMatch(4);
			} else {
				continue;
			}
		}
		// netnames are distinct so just return when we get an exact match.
		if ( !strcmp(name, pattern) ) {
			return idnum;
		}
	}

	matched &= ~matchedTwice;		// Matched once

	if ( matched & 16 ) {
		return matches[4];
	} else if ( matched & 8 ) {
		return matches[3];
	} else if ( matched & 4 ) {
		return matches[2];
	} else if ( matched & 2 ) {
		return matches[1];
	} else if ( matched & 1 ) {
		return matches[0];
	} else if ( matchedTwice ) {
		return -2;
	} else {
		return -1;
	}
}

/*
==================
G_ClientNumberFromString

Returns a player number for either a number or name string
When invalid returns -1 and sets *errorMsg to error string.
==================
*/
int G_ClientNumberFromString( const char *s, const char **errorMsg ) {
	gclient_t	*cl;
	int			idnum;

	if ( !s || !s[0] ) {
		*errorMsg = "No player name provided\n";
		return -1;
	}

	// numeric values could be slot numbers
	if ( Q_IsInteger( s ) ) {
		idnum = atoi( s );
		if ( idnum < 0 && idnum >= level.maxclients ) {
			*errorMsg = va("Bad client slot: %i\n", idnum);
			return -1;
		}
		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			*errorMsg = va("Client %i is not on the server\n", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	idnum = G_ClientNumberFromPattern(s);
	if ( idnum >= 0 ) {
		return idnum;
	} else if ( idnum == -1 ) {
		*errorMsg = va("There is no user matching '%s" S_COLOR_WHITE "' on the server\n", s);
	} else if ( idnum == -2 ) {
		*errorMsg = va("There are multiple users with '%s" S_COLOR_WHITE "' in their names. Please be more specific.\n", s);
	}
	return -1;
}

/*
==================
G_CrosshairPlayer

Inferior, serverside variant of CG_CrosshairPlayer. ent must be a
client.
==================
*/
static gentity_t *G_CrosshairPlayer( gentity_t *ent, int maxDist ) {
	trace_t tr;
	vec3_t org, forward, fwdOrg;

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	VectorCopy(ent->client->ps.origin, org);
	org[2] += ent->client->ps.viewheight;
	VectorMA(org, maxDist, forward, fwdOrg);

	G_Trace(&tr, org, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

	if (tr.fraction != 1 && tr.entityNum < MAX_CLIENTS) {
		return &g_entities[tr.entityNum];
	}
	return NULL;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		name[MAX_TOKEN_CHARS];
	gitem_t	*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];

	trap_Argv( 1, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all)
	{
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= LEGAL_ITEMS;
	}

	if (give_all || Q_stricmp( name, "health") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->health = atoi(arg);
			if (ent->health > ent->client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
			}
		}
		else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		ent->client->ps.stats[STAT_HEALTH] = ent->health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = LEGAL_WEAPONS;
		if (!give_all)
			return;
	}

	if ( !give_all && Q_stricmp(name, "weaponnum") == 0 )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi(arg));
		return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		int num = INFINITE_AMMO;
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			num = atoi(arg);
		}
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = num;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		if (trap_Argc() == 3) {
			trap_Argv( 2, arg, sizeof( arg ) );
			ent->client->ps.stats[STAT_ARMOR] = atoi(arg);
		} else {
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if (!give_all)
			return;
	}

	if (Q_stricmp(name, "excellent") == 0) {
		ent->client->pers.persistant[PERS_EXCELLENT_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_EXCELLENT;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->pers.persistant[PERS_IMPRESSIVE_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0 ||
		Q_stricmp(name, "humiliation") == 0) {
		ent->client->pers.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_GAUNTLET;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "denied") == 0) {
		ent->client->pers.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_DENIED;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "capture") == 0) {
		ent->client->pers.persistant[PERS_CAPTURES]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_CAP;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->pers.persistant[PERS_DEFEND_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_DEFEND;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->pers.persistant[PERS_ASSIST_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_ASSIST;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn( ENTITYNUM_WORLD );
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	const char	*msg;

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	const char	*msg;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	const char	*msg;

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = (qboolean)!ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	// doesn't work in single player
	if ( level.gametype != 0 ) {
		trap_SendServerCommand( ent-g_entities,
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_TeamTask_f
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if (level.gametype == GT_TOURNAMENT && level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	int clientnum;

	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->info.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->info.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		for ( clientnum = 0; clientnum < level.maxclients; clientnum++ ) {
			if ( level.clients[ clientnum ].pers.connected == CON_CONNECTED
			     && !level.clients[ clientnum ].ps.duelInProgress ) {
				trap_SendServerCommand( clientnum, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->info.netname, G_GetStripEdString("SVINGAME", "JOINEDTHESPECTATORS")));
			}
		}
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (level.gametype == GT_TOURNAMENT)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->info.netname, G_GetStripEdString("SVINGAME", "VERSUS"), client->info.netname));
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->info.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			for ( clientnum = 0; clientnum < level.maxclients; clientnum++ ) {
				if ( level.clients[ clientnum ].pers.connected == CON_CONNECTED
				     && !level.clients[ clientnum ].ps.duelInProgress ) {
					trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
					client->info.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
				}
			}
		}
	}
}

/*
=================
SetTeamSpec
=================
*/
qboolean SetTeamSpec( gentity_t *ent, team_t team, spectatorState_t specState, int specClient )
{
	gclient_t	*client;
	int			clientNum;
	team_t		oldTeam;
	int			teamLeader;
	qboolean	oldSpec, newSpec;

	client = ent->client;
	clientNum = client - level.clients;
	oldTeam = client->sess.sessionTeam;
	oldSpec = (qboolean)(client->sess.spectatorState != SPECTATOR_NOT);
	newSpec = (qboolean)(specState != SPECTATOR_NOT);

	assert( !(team == TEAM_SPECTATOR && specState == SPECTATOR_NOT) );

	if ( !oldSpec && newSpec ) {
		// if the player was dead leave the body
		CopyToBodyQue(ent);

		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		client->ps.fd.forceDoInit = qtrue;
		client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_LEAVE);
	}

	// specClient < 0 is dedicated spectator; see SortRanks
	if ( specState != SPECTATOR_FOLLOW && specClient < 0 )
		specClient = 0;

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	if ( team == oldTeam ) {
		// handle switching from/to SPECTATOR_NOT without changing teams
		if ( newSpec != oldSpec ) {
			if ( newSpec ) {
				trap_UnlinkEntity( ent );
			} else {
				gentity_t	*tent;

				tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN, clientNum );
				tent->s.clientNum = ent->s.number;

				// don't keep force powers of last followed player
				ent->client->ps.fd.forceDoInit = qtrue;
			}

			ClientSpawn( ent );
		}
		return qfalse;
	}

	//
	// execute the team change
	//

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;

	if ( team == TEAM_SPECTATOR ) {
		// they go to the end of the line for tournaments
		AddTournamentQueue(client);

		if ( client->ps.duelInProgress ) {
			client->sess.losses++;
		}
	}

	// every time we change teams make sure our force powers are set right
	client->ps.fd.forceDoInit = qtrue;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUpdateConfigString( clientNum );

	ClientBegin( clientNum, qfalse );

	return qtrue;
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam( gentity_t *ent, team_t team )
{
	spectatorState_t state = (team == TEAM_SPECTATOR) ? SPECTATOR_FREE : SPECTATOR_NOT;

	return SetTeamSpec( ent, team, state, ent->client->sess.spectatorClient );
}

/*
=================
SetTeamFromString
=================
*/
static void SetTeamFromString( gentity_t *ent, char *s ) {
	team_t				team;
	int					clientNum;
	spectatorState_t	specState;

	//
	// see what change is requested
	//
	clientNum = ent - g_entities;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( GT_Team(level.gametype) ) {
		// if running a team game, assign player to one of the teams
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			if ( !ValidateTeam(clientNum, TEAM_RED) ) {
				trap_SendServerCommand( clientNum,
					va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYRED")) );
				return;
			}
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			if ( !ValidateTeam(clientNum, TEAM_BLUE) ) {
				trap_SendServerCommand( clientNum,
					va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TOOMANYBLUE")) );
				return;
			}
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
			if ( team == TEAM_SPECTATOR ) {
				trap_SendServerCommand( clientNum, "print \"Teams are full.\n\""); // TRANSLATE
				return;
			}
		}

		// new players need to wait for next round
		if ( level.round > 0 && level.gametype != GT_REDROVER && !level.roundQueued ) {
			specState = SPECTATOR_FOLLOW;
			ent->client->sess.spectatorClient = FOLLOW_TEAM;
		}
	} else {
		team = TEAM_FREE;
		if ( !ValidateTeam(clientNum, TEAM_FREE) ) {
			trap_SendServerCommand( clientNum, "print \"Game is full.\n\""); // TRANSLATE
			return;
		}
	}

	if ( level.teamLock[team] ) {
		trap_SendServerCommand( clientNum, va("print \"%s%s" S_COLOR_WHITE " team is locked.\n\"",
				BG_TeamColor(team), BG_TeamName(team, CASE_NORMAL)) );
		return;
	}

	if ( SetTeamSpec( ent, team, specState, ent->client->sess.spectatorClient ) ) {
		ent->client->prof.switchTeamTime = level.time + 5000;
	};
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	gclient_t	*client = ent->client;

	// bots can follow too
	// ent->r.svFlags &= ~SVF_BOT;
	client->sess.spectatorState = SPECTATOR_FREE;
	client->ps.commandTime = client->pers.cmd.serverTime;
	memcpy( client->ps.persistant, client->pers.persistant, sizeof( client->ps.persistant ) );
	client->ps.pm_type = PM_SPECTATOR;
	client->ps.pm_flags &= ~PMF_FOLLOW;
	client->ps.eFlags &= ~EF_DISINTEGRATION;
	client->ps.clientNum = ent - g_entities;
	client->ps.weapon = WP_NONE;
	client->ps.viewangles[ROLL] = 0.0f;
	ent->health = client->ps.stats[STAT_HEALTH] = 100;
	client->ps.legsAnim = 0;
	client->ps.legsTimer = 0;
	client->ps.torsoAnim = 0;
	client->ps.torsoTimer = 0;
	client->ps.emplacedIndex = 0;
	client->ps.isJediMaster = qfalse;
	client->ps.zoomMode = ZOOM_NONE;
	client->ps.zoomLocked = qfalse;
	client->ps.zoomLockTime = 0;
	client->ps.fallingToDeath = 0;
	client->ps.forceHandExtend = HANDEXTEND_NONE;
	client->ps.duelInProgress = qfalse;
	client->ps.weaponTime = 0;
	client->ps.saberHolstered = qtrue;
	client->ps.saberLockTime = 0;
	client->ps.saberLockFrame = 0;
	client->ps.saberLockEnemy = 0;
	client->ps.saberMove = LS_NONE;
	client->ps.saberBlocked = BLOCKED_NONE;
	client->ps.saberBlocking = BLK_NO;
	client->ps.saberEntityNum = ENTITYNUM_NONE;
	memset( client->ps.powerups, 0, sizeof( client->ps.powerups ) );
	client->ps.fd.forceDoInit = qtrue;

	SetClientViewAngle( ent, client->ps.viewangles );
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];
	const char	*printTeam;

	if ( trap_Argc() != 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			printTeam = G_GetStripEdString("SVINGAME", "PRINTBLUETEAM");
			break;
		case TEAM_RED:
			printTeam = G_GetStripEdString("SVINGAME", "PRINTREDTEAM");
			break;
		case TEAM_FREE:
			printTeam = G_GetStripEdString("SVINGAME", "PRINTFREETEAM");
			break;
		case TEAM_SPECTATOR:
			printTeam = G_GetStripEdString("SVINGAME", "PRINTSPECTEAM");
			break;
		default:
			return;
		}
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", printTeam) );
		return;
	}

	if ( ent->client->prof.switchTeamTime > level.time ) {
		if ( ent->client->prof.switchTeamTime - level.time > 5000 ) {
			trap_SendServerCommand( ent-g_entities,
				va("print \"You were removed. May not switch teams for %d more seconds.\n\"",
					(ent->client->prof.switchTeamTime - level.time + 999) / 1000));
		} else {
			trap_SendServerCommand( ent-g_entities,
				va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSWITCH")) );
		}
		return;
	}

	if (gEscaping)
	{
		return;
	}

	if (g_requireClientside.integer && !ent->client->pers.registered) {
		// clientside message will be printed in ClientBegin
		ClientBegin( ent->s.number, qfalse );
		return;
	} else {
		trap_Argv( 1, s, sizeof( s ) );
	}

	SetTeamFromString( ent, s );
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
//	Cmd_Kill_f(ent);
	if (ent->client->sess.spectatorState != SPECTATOR_NOT)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, G_GetStripEdString("SVINGAME", "FORCEPOWERCHANGED")) );

	ent->client->ps.fd.forceDoInit = qtrue;
argCheck:
	if (level.gametype == GT_TOURNAMENT)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1 && !level.intermissionQueued && !level.intermissiontime)
	{
		//if there's an arg, assume it's a combo team command from the UI.
		Cmd_Team_f(ent);
	}
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	const char	*errorMsg;
	char	arg[MAX_TOKEN_CHARS];
	int		i;

	// don't mess with following non-spectators for now
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
		ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		return;
	}

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && level.teamLock[TEAM_SPECTATOR] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s%s" S_COLOR_WHITE " team is locked.\n\"",
				BG_TeamColor(TEAM_SPECTATOR), BG_TeamName(TEAM_SPECTATOR, CASE_NORMAL)) );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	if (!strcmp(arg, "-1") || !Q_stricmp(arg, "first")) {
		i = FOLLOW_FIRST;
	} else if (!strcmp(arg, "-2") || !Q_stricmp(arg, "second"))
		i = FOLLOW_SECOND;
	else {
		i = G_ClientNumberFromString( arg, &errorMsg );
		if ( i == -1 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}
	}

	if ( SetTeamSpec( ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, i ) ) {
		ent->client->prof.switchTeamTime = level.time + 5000;
	};
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	gclient_t	*client = ent->client;
	qboolean	teamRestrict;
	team_t		team;
	int			clientnum;
	int			original;

	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		return;
	} else if ( client->sess.spectatorState == SPECTATOR_FREE ) {
		gentity_t *target = G_CrosshairPlayer(ent, 8192);

		if (target) {
			SetTeamSpec(ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, target->s.number);
			return;
		}
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = client->sess.spectatorClient;

	if (clientnum == FOLLOW_FIRST) {
		clientnum = level.follow1;
	} else if (clientnum == FOLLOW_SECOND) {
		clientnum = level.follow2;
	}
	if ( clientnum < 0 || clientnum >= level.maxclients ) {
		clientnum = 0;
	}

	// can't make yourself a following non-spectator here
	team = client->sess.sessionTeam;
	if (g_restrictSpectator.integer && client->sess.sessionTeam != TEAM_SPECTATOR) {
		teamRestrict = qtrue;
	} else {
		teamRestrict = qfalse;
	}

	original = clientnum;

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT ) {
			continue;
		}

		// can't spy on opposing team
		if ( teamRestrict && level.clients[ clientnum ].sess.sessionTeam != team ) {
			continue;
		}

		// this is good, we can use it
		SetTeamSpec( ent, team, SPECTATOR_FOLLOW, clientnum );
		return;
	} while ( clientnum != original );

	// leave it where it was
}

static void Cmd_FollowNext_f( gentity_t *ent )
{
	Cmd_FollowCycle_f( ent, 1 );
}

static void Cmd_FollowPrev_f( gentity_t *ent )
{
	Cmd_FollowCycle_f( ent, -1 );
}

void Cmd_SmartFollowCycle_f( gentity_t *ent )
{
	gclient_t	*client = ent->client;
	gclient_t	*ci;
	qboolean	teamRestrict;
	team_t		followTeam;
	int			clientNum, clientRank;
	int			player;
	int			i;

	clientNum = -1;

	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		return;
	} else if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		clientNum = client->sess.spectatorClient;
	} else if ( client->sess.spectatorState == SPECTATOR_FREE ) {
		gentity_t	*target = G_CrosshairPlayer(ent, 8192);

		if ( target ) {
			player = target->s.number;
			goto followPlayer;
		}
	}

	if (clientNum == FOLLOW_FIRST) {
		clientNum = level.follow1;
	} else if (clientNum == FOLLOW_SECOND) {
		clientNum = level.follow2;
	}

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		clientNum = level.sortedClients[0];
	}

	ci = level.clients + clientNum;

	// Sanity check
	if (ci->pers.connected != CON_CONNECTED  ||
		ci->sess.spectatorState != SPECTATOR_NOT)
	{
		StopFollowing(ent);
		return;
	}

	// can't make yourself a following non-spectator here
	if ( g_restrictSpectator.integer && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		teamRestrict = qtrue;
		followTeam = client->sess.sessionTeam;
	} else {
		teamRestrict = qfalse;
		followTeam = ci->sess.sessionTeam;
	}

	// Alternate between dueling players
	if ( ci->ps.duelInProgress ) {
		player = ci->ps.duelIndex;

		if ( !teamRestrict || level.clients[player].sess.sessionTeam == followTeam )
			goto followPlayer;
	}

	i = 0;
	while (i < level.numPlayingClients && level.sortedClients[i] != clientNum) {
		i++;
	}
	if (i >= level.numPlayingClients) {
		return;
	}
	// numPlayingClients > 0 and sortedClients[i] == clientNum now.

	clientRank = i;

	// Try to find a powerup player first
	do {
		if (--i < 0) {
			i = level.numPlayingClients - 1;
		}
		ci = &level.clients[level.sortedClients[i]];

		if (ci->sess.spectatorState != SPECTATOR_NOT)
			continue;
		if (teamRestrict && ci->sess.sessionTeam != followTeam)
			continue;

		if (ci->ps.isJediMaster || ci->ps.powerups[PW_REDFLAG] ||
			ci->ps.powerups[PW_BLUEFLAG] || ci->ps.powerups[PW_YSALAMIRI])
		{
			player = level.sortedClients[i];
			goto followPlayer;
		}
	} while (i != clientRank);

	if ( GT_Team(level.gametype) ) {
		// Cycle through sorted team
		do {
			if (--i < 0) {
				i = level.numPlayingClients - 1;
			}
			ci = &level.clients[level.sortedClients[i]];

			if (ci->sess.spectatorState == SPECTATOR_NOT && ci->sess.sessionTeam == followTeam) {
				player = level.sortedClients[i];
				goto followPlayer;
			}
		} while (i != clientRank);
	} else {
		// Cycle through sorted players
		do {
			if (--i < 0) {
				i = level.numPlayingClients - 1;
			}
			ci = &level.clients[level.sortedClients[i]];

			if (ci->sess.spectatorState == SPECTATOR_NOT) {
				player = level.sortedClients[i];
				goto followPlayer;
			}
		} while (i != clientRank);
	}
	return;
followPlayer:
	SetTeamSpec( ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, player );
}

/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM
		 && ent->client->sess.sessionTeam != other->client->sess.sessionTeam ) {
		return;
	}
	if ( g_restrictChat.integer &&
		 other->client->ps.duelInProgress &&
		 other->client->ps.duelIndex != ent->s.number &&
		 other != ent ) {
		return;
	}

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"",
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( LOG_SAY, "Say: %i: %s: %s\n", ent->s.number, ent->client->info.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->info.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( LOG_SAY_TEAM, "SayTeam: %i %s: %s: %s\n", ent->s.number,
			BG_TeamName(ent->client->sess.sessionTeam, CASE_UPPER), ent->client->info.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ",
				ent->client->info.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->info.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && GT_Team(level.gametype) &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->info.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->info.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent ) {
	const char	*p;
	int			mode;

	if ( trap_Argc () < 2 ) {
		return;
	}

	p = ConcatArgs( 1 );

	mode = SAY_ALL;
	if ( g_restrictChat.integer && ent->client->sess.sessionTeam == TEAM_SPECTATOR )
		if ( !level.warmupTime && !level.intermissiontime )
			if ( !GT_Round(level.gametype) || level.round > 0 )
				mode = SAY_TEAM;

	G_Say( ent, NULL, mode, p );

	if ( !strcmp( p, "!info" ) )
	{
		trap_SendServerCommand( ent-g_entities,
			"print \"" S_LINE_PREFIX S_COLOR_WHITE "This server is running "
			GAMEVERSION_C S_COLOR_WHITE " version " GIT_VERSION " by "
			S_COLOR_BRAND AUTHOR S_COLOR_WHITE ".\n\"" );
	}
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( gentity_t *ent ) {
	const char	*p;

	if ( trap_Argc () < 2 ) {
		return;
	}

	p = ConcatArgs( 1 );

	G_Say( ent, NULL, SAY_TEAM, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	const char	*errorMsg;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	// short circuit for speaking with dedicated server
	if ( !strcmp(arg, "server") ) {
		char *name;
		p = ConcatArgs( 2 );
		G_LogPrintf( LOG_TELL, "Tell: %i %i: %s to server: %s\n",
			ent->s.number, ent->s.number, ent->client->info.netname, p );
		name = va(EC"[%s" S_COLOR_WHITE EC "]"EC": ", ent->client->info.netname);
		trap_SendServerCommand( ent-g_entities, va("chat \"%s" S_COLOR_MAGENTA "%s\"", name, p) );
		return;
	}

	targetNum = G_ClientNumberFromString( arg, &errorMsg );
	if ( targetNum == -1 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	if ( g_restrictChat.integer ) {
		if ( !level.warmupTime && !level.intermissiontime &&
			( !GT_Round(level.gametype) || level.round > 0 ) &&
			 ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
			 target->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			trap_SendServerCommand( ent-g_entities,
			"print \"Chatting to players in game is disabled on the server.\n\"" );
			return;
		}
		if ( target->client->ps.duelInProgress &&
			 target->client->ps.duelIndex != ent->s.number ) {
			trap_SendServerCommand( ent-g_entities,
			"print \"Chatting to dueling players is disabled on the server.\n\"" );
			return;
		}
	}

	G_LogPrintf( LOG_TELL, "Tell: %i %i: %s to %s: %s\n", ent->s.number, target->s.number,
		ent->client->info.netname, target->client->info.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

#ifdef MISSIONPACK
static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	const char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if (level.gametype == GT_TOURNAMENT) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( !GT_Team(level.gametype) && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->info.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char		*p;

	if ( trap_Argc () < 2 && !arg0 ) {
		return;
	}

	if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	const char	*errorMsg;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = G_ClientNumberFromString( arg, &errorMsg );
	if ( targetNum == -1 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( LOG_VTELL, "VTell: %i %i: %s to %s: %s\n", ent->s.number,
		target->s.number, ent->client->info.netname, target->client->info.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_STUN_BATON) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (GT_Team(level.gametype)) {
		// praise a team mate who just got a reward
		for(i = 0; i < level.maxclients; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
#endif // MISSIONPACK

static const char * const gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > (int)ARRAY_LEN(gc_orders) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	voteCmd_t		voteCmd;
	const char		*voteName;
	int				i;
	char			arg1[MAX_STRING_TOKENS];
	const char		*arg2;
	const char		*errorMsg;
	const char		*mapInfo;
	char			s[MAX_STRING_CHARS];
	int				voteMask;

	typedef struct {
		const char	*name;		// must be lowercase
		const char	*longName;
		const char	*synopsis;
	} voteCmdInfo_t;

	static const voteCmdInfo_t voteCmds[CV_MAX] = {
		{ "invalid",		"Invalid",		"" },				// CV_INVALID
		{ "map_restart",	"Map Restart",	"" }, 				// CV_MAP_RESTART
		{ "nextmap",		"Next Map",		"" },				// CV_NEXTMAP
		{ "map",			"Map",			" <name>" },		// CV_MAP
		{ "gametype",		"Gametype",		" <name>" },		// CV_GAMETYPE
		{ "kick",			"Kick",			" <name|num>" },	// CV_KICK
		{ "shuffle",		"Shuffle",		"" },				// CV_SHUFFLE
		{ "g_dowarmup",		"Do Warmup",	" <0|1>" },			// CV_DOWARMUP
		{ "timelimit",		"Timelimit",	" <minutes>" },		// CV_TIMELIMIT
		{ "fraglimit",		"Fraglimit",	" <frags>" },		// CV_FRAGLIMIT
		{ "roundlimit",		"Roundlimit",	" <rounds>" },		// CV_ROUNDLIMIT
		{ "teamsize",		"Team Size",	" <size>" },		// CV_TEAMSIZE
		{ "remove",			"Remove",		" <name|num>" },	// CV_REMOVE
		{ "wk",				"With Kicks",	" <0|1>" },			// CV_WK
		{ "mode",			"Mode",			" <name>" },		// CV_MODE
		{ "matchmode",		"Match Mode",	" <0|1>" },			// CV_MATCH
		{ "capturelimit",	"Capturelimit",	" <caps>" },		// CV_CAPTURELIMIT
		{ "poll",			"Poll",			" <question>" },	// CV_POLL
		{ "referee",		"Referee",		" <name|num>" },	// CV_REFEREE
	};

	if ( ent->client->sess.referee ) {
		if (g_allowRefVote.integer == 1) {
			voteMask = -1;
		} else {
			voteMask = g_allowRefVote.integer;
		}
	} else {
		if ( !g_allowVote.integer ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
			return;
		}

		if ( level.voteTime || level.voteExecuteTime ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEINPROGRESS")) );
			return;
		}
		/*
		  if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		  trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXVOTES")) );
		  return;
		  }
		*/
		if ( ent->s.number == level.voteClient && level.voteCooldown > level.time ) {
			trap_SendServerCommand( ent-g_entities,
				va("print \"You must wait %d seconds before calling a new vote.\n\"", g_voteCooldown.integer) );
			return;
		}
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
			return;
		}

		if (g_allowVote.integer == 1) {
			voteMask = -1;
		} else {
			voteMask = g_allowVote.integer;
		}
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2 = ConcatArgs( 2 );

	Q_strlwr( arg1 );

	voteCmd = CV_INVALID;
	for ( i = CV_FIRST; i < (int)ARRAY_LEN(voteCmds); i++ ) {
		if ( !((1 << i) & voteMask) ) {
			continue;
		}
		if ( !strcmp( arg1, voteCmds[i].name ) ) {
			voteCmd = (voteCmd_t)i;
			break;
		}
	}

	if ( voteCmd == CV_INVALID ) {
		const int	columns = 3;
		const char	*fmt = "%-26s";
		char		line[DEFAULT_CONSOLE_WIDTH + 1];
		int			j;

		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string. Allowed votes are:\n\"" );

		line[0] = '\0';
		j = 0;
		for (i = CV_FIRST; i < (int)ARRAY_LEN(voteCmds); i++) {
			if ((1 << i) & voteMask) {
				char	synopsis[DEFAULT_CONSOLE_WIDTH + 1];

				Com_sprintf(synopsis, sizeof(synopsis), "%s%s", voteCmds[i].name, voteCmds[i].synopsis);
				Q_strcat(line, sizeof(line), va(fmt, synopsis));

				j++;
				if (j % columns == 0) {
					trap_SendServerCommand(ent-g_entities, va("print \"%s\n\"", line));
					line[0] = '\0';
				}
			}
		}

		trap_SendServerCommand(ent-g_entities, va("print \"%s\n\"", line));
		return;
	}

	if( strchr( arg2, ';' ) || strchr( arg2, '\n' ) || strchr( arg2, '\r' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote string contains forbidden characters.\n\"" );
		return;
	}

	voteName = voteCmds[voteCmd].longName;

	switch ( voteCmd ) {
	case CV_GAMETYPE:
		i = G_GametypeForString( arg2 );

		if ( i == GT_MAX_GAME_TYPE ) {
			char		gametypes[MAX_PRINT_TEXT];
			qboolean	printSep = qfalse;

			gametypes[0] = '\0';
			for (i = 0; i < GT_MAX_GAME_TYPE; i++) {
				if (GT_Valid(i)) {
					if (printSep)
						Q_strcat(gametypes, sizeof(gametypes), ", ");
					Q_strcat(gametypes, sizeof(gametypes), gametypeShort[i]);
					printSep = qtrue;
				}
			}

			Q_strlwr( gametypes );

			trap_SendServerCommand( ent-g_entities,
				va("print \"Valid gametypes are: %s\n\"", gametypes) );
			return;
		}

		level.voteArg = i;

		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_gametype %d", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s",
			voteName, gametypeLong[i] );
		break;
	case CV_MAP:
	{
		const char	*mapName;
		arena_t		arena;

		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		arena = G_GetArenaByMap( arg2 );

		if ( !G_DoesArenaSupportGametype( arena, level.gametype ) )
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", G_GetStripEdString("SVINGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		mapInfo = G_GetArenaInfo( arena );
		mapName = Info_ValueForKey( mapInfo, "map" );

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "map %s; set nextmap \"%s\"", mapName, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "map %s", mapName );
		}

		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			"%s %s" S_COLOR_WHITE " (%s" S_COLOR_WHITE ")", voteName,
			Info_ValueForKey( mapInfo, "longname" ), mapName );
		break;
	}
	case CV_KICK:
		i = G_ClientNumberFromString( arg2, &errorMsg );

		if ( i == -1 )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}

		level.voteArg = i;

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", i );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s",
			voteName, g_entities[i].client->info.netname );
		break;
	case CV_REFEREE:
		i = G_ClientNumberFromString( arg2, &errorMsg );

		if ( i == -1 )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}

		level.voteArg = i;

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "referee %d", i );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s",
			voteName, g_entities[i].client->info.netname );
		break;
	case CV_NEXTMAP:
		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", voteName );
		break;
	case CV_TEAMSIZE:
		i = atoi( arg2 );

		if ( 0 < i && i < g_teamsizeMin.integer ) {
			trap_SendServerCommand( ent-g_entities,
				va("print \"teamsize must be greater than %d.\n\"",	g_teamsizeMin.integer - 1 ) );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "teamsize \"%d\"", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %d", voteName, i );
		break;
	case CV_REMOVE:
		i = G_ClientNumberFromString( arg2, &errorMsg );

		if ( i == -1 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "remove %d", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s",
			voteName, g_entities[i].client->info.netname );
		break;
	case CV_WK:
		i = atoi( arg2 );

		if ( i == 0 )
		{
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"dmflags %d; g_friendlyFire 1", g_dmflags.integer | DF_NO_KICK );
			Q_strncpyz( level.voteDisplayString, "No Kick", sizeof( level.voteDisplayString ) );
		}
		else
		{
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"dmflags %d; g_friendlyFire 0", g_dmflags.integer & ~DF_NO_KICK );
			Q_strncpyz( level.voteDisplayString, "With Kick", sizeof( level.voteDisplayString ) );
		}
		break;
	case CV_MODE:
	{
		fileHandle_t	f;

		if ( arg2[0] == '\0' ) {
			trap_SendServerCommand( ent-g_entities, "print \"Usage: callvote mode <name>\n\"" );
			return;
		}
		if ( trap_FS_FOpenFile( va( "modes/%s.cfg", arg2 ), &f, FS_READ) < 0 ) {
			trap_SendServerCommand( ent-g_entities, "print \"Unknown mode.\n\"" );
			return;
		}
		trap_FS_FCloseFile( f );

		Com_sprintf( level.voteString, sizeof( level.voteString ), "mode \"%s\"", arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", voteName, arg2 );
		break;
	}
	case CV_MATCH:
		i = atoi( arg2 );

		if ( i == 0 ) {
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"g_restrictChat 0; g_restrictSpectator 0; g_requireClientside 0; g_damagePlums 1" );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Practice Mode" );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"g_restrictChat 1; g_restrictSpectator 1; g_requireClientside 1; g_damagePlums 0" );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Match Mode" );
		}
		break;
	case CV_POLL:
		if (arg2[0] == '\0') {
			trap_SendServerCommand( ent-g_entities, "print \"Usage: callvote poll <question>\n\"" );
		}
		Q_strncpyz( level.voteDisplayString, arg2, sizeof( level.voteDisplayString ) );
		break;
	case CV_MAP_RESTART:
		Com_sprintf( level.voteString, sizeof( level.voteString ), "map_restart 0" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", voteName );
		break;
	case CV_SHUFFLE:
		// no argument votes
		Com_sprintf( level.voteString, sizeof( level.voteString ), "shuffle" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", voteName );
		break;
	default:
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", voteName, arg2 );
	}

	trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s: %s\n\"",
			ent->client->info.netname,
			G_GetStripEdString("SVINGAME", "PLCALLEDVOTE"),
			level.voteDisplayString) );

	// start the voting, the caller autoamtically votes yes
	level.voteCmd = voteCmd;
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	level.voteClient = ent->s.number;
	level.voteReferee = ent->client->sess.referee ? VOTE_YES : VOTE_NONE;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
		level.clients[i].pers.vote = VOTE_NONE;
	}
	ent->client->ps.eFlags |= EF_VOTED;
	ent->client->pers.vote = VOTE_YES;

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, "1" );
	trap_SetConfigstring( CS_VOTE_NO, "0" );

}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[2];
	vote_t		vote;

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );
	vote = ( msg[0] == 'y' || msg[0] == 'Y' || msg[0] == '1' ) ? VOTE_YES : VOTE_NO;

	if ( ent->client->sess.referee ) {
		level.voteReferee = vote;
	}

	if ( vote != ent->client->pers.vote ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLVOTECAST")) );
		ent->client->pers.vote = vote;
		CalculateRanks();
	} else {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEALREADY")) );
	}
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int			i, cs_offset;
	char		arg1[MAX_STRING_TOKENS];
	const char	*arg2;
	team_t		team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADY")) );
		return;
	}
	/*
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXTEAMVOTES")) );
		return;
	}
	*/
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2 = ConcatArgs( 2 );

	if( strchr( arg2, ';' ) || strchr( arg2, '\n' ) || strchr( arg2, '\r' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote string contains forbidden characters.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->s.number;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !g_entities[i].inuse ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].info.netname, sizeof(netname));
					Q_CleanStr(netname);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}

		Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[0] ), "%s %d", arg1, i );
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i,
				va("print \"%s" S_COLOR_WHITE " called a team vote (Leader %s" S_COLOR_WHITE ")\n\"",
					ent->client->info.netname, level.clients[i].info.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team) {
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
			level.clients[i].pers.teamVote = VOTE_NONE;
		}
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;
	ent->client->pers.teamVote = VOTE_YES;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, "1" );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, "0" );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	team_t		team;
	vote_t		vote;
	int			cs_offset;
	char		msg[2];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOTEAMVOTEINPROG")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );
	vote = ( msg[0] == 'y' || msg[0] == 'Y' || msg[0] == '1' ) ? VOTE_YES : VOTE_NO;

	if ( vote != ent->client->pers.teamVote ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLTEAMVOTECAST")) );
		ent->client->pers.teamVote = vote;
		CalculateRanks();
	} else {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADYCAST")) );
	}
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

int G_ItemUsable(playerState_t *ps, holdable_t forcedUse)
{
	vec3_t fwd, fwdorg, dest, pos;
	vec3_t yawonly;
	vec3_t mins, maxs;
	vec3_t trtest;
	trace_t tr;

	if (ps->usingATST)
	{
		return 0;
	}

	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = (holdable_t)bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly[ROLL] = 0;
		yawonly[PITCH] = 0;
		yawonly[YAW] = ps->viewangles[YAW];

		VectorSet( mins, -8, -8, 0 );
		VectorSet( maxs, 8, 8, 24 );

		AngleVectors(yawonly, fwd, NULL, NULL);

		fwdorg[0] = ps->origin[0] + fwd[0]*64;
		fwdorg[1] = ps->origin[1] + fwd[1]*64;
		fwdorg[2] = ps->origin[2] + fwd[2]*64;

		trtest[0] = fwdorg[0] + fwd[0]*16;
		trtest[1] = fwdorg[1] + fwd[1]*16;
		trtest[2] = fwdorg[2] + fwd[2]*16;

		G_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins[0] = -8;
		mins[1] = -8;
		mins[2] = 0;

		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		AngleVectors (ps->viewangles, fwd, NULL, NULL);
		fwd[2] = 0;
		VectorMA(ps->origin, 64, fwd, dest);
		G_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9f && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			G_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	default:
		return 1;
	}
}

extern int saberOffSound;
extern int saberOnSound;

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	if (!saberOffSound || !saberOnSound)
	{
		saberOffSound = G_SoundIndex("sound/weapons/saber/saberoffquick.wav");
		saberOnSound = G_SoundIndex("sound/weapons/saber/saberon.wav");
	}

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered)
		{
			ent->client->ps.saberHolstered = qfalse;
			G_Sound(ent, CHAN_AUTO, saberOnSound);
		}
		else
		{
			ent->client->ps.saberHolstered = qtrue;
			G_Sound(ent, CHAN_AUTO, saberOffSound);

			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	forceLevel_t selectLevel = FORCE_LEVEL_0;

	if ( !ent || !ent->client )
	{
		return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	selectLevel = (forceLevel_t)(selectLevel + 1);
	if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABERATTACK] )
	{
		selectLevel = FORCE_LEVEL_1;
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->saberCycleQueue = selectLevel;
	}
}

void Cmd_EngageDuel_f(gentity_t *ent)
{
	gentity_t *challenged;

	if (!g_privateDuel.integer)
	{
		return;
	}

	if (level.gametype == GT_TOURNAMENT)
	{ //rather pointless in this mode..
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (GT_Team(level.gametype))
	{ //no private dueling in team modes
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

	/*
	if (!ent->client->ps.saberHolstered)
	{ //must have saber holstered at the start of the duel
		return;
	}
	*/
	//NOTE: No longer doing this..

	if (ent->client->ps.saberInFlight)
	{
		return;
	}

	if (ent->client->ps.duelInProgress)
	{
		return;
	}

	challenged = G_CrosshairPlayer(ent, 256);

	if (!challenged || !challenged->client || !challenged->inuse ||
		challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
		challenged->client->ps.weapon != WP_SABER || challenged->client->ps.saberInFlight)
	{
		return;
	}
	if (challenged->client->ps.duelInProgress) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "CANTDUEL_BUSY")) );
		return;
	}

	ent->client->ps.duelIndex = challenged->s.number;

	if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
	{
		unsigned dimension;

		char *s = va("print \"%s" S_COLOR_WHITE " %s %s" S_COLOR_WHITE "!\n\"", challenged->client->info.netname,
			G_GetStripEdString("SVINGAME", "PLDUELACCEPT"), ent->client->info.netname);
		trap_SendServerCommand(-1, s);

		dimension = G_GetFreeDuelDimension();
		ent->dimension = dimension;
		challenged->dimension = dimension;

		if (ent->client->ps.saberEntityNum != ENTITYNUM_NONE) {
			g_entities[ent->client->ps.saberEntityNum].dimension = dimension;
		}
		if (challenged->client->ps.saberEntityNum != ENTITYNUM_NONE) {
			g_entities[challenged->client->ps.saberEntityNum].dimension = dimension;
		}

		ent->client->ps.duelInProgress = qtrue;
		challenged->client->ps.duelInProgress = qtrue;

		ent->client->ps.duelTime = level.time + 2000;
		challenged->client->ps.duelTime = level.time + 2000;

		G_AddEvent(ent, EV_PRIVATE_DUEL, 1);
		G_AddEvent(challenged, EV_PRIVATE_DUEL, 1);

		//Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)

		if (!ent->client->ps.saberHolstered)
		{
			G_Sound(ent, CHAN_AUTO, saberOffSound);
			ent->client->ps.weaponTime = 400;
			ent->client->ps.saberHolstered = qtrue;
		}
		if (!challenged->client->ps.saberHolstered)
		{
			G_Sound(challenged, CHAN_AUTO, saberOffSound);
			challenged->client->ps.weaponTime = 400;
			challenged->client->ps.saberHolstered = qtrue;
		}

		ent->client->ps.stats[STAT_ARMOR] =
			ent->client->ps.stats[STAT_HEALTH] =
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		challenged->client->ps.stats[STAT_ARMOR] =
			challenged->client->ps.stats[STAT_HEALTH] =
			challenged->health = challenged->client->ps.stats[STAT_MAX_HEALTH];

		G_StartPrivateDuel(ent);
		G_StartPrivateDuel(challenged);
	}
	else
	{
		//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
		trap_SendServerCommand( challenged-g_entities, va("cp \"%s" S_COLOR_WHITE "\n%s\n\"", ent->client->info.netname, G_GetStripEdString("SVINGAME", "PLDUELCHALLENGE")) );
		trap_SendServerCommand( ent-g_entities, va("cp \"%s\n%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELCHALLENGED"), challenged->client->info.netname) );
		ent->client->ps.duelTime = level.time + 5000;
	}

	ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
	ent->client->ps.forceHandExtendTime = level.time + 1000;
}

static void Cmd_TheDestroyer_f(gentity_t *ent)
{
	if (!ent->client->ps.saberHolstered || ent->client->ps.weapon != WP_SABER)
		return;

	Cmd_ToggleSaber_f(ent);

	if (!ent->client->ps.saberHolstered)
	{
		if (ent->client->ps.dualBlade)
		{
			ent->client->ps.dualBlade = qfalse;
			//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
		}
		else
		{
			ent->client->ps.dualBlade = qtrue;

			trap_SendServerCommand( -1, va("print \"%sTHE DESTROYER COMETH\n\"", S_COLOR_RED) );
			G_ScreenShake(vec3_origin, NULL, 10.0f, 800, qtrue);
			//ent->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
		}
	}
}

static void Cmd_AddBot_f(gentity_t *ent)
{
	trap_SendServerCommand( ent-g_entities, va("print \"%s.\n\"", G_GetStripEdString("SVINGAME", "ONLY_ADD_BOTS_AS_SERVER")));
}

static void Cmd_RageQuit_f(gentity_t *ent)
{
	trap_DropClient( ent-g_entities, S_COLOR_RED "ragequit" );
}

static void Cmd_Ready_f(gentity_t *ent)
{
	gclient_t	*client = ent->client;

	if (!level.warmupTime) {
		return;
	}

	if (client->readyTime + 1000 > level.time) {
		G_SendServerCommand(ent-g_entities,
			"print \"May not use this command more than once per second.\n\"");
		return;
	}

	if (client->pers.ready) {
		client->pers.ready = qfalse;
		G_SendServerCommand(-1, "cp \"%s" S_COLOR_WHITE " is "
			S_COLOR_RED "not ready" S_COLOR_WHITE ".\n\"",
			client->info.netname);
	} else {
		client->pers.ready = qtrue;
		G_SendServerCommand(-1, "cp \"%s" S_COLOR_WHITE " is "
			S_COLOR_GREEN "ready" S_COLOR_WHITE ".\n\"",
			client->info.netname);
	}

	client->readyTime = level.time;
	G_UpdateClientReadyFlags();
}

static void Cmd_Timeout_f(gentity_t *ent)
{
	if (ent->client->pers.timeouts >= g_timeoutLimit.integer) {
		trap_SendServerCommand(ent-g_entities, "print \"You may not call any more timeouts this game.\n\"");
		return;
	}

	if (level.unpauseTime < level.time) {
		ent->client->pers.timeouts++;
		level.timeoutClient = ent->s.number;
		level.unpauseTime = level.time + 30000;
		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " called a timeout.\n\"", ent->client->info.netname));
	}
}

static void Cmd_Timein_f(gentity_t *ent)
{
	if (level.unpauseTime > level.time + 5000 && level.timeoutClient == ent->s.number) {
		level.unpauseTime = level.time + 5000;
		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " called a timein.\n\"", ent->client->info.netname));
	}
}

static void Cmd_Referee_f(gentity_t *ent)
{
	if (trap_Argc() < 2) {
		trap_SendServerCommand(ent-g_entities, "print \"Usage: referee <password>\n\"");
		return;
	}

	if (!strcmp(g_refereePassword.string, "") ||
		strcmp(g_refereePassword.string, ConcatArgs(1))) {
		trap_SendServerCommand(ent-g_entities, "print \"Incorrect password.\n\"");
		return;
	}

	trap_SendConsoleCommand(EXEC_APPEND, va("referee %d\n", ent->s.number));
}

#ifdef _DEBUG
void PM_SetAnim(int setAnimParts,int anim,unsigned setAnimFlags, int blendTime);
void ScorePlum( int clientNum, vec3_t origin, int score );

static void Cmd_DebugPlum_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int score = 30;

	if (argNum > 1)
	{
		trap_Argv( 1, arg, sizeof( arg ) );
		score = atoi(arg);
	}
	ScorePlum(self->s.number, self->client->ps.origin, score);
}

extern stringID_table_t animTable[MAX_ANIMATIONS+1];

static void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = (saberMoveName_t)atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = (saberMoveName_t)(LS_MOVE_MAX - 1);
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

static void Cmd_DebugSetBodyAnim_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;
	pmove_t pmv;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = G_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = level.gametype;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, i, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);

	Com_Printf("Set body anim to %s\n", arg);
}

void DismembermentTest(gentity_t *self);

static void Cmd_HeadExplodey_f(gentity_t *ent)
{
	Cmd_Kill_f (ent);
	if (ent->health < 1)
	{
		float presaveVel = ent->client->ps.velocity[2];
		ent->client->ps.velocity[2] = 500;
		DismembermentTest(ent);
		ent->client->ps.velocity[2] = presaveVel;
	}
}

static void StandardSetBodyAnim(gentity_t *self, int anim, unsigned flags)
{
	pmove_t pmv;

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = G_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = level.gametype;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, anim, flags, 0);
}

static void Cmd_LoveAndPeace_f(gentity_t *ent)
{
	trace_t tr;
	vec3_t fPos;

	AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

	fPos[0] = ent->client->ps.origin[0] + fPos[0]*40;
	fPos[1] = ent->client->ps.origin[1] + fPos[1]*40;
	fPos[2] = ent->client->ps.origin[2] + fPos[2]*40;

	G_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

	if (tr.entityNum < MAX_CLIENTS && tr.entityNum != ent->s.number)
	{
		gentity_t *other = &g_entities[tr.entityNum];

		if (other && other->inuse && other->client)
		{
			vec3_t entDir;
			vec3_t otherDir;
			vec3_t entAngles;
			vec3_t otherAngles;

			if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
			{
				Cmd_ToggleSaber_f(ent);
			}

			if (other->client->ps.weapon == WP_SABER && !other->client->ps.saberHolstered)
			{
				Cmd_ToggleSaber_f(other);
			}

			if ((ent->client->ps.weapon != WP_SABER || ent->client->ps.saberHolstered) &&
				(other->client->ps.weapon != WP_SABER || other->client->ps.saberHolstered))
			{
				VectorSubtract( other->client->ps.origin, ent->client->ps.origin, otherDir );
				VectorCopy( ent->client->ps.viewangles, entAngles );
				entAngles[YAW] = vectoyaw( otherDir );
				SetClientViewAngle( ent, entAngles );

				StandardSetBodyAnim(ent, BOTH_KISSER1LOOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
				ent->client->ps.saberMove = LS_NONE;
				ent->client->ps.saberBlocked = BLOCKED_NONE;
				ent->client->ps.saberBlocking = BLK_NO;

				VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
				VectorCopy( other->client->ps.viewangles, otherAngles );
				otherAngles[YAW] = vectoyaw( entDir );
				SetClientViewAngle( other, otherAngles );

				StandardSetBodyAnim(other, BOTH_KISSEE1LOOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
				other->client->ps.saberMove = LS_NONE;
				other->client->ps.saberBlocked = BLOCKED_NONE;
				other->client->ps.saberBlocking = BLK_NO;
			}
		}
	}
}

void DismembermentByNum(gentity_t *self, int num);

static void Cmd_DebugDismemberment_f(gentity_t *ent)
{
	Cmd_Kill_f (ent);
	if (ent->health < 1)
	{
		char	arg[MAX_STRING_CHARS];
		int		iArg = 0;

		if (trap_Argc() > 1)
		{
			trap_Argv( 1, arg, sizeof( arg ) );

			if (arg[0])
			{
				iArg = atoi(arg);
			}
		}

		DismembermentByNum(ent, iArg);
	}

}

static void Cmd_DebugKnockMeDown_f(gentity_t *ent)
{
	ent->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
	ent->client->ps.forceDodgeAnim = 0;
	if (trap_Argc() > 1)
	{
		ent->client->ps.forceHandExtendTime = level.time + 1100;
		ent->client->ps.quickerGetup = qfalse;
	}
	else
	{
		ent->client->ps.forceHandExtendTime = level.time + 700;
		ent->client->ps.quickerGetup = qtrue;
	}
}
#endif // _DEBUG

static int printfClientNum;
__attribute__ ((format (printf, 1, 2)))
static void G_CmdPrintf(const char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	G_SendServerCommand(printfClientNum, "print \"%s\"", text);
}
__attribute__ ((format (printf, 2, 3)))
static void G_CmdLogPrintf(int event, const char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	G_LogPrintf(event, "Referee: %d %s", printfClientNum, text);
}

#define CMD_NOINTERMISSION	0x01
#define CMD_CHEAT			0x02
#define CMD_ALIVE			0x04
#define CMD_REFEREE			0x08	// update these in cg_players.c::CG_RefereeMode

typedef struct {
	const char	*name;				// must be lower-case for comparing
	void		(*function)(gentity_t *);
	int			flags;				// allow during intermission
} clientCommand_t;

static const clientCommand_t commands[] = {
	{ "say", Cmd_Say_f, 0 },
	{ "say_team", Cmd_SayTeam_f, 0 },
	{ "tell", Cmd_Tell_f, 0 },
	{ "score", Cmd_Score_f, 0 },
	{ "kill", Cmd_Kill_f, CMD_ALIVE | CMD_NOINTERMISSION },
	{ "follow", Cmd_Follow_f, CMD_NOINTERMISSION },
	{ "follownext", Cmd_FollowNext_f, CMD_NOINTERMISSION },
	{ "followprev", Cmd_FollowPrev_f, CMD_NOINTERMISSION },
	{ "ready", Cmd_Ready_f, CMD_NOINTERMISSION },
	{ "team", Cmd_Team_f, CMD_NOINTERMISSION },
	{ "forcechanged", Cmd_ForceChanged_f, 0 },
	{ "where", Cmd_Where_f, 0 },
	{ "callvote", Cmd_CallVote_f, CMD_NOINTERMISSION },
	{ "vote", Cmd_Vote_f, CMD_NOINTERMISSION },
	{ "callteamvote", Cmd_CallTeamVote_f, CMD_NOINTERMISSION },
	{ "teamvote", Cmd_TeamVote_f, CMD_NOINTERMISSION },
	{ "ragequit", Cmd_RageQuit_f, 0 },
	{ "gc", Cmd_GameCommand_f, CMD_NOINTERMISSION },
	{ "timeout", Cmd_Timeout_f, CMD_ALIVE | CMD_NOINTERMISSION },
	{ "timein", Cmd_Timein_f, CMD_ALIVE | CMD_NOINTERMISSION },
	{ "referee", Cmd_Referee_f, 0 },
	{ "give", Cmd_Give_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "god", Cmd_God_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "notarget", Cmd_Notarget_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "noclip", Cmd_Noclip_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "setviewpos", Cmd_SetViewpos_f, CMD_CHEAT | CMD_NOINTERMISSION },
	{ "teamtask", Cmd_TeamTask_f, CMD_CHEAT | CMD_NOINTERMISSION },
	{ "levelshot", Cmd_LevelShot_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "thedestroyer", Cmd_TheDestroyer_f, CMD_CHEAT | CMD_ALIVE | CMD_NOINTERMISSION },
	{ "addbot", Cmd_AddBot_f, 0 },
#ifdef _DEBUG
	{ "headexplodey", Cmd_HeadExplodey_f, CMD_CHEAT },
	{ "g2animent", G_CreateExampleAnimEnt, CMD_CHEAT },
	{ "loveandpeace", Cmd_LoveAndPeace_f, CMD_CHEAT },
	{ "debugplum", Cmd_DebugPlum_f, CMD_CHEAT },
	{ "debugsetsabermove", Cmd_DebugSetSaberMove_f, CMD_CHEAT },
	{ "debugsetbodyanim", Cmd_DebugSetBodyAnim_f, CMD_CHEAT },
	{ "debugdismemberment", Cmd_DebugDismemberment_f, CMD_CHEAT },
	{ "debugknockmedown", Cmd_DebugKnockMeDown_f, CMD_CHEAT },
#endif
};

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	const clientCommand_t	*command = NULL;
	gentity_t				*ent;
	char					cmd[MAX_TOKEN_CHARS];
	unsigned				i;

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
		return;		// not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );
	Q_strlwr( cmd );

	//rww - redirect bot commands
	if (!strncmp(cmd, "bot_", 4) && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	// redirect referee commands
	printfClientNum = clientNum;
	ref.Printf = G_CmdPrintf;
	ref.LogPrintf = G_CmdLogPrintf;

	if (ent->client->sess.referee && RefereeCommand(cmd)) {
		return;
	}

	for ( i = 0; i < ARRAY_LEN(commands); i++ ) {
		if ( !strcmp( cmd, commands[i].name ) ) {
			command = &commands[i];
			break;
		}
	}

	if ( command == NULL ) {
		trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		return;
	}

	if ( command->flags & CMD_CHEAT ) {
		if ( !g_cheats.integer ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
			return;
		}
	}

	if ( command->flags & CMD_NOINTERMISSION ) {
		if ( level.intermissiontime || level.intermissionQueued ) {
			trap_SendServerCommand( clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd ) );
			return;
		}
	}

	if ( command->flags & CMD_ALIVE ) {
		if ( ent->health <= 0 || ent->client->sess.spectatorState != SPECTATOR_NOT ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
			return;
		}
	}

	if ( command->flags & CMD_REFEREE ) {
		if ( !ent->client->sess.referee ) {
			trap_SendServerCommand( clientNum, "print \"Only referees may use this command.\n\"" );
		}
	}

	command->function( ent );
}
