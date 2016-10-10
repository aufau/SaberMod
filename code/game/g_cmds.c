// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

#include "../../ui/menudef.h"			// for the voice chats

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

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		// perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;
		dead = ( cl->sess.spectatorState != SPECTATOR_NOT || cl->ps.stats[STAT_HEALTH] <= 0 );

		netDamage = cl->pers.totalDamageDealtToEnemies;
		netDamage -= cl->pers.totalDamageTakenFromEnemies;
		netDamage /= 100; // Don't send exact data

		Com_sprintf (entry, sizeof(entry),
			" %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
			level.sortedClients[i],
			cl->ps.persistant[PERS_SCORE],
			ping,
			(level.time - cl->pers.enterTime)/60000,
			scoreFlags,
			g_entities[level.sortedClients[i]].s.powerups,
			accuracy,
			cl->ps.persistant[PERS_KILLED],
			cl->ps.persistant[PERS_KILLS],
			netDamage,
//			cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
//			cl->ps.persistant[PERS_EXCELLENT_COUNT],
//			cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT],
			cl->ps.persistant[PERS_DEFEND_COUNT],
			cl->ps.persistant[PERS_ASSIST_COUNT],
			dead,
//			perfect,
			cl->ps.persistant[PERS_CAPTURES]);
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
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MUSTBEALIVE")));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

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
	char		*name;
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

		name = cl->pers.netname;

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
int G_ClientNumberFromString( const char *s, char **errorMsg ) {
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
		if ( cl->pers.connected != CON_CONNECTED ) {
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

	trap_Trace(&tr, org, NULL, NULL, fwdOrg, ent->s.number, MASK_PLAYERSOLID);

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
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;
	char		arg[MAX_TOKEN_CHARS];

	if ( !CheatsOk( ent ) ) {
		return;
	}

	trap_Argv( 1, name, sizeof( name ) );

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all)
	{
		// don't give HI_NONE, it's broken in cgame
		for (i = HI_NONE + 1; i < HI_NUM_HOLDABLE; i++)
		{
			if (i != HI_DATAPAD)
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
		}
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
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_EXCELLENT;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "impressive") == 0) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "gauntletaward") == 0 ||
		Q_stricmp(name, "humiliation") == 0) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		ent->client->ps.eFlags &= ~EF_AWARDS;
		ent->client->ps.eFlags |= EF_AWARD_GAUNTLET;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "denied") == 0) {
		ent->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
		ent->s.eFlags &= ~EF_AWARDS;
		ent->s.eFlags |= EF_AWARD_DENIED;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "capture") == 0) {
		ent->client->ps.persistant[PERS_CAPTURES]++;
		ent->s.eFlags &= ~EF_AWARDS;
		ent->s.eFlags |= EF_AWARD_CAP;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "defend") == 0) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		ent->s.eFlags &= ~EF_AWARDS;
		ent->s.eFlags |= EF_AWARD_DEFEND;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}
	if (Q_stricmp(name, "assist") == 0) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		ent->s.eFlags &= ~EF_AWARDS;
		ent->s.eFlags |= EF_AWARD_ASSIST;
		ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
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
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

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
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

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
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

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
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities,
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
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
	if ( ent->client->sess.spectatorState != SPECTATOR_NOT ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.numPlayingClients > 1 && !level.warmupTime)
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

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];

		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
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
			client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		for ( clientnum = 0; clientnum < level.maxclients; clientnum++ ) {
			if ( level.clients[ clientnum ].pers.connected == CON_CONNECTED
			     && !level.clients[ clientnum ].ps.duelInProgress ) {
				trap_SendServerCommand( clientnum, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHESPECTATORS")));
			}
		}
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (g_gametype.integer == GT_TOURNAMENT)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->pers.netname, G_GetStripEdString("SVINGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
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
					client->pers.netname, G_GetStripEdString("SVINGAME", "JOINEDTHEBATTLE")));
				}
			}
		}
	}

	G_LogPrintf ( LOG_TEAM_SWITCH, "SetTeam: %i %s %s: %s joined the %s team\n",
		client - &level.clients[0],
		teamNameUpperCase[oldTeam],
		teamNameUpperCase[client->sess.sessionTeam],
		client->pers.netname, teamNameLowerCase[client->sess.sessionTeam]);
}

