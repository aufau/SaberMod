/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2015-2021 Witold Pilat <witold.pilat@gmail.com>

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

// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"

#ifdef UNUSED
/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing ) {
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// advance a random amount first
	i = id_rand() % (int)spacing;
	VectorMA( move, i, vec, move );

	VectorScale (vec, spacing, vec);

	for ( ; i < len; i += spacing ) {
		localEntity_t	*le;
		refEntity_t		*re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000 + (int) (random() * 250);
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy( move, le->pos.trBase );
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		VectorAdd (move, vec, move);
	}
}
#endif // UNUSED

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *CG_SmokePuff( const vec3_t p, const vec3_t vel,
				   float radius,
				   float r, float g, float b, float a,
				   int duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static unsigned	seed = 0x92u;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime * 0.001f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0f / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0f / ( le->endTime - le->startTime );
	}
	le->color[0] = r;
	le->color[1] = g;
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

void CG_TestLine( const vec3_t start, const vec3_t end, int time, unsigned int color, int radius) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leType = LE_LINE;
	le->startTime = cg.time;
	le->endTime = cg.time + time;
	le->lifeRate = 1.0f / ( le->endTime - le->startTime );

	re = &le->refEntity;
	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin);
	re->shaderTime = cg.time * 0.001f;

	re->reType = RT_LINE;
	re->radius = 0.5f*radius;
	re->customShader = cgs.media.whiteShader; //trap_R_RegisterShaderNoMip("textures/colombia/canvas_doublesided");

	re->shaderTexCoord[0] = re->shaderTexCoord[1] = 1.0f;

	if (color==0)
	{
		re->shaderRGBA[0] = re->shaderRGBA[1] = re->shaderRGBA[2] = re->shaderRGBA[3] = 0xff;
	}
	else
	{
		re->shaderRGBA[0] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[1] = color & 0xff;
		color >>= 8;
		re->shaderRGBA[2] = color & 0xff;
//		color >>= 8;
//		re->shaderRGBA[3] = color & 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	le->color[3] = 1.0;
}

/*
==================
CG_ThrowChunk
==================
*/
static void CG_ThrowChunk( const vec3_t origin, const vec3_t velocity, qhandle_t hModel, int optionalSound, int startalpha ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + (int)(random() * 3000);

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	le->angles.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	VectorSet(le->angles.trBase, 20, 20, 20);
	VectorCopy( velocity, le->angles.trDelta );
	le->pos.trTime = cg.time;
	le->angles.trTime = cg.time;

	le->leFlags = LEF_TUMBLE;

	le->angles.trBase[YAW] = 180;

	le->bounceFactor = 0.3f;
	le->bounceSound = optionalSound;

	le->forceAlpha = startalpha;
}

//----------------------------
//
// Breaking Glass Technology
//
//----------------------------

// Since we have shared verts when we tesselate the glass sheet, it helps to have a
//	random offset table set up up front.

#define GLASS_GRID	20

#define	FX_ALPHA_NONLINEAR	0x00000004
#define FX_APPLY_PHYSICS	0x02000000
#define FX_USE_ALPHA		0x08000000

