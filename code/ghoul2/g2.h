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

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif
#if !defined(G2_H_INC)
#define G2_H_INC


#define BONE_ANGLES_PREMULT			0x0001
#define BONE_ANGLES_POSTMULT		0x0002
#define BONE_ANGLES_REPLACE			0x0004

#define BONE_ANGLES_TOTAL			( BONE_ANGLES_PREMULT | BONE_ANGLES_POSTMULT | BONE_ANGLES_REPLACE )
#define BONE_ANIM_OVERRIDE			0x0008
#define BONE_ANIM_OVERRIDE_LOOP		0x0010
#define BONE_ANIM_OVERRIDE_FREEZE	( 0x0040 + BONE_ANIM_OVERRIDE )
#define BONE_ANIM_BLEND				0x0080
#define BONE_ANIM_TOTAL				( BONE_ANIM_OVERRIDE | BONE_ANIM_OVERRIDE_LOOP | BONE_ANIM_OVERRIDE_FREEZE | BONE_ANIM_BLEND)


// defines to setup the
#define		ENTITY_WIDTH 12
#define		MODEL_WIDTH	10
#define		BOLT_WIDTH	10

#define		MODEL_AND	((1<<MODEL_WIDTH)-1)
#define		BOLT_AND	((1<<BOLT_WIDTH)-1)
#define		ENTITY_AND	((1<<ENTITY_WIDTH)-1)

#define		BOLT_SHIFT	0
#define		MODEL_SHIFT	(BOLT_SHIFT + BOLT_WIDTH)
#define		ENTITY_SHIFT (MODEL_SHIFT + MODEL_WIDTH)


#endif // G2_H_INC

