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

#include "g_local.h"

/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;
	int			wins, losses;

	if ( level.gametype == GT_TOURNAMENT ) {
		wins = client->sess.wins;
		losses = client->sess.losses;
	} else {
		wins = 0;
		losses = 0;
	}

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i",
		client->sess.sessionTeam,
		client->sess.spectatorNum,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		wins,
		losses,
		client->sess.teamLeader,
		client->sess.setForce,
		client->sess.saberLevel,
		client->sess.selectedFP,
		client->sess.motdSeen,
		client->sess.referee
		);

	var = va( "session%i", (int)(client - level.clients) );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;
	int setForce;
	int saberLevel;
	int selectedFP;
	int motdSeen;
	int referee;

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorNum,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,                   // bk010221 - format
		&setForce,
		&saberLevel,
		&selectedFP,
		&motdSeen,
		&referee
		);

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
	client->sess.setForce = (qboolean)setForce;
	client->sess.saberLevel = (forceLevel_t)saberLevel;
	client->sess.selectedFP = (forcePowers_t)selectedFP;
	client->sess.motdSeen = (qboolean)motdSeen;
	client->sess.referee = (qboolean)referee;

	client->ps.fd.saberAnimLevel = client->sess.saberLevel;
	client->ps.fd.forcePowerSelected = client->sess.selectedFP;

	if ( level.gametype != GT_TOURNAMENT ) {
		client->sess.wins = 0;
		client->sess.losses = 0;
	}
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean isBot, qboolean firstTime ) {
	clientSession_t	*sess;
	const char		*value;
	int				clientNum;
	qboolean		referee;

	sess = &client->sess;
	clientNum = client - level.clients;
	referee = sess->referee;

	memset(sess, 0, sizeof(*sess));

	if ( !firstTime ) {
		sess->referee = referee;
	}

	// initial team determination
	if ( GT_Team(level.gametype) ) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
		} else {
			// always spawn as spectator in team games
			if (!isBot)
			{
				sess->sessionTeam = TEAM_SPECTATOR;
			}
			else
			{ //Bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if (value[0] == 'r' || value[0] == 'R')
				{
					sess->sessionTeam = ValidateTeam( -1, TEAM_RED ) ?
						TEAM_RED : TEAM_SPECTATOR;
				}
				else if (value[0] == 'b' || value[0] == 'B')
				{
					sess->sessionTeam = ValidateTeam( -1, TEAM_BLUE ) ?
						TEAM_BLUE : TEAM_SPECTATOR;
				}
				else if (value[0] == 's' || value[0] == 'S')
				{
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else
				{
					sess->sessionTeam = PickTeam( -1 );
				}
			}
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( level.gametype ) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
			case GT_SINGLE_PLAYER:
				sess->sessionTeam = ValidateTeam( clientNum, TEAM_FREE ) ?
					TEAM_FREE : TEAM_SPECTATOR;
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	if ( sess->sessionTeam == TEAM_SPECTATOR )
		sess->spectatorState = SPECTATOR_FREE;
	else if ( level.round > 0 && !level.roundQueued && level.gametype != GT_REDROVER )
		sess->spectatorState = SPECTATOR_FREE;
	else
		sess->spectatorState = SPECTATOR_NOT;

	AddTournamentQueue( client );

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( (int)level.gametype != gt ) {
		level.newSession = qtrue;
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", level.gametype) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