static void CG_DoGlassQuad( vec3_t p[4], vec2_t uv[4], qboolean stick, int time, const vec3_t dmgDir )
{
	float	bounce;
	vec3_t	rotDelta;
	vec3_t	vel, accel;
	vec3_t	rgb1;
	addpolyArgStruct_t apArgs;
	int		i, i_2;

	VectorSet( vel, crandom() * 12, crandom() * 12, -1 );

	if ( !stick )
	{
		// We aren't a motion delayed chunk, so let us move quickly
		VectorMA( vel, 0.3f, dmgDir, vel );
	}

	// Set up acceleration due to gravity, 800 is standard QuakeIII gravity, so let's use something close
	VectorSet( accel, 0.0f, 0.0f, -(600.0f + random() * 100.0f ) );

	// We are using an additive shader, so let's set the RGB low so we look more like transparent glass
//	VectorSet( rgb1, 0.1f, 0.1f, 0.1f );
	VectorSet( rgb1, 1.0f, 1.0f, 1.0f );

	// Being glass, we don't want to bounce much
	bounce = random() * 0.2f + 0.15f;

	// Set up our random rotate, we only do PITCH and YAW, not ROLL.  This is something like degrees per second
	VectorSet( rotDelta, crandom() * 40.0f, crandom() * 40.0f, 0.0f );

	//In an ideal world, this might actually work.
	/*
	CPoly *pol = FX_AddPoly(p, uv, 4,			// verts, ST, vertCount
			vel, accel,				// motion
			0.15f, 0.0f, 85.0f,		// alpha start, alpha end, alpha parm ( begin alpha fade when 85% of life is complete )
			rgb1, rgb1, 0.0f,		// rgb start, rgb end, rgb parm ( not used )
			rotDelta, bounce, time,	// rotation amount, bounce, and time to delay motion for ( zero if no delay );
			6000,					// life
			cgi_R_RegisterShader( "gfx/misc/test_crackle" ),
			FX_APPLY_PHYSICS | FX_ALPHA_NONLINEAR | FX_USE_ALPHA );

	if ( random() > 0.95f && pol )
	{
		pol->AddFlags( FX_IMPACT_RUNS_FX | FX_KILL_ON_IMPACT );
		pol->SetImpactFxID( theFxScheduler.RegisterEffect( "glass_impact" ));
	}
	*/

	//rww - this is dirty.

	for (i = 0; i < 4; i++) {
		for (i_2 = 0; i_2 < 3; i_2++) {
			apArgs.p[i][i_2] = p[i][i_2];
		}
	}

	for (i = 0; i < 4; i++) {
		for (i_2 = 0; i_2 < 2; i_2++) {
			apArgs.ev[i][i_2] = uv[i][i_2];
		}
	}

	apArgs.numVerts = 4;
	VectorCopy(vel, apArgs.vel);
	VectorCopy(accel, apArgs.accel);

	apArgs.alpha1 = 0.15f;
	apArgs.alpha2 = 0.0f;
	apArgs.alphaParm = 85.0f;

	VectorCopy(rgb1, apArgs.rgb1);
	VectorCopy(rgb1, apArgs.rgb2);

	apArgs.rgbParm = 0.0f;

	VectorCopy(rotDelta, apArgs.rotationDelta);

	apArgs.bounce = bounce;
	apArgs.motionDelay = time;
	apArgs.killTime = 6000;
	apArgs.shader = cgs.media.glassShardShader;
	apArgs.flags = (FX_APPLY_PHYSICS | FX_ALPHA_NONLINEAR | FX_USE_ALPHA);

	trap_FX_AddPoly(&apArgs);
}

static void CG_CalcBiLerp( vec3_t verts[4], vec3_t subVerts[4], vec2_t uv[4] )
{
	vec3_t	temp;

	// Nasty crap
	VectorScale( verts[0],	1.0f - uv[0][0],	subVerts[0] );
	VectorMA( subVerts[0],	uv[0][0],			verts[1], subVerts[0] );
	VectorScale( subVerts[0], 1.0f - uv[0][1],	temp );
	VectorScale( verts[3],	1.0f - uv[0][0],	subVerts[0] );
	VectorMA( subVerts[0],	uv[0][0],			verts[2], subVerts[0] );
	VectorMA( temp,			uv[0][1],			subVerts[0], subVerts[0] );

	VectorScale( verts[0],	1.0f - uv[1][0],	subVerts[1] );
	VectorMA( subVerts[1],	uv[1][0],			verts[1], subVerts[1] );
	VectorScale( subVerts[1], 1.0f - uv[1][1],	temp );
	VectorScale( verts[3],	1.0f - uv[1][0],	subVerts[1] );
	VectorMA( subVerts[1],	uv[1][0],			verts[2], subVerts[1] );
	VectorMA( temp,			uv[1][1],			subVerts[1], subVerts[1] );

	VectorScale( verts[0],	1.0f - uv[2][0],	subVerts[2] );
	VectorMA( subVerts[2],	uv[2][0],			verts[1], subVerts[2] );
	VectorScale( subVerts[2], 1.0f - uv[2][1],		temp );
	VectorScale( verts[3],	1.0f - uv[2][0],	subVerts[2] );
	VectorMA( subVerts[2],	uv[2][0],			verts[2], subVerts[2] );
	VectorMA( temp,			uv[2][1],			subVerts[2], subVerts[2] );

	VectorScale( verts[0],	1.0f - uv[3][0],	subVerts[3] );
	VectorMA( subVerts[3],	uv[3][0],			verts[1], subVerts[3] );
	VectorScale( subVerts[3], 1.0f - uv[3][1],	temp );
	VectorScale( verts[3],	1.0f - uv[3][0],	subVerts[3] );
	VectorMA( subVerts[3],	uv[3][0],			verts[2], subVerts[3] );
	VectorMA( temp,			uv[3][1],			subVerts[3], subVerts[3] );
}
// bilinear
//f(p',q') = (1 - y) × {[(1 - x) × f(p,q)] + [x × f(p,q+1)]} + y × {[(1 - x) × f(p+1,q)] + [x × f(p+1,q+1)]}.