/*
=================
SetTeamSpec

Doesn't handle switching from/to SPECTATOR_NOT state without team
change yet.
=================
*/
static qboolean SetTeamSpec( gentity_t *ent, team_t team, spectatorState_t specState, int specClient )
{
	gclient_t	*client;
	int			clientNum;
	team_t		oldTeam;
	int			teamLeader;

	client = ent->client;
	clientNum = client - level.clients;
	oldTeam = client->sess.sessionTeam;

	if ( client->sess.spectatorState == SPECTATOR_FOLLOW && specState != SPECTATOR_FOLLOW) {
		StopFollowing( ent );
	}

	// fast path for switching followed player
	if ( team == oldTeam) {
		client->sess.spectatorState = specState;
		client->sess.spectatorClient = specClient;
		return qfalse;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body
	CopyToBodyQue(ent);

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);

	}

	if ( team == TEAM_SPECTATOR ) {
		// they go to the end of the line for tournaments
		client->sess.spectatorTime = level.time;

		if ( client->ps.duelInProgress ) {
			client->sess.losses++;
		}
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->ps.fd.forceDoInit = 1; // every time we change teams make sure our force powers are set right

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
	ClientUserinfoChanged( clientNum );

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
	return SetTeamSpec( ent, team, state, 0 );
}

