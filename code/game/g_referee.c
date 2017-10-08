/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 2015-2017 Witold Pilat <witold.pilat@gmail.com>

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

// g_referee.c -- exclusive referee commands

void Cmd_Referee_f(gentity_t *ent) {
	const char	*errorMsg;
	gclient_t	*client = ent->client;
	int			targetNum;
	char		arg[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	targetNum = G_ClientNumberFromString(arg, &errorMsg);
	if (targetNum == -1) {
		trap_SendServerCommand(ent->s.number, va("print \"%s\"", errorMsg));
		return;
	}

	if (!client->sess.referee) {
		client->sess.referee = qtrue;
		ClientUpdateConfigString(ent->s.number);
		G_SendServerCommand(-1, "print \"%s" S_COLOR_WHITE " became a referee\n\"",
			client->info.netname);
	}
}

void Cmd_UnReferee_f(gentity_t *ent) {
	if (ent->client->sess.referee) {
		ent->client->sess.referee = qfalse;
		ClientUpdateConfigString(ent->s.number);
		G_SendServerCommand(-1, "print \"%s" S_COLOR_WHITE " is not a referee anymore\n\"",
			ent->client->info.netname);
	}
}

void Cmd_LockTeam_f(gentity_t *ent) {
	qboolean	lock;
	const char	*prefix;
	char		str[MAX_TOKEN_CHARS];
	team_t		team;
	int			argc = trap_Argc();
	int			i;

	trap_Argv(0, str, sizeof(str));

	lock = (qboolean)!Q_stricmp(str, "lockteam");
	prefix = lock ? "" : "un";

	if (argc < 2) {
		G_SendServerCommand(ent->s.number, "print \"Usage: %slockteam <teams>\n\"", prefix);
		return;
	}

	for (i = 1; i < argc; i++) {
		trap_Argv( i, str, sizeof( str ) );

		team = BG_TeamFromString( str );
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

void Cmd_ForceTeam_f(gentity_t *ent) {
	char		str[MAX_TOKEN_CHARS];
	const char	*errorMsg;
	int			clientNum;
	int			lastClient;
	team_t		team;

	if (trap_Argc() < 3) {
		G_SendServerCommand(ent->s.number, "print \""
			"Usage: forceteam <player> <team>\n"
			"       forceteam all <team>\n" "\"");
		return;
	}
	// find the player
	trap_Argv(1, str, sizeof(str));
	if (!strcmp(str, "all")) {
		clientNum = 0;
		lastClient = level.maxclients - 1;
	} else {
		clientNum = G_ClientNumberFromString(str, &errorMsg);
		if (clientNum == -1) {
			G_SendServerCommand(ent->s.number, "print \"%s\"", errorMsg);
			return;
		}
		lastClient = clientNum;
	}

	// set the team
	trap_Argv(2, str, sizeof(str));
	team = BG_TeamFromString(str);
	if (team == TEAM_NUM_TEAMS) {
		return;
	}

	for (; clientNum <= lastClient; clientNum++) {
		gentity_t *targEnt = &g_entities[clientNum];

		if (targEnt->inuse) {
			SetTeam( targEnt, team );
			targEnt->client->prof.switchTeamTime = level.time + 5000;
		}
	}
}

void Cmd_Announce_f(gentity_t *ent) {
	char	*str = ConcatArgs(1);

	if (!str[0]) {
		trap_SendServerCommand(ent->s.number, "print \"Usage: announce <message|motd>\n\"");
		return;
	}

	if (!Q_stricmp(str, "motd")) {
		trap_SendServerCommand(-1, "motd");
	} else {
		G_CenterPrintPersistant(Q_SanitizeStr(str));
	}
}
