#include "g_local.h"

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