/*
=================
SetTeamFromString
=================
*/
static void SetTeamFromString( gentity_t *ent, char *s ) {
	int					team;
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
	} else if ( GT_Team(g_gametype.integer) ) {
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
	} else {
		team = TEAM_FREE;
		if ( !ValidateTeam(clientNum, TEAM_FREE) ) {
			trap_SendServerCommand( clientNum, "print \"Game is full.\n\""); // TRANSLATE
			return;
		}
	}

	if ( level.teamLock[team] ) {
		trap_SendServerCommand( clientNum, va("print \"%s%s" S_COLOR_WHITE " team is locked.\n\"",
				teamColorString[team], teamName[team]) );
		return;
	}

	if ( SetTeamSpec( ent, team, specState, 0 ) ) {
		ent->client->switchTeamTime = level.time + 5000;
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
	int			i;

	// don't allow team player free floating
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		Cmd_FollowCycle_f( ent, 1 );
		return;
	}

	ent->r.svFlags &= ~SVF_BOT;
	client->sess.sessionTeam = TEAM_SPECTATOR;
	client->sess.spectatorState = SPECTATOR_FREE;
	client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;
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
	client->ps.zoomMode = 0;
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
	client->ps.saberBlocked = 0;
	client->ps.saberBlocking = 0;
	client->ps.saberEntityNum = ENTITYNUM_NONE;
	for (i = 0; i < PW_NUM_POWERUPS; i++)
		client->ps.powerups[i] = 0;

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

	if ( ent->client->switchTeamTime > level.time ) {
		if ( ent->client->switchTeamTime - level.time > 5000 ) {
			trap_SendServerCommand( ent-g_entities,
				va("print \"You were removed. May not switch teams for %d seconds\n\"",
					(ent->client->switchTeamTime - level.time + 999) / 1000));
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

	trap_Argv( 1, s, sizeof( s ) );

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

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (g_gametype.integer == GT_TOURNAMENT)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
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
	char	*errorMsg;
	char	arg[MAX_TOKEN_CHARS];
	int		i;

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && level.teamLock[TEAM_SPECTATOR] ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s%s" S_COLOR_WHITE " team is locked.\n\"",
				teamColorString[TEAM_SPECTATOR], teamName[TEAM_SPECTATOR]) );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	if (!strcmp(arg, "-1") || !Q_stricmp(arg, "first")) {
		i = -1;
	} else if (!strcmp(arg, "-2") || !Q_stricmp(arg, "second"))
		i = -2;
	else {
		i = G_ClientNumberFromString( arg, &errorMsg );
		if ( i == -1 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}
	}

	if ( SetTeamSpec( ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, i) ) {
		ent->client->switchTeamTime = level.time + 5000;
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
			SetTeamSpec(ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, target->client->ps.clientNum);
			return;
		}
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = client->sess.spectatorClient;

	if (clientnum == -1) {
		clientnum = level.follow1;
	} else if (clientnum == -2) {
		clientnum = level.follow2;
	}
	if ( clientnum < 0 || clientnum >= level.maxclients ) {
		clientnum = 0;
	}

	// can't make yourself a following non-spectator here
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		teamRestrict = qtrue;
		team = client->sess.sessionTeam;
	} else {
		teamRestrict = qfalse;
		team = TEAM_SPECTATOR;
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

void Cmd_SmartFollowCycle_f( gentity_t *ent )
{
	gclient_t	*client = ent->client;
	gclient_t	*ci;
	qboolean	teamRestrict;
	team_t		followTeam;
	int			clientNum, clientRank;
	int			i;

	clientNum = -1;

	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		return;
	} else if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		clientNum = client->sess.spectatorClient;
	} else if ( client->sess.spectatorState == SPECTATOR_FREE ) {
		gentity_t	*target = G_CrosshairPlayer(ent, 8192);

		if ( target ) {
			SetTeamSpec(ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, target->client->ps.clientNum);
			return;
		}
	}

	if (clientNum == -1) {
		clientNum = level.follow1;
	} else if (clientNum == -2) {
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
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		teamRestrict = qtrue;
		followTeam = client->sess.sessionTeam;
	} else {
		teamRestrict = qfalse;
		followTeam = ci->sess.sessionTeam;
	}

	// Alternate between dueling players
	if ( ci->ps.duelInProgress ) {
		if ( !teamRestrict ||
			level.clients[ci->ps.duelIndex].sess.sessionTeam == followTeam )
		{
			SetTeamSpec( ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, ci->ps.duelIndex );
		}
		return;
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
			SetTeamSpec( ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, level.sortedClients[i] );
			// must return here, SetTeamSpec calls CalculateRanks!
			return;
		}
	} while (i != clientRank);

	if ( GT_Team(g_gametype.integer) ) {
		// Cycle through sorted team
		do {
			if (--i < 0) {
				i = level.numPlayingClients - 1;
			}
			ci = &level.clients[level.sortedClients[i]];

			if (ci->sess.spectatorState == SPECTATOR_NOT && ci->sess.sessionTeam == followTeam) {
				SetTeamSpec( ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, level.sortedClients[i] );
				return;
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
				SetTeamSpec( ent, client->sess.sessionTeam, SPECTATOR_FOLLOW, level.sortedClients[i] );
				return;
			}
		} while (i != clientRank);
	}
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
		G_LogPrintf( LOG_SAY, "Say: %i: %s: %s\n", ent->s.number, ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf( LOG_SAY_TEAM, "SayTeam: %i %s: %s: %s\n", ent->s.number,
			teamNameUpperCase[ent->client->sess.sessionTeam], ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && GT_Team(g_gametype.integer) &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
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
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
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

	if ( g_restrictChat.integer && mode == SAY_ALL && !level.warmupTime &&
		 ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		mode = SAY_TEAM;
	}

	G_Say( ent, NULL, mode, p );
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
	char		*errorMsg;
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
			ent->s.number, ent->s.number, ent->client->pers.netname, p );
		name = va(EC"[%s" S_COLOR_WHITE EC "]"EC": ", ent->client->pers.netname);
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
		if ( !level.warmupTime &&
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
		ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}


static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

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
	if (g_gametype.integer == GT_TOURNAMENT) {
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

	if ( !GT_Team(g_gametype.integer) && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
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
	char		*errorMsg;
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
		target->s.number, ent->client->pers.netname, target->client->pers.netname, id );
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

	if (GT_Team(g_gametype.integer)) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
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



static char	*gc_orders[] = {
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
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
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

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Single Player",
	"Team FFA",
	"N/A",
	"Capture the Flag",
	"Capture the Ysalamiri",
	"Red Rover",
	"Clan Arena",
};

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	voteCommand_t	voteCmd;
	int				i;
	char			arg1[MAX_STRING_TOKENS];
	const char		*arg2;
	char			*errorMsg;
	char			s[MAX_STRING_CHARS];

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE")) );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEINPROGRESS")) );
		return;
	}
	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXVOTES")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOSPECVOTE")) );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2 = ConcatArgs( 2 );

	if ( !Q_stricmp( arg1, "map_restart" ) )     voteCmd = CV_MAP_RESTART;
	else if ( !Q_stricmp( arg1, "nextmap" ) )    voteCmd = CV_NEXTMAP;
	else if ( !Q_stricmp( arg1, "map" ) )        voteCmd = CV_MAP;
	else if ( !Q_stricmp( arg1, "g_gametype" ) ) voteCmd = CV_GAMETYPE;
	else if ( !Q_stricmp( arg1, "gametype" ) )   voteCmd = CV_GAMETYPE;
	else if ( !Q_stricmp( arg1, "kick" ) )       voteCmd = CV_KICK;
	else if ( !Q_stricmp( arg1, "clientkick" ) ) voteCmd = CV_CLIENTKICK;
	else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) voteCmd = CV_DOWARMUP;
	else if ( !Q_stricmp( arg1, "timelimit" ) )  voteCmd = CV_TIMELIMIT;
	else if ( !Q_stricmp( arg1, "fraglimit" ) )  voteCmd = CV_FRAGLIMIT;
	else if ( !Q_stricmp( arg1, "roundlimit" ) ) voteCmd = CV_ROUNDLIMIT;
	else if ( !Q_stricmp( arg1, "teamsize" ) )   voteCmd = CV_TEAMSIZE;
	else if ( !Q_stricmp( arg1, "remove" ) )     voteCmd = CV_REMOVE;
	else if ( !Q_stricmp( arg1, "nk" ) )         voteCmd = CV_NOKICK;
	else if ( !Q_stricmp( arg1, "wk" ) )         voteCmd = CV_WITHKICK;
	else                                         voteCmd = CV_INVALID;

	if ( voteCmd == CV_INVALID ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, gametype <name>, kick <player>, clientkick <clientnum>, g_doWarmup <0|1>, timelimit <time>, fraglimit <frags>, roundlimit <rounds>, teamsize <size>, remove <player>, wk, nk.\n\"" );
		return;
	}

	if ( g_allowVote.integer != 1 ) {
		if ( ( ( 1 << voteCmd ) & g_allowVote.integer ) == 0 ) {
			trap_SendServerCommand( ent-g_entities, "print \"This vote has been disabled by the server administrator.\n\"");
			return;
		}
	}

	if( strchr( arg2, ';' ) || strchr( arg2, '\n' ) || strchr( arg2, '\r' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote string contains forbidden characters.\n\"" );
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}

	switch ( voteCmd ) {
	case CV_GAMETYPE:
		// special case for g_gametype, check for bad values
		if ( isdigit( arg2[0] ) ) {
			i = atoi( arg2 );
		} else {
			for (i = 0; i < GT_MAX_GAME_TYPE; i++) {
				if ( !Q_stricmp(gameNames[i], arg2) ||
					!Q_stricmp(machineGameNames[i], arg2) )
					break;
			}
		}

		if( i == GT_SINGLE_PLAYER || i == GT_SAGA || i < 0 || i >= GT_MAX_GAME_TYPE) {
			char		gametypes[MAX_PRINT_TEXT] = { 0 };
			qboolean	printSep = qfalse;
			char		*p;

			for (i = 0; i < GT_MAX_GAME_TYPE; i++) {
				if (i != GT_SINGLE_PLAYER && i != GT_SAGA) {
					if (printSep)
						Q_strcat(gametypes, sizeof(gametypes), ", ");
					Q_strcat(gametypes, sizeof(gametypes), machineGameNames[i]);
					printSep = qtrue;
				}
			}
			for (p = gametypes; *p != '\0'; p++)
				*p = tolower(*p);

			trap_SendServerCommand( ent-g_entities,
				va("print \"Valid gametypes are: %s\n\"", gametypes) );
			return;
		}

		level.votingGametype = qtrue;
		level.votingGametypeTo = i;

		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_gametype %d", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "gametype %s", gameNames[i] );
		break;
	case CV_MAP:
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation

		if (!G_DoesMapSupportGametype(arg2, trap_Cvar_VariableIntegerValue("g_gametype")))
		{
			//trap_SendServerCommand( ent-g_entities, "print \"You can't vote for this map, it isn't supported by the current gametype.\n\"" );
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME")) );
			return;
		}

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		break;
	case CV_CLIENTKICK:
		i = atoi ( arg2 );

		if ( i < 0 || i >= MAX_CLIENTS )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", i ) );
			return;
		}

		if ( g_entities[i].client->pers.connected == CON_DISCONNECTED )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", i ) );
			return;
		}

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[i].client->pers.netname );
		break;
	case CV_KICK:
		i = G_ClientNumberFromString( arg2, &errorMsg );

		if ( i == -1 )
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}

		Com_sprintf ( level.voteString, sizeof(level.voteString ), "clientkick %d", i );
		Com_sprintf ( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", g_entities[i].client->pers.netname );
		break;
	case CV_NEXTMAP:
		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (!*s) {
			trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		break;
	case CV_TEAMSIZE:
		i = atoi( arg2 );

		if ( i != 0 && g_teamsizeMin.integer > 0 && i < g_teamsizeMin.integer ) {
			trap_SendServerCommand( ent-g_entities,
				va("print \"teamsize must be greater than %d.\n\"",	g_teamsizeMin.integer - 1 ) );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		break;
	case CV_REMOVE:
		i = G_ClientNumberFromString( arg2, &errorMsg );

		if ( i == -1 ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\"", errorMsg) );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
			"%s %s", arg1, g_entities[i].client->pers.netname );
		break;
	case CV_NOKICK:
		i = atoi ( arg2 );

		if ( i < 1 ) {
			i = 3;
		}
		if ( i > 2 ) {
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"dmflags %d; g_friendlyFire 1; g_noKick 3; ", g_dmflags.integer | DF_NO_KICK );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ),
				"dmflags %d; g_friendlyFire 1; g_noKick %d; ", i, g_dmflags.integer & ~DF_NO_KICK );
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "No Kick %d", i );
		break;
	case CV_WITHKICK:
		Com_sprintf( level.voteString, sizeof( level.voteString ),
			"dmflags %d; g_friendlyFire 0; g_noKick 0", g_dmflags.integer & ~DF_NO_KICK );
		Q_strncpyz( level.voteDisplayString, "With Kick", sizeof( level.voteDisplayString ) );
		break;
	default:
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}

	trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLCALLEDVOTE") ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "VOTEALREADY")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLVOTECAST")) );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int			i, team, cs_offset;
	char		arg1[MAX_STRING_TOKENS];
	const char	*arg2;

	team = ent->client->sess.sessionTeam;
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
	if ( ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "MAXTEAMVOTES")) );
		return;
	}
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
			i = ent->client->ps.clientNum;
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
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
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
			trap_SendServerCommand( i, va("print \"%s" S_COLOR_WHITE " called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

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
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "TEAMVOTEALREADYCAST")) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOVOTEASSPEC")) );
		return;
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLTEAMVOTECAST")) );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
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

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NOCHEATS")));
		return;
	}
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



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}