static void CG_CalcHeightWidth( vec3_t verts[4], float *height, float *width )
{
	vec3_t	dir1, dir2, cross;

	VectorSubtract( verts[3], verts[0], dir1 ); // v
	VectorSubtract( verts[1], verts[0], dir2 ); // p-a
	CrossProduct( dir1, dir2, cross );
	*width = VectorNormalize( cross ) / VectorNormalize( dir1 ); // v
	VectorSubtract( verts[2], verts[0], dir2 ); // p-a
	CrossProduct( dir1, dir2, cross );
	*width += VectorNormalize( cross ) / VectorNormalize( dir1 ); // v
	*width *= 0.5f;

	VectorSubtract( verts[1], verts[0], dir1 ); // v
	VectorSubtract( verts[2], verts[0], dir2 ); // p-a
	CrossProduct( dir1, dir2, cross );
	*height = VectorNormalize( cross ) / VectorNormalize( dir1 ); // v
	VectorSubtract( verts[3], verts[0], dir2 ); // p-a
	CrossProduct( dir1, dir2, cross );
	*height += VectorNormalize( cross ) / VectorNormalize( dir1 ); // v
	*height *= 0.5f;
}
//Consider a line in 3D with position vector "a" and direction vector "v" and
// let "p" be the position vector of an arbitrary point in 3D
//dist = len( crossprod(p-a,v) ) / len(v);

#define TIME_DECAY_SLOW		0.1f
#define TIME_DECAY_MED		0.04f
#define TIME_DECAY_FAST		0.009f

