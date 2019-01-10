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

// cg_scoreboard -- draw the scoreboard on top of the game screen

#include "cg_local.h"
#include "../ui/ui_shared.h"

#define	SCOREBOARD_X		(0.5f * cgs.screenWidth - 320.0f)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	25
#define SB_INTER_HEIGHT		15 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved

#define SB_RIGHT_MARKER_X	(SCOREBOARD_X+68)
#define SB_FLAGICON_X		(SCOREBOARD_X+68)

#define SB_SCORELINE_X		(SCOREBOARD_X+100)
#define SB_SCORELINE_WIDTH	(cgs.screenWidth - SB_SCORELINE_X * 2)

#define SB_RATING_WIDTH	    0 // (6 * BIGCHAR_WIDTH)

#define SB_SCALE			0.75f
#define SB_SCALE_LARGE		1.0f

#define SB_MIN_PADDING		6
#define SB_MAX_PADDING		30

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed

typedef enum {
	SBC_SCORE,
	SBC_CAP,
	SBC_AST,
	SBC_DEF,
	SBC_TIME,
	SBC_PING,
	SBC_W_L,
	SBC_W_L_SM,
	SBC_K_D,
	SBC_NET_DMG,
	SBC_MAX
} sbColumn_t;

typedef struct {
	float		scale;
	float		drop;
	float		width[2];
} sbColumnData_t;

static sbColumnData_t columnData[SBC_MAX];

