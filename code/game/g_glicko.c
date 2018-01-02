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

// g_glicko.c -- implementation of Mark Glickman's Glicko rating system

// note: Glicko-2 doesn't improve upon Glicko when ratings are
// evaluated after each game.

#include "g_local.h"

#define STEP 400	// odds of defeating a player rated this many
					// points higher are 1:10

// result must be 1 if A wins, 0 if B wins and 0.5 in case of a draw
static glicko_t G_GlickoUpdate(const glicko_t *A, const glicko_t *B, float result) {
#define Q (M_LN10 / STEP)
	float gRDB = sqrtf(M_PI * M_PI / (M_PI * M_PI + 3 * Q * Q * B->RD * B->RD));
	float E = 1 / (1 + expf(Q * gRDB * (B->R - A->R)));
	float denom = 1 / (A->RD * A->RD) + (Q * Q * gRDB * gRDB * E * (1 - E));
	float newRD = sqrtf(1 / denom);
	glicko_t glicko;

	glicko.R = A->R + Q / denom * gRDB * (result - E);
	glicko.RD = MAX(newRD, g_glickoMinRD.value);

	return glicko;
}

void RatingPlum(int clientNum, const playerState_t *ps, int rating) {
	gentity_t	*plum;
	vec3_t		origin;

	VectorCopy(ps->origin, origin);
	origin[2] += ps->viewheight;
	plum = G_TempEntity( origin, EV_SCOREPLUM, clientNum );
	// only send this temp entity to a single client
	plum->r.svFlags |= SVF_SINGLECLIENT;
	plum->r.singleClient = clientNum;
	//
	plum->s.otherEntityNum = clientNum;
	plum->s.time = rating;
	plum->s.eventParm = PLUM_RATING;
}

void G_GlickoAddResult(gentity_t *winner, gentity_t *loser) {
	glicko_t	winnerGlicko;
	glicko_t	loserGlicko;
	int			deltaR;
	static fileHandle_t glickoLog;

	if (!level.glickoLadder) {
		return;
	}

	if (!glickoLog) {
		trap_FS_FOpenFile(va("glicko-%d.log", trap_RealTime(NULL)), &glickoLog, FS_WRITE);
	}

	{
		char *s = va("%d %d\n", winner->client->prof.id, loser->client->prof.id);
		trap_FS_Write(s, strlen(s), glickoLog);
	}

	winnerGlicko = G_GlickoUpdate(&winner->client->prof.glicko, &loser->client->prof.glicko, 1);
	loserGlicko = G_GlickoUpdate(&loser->client->prof.glicko, &winner->client->prof.glicko, 0);

	deltaR = (int)winnerGlicko.R - (int)winner->client->prof.glicko.R;
	RatingPlum(winner->s.number, &winner->client->ps, deltaR);
	deltaR = (int)loserGlicko.R - (int)loser->client->prof.glicko.R;
	RatingPlum(loser->s.number, &loser->client->ps, deltaR);

	winner->client->prof.glicko = winnerGlicko;
	loser->client->prof.glicko = loserGlicko;
	winner->client->pers.persistant[PERS_SCORE] = winnerGlicko.R;
	loser->client->pers.persistant[PERS_SCORE] = loserGlicko.R;

	CalculateRanks();
}

void G_GlickoAddDraw(gentity_t *player1, gentity_t *player2) {
	glicko_t	glicko1;
	glicko_t	glicko2;

	if (!level.glickoLadder) {
		return;
	}

	glicko1 = G_GlickoUpdate(&player1->client->prof.glicko, &player2->client->prof.glicko, 0.5f);
	glicko2 = G_GlickoUpdate(&player1->client->prof.glicko, &player2->client->prof.glicko, 0.5f);

	player1->client->prof.glicko = glicko1;
	player2->client->prof.glicko = glicko2;
	player1->client->pers.persistant[PERS_SCORE] = glicko1.R;
	player2->client->pers.persistant[PERS_SCORE] = glicko2.R;

	CalculateRanks();
}

void G_GlickoUpdateScore(gentity_t *player) {
	if (!level.glickoLadder) {
		return;
	}

	player->client->pers.persistant[PERS_SCORE] = player->client->prof.glicko.R;
}
