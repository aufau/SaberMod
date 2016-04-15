#include <g_local.h>

typedef enum {
	STAT_SCORE,
	STAT_KILLS,
	STAT_CAPS,
	STAT_DEFEND,
	STAT_ASSIST,
	STAT_DMG,
	STAT_NET_DMG,
	STAT_MAX_ASC = STAT_NET_DMG,
	// following stats are 'better' when lower
	STAT_KILLED,
	STAT_RCV,
	STAT_TDMG,
	STAT_TRCV,
	STAT_MAX
} playerStat_t;

typedef struct {
	const char * const	label;
	const char * const	shortLabel;
	int					width;
} statColumn_t;

#define STAT_COL_WIDTH 6

// Keep this in the same order as playerStat_t
const statColumn_t statCol[STAT_MAX] = {
	{ "Score",			"S", 3 },
	{ "Kills",			"K", 3 },
	{ "Captures",		"Cap", 3 },
	{ "Defends",		"Def", 3 },
	{ "Assists",		"Ast", 3 },
	{ "Damage",			"Dmg", 5 },
	{ "Net Damage",		"NetD", 5 },
	{ "Deaths",			"D", 3 },
	{ "Received",		"Rcv", 5 },
	{ "Team Damage",	"TDmg", 5 },
	{ "Team Received",	"TRcv", 5 },
};

static playerStat_t logColumns[] =
{ STAT_SCORE, STAT_KILLS, STAT_KILLED, STAT_CAPS, STAT_DEFEND, STAT_ASSIST, STAT_DMG, STAT_RCV, STAT_TDMG, STAT_TRCV };
static playerStat_t ffaColumns[] =
{ STAT_SCORE, STAT_KILLS, STAT_KILLED, STAT_DMG, STAT_RCV, STAT_NET_DMG, STAT_MAX };
static playerStat_t ctfColumns[] =
{ STAT_SCORE, STAT_CAPS, STAT_DEFEND, STAT_ASSIST, STAT_KILLS, STAT_KILLED, STAT_DMG, STAT_RCV, STAT_TDMG, STAT_TRCV, STAT_NET_DMG, STAT_MAX };
static playerStat_t tffaColumns[] =
{ STAT_SCORE, STAT_KILLS, STAT_KILLED, STAT_DMG, STAT_RCV, STAT_TDMG, STAT_TRCV, STAT_NET_DMG, STAT_MAX };

static void GetStats( int *stats, gclient_t *cl )
{
	stats[STAT_SCORE] = cl->ps.persistant[PERS_SCORE];
	stats[STAT_CAPS] = cl->ps.persistant[PERS_CAPTURES];
	stats[STAT_DEFEND] = cl->ps.persistant[PERS_DEFEND_COUNT];
	stats[STAT_ASSIST] = cl->ps.persistant[PERS_ASSIST_COUNT];
	stats[STAT_KILLED] = cl->ps.persistant[PERS_KILLED];
	stats[STAT_KILLS] = cl->ps.persistant[PERS_KILLS];
	stats[STAT_NET_DMG] = cl->pers.totalDamageDealtToEnemies
		- cl->pers.totalDamageTakenFromEnemies;
	stats[STAT_DMG] = cl->pers.totalDamageDealtToEnemies;
	stats[STAT_RCV] = cl->pers.totalDamageTakenFromEnemies;
	stats[STAT_TDMG] = cl->pers.totalDamageDealtToAllies;
	stats[STAT_TRCV] = cl->pers.totalDamageTakenFromAllies;
}

static void PrintStatsHeader( playerStat_t *columns )
{
	char		line[DEFAULT_CONSOLE_WIDTH];
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			pad;
	int			i;

	pad = MAX_NAME_LEN - STRLEN("Name");
	p += Com_sprintf(p, e - p, "Name%s", Spaces(pad));

	for (i = 0; columns[i] != STAT_MAX; i++) {
		playerStat_t stat = columns[i];

		pad = statCol[stat].width - strlen(statCol[stat].shortLabel);
		p += Com_sprintf(p, e - p, " %s%s", statCol[stat].shortLabel, Spaces(pad));
	}

	trap_SendServerCommand(-1, va("print \"%s\n\"", line));

}

static void PrintStatsSeparator( playerStat_t *columns, const char *colorString )
{
	char		line[DEFAULT_CONSOLE_WIDTH];
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			i;

	p += Com_sprintf(p, e - p, Dashes(MAX_NAME_LEN));

	for (i = 0; columns[i] != STAT_MAX; i++) {
		playerStat_t stat = columns[i];

		p += Com_sprintf(p, e - p, " %s", Dashes(statCol[stat].width));
	}

	trap_SendServerCommand(-1,
		va("print \"%s%s\n\"", colorString, line));
}

