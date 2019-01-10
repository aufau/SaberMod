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

// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
void CG_CheckAmmo( void ) {
#if 0
	int		i;
	int		total;
	int		previous;
	int		weapons;

	// see about how many seconds of ammo we have remaining
	weapons = cg.snap->ps.stats[ STAT_WEAPONS ];
	total = 0;
	for ( i = WP_BRYAR_PISTOL; i < WP_NUM_WEAPONS ; i++ ) {
		if ( ! ( weapons & ( 1 << i ) ) ) {
			continue;
		}
		switch ( i )
		{
		case WP_BRYAR_PISTOL:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
		case WP_ROCKET_LAUNCHER:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
		case WP_EMPLACED_GUN:
			total += cg.snap->ps.ammo[weaponData[i].ammoIndex] * 1000;
			break;
		default:
			total += cg.snap->ps.ammo[weaponData[i].ammoIndex] * 200;
			break;
		}
		if ( total >= 5000 ) {
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	previous = cg.lowAmmoWarning;

	if ( total == 0 ) {
		cg.lowAmmoWarning = 2;
	} else {
		cg.lowAmmoWarning = 1;
	}

	if (cg.snap->ps.weapon == WP_SABER)
	{
		cg.lowAmmoWarning = 0;
	}

	// play a sound on transitions
	if ( cg.lowAmmoWarning != previous ) {
		trap_S_StartLocalSound( cgs.media.noAmmoSound, CHAN_LOCAL_SOUND );
	}
#endif
	//disabled silly ammo warning stuff for now
}

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float		left, front, up;
	float		kick;
	int			health;
	float		scale;
	vec3_t		dir;
	vec3_t		angles;
	float		dist;
	float		yaw, pitch;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 ) {
		scale = 1;
	} else {
		scale = 40.0f / health;
	}
	kick = damage * scale;

	if (kick < 5)
		kick = 5;
	if (kick > 10)
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte * (360.0f / 255);
		yaw = yawByte * (360.0f / 255);

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );
		VectorSubtract( vec3_origin, dir, dir );

		front = DotProduct (dir, cg.refdef.viewaxis[0] );
		left = DotProduct (dir, cg.refdef.viewaxis[1] );
		up = DotProduct (dir, cg.refdef.viewaxis[2] );

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength( dir );
		if ( dist < 0.1f ) {
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;

		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1f ) {
			front = 0.1f;
		}
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	cg.damageX = Com_Clamp( -1.0f, 1.0f, cg.damageX );
	cg.damageY = Com_Clamp( -1.0f, 1.0f, cg.damageY );

	// don't let the screen flashes vary as much
	if ( kick > 10 ) {
		kick = 10;
	}
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.time;
}




/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( const playerState_t *ps ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
	cg.weaponSelectTime = cg.time;

	// select the weapon the server says we are using
	cg.weaponSelect = ps->weapon;
}

/*
==============
CG_CheckPlayerstateEvents
==============
*/
static void CG_CheckPlayerstateEvents( const playerState_t *ps, const playerState_t *ops ) {
	int			i;
	int			event;
	centity_t	*cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, cent->lerpOrigin );
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
			|| (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS-1)] != ops->events[i & (MAX_PS_EVENTS-1)]) ) {

			event = ps->events[ i & (MAX_PS_EVENTS-1) ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];
			CG_EntityEvent( cent, cent->lerpOrigin );

			cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS-1) ] = event;

			cg.eventSequence++;
		}
	}
}

#ifdef UNUSED
/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents( playerState_t *ps ) {
	int i;
	int event;
	centity_t	*cent;

	cent = &cg.predictedPlayerEntity;
	for ( i = ps->eventSequence - MAX_PS_EVENTS ; i < ps->eventSequence ; i++ ) {
		//
		if (i >= cg.eventSequence) {
			continue;
		}
		// if this event is not further back in than the maximum predictable events we remember
		if (i > cg.eventSequence - MAX_PREDICTED_EVENTS) {
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[i & (MAX_PS_EVENTS-1)] != cg.predictableEvents[i & (MAX_PREDICTED_EVENTS-1) ] ) {

				event = ps->events[ i & (MAX_PS_EVENTS-1) ];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[ i & (MAX_PS_EVENTS-1) ];
				CG_EntityEvent( cent, cent->lerpOrigin );

				cg.predictableEvents[ i & (MAX_PREDICTED_EVENTS-1) ] = event;

				if ( cg_showmiss.integer ) {
					CG_Printf("WARNING: changed predicted event\n");
				}
			}
		}
	}
}
#endif // UNUSED

