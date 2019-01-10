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

#include "g_local.h"
#include "bg_version.h"
#include "../ghoul2/g2.h"

// g_client.c -- client functions that don't happen every frame

static const vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
static const vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*QUAKED info_player_imperial (1 0 0) (-16 -16 -24) (16 16 32)
saga start point - imperial
*/
void SP_info_player_imperial(gentity_t *ent) {
	if (level.gametype != GT_SAGA)
	{ //turn into a DM spawn if not in saga game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
	}
}

/*QUAKED info_player_rebel (1 0 0) (-16 -16 -24) (16 16 32)
saga start point - rebel
*/
void SP_info_player_rebel(gentity_t *ent) {
	if (level.gametype != GT_SAGA)
	{ //turn into a DM spawn if not in saga game mode
		ent->classname = "info_player_deathmatch";
		SP_info_player_deathmatch( ent );
	}
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

#define JMSABER_RESPAWN_TIME 20000 //in case it gets stuck somewhere no one can reach

void ThrowSaberToAttacker(gentity_t *self, gentity_t *attacker)
{
	gentity_t *ent = &g_entities[self->client->ps.saberIndex];
	vec3_t a;
	int altVelocity = 0;

	if (!ent || ent->enemy != self)
	{ //something has gone very wrong (this should never happen)
		//but in case it does.. find the saber manually
#ifdef _DEBUG
		Com_Printf("Lost the saber! Attempting to use global pointer..\n");
#endif
		ent = gJMSaberEnt;

		if (!ent)
		{
#ifdef _DEBUG
			Com_Printf("The global pointer was NULL. This is a bad thing.\n");
#endif
			return;
		}

#ifdef _DEBUG
		Com_Printf("Got it (%i). Setting enemy to client %i.\n", ent->s.number, self->s.number);
#endif

		ent->enemy = self;
		self->client->ps.saberIndex = ent->s.number;
	}

	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, "-1" );

	if (attacker && attacker->client && self->client->ps.saberInFlight)
	{ //someone killed us and we had the saber thrown, so actually move this saber to the saber location
	  //if we killed ourselves with saber thrown, however, same suicide rules of respawning at spawn spot still
	  //apply.
		gentity_t *flyingsaber = &g_entities[self->client->ps.saberEntityNum];

		if (flyingsaber && flyingsaber->inuse)
		{
			VectorCopy(flyingsaber->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(flyingsaber->s.pos.trDelta, ent->s.pos.trDelta);
			VectorCopy(flyingsaber->s.apos.trBase, ent->s.apos.trBase);
			VectorCopy(flyingsaber->s.apos.trDelta, ent->s.apos.trDelta);

			VectorCopy(flyingsaber->r.currentOrigin, ent->r.currentOrigin);
			VectorCopy(flyingsaber->r.currentAngles, ent->r.currentAngles);
			altVelocity = 1;
		}
	}

	self->client->ps.saberInFlight = qtrue; //say he threw it anyway in order to properly remove from dead body

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.eFlags &= ~(EF_NODRAW);
	ent->s.modelGhoul2 = 1;
	ent->s.eType = ET_MISSILE;
	ent->enemy = NULL;

	if (!attacker || !attacker->client)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->time1 = 0;
		trap_LinkEntity(ent);
		return;
	}

	if (!altVelocity)
	{
		VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
		VectorCopy(self->s.pos.trBase, ent->s.origin);
		VectorCopy(self->s.pos.trBase, ent->r.currentOrigin);

		VectorSubtract(attacker->client->ps.origin, ent->s.pos.trBase, a);

		VectorNormalize(a);

		ent->s.pos.trDelta[0] = a[0]*256;
		ent->s.pos.trDelta[1] = a[1]*256;
		ent->s.pos.trDelta[2] = 256;
	}

	trap_LinkEntity(ent);
}

void JMSaberThink(gentity_t *ent)
{
	gJMSaberEnt = ent;

	if (ent->enemy)
	{
		if (!ent->enemy->client || !ent->enemy->inuse)
		{ //disconnected?
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.pos.trBase);
			VectorCopy(ent->enemy->s.pos.trBase, ent->s.origin);
			VectorCopy(ent->enemy->s.pos.trBase, ent->r.currentOrigin);
			ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
			ent->s.eFlags &= ~(EF_NODRAW);
			ent->s.modelGhoul2 = 1;
			ent->s.eType = ET_MISSILE;
			ent->enemy = NULL;

			ent->time1 = level.time; //respawn next think
			trap_LinkEntity(ent);
		}
		else
		{
			ent->time1 = level.time + JMSABER_RESPAWN_TIME;
		}
	}
	else if (ent->time1 && ent->time1 < level.time)
	{
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);
		ent->time1 = 0;
		trap_LinkEntity(ent);
	}

	ent->nextthink = level.time + 50;
	G_RunObject(ent);
}

void JMSaberTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int i = 0;
//	gentity_t *te;

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (other->client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER))
	{
		return;
	}

	if (other->client->ps.isJediMaster)
	{
		return;
	}

	self->enemy = other;
	other->client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
	other->client->ps.weapon = WP_SABER;
	other->s.weapon = WP_SABER;
	G_AddEvent(other, EV_BECOME_JEDIMASTER, 0);

	// Track the jedi master
	trap_SetConfigstring ( CS_CLIENT_JEDIMASTER, va("%i", other->s.number ) );

	if (g_spawnInvulnerability.integer)
	{
		other->client->ps.eFlags |= EF_INVULNERABLE;
		other->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"", other->client->info.netname, G_GetStripEdString("SVINGAME", "BECOMEJM")) );

	other->client->ps.isJediMaster = qtrue;
	other->client->ps.saberIndex = self->s.number;

	if (other->health < 200 && other->health > 0)
	{ //full health when you become the Jedi Master
		other->client->ps.stats[STAT_HEALTH] = other->health = 200;
	}

	if (other->client->ps.fd.forcePower < 100)
	{
		other->client->ps.fd.forcePower = 100;
	}

	while (i < NUM_FORCE_POWERS)
	{
		other->client->ps.fd.forcePowersKnown |= (1 << i);
		other->client->ps.fd.forcePowerLevel[i] = FORCE_LEVEL_3;

		i++;
	}

	self->time1 = level.time + JMSABER_RESPAWN_TIME;

	self->s.modelindex = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelGhoul2 = 0;
	self->s.eType = ET_GENERAL;

	/*
	te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = self->s.number;
	*/
	G_KillG2Queue(self->s.number);
}