void CG_DoGlass( vec3_t verts[4], vec3_t normal, vec3_t dmgPt, vec3_t dmgDir, float dmgRadius, int maxShards )
{
	int			i, t;
	int			mxHeight, mxWidth;
	float		height, width;
	float		stepWidth, stepHeight;
	float		timeDecay;
	float		dif;
	int			time = 0;
	int			glassShards = 0;
	qboolean	stick = qtrue;
	vec3_t		subVerts[4];
	vec2_t		biPoints[4];
	float		biGridX[GLASS_GRID + 1][GLASS_GRID + 1];
	float		biGridZ[GLASS_GRID + 1][GLASS_GRID + 1];

	// To do a smarter tesselation, we should figure out the relative height and width of the brush face,
	//	then use this to pick a lod value from 1-3 in each axis.  This will give us 1-9 lod levels, which will
	//	hopefully be sufficient.
	CG_CalcHeightWidth( verts, &height, &width );

	trap_S_StartSound( dmgPt, -1, CHAN_AUTO, trap_S_RegisterSound("sound/effects/glassbreak1.wav"));

	// Pick "LOD" for height
	if ( height < 100 )
	{
		mxHeight = 5;
		timeDecay = TIME_DECAY_SLOW;
	}
	else if ( height > 220 )
	{
		mxHeight = GLASS_GRID;
		timeDecay = TIME_DECAY_FAST;
	}
	else
	{
		mxHeight = 10;
		timeDecay = TIME_DECAY_MED;
	}

	// Pick "LOD" for width
	if ( width < 100 )
	{
		mxWidth = 5;
		timeDecay = ( timeDecay + TIME_DECAY_SLOW ) * 0.5f;
	}
	else if ( width > 220 )
	{
		mxWidth = GLASS_GRID;
		timeDecay = ( timeDecay + TIME_DECAY_FAST ) * 0.5f;
	}
	else
	{
		mxWidth = 10;
		timeDecay = ( timeDecay + TIME_DECAY_MED ) * 0.5f;
	}

	stepWidth = 1.0f / mxWidth;
	stepHeight = 1.0f / mxHeight;

	// Glass surface is cut into shards with a mxWidth x mxHeight
	// grid. Each piece is animated by the FX system independently

	// offset grid crossing points with random values to create
	// various shapes of glass shards
	for ( i = 0; i <= mxHeight; i++ ) {
		float z = (float)i / mxHeight;

		for ( t = 0; t <= mxWidth; t++ ) {
			float x = (float)t / mxWidth;

			// Offset break points inside, not on the boundary
			if (i > 0 && t > 0 && i < mxHeight && t < mxWidth) {
				biGridX[i][t] = x + 0.49f * stepWidth * crandom();
				biGridZ[i][t] = z + 0.49f * stepHeight * crandom();
			} else {
				biGridX[i][t] = x;
				biGridZ[i][t] = z;
			}
		}
	}

	// create glass shards based on crossing points
	for ( i = 0; i < mxHeight; i++ )
	{
		for ( t = 0; t < mxWidth; t++ )
		{
			Vector2Set( biPoints[0], biGridX[i][t], biGridZ[i][t] );
			Vector2Set( biPoints[1], biGridX[i][t + 1], biGridZ[i][t + 1] );
			Vector2Set( biPoints[2], biGridX[i + 1][t + 1], biGridZ[i + 1][t + 1] );
			Vector2Set( biPoints[3], biGridX[i + 1][t], biGridZ[i + 1][t] );

			CG_CalcBiLerp( verts, subVerts, biPoints );

			dif = DistanceSquared( subVerts[0], dmgPt ) * timeDecay - random() * 32;

			// If we decrease dif, we are increasing the impact area, making it more likely to blow out large holes
			dif -= dmgRadius * dmgRadius;

			if ( dif > 1 )
			{
				stick = qtrue;
				time = dif + random() * 200;
			}
			else
			{
				stick = qfalse;
				time = 0;
			}

			CG_DoGlassQuad( subVerts, biPoints, stick, time, dmgDir );
			glassShards++;

			if (maxShards && glassShards >= maxShards)
			{
				return;
			}
		}
	}
}

