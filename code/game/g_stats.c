/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

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

typedef enum {
	STATS_SCORE,
	STATS_KILLS,
	STATS_CAPS,
	STATS_DEFEND,
	STATS_ASSIST,
	STATS_ACC,		// accuracy
	STATS_IMPRESSIVE,
	STATS_DMG,		// damage dealt to enemies
	STATS_NET_DMG,	// netto damage
	STATS_MAX_ASC = STATS_NET_DMG,
	// following stats are 'better' when lower
	STATS_KILLED,	// died
	STATS_RCV,		// damage received from enemies
	STATS_TDMG,		// damage dealt to teammates
	STATS_TRCV,		// damage received from teammates
	STATS_MAX
} playerStat_t;

typedef struct {
	const char * const	label;
	const char * const	shortLabel;
	const int			width;
	qboolean			disabled;
} statColumn_t;

#define STATS_COL_WIDTH 6

// Keep this in the same order as playerStat_t
static statColumn_t statCol[STATS_MAX] = {
	{ "Score",			"S", 4 },
	{ "Kills",			"K", 3 },
	{ "Captures",		"Cap", 3 },
	{ "Defends",		"Def", 3 },
	{ "Assists",		"Ast", 3 },
	{ "Accuracy",		"Acc", 3 },
	{ "Impressive",		"Imp", 3 },
	{ "Damage",			"Dmg", 5 },
	{ "Net Damage",		"Net", 5 },
	{ "Deaths",			"D", 3 },
	{ "Received",		"Rcv", 5 },
	{ "Team Damage",	"TDmg", 5 },
	{ "Team Received",	"TRcv", 5 },
};

static const playerStat_t logColumns[] =
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_CAPS, STATS_DEFEND, STATS_ASSIST, STATS_DMG, STATS_RCV, STATS_TDMG, STATS_TRCV, STATS_IMPRESSIVE, STATS_ACC };
// all other columns must end with STATS_MAX
static const playerStat_t ffaColumns[] =
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_DMG, STATS_RCV, STATS_NET_DMG, STATS_ACC, STATS_MAX };
static const playerStat_t ctfColumns[] =
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_CAPS, STATS_DEFEND, STATS_ASSIST, STATS_DMG, STATS_ACC, STATS_MAX };
static const playerStat_t tffaColumns[] = // 47 characters
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_DMG, STATS_RCV, STATS_TDMG, STATS_TRCV, STATS_NET_DMG, STATS_ACC, STATS_MAX };
static const playerStat_t iffaColumns[] =
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_IMPRESSIVE, STATS_ACC, STATS_MAX };
static const playerStat_t ictfColumns[] =
{ STATS_SCORE, STATS_KILLS, STATS_KILLED, STATS_CAPS, STATS_DEFEND, STATS_ASSIST, STATS_IMPRESSIVE, STATS_ACC, STATS_MAX };

static void GetStats( int *stats, gclient_t *cl )
{
	const int *persistant = cl->pers.persistant;

	stats[STATS_SCORE] = persistant[PERS_SCORE];
	stats[STATS_CAPS] = persistant[PERS_CAPTURES];
	stats[STATS_DEFEND] = persistant[PERS_DEFEND_COUNT];
	stats[STATS_ASSIST] = persistant[PERS_ASSIST_COUNT];
	if (cl->pers.accuracy_shots > 0)
		stats[STATS_ACC] = 100.0f * cl->pers.accuracy_hits / cl->pers.accuracy_shots;
	else
		stats[STATS_ACC] = 0;
	stats[STATS_IMPRESSIVE] = persistant[PERS_IMPRESSIVE_COUNT];
	stats[STATS_KILLED] = persistant[PERS_KILLED];
	stats[STATS_KILLS] = persistant[PERS_KILLS];
	stats[STATS_NET_DMG] = cl->pers.totalDamageDealtToEnemies
		- cl->pers.totalDamageTakenFromEnemies;
	stats[STATS_DMG] = cl->pers.totalDamageDealtToEnemies;
	stats[STATS_RCV] = cl->pers.totalDamageTakenFromEnemies;
	stats[STATS_TDMG] = cl->pers.totalDamageDealtToAllies;
	stats[STATS_TRCV] = cl->pers.totalDamageTakenFromAllies;
}