gentity_t *gJMSaberEnt = NULL;

/*QUAKED info_jedimaster_start (1 0 0) (-16 -16 -24) (16 16 32)
"jedi master" saber spawn point
*/
void SP_info_jedimaster_start(gentity_t *ent)
{
	if (level.gametype != GT_JEDIMASTER)
	{
		gJMSaberEnt = NULL;
		G_FreeEntity(ent);
		return;
	}

	ent->enemy = NULL;

	ent->s.eFlags = EF_BOUNCE_HALF;

	ent->s.modelindex = G_ModelIndex("models/weapons2/saber/saber_w.glm");
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 20;
	//ent->s.eType = ET_GENERAL;
	ent->s.eType = ET_MISSILE;
	ent->s.weapon = WP_SABER;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	VectorSet( ent->r.maxs, 3, 3, 3 );
	VectorSet( ent->r.mins, -3, -3, -3 );
	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->isSaberEntity = qtrue;

	ent->bounceCount = -5;

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = JMSaberTouch;

	trap_LinkEntity(ent);

	ent->think = JMSaberThink;
	ent->nextthink = level.time + 50;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *ent, gentity_t *spot ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	if (ent) {
		num = G_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES, ent->s.number );
	} else {
		num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );
	}

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}

#ifdef UNUSED
/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( gentity_t *ent ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( ent, spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = id_rand() % count;
	return spots[ selection ];
}
#endif // UNUSED

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( gentity_t *ent, const vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( ent, spot ) ) {
			continue;
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}
	if (!numSpots) {
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( gentity_t *ent, const vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	return SelectRandomFurthestSpawnPoint( ent, avoidPoint, origin, angles );

	/*
	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
	*/
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( spot->spawnflags & 1 ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( ent, spot ) ) {
		return SelectSpawnPoint( ent, vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=======================================================================

BODYQUE

=======================================================================
*/

#define BODY_SINK_TIME		45000

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn( ENTITYNUM_NONE );
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > BODY_SINK_TIME + 1500 ) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t		*body;
	int			contents;

	if (level.intermissiontime) {
		return;
	}

	if (ent->r.contents != CONTENTS_CORPSE) {
		return;
	}

	if (ent->client->sess.spectatorState != SPECTATOR_NOT) {
		return;
	}

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		return;
	}

	if (ent->client->ps.eFlags & EF_DISINTEGRATION)
	{ //for now, just don't spawn a body if you got disint'd
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);
	body->s = ent->s;

	//avoid oddly angled corpses floating around
	body->s.angles[PITCH] = body->s.angles[ROLL] = body->s.apos.trBase[PITCH] = body->s.apos.trBase[ROLL] = 0;

	body->s.g2radius = 100;

	body->s.eType = ET_BODY;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc

	if (ent->client->ps.eFlags & EF_DISINTEGRATION)
	{
		body->s.eFlags |= EF_DISINTEGRATION;
	}

	VectorCopy(ent->client->ps.lastHitLoc, body->s.origin2);

	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	body->s.weapon = ent->s.bolt2;

	if (body->s.weapon == WP_SABER && ent->client->ps.saberInFlight)
	{
		body->s.weapon = WP_BLASTER; //lie to keep from putting a saber on the corpse, because it was thrown at death
	}

	G_AddEvent(body, EV_BODY_QUEUE_COPY, ent->s.number);

	body->r.svFlags = ent->r.svFlags | SVF_BROADCAST;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->s.torsoAnim = body->s.legsAnim = ANIM(ent->client->ps.legsAnim);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + BODY_SINK_TIME;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );

	G_BlameForEntity( ent->s.number, body );
	trap_LinkEntity (body);
}

//======================================================================

/*
==================
ResetClientState

Remove items, force powers, powerups and sounds
==================
*/
void ResetClientState( gentity_t *self )
{
	//if he was charging or anything else, kill the sound
	G_MuteSound(self->s.number, CHAN_WEAPON);

	BlowDetpacks(self); //blow detpacks if they're planted

	self->client->ps.fd.forceDeactivateAll = qtrue;

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->client->ps.zoomMode = ZOOM_NONE;
	self->s.loopSound = 0;

	if ( self->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
		Team_ReturnFlag( TEAM_FREE );
	}
	else if ( self->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
		Team_ReturnFlag( TEAM_RED );
	}
	else if ( self->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
		Team_ReturnFlag( TEAM_BLUE );
	}

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );
}

/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, const vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	gentity_t	*tent;

	CopyToBodyQue (ent);

	if (gEscaping)
	{
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->sess.spectatorClient = 0;

		ent->client->pers.teamState.state = TEAM_BEGIN;
	}

	trap_UnlinkEntity (ent);
	ClientSpawn(ent);

	// add a teleportation effect
	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN, ent->s.number );
	tent->s.clientNum = ent->s.number;
}

/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount( int ignoreClientNum, team_t team, qboolean dead ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].pers.connected != CON_CONNECTED ||
			level.clients[i].sess.sessionTeam != team ||
			i == ignoreClientNum ) {
			continue;
		}

		if ( !dead ) {
			if (level.clients[i].sess.spectatorState != SPECTATOR_NOT ||
				level.clients[i].ps.stats[STAT_HEALTH] <= 0 ||
				level.clients[i].ps.fallingToDeath) {
				continue;
			}
		}

		count++;
	}

	return count;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( team_t team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}