int G_ItemUsable(playerState_t *ps, int forcedUse)
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
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
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

		trap_Trace(&tr, ps->origin, mins, maxs, trtest, ps->clientNum, MASK_PLAYERSOLID);

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
		trap_Trace(&tr, ps->origin, mins, maxs, dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(tr.endpos, pos);
			VectorSet( dest, pos[0], pos[1], pos[2] - 4096 );
			trap_Trace( &tr, pos, mins, maxs, dest, ps->clientNum, MASK_SOLID );
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
	int selectLevel = 0;

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

	selectLevel++;
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

	if (g_gametype.integer == GT_TOURNAMENT)
	{ //rather pointless in this mode..
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (GT_Team(g_gametype.integer))
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

	if (challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time)
	{
		char *s = va("print \"%s" S_COLOR_WHITE " %s %s!\n\"", challenged->client->pers.netname,
			G_GetStripEdString("SVINGAME", "PLDUELACCEPT"), ent->client->pers.netname);
		trap_SendServerCommand(-1, s);

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
	}
	else
	{
		//Print the message that a player has been challenged in private, only announce the actual duel initiation in private
		trap_SendServerCommand( challenged-g_entities, va("cp \"%s" S_COLOR_WHITE "\n%s\n\"", ent->client->pers.netname, G_GetStripEdString("SVINGAME", "PLDUELCHALLENGE")) );
		trap_SendServerCommand( ent-g_entities, va("cp \"%s\n%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELCHALLENGED"), challenged->client->pers.netname) );
		ent->client->ps.duelTime = level.time + 5000;
	}

	ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
	ent->client->ps.forceHandExtendTime = level.time + 1000;

	ent->client->ps.duelIndex = challenged->s.number;
}

