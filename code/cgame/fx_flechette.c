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

// Golan Arms Flechette Weapon

#include "cg_local.h"

/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.flechetteShotEffect, cent->lerpOrigin, forward );
}

/*
-------------------------
FX_FlechetteWeaponHitWall
-------------------------
*/
void FX_FlechetteWeaponHitWall( vec3_t origin, vec3_t normal )
{
	trap_FX_PlayEffectID( cgs.effects.flechetteWallImpactEffect, origin, normal );
}

/*
-------------------------
FX_FlechetteWeaponHitPlayer
-------------------------
*/
void FX_FlechetteWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid )
{
//	if ( humanoid )
//	{
		trap_FX_PlayEffectID( cgs.effects.flechetteFleshImpactEffect, origin, normal );
//	}
//	else
//	{
//		trap_FX_PlayEffect( "blaster/droid_impact", origin, normal );
//	}
}


/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
	{
		forward[2] = 1.0f;
	}

	trap_FX_PlayEffectID( cgs.effects.flechetteAltShotEffect, cent->lerpOrigin, forward );
}