/*
================
ValidateTeam

Checks if client can join/stay in a given team. PickTeam does
validation on it's own.
================
*/
qboolean ValidateTeam( int ignoreClientNum, team_t team )
{
	int count, otherCount;

	// Works in FFA and counts CON_CONNECTING clients unlike
	// level.numNonSpectatorClients
	count = TeamCount( ignoreClientNum, team, qtrue );

	if ( level.gametype == GT_TOURNAMENT && count >= 2 ) {
		return qfalse;
	}
	if ( g_teamsize.integer > 0 && count >= g_teamsize.integer ) {
		return qfalse;
	}
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		if ( g_teamForceBalance.integer && !g_trueJedi.integer ) {
			otherCount = TeamCount( ignoreClientNum,
				team == TEAM_RED ? TEAM_BLUE : TEAM_RED, qtrue );

			if ( count - otherCount >= g_teamForceBalance.integer ) {
				return qfalse;
			}
		}
	}
	return qtrue;
}

/*
================
PickTeam

Picks a weaker team. If it's not valid (see ValidateTeam), returns
TEAM_SPECTATOR.
================
*/
team_t PickTeam( int ignoreClientNum ) {
	int			counts[TEAM_NUM_TEAMS];
	qboolean	locked[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE, qtrue );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED, qtrue );
	// team can be locked by team lock and teamsize
	locked[TEAM_RED] = level.teamLock[TEAM_RED];
	locked[TEAM_BLUE] = level.teamLock[TEAM_BLUE];

	if ( g_teamsize.integer > 0 ) {
		if ( counts[TEAM_BLUE] >= g_teamsize.integer )
			locked[TEAM_BLUE] = qtrue;
		if ( counts[TEAM_RED] >= g_teamsize.integer )
			locked[TEAM_RED] = qtrue;
	}
	// pick team
	if ( !locked[TEAM_BLUE] && !locked[TEAM_RED] ) {
		if ( counts[TEAM_RED] > counts[TEAM_BLUE] )
			return TEAM_BLUE;
		if ( counts[TEAM_BLUE] > counts[TEAM_RED] )
			return TEAM_RED;
		if ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] )
			return TEAM_RED;
		return TEAM_BLUE;
	}
	// at least one is locked
	if ( !locked[TEAM_BLUE] )
		return TEAM_BLUE;
	if ( !locked[TEAM_RED] )
		return TEAM_RED;

	return TEAM_SPECTATOR;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = Q_strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientSetName
============
*/
static void ClientSetName( gclient_t *client, const char *in ) {
	char	cleanName[MAX_NETNAME - 5]; // "^7(1)" suffix
	char	*name;
	int		clientNum;
	int		i, num;
    int		characters;
    int		spaces;
	int		ats;
    char	ch;
    char	*p;
    char	*end;
	qboolean	free;

    p = cleanName;
    end = cleanName + sizeof(cleanName) - 1;
    characters = 0;
    spaces = 0;
	ats = 0;

    for ( ; *in == ' '; in++ );

    do {
        ch = *in++;

        if ( ch == ' ' ) {
            if ( spaces <= 3 ) {
                spaces++;
				ats = 0;
                *p++ = ch;
            }
        } else if ( ch == '@' ) {
			if ( ats < 2 ) {
				spaces = 0;
				ats++;
				characters++;
				*p++ = ch;
			}
		} else if ( ch == Q_COLOR_ESCAPE && Q_IsColorChar(*in) ) {
            if ( p + 1 < end ) {
                *p++ = ch;
                *p++ = *in;
            }
			in++;
        } else if ( isgraph(ch) ) {
            spaces = 0;
			ats = 0;
            characters++;
            *p++ = ch;
        }
    } while ( ch != '\0' && p < end && characters + spaces < MAX_NAME_LEN - 3); // "(1)" suffix

    *p = '\0';

    // don't allow empty names
    if( characters == 0 ) {
        strncpy( cleanName, "Padawan", sizeof(cleanName) );
    }

	name = client->info.netname;
	Q_strncpyz(name, cleanName, MAX_NETNAME);
	clientNum = client - level.clients;
	num = 1;

	while ((free = qtrue)) {
		for (i = 0; i < level.maxclients; i++) {
			if (level.clients[i].pers.connected == CON_DISCONNECTED) {
				continue;
			}
			if (i == clientNum) {
				continue;
			}
			if (!strncmp(name, level.clients[i].info.netname, MAX_NETNAME)) {
				free = qfalse;
				break;
			}
		}
		if (free) {
			break;
		}
		Com_sprintf(name, MAX_NETNAME, "%s" S_COLOR_WHITE "(%c)", cleanName, '0' + num);
		num++;
	}
}

#ifdef _DEBUG
void G_DebugWrite(const char *path, const char *text)
{
	fileHandle_t f;

	trap_FS_FOpenFile( path, &f, FS_APPEND );
	trap_FS_Write(text, strlen(text), f);
	trap_FS_FCloseFile(f);
}
#endif

