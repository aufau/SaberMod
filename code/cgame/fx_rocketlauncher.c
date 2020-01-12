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

// Rocket Launcher Weapon

#include "cg_local.h"

/*
---------------------------
FX_RocketProjectileThink
---------------------------
*/

void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.rocketShotEffect, cent->lerpOrigin, forward );
}

/*
---------------------------
FX_RocketHitWall
---------------------------
*/

void FX_RocketHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.rocketExplosionEffect, origin, normal );
}

/*
---------------------------
FX_RocketHitPlayer
---------------------------
*/

void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
	trap_FX_PlayEffectID( cgs.effects.rocketExplosionEffect, origin, normal );
}

/*
---------------------------
FX_RocketAltProjectileThink
---------------------------
*/

void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.rocketShotEffect, cent->lerpOrigin, forward );
}
