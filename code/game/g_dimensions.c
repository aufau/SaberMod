#include "g_local.h"

void G_EntitySnapshotControl( gentity_t *ent, int blame )
{
	if (level.mvapi) {
		mvsharedEntity_t	*mvEnt;
		int					i;

		assert(blame >= 0 && blame < MAX_GENTITIES);

		mvEnt = mv_entities + ent->s.number;

		for (i = 0; i < MAX_CLIENTS; i++) {
			mvEnt->snapshotIgnore[i] = 0;
		}

		// never ignore ENTITYNUM_WORLD, at least for EV_CLIENTJOIN
		if (blame >= MAX_CLIENTS) {
			blame = g_entities[blame].r.ownerNum;

			if (blame >= MAX_CLIENTS) {
				blame = g_entities[blame].r.ownerNum;

				if (blame >= MAX_CLIENTS) {
					assert(0); // just for testing
					return;
				}
			}
		}

		for (i = 0; i < MAX_CLIENTS; i++) {
			playerState_t *ps;

			if (!g_entities[i].inuse) {
				continue;
			}

			ps = &level.clients[i].ps;

			if (ps->duelInProgress && blame != i && blame != ps->duelIndex) {
				mvEnt->snapshotIgnore[i] = 1;
			}
		}
	}
}

qboolean G_EntitiesCollide(gentity_t *ent1, gentity_t *ent2)
{
	assert(ent1->s.number != ent2->s.number);

	if (ent1->client->ps.duelInProgress) {
		if (ent2->s.number == ent1->client->ps.duelIndex) {
			return qtrue;
		} else {
			return qfalse;
		}
	} else if (ent2->client->ps.duelInProgress) {
		return qfalse;
	} else {
		return qtrue;
	}
}

void G_StartPrivateDuel(gentity_t *ent, gentity_t *challenged)
{
	if (level.mvapi) {
		int	clientNum = ent->s.clientNum;
		int challengedNum = challenged->s.clientNum;
		int	i;

		for (i = 0; i < MAX_CLIENTS; i++) {
			mv_entities[i].snapshotIgnore[clientNum] = 1;
			mv_entities[i].snapshotIgnore[challengedNum] = 1;
		}

		mv_entities[clientNum].snapshotIgnore[clientNum] = 0;
		mv_entities[clientNum].snapshotIgnore[challengedNum] = 0;
		mv_entities[challengedNum].snapshotIgnore[challengedNum] = 0;
		mv_entities[challengedNum].snapshotIgnore[clientNum] = 0;
	}
}

void G_StopPrivateDuel(gentity_t *ent)
{
	if (level.mvapi) {
		int clientNum = ent->s.clientNum;
		int i;

		for (i = 0; i < level.maxclients; i++) {
			mv_entities[i].snapshotIgnore[clientNum] = 0;
		}
	}
}