/*
==================
CG_GlassShatter
Break glass with fancy method
==================
*/
void CG_GlassShatter(int entnum, vec3_t dmgPt, vec3_t dmgDir, float dmgRadius, int maxShards)
{
	vec3_t		verts[4], normal;

	if (cgs.inlineDrawModel[cg_entities[entnum].currentState.modelindex])
	{
		// FIXME: trap_R_GetBModelVerts() should return corner
		// coordinates of the glass surface, but brush models are
		// partitioned by the bsp algorithm and it may return a
		// surface partition instead of the whole thing (which may be
		// have only 3 verts, like on duel_training)
		trap_R_GetBModelVerts(cgs.inlineDrawModel[cg_entities[entnum].currentState.modelindex], verts, normal);

		/*
		CG_TestLine( verts[0], verts[1], 1000000, -1, 1);
		CG_TestLine( verts[1], verts[2], 1000000, -1, 1);
		CG_TestLine( verts[2], verts[3], 1000000, -1, 1);
		CG_TestLine( verts[3], verts[0], 1000000, -1, 1);
		*/

		CG_DoGlass(verts, normal, dmgPt, dmgDir, dmgRadius, maxShards);
	}
	//otherwise something awful has happened.
}
#ifdef UNUSED
/*
==================
CG_GlassShatter_Old
Throws glass shards from within a given bounding box in the world
==================
*/
void CG_GlassShatter_Old(int entnum, vec3_t org, vec3_t mins, vec3_t maxs)
{
	vec3_t velocity, a, shardorg, dif, difx;
	float windowmass;
	float shardsthrow = 0;
	char chunkname[256];

	trap_S_StartSound(org, entnum, CHAN_BODY, trap_S_RegisterSound("sound/effects/glassbreak1.wav"));

	VectorSubtract(maxs, mins, a);

	windowmass = VectorLength(a); //should give us some idea of how big the chunk of glass is

	while (shardsthrow < windowmass)
	{
		velocity[0] = crandom()*150;
		velocity[1] = crandom()*150;
		velocity[2] = 150 + crandom()*75;

		Com_sprintf(chunkname, sizeof(chunkname), "models/chunks/glass/glchunks_%i.md3", Q_irand(1, 6));
		VectorCopy(org, shardorg);

		dif[0] = (maxs[0]-mins[0])/2;
		dif[1] = (maxs[1]-mins[1])/2;
		dif[2] = (maxs[2]-mins[2])/2;

		if (dif[0] < 2)
		{
			dif[0] = 2;
		}
		if (dif[1] < 2)
		{
			dif[1] = 2;
		}
		if (dif[2] < 2)
		{
			dif[2] = 2;
		}

		difx[0] = Q_irand(1, 2 * 0.9f * dif[0]);
		difx[1] = Q_irand(1, 2 * 0.9f * dif[1]);
		difx[2] = Q_irand(1, 2 * 0.9f * dif[2]);

		if (difx[0] > dif[0])
		{
			shardorg[0] += difx[0]-(dif[0]);
		}
		else
		{
			shardorg[0] -= difx[0];
		}
		if (difx[1] > dif[1])
		{
			shardorg[1] += difx[1]-(dif[1]);
		}
		else
		{
			shardorg[1] -= difx[1];
		}
		if (difx[2] > dif[2])
		{
			shardorg[2] += difx[2]-(dif[2]);
		}
		else
		{
			shardorg[2] -= difx[2];
		}

		//CG_TestLine(org, shardorg, 5000, 0x0000ff, 3);

		CG_ThrowChunk( shardorg, velocity, trap_R_RegisterModel( chunkname ), 0, 254 );

		shardsthrow += 10;
	}
}
#endif // UNUSED
/*
==================
CG_CreateDebris
Throws specified debris from within a given bounding box in the world
==================
*/
#define NUM_DEBRIS_MODELS_GLASS				8
#define NUM_DEBRIS_MODELS_WOOD				8
#define NUM_DEBRIS_MODELS_CHUNKS			3
#define NUM_DEBRIS_MODELS_ROCKS				4 //12

int dbModels_Glass[NUM_DEBRIS_MODELS_GLASS];
int dbModels_Wood[NUM_DEBRIS_MODELS_WOOD];
int dbModels_Chunks[NUM_DEBRIS_MODELS_CHUNKS];
int dbModels_Rocks[NUM_DEBRIS_MODELS_ROCKS];

