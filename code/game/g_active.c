/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2020 Witold Pilat <witold.pilat@gmail.com>

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

qboolean PM_SaberInTransition( int move );
qboolean PM_SaberInStart( int move );
qboolean PM_SaberInReturn( saberMoveName_t move );

void P_SetTwitchInfo(gclient_t	*client)
{
	client->ps.painTime = level.time;
	client->ps.painDirection ^= 1;
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	int		count;
	vec3_t	angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = (360 + angles[PITCH]) * (256 / 360.0f);
		client->ps.damageYaw = angles[YAW] * (256 / 360.0f);
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		int fakeHealth = (player->health / 25) * 25 + irand(0, 24);

		// don't do more than two pain sounds a second
		if ( level.time - client->ps.painTime < 500 ) {
			return;
		}
		P_SetTwitchInfo(client);
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, fakeHealth);
		client->ps.damageEvent++;

		if (client->damage_armor && !client->damage_blood)
		{
			client->ps.damageType = 1; //pure shields
		}
		else if (client->damage_armor)
		{
			client->ps.damageType = 2; //shields and health
		}
		else
		{
			client->ps.damageType = 0; //pure health
		}
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean	envirosuit;
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = (qboolean)(ent->client->ps.powerups[PW_BATTLESUIT] > level.time);

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex(/*"*drown.wav"*/"sound/player/gurp1.wav"));
				} else if (id_rand()&1) {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp1.wav"));
				} else {
					G_Sound(ent, CHAN_VOICE, G_SoundIndex("sound/player/gurp2.wav"));
				}

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL,
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel &&
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL,
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL,
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}





