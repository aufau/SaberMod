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

void G_GlickoAddResult(gentity_t *winner, gentity_t *loser) {
	glicko_t	winnerGlicko;
	glicko_t	loserGlicko;

	if (!level.glickoLadder) {
		return;
	}

	winnerGlicko = G_GlickoUpdate(&winner->client->prof.glicko, &loser->client->prof.glicko, 1);
	loserGlicko = G_GlickoUpdate(&loser->client->prof.glicko, &winner->client->prof.glicko, 0);

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
