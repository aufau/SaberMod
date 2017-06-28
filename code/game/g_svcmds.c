/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2017 Witold Pilat <witold.pilat@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
================================================================================
*/

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

// extern	vmCvar_t	g_banIPs;
// extern	vmCvar_t	g_filterBan;


typedef struct ipFilter_s
{
	unsigned	mask;
	unsigned	compare;
} ipFilter_t;

#define	MAX_IPFILTERS	1024

static ipFilter_t	ipFilters[MAX_IPFILTERS];
static int			numIPFilters;


char	*ConcatArgs( int start );

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, ipFilter_t *f)
{
	char		num[128];
	int			i, j;
	unsigned	compare = 0;
	unsigned	mask = 0;
	byte		*c = (byte *)&compare;
	byte		*m = (byte *)&mask;

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		c[i] = atoi(num);
		if (c[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = mask;
	f->compare = compare;

	return qtrue;
}

/*
=================
G_StringToIPv4
=================
*/
qipv4_t G_StringToIPv4(const char *s)
{
	qipv4_t	ip;
	char	c;
	int		i;

	if ( !strcmp( s, "localhost" ) )
	{
		ip.b[0] = 127;
		ip.b[1] = 0;
		ip.b[2] = 0;
		ip.b[3] = 1;

		return ip;
	}

	for ( i = 0; i < 4; i++ )
	{
		unsigned	b = 0;

		c = *s;
		while ( isdigit( c ) ) {
			b = b * 10 + (c - '0');
			s++;
			c = *s;
		}

		if ( b > 0xffu ) {
			ip.ui = 0;
			return ip;
		}

		ip.b[i] = b;
		s++;

		if ( c != '.' )
			break;
	}

	if ( i != 3 )
		ip.ui = 0;
	if ( c != '\0' && c != ':' )
		ip.ui = 0;

	return ip;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans (void)
{
	byte		*c;
	int			i;
	char		iplist[MAX_INFO_STRING];

	*iplist = 0;
	for (i = 0 ; i < numIPFilters ; i++)
	{
		if (ipFilters[i].compare == 0xffffffff)
			continue;

		c = (byte *)&ipFilters[i].compare;
		Com_sprintf( iplist + strlen(iplist), sizeof(iplist) - strlen(iplist),
			"%i.%i.%i.%i ", c[0], c[1], c[2], c[3]);
	}

	trap_Cvar_Set( "g_banIPs", iplist );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket (const char *from)
{
	int			i;
	unsigned	mask = 0;
	byte		*m = (byte *)&mask;
	const char	*p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	for (i=0 ; i<numIPFilters ; i++)
		if ( (mask & ipFilters[i].mask) == ipFilters[i].compare)
			return (qboolean)(g_filterBan.integer != 0);

	return (qboolean)(g_filterBan.integer == 0);
}

/*
=================
AddIP
=================
*/
static void AddIP( char *str )
{
	int		i;

	for (i = 0 ; i < numIPFilters ; i++)
		if (ipFilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numIPFilters)
	{
		if (numIPFilters == MAX_IPFILTERS)
		{
			G_Printf ("IP filter list is full\n");
			return;
		}
		numIPFilters++;
	}

	if (!StringToFilter (str, &ipFilters[i]))
		ipFilters[i].compare = 0xffffffffu;

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void)
{
	char *s, *t;
	char		str[MAX_TOKEN_CHARS];

	Q_strncpyz( str, g_banIPs.string, sizeof(str) );

	for (t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr(s, ' ');
		if (!s)
			break;
		while (*s == ' ')
			*s++ = 0;
		if (*t)
			AddIP( t );
		t = s;
	}
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f (void)
{
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f (void)
{
	ipFilter_t	f;
	int			i;
	char		str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf("Usage:  sv removeip <ip-mask>\n");
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if (!StringToFilter (str, &f))
		return;

	for (i=0 ; i<numIPFilters ; i++) {
		if (ipFilters[i].mask == f.mask	&&
			ipFilters[i].compare == f.compare) {
			ipFilters[i].compare = 0xffffffffu;
			G_Printf ("Removed.\n");

			UpdateIPBans();
			return;
		}
	}

	G_Printf ( "Didn't find %s.\n", str );
}

/*
===================
Svcmd_EntityList_f
===================
*/
void	Svcmd_EntityList_f (void) {
	int			e;
	gentity_t		*check;

	check = g_entities+1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if ( !check->inuse ) {
			continue;
		}
		G_Printf("%3i:", e);
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_GRAPPLE:
			G_Printf("ET_GRAPPLE          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}

		if ( check->classname ) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

static team_t G_TeamFromLetter( const char letter )
{
	switch (letter) {
	case 's':
	case 'S':
		return TEAM_SPECTATOR;
	case 'f':
	case 'F':
		return TEAM_FREE;
	case 'r':
	case 'R':
		return TEAM_RED;
	case 'b':
	case 'B':
		return TEAM_BLUE;
	default:
		G_Printf( "Valid teams are: spectator free red blue\n" );
		return TEAM_NUM_TEAMS;
	}
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void	Svcmd_ForceTeam_f( void ) {
	char		str[MAX_TOKEN_CHARS];
	const char	*errorMsg;
	int			clientNum;
	gentity_t	*ent;
	team_t		team;

	if ( trap_Argc() < 3 ) {
		G_Printf( "Usage: forceteam <player> <team>\n" );
		return;
	}
	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	clientNum = G_ClientNumberFromString( str, &errorMsg );
	if ( clientNum == -1 ) {
		trap_Print( errorMsg );
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	team = G_TeamFromLetter( str[0] );
	if ( team != TEAM_NUM_TEAMS ) {
		ent = g_entities + clientNum;
		SetTeam( ent, team );
		ent->client->prof.switchTeamTime = level.time + 5000;
	}
}

/*
===================
Svcmd_LockTeam_f

lockteam <teams>
===================
*/
void	Svcmd_LockTeam_f( qboolean lock )
{
	const char	*prefix = lock ? "" : "un";
	char		str[MAX_TOKEN_CHARS];
	team_t		team;
	int			argc = trap_Argc();
	int			i;

	if ( argc < 2 ) {
		G_Printf( "Usage: %slockteam <teams>\n", prefix );
		return;
	}

	for (i = 1; i < argc; i++) {
		trap_Argv( i, str, sizeof( str ) );

		team = G_TeamFromLetter( str[0] );
		if ( team == TEAM_NUM_TEAMS ) {
			return;
		}

		if (level.teamLock[team] != lock) {
			level.teamLock[team] = lock;
			trap_SendServerCommand( -1, va("print \"%s%s" S_COLOR_WHITE " team was %slocked.\n\"",
					BG_TeamColor(team), BG_TeamName(team, CASE_NORMAL), prefix) );
		}
	}
}

/*
===================
Svcmd_Remove_f

remove <player> [seconds]
===================
*/
void	Svcmd_Remove_f( void )
{
	gentity_t	*ent;
	char		str[MAX_TOKEN_CHARS];
	const char	*errorMsg;
	int			clientNum;
	int			delay;

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage: remove <player> [seconds]\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	clientNum = G_ClientNumberFromString( str, &errorMsg );
	if ( clientNum == -1 ) {
		trap_Print( errorMsg );
		return;
	}

	trap_Argv( 2, str, sizeof( str ) );
	delay = 1000 * atoi( str );
	if ( delay == 0 ) {
		delay = 30 * 1000;
	}

	ent = g_entities + clientNum;
	// make him dedicated spectator so he doesn't join the queue if inactive
	if ( level.gametype == GT_TOURNAMENT )
		SetTeamSpec( ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, -1 );
	else
		SetTeam( ent, TEAM_SPECTATOR );
	// overwrite "joined the spectators" message
	trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " was removed from battle\n\"", ent->client->info.netname) );
	ent->client->prof.switchTeamTime = level.time + delay;
}

static void G_CenterPrintPersistant( const char *str ) {
	const char	*cmd[2];
	int			i;

	cmd[0] = va( "cp \"%s\"", str );
	cmd[1] = va( "cpp \"%s\"", str );

	for ( i = 0; i < level.maxclients; i++ ) {
		const gclient_t	*client = &level.clients[i];

		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		trap_SendServerCommand( i, cmd[client->pers.registered] );
	}
}

/*
===================
Svcmd_Announce_f

announce "<message|motd>"
===================
*/
void	Svcmd_Announce_f( void )
{
	char	*str = ConcatArgs(1);

	if ( !str[0] ) {
		G_Printf( "Usage: announce <message|motd>\n" );
		return;
	}

	if ( !Q_stricmp( str, "motd" ) ) {
		trap_SendServerCommand( -1, "motd" );
	} else {
		G_CenterPrintPersistant( Q_SanitizeStr( str ) );
	}
}

/*
===================
Svcmd_Mode_f

mode <mode|default>
===================
*/
static void	Svcmd_Mode_f( void )
{
	const char		*mode;
	char			map[MAX_QPATH];
	qboolean		setMap;
	fileHandle_t	f;

	if ( trap_Argc() < 2 ) {
		char	modes[MAX_INFO_STRING];
		char	*ptr = modes;

		G_Printf( "Usage: mode <mode|default>\n" );
		G_Printf( "Available modes:\n" );
		trap_GetConfigstring( CS_MODES, modes, sizeof( modes ) );

		do {
			if ( *ptr == '\\' )
				*ptr = '\n';
		} while ( *ptr++ );

		trap_Print( modes );
		return;
	}

	mode = ConcatArgs(1);
	setMap = qfalse;

	if ( !Q_stricmp(mode, "default") ) {
		mode = g_modeDefault.string;
		trap_Cvar_VariableStringBuffer( "g_modeDefaultMap", map, sizeof( map ) );

		if ( strcmp( map, "" ) != 0 && strcmp( map, "0" ) != 0 && strcmp( map, "none" ) != 0 )
			setMap = qtrue;
	}

	if ( trap_FS_FOpenFile( va( "modes/%s.cfg", mode), &f, FS_READ) < 0 ) {
		G_Printf( "Invalid mode.\n" );
		return;
	}
	trap_FS_FCloseFile( f );

	G_LogPrintf( LOG_GAME, "Mode: %s\n", mode );
	trap_SendServerCommand( -1, va("print \"Changing mode to %s.\n\"", mode) );
	trap_Cvar_Set( "g_mode", mode );
	trap_SendConsoleCommand( EXEC_APPEND, va("exec \"modes/%s\"\n", mode) );

	if ( setMap ) {
		trap_SendConsoleCommand( EXEC_APPEND, va( "map \"%s\"\n", map ) );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}
}

/*
===================
Svcmd_Tell_f

tell <player> [message]
===================
*/
void	Svcmd_Tell_f( void )
{
	char		str[MAX_TOKEN_CHARS];
	const char	*message;
	const char	*errorMsg;
	int			clientNum;

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage: tell <player> [message]\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );
	clientNum = G_ClientNumberFromString( str, &errorMsg );
	if ( clientNum == -1 ) {
		trap_Print( errorMsg );
		return;
	}

	message = ConcatArgs( 2 );
	G_LogPrintf( LOG_TELL, "Tell: %i %i: server to %s: %s\n", clientNum, clientNum,
		level.clients[clientNum].info.netname, message );
	trap_SendServerCommand( clientNum, va("chat \"[server]: %s\"", message) );
}

/*
===================
Svcmd_Shuffle_f

Shuffle teams. Simillar to a card shuffle (riffle). Randomly selected
half of players from team RED go to team BLUE and vice-versa. This
method guarantees that there is a noticeable team change, minimizes
chance of getting previous teams in consecutive calls and balances
team counts.
===================
*/
static void	Svcmd_Shuffle_f( void )
{
	qboolean	change[MAX_CLIENTS] = { qfalse };
	int			count[TEAM_NUM_TEAMS] = { 0 };
	team_t		first, second, team;
	int			i;

	if ( !GT_Team( level.gametype ) ) {
		return;
	}

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			count[level.clients[i].sess.sessionTeam]++;
		}
	}

	if ( count[TEAM_RED] > count[TEAM_BLUE] ) {
		first = TEAM_RED;
	} else if ( count[TEAM_RED] < count[TEAM_BLUE] ) {
		first = TEAM_BLUE;
	} else {
		first = ( rand() & 1 ) ? TEAM_RED : TEAM_BLUE;
	}

	second = otherTeam[first];

	team = first;
	while ( 1 ) {
		int		changed = 0;
		int		left = count[team];
		int		changeNum = count[team] / 2;

		for ( i = 0; i < level.maxclients; i++ ) {
			gclient_t	*client = &level.clients[i];

			if ( changed >= changeNum ) {
				break;
			}

			if ( client->pers.connected != CON_DISCONNECTED &&
				client->sess.sessionTeam == team )
			{
				left--;

				if ( change[i] ) {
					continue;
				}

				if ( changed + left < changeNum ||
					irand( 1, count[team] ) <= changeNum )
				{
					changed++;
					change[i] = qtrue;
					client->sess.sessionTeam = otherTeam[team];
					client->sess.teamLeader = qfalse;
				}
			}
		}

		if ( team == second ) {
			break;
		}
		team = second;
	}

	CheckTeamLeader( TEAM_RED );
	CheckTeamLeader( TEAM_BLUE );

	team = first;
	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t	*client = &level.clients[i];

		if ( client->pers.connected != CON_DISCONNECTED ) {
			if ( change[i] ) {
				ClientUserinfoChanged( i );

				if ( client->pers.connected == CON_CONNECTED ) {
					ClientBegin( i, qfalse );
				}
			}
		}
	}

	CalculateRanks();

	trap_SendServerCommand( -1, "cp \"Shuffled teams.\"" );
}

static void Svcmd_Players_f( void ) {
	int		now = trap_Milliseconds();
	int		i;

	G_Printf( "num client      cgame    fps  packets team name\n" );
	G_Printf( "--- ----------- -------- ---- ------- ---- %s\n", Dashes( MAX_NAME_LEN ) );

	for ( i = 0; i < level.maxclients; i++ ) {
		gclient_t	*client = &level.clients[i];
		char		userinfo[MAX_INFO_VALUE];
		char		clientVersion[MAX_INFO_VALUE];
		const char	*cgame;
		const char	*value;
		int			lastCmdTime;
		int			lastThinkTime = 0;
		int			packets = 0;
		int			fps = 0;
		int			j;

		if ( client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		trap_GetUserinfo( i, userinfo, sizeof(userinfo) );

		value = Info_ValueForKey(userinfo, GAMEVERSION);
		if ( value[0] ) {
			cgame = value;
		} else {
			cgame = "";
		}

		value = Info_ValueForKey(userinfo, "JK2MV");
		if ( value[0] ) {
			Com_sprintf( clientVersion, sizeof( clientVersion ), "JK2MV %s", value );
		} else {
			Com_sprintf( clientVersion, sizeof( clientVersion ), "" );
		}

		lastCmdTime = client->cmdStats[client->cmdIndex & CMD_MASK].serverTime;
		for ( j = client->cmdIndex; ((client->cmdIndex - j + 1) & CMD_MASK) != 0; j-- ) {
			ucmdStat_t *stat = &client->cmdStats[j & CMD_MASK];

			if ( stat->serverTime + 1000 >= lastCmdTime ) {
				fps++;
			}

			if ( stat->thinkTime + 1000 >= now ) {
				if ( stat->thinkTime != lastThinkTime ) {
					lastThinkTime = stat->thinkTime;
					packets++;
				}
			}
		}

		G_Printf( "%3d %-11.11s %-8.8s %4d %7d %-4.4s %s\n",
			i,
			clientVersion,
			cgame,
			fps,
			packets,
			BG_TeamName( client->sess.sessionTeam, CASE_NORMAL ),
			client->info.netname );
	}
}

/*
=================
ConsoleCommand

=================
*/
qboolean	ConsoleCommand( void ) {
	char	cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp (cmd, "entitylist") == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "forceteam") == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "addbot") == 0) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "botlist") == 0) {
		Svcmd_BotList_f();
		return qtrue;
	}

/*	if (Q_stricmp (cmd, "abort_podium") == 0) {
		Svcmd_AbortPodium_f();
		return qtrue;
	}
*/
	if (Q_stricmp (cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "listip") == 0) {
		trap_SendConsoleCommand( EXEC_NOW, "g_banIPs\n" );
		return qtrue;
	}

	if (Q_stricmp (cmd, "announce") == 0) {
		Svcmd_Announce_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "remove") == 0) {
		Svcmd_Remove_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "lockteam") == 0) {
		Svcmd_LockTeam_f( qtrue );
		return qtrue;
	}

	if (Q_stricmp (cmd, "unlockteam") == 0) {
		Svcmd_LockTeam_f( qfalse );
		return qtrue;
	}

	if (Q_stricmp (cmd, "mode") == 0) {
		Svcmd_Mode_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "shuffle") == 0) {
		Svcmd_Shuffle_f();
		return qtrue;
	}

	if (Q_stricmp (cmd, "players") == 0) {
		Svcmd_Players_f();
		return qtrue;
	}

	if (g_dedicated.integer) {
		if (Q_stricmp (cmd, "say") == 0) {
			char *message = ConcatArgs(1);

			G_LogPrintf( LOG_SAY, "Say: server: %s\n", message );
			// we're missing control character \x19
			trap_SendServerCommand( -1, va("chat \"server: %s\"", message ) );
			return qtrue;
		}

		if (Q_stricmp (cmd, "tell") == 0) {
			Svcmd_Tell_f();
			return qtrue;
		}

		G_Printf( "Unknown command. Use \\say or \\tell to communicate with players.\n" );
		return qtrue;
	}

	return qfalse;
}