//==============================================================
extern void G_ApplyKnockback( gentity_t *targ, const vec3_t newDir, float knockback );
void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf )
{
	float magnitude, my_mass;
	vec3_t	velocity;
	int cont;

	if( self->client )
	{
		VectorCopy( self->client->ps.velocity, velocity );
		my_mass = self->mass;
	}
	else
	{
		VectorCopy( self->s.pos.trDelta, velocity );
		if ( self->s.pos.trType == TR_GRAVITY )
		{
			velocity[2] -= 0.25f * g_gravity.value;
		}
		if( !self->mass )
		{
			my_mass = 1;
		}
		else if ( self->mass <= 10 )
		{
			my_mass = 10;
		}
		else
		{
			my_mass = self->mass;///10;
		}
	}

	magnitude = VectorLength( velocity ) * my_mass / 10;

	/*
	if(pointcontents(self.absmax)==CONTENT_WATER)//FIXME: or other watertypes
		magnitude/=3;							//water absorbs 2/3 velocity

	if(self.classname=="barrel"&&self.aflag)//rolling barrels are made for impacts!
		magnitude*=3;

	if(self.frozen>0&&magnitude<300&&self.flags&FL_ONGROUND&&loser==world&&self.velocity_z<-20&&self.last_onground+0.3<time)
		magnitude=300;
	*/

	if ( !self->client || self->client->ps.lastOnGround+300<level.time || ( self->client->ps.lastOnGround+100 < level.time && other->material >= MAT_GLASS ) )
	{
		vec3_t dir1, dir2;
		float force = 0, dot;

		if ( other->material >= MAT_GLASS )
			magnitude *= 2;

		//damage them
		if ( magnitude >= 100 && other->s.number < ENTITYNUM_WORLD )
		{
			VectorCopy( velocity, dir1 );
			VectorNormalize( dir1 );
			if( VectorCompare( other->r.currentOrigin, vec3_origin ) )
			{//a brush with no origin
				VectorCopy ( dir1, dir2 );
			}
			else
			{
				VectorSubtract( other->r.currentOrigin, self->r.currentOrigin, dir2 );
				VectorNormalize( dir2 );
			}

			dot = DotProduct( dir1, dir2 );

			if ( dot >= 0.2f )
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= (magnitude/50);

			cont = trap_PointContents( other->r.absmax, other->s.number );
			if( (cont&CONTENTS_WATER) )//|| (self.classname=="barrel"&&self.aflag))//FIXME: or other watertypes
			{
				force /= 3;							//water absorbs 2/3 velocity
			}

			/*
			if(self.frozen>0&&force>10)
				force=10;
			*/

			if( ( force >= 1 && other->s.number != 0 ) || force >= 10)
			{
	/*
				dprint("Damage other (");
				dprint(loser.classname);
				dprint("): ");
				dprint(ftos(force));
				dprint("\n");
	*/
				if ( other->r.svFlags & SVF_GLASS_BRUSH )
				{
					other->splashRadius = 0.25f * (self->r.maxs[0] - self->r.mins[0]);
				}
				if ( other->takedamage )
				{
					G_Damage( other, self, self, velocity, self->r.currentOrigin, force, DAMAGE_NO_ARMOR, MOD_CRUSH);//FIXME: MOD_IMPACT
				}
				else
				{
					G_ApplyKnockback( other, dir2, force );
				}
			}
		}

		if ( damageSelf && self->takedamage )
		{
			//Now damage me
			//FIXME: more lenient falling damage, especially for when driving a vehicle
			if ( self->client && self->client->ps.fd.forceJumpZStart )
			{//we were force-jumping
				if ( self->r.currentOrigin[2] >= self->client->ps.fd.forceJumpZStart )
				{//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.fd.forceJumpZStart-self->r.currentOrigin[2])/3;
				}
			}
			//if(self.classname!="monster_mezzoman"&&self.netname!="spider")//Cats always land on their feet
				if( ( magnitude >= 100 + self->health && self->s.number != 0 && self->s.weapon != WP_SABER ) || ( magnitude >= 700 ) )//&& self.safe_time < level.time ))//health here is used to simulate structural integrity
				{
					if ( (self->s.weapon == WP_SABER || self->s.number == 0) && self->client && self->client->ps.groundEntityNum < ENTITYNUM_NONE && magnitude < 1000 )
					{//players and jedi take less impact damage
						//allow for some lenience on high falls
						magnitude /= 2;
						/*
						if ( self.absorb_time >= time )//crouching on impact absorbs 1/2 the damage
						{
							magnitude/=2;
						}
						*/
					}
					magnitude /= 40;
					magnitude = magnitude - force/2;//If damage other, subtract half of that damage off of own injury
					if ( magnitude >= 1 )
					{
		//FIXME: Put in a thingtype impact sound function
		/*
						dprint("Damage self (");
						dprint(self.classname);
						dprint("): ");
						dprint(ftos(magnitude));
						dprint("\n");
		*/
						/*
						if ( self.classname=="player_sheep "&& self.flags&FL_ONGROUND && self.velocity_z > -50 )
							return;
						*/
						G_Damage( self, NULL, NULL, NULL, self->r.currentOrigin, magnitude/2, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
					}
				}
		}

		//FIXME: slow my velocity some?

		// NOTENOTE We don't use lastimpact as of yet
//		self->lastImpact = level.time;

		/*
		if(self.flags&FL_ONGROUND)
			self.last_onground=time;
		*/
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static const vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = G_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES, ent->s.number );

	// can't use ent->r.absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.spectatorState != SPECTATOR_NOT ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}


/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_MoverTouchPushTriggers( gentity_t *ent, const vec3_t oldOrg )
{
	int			i, num;
	float		step, stepSize, dist;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs, dir, size, checkSpot;
	static const vec3_t	range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if ( !VectorLengthSquared( ent->s.pos.trDelta ) )
	{
		return;
	}

	VectorSubtract( ent->r.mins, ent->r.maxs, size );
	stepSize = VectorLength( size );
	if ( stepSize < 1 )
	{
		stepSize = 1;
	}

	VectorSubtract( ent->r.currentOrigin, oldOrg, dir );
	dist = VectorNormalize( dir );
	for ( step = 0; step <= dist; step += stepSize )
	{
		VectorMA( ent->r.currentOrigin, step, dir, checkSpot );
		VectorSubtract( checkSpot, range, mins );
		VectorAdd( checkSpot, range, maxs );

		num = G_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES, ent->s.number );

		// can't use ent->r.absmin, because that has a one unit pad
		VectorAdd( checkSpot, ent->r.mins, mins );
		VectorAdd( checkSpot, ent->r.maxs, maxs );

		for ( i=0 ; i<num ; i++ )
		{
			hit = &g_entities[touch[i]];

			if ( hit->s.eType != ET_PUSH_TRIGGER )
			{
				continue;
			}

			if ( hit->touch == NULL )
			{
				continue;
			}

			if ( !( hit->r.contents & CONTENTS_TRIGGER ) )
			{
				continue;
			}


			if ( !trap_EntityContact( mins, maxs, hit ) )
			{
				continue;
			}

			memset( &trace, 0, sizeof(trace) );

			if ( hit->touch != NULL )
			{
				hit->touch(hit, ent, &trace);
			}
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal
		client->ps.basespeed = 400;

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = G_Trace;
		pm.pointcontents = trap_PointContents;

		pm.animations = NULL;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( ( client->buttons & BUTTON_USE ) && ! ( client->oldbuttons & BUTTON_USE ) ) {
		Cmd_SmartFollowCycle_f( ent );
	}

	if (client->sess.spectatorState == SPECTATOR_FOLLOW && (ucmd->upmove > 0))
	{ //jump now removes you from follow mode
		StopFollowing(ent);
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( client->pers.cmd.forwardmove ||
		client->pers.cmd.rightmove ||
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & (BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
	{
		client->inactivityTime = level.time;
		client->inactivityWarning = qfalse;
	}
	else if ( g_inactivity.integer && !client->info.localClient )
	{
		if ( level.time > client->inactivityTime + g_inactivity.integer * 1000 - 10000 ) {
			if ( level.time > client->inactivityTime + g_inactivity.integer * 1000 ) {
				trap_DropClient( client - level.clients, "Dropped due to inactivity" );
				return qfalse;
			}
			if ( !client->inactivityWarning ) {
				client->inactivityWarning = qtrue;
				trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
			}
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 )
	{
		client->timeResidual -= 1000;

		// count down health when over max
		if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health--;
			client->ps.stats[STAT_HEALTH] = ent->health;
		}

		// count down armor when over max
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
		}
	}
}

/*
===========
G_UpdateClientReadyFlags
============
*/
void G_UpdateClientReadyFlags( void ) {
	int mask = 0;
	int i;

	for (i = 0; i < level.maxclients; i++) {
		gentity_t *ent = &g_entities[i];

		if (ent->inuse &&
			ent->client->pers.connected == CON_CONNECTED &&
			ent->client->pers.ready)
		{
			mask |= 1 << i;
		}
	}

	trap_SetConfigstring(CS_READY, va("%d", mask));
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( level.gametype != GT_TOURNAMENT || level.duelExit ) {
		if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
			// this used to be an ^1 but once a player says ready, it should stick
			if (!client->pers.ready) {
				client->pers.ready = qtrue;
				G_UpdateClientReadyFlags();
			}
		}
	}
}

/*
====================
ClientPauseThink
====================
*/
static void ClientPauseThink( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	usercmd_t	*cmd = &client->pers.cmd;

	// Pmove substitute, should be Pmove_paused
	client->ps.commandTime = cmd->serverTime;
	if ( cmd->buttons & BUTTON_TALK ) {
		ent->s.eFlags |= EF_TALK;
		client->ps.eFlags |= EF_TALK;
	} else {
		ent->s.eFlags &= ~EF_TALK;
		client->ps.eFlags &= ~EF_TALK;
	}
	// can't move, stop prediction
	client->ps.pm_type = PM_PAUSED;
	// force the same view angles as before
	SetClientViewAngle(ent, client->ps.viewangles);
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i;//, j;
	int		event;
	gclient_t *client;
	int		damage;
//	vec3_t	origin, angles;
//	qboolean	fired;
//	gitem_t *item;
//	gentity_t *drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL:
		case EV_ROLL:
			{
				int delta = client->ps.eventParms[ i & (MAX_PS_EVENTS-1) ];

				if (client->ps.fallingToDeath)
				{
					break;
				}

				if ( ent->s.eType != ET_PLAYER )
				{
					break;		// not in the player model
				}

				if ( g_dmflags.integer & DF_NO_FALLING )
				{
					break;
				}

				if (delta <= 44)
				{
					break;
				}

				damage = delta*0.16f; //good enough for now, I guess

				ent->pain_debounce_time = level.time + 200;	// no normal pain sound
				G_Damage (ent, NULL, NULL, NULL, NULL, damage, DAMAGE_NO_ARMOR, MOD_FALLING);
			}
			break;
		case EV_FIRE_WEAPON:
			FireWeapon( ent, qfalse );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_ALT_FIRE:
			FireWeapon( ent, qtrue );
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		case EV_SABER_ATTACK:
			ent->client->dangerTime = level.time;
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = 0;
			break;

		//rww - Note that these must be in the same order (ITEM#-wise) as they are in holdable_t
		case EV_USE_ITEM1: //seeker droid
			ItemUse_Seeker(ent);
			break;
		case EV_USE_ITEM2: //shield
			ItemUse_Shield(ent);
			break;
		case EV_USE_ITEM3: //medpack
			ItemUse_MedPack(ent);
			break;
		case EV_USE_ITEM4: //datapad
			//G_Printf("Used Datapad\n");
			break;
		case EV_USE_ITEM5: //binoculars
			ItemUse_Binoculars(ent);
			break;
		case EV_USE_ITEM6: //sentry gun
			ItemUse_Sentry(ent);
			break;

		default:
			break;
		}
	}

}

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, (entity_event_t)event, ps->clientNum );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue, qfalse );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

