/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

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

#define	FORCE_POWER_MAX	100
#define MAX_GRIP_DISTANCE 256
#define MAX_TRICK_DISTANCE 512
#define FORCE_JUMP_CHARGE_TIME 6400//3000.0f
#define GRIP_DRAIN_AMOUNT 30
#define FORCE_LIGHTNING_RADIUS 300
#define MAX_DRAIN_DISTANCE 512

enum
{
	FJ_FORWARD,
	FJ_BACKWARD,
	FJ_RIGHT,
	FJ_LEFT,
	FJ_UP
};

extern vmCvar_t g_MaxHolocronCarry;
extern const int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];
extern const float forceJumpHeight[NUM_FORCE_POWER_LEVELS];
extern const float forceJumpStrength[NUM_FORCE_POWER_LEVELS];