static const sbColumn_t ffaColumns[] = { SBC_SCORE, SBC_K_D, SBC_NET_DMG, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t iffaColumns[] = { SBC_SCORE, SBC_K_D, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t ffaDuelColumns[] = { SBC_SCORE, SBC_W_L_SM, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t duelColumns[] = { SBC_SCORE, SBC_W_L, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t duelFraglimit1Columns[] = { SBC_W_L, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t ctfColumns[] = { SBC_SCORE, SBC_K_D, SBC_CAP, SBC_AST, SBC_DEF, SBC_PING, SBC_TIME, SBC_MAX };
static const sbColumn_t caColumns[] = { SBC_SCORE, SBC_K_D, SBC_PING, SBC_TIME, SBC_MAX };

static void CG_InitScoreboardColumn(sbColumn_t field, const char *label, const char *maxValue, float scale)
{
	sbColumnData_t	*column = columnData + field;
	float			labelW, fieldW;

	column->scale = scale;
	column->drop = CG_Text_Height(maxValue, 1.0f, FONT_SMALL)
		- CG_Text_Height(maxValue, scale, FONT_SMALL);

	labelW = CG_Text_Width(label, 1.0f, FONT_MEDIUM);
	fieldW = CG_Text_Width(maxValue, scale, FONT_SMALL);
	column->width[1] = MAX(labelW, fieldW);

	fieldW = CG_Text_Width(maxValue, SB_SCALE * scale, FONT_SMALL);
	column->width[0] = MAX(labelW, fieldW);
}

// also initializes columnData for CG_DrawScoreboardField
static void CG_DrawScoreboardLabel(sbColumn_t field, float x, float y)
{
	const char	*label = NULL;

	switch (field) {
	case SBC_SCORE:
		label = CG_GetStripEdString("MENUS3", "SCORE");
		CG_InitScoreboardColumn(field, label, "-999", SB_SCALE_LARGE);
		break;
	case SBC_CAP:
		label = "C";
		CG_InitScoreboardColumn(field, label, "99", SB_SCALE);
		break;
	case SBC_AST:
		label = "A";
		CG_InitScoreboardColumn(field, label, "99", SB_SCALE);
		break;
	case SBC_DEF:
		label = "D";
		CG_InitScoreboardColumn(field, label, "99", SB_SCALE);
		break;
	case SBC_TIME:
		label = CG_GetStripEdString("MENUS3", "TIME");
		CG_InitScoreboardColumn(field, label, "9999", SB_SCALE_LARGE);
		break;
	case SBC_PING:
		label = CG_GetStripEdString("MENUS0", "PING");
		CG_InitScoreboardColumn(field, label, "999", SB_SCALE_LARGE);
		break;
	case SBC_W_L:
		// Assumption: W/L is always next to score column and "Score"
		// label leaves some extra space to the right. Use it.
		label = CG_GetStripEdString("INGAMETEXT", "W_L");
		CG_InitScoreboardColumn(field, label, "9/99", SB_SCALE_LARGE);
		break;
	case SBC_W_L_SM:
		label = CG_GetStripEdString("INGAMETEXT", "W_L");
		CG_InitScoreboardColumn(field, label, "9/99", SB_SCALE);
		break;
	case SBC_K_D:
		// Assumption: K/D is always next to score column and "Score"
		// label leaves some extra space to the right. Use it.
		label = "K/D";
		CG_InitScoreboardColumn(field, label, "99/999", SB_SCALE);
		// Hack: doesn't scale and translate
		x += CG_Text_Width("99", SB_SCALE, FONT_SMALL) - CG_Text_Width("K", 1.0f, FONT_MEDIUM);
		break;
	case SBC_NET_DMG:
		label = "Net";
		CG_InitScoreboardColumn(field, label, "-99.9k", SB_SCALE);
		break;
	case SBC_MAX:
		return;
	}

	CG_Text_Paint ( x, y, 1.0f, colorWhite, label, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
}

static void CG_DrawScoreboardField(sbColumn_t field, float x, float y, float scale, score_t *score)
{
	clientInfo_t	*ci = &cgs.clientinfo[score->client];
	float			s = scale * columnData[field].scale;
	qboolean		spectator = (qboolean)(ci->team == TEAM_SPECTATOR);

	y += scale * columnData[field].drop;

	switch (field) {
	case SBC_SCORE:
		if (!spectator) {
			CG_Text_Paint (x, y, s, colorWhite, va("%i", score->score),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_CAP:
		if (!spectator && score->captures != 0) {
			CG_Text_Paint (x, y, s, colorWhite, va("%i", score->captures),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_AST:
		if (!spectator && score->assistCount != 0) {
			CG_Text_Paint (x, y, s, colorWhite, va("%i", score->assistCount),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_DEF:
		if (!spectator && score->defendCount != 0) {
			CG_Text_Paint (x, y, s, colorWhite, va("%i", score->defendCount),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_TIME:
		CG_Text_Paint (x, y, s, colorWhite, va("%i", score->time),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		break;
	case SBC_PING:
		CG_Text_Paint (x, y, s, colorWhite, va("%i", score->ping),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		break;
	case SBC_W_L:
	case SBC_W_L_SM:
		if (!spectator || ci->wins != 0 || ci->losses != 0) {
			x +=  CG_Text_Width(va("9"), SB_SCALE_LARGE, FONT_SMALL) - CG_Text_Width(va("%i", ci->wins), s, FONT_SMALL);
			CG_Text_Paint (x, y, s, colorWhite, va("%i/%i", ci->wins, ci->losses),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_K_D:
		if (!spectator) {
			x +=  CG_Text_Width(va("99"), SB_SCALE, FONT_SMALL) - CG_Text_Width(va("%i", score->kills), s, FONT_SMALL);
			CG_Text_Paint (x, y, s, colorWhite, va("%i/%i", score->kills, score->deaths),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_NET_DMG:
		if (spectator) {
		} else if (score->netDamage != 0) {
			CG_Text_Paint (x, y, s, colorWhite, va("%+.1fk", score->netDamage / 10.0f),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		} else {
			CG_Text_Paint (x, y, s, colorWhite, " 0",0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}
		break;
	case SBC_MAX:
		break;
	}
}

/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, const sbColumn_t *columns, score_t *score, const float *color, float fade, qboolean largeFormat )
{
	//vec3_t	headAngles;
	clientInfo_t	*ci;
	float			iconx;
	float			x;
	int				i;
	float			padding, columnsWidth;
	float			scale;

	if ( largeFormat )
	{
		scale = SB_SCALE_LARGE;
	}
	else
	{
		scale = SB_SCALE;
	}

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	ci = &cgs.clientinfo[score->client];

	iconx = SB_FLAGICON_X;

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y, 32, 32, TEAM_RED, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 32, 32, TEAM_RED, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y, 32, 32, TEAM_BLUE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 32, 32, TEAM_BLUE, qfalse );
		}
	} else {
		// draw the wins / losses
		/*
		if ( cgs.gametype == GT_TOURNAMENT )
		{
			CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, va("%i/%i", ci->wins, ci->losses ), color );
		}
		*/
		//rww - in duel, we now show wins/losses in place of "frags". This is because duel now defaults to 1 kill per round.
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum )
	{
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
			|| GT_Team(cgs.gametype) ) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7f;
		CG_FillRect( SB_SCORELINE_X - 5, y + 2, SB_SCORELINE_WIDTH + 10, largeFormat?SB_NORMAL_HEIGHT:SB_INTER_HEIGHT, hcolor );
	}

	// columns

	x = SB_SCORELINE_X;
	CG_Text_Paint (x, y, 0.9f * scale, colorWhite, ci->name,0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );

	x = cgs.screenWidth - SB_SCORELINE_X;

	columnsWidth = 0;
	for (i = 0; columns[i] != SBC_MAX; i++) {
		columnsWidth += columnData[columns[i]].width[largeFormat];
	}

	// Stretch columns to at least half scoreboard width
	padding = SB_SCORELINE_WIDTH / 2 - columnsWidth;
	padding /= (i - 1);
	if (padding < SB_MIN_PADDING) {
		padding = SB_MIN_PADDING;
	} else if (padding > SB_MAX_PADDING) {
		padding = SB_MAX_PADDING;
	}
	x -= columnsWidth;
	x -= (i - 1) * padding;

	for (i = 0; columns[i] != SBC_MAX; i++) {
		sbColumn_t column = columns[i];

		CG_DrawScoreboardField(column, x, y, scale, score);
		x += columnData[column].width[largeFormat] + padding;
	}

	// add the "ready" marker for intermission exiting
	if ( cg.warmup || cg.intermissionStarted )
	{
		if ( cgs.readyClients & ( 1 << score->client ) )
		{
			UI_DrawScaledProportionalString(SB_RIGHT_MARKER_X, y + 2,
				CG_GetStripEdString("INGAMETEXT", "READY"), UI_RIGHT, colorWhite, 0.7f * scale);
		}
	}
	else if ( GT_Round(cgs.gametype) && cg.predictedPlayerState.pm_type != PM_INTERMISSION &&
		score->dead && ci->team != TEAM_SPECTATOR )
	{
		UI_DrawScaledProportionalString(SB_RIGHT_MARKER_X, y + 2,
			CG_GetStripEdString("SABERINGAME", "DEAD"), UI_RIGHT, colorRed, 0.7f * scale);
	}
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, const sbColumn_t *columns, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly )
{
	int		i;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		if ( !countOnly )
		{
			qboolean largeFormat = (qboolean)(lineHeight == SB_NORMAL_HEIGHT);
			CG_DrawClientScore( y + lineHeight * count, columns, score, color, fade, largeFormat );
		}

		count++;
	}

	return count;
}

int CG_GetTeamCount(team_t team, int maxClients)
{
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	score_t	*score;

	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ )
	{
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team )
		{
			continue;
		}

		count++;
	}

	return count;
}
/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	const sbColumn_t	*columns;
	qboolean			largeFormat;
	float	x, y;
	int		i, n1, n2;
	float	fade;
	const float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;
	int columnsWidth, padding;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	// fragged by ... line
	// or if in intermission and duel, prints the winner of the duel round
	if (cgs.gametype == GT_TOURNAMENT && cgs.duelWinner != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		s = va("%s" S_COLOR_WHITE " %s", cgs.clientinfo[cgs.duelWinner].name, CG_GetStripEdString("INGAMETEXT", "DUEL_WINS") );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = 0.5f * ( cgs.screenWidth - w );
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = 0.5f * ( cgs.screenWidth - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) );
		y = 40;
		CG_Text_Paint ( x, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if (cgs.gametype == GT_TOURNAMENT && cgs.duelist1 != -1 && cgs.duelist2 != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		s = va("%s" S_COLOR_WHITE " %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStripEdString("INGAMETEXT", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = 0.5f * ( cgs.screenWidth - w );
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = 0.5f * ( cgs.screenWidth - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) );
		y = 40;
		CG_Text_Paint ( x, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( cg.killerName[0] ) {
		s = va("%s %s", CG_GetStripEdString("INGAMETEXT", "KILLEDBY"), cg.killerName );
		/*w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = 0.5f * ( cgs.screenWidth - w );
		y = 40;
		CG_DrawBigString( x, y, s, fade );
		*/
		x = 0.5f * ( cgs.screenWidth - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) );
		y = 40;
		CG_Text_Paint ( x, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// current rank
	if ( !GT_Team(cgs.gametype) || cgs.gametype == GT_REDROVER ) {
		if (cg.snap->ps.pm_type != PM_SPECTATOR )
		{
			char sPlace[256];
			char sOf[256];
			char sWith[256];

			trap_SP_GetStringTextString("INGAMETEXT_PLACE",	sPlace,	sizeof(sPlace));
			trap_SP_GetStringTextString("INGAMETEXT_OF",	sOf,	sizeof(sOf));
			trap_SP_GetStringTextString("INGAMETEXT_WITH",	sWith,	sizeof(sWith));

			s = va("%s %s (%s %i) %s %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				sPlace,
				sOf,
				cg.numScores,
				sWith,
				cg.snap->ps.persistant[PERS_SCORE] );
			//w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = 0.5f * cgs.screenWidth;
			y = 60;
			//CG_DrawBigString( x, y, s, fade );
			UI_DrawProportionalString(x, y, s, UI_CENTER|UI_DROPSHADOW, colorTable[CT_WHITE]);
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
		}

		x = 0.5f * ( cgs.screenWidth - CG_Text_Width ( s, 1.0f, FONT_MEDIUM ) );
		y = 60;

		CG_Text_Paint ( x, y, 1.0f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// scoreboard

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL ) {
		largeFormat = qfalse;
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		largeFormat = qtrue;
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 8;
	}

	// columns

	switch (cgs.gametype) {
	case GT_FFA:
		columns = cgs.privateDuel ? ffaDuelColumns : ffaColumns;
		break;
	case GT_TOURNAMENT:
		if (cgs.fraglimit == 1) {
			columns = duelFraglimit1Columns;
		} else {
			columns = duelColumns;
		}
		break;
	case GT_CTF:
		columns = ctfColumns;
		break;
	case GT_CLANARENA:
		columns = caColumns;
		break;
	default:
		columns = ffaColumns;
	}

	if ( cgs.instagib && columns == ffaColumns )
		columns = iffaColumns;

	// header

	y = SB_HEADER;

	CG_DrawPic ( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap_R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );

	x = SB_SCORELINE_X;

	CG_Text_Paint ( x, y, 1.0f, colorWhite, CG_GetStripEdString("SABERINGAME", "NAME"),0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );

	x = cgs.screenWidth - SB_SCORELINE_X;

	columnsWidth = 0;
	for (i = 0; columns[i] != SBC_MAX; i++) {
		columnsWidth += columnData[columns[i]].width[largeFormat];
	}

	// Stretch columns to at least half scoreboard width
	padding = SB_SCORELINE_WIDTH / 2 - columnsWidth;
	padding /= (i - 1);
	if (padding < SB_MIN_PADDING) {
		padding = SB_MIN_PADDING;
	} else if (padding > SB_MAX_PADDING) {
		padding = SB_MAX_PADDING;
	}
	x -= columnsWidth;
	x -= (i - 1) * padding;

	for (i = 0; columns[i] != SBC_MAX; i++) {
		sbColumn_t column = columns[i];

		CG_DrawScoreboardLabel(column, x, y);
		x += columnData[column].width[largeFormat] + padding;
	}

	y = SB_TOP;

	localClient = qfalse;


	//I guess this should end up being able to display 19 clients at once.
	//In a team game, if there are 9 or more clients on the team not in the lead,
	//we only want to show 10 of the clients on the team in the lead, so that we
	//have room to display the clients in the lead on the losing team.

	//I guess this can be accomplished simply by printing the first teams score with a maxClients
	//value passed in related to how many players are on both teams.
	if ( GT_Team(cgs.gametype) ) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			int team1MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, columns, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SB_SCORELINE_WIDTH + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, columns, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			n2 = CG_TeamScoreboard( y, columns, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SB_SCORELINE_WIDTH + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, columns, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			maxClients -= n1 + n2;
		} else {
			int team1MaxCl = CG_GetTeamCount(TEAM_BLUE, maxClients);
			int team2MaxCl = CG_GetTeamCount(TEAM_RED, maxClients);

			if (team1MaxCl > 10 && (team1MaxCl+team2MaxCl) > maxClients)
			{
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if (team1MaxCl < 10)
				{
					team1MaxCl = 10;
				}
			}

			team2MaxCl = (maxClients-team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, columns, TEAM_BLUE, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SB_SCORELINE_WIDTH + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, columns, TEAM_BLUE, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			n2 = CG_TeamScoreboard( y, columns, TEAM_RED, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SB_SCORELINE_WIDTH + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, columns, TEAM_RED, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			maxClients -= n1 + n2;
		}
		n1 = CG_TeamScoreboard( y, columns, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, columns, TEAM_FREE, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, columns, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, columns, &cg.scores[i], fadeColor, fade, largeFormat );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	if ( cg.deferredPlayerLoading == 60 && !cg.demoPlayback ) {
		trap_SendConsoleCommand("ui_macroscan 1\n");
	}

	return qtrue;
}

//================================================================================