extern int saberOffSound;
extern int saberOnSound;

/*
==================
G_UpdateClientBroadcasts

Determines whether this client should be broadcast to any other clients.
A client is broadcast when another client is using force sight or is
==================
*/
#define MAX_JEDIMASTER_DISTANCE	2500
#define MAX_JEDIMASTER_FOV		100

#define MAX_SIGHT_DISTANCE		1500
#define MAX_SIGHT_FOV			100

static void G_UpdateForceSightBroadcasts ( gentity_t *self )
{
	int i;

	// Any clients with force sight on should see this client
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		// Not using force sight so we shouldnt broadcast to this one
		if ( !(ent->client->ps.fd.forcePowersActive & (1<<FP_SEE) ) )
		{
			continue;
		}

		VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_SIGHT_DISTANCE * MAX_SIGHT_DISTANCE )
		{
			continue;
		}

		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_SIGHT_FOV, angles ) )
		{
			break;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));

		break;
	}
}

static void G_UpdateJediMasterBroadcasts ( gentity_t *self )
{
	int i;

	// Not jedi master mode then nothing to do
	if ( level.gametype != GT_JEDIMASTER )
	{
		return;
	}

	// This client isnt the jedi master so it shouldnt broadcast
	if ( !self->client->ps.isJediMaster )
	{
		return;
	}

	// Broadcast ourself to all clients within range
	for ( i = 0; i < level.numConnectedClients; i ++ )
	{
		gentity_t *ent = &g_entities[level.sortedClients[i]];
		float	  dist;
		vec3_t	  angles;

		if ( ent == self )
		{
			continue;
		}

		VectorSubtract( self->client->ps.origin, ent->client->ps.origin, angles );
		dist = VectorLengthSquared ( angles );
		vectoangles ( angles, angles );

		// Too far away then just forget it
		if ( dist > MAX_JEDIMASTER_DISTANCE * MAX_JEDIMASTER_DISTANCE )
		{
			continue;
		}

		// If not within the field of view then forget it
		if ( !InFieldOfVision ( ent->client->ps.viewangles, MAX_JEDIMASTER_FOV, angles ) )
		{
			continue;
		}

		// Turn on the broadcast bit for the master and since there is only one
		// master we are done
		self->r.broadcastClients[ent->s.number/32] |= (1 << (ent->s.number%32));
	}
}

void G_UpdateClientBroadcasts ( gentity_t *self )
{
	// Clear all the broadcast bits for this client
	memset ( self->r.broadcastClients, 0, sizeof ( self->r.broadcastClients ) );

	// The jedi master is broadcast to everyone in range
	G_UpdateJediMasterBroadcasts ( self );

	// Anyone with force sight on should see this client
	G_UpdateForceSightBroadcasts ( self );
}