static void PrintStatsHeader( const playerStat_t *columns )
{
	char		line[DEFAULT_CONSOLE_WIDTH + 1];
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			pad;
	int			i;

	pad = MAX_NAME_LEN - STRLEN("Name");
	p += Com_sprintf(p, e - p, "Name%s", Spaces(pad));

	for (i = 0; columns[i] != STATS_MAX; i++) {
		playerStat_t stat = columns[i];

		if (statCol[stat].disabled)
			continue;

		pad = statCol[stat].width - strlen(statCol[stat].shortLabel);
		p += Com_sprintf(p, e - p, " %s%s", statCol[stat].shortLabel, Spaces(pad));
	}

	trap_SendServerCommand(-1, va("print \"%s\n\"", line));

}

static void PrintStatsSeparator( const playerStat_t *columns, const char *colorString )
{
	char		line[DEFAULT_CONSOLE_WIDTH + 1];
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			i;

	p += Com_sprintf(p, e - p, "%s", Dashes(MAX_NAME_LEN));

	for (i = 0; columns[i] != STATS_MAX; i++) {
		playerStat_t stat = columns[i];

		if (statCol[stat].disabled)
			continue;

		p += Com_sprintf(p, e - p, " %s", Dashes(statCol[stat].width));
	}

	trap_SendServerCommand(-1,
		va("print \"%s%s\n\"", colorString, line));
}

static void PrintClientStats( gclient_t *cl, const playerStat_t *columns, const int *bestStats )
{
	char		line[2 * DEFAULT_CONSOLE_WIDTH + 1]; // extra space for color codes
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			stats[STATS_MAX];
	size_t		pad;
	int			i;

	GetStats(stats, cl);

	pad = MAX_NAME_LEN - Q_PrintStrlen(cl->info.netname);
	p += Com_sprintf(p, e - p, "%s%s" S_COLOR_WHITE, cl->info.netname, Spaces(pad));

	for (i = 0; columns[i] != STATS_MAX; i++) {
		playerStat_t stat = columns[i];
		const char *value = va("%i", stats[stat]);
		int len = strlen(value);

		if (statCol[stat].disabled)
			continue;

		if (stats[stat] >= 1000 && len > statCol[stat].width) {
			value = va("%ik", stats[stat] / 1000);
			len = strlen(value);
		}
		if (len > statCol[stat].width) {
			value = "";
			len = 0;
		}

		pad = statCol[stat].width - len;
		if (stats[stat] == bestStats[stat]) {
			p += Com_sprintf(p, e - p, S_COLOR_GREEN " %s%s" S_COLOR_WHITE, value, Spaces(pad));
		} else {
			p += Com_sprintf(p, e - p, " %s%s", value, Spaces(pad));
		}
	}

	trap_SendServerCommand(-1, va("print \"%s\n\"", line));
}