/*
==================
pushReward
==================
*/
#ifdef JK2AWARDS
static qboolean pushReward(sfxHandle_t sfx, qhandle_t shader, int rewardCount) {
	if ((sfx || shader) && cg.rewardStack < (MAX_REWARDSTACK-1)) {
		cg.rewardStack++;
		cg.rewardSound[cg.rewardStack] = sfx;
		cg.rewardShader[cg.rewardStack] = shader;
		cg.rewardCount[cg.rewardStack] = rewardCount;
		return qtrue;
	}
	return qfalse;
}
#endif

int cgAnnouncerTime = 0; //to prevent announce sounds from playing on top of each other

/*
==================
CG_CheckLocalSounds
==================
*/
static void CG_CheckLocalSounds( const playerState_t *ps, const playerState_t *ops ) {
	int			highScore, reward;
#ifdef JK2AWARDS
	sfxHandle_t sfx;
#endif

	// don't play the sounds if the player just changed teams
	if ( ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM] ) {
		return;
	}

	// health changes of more than -3 should make pain sounds
	if (cg_oldPainSounds.integer)
	{
		if ( ps->stats[STAT_HEALTH] < (ops->stats[STAT_HEALTH] - 3))
		{
			if ( ps->stats[STAT_HEALTH] > 0 )
			{
				CG_PainEvent( &cg.predictedPlayerEntity, ps->stats[STAT_HEALTH] );
			}
		}
	}

	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

#ifdef JK2AWARDS
	// reward sounds
	reward = qfalse;
	if (ps->persistant[PERS_CAPTURES] != ops->persistant[PERS_CAPTURES]) {
		sfx = cgs.media.captureSound;
		if (pushReward(sfx, cgs.media.medalCapture, ps->persistant[PERS_CAPTURES]))
			reward = qtrue;
		//Com_Printf("capture\n");
	}
	if (ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT]) {
		sfx = cgs.media.impressiveSound;
		if (pushReward(sfx, cgs.media.medalImpressive, ps->persistant[PERS_IMPRESSIVE_COUNT]))
			reward = qtrue;
		//Com_Printf("impressive\n");
	}
	if (ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT]) {
		sfx = cgs.media.excellentSound;
		if (pushReward(sfx, cgs.media.medalExcellent, ps->persistant[PERS_EXCELLENT_COUNT]))
			reward = qtrue;
		//Com_Printf("excellent\n");
	}
	if (ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT]) {
		sfx = cgs.media.humiliationSound;
		if (pushReward(sfx, cgs.media.medalGauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT]))
			reward = qtrue;
		//Com_Printf("guantlet frag\n");
	}
	if (ps->persistant[PERS_DEFEND_COUNT] != ops->persistant[PERS_DEFEND_COUNT]) {
		sfx = cgs.media.defendSound;
		if (pushReward(sfx, cgs.media.medalDefend, ps->persistant[PERS_DEFEND_COUNT]))
			reward = qtrue;
		//Com_Printf("defend\n");
	}
	if (ps->persistant[PERS_ASSIST_COUNT] != ops->persistant[PERS_ASSIST_COUNT]) {
		sfx = cgs.media.assistSound;
		if (pushReward(sfx, cgs.media.medalAssist, ps->persistant[PERS_ASSIST_COUNT]))
			reward = qtrue;
		//Com_Printf("assist\n");
	}
	// if any of the player event bits changed
	if (ps->persistant[PERS_PLAYEREVENTS] != ops->persistant[PERS_PLAYEREVENTS]) {
		if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) !=
			(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD)) {
			if ( cgs.media.deniedSound ) {
				trap_S_StartLocalSound( cgs.media.deniedSound, CHAN_ANNOUNCER );
				reward = qtrue;
			}
		}
		/*
		else if ((ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) !=
				(ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD)) {
			if ( cgs.media.humiliationSound ) {
				trap_S_StartLocalSound( cgs.media.humiliationSound, CHAN_ANNOUNCER );
				reward = qtrue;
			}
		}
		*/
	}