/*
===========
SetupGameGhoul2Model

There are two ghoul2 model instances per player (actually three).  One is on the clientinfo (the base for the client side
player, and copied for player spawns and for corpses).  One is attached to the centity itself, which is the model acutally
animated and rendered by the system.  The final is the game ghoul2 model.  This is animated by pmove on the server, and
is used for determining where the lightsaber should be, and for per-poly collision tests.
===========
*/
void *g2SaberInstance = NULL;
void SetupGameGhoul2Model(gclient_t *client, const char *modelname)
{
	int handle;
	char		afilename[MAX_QPATH];
	char		/**GLAName,*/ *slash;
	char		GLAName[MAX_QPATH];

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (client->ghoul2 && trap_G2_HaveWeGhoul2Models(client->ghoul2))
	{
		trap_G2API_CleanGhoul2Models(&(client->ghoul2));
	}

	/*
	Com_sprintf( afilename, sizeof( afilename ), "models/players/%s/model.glm", modelname );
	handle = trap_G2API_InitGhoul2Model(&client->ghoul2, afilename, 0, 0, -20, 0, 0);
	if (handle<0)
	{
		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&client->ghoul2, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}
	}
	*/

	//rww - just load the "standard" model for the server"
	if (!precachedKyle)
	{
		Com_sprintf( afilename, sizeof( afilename ), "models/players/kyle/model.glm" );
		handle = trap_G2API_InitGhoul2Model(&precachedKyle, afilename, 0, 0, -20, 0, 0);

		if (handle<0)
		{
			return;
		}
	}

	if (precachedKyle && trap_G2_HaveWeGhoul2Models(precachedKyle))
	{
		trap_G2API_DuplicateGhoul2Instance(precachedKyle, &client->ghoul2);
	}
	else
	{
		return;
	}

	// The model is now loaded.

	GLAName[0] = 0;

	if (!BGPAFtextLoaded)
	{
		//get the location of the animation.cfg
		//GLAName = trap_G2API_GetGLAName( client->ghoul2, 0);
		trap_G2API_GetGLAName( client->ghoul2, 0, GLAName);

		if (!GLAName[0])
		{
			if (!BG_ParseAnimationFile("models/players/_humanoid/animation.cfg"))
			{
				Com_Printf( "Failed to load animation file %s\n", afilename );
				return;
			}
			return;
		}
		Q_strncpyz( afilename, GLAName, sizeof( afilename ));
		slash = strrchr( afilename, '/' );
		if ( slash )
		{
			Q_strncpyz(slash, "/animation.cfg", sizeof(afilename) - (slash - afilename));
		}	// Now afilename holds just the path to the animation.cfg
		else
		{	// Didn't find any slashes, this is a raw filename right in base (whish isn't a good thing)
			return;
		}

		// Try to load the animation.cfg for this model then.
		if ( !BG_ParseAnimationFile( afilename ) )
		{	// The GLA's animations failed
			if (!BG_ParseAnimationFile("models/players/_humanoid/animation.cfg"))
			{
				Com_Printf( "Failed to load animation file %s\n", afilename );
				return;
			}
		}
	}

	trap_G2API_AddBolt(client->ghoul2, 0, "*r_hand");
	trap_G2API_AddBolt(client->ghoul2, 0, "*l_hand");

	// NOTE - ensure this sequence of bolt and bone accessing are always the same because the client expects them in a certain order
	trap_G2API_SetBoneAnim(client->ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
	trap_G2API_SetBoneAngles(client->ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, level.time);
	trap_G2API_SetBoneAngles(client->ghoul2, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, level.time);

	if (!g2SaberInstance)
	{
		trap_G2API_InitGhoul2Model(&g2SaberInstance, "models/weapons2/saber/saber_w.glm", 0, 0, -20, 0, 0);

		if (g2SaberInstance)
		{
			// indicate we will be bolted to model 0 (ie the player) on bolt 0 (always the right hand) when we get copied
			trap_G2API_SetBoltInfo(g2SaberInstance, 0, 0);
			// now set up the gun bolt on it
			trap_G2API_AddBolt(g2SaberInstance, 0, "*flash");
		}
	}

	if (g2SaberInstance)
	{
		trap_G2API_CopySpecificGhoul2Model(g2SaberInstance, 0, client->ghoul2, 1);
	}
}

/*
===========
ClientUpdateConfigString

Update player's configstring.
===========
*/
void ClientUpdateConfigString( int clientNum ) {
	const gclient_t	*client = &level.clients[clientNum];
	char			*s;

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	s = va("n\\%s\\t\\%i\\model\\%s\\c1\\%i\\c2\\%i\\hc\\%i\\w\\%i\\l\\%i\\skill\\%i\\tt\\%i\\tl\\%i\\r\\%i",
		client->info.netname,
		client->sess.sessionTeam,
		client->info.model,
		client->info.color1,
		client->info.color2,
		client->info.maxHealth,
		client->sess.wins,
		client->sess.losses,
		client->info.skill,
		client->info.teamTask,
		client->sess.teamLeader,
		client->sess.referee);

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gclient_t	*client;
	qboolean	privateDuel;
	const char	*s;
	char		oldname[MAX_NETNAME];
	char		userinfo[MAX_INFO_STRING];
	int			health;

	client = &level.clients[clientNum];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	client->info.localClient = (qboolean)!strcmp( s, "localhost" );

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	client->info.predictItemPickup = (qboolean)!atoi( s );

	// set name
	Q_strncpyz ( oldname, client->info.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientSetName( client, s );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			Q_strncpyz( client->info.netname, "scoreboard", sizeof(client->info.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED && strcmp(oldname, client->info.netname) != 0 ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s %s\n\"",
				oldname, G_GetStripEdString("SVINGAME", "PLRENAME"),
				client->info.netname) );
		G_LogPrintf( LOG_RENAME, "ClientRename: %i %s: %s renamed to %s\n",
			clientNum, client->info.netname, oldname, client->info.netname );
	}

	// set max health
	health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( health < 1 || health > 100 ) {
		health = 100;
	}
	client->info.maxHealth = client->ps.stats[STAT_MAX_HEALTH] = health;

	// set model
	if( GT_Team(level.gametype) ) {
		s = Info_ValueForKey (userinfo, "team_model");
	} else {
		s = Info_ValueForKey (userinfo, "model");
	}

	Q_strncpyz( client->info.model, s, sizeof( client->info.model ) );

	s = Info_ValueForKey( userinfo, "teamoverlay" );
	if ( ! *s || atoi( s ) != 0 ) {
		client->info.teamInfo = qtrue;
	} else {
		client->info.teamInfo = qfalse;
	}

	/*
	s = Info_ValueForKey( userinfo, "cg_pmove_fixed" );
	if ( !*s || atoi( s ) == 0 ) {
		client->pers.pmoveFixed = qfalse;
	}
	else {
		client->pers.pmoveFixed = qtrue;
	}
	*/
	s = Info_ValueForKey( userinfo, "cg_privateDuel" );
	privateDuel = ( *s && atoi( s ) ) ? qtrue : qfalse;
	if (privateDuel != client->info.privateDuel) {
		client->info.privateDuel = privateDuel;

		if (privateDuel) {
			G_StartPrivateDuel( &g_entities[clientNum] );
		} else {
			G_StopPrivateDuel( &g_entities[clientNum] );
		}
	}

	// team task (0 = none, 1 = offence, 2 = defence)
	client->info.teamTask = atoi( Info_ValueForKey(userinfo, "teamtask") );

	// colors
	client->info.color1 = atoi( Info_ValueForKey( userinfo, "color1" ) );
	client->info.color2 = atoi( Info_ValueForKey( userinfo, "color2" ) );

	ClientUpdateConfigString( clientNum );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
const char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	const char	*value;
//	char		*areabits;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	char		address[MAX_INFO_VALUE];
	gentity_t	*ent;
	gentity_t	*te;
	qipv4_t		ip;
	int			qport;
	qboolean	reconnected = qfalse;
	clientProfile_t	savedProf;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	Q_strncpyz(address, value, sizeof(address));
	if ( G_FilterPacket( value ) ) {
		return "Banned.";
	}
	ip = G_StringToIPv4( value );

	value = Info_ValueForKey (userinfo, "qport");
	qport = atoi( value );

	if ( !( ent->r.svFlags & SVF_BOT ) && !isBot && g_needpass.integer ) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			static char sTemp[1024];
			Q_strncpyz(sTemp, G_GetStripEdString("SVINGAME","INVALID_PASSWORD"), sizeof (sTemp) );
			return sTemp;// return "Invalid password";
		}
	}

	if ( clientNum >= level.maxclients )
		return "Too many clients";

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	// players can't get rid of removed status by simply reconnecting
	if ( ip.ui == client->prof.ip.ui && qport == client->prof.qport ) {
		savedProf = client->prof;
		reconnected = qtrue;
	}