static void G_SwitchTeam( gentity_t *ent ) {
	gclient_t	*client;
	team_t		team, oldTeam;
	int			teamLeader;
	int			clientNum;

	client = ent->client;
	clientNum = client - level.clients;
	oldTeam = client->sess.sessionTeam;

	if (oldTeam == TEAM_RED) {
		team = TEAM_BLUE;
	} else if (oldTeam == TEAM_BLUE) {
		team = TEAM_RED;
	} else {
		return;
	}

	client->sess.sessionTeam = team;
	client->ps.fd.forceDoInit = qtrue; // every time we change teams make sure our force powers are set right
	client->sess.teamLeader = qfalse;
	teamLeader = TeamLeader( team );
	// if there is no team leader or the team leader is a bot and this client is not a bot
	if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
		SetLeader( team, clientNum );
	}
	// make sure there is a team leader on the team the player came from
	CheckTeamLeader( oldTeam );

	// get and distribute relevent paramters
	CalculateRanks();
	ClientUpdateConfigString( clientNum );
}

/*
==============
G_Respawn

Handle manual respawn
==============
*/
void G_Respawn( gentity_t *ent ) {
	gclient_t	*client = ent->client;

	if ( level.intermissionQueued )
		return;

	if ( level.gametype == GT_REDROVER ) {
		if ( !level.roundQueued )
			G_SwitchTeam(ent);
		respawn( ent );
	} else if ( level.round > 0 && !level.roundQueued && !level.warmupTime) {
		team_t	team = client->sess.sessionTeam;

		// currently this is the only entry point for spactating while
		// not in TEAM_SPECTATOR. The exit is in NextRound.
		SetTeamSpec( ent, team, SPECTATOR_FOLLOW, FOLLOW_TEAM );
	} else {
		respawn( ent );
	}
}