void PM_SetAnim(int setAnimParts,int anim,int setAnimFlags, int blendTime);

#ifdef _DEBUG
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
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

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
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
	pmv.trace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, i, flags, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	pmove_t pmv;

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &self->client->ps;
	pmv.animations = bgGlobalAnimations;
	pmv.cmd = self->client->pers.cmd;
	pmv.trace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	pm = &pmv;
	PM_SetAnim(SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t *self);

#ifdef _DEBUG
void DismembermentByNum(gentity_t *self, int num);
#endif

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char	cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
		return;		// not fully in game yet
	}


	trap_Argv( 0, cmd, sizeof( cmd ) );

	//rww - redirect bot commands
	if (strstr(cmd, "bot_") && AcceptBotCommand(cmd, ent))
	{
		return;
	}
	//end rww

	if (Q_stricmp (cmd, "say") == 0) {
		Cmd_Say_f (ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0) {
		Cmd_Say_f (ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "tell") == 0) {
		Cmd_Tell_f ( ent );
		return;
	}
	if (Q_stricmp (cmd, "vsay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vsay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qfalse);
		return;
	}
	if (Q_stricmp (cmd, "vtell") == 0) {
		Cmd_VoiceTell_f ( ent, qfalse );
		return;
	}
	if (Q_stricmp (cmd, "vosay") == 0) {
		Cmd_Voice_f (ent, SAY_ALL, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "vosay_team") == 0) {
		Cmd_Voice_f (ent, SAY_TEAM, qfalse, qtrue);
		return;
	}
	if (Q_stricmp (cmd, "votell") == 0) {
		Cmd_VoiceTell_f ( ent, qtrue );
		return;
	}
	if (Q_stricmp (cmd, "vtaunt") == 0) {
		Cmd_VoiceTaunt_f ( ent );
		return;
	}
	if (Q_stricmp (cmd, "score") == 0) {
		Cmd_Score_f (ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime)
	{
		qboolean giveError = qfalse;

		if (!Q_stricmp(cmd, "give"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "god"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "notarget"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "noclip"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "kill"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamtask"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "levelshot"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follow"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "follownext"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "followprev"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "team"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "forcechanged"))
		{ //special case: still update force change
			Cmd_ForceChanged_f (ent);
			return;
		}
		else if (!Q_stricmp(cmd, "where"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "vote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "callteamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "teamvote"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "gc"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "setviewpos"))
		{
			giveError = qtrue;
		}
		else if (!Q_stricmp(cmd, "stats"))
		{
			giveError = qtrue;
		}

		if (giveError)
		{
			trap_SendServerCommand( clientNum, va("print \"You cannot perform this task (%s) during the intermission.\n\"", cmd ) );
		}
		else
		{
			Cmd_Say_f (ent, qfalse, qtrue);
		}
		return;
	}

	if (Q_stricmp (cmd, "give") == 0)
	{
		Cmd_Give_f (ent);
	}
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "teamtask") == 0)
		Cmd_TeamTask_f (ent);
	else if (Q_stricmp (cmd, "levelshot") == 0)
		Cmd_LevelShot_f (ent);
	else if (Q_stricmp (cmd, "follow") == 0)
		Cmd_Follow_f (ent);
	else if (Q_stricmp (cmd, "follownext") == 0)
		Cmd_FollowCycle_f (ent, 1);
	else if (Q_stricmp (cmd, "followprev") == 0)
		Cmd_FollowCycle_f (ent, -1);
	else if (Q_stricmp (cmd, "team") == 0)
		Cmd_Team_f (ent);
	else if (Q_stricmp (cmd, "forcechanged") == 0)
		Cmd_ForceChanged_f (ent);
	else if (Q_stricmp (cmd, "where") == 0)
		Cmd_Where_f (ent);
	else if (Q_stricmp (cmd, "callvote") == 0)
		Cmd_CallVote_f (ent);
	else if (Q_stricmp (cmd, "vote") == 0)
		Cmd_Vote_f (ent);
	else if (Q_stricmp (cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f (ent);
	else if (Q_stricmp (cmd, "teamvote") == 0)
		Cmd_TeamVote_f (ent);
	else if (Q_stricmp (cmd, "gc") == 0)
		Cmd_GameCommand_f( ent );
	else if (Q_stricmp (cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f( ent );
	else if (Q_stricmp (cmd, "stats") == 0)
		Cmd_Stats_f( ent );
	/*
	else if (Q_stricmp(cmd, "#mm") == 0 && CheatsOk( ent ))
	{
		G_PlayerBecomeATST(ent);
	}
	*/
	//I broke the ATST when I restructured it to use a single global anim set for all client animation.
	//You can fix it, but you'll have to implement unique animations (per character) again.
#ifdef _DEBUG //sigh..
	else if (Q_stricmp(cmd, "headexplodey") == 0 && CheatsOk( ent ))
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
	else if (Q_stricmp(cmd, "g2animent") == 0 && CheatsOk( ent ))
	{
		G_CreateExampleAnimEnt(ent);
	}
	else if (Q_stricmp(cmd, "loveandpeace") == 0 && CheatsOk( ent ))
	{
		trace_t tr;
		vec3_t fPos;

		AngleVectors(ent->client->ps.viewangles, fPos, 0, 0);

		fPos[0] = ent->client->ps.origin[0] + fPos[0]*40;
		fPos[1] = ent->client->ps.origin[1] + fPos[1]*40;
		fPos[2] = ent->client->ps.origin[2] + fPos[2]*40;

		trap_Trace(&tr, ent->client->ps.origin, 0, 0, fPos, ent->s.number, ent->clipmask);

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
					ent->client->ps.saberBlocked = 0;
					ent->client->ps.saberBlocking = 0;

					VectorSubtract( ent->client->ps.origin, other->client->ps.origin, entDir );
					VectorCopy( other->client->ps.viewangles, otherAngles );
					otherAngles[YAW] = vectoyaw( entDir );
					SetClientViewAngle( other, otherAngles );

					StandardSetBodyAnim(other, BOTH_KISSEE1LOOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS);
					other->client->ps.saberMove = LS_NONE;
					other->client->ps.saberBlocked = 0;
					other->client->ps.saberBlocking = 0;
				}
			}
		}
	}
#endif
	else if (Q_stricmp(cmd, "thedestroyer") == 0 && CheatsOk( ent ) && ent && ent->client && ent->client->ps.saberHolstered && ent->client->ps.weapon == WP_SABER)
	{
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
#ifdef _DEBUG
	else if (Q_stricmp(cmd, "debugSetSaberMove") == 0)
	{
		Cmd_DebugSetSaberMove_f(ent);
	}
	else if (Q_stricmp(cmd, "debugSetBodyAnim") == 0)
	{
		Cmd_DebugSetBodyAnim_f(ent, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
	}
	else if (Q_stricmp(cmd, "debugDismemberment") == 0)
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
	else if (Q_stricmp(cmd, "debugKnockMeDown") == 0)
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
#endif

	else
	{
		if (Q_stricmp(cmd, "addbot") == 0)
		{ //because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
//			trap_SendServerCommand( clientNum, va("print \"You can only add bots as the server.\n\"" ) );
			trap_SendServerCommand( clientNum, va("print \"%s.\n\"", G_GetStripEdString("SVINGAME", "ONLY_ADD_BOTS_AS_SERVER")));
		}
		else
		{
			trap_SendServerCommand( clientNum, va("print \"unknown cmd %s\n\"", cmd ) );
		}
	}
}