//	areabits = client->areabits;

	memset( client, 0, sizeof(*client) );

	if ( reconnected ) {
		client->prof = savedProf;
	} else {
		client->prof.ip = ip;
		client->prof.qport = qport;

		if ( level.voteClient == clientNum ) {
			level.voteCooldown = 0;
		}
	}

	client->pers.connected = CON_CONNECTING;
	client->pers.enterTime = level.time;

	G_ReadSessionData( client );
	// read or initialize the session data
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo, isBot, firstTime );
	}

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, (qboolean)!firstTime ) ) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );
	if ( isBot ) {
		G_LogPrintf( LOG_CONNECT, "BotConnect: %i: %s connected\n",
			clientNum, client->info.netname);
	} else {
		G_LogPrintf( LOG_CONNECT, "ClientConnect: %i %s: %s connected\n",
			clientNum, address, client->info.netname);
	}

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime ) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->info.netname, G_GetStripEdString("SVINGAME", "PLCONNECT")) );
	}

	if ( GT_Team(level.gametype) && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	te = G_TempEntity( vec3_origin, EV_CLIENTJOIN, ENTITYNUM_WORLD ); // ENTITYNUM_WORLD for a good reason
	te->r.svFlags |= SVF_BROADCAST;
	te->s.eventParm = clientNum;

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );

	return NULL;
}

void G_WriteClientSessionData( gclient_t *client );