static void PrintClientStats( gclient_t *cl, playerStat_t *columns, int *bestStats )
{
	char		line[2 * DEFAULT_CONSOLE_WIDTH]; // extra space for color codes
	char		*p = line;
	const char	*e = line + sizeof(line);
	int			stats[STAT_MAX];
	int			pad;
	int			i;

	GetStats(stats, cl);

	pad = MAX_NAME_LEN - Q_PrintStrlen(cl->pers.netname);
	p += Com_sprintf(p, e - p, "%s%s" S_COLOR_WHITE, cl->pers.netname, Spaces(pad));

	for (i = 0; columns[i] != STAT_MAX; i++) {
		playerStat_t stat = columns[i];
		char *value = va("%i", stats[stat]);
		int len = strlen(value);

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
	playerStat_t	*columns;
	gclient_t		*cl;
	int				stats[STAT_MAX];
	int				bestStats[STAT_MAX];
	qboolean		reallyBest[STAT_MAX] = { qfalse };
	int				i, j;

	if (level.numPlayingClients == 0) {
		return;
	}

	cl = &level.clients[level.sortedClients[0]];
	GetStats(bestStats, cl);

	for (i = 1; i < level.numPlayingClients; i++) {
		cl = &level.clients[level.sortedClients[i]];
		GetStats(stats, cl);

		for (j = 0; j <= STAT_MAX_ASC; j++) {
			if (stats[j] != bestStats[j]) {
				reallyBest[j] = qtrue;
			}
			if (stats[j] > bestStats[j]) {
				bestStats[j] = stats[j];
			}
		}
		for (; j < STAT_MAX; j++) {
			if (stats[j] != bestStats[j]) {
				reallyBest[j] = qtrue;
			}
			if (stats[j] < bestStats[j]) {
				bestStats[j] = stats[j];
			}
		}
	}

	// Don't highlight the stat if it's the same for all players
	for (j = 0; j < STAT_MAX; j++) {
		if (!reallyBest[j]) {
			bestStats[j] = INT_MAX;
		}
	}

	trap_SendServerCommand(-1, "print \"\n\"");

	if (GT_Team(g_gametype.integer)) {
		if (g_gametype.integer == GT_CTF) {
			columns = ctfColumns;
		} else {
			columns = tffaColumns;
		}

		PrintStatsHeader(columns);

		PrintStatsSeparator(columns, teamColorString[TEAM_RED]);
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			if (cl->sess.sessionTeam == TEAM_RED)
				PrintClientStats(cl, columns, bestStats);
		}

		PrintStatsSeparator(columns, teamColorString[TEAM_BLUE]);
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			if (cl->sess.sessionTeam == TEAM_BLUE) {
				PrintClientStats(cl, columns, bestStats);
			}
		}
	} else {
		columns = ffaColumns;

		PrintStatsHeader(columns);

		PrintStatsSeparator(columns, teamColorString[TEAM_FREE]);
		for (i = 0; i < level.numPlayingClients; i++) {
			cl = level.clients + level.sortedClients[i];
			if (cl->sess.sessionTeam == TEAM_FREE) {
				PrintClientStats(cl, columns, bestStats);
			}
		}
	}

	trap_SendServerCommand(-1, "print \"\n\"");

}

static void G_LogStatsHeader(void)
{
	char header[STRLEN("SH: Num")
		+ ARRAY_LEN(logColumns) * (STAT_COL_WIDTH + 1)
		+ 1 + STRLEN("Team")
		+ 1 + STRLEN("Name\n") + 1];
	int  i;

	Q_strncpyz(header, "SH: Num", sizeof(header));
	for (i = 0; i < ARRAY_LEN(logColumns); i++) {
		const statColumn_t	*col = statCol + logColumns[i];
		const char			*label;

		if (col->label && strlen(col->label) <= STAT_COL_WIDTH) {
			label = col->label;
		} else {
			label = col->shortLabel;
		}

		Q_strcat(header, sizeof(header), va(" %s%s",
				Spaces(STAT_COL_WIDTH - strlen(label)), label));
	}
	G_LogPrintf(LOG_GAME_STATS, "%s %s %s\n", header, "Team", "Name");
}

static void G_LogStatsRow(int clientNum)
{
	char row[STRLEN("SR: Num")
		+ ARRAY_LEN(logColumns) * (STAT_COL_WIDTH + 1)
		+ 1 + STRLEN("Team")
		+ 1 + MAX_NETNAME + 1];
	int			stats[STAT_MAX];
	gclient_t	*client = level.clients + clientNum;
	int			i;

	GetStats(stats, client);

	Com_sprintf(row, sizeof(row), "SR: %3i", clientNum);
	for (i = 0; i < ARRAY_LEN(logColumns); i++) {
		Q_strcat(row, sizeof(row),
			va(" %" STR(STAT_COL_WIDTH) "i", stats[logColumns[i]]));
	}

	G_LogPrintf(LOG_GAME_STATS, "%s %-4s %s\n", row,
		teamNameUpperCase[client->sess.sessionTeam],
		client->pers.netname);
}

void G_LogStats(void)
{
	int		i;

	G_LogStatsHeader();

	for (i = 0; i < level.numPlayingClients; i++) {
		G_LogStatsRow(level.sortedClients[i]);
	}
}
