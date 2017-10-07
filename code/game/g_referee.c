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