/*
===========
ClientBegin

called when a client has finished connecting, and is ready to be
placed into the level. This will happen every level load (after
ClientConnect) and on transition between teams, but doesn't happen on
respawns. allowTeamReset will be set on map_restart.
============
*/
void ClientBegin( int clientNum, qboolean allowTeamReset ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags, i;
	char		userinfo[MAX_INFO_VALUE];
	const char	*modelname;
	const char	*gameversion;

	ent = g_entities + clientNum;

	if ((ent->r.svFlags & SVF_BOT) && GT_Team(level.gametype))
	{
		if (allowTeamReset)
		{
			const char *team;
			team_t preSess;

			//SetTeam(ent, "");
			ent->client->sess.sessionTeam = PickTeam(clientNum);
			trap_GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

			if (ent->client->sess.sessionTeam == TEAM_RED)
			{
				team = "Red";
			}
			else if (ent->client->sess.sessionTeam == TEAM_BLUE)
			{
				team = "Blue";
			}
			else {
				team = "Spectator";
			}

			Info_SetValueForKey( userinfo, "team", team );

			trap_SetUserinfo( clientNum, userinfo );

			ent->client->pers.persistant[ PERS_TEAM ] = ent->client->sess.sessionTeam;

			preSess = ent->client->sess.sessionTeam;
			G_ReadSessionData( ent->client );
			ent->client->sess.sessionTeam = preSess;
			G_WriteClientSessionData(ent->client);
			ClientUserinfoChanged( clientNum );
			ClientBegin(clientNum, qfalse);
			return;
		}
	}

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent, clientNum );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	memset( &client->pers, 0, sizeof( client->pers ) );
	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	G_UpdateClientReadyFlags();

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, (forcePowers_t)i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	client->ps.hasDetPackPlanted = qfalse;

	//first-time force power initialization
	WP_InitForcePowers( ent );

	// First time model setup for that player.
	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	modelname = Info_ValueForKey (userinfo, "model");
	SetupGameGhoul2Model(client, modelname);

	if (ent->client->ghoul2)
	{
		ent->bolt_Head = trap_G2API_AddBolt(ent->client->ghoul2, 0, "cranium");
		ent->bolt_Waist = trap_G2API_AddBolt(ent->client->ghoul2, 0, "thoracic");
		ent->bolt_LArm = trap_G2API_AddBolt(ent->client->ghoul2, 0, "lradius");
		ent->bolt_RArm = trap_G2API_AddBolt(ent->client->ghoul2, 0, "rradius");
		ent->bolt_LLeg = trap_G2API_AddBolt(ent->client->ghoul2, 0, "ltibia");
		ent->bolt_RLeg = trap_G2API_AddBolt(ent->client->ghoul2, 0, "rtibia");
		ent->bolt_Motion = trap_G2API_AddBolt(ent->client->ghoul2, 0, "Motion");
	}

	gameversion = Info_ValueForKey(userinfo, GAMEVERSION);
	client->pers.registered = strcmp(gameversion, GIT_VERSION) == 0 ? qtrue : qfalse;
	if (client->pers.registered) {
		if (client->sess.sessionTeam != TEAM_SPECTATOR && !client->sess.motdSeen) {
			trap_SendServerCommand( clientNum, "motd" );
			client->sess.motdSeen = qtrue;
		}
	} else {
		if (g_requireClientside.integer && client->sess.sessionTeam != TEAM_SPECTATOR) {
			SetTeam(ent, TEAM_SPECTATOR);
			return;
		}

		if (gameversion[0] == '\0') {
			trap_SendServerCommand( clientNum, "cp \"Please download\n" GAME_VERSION " clientside\"");
		} else {
			trap_SendServerCommand( clientNum, "cp \""
				"Your game and this server run different versions of\n"
				GAMEVERSION "\n"
				"Please download " GAME_VERSION " clientside\"");
		}

		if (trap_Cvar_VariableIntegerValue("mv_httpdownloads") && Info_ValueForKey(userinfo, "JK2MV")[0])
		{
			trap_SendServerCommand( clientNum, "print\"" S_LINE_PREFIX S_COLOR_WHITE
				"To update your clientside mod type `\\mv_allowdownload 1` in the console and reconnect.\n\"" );
		}
		else if (trap_Cvar_VariableIntegerValue("sv_allowDownload"))
		{
			trap_SendServerCommand( clientNum, "print\"" S_LINE_PREFIX S_COLOR_WHITE
				"To update your clientside mod type `\\cl_allowdownload 1` in the console and reconnect.\n\"" );
		}
		else
		{
			trap_SendServerCommand( clientNum, "print\"" S_LINE_PREFIX S_COLOR_WHITE
				"Download " GAME_VERSION " clientside from https://github.com/aufau/SaberMod/releases\n\"" );
		}
	}

	if ( ent->r.svFlags & SVF_BOT )
		gameversion = "BOT";
	else if ( gameversion[0] == '\0' )
		gameversion = "UNKNOWN";

	// don't start as dead
	if ( level.roundQueued || level.round == 0 )
		if ( client->sess.sessionTeam != TEAM_SPECTATOR )
			client->sess.spectatorState = SPECTATOR_NOT;

	G_LogPrintf( LOG_BEGIN, "ClientBegin: %i %s %s: %s joined the %s team\n",
		clientNum, BG_TeamName(client->sess.sessionTeam, CASE_UPPER), gameversion,
		client->info.netname, BG_TeamName(client->sess.sessionTeam, CASE_NORMAL) );

	// locate ent at a spawn point
	ClientSpawn( ent );

	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		// send event
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN, clientNum );
		tent->s.clientNum = ent->s.number;

		if ( level.gametype != GT_TOURNAMENT  ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " %s\n\"", client->info.netname, G_GetStripEdString("SVINGAME", "PLENTER")) );
		}
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	G_ClearClientLog(clientNum);
}