void CG_CreateDebris(int entnum, const vec3_t org, const vec3_t mins, const vec3_t maxs, int debrissound, int debrismodel)
{
	vec3_t velocity, a, shardorg, dif, difx;
	float windowmass;
	float shardsthrow = 0;
	int omodel = debrismodel;

	if (omodel == DEBRIS_SPECIALCASE_GLASS && !dbModels_Glass[0])
	{ //glass no longer exists, using it for metal.
		dbModels_Glass[0] = trap_R_RegisterModel("models/chunks/metal/metal1_1.md3");
		dbModels_Glass[1] = trap_R_RegisterModel("models/chunks/metal/metal1_2.md3");
		dbModels_Glass[2] = trap_R_RegisterModel("models/chunks/metal/metal1_3.md3");
		dbModels_Glass[3] = trap_R_RegisterModel("models/chunks/metal/metal1_4.md3");
		dbModels_Glass[4] = trap_R_RegisterModel("models/chunks/metal/metal2_1.md3");
		dbModels_Glass[5] = trap_R_RegisterModel("models/chunks/metal/metal2_2.md3");
		dbModels_Glass[6] = trap_R_RegisterModel("models/chunks/metal/metal2_3.md3");
		dbModels_Glass[7] = trap_R_RegisterModel("models/chunks/metal/metal2_4.md3");
	}
	if (omodel == DEBRIS_SPECIALCASE_WOOD && !dbModels_Wood[0])
	{
		dbModels_Wood[0] = trap_R_RegisterModel("models/chunks/crate/crate1_1.md3");
		dbModels_Wood[1] = trap_R_RegisterModel("models/chunks/crate/crate1_2.md3");
		dbModels_Wood[2] = trap_R_RegisterModel("models/chunks/crate/crate1_3.md3");
		dbModels_Wood[3] = trap_R_RegisterModel("models/chunks/crate/crate1_4.md3");
		dbModels_Wood[4] = trap_R_RegisterModel("models/chunks/crate/crate2_1.md3");
		dbModels_Wood[5] = trap_R_RegisterModel("models/chunks/crate/crate2_2.md3");
		dbModels_Wood[6] = trap_R_RegisterModel("models/chunks/crate/crate2_3.md3");
		dbModels_Wood[7] = trap_R_RegisterModel("models/chunks/crate/crate2_4.md3");
	}
	if (omodel == DEBRIS_SPECIALCASE_CHUNKS && !dbModels_Chunks[0])
	{
		dbModels_Chunks[0] = trap_R_RegisterModel("models/chunks/generic/chunks_1.md3");
		dbModels_Chunks[1] = trap_R_RegisterModel("models/chunks/generic/chunks_2.md3");
	}
	if (omodel == DEBRIS_SPECIALCASE_ROCK && !dbModels_Rocks[0])
	{
		dbModels_Rocks[0] = trap_R_RegisterModel("models/chunks/rock/rock1_1.md3");
		dbModels_Rocks[1] = trap_R_RegisterModel("models/chunks/rock/rock1_2.md3");
		dbModels_Rocks[2] = trap_R_RegisterModel("models/chunks/rock/rock1_3.md3");
		dbModels_Rocks[3] = trap_R_RegisterModel("models/chunks/rock/rock1_4.md3");
		/*
		dbModels_Rocks[4] = trap_R_RegisterModel("models/chunks/rock/rock2_1.md3");
		dbModels_Rocks[5] = trap_R_RegisterModel("models/chunks/rock/rock2_2.md3");
		dbModels_Rocks[6] = trap_R_RegisterModel("models/chunks/rock/rock2_3.md3");
		dbModels_Rocks[7] = trap_R_RegisterModel("models/chunks/rock/rock2_4.md3");
		dbModels_Rocks[8] = trap_R_RegisterModel("models/chunks/rock/rock3_1.md3");
		dbModels_Rocks[9] = trap_R_RegisterModel("models/chunks/rock/rock3_2.md3");
		dbModels_Rocks[10] = trap_R_RegisterModel("models/chunks/rock/rock3_3.md3");
		dbModels_Rocks[11] = trap_R_RegisterModel("models/chunks/rock/rock3_4.md3");
		*/
	}

	VectorSubtract(maxs, mins, a);

	windowmass = VectorLength(a); //should give us some idea of how big the chunk of glass is

	while (shardsthrow < windowmass)
	{
		velocity[0] = crandom()*150;
		velocity[1] = crandom()*150;
		velocity[2] = 150 + crandom()*75;

		if (omodel == DEBRIS_SPECIALCASE_GLASS)
		{
			debrismodel = dbModels_Glass[Q_irand(0, NUM_DEBRIS_MODELS_GLASS-1)];
		}
		else if (omodel == DEBRIS_SPECIALCASE_WOOD)
		{
			debrismodel = dbModels_Wood[Q_irand(0, NUM_DEBRIS_MODELS_WOOD-1)];
		}
		else if (omodel == DEBRIS_SPECIALCASE_CHUNKS)
		{
			debrismodel = dbModels_Chunks[Q_irand(0, NUM_DEBRIS_MODELS_CHUNKS-1)];
		}
		else if (omodel == DEBRIS_SPECIALCASE_ROCK)
		{
			debrismodel = dbModels_Rocks[Q_irand(0, NUM_DEBRIS_MODELS_ROCKS-1)];
		}

		VectorCopy(org, shardorg);

		dif[0] = (maxs[0]-mins[0])/2;
		dif[1] = (maxs[1]-mins[1])/2;
		dif[2] = (maxs[2]-mins[2])/2;

		if (dif[0] < 2)
		{
			dif[0] = 2;
		}
		if (dif[1] < 2)
		{
			dif[1] = 2;
		}
		if (dif[2] < 2)
		{
			dif[2] = 2;
		}

		difx[0] = Q_irand(1, 2 * 0.9f * dif[0]);
		difx[1] = Q_irand(1, 2 * 0.9f * dif[1]);
		difx[2] = Q_irand(1, 2 * 0.9f * dif[2]);

		if (difx[0] > dif[0])
		{
			shardorg[0] += difx[0]-(dif[0]);
		}
		else
		{
			shardorg[0] -= difx[0];
		}
		if (difx[1] > dif[1])
		{
			shardorg[1] += difx[1]-(dif[1]);
		}
		else
		{
			shardorg[1] -= difx[1];
		}
		if (difx[2] > dif[2])
		{
			shardorg[2] += difx[2]-(dif[2]);
		}
		else
		{
			shardorg[2] -= difx[2];
		}

		//CG_TestLine(org, shardorg, 5000, 0x0000ff, 3);

		CG_ThrowChunk( shardorg, velocity, debrismodel, debrissound, 0 );

		shardsthrow += 10;
	}
}