#else
	reward = qfalse;
#endif

	if ( cg.warmup ) {
		return;
	}

	// lead changes
	if ( !reward && cgAnnouncerTime < cg.time ) {
		// never play lead changes during warmup
		if ( ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK]
			 && (!GT_Team(cgs.gametype) || cgs.gametype == GT_REDROVER) ) {
			if (  ps->persistant[PERS_RANK] == 0 ) {
				CG_AddBufferedSound(cgs.media.takenLeadSound);
				cgAnnouncerTime = cg.time + 3000;
			} else if ( ps->persistant[PERS_RANK] == RANK_TIED_FLAG ) {
				//CG_AddBufferedSound(cgs.media.tiedLeadSound);
			} else if ( ( ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG ) == 0 ) {
				//rww - only bother saying this if you have more than 1 kill already.
				//joining the server and hearing "the force is not with you" is silly.
				if (ps->persistant[PERS_SCORE] > 0)
				{
					CG_AddBufferedSound(cgs.media.lostLeadSound);
					cgAnnouncerTime = cg.time + 3000;
				}
			}
		}
	}

	// timelimit warnings
	if ( cgs.timelimit > 0 && cgAnnouncerTime < cg.time ) {
		int		msec;

		msec = cg.gameTime - cgs.levelStartTime;
		if ( !( cg.timelimitWarnings & 4 ) && msec > ( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 1 | 2 | 4;
			//trap_S_StartLocalSound( cgs.media.suddenDeathSound, CHAN_ANNOUNCER );
		}
		else if ( !( cg.timelimitWarnings & 2 ) && msec > (cgs.timelimit - 1) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1 | 2;
			trap_S_StartLocalSound( cgs.media.oneMinuteSound, CHAN_ANNOUNCER );
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.timelimit > 5 && !( cg.timelimitWarnings & 1 ) && msec > (cgs.timelimit - 5) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			trap_S_StartLocalSound( cgs.media.fiveMinuteSound, CHAN_ANNOUNCER );
			cgAnnouncerTime = cg.time + 3000;
		}
	}

	// fraglimit warnings
	if ( cgs.fraglimit > 0 && cgAnnouncerTime < cg.time && !GT_Flag(cgs.gametype)
		&& !GT_Round(cgs.gametype) && cgs.gametype != GT_TOURNAMENT ) {
		highScore = cgs.scores1;
		if ( !( cg.fraglimitWarnings & 4 ) && highScore == (cgs.fraglimit - 1) ) {
			cg.fraglimitWarnings |= 1 | 2 | 4;
			CG_AddBufferedSound(cgs.media.oneFragSound);
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.fraglimit > 2 && !( cg.fraglimitWarnings & 2 ) && highScore == (cgs.fraglimit - 2) ) {
			cg.fraglimitWarnings |= 1 | 2;
			CG_AddBufferedSound(cgs.media.twoFragSound);
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.fraglimit > 3 && !( cg.fraglimitWarnings & 1 ) && highScore == (cgs.fraglimit - 3) ) {
			cg.fraglimitWarnings |= 1;
			CG_AddBufferedSound(cgs.media.threeFragSound);
			cgAnnouncerTime = cg.time + 3000;
		}
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState( const playerState_t *ps, playerState_t *ops ) {
	// respawning
	if ( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT] ) {
		CG_Respawn( ps );
	}

	if ( cg.mapRestart ) {
		CG_Respawn( ps );
		cg.mapRestart = qfalse;
	}

	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	if ( ps->pm_type != PM_INTERMISSION &&
		ps->pm_type != PM_SPECTATOR ) {
		CG_CheckLocalSounds( ps, ops );
	}

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents( ps, ops );

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight ) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}

