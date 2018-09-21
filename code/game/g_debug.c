#if !defined(NDEBUG) && !defined(Q3_VM)

#include "g_local.h"

void G_StaticCheck(void)
{
	assert(EV_MAX <= 256);
}

void G_EntityStateCheckRep(const entityState_t *s)
{
}

void G_ClientCheckRep(const gclient_t *cl)
{
	assert(0 <= cl->saberCycleQueue && cl->saberCycleQueue < NUM_FORCE_POWER_LEVELS);
	assert(-1 <= cl->lastkilled_client && cl->lastkilled_client < MAX_CLIENTS);
	assert(cl->ghoul2);
}

void G_EntityCheckRep(const gentity_t *ent)
{
	int i;
	int num = ent - g_entities;
	qboolean isSentryGun = qfalse;
	qboolean isBody = qfalse;
	qboolean isTempEntity = qfalse;
	qboolean isSpawnEntity = qfalse;

	assert(0 <= num && num < MAX_GENTITIES);

	if (ent->inuse == qfalse) {
		if (num < MAX_CLIENTS) {
			assert(ent->classname == NULL || !strcmp(ent->classname, "disconnected"));
		} else {
			gentity_t zeroEnt = { { 0 } };

			zeroEnt.classname = ent->classname;
			zeroEnt.freetime = ent->freetime;
			assert(ent->classname == NULL || !strcmp(ent->classname, "freed"));
			assert(0 <= zeroEnt.freetime && zeroEnt.freetime <= level.time);
			assert(!memcmp(ent, &zeroEnt, sizeof(zeroEnt)));
		}

		return;
	}

	assert (ent->s.number == num);
	G_EntityStateCheckRep(&ent->s);
	// G_EntitySharedCheckRep(&ent->r);

	if (ent->client) {
		assert(level.clients + num == ent->client);
		if (ent->client->pers.connected == CON_CONNECTED) {
			// assert(ent->health == ent->client->ps.stats[STAT_HEALTH]);
			assert(ent->blameEntityNum == num);
			G_ClientCheckRep(ent->client);
		}
	}

	// TODO: Check entity depending on it's class
	assert(ent->classname);
	if (!strcmp(ent->classname, "sentryGun"))
	{
		isSentryGun = qtrue;
	}
	else if (!strcmp(ent->classname, "bodyque"))
	{
		isBody = qtrue;
		for (i = 0; i < BODY_QUEUE_SIZE && level.bodyQue[i] != ent; i++);
		assert(i < BODY_QUEUE_SIZE); // in body queue
	}
	else if (!strcmp(ent->classname, "tempEntity") && !(ent->s.eFlags & EF_SOUNDTRACKER))
	{
		int event = ent->s.eType & ~EV_EVENT_BITS;
		isTempEntity = qtrue;
		assert(ent->freeAfterEvent);
		assert(ET_EVENTS <= event && event < ET_EVENTS + EV_MAX);
	}
	else if (G_IsSpawnEntity(ent))
	{
		isSpawnEntity = qtrue;
	}

	// ent->spawnflags
	assert(0 <= ent->teamnodmg && ent->teamnodmg < TEAM_NUM_TEAMS);
	if (ent->roffname)
		assert(ent->roffid != -1);
	// ent->rofftarget

	// ent->model
	// ent->model2

	// Saga fields:

	// ent->objective
	// ent->side

	// Saga fields: End

	if (ent->aimDebounceTime != 0)
		assert(isSentryGun);

	if (ent->painDebounceTime != 0)
		assert(isSentryGun);

	if (ent->attackDebounceTime != 0)
		assert(isSentryGun);

	if (ent->noDamageTeam != 0) {
		assert(isSentryGun);
		assert (0 <= ent->noDamageTeam && ent->noDamageTeam < TEAM_NUM_TEAMS);
	}

	if (ent->neverFree)
		assert(isBody);

	if (ent->flags != 0)
		assert(0x10 <= ent->flags && ent->flags < 0x10000);

	assert(ent->freetime == 0);

	if (ent->eventTime || ent->freeAfterEvent || ent->unlinkAfterEvent) {
		if (isTempEntity) {
			int event = ent->s.eType & ~EV_EVENT_BITS;
			assert(ET_EVENTS <= event && event < ET_EVENTS + EV_MAX);
		} else if (!ent->client && isSpawnEntity && g_checkSpawnEntities.integer) {
			int event = ent->s.event & ~EV_EVENT_BITS;
			assert(ET_EVENTS <= event && event < ET_EVENTS + EV_MAX);
		}
	}

	// ent->physicsObject
	// ent->physicsBounce
	// ent->clipmask

	// ...

	// if (ent->isSaberEntity)
	//	G_SaberEntityCheckRep(ent);

	// if (ent->item)
	//	G_ItemEntityCheckRep(ent);
}

#else // !defined(NDEBUG) && !defined(Q3_VM)
static int dummy;	// silence empty file warning
#endif