static qboolean AllForceDisabled(int force)
{
	int i;

	if (force)
	{
		for (i=0;i<NUM_FORCE_POWERS;i++)
		{
			if (!(force & (1<<i)))
			{
				return qfalse;
			}
		}

		return qtrue;
	}

	return qfalse;
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
extern qboolean WP_HasForcePowers( const playerState_t *ps );
void ClientSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	clientUserinfo_t	savedInfo;
	clientProfile_t		savedProf;
	gentity_t	*spawnPoint;
	int		flags;
	int		savedPing;
//	char	*savedAreaBits;
	int		eventSequence;
//	char	userinfo[MAX_INFO_STRING];
	forcedata_t			savedForce;
	void		*ghoul2save;
	int		saveSaberNum = ENTITYNUM_NONE;
	int		wDisable = 0;
	int		wSpawn = 0;

	index = ent - g_entities;
	client = ent->client;

	if (client->ps.fd.forceDoInit)
	{ //force a reread of force powers
		WP_InitForcePowers( ent );
	}
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( client->sess.spectatorState != SPECTATOR_NOT ) {
		spawnPoint = SelectSpectatorSpawnPoint (
						spawn_origin, spawn_angles);
	} else if ( GT_Flag(level.gametype) ) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint ( ent,
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles);
	}
	else if (level.gametype == GT_SAGA)
	{
		spawnPoint = SelectSagaSpawnPoint ( ent,
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles);
	}
	else {
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->info.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( ent, spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint ( ent,
					client->ps.origin,
					spawn_origin, spawn_angles);
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedInfo = client->info;
	savedProf = client->prof;
	savedPing = client->ps.ping;
//	savedAreaBits = client->areabits;
	eventSequence = client->ps.eventSequence;

	savedForce = client->ps.fd;

	ghoul2save = client->ghoul2;

	saveSaberNum = client->ps.saberEntityNum;

	memset (client, 0, sizeof(*client)); // bk FIXME: Com_Memset?

	//rww - Don't wipe the ghoul2 instance or the animation data
	client->ghoul2 = ghoul2save;

	//or the saber ent num
	client->ps.saberEntityNum = saveSaberNum;

	client->ps.fd = savedForce;

	client->ps.duelIndex = ENTITYNUM_NONE;

	client->pers = saved;
	client->info = savedInfo;
	client->sess = savedSess;
	client->prof = savedProf;
	client->ps.ping = savedPing;
//	client->areabits = savedAreaBits;
	client->lastkilled_client = -1;

	client->ps.eventSequence = eventSequence;
	if ( client->sess.spectatorState == SPECTATOR_NOT ) {
		// increment the spawncount so the client will detect the respawn
		client->pers.persistant[PERS_SPAWN_COUNT]++;
	}
	client->pers.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->info.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	//
	// give default weapons
	//

	if (level.gametype == GT_TOURNAMENT)
	{
		wDisable = g_duelWeaponDisable.integer;
	}
	else
	{
		wDisable = g_weaponDisable.integer;
	}

	// g_spawnWeapons 0 means original behaviour, we're checking it later
	wSpawn = g_spawnWeapons.integer & LEGAL_WEAPONS;

	if ( level.gametype != GT_HOLOCRON
		&& level.gametype != GT_JEDIMASTER
		&& !HasSetSaberOnly()
		&& !AllForceDisabled( g_forcePowerDisable.integer )
		&& g_trueJedi.integer )
	{
		if ( GT_Team(level.gametype)
			 && (client->sess.sessionTeam == TEAM_BLUE || client->sess.sessionTeam == TEAM_RED) )
		{//In Team games, force one side to be merc and other to be jedi
			if ( level.numPlayingClients > 0 )
			{//already someone in the game
				team_t	forceTeam = TEAM_SPECTATOR;

				for ( i = 0 ; i < level.maxclients ; i++ )
				{
					if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
						continue;
					}
					if ( level.clients[i].sess.sessionTeam == TEAM_BLUE || level.clients[i].sess.sessionTeam == TEAM_RED )
					{//in-game
						if ( WP_HasForcePowers( &level.clients[i].ps ) )
						{//this side is using force
							forceTeam = level.clients[i].sess.sessionTeam;
						}
						else
						{//other team is using force
							if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							{
								forceTeam = TEAM_RED;
							}
							else
							{
								forceTeam = TEAM_BLUE;
							}
						}
						break;
					}
				}
				if ( WP_HasForcePowers( &client->ps ) && client->sess.sessionTeam != forceTeam )
				{//using force but not on right team, switch him over
					SetTeam( ent, forceTeam );
					return;
				}
			}
		}

		if ( WP_HasForcePowers( &client->ps ) )
		{
			client->ps.trueNonJedi = qfalse;
			client->ps.trueJedi = qtrue;
			//make sure they only use the saber
			client->ps.weapon = WP_SABER;
			client->ps.stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
		else
		{//no force powers set
			client->ps.trueNonJedi = qtrue;
			client->ps.trueJedi = qfalse;
			// no saber for you, even with g_spawnWeapons
			wSpawn &= ~(1 << WP_SABER);

			if (!g_spawnWeapons.integer) {
				wSpawn = (1 << WP_BRYAR_PISTOL) | (1 << WP_BLASTER) | (1 << WP_BOWCASTER);
				wSpawn &= ~wDisable;
				wSpawn |= (1 << WP_STUN_BATON);
			}
		}
	}
	else
	{//jediVmerc is incompatible with this gametype, turn it off!
		trap_Cvar_Set( "g_jediVmerc", "0" );

		if (!g_spawnWeapons.integer) {
			// these few lines reproduce original logic suprisingly
			// give base weapon
			if (level.gametype == GT_HOLOCRON)
				wSpawn = 1 << WP_SABER;
			else if (level.gametype == GT_JEDIMASTER)
				wSpawn = 1 << WP_STUN_BATON;
			else if (client->ps.fd.forcePowerLevel[FP_SABERATTACK])
				wSpawn = 1 << WP_SABER;
			else
				wSpawn = 1 << WP_STUN_BATON;

			// give bryar conditionally
			if (!(wDisable & (1 << WP_BRYAR_PISTOL)) || level.gametype == GT_JEDIMASTER)
				wSpawn |= 1 << WP_BRYAR_PISTOL;
		}
	}

	// few sane restriction to match original behaviour without
	// introducing more cvars

	// WP_NONE is not usable yet, give saber or baton
	if (!(wSpawn & ~(1 << WP_NONE)))
		wSpawn = (1 << WP_STUN_BATON) | (1 << WP_SABER);

	// disable saber if player has no force attack
	if (!client->ps.fd.forcePowerLevel[FP_SABERATTACK])
		wSpawn &= ~(1 << WP_SABER);

	// disable stun baton if player has saber already
	if (wSpawn & (1 << WP_SABER))
		wSpawn &= ~(1 << WP_STUN_BATON);

	// weapons given on spawn need to be precached in ClearRegisteredItems()
	client->ps.stats[STAT_WEAPONS] = wSpawn | (1 << WP_NONE);

	//
	// initial weapon selection
	//

	if ((wSpawn & (1 << client->ps.weapon)) &&
		client->ps.weapon != WP_NONE)
	{
		// spawn player with last selected weapon if it's available
	}
	else
	{
		// if not try 1. bryar 2. saber 3. highest available
		// this is consisten with original behaviour
		if (client->ps.stats[STAT_WEAPONS] & (1 << WP_BRYAR_PISTOL)) {
			client->ps.weapon = WP_BRYAR_PISTOL;
		} else if (client->ps.stats[STAT_WEAPONS] & (1 << WP_SABER)) {
			client->ps.weapon = WP_SABER;
		} else {
			client->ps.weapon = WP_NONE;
			for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
				if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
					client->ps.weapon = (weapon_t)i;
					break;
				}
			}
		}
	}

	//
	// ammo distribution
	//

	if (g_infiniteAmmo.integer) {
		for (i = AMMO_NONE; i < AMMO_MAX; i++)
			client->ps.ammo[i] = INFINITE_AMMO;
	} else {
		// give ammo only for weapons owned by player
		for (i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++) {
			if (wSpawn & (1 << i)) {
				// gitem_t	*it = BG_FindItemForWeapon(i);
				ammo_t	ammo = weaponData[i].ammoIndex;

				// GT_Round check is here because it determines
				// spawning weapons too
				if (GT_Round(level.gametype)) {
					client->ps.ammo[ammo] = ammoData[ammo].max;
				} else {
					// give weapon pickup ammo quantity
					// client->ps.ammo[ammo] = item->quantity;
					client->ps.ammo[ammo] = ammoData[ammo].init;
				}
			}
		}
	}

	//
	// give default holdable items
	//

	{
		int iSpawn = g_spawnItems.integer & LEGAL_ITEMS;
		int item = 0;

		// items given on spawn need to be precached in ClearRegisteredItems()
		client->ps.stats[STAT_HOLDABLE_ITEMS] = iSpawn;

		if ( iSpawn ) {
			while ( !(iSpawn & 1) ) {
				item++;
				iSpawn >>= 1;
			}
		}

		client->ps.stats[STAT_HOLDABLE_ITEM] = item;
	}

	//
	// all the hard work we've done is wasted on spectators :-(
	//

	if ( client->sess.spectatorState != SPECTATOR_NOT )
	{
		client->ps.stats[STAT_WEAPONS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEMS] = 0;
		client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
		// for some reasone this causes bad animation after rejoining team
		// client->ps.weapon = WP_NONE;
		client->ps.weapon = WP_STUN_BATON;
	}

	client->ps.rocketLockIndex = MAX_CLIENTS;
	client->ps.rocketLockTime = 0;

	//rww - Set here to initialize the circling seeker drone to off.
	//A quick note about this so I don't forget how it works again:
	//ps.genericEnemyIndex is kept in sync between the server and client.
	//When it gets set then an entitystate value of the same name gets
	//set along with an entitystate flag in the shared bg code. Which
	//is why a value needs to be both on the player state and entity state.
	//(it doesn't seem to just carry over the entitystate value automatically
	//because entity state value is derived from player state data or some
	//such)
	client->ps.genericEnemyIndex = -1;

	client->ps.isJediMaster = qfalse;

	client->ps.fallingToDeath = 0;

	//Do per-spawn force power initialization
	WP_SpawnInitForcePowers( ent );

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 1.25f;

	// Start with a small amount of armor as well.
	client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * g_spawnShield.integer / 100;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		G_KillBox( ent );
		trap_LinkEntity (ent);

		// force the base weapon up
		client->ps.weaponstate = WEAPON_READY;
	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = WeaponReadyAnim[client->ps.weapon];
	client->ps.legsAnim = WeaponReadyAnim[client->ps.weapon];

	G_LogPrintf( LOG_SPAWN, "ClientSpawn: %d %s %d: %s spawned in the %s team as %s\n",
		index, BG_TeamName(client->sess.sessionTeam, CASE_UPPER),
		client->sess.spectatorState, client->info.netname,
		BG_TeamName(client->sess.sessionTeam, CASE_NORMAL),
		(client->sess.spectatorState == SPECTATOR_NOT) ? "player" : "spectator" );

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		int spawnWeapons = client->ps.stats[STAT_WEAPONS];
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );

		// fau - this respects new weapon selection rules earlier
		// while maintaining compatibility with maps giving weapons
		// on spawn.
		if ( client->ps.stats[STAT_WEAPONS] != spawnWeapons ) {
			// select the highest weapon number available, after any
			// spawn given items have fired
			client->ps.weapon = WP_NONE;
			for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- ) {
				if ( client->ps.stats[STAT_WEAPONS] & ( 1 << i ) ) {
					client->ps.weapon = (weapon_t)i;
					break;
				}
			}
		}
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	if ( !(ent->r.svFlags & SVF_BOT) ) {
		client->ps.commandTime = client->pers.cmd.serverTime - 100;
		ClientThink_real( ent );
	}

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue, qfalse );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );

		//init saber ent
		WP_SaberInitBladeData( ent );
	}

	if (g_spawnInvulnerability.integer)
	{
		ent->client->ps.eFlags |= EF_INVULNERABLE;
		ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
	}

	// run the presend to set anything else
	ClientEndFrame( ent );
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		if (ent->client->ps.fd.forcePowersActive & (1 << i))
		{
			WP_ForcePowerStop(ent, (forcePowers_t)i);
		}
		i++;
	}

	i = TRACK_CHANNEL_1;

	while (i < NUM_TRACK_CHANNELS)
	{
		if (ent->client->ps.fd.killSoundEntIndex[i-50] && ent->client->ps.fd.killSoundEntIndex[i-50] < MAX_GENTITIES && ent->client->ps.fd.killSoundEntIndex[i-50] > 0)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[i-50], CHAN_VOICE);
		}
		i++;
	}
	i = 0;

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT, clientNum );
		tent->s.clientNum = ent->s.number;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems( ent );
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		G_LogPrintf( LOG_CONNECT, "BotDisconnect: %i: %s disconnected\n",
			clientNum, ent->client->info.netname );
	} else {
		G_LogPrintf( LOG_CONNECT, "ClientDisconnect: %i: %s disconnected\n",
			clientNum, ent->client->info.netname );
	}

	// if we are playing in tourney mode, give a win to the other player and clear his frags for this round
	if ( (level.gametype == GT_TOURNAMENT )
		&& !level.intermissionQueued
		&& !level.intermissiontime
		&& !level.warmupTime ) {
		if ( level.sortedClients[1] == clientNum ) {
			level.clients[ level.sortedClients[0] ].pers.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[0] ].sess.wins++;
			ClientUpdateConfigString( level.sortedClients[0] );
		}
		else if ( level.sortedClients[0] == clientNum ) {
			level.clients[ level.sortedClients[1] ].pers.persistant[PERS_SCORE] = 0;
			level.clients[ level.sortedClients[1] ].sess.wins++;
			ClientUpdateConfigString( level.sortedClients[1] );
		}
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->pers.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, 0 );
	}

	G_ClearClientLog(clientNum);

	// disown entities blaming this client
	for ( i = 0; i < level.num_entities; i++ ) {
		if ( g_entities[i].inuse && g_entities[i].blameEntityNum == clientNum ) {
			G_BlameForEntity( ENTITYNUM_NONE, &g_entities[i] );
		}
	}
}


