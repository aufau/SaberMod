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

//
// fx_*.c
//

// NOTENOTE This is not the best, DO NOT CHANGE THESE!
#define	FX_ALPHA_LINEAR		0x00000001
#define	FX_SIZE_LINEAR		0x00000100



// Bryar
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarHitWall( vec3_t origin, vec3_t normal );
void FX_BryarAltHitWall( vec3_t origin, vec3_t normal, int power );
void FX_BryarHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_BryarAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Blaster
void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BlasterWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_BlasterWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Disruptor
void FX_DisruptorMainShot( vec3_t start, vec3_t end );
void FX_DisruptorAltShot( vec3_t start, vec3_t end, qboolean fullCharge );
void FX_DisruptorAltMiss( vec3_t origin, vec3_t normal );
void FX_DisruptorAltHit( vec3_t origin, vec3_t normal );
void FX_DisruptorHitWall( vec3_t origin, vec3_t normal );
void FX_DisruptorHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Bowcaster
void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterHitWall( vec3_t origin, vec3_t normal );
void FX_BowcasterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Heavy Repeater
void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterAltHitWall( vec3_t origin, vec3_t normal );
void FX_RepeaterHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// DEMP2
void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_HitWall( vec3_t origin, vec3_t normal );
void FX_DEMP2_HitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_DEMP2_AltDetonate( const vec3_t org, float size );

// Golan Arms Flechette
void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteWeaponHitWall( vec3_t origin, vec3_t normal );
void FX_FlechetteWeaponHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Personal Rocket Launcher
void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketHitWall( vec3_t origin, vec3_t normal );
void FX_RocketHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );

// Turret
void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_TurretHitWall( vec3_t origin, vec3_t normal );
void FX_TurretHitPlayer( vec3_t origin, vec3_t normal, qboolean humanoid );