static void G_FinishDuel ( gentity_t *ent )
{
	ent->dimension = DEFAULT_DIMENSION;
	// remove once all non-player entities use dimensions
	if (ent->client->ps.saberEntityNum != ENTITYNUM_NONE) {
		g_entities[ent->client->ps.saberEntityNum].dimension = DEFAULT_DIMENSION;
	}
	ent->client->ps.duelInProgress = qfalse;
	ent->client->duelStarted = qfalse;
	G_AddEvent(ent, EV_PRIVATE_DUEL, 0);
	G_StopPrivateDuel(ent);

	if (ent->health > 0) {
		G_KillBox(ent);
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	int			i;
	usercmd_t	*ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		// always initialize playerstate on ClientBegin
		if ( client->ps.commandTime != 0 ) {
			return;
		}
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( g_pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (g_pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if ( g_pmove_fixed.integer || client->info.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + g_pmove_msec.integer-1) / g_pmove_msec.integer) * g_pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.spectatorState != SPECTATOR_NOT ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD )
			return;
		if ( ent->r.svFlags & SVF_BOT )
			return;

		SpectatorThink( ent, ucmd );
		return;
	}

	if (level.unpauseTime > level.time) {
		ClientPauseThink( ent );
		return;
	}

	if (ent && ent->client && (ent->client->ps.eFlags & EF_INVULNERABLE))
	{
		if (ent->client->invulnerableTimer <= level.time)
		{
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		}
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	// clear the rewards if time
	if ( level.time > client->rewardTime ) {
		client->ps.eFlags &= ~EF_AWARDS;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.eFlags & EF_DISINTEGRATION ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else if ( GT_Round(level.gametype) &&
		( level.intermissionQueued || level.roundQueued ) ) {
		client->ps.pm_type = PM_HARMLESS;
	} else if (client->ps.forceGripChangeMovetype) {
		client->ps.pm_type = client->ps.forceGripChangeMovetype;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.gravity = g_gravity.integer;

	// set speed
	client->ps.speed = g_speed.integer;
	client->ps.basespeed = g_speed.integer;

	if (client->ps.pm_type == PM_HARMLESS) {
		if (client->ps.weapon == WP_SABER && !client->ps.saberHolstered) {
			Cmd_ToggleSaber_f(ent);
		}
	}
	if (ent->client->ps.duelInProgress)
	{
		gentity_t *duelAgainst = &g_entities[ent->client->ps.duelIndex];

		switch(ucmd->generic_cmd)
		{
		case GENCMD_USE_SEEKER:
		case GENCMD_USE_FIELD:
		case GENCMD_USE_BACTA:
		case GENCMD_USE_SENTRY:
			ucmd->generic_cmd = 0;
		}

		if (ent->client->ps.duelTime < level.time)
		{
			if (!ent->client->duelStarted)
			{
				G_AddEvent(ent, EV_PRIVATE_DUEL, 2);
				ent->client->duelStarted = qtrue;
			}

			if (duelAgainst && duelAgainst->client && duelAgainst->inuse &&
				!duelAgainst->client->duelStarted)
			{
				G_AddEvent(duelAgainst, EV_PRIVATE_DUEL, 2);
				duelAgainst->client->duelStarted = qtrue;
			}
		}

		if (!duelAgainst || !duelAgainst->client || !duelAgainst->inuse ||
			duelAgainst->client->ps.duelIndex != ent->s.number)
		{
			G_FinishDuel(ent);
		}
		else if (duelAgainst->health < 1 || duelAgainst->client->ps.stats[STAT_HEALTH] < 1)
		{
			char *s;

			G_FinishDuel(ent);
			G_FinishDuel(duelAgainst);

			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				int duelTime = (level.time - ent->client->ps.duelTime) / 1000;
				int minutes = duelTime / 60;
				int seconds = duelTime % 60;

				G_LogPrintf(LOG_PRIVATE_DUEL,
					"DuelWin: %i %i %i %i: %s has defeated %s in %02i:%02i with %i/%i left\n",
					ent->s.number, duelAgainst->s.number,
					ent->client->ps.stats[STAT_HEALTH],	ent->client->ps.stats[STAT_ARMOR],
					ent->client->info.netname, duelAgainst->client->info.netname,
					minutes, seconds,
					ent->client->ps.stats[STAT_HEALTH],	ent->client->ps.stats[STAT_ARMOR]);

				s = va("print \"%s" S_COLOR_WHITE " %s %s" S_COLOR_WHITE
					" in " S_COLOR_CYAN "%02i" S_COLOR_WHITE ":" S_COLOR_CYAN "%02i" S_COLOR_WHITE
					" with " S_COLOR_RED "%i" S_COLOR_WHITE "/" S_COLOR_GREEN "%i" S_COLOR_WHITE " left!\n\"",
					ent->client->info.netname,
					G_GetStripEdString("SVINGAME", "PLDUELWINNER"),
					duelAgainst->client->info.netname,
					minutes,
					seconds,
					ent->client->ps.stats[STAT_HEALTH],
					ent->client->ps.stats[STAT_ARMOR]);
			}
			else
			{ //it was a draw, because we both managed to die in the same frame
				G_LogPrintf(LOG_PRIVATE_DUEL,
					"DuelTie: %i %i: The duel between %s and %s is ended in a draw, both fighters have died\n",
					ent->s.number, duelAgainst->s.number,
					ent->client->info.netname, duelAgainst->client->info.netname);

				s = va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELTIE"));
			}
			trap_SendServerCommand(-1, s);

			//Winner gets full health.. providing he's still alive
			if (ent->health > 0 && ent->client->ps.stats[STAT_HEALTH] > 0)
			{
				if (ent->health < ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_HEALTH] = ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
				}

				if (g_spawnInvulnerability.integer)
				{
					ent->client->ps.eFlags |= EF_INVULNERABLE;
					ent->client->invulnerableTimer = level.time + g_spawnInvulnerability.integer;
				}

				ent->client->sess.wins++;
			}
			else
			{
				ent->client->sess.losses++;
			}

			duelAgainst->client->sess.losses++;
			ClientUpdateConfigString( ent->client->ps.clientNum );
			ClientUpdateConfigString( duelAgainst->client->ps.clientNum );
		}
		else
		{
			vec3_t vSub;
			float subLen = 0;

			VectorSubtract(ent->client->ps.origin, duelAgainst->client->ps.origin, vSub);
			subLen = VectorLength(vSub);

			if (subLen >= 1024)
			{
				G_FinishDuel(ent);
				G_FinishDuel(duelAgainst);

				G_LogPrintf(LOG_PRIVATE_DUEL, "DuelStop: %i %i: The duel between %s and %s has been severed\n",
					ent->s.number, duelAgainst->s.number,
					ent->client->info.netname, duelAgainst->client->info.netname);
				trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStripEdString("SVINGAME", "PLDUELSTOP")) );
			}
		}
	}

	/*
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}
	*/

	if (client->ps.usingATST && ent->health > 0)
	{ //we have special shot clip boxes as an ATST
		ent->r.contents |= CONTENTS_NOSHOT;
		ATST_ManageDamageBoxes(ent);
	}
	else
	{
		ent->r.contents &= ~CONTENTS_NOSHOT;
		client->damageBoxHandle_Head = 0;
		client->damageBoxHandle_RLeg = 0;
		client->damageBoxHandle_LLeg = 0;
	}

	//rww - moved this stuff into the pmove code so that it's predicted properly
	//BG_AdjustClientSpeed(&client->ps, &client->pers.cmd, level.time);

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	if (!level.intermissionQueued &&
		ent->client && ent->client->ps.fallingToDeath &&
		(level.time - FALL_FADE_TIME) > ent->client->ps.fallingToDeath)
	{ //die!
		player_die(ent, ent, ent, 100000, MOD_FALLING);
		G_Respawn(ent);

		G_MuteSound(ent->s.number, CHAN_VOICE); //stop screaming, because you are dead!

		// ClientThink_real is called in G_Respawn with updated
		// gclient_t state, don't continue here
		return;
	}

	if (ent->client->ps.otherKillerTime > level.time &&
		ent->client->ps.groundEntityNum != ENTITYNUM_NONE &&
		ent->client->ps.otherKillerDebounceTime < level.time)
	{
		ent->client->ps.otherKillerTime = 0;
		ent->client->ps.otherKiller = ENTITYNUM_NONE;
	}
	else if (ent->client->ps.otherKillerTime > level.time &&
		ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
	{
		if (ent->client->ps.otherKillerDebounceTime < (level.time + 100))
		{
			ent->client->ps.otherKillerDebounceTime = level.time + 100;
		}
	}

//	WP_ForcePowersUpdate( ent, msec, ucmd); //update any active force powers
//	WP_SaberPositionUpdate(ent, ucmd); //check the server-side saber point, do apprioriate server-side actions (effects are cs-only)

	if ((ent->client->pers.cmd.buttons & BUTTON_USE) && ent->client->ps.useDelay < level.time)
	{
		TryUse(ent);
		ent->client->ps.useDelay = level.time + 100;
	}

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = G_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = (qboolean)((g_dmflags.integer & DF_NO_FOOTSTEPS) > 0);
	pm.noKick = (qboolean)((g_dmflags.integer & DF_NO_KICK) > 0);
	pm.noYDFA = (qboolean)((g_dmflags.integer & DF_NO_YDFA) > 0);
	pm.newPmove = (qboolean)((g_dmflags.integer & DF_NEW_PMOVE) > 0);

	pm.pmove_fixed = g_pmove_fixed.integer | client->info.pmoveFixed;
	pm.pmove_msec = g_pmove_msec.integer;

	pm.animations = bgGlobalAnimations;//NULL;

	pm.gametype = level.gametype;

	VectorCopy( client->ps.origin, client->oldOrigin );

	if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
		if ( level.time - level.intermissionQueued >= 1000  ) {
			pm.cmd.buttons = 0;
			pm.cmd.forwardmove = 0;
			pm.cmd.rightmove = 0;
			pm.cmd.upmove = 0;
			if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
				trap_SendConsoleCommand( EXEC_APPEND, "centerview\n");
			}
			ent->client->ps.pm_type = PM_SPINTERMISSION;
		}
	}

	for ( i = 0 ; i < level.maxclients ; i++ )
	{
		if (g_entities[i].inuse)
		{
			pm.bgClients[i] = &g_entities[i].client->ps;
		}
	}

	if (ent->client->ps.saberLockTime > level.time)
	{
		gentity_t *blockOpp = &g_entities[client->ps.saberLockEnemy];

		if (blockOpp->inuse && blockOpp->client)
		{
			vec3_t lockDir, lockAng;

			//VectorClear( ent->client->ps.velocity );
			VectorSubtract( blockOpp->r.currentOrigin, ent->r.currentOrigin, lockDir );
			//lockAng[YAW] = vectoyaw( defDir );
			vectoangles(lockDir, lockAng);
			SetClientViewAngle( ent, lockAng );
		} else {
			client->ps.weaponTime = 0;
			client->ps.saberLockFrame = 0;
			client->ps.saberLockTime = 0;
			client->ps.saberLockEnemy = 0; // ENTITYNUM_NONE (could break unpatched cgame modules?)
			client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
		}

		if ( ( ent->client->buttons & BUTTON_ATTACK ) && ! ( ent->client->oldbuttons & BUTTON_ATTACK ) )
		{
			ent->client->ps.saberLockHits++;
		}
		if (ent->client->ps.saberLockHits > 2)
		{
			if (!ent->client->ps.saberLockAdvance)
			{
				ent->client->ps.saberLockHits -= 3;
			}
			ent->client->ps.saberLockAdvance = qtrue;
		}
	}
	else
	{
		ent->client->ps.saberLockFrame = 0;
		//check for taunt
		if ( (pm.cmd.generic_cmd == GENCMD_ENGAGE_DUEL) && (level.gametype == GT_TOURNAMENT) )
		{//already in a duel, make it a taunt command
			pm.cmd.buttons |= BUTTON_GESTURE;
		}
	}

	Pmove (&pm);

	if (pm.checkDuelLoss)
	{
		if (pm.checkDuelLoss > 0 && pm.checkDuelLoss <= MAX_CLIENTS)
		{
			gentity_t *clientLost = &g_entities[pm.checkDuelLoss-1];

			if (clientLost && clientLost->inuse && clientLost->client && Q_irand(0, 40) > clientLost->health)
			{
				vec3_t attDir;
				VectorSubtract(ent->client->ps.origin, clientLost->client->ps.origin, attDir);

				VectorClear(clientLost->client->ps.velocity);
				clientLost->client->ps.forceHandExtend = HANDEXTEND_NONE;
				clientLost->client->ps.forceHandExtendTime = 0;

				gGAvoidDismember = 1;
				G_Damage(clientLost, ent, ent, attDir, clientLost->client->ps.origin, 9999, DAMAGE_NO_PROTECTION, MOD_SABER);

				if (clientLost->health < 1)
				{
					animNumber_t tempAnim = ANIM(clientLost->client->ps.legsAnim);
					gGAvoidDismember = 2;
					G_CheckForDismemberment(clientLost, clientLost->client->ps.origin, 999, tempAnim);
				}

				gGAvoidDismember = 0;
			}
		}

		pm.checkDuelLoss = 0;
	}

	switch(pm.cmd.generic_cmd)
	{
	case 0:
		break;
	case GENCMD_SABERSWITCH:
		Cmd_ToggleSaber_f(ent);
		break;
	case GENCMD_ENGAGE_DUEL:
		if ( level.gametype == GT_TOURNAMENT )
		{//already in a duel, made it a taunt command
		}
		else
		{
			Cmd_EngageDuel_f(ent);
		}
		break;
	case GENCMD_FORCE_HEAL:
		ForceHeal(ent);
		break;
	case GENCMD_FORCE_SPEED:
		ForceSpeed(ent, 0);
		break;
	case GENCMD_FORCE_THROW:
		ForceThrow(ent, qfalse);
		break;
	case GENCMD_FORCE_PULL:
		ForceThrow(ent, qtrue);
		break;
	case GENCMD_FORCE_DISTRACT:
		ForceTelepathy(ent);
		break;
	case GENCMD_FORCE_RAGE:
		ForceRage(ent);
		break;
	case GENCMD_FORCE_PROTECT:
		ForceProtect(ent);
		break;
	case GENCMD_FORCE_ABSORB:
		ForceAbsorb(ent);
		break;
	case GENCMD_FORCE_HEALOTHER:
		ForceTeamHeal(ent);
		break;
	case GENCMD_FORCE_FORCEPOWEROTHER:
		ForceTeamForceReplenish(ent);
		break;
	case GENCMD_FORCE_SEEING:
		ForceSeeing(ent);
		break;
	case GENCMD_USE_SEEKER:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SEEKER)) &&
			G_ItemUsable(&ent->client->ps, HI_SEEKER) )
		{
			ItemUse_Seeker(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SEEKER, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
		}
		break;
	case GENCMD_USE_FIELD:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SHIELD)) &&
			G_ItemUsable(&ent->client->ps, HI_SHIELD) )
		{
			ItemUse_Shield(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SHIELD, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SHIELD);
		}
		break;
	case GENCMD_USE_BACTA:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC)) &&
			G_ItemUsable(&ent->client->ps, HI_MEDPAC) )
		{
			ItemUse_MedPack(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_MEDPAC, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_MEDPAC);
		}
		break;
	case GENCMD_USE_ELECTROBINOCULARS:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
			G_ItemUsable(&ent->client->ps, HI_BINOCULARS) )
		{
			ItemUse_Binoculars(ent);
			if (ent->client->ps.zoomMode == ZOOM_NONE)
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
			}
			else
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
			}
		}
		break;
	case GENCMD_ZOOM:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_BINOCULARS)) &&
			G_ItemUsable(&ent->client->ps, HI_BINOCULARS) )
		{
			ItemUse_Binoculars(ent);
			if (ent->client->ps.zoomMode == ZOOM_NONE)
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 1);
			}
			else
			{
				G_AddEvent(ent, EV_USE_ITEM0+HI_BINOCULARS, 2);
			}
		}
		break;
	case GENCMD_USE_SENTRY:
		if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_SENTRY_GUN)) &&
			G_ItemUsable(&ent->client->ps, HI_SENTRY_GUN) )
		{
			ItemUse_Sentry(ent);
			G_AddEvent(ent, EV_USE_ITEM0+HI_SENTRY_GUN, 0);
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SENTRY_GUN);
		}
		break;
	case GENCMD_SABERATTACKCYCLE:
		Cmd_SaberAttackCycle_f(ent);
		break;
	default:
		break;
	}

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue, (qboolean)g_smoothClients.integer );

	SendPendingPredictableEvents( &ent->client->ps );

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	if ( pm.useEvent )
	{
		//TODO: Use
//		TryUse( ent );
	}

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
//	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// Did we kick someone in our pmove sequence?
	if (client->ps.forceKickFlip)
	{
		gentity_t *faceKicked = &g_entities[client->ps.forceKickFlip-1];

		if (faceKicked && faceKicked->client && g_kickMethod.integer > KICK_NOEFFECT &&
			(!OnSameTeam(ent, faceKicked) || g_friendlyFire.integer))
		{
			if ( faceKicked && faceKicked->client && faceKicked->health && faceKicked->takedamage )
			{//push them away and do pain
				vec3_t oppDir;
				int strength;
				int dflag = DAMAGE_NO_ARMOR;

				// pmove sets velocity to 150 on XY. Z could go as
				// low as 200, resulting in strength 12
				strength = 0.05f * (int)VectorNormalize2( client->ps.velocity, oppDir );

				if (g_kickMethod.integer == KICK_NODAMAGE) {
					dflag |= DAMAGE_NO_DAMAGE;
				}

				VectorScale( oppDir, -1, oppDir );

				G_Damage( faceKicked, ent, ent, oppDir, client->ps.origin, strength, dflag, MOD_MELEE );

				if (g_kickMethod.integer == KICK_LEAGUE ||
					faceKicked->client->ps.weapon != WP_SABER ||
					faceKicked->client->ps.fd.saberAnimLevel < FORCE_LEVEL_3 ||
					(   !BG_SaberInAttack(faceKicked->client->ps.saberMove) &&
						!PM_SaberInStart(faceKicked->client->ps.saberMove) &&
						!PM_SaberInReturn(faceKicked->client->ps.saberMove) &&
						!PM_SaberInTransition(faceKicked->client->ps.saberMove)	) )
				{
					if (faceKicked->health > 0 &&
						faceKicked->client->ps.stats[STAT_HEALTH] > 0 &&
						faceKicked->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN)
					{
						if (Q_irand(1, 10) <= 3)
						{ //only actually knock over sometimes, but always do velocity hit
							faceKicked->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							faceKicked->client->ps.forceHandExtendTime = level.time + 1100;
							faceKicked->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
						}

						faceKicked->client->ps.otherKiller = ent->s.number;
						faceKicked->client->ps.otherKillerTime = level.time + 5000;
						faceKicked->client->ps.otherKillerDebounceTime = level.time + 100;

						faceKicked->client->ps.velocity[0] = oppDir[0]*(strength*40);
						faceKicked->client->ps.velocity[1] = oppDir[1]*(strength*40);
						faceKicked->client->ps.velocity[2] = 200;
					}
				}

				G_Sound( faceKicked, CHAN_AUTO, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
			}
		}

		client->ps.forceKickFlip = 0;
	}

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime && !gDoSlowMoDuel ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 &&
				( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
				G_Respawn( ent );
				return;
			}

			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
				G_Respawn( ent );
			}
		}
		else if (gDoSlowMoDuel)
		{
			client->respawnTime = level.time + 1000;
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

	G_UpdateClientBroadcasts ( ent );
}