/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum( int client, const vec3_t org, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			angles;

	// only visualize for the client that scored
	if (client != cg.predictedPlayerState.clientNum || cg_damagePlums.integer == 0) {
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 1000;
	le->lifeRate = 1.0f / ( le->endTime - le->startTime );

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;

	VectorCopy( org, le->pos.trBase );

	re = &le->refEntity;

	re->reType = RT_SPRITE;

	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}

#ifdef UNUSED
/*
-------------------------
CG_ExplosionEffects

Used to find the player and shake the camera if close enough
intensity ranges from 1 (minor tremble) to 16 (major quake)
-------------------------
*/

void CG_ExplosionEffects( vec3_t origin, float intensity, int radius)
{
	//FIXME: When exactly is the vieworg calculated in relation to the rest of the frame?s

	vec3_t	dir;
	float	dist;
	// float	intensityScale;
	// float	realIntensity;

	VectorSubtract( cg.refdef.vieworg, origin, dir );
	dist = VectorNormalize( dir );

	//Use the dir to add kick to the explosion

	if ( dist > radius )
		return;

	// intensityScale = 1 - ( dist / (float) radius );
	// realIntensity = intensity * intensityScale;

//	CGCam_Shake( realIntensity, 750 ); // 500 seemed a bit too quick
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, int numFrames, qhandle_t shader,
								int msec, qboolean isSprite, float scale, int flags )
{
	float			ang = 0;
	localEntity_t	*ex;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CG_MakeExplosion: msec = %i", msec );
	}

	// skew the time a bit so they aren't all in sync
	offset = id_rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;
		ex->refEntity.rotation = id_rand() % 360;
		ex->radius = scale;
		VectorScale( dir, 16, tmpVec );
		VectorAdd( tmpVec, origin, newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, newOrigin );

		// set axis with random rotate when necessary
		if ( !dir )
		{
			AxisClear( ex->refEntity.axis );
		}
		else
		{
			if ( !(flags & LEF_NO_RANDOM_ROTATE) )
				ang = id_rand() % 360;
			VectorCopy( dir, ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;
	ex->lifeRate = (float)numFrames / msec;
	ex->leFlags = flags;

	//Scale the explosion
	if (scale != 1) {
		ex->refEntity.nonNormalizedAxes = qtrue;

		VectorScale( ex->refEntity.axis[0], scale, ex->refEntity.axis[0] );
		VectorScale( ex->refEntity.axis[1], scale, ex->refEntity.axis[1] );
		VectorScale( ex->refEntity.axis[2], scale, ex->refEntity.axis[2] );
	}
	// set origin
	VectorCopy ( newOrigin, ex->refEntity.origin);
	VectorCopy ( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
-------------------------
CG_SurfaceExplosion

Adds an explosion to a surface
-------------------------
*/

#define NUM_SPARKS		12
#define NUM_PUFFS		1
#define NUM_EXPLOSIONS	4

void CG_SurfaceExplosion( vec3_t origin, vec3_t normal, float radius, float shake_speed, qboolean smoke )
{
	localEntity_t	*le;
	//FXTrail			*particle;
	vec3_t			direction, new_org;
	static const vec3_t	velocity = { 0.0f, 0.0f, 16.0f };
	vec3_t			temp_org, temp_vel;
	int				i;

	/*
	//Sparks
	int numSparks = 16 + (random() * 16.0f);

	for ( i = 0; i < numSparks; i++ )
	{
		float scale = 0.25f + (random() * 2.0f);

		particle = FX_AddTrail( origin,
								NULL,
								NULL,
								32.0f,
								-64.0f,
								scale,
								-scale,
								1.0f,
								0.0f,
								0.25f,
								4000.0f,
								cgs.media.sparkShader,
								rand() & FXF_BOUNCE);
		if ( particle == NULL )
			return;

		FXE_Spray( normal, 500, 150, 1.0f, 768 + (rand() & 255), (FXPrimitive *) particle );
	}
	*/

	//Smoke
	//Move this out a little from the impact surface
	VectorMA( origin, 4, normal, new_org );

	for ( i = 0; i < 4; i++ )
	{
		VectorSet( temp_org, new_org[0] + (crandom() * 16.0f), new_org[1] + (crandom() * 16.0f), new_org[2] + (random() * 4.0f) );
		VectorSet( temp_vel, velocity[0] + (crandom() * 8.0f), velocity[1] + (crandom() * 8.0f), velocity[2] + (crandom() * 8.0f) );

/*		FX_AddSprite(	temp_org,
						temp_vel,
						NULL,
						64.0f + (random() * 32.0f),
						16.0f,
						1.0f,
						0.0f,
						20.0f + (crandom() * 90.0f),
						0.5f,
						1500.0f,
						cgs.media.smokeShader, FXF_USE_ALPHA_CHAN );*/
	}

	//Core of the explosion

	//Orient the explosions to face the camera
	VectorSubtract( cg.refdef.vieworg, origin, direction );
	VectorNormalize( direction );

	//Tag the last one with a light
	le = CG_MakeExplosion( origin, direction, cgs.media.explosionModel, 6, cgs.media.surfaceExplosionShader, 500, qfalse, radius * 0.02f + (random() * 0.3f), 0);
	le->light = 150;
	VectorSet( le->lightColor, 0.9f, 0.8f, 0.5f );

	for ( i = 0; i < NUM_EXPLOSIONS-1; i ++)
	{
		VectorSet( new_org, (origin[0] + (16 + (crandom() * 8))*crandom()), (origin[1] + (16 + (crandom() * 8))*crandom()), (origin[2] + (16 + (crandom() * 8))*crandom()) );
		le = CG_MakeExplosion( new_org, direction, cgs.media.explosionModel, 6, cgs.media.surfaceExplosionShader, 300 + (id_rand() & 99), qfalse, radius * 0.05f + (crandom() *0.3f), 0);
	}

	//Shake the camera
	CG_ExplosionEffects( origin, shake_speed, 350 );

	// The level designers wanted to be able to turn the smoke spawners off.  The rationale is that they
	//	want to blow up catwalks and such that fall down...when that happens, it shouldn't really leave a mark
	//	and a smoke spewer at the explosion point...
	if ( smoke )
	{
		VectorMA( origin, -8, normal, temp_org );
//		FX_AddSpawner( temp_org, normal, NULL, NULL, 100, random()*25.0f, 5000.0f, (void *) CG_SmokeSpawn );

		//Impact mark
		//FIXME: Replace mark
		//CG_ImpactMark( cgs.media.burnMarkShader, origin, normal, random()*360, 1,1,1,1, qfalse, 8, qfalse );
	}
}

/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed( vec3_t origin, int entityNum ) {
	localEntity_t	*ex;

	if ( !cg_blood.integer ) {
		return;
	}

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 500;

	VectorCopy ( origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = id_rand() % 360;
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = 0;//cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if ( entityNum == cg.snap->ps.clientNum ) {
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
	}
}



/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchGib( vec3_t origin, vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}
#endif // UNUSED