void G_PrintStats(void) {
	const playerStat_t	*columns;
	gclient_t			*cl;
	int					stats[STATS_MAX];
	int					bestStats[STATS_MAX];
	qboolean			reallyBest[STATS_MAX] = { qfalse };
	int					i, j;

	if (level.numPlayingClients == 0) {
		return;
	}

	cl = &level.clients[level.sortedClients[0]];
	GetStats(bestStats, cl);

	for (i = 1; i < level.numPlayingClients; i++) {
		cl = &level.clients[level.sortedClients[i]];
		GetStats(stats, cl);

		for (j = 0; j <= STATS_MAX_ASC; j++) {
			if (stats[j] != bestStats[j]) {
				reallyBest[j] = qtrue;
			}
			if (stats[j] > bestStats[j]) {
				bestStats[j] = stats[j];
			}
		}
		for (; j < STATS_MAX; j++) {
			if (stats[j] != bestStats[j]) {
				reallyBest[j] = qtrue;
			}
			if (stats[j] < bestStats[j]) {
				bestStats[j] = stats[j];
			}
		}
	}

	// Don't highlight the stat if it's the same for all players
	for (j = 0; j < STATS_MAX; j++) {
		if (!reallyBest[j]) {
			bestStats[j] = INT_MAX;
		}
	}

	// disable stats irrelevant in current gametype. column arrays can
	// be removed at this point, but i'd rather keep line length under
	// firm control
	statCol[STATS_ACC].disabled = HasSetSaberOnly();
	statCol[STATS_IMPRESSIVE].disabled = (qboolean)!!(g_spawnWeapons.integer & WP_DISRUPTOR);

	trap_SendServerCommand(-1, "print \"\n\"");

	if (GT_Team(level.gametype) && level.gametype != GT_REDROVER) {
		if (level.gametype == GT_CTF) {
			if (g_instagib.integer)
				columns = ictfColumns;
			else
				columns = ctfColumns;
		} else {
			if (g_instagib.integer)
				columns = iffaColumns;
			else
				columns = tffaColumns;
		}

		PrintStatsHeader(columns);

		PrintStatsSeparator(columns, BG_TeamColor(TEAM_RED));
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			if (cl->sess.sessionTeam == TEAM_RED)
				PrintClientStats(cl, columns, bestStats);
		}

		PrintStatsSeparator(columns, BG_TeamColor(TEAM_BLUE));
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			if (cl->sess.sessionTeam == TEAM_BLUE) {
				PrintClientStats(cl, columns, bestStats);
			}
		}
	} else {
		if (g_instagib.integer) {
			columns = iffaColumns;
		} else if (level.gametype == GT_REDROVER) {
			columns = tffaColumns;
		} else {
			columns = ffaColumns;
		}

		PrintStatsHeader(columns);

		PrintStatsSeparator(columns, BG_TeamColor(TEAM_FREE));
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			PrintClientStats(cl, columns, bestStats);
		}
	}

	trap_SendServerCommand(-1, "print \"\n\"");

}

static void G_LogStatsHeader(void)
{
	char header[STRLEN("SH: Num")
		+ ARRAY_LEN(logColumns) * (STATS_COL_WIDTH + 1)
		+ 1 + STRLEN("Team")
		+ 1 + STRLEN("Name\n") + 1];
	unsigned i;

	Q_strncpyz(header, "SH: Num", sizeof(header));
	for (i = 0; i < ARRAY_LEN(logColumns); i++) {
		const statColumn_t	*col = statCol + logColumns[i];
		const char			*label;

		if (col->label && strlen(col->label) <= STATS_COL_WIDTH) {
			label = col->label;
		} else {
			label = col->shortLabel;
		}

		Q_strcat(header, sizeof(header), va(" %s%s",
				Spaces(STATS_COL_WIDTH - strlen(label)), label));
	}
	G_LogPrintf(LOG_GAME_STATS, "%s %s %s\n", header, "Team", "Name");
}

static void G_LogStatsRow(int clientNum)
{
	char row[STRLEN("SR: Num")
		+ ARRAY_LEN(logColumns) * (STATS_COL_WIDTH + 1)
		+ 1 + STRLEN("Team")
		+ 1 + MAX_NETNAME + 1];
	int			stats[STATS_MAX];
	gclient_t	*client = level.clients + clientNum;
	unsigned 	i;

	GetStats(stats, client);

	Com_sprintf(row, sizeof(row), "SR: %3i", clientNum);
	for (i = 0; i < ARRAY_LEN(logColumns); i++) {
		Q_strcat(row, sizeof(row),
			va(" %" STR(STATS_COL_WIDTH) "i", stats[logColumns[i]]));
	}

	G_LogPrintf(LOG_GAME_STATS, "%s %-4s %s\n", row,
		BG_TeamName(client->sess.sessionTeam, CASE_UPPER),
		client->info.netname);
}

void G_LogStats(void)
{
	int		i;

	G_LogStatsHeader();

	for (i = 0; i < level.numPlayingClients; i++) {
		G_LogStatsRow(level.sortedClients[i]);
	}
}
