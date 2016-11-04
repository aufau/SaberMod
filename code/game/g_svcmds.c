/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2016 Witold Pilat <witold.pilat@gmail.com>

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
qboolean G_FilterPacket (char *from)
{
	int			i;
	unsigned	mask = 0;
	byte		*m = (byte *)&mask;
	char		*p;

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
			return g_filterBan.integer != 0;

	return g_filterBan.integer == 0;
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
	char		*errorMsg;
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
		G_Printf( errorMsg );
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	team = G_TeamFromLetter( str[0] );
	if ( team != TEAM_NUM_TEAMS ) {
		ent = g_entities + clientNum;
		SetTeam( ent, team );
		ent->client->switchTeamTime = level.time + 5000;
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
					teamColorString[team], teamName[team], prefix) );
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
	char		*errorMsg;
	int			clientNum;
	int			delay;

	if ( trap_Argc() < 2 ) {
		trap_Print( "Usage: remove <player> [seconds]\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	clientNum = G_ClientNumberFromString( str, &errorMsg );
	if ( clientNum == -1 ) {
		G_Printf( errorMsg );
		return;
	}

	trap_Argv( 2, str, sizeof( str ) );
	delay = 1000 * atoi( str );
	if ( delay == 0 ) {
		delay = 30 * 1000;
	}

	ent = g_entities + clientNum;
	// make him dedicated spectator so he doesn't join the queue if inactive
	if ( g_gametype.integer == GT_TOURNAMENT )
		SetTeamSpec( ent, TEAM_SPECTATOR, SPECTATOR_FOLLOW, -1 );
	else
		SetTeam( ent, TEAM_SPECTATOR );
	// overwrite "joined the spectators" message
	trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " was removed from battle\n\"", ent->client->pers.netname) );
	ent->client->switchTeamTime = level.time + delay;
}

/*
===================
Svcmd_Announce_f

announce "<message>"
===================
*/
void	Svcmd_Announce_f( void )
{
	char	*str = ConcatArgs(1);

	if ( !str[0] ) {
		trap_Print( "Usage: announce <message>\n" );
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\n\"", Q_SanitizeStr(str)));
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

	if ( !Q_stricmp(mode, "default") )
		mode = g_modeDefault.string;

	if ( trap_FS_FOpenFile( va( "modes/%s.cfg", mode), &f, FS_READ) < 0 ) {
		G_Printf( "Invalid mode.\n" );
		return;
	}
	trap_FS_FCloseFile( f );

	G_LogPrintf( LOG_GAME, "Mode: %s\n", mode );
	trap_SendServerCommand( -1, va("print \"Changing mode to %s.\n\"", mode) );
	trap_Cvar_Set( "g_mode", mode );
	trap_SendConsoleCommand( EXEC_APPEND, va("exec \"modes/%s\"\n", mode) );
	// we don't realy know what gametype it's going to be and admin
	// can set nextmap in mode config.
	trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
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
	char		*message;
	char		*errorMsg;
	int			clientNum;

	if ( trap_Argc() < 2 ) {
		trap_Print( "Usage: tell <player> [message]\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );
	clientNum = G_ClientNumberFromString( str, &errorMsg );
	if ( clientNum == -1 ) {
		G_Printf( errorMsg );
		return;
	}

	message = ConcatArgs( 2 );
	G_LogPrintf( LOG_TELL, "Tell: %i %i: server to %s: %s\n", clientNum, clientNum,
		level.clients[clientNum].pers.netname, message );
	trap_SendServerCommand( clientNum, va("chat \"[server]: %s\"", message) );
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