/*
==================
G_CheckClientTimeouts

Checks whether a client has exceded any timeouts and act accordingly
==================
*/
void G_CheckClientTimeouts ( gentity_t *ent )
{
	// Only timeout supported right now is the timeout to spectator mode
	if ( !g_timeouttospec.integer )
	{
		return;
	}

	// Already a spectator, no need to boot them to spectator
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		return;
	}

	// See how long its been since a command was received by the client and if its
	// longer than the timeout to spectator then force this client into spectator mode
	if ( level.time - ent->client->pers.cmd.serverTime > g_timeouttospec.integer * 1000 )
	{
		SetTeam ( ent, TEAM_SPECTATOR );
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	gclient_t *client = &level.clients[clientNum];

	trap_GetUsercmd( clientNum, &client->pers.cmd );

	client->cmdIndex++;
	client->cmdStats[client->cmdIndex & CMD_MASK].serverTime = client->pers.cmd.serverTime;
	client->cmdStats[client->cmdIndex & CMD_MASK].thinkTime = trap_Milliseconds();

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	usercmd_t	*cmd = &client->pers.cmd;

	if ( (ent->r.svFlags & SVF_BOT) || g_synchronousClients.integer )
	{
		cmd->serverTime = level.time;

		ClientThink_real( ent );
	}
	else if (g_antiWarp.integer &&
		client->lastCmdTime > 0 &&
		client->lastCmdTime < level.time - g_antiWarpTime.integer &&
//		client->lastCmdTime > level.time - 1000 && //
		client->pers.connected == CON_CONNECTED &&
		client->sess.spectatorState == SPECTATOR_NOT &&
		client->ps.pm_type != PM_DEAD)
	{
		client->ps.eFlags |= EF_CONNECTION;

		if (g_antiWarpTime.integer == 2)
		{
			// create a fake user command to make him move, causing client
			// prediction error for a warping player
			cmd->serverTime = level.time + (cmd->serverTime - client->lastCmdTime);
			cmd->buttons = 0;
			cmd->generic_cmd = 0;	// let go any force power eg grip
			cmd->forwardmove = 0;
			cmd->rightmove = 0;
			cmd->upmove = 0;

			ClientThink_real( ent );
		}
	}
	else
	{
		client->ps.eFlags &= ~EF_CONNECTION;
	}
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	gclient_t	*cl;
	int			clientNum;

	if ( ent->r.svFlags & SVF_BOT ) {
		// always update ps.persistant before returning
		memcpy( client->ps.persistant, client->pers.persistant, sizeof( client->ps.persistant ) );
		return;
	}

	clientNum = client->sess.spectatorClient;

	// team follow1 and team follow2 go to the score leader and runner up
	if ( clientNum == FOLLOW_FIRST ) {
		clientNum = level.follow1;
	} else if ( clientNum == FOLLOW_SECOND ) {
		clientNum = level.follow2;
	}

	// player in a team may only follow his teammates
	if ( client->sess.sessionTeam != TEAM_SPECTATOR &&
		( g_restrictSpectator.integer || clientNum == FOLLOW_TEAM ) )
	{
		qboolean	found = qfalse;
		team_t		team = client->sess.sessionTeam;
		int			original;

		if ( clientNum < 0 || clientNum >= level.maxclients ) {
			clientNum = 0;
		}

		original = clientNum;

		do {
			cl = level.clients + clientNum;

			if (cl->pers.connected == CON_CONNECTED &&
				cl->sess.spectatorState == SPECTATOR_NOT &&
				cl->sess.sessionTeam == team )
			{
				found = qtrue;
				break;
			}

			clientNum++;
			if ( clientNum >= level.maxclients )
				clientNum = 0;

		} while (clientNum != original);

		if ( found ) {
			client->sess.spectatorState = SPECTATOR_FOLLOW;
			client->sess.spectatorClient = clientNum;
		} else if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			// allow spectating when last player disconnected or died
			StopFollowing( ent );
		}
	}

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		flags;

		if ( clientNum >= 0 && clientNum < level.maxclients ) {
			cl = &level.clients[ clientNum ];

			if ( cl->pers.connected == CON_CONNECTED && cl->sess.spectatorState == SPECTATOR_NOT ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				client->ps = cl->ps;
				client->ps.pm_flags |= PMF_FOLLOW;
				client->ps.eFlags = flags;
				client->ps.persistant[PERS_SPAWN_COUNT] = client->pers.persistant[PERS_SPAWN_COUNT];
			} else if ( client->sess.spectatorClient >= 0 ) {
				StopFollowing(ent);
			}
			// don't drop dedicated follow spectators if ranks weren't
			// updated soon enough
			// (eg in SetTeamSpec->ClientBegin->ClientSpawn->SpectatorClientEndFrame)
		} else {
			StopFollowing(ent);
		}
		return;
	}

	if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}

	memcpy( client->ps.persistant, client->pers.persistant, sizeof( client->ps.persistant ) );
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	gclient_t	*client = ent->client;
	int			i;

	if ( ent->client->sess.spectatorState != SPECTATOR_NOT ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ent->client->ps.powerups[ i ] < level.time ) {
			ent->client->ps.powerups[ i ] = 0;
		}
	}

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && (ent->client->ps.pm_type == PM_NORMAL || ent->client->ps.pm_type == PM_FLOAT) ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		memcpy( client->ps.persistant, client->pers.persistant, sizeof( client->ps.persistant ) );
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue, (qboolean)g_smoothClients.integer );

	SendPendingPredictableEvents( &ent->client->ps );

	memcpy( client->ps.persistant, client->pers.persistant, sizeof( client->ps.persistant ) );

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


