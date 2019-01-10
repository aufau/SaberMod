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

qboolean (*G_Collide) (const gentity_t *ent1, const gentity_t *ent2);
void (*G_Trace) (trace_t *results, const vec3_t start, const vec3_t mins,
	const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
int (*G_EntitiesInBox) (const vec3_t mins, const vec3_t maxs,
	int *entityList, int maxcount, int entityNum);

/*
=================
G_BlameForEntity

Set blameEntityNum and snapshotIgnore fields for all clients.
=================
*/
void G_BlameForEntity( int blame, gentity_t *ent )
{
	assert(blame >= 0 && blame < MAX_GENTITIES);

	if (blame >= MAX_CLIENTS && blame < ENTITYNUM_MAX_NORMAL) {
		blame = g_entities[blame].blameEntityNum;
		assert(blame < MAX_CLIENTS || blame == ENTITYNUM_NONE || blame == ENTITYNUM_WORLD);
	}

	ent->blameEntityNum = blame;
	if (ent - g_entities < MAX_CLIENTS) {
		// for team pass-through
		ent->dimension = 1u << ent->client->sess.sessionTeam;
	} else {
		// ent->dimension = g_entities[blame].dimension;
		// For now cgame can ignore only dueling players
		ent->dimension = GT_Team(level.gametype) ? 0 : ALL_DIMENSIONS;
	}

	if (g_mvapi) {
		uint8_t	*snapshotIgnore = mv_entities[ent->s.number].snapshotIgnore;
		int		i;

		if (blame == ENTITYNUM_WORLD) {
			memset(snapshotIgnore, 0, sizeof(snapshotIgnore[0]) * MAX_CLIENTS);
		} else {
			for (i = 0; i < level.maxclients; i++) {
				gclient_t	*client = level.clients + i;

				snapshotIgnore[i] = client->pers.connected == CON_CONNECTED &&
					client->info.privateDuel && client->ps.duelInProgress &&
					blame != i && blame != client->ps.duelIndex;
			}
		}
	}
}

unsigned G_GetFreeDuelDimension(void)
{
	unsigned dimension;
	qboolean free;
	int i;

	for (dimension = 1u << 15; dimension != 0; dimension <<= 1) {
		free = qtrue;

		for (i = 0; i < level.maxclients; i++) {
			if (!g_entities[i].inuse) {
				continue;
			}
			if ((g_entities[i].dimension & dimension) != 0) {
				free = qfalse;
				break;
			}
		}

		if (free) {
			return dimension;
		}
	}

	assert(0); // didn't find a free dimension
	return DEFAULT_DIMENSION;
}

void G_StartPrivateDuel(gentity_t *ent)
{
	if (g_mvapi) {
		int	clientNum = ent->s.number;
		int opponentNum;
		int	i;

		if (!ent->client->info.privateDuel) {
			return;
		}
		if (!ent->client->ps.duelInProgress) {
			return;
		}

		opponentNum = ent->client->ps.duelIndex;

		for (i = 0; i < level.num_entities; i++) {
			if (g_entities[i].inuse) {
				int blame = g_entities[i].blameEntityNum;

				if (blame == ENTITYNUM_WORLD || blame == clientNum || blame == opponentNum) {
					mv_entities[i].snapshotIgnore[clientNum] = 0;
				} else {
					mv_entities[i].snapshotIgnore[clientNum] = 1;
				}
			}
		}
	}
}

void G_StopPrivateDuel(gentity_t *ent)
{
	if (g_mvapi) {
		int clientNum = ent->s.number;
		int i;

		for (i = 0; i < level.num_entities; i++) {
			mv_entities[i].snapshotIgnore[clientNum] = 0;
		}
	}
}

// Dimension - only players in the same dimension collide

static qboolean G_DimensionCollide(const gentity_t *ent1, const gentity_t *ent2)
{
	return (qboolean)!!(ent1->dimension & ent2->dimension);
}

static void G_DimensionTrace (trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask)
{
	trap_Trace(results, start, mins, maxs, end, passEntityNum, contentMask);

	if (results->entityNum < ENTITYNUM_MAX_NORMAL) {
		gentity_t	*passEnt = g_entities + passEntityNum;
		gentity_t	*ent = g_entities + results->entityNum;

		if (!G_Collide(ent, passEnt)) {
			int contents;

			contents = ent->r.contents;
			ent->r.contents = 0;

			G_DimensionTrace(results, start, mins, maxs, end, passEntityNum, contentMask);

			ent->r.contents = contents;
			return;
		}
	}

	// startsolid is qtrue if a trace starts inside of a solid area
	// and tracing continues. Remaining trace fields correspond to
	// trace.entityNum and are correct after filtering above. Here
	// we must only take care of a situation where startolid is qtrue
	// when trace starts from inside of an entity in another
	// dimension. To do so, retrace with start == end and filter
	// resulting entities. --fau

	if (results->startsolid && start != end) {
		trace_t tw;

		G_DimensionTrace(&tw, start, mins, maxs, start, passEntityNum, contentMask);
		results->startsolid = tw.startsolid;
	}
}

static int G_DimensionEntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount, int entityNum)
{
	gentity_t	*passEnt = g_entities + entityNum;
	int	fullCount;
	int	count;
	int	i;

	fullCount = trap_EntitiesInBox(mins, maxs, entityList, maxcount);

	for (i = 0, count = 0; i < fullCount; i++) {
		if (G_Collide(g_entities + entityList[i], passEnt)) {
			entityList[count] = entityList[i];
			count++;
		}
	}

	return count;
}

// Anti-Dimension - players in the same dimension don't collide

static qboolean G_AntiDimensionCollide(const gentity_t *ent1, const gentity_t *ent2)
{
	return (qboolean)!(ent1->dimension & ent2->dimension);
}

// Normal - standard collisions

static qboolean G_NormalCollide(const gentity_t *ent1, const gentity_t *ent2)
{
	return qtrue;
}

static int G_NormalEntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount, int entityNum)
{
	return trap_EntitiesInBox(mins, maxs, entityList, maxcount);
}

/*
================
G_UpdateCollisionMap

Update collision map functions to current server settings
================
*/
void G_UpdateCollisionMap(void)
{
	G_Collide = G_NormalCollide;
	G_Trace = trap_Trace;
	G_EntitiesInBox = G_NormalEntitiesInBox;

	switch (level.gametype) {
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_SINGLE_PLAYER:
		if (g_privateDuel.integer) {
			G_Collide = G_DimensionCollide;
			G_Trace = G_DimensionTrace;
			G_EntitiesInBox = G_DimensionEntitiesInBox;
		}
		break;
	case GT_TOURNAMENT:
		break;
	default:	// Team
		if (g_dmflags.integer & DF_TEAM_PASS) {
			G_Collide = G_AntiDimensionCollide;
			G_Trace = G_DimensionTrace;
			G_EntitiesInBox = G_DimensionEntitiesInBox;
		}
		break;
	}
}
