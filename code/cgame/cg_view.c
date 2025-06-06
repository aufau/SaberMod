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

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

#include "cg_local.h"

#if !defined(CL_LIGHT_H_INC)
	#include "cg_lights.h"
#endif

#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
#define CAMERA_SIZE	4


/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	//cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;

	// rww - 9-13-01 [1-26-01-sof2]
	cg.testModelEntity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

//==============================================================================

//==============================================================================
//==============================================================================
// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

static const vec3_t	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static const vec3_t	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };

typedef struct dampPos_s {
	vec3_t	ideal;		// ideal location
	vec3_t	prevIdeal;
	vec3_t	damp;		// position = ideal + damp
} dampPos_t;

static struct {
	dampPos_t	target;
	dampPos_t	loc;
	vec3_t		fwd;
	vec3_t		focus;
	float		lastYaw;
	int			lastTime;
	float		lastTimeFrac;
	qboolean	smooth;			// Use new, smooth camera damping
	int			fps;			// FPS to emulate with smooth camera damping
} cam;

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdefViewAngles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(void)
{
#ifdef ATST
	if (cg.snap && cg.snap->ps.usingATST)
	{
		thirdPersonVertOffset = 200;
	}
#endif

	// Initialize IdealTarget
	if (cg.hasFallVector)
	{
		VectorCopy(cg.fallVector, cam.focus);
	}
	else
	{
		VectorCopy(cg.refdef.vieworg, cam.focus);
	}

	// Add in the new viewheight
	cam.focus[2] += cg.snap->ps.viewheight;

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
//	VectorMA(cam.focus, thirdPersonVertOffset, cameraup, cam.target.ideal);

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
	VectorCopy( cam.focus, cam.target.ideal );
	cam.target.ideal[2] += cg_thirdPersonVertOffset.value;
	//VectorMA(cam.focus, cg_thirdPersonVertOffset.value, cameraup, cam.target.ideal);
}



/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(void)
{
	float thirdPersonRange = cg.spec.thirdPersonRange;
#ifdef ATST
	if (cg.snap && cg.snap->ps.usingATST)
	{
		thirdPersonRange = 300;
	}
#endif
	VectorMA(cam.target.ideal, -(thirdPersonRange), cam.fwd, cam.loc.ideal);
}



static void CG_ResetThirdPersonViewDamp(void)
{
	trace_t trace;
	vec3_t	target;

	// Set the cam.target.ideal
	CG_CalcIdealThirdPersonViewTarget();

	// Set the cam.loc.ideal
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorClear(cam.loc.damp);
	VectorClear(cam.target.damp);
	VectorCopy(cam.loc.ideal, cam.loc.prevIdeal);
	VectorCopy(cam.target.ideal, cam.target.prevIdeal);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cam.focus, cameramins, cameramaxs, cam.target.ideal, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0f)
	{
		VectorSubtract(trace.endpos, cam.target.ideal, cam.target.damp);
	}

	VectorAdd(cam.target.ideal, cam.target.damp, target);

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, target, cameramins, cameramaxs, cam.loc.ideal, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0f)
	{
		VectorSubtract(trace.endpos, cam.loc.ideal, cam.loc.damp);
	}
}

static void CG_DampPosition(dampPos_t *pos, float dampfactor, float dtime)
{
	vec3_t idealDelta;

	if ( dtime <= 0.0f )
		return;

	VectorSubtract(pos->ideal, pos->prevIdeal, idealDelta);
	// saving previous ideal position only when dtime > 0 makes camera
	// freeze when player is lagging
	VectorCopy(pos->ideal, pos->prevIdeal);

	if ( cam.smooth )
	{
		// FPS-independent solution thanks to semigroup property:
		// If t1, t2 are positive time periods, dampfactor and
		// velocity (idealDelta) don't change then:
		// damp_(t1 + t2)(pos) = damp_t1(damp_t2(pos))

		// if dtime == 1 (stable framerate equal to cg_camerafps)
		// result is the same as in original code.
		vec3_t	shift;
		float	invdtime;
		float	timeadjfactor;
		float	codampfactor;

		// dtime is relative: physics time / emulated time
		dtime *= cam.fps / 1000.0f;
		invdtime = 1.0f / dtime;
		timeadjfactor = powf(dampfactor, dtime);
		// shift = (idealDelta / dtime) * (dampfactor / (1 - dampfactor))
		codampfactor = dampfactor / (1.0f - dampfactor);
		VectorScale(idealDelta, invdtime, shift);
		VectorScale(shift, codampfactor, shift);
		// damp(dtime) = dampfactor^dtime * (damp(0) + shift) - shift
		pos->damp[0] = timeadjfactor * (pos->damp[0] + shift[0]) - shift[0];
		pos->damp[1] = timeadjfactor * (pos->damp[1] + shift[1]) - shift[1];
		pos->damp[2] = timeadjfactor * (pos->damp[2] + shift[2]) - shift[2];
	}
	else
	{
		// Original JK2 camera damping:
		// idealDelta_n = ideal_n+1 - ideal_n
		// damp_n+1 = dampfactor * (damp_n - idealDelta_n)
		VectorSubtract(pos->damp, idealDelta, pos->damp);
		VectorScale(pos->damp, dampfactor, pos->damp);
	}
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp(float dtime)
{
	trace_t trace;
	vec3_t	target;
	float	dampfactor;

	// Set the cam.target.ideal
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	if (cg_thirdPersonTargetDamp.value >= 1.0f)
	{	// No damping.
		VectorClear(cam.target.damp);
	}
	else if (cg_thirdPersonTargetDamp.value > 0.0f)
	{
		dampfactor = 1.0f - cg_thirdPersonTargetDamp.value;	// We must exponent the amount LEFT rather than the amount bled off

		CG_DampPosition(&cam.target, dampfactor, dtime);
	}
	else
	{
		VectorSubtract(cam.target.prevIdeal, cam.target.ideal, cam.target.damp);
	}

	// Now we trace to see if the new location is cool or not.
	VectorAdd(cam.target.ideal, cam.target.damp, target);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cam.focus, cameramins, cameramaxs, target, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0f)
	{
		VectorSubtract(trace.endpos, cam.target.ideal, cam.target.damp);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
static void CG_UpdateThirdPersonCameraDamp(float dtime, float stiffFactor, float pitch)
{
	trace_t trace;
	vec3_t	location, target;
	float	dampfactor;

	// Set the cam.loc.ideal
	CG_CalcIdealThirdPersonViewLocation();

	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor = 0.0f;
	if (cg_thirdPersonCameraDamp.value != 0.0f)
	{
		// Note that the camera pitch has already been capped off to 89.
		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 89.0f;
		dampfactor = (1.0f - cg_thirdPersonCameraDamp.value) * pitch * pitch;

		dampfactor += cg_thirdPersonCameraDamp.value;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (stiffFactor > 0.0f)
		{	// The stiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0f - dampfactor) * stiffFactor;
		}
	}

	if (dampfactor >= 1.0f)
	{	// No damping.
		VectorClear(cam.loc.damp);
	}
	else if (dampfactor > 0.0f)
	{
		dampfactor = 1.0f - dampfactor;	// We must exponent the amount LEFT rather than the amount bled off

		CG_DampPosition(&cam.loc, dampfactor, dtime);
	}
	else
	{
		VectorSubtract(cam.loc.prevIdeal, cam.loc.ideal, cam.loc.damp);
	}

	VectorAdd(cam.loc.ideal, cam.loc.damp, location);
	VectorAdd(cam.target.ideal, cam.target.damp, target);

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, target, cameramins, cameramaxs, location, cg.snap->ps.clientNum, MASK_CAMERACLIP);

	if (trace.fraction < 1.0f)
	{
		VectorSubtract( trace.endpos, cam.loc.ideal, cam.loc.damp );

		//FIXME: when the trace hits movers, it gets very very jaggy... ?
		/*
		//this doesn't actually help any
		if ( trace.entityNum != ENTITYNUM_WORLD )
		{
			centity_t *cent = &cg_entities[trace.entityNum];
			gentity_t *gent = &g_entities[trace.entityNum];
			if ( cent != NULL && gent != NULL )
			{
				if ( cent->currentState.pos.trType == TR_LINEAR || cent->currentState.pos.trType == TR_LINEAR_STOP )
				{
					vec3_t	diff;
					VectorSubtract( cent->lerpOrigin, gent->currentOrigin, diff );
					VectorAdd( location, diff, location );
				}
			}
		}
		*/
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}


/*
===============`
CG_OffsetThirdPersonView

===============
*/
static void CG_OffsetThirdPersonView( void )
{
	vec3_t	target, location, diff;
	vec3_t	focusAngles;
	float	dtime;

	// Establish camera damping parameters
	if (cg_smoothCameraFPS.integer) {
		cam.fps = cg_smoothCameraFPS.integer;
	} else if (cg_com_maxfps.integer) {
		cam.fps = MIN(cg_com_maxfps.integer, 1000);
	}

	cam.smooth = cg_smoothCamera.integer && (cam.fps >= CAMERA_MIN_FPS);

	// Set camera viewing direction.
	VectorCopy( cg.refdefViewAngles, focusAngles );

	// if dead, look at killer
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		focusAngles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
	}
	else
	{	// Add in the third Person Angle.
		focusAngles[YAW] += cg_thirdPersonAngle.value;
		focusAngles[PITCH] += cg_thirdPersonPitchOffset.value;
	}

	// Cap the pitch within reasonable limits
	if (focusAngles[PITCH] > 80.0f)
	{
		focusAngles[PITCH] = 80.0f;
	}
	else if (focusAngles[PITCH] < -80.0f)
	{
		focusAngles[PITCH] = -80.0f;
	}

	AngleVectors(focusAngles, cam.fwd, NULL, NULL);

	// The next thing to do is to see if we need to calculate a new camera target location.

	dtime = cg.predictedPlayerState.commandTime - cam.lastTime;
	dtime += cg.predictedTimeFrac - cam.lastTimeFrac;

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cam.lastTime == 0 || dtime < 0.0f || cg.thisFrameTeleport || cg.snap->ps.pm_type == PM_PAUSED)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		float	stiffFactor;
		float	deltayaw;
		float	pitch;

		deltayaw = fabsf(focusAngles[YAW] - cam.lastYaw);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabsf(deltayaw - 360.0f);
		}
		if (cam.smooth) {
			if ( dtime > 0.0f ) {
				stiffFactor = deltayaw / dtime;
			} else {
				stiffFactor = 0.0f;
			}
		} else {
			stiffFactor = deltayaw / cg.frametime;
		}
		if (stiffFactor < 1.0f)
		{
			stiffFactor = 0.0f;
		}
		else if (stiffFactor > 2.5f)
		{
			stiffFactor = 0.75f;
		}
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			stiffFactor = (stiffFactor - 1.0f) * 0.5f;
		}

		pitch = Q_fabs(focusAngles[PITCH]);

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp(dtime);
		CG_UpdateThirdPersonCameraDamp(dtime, stiffFactor, pitch);
	}

	// Now interestingly, the Quake method is to calculate a target focus point above the player, and point the camera at it.
	// We won't do that for now.

	VectorAdd(cam.target.ideal, cam.target.damp, target);
	VectorAdd(cam.loc.ideal, cam.loc.damp, location);

	// We must now take the angle taken from the camera target and location.
	VectorSubtract(target, location, diff);
	if ( VectorLengthSquared( diff ) < 0.01f * 0.01f )
	{//must be hitting something, need some value to calc angles, so use cam forward
		VectorCopy( cam.fwd, diff );
	}

	vectoangles(diff, cg.refdefViewAngles);

	// Temp: just move the camera to the side a bit
	if ( cg_thirdPersonHorzOffset.value != 0.0f )
	{
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		VectorMA( location, cg_thirdPersonHorzOffset.value,
			cg.refdef.viewaxis[1], location );
	}

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(location, cg.refdef.vieworg);

	cam.lastTime = cg.predictedPlayerState.commandTime;
	cam.lastTimeFrac = cg.predictedTimeFrac;
	cam.lastYaw = focusAngles[YAW];
}



/*
===============
CG_OffsetThirdPersonView

===============
*//*
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( cg.refdefViewAngles, focusAngles );

	// if dead, look at killer
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enogh that this is poking out

			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_CAMERACLIP);
			VectorCopy( trace.endpos, view );
		}
	}


	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}*/

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	int				timeDelta;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	VectorAdd (angles, cg.kick_angles, angles);

	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0f - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching accentuates roll
	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0f - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;

	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

#ifdef UNUSED
void CG_ZoomDown_f( void ) {
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void CG_ZoomUp_f( void ) {
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}



/*
====================
CG_CalcFovFromX

Calcs Y FOV from given X FOV
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

qboolean CG_CalcFOVFromX( float fov_x )
{
	float	x;
//	float	phase;
//	float	v;
//	int		contents;
	float	fov_y;
	qboolean	inwater;

	x = cg.refdef.width / tanf( DEG2RAD( 0.5f * fov_x ) );
	fov_y = RAD2DEG( 2 * atan2f( cg.refdef.height, x ) );

	// there's a problem with this, it only takes the leafbrushes into account, not the entity brushes,
	//	so if you give slime/water etc properties to a func_door area brush in order to move the whole water
	//	level up/down this doesn't take into account the door position, so warps the view the whole time
	//	whether the water is up or not. Fortunately there's only one slime area in Trek that you can be under,
	//	so lose it...
#if 0
/*
	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}
*/
#else
	inwater = qfalse;
#endif


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	return (inwater);
}
#endif // UNUSED

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4
float zoomFov; //this has to be global client-side

static int CG_CalcFov( void ) {
	float	x;
	float	phase;
	float	v;
	int		contents;
	float	fov_x, fov_y;
	float	f;
	int		inwater;
	float	cgFov;

	cgFov = CLAMP(1, MAX_FOV, cg_fov.value);

	if ( cgs.dmflags & DF_FIXED_FOV ) {
		// dmflag to use basejk fov limit
		cgFov = MIN(97, cgFov);
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 80;//90;
	} else {
		fov_x = cgFov;

		switch (cg.predictedPlayerState.zoomMode) {
		case ZOOM_BINOCULARS:
			if (zoomFov > 40.0f)
			{
				zoomFov -= cg.frametime * 0.075f;

				if (zoomFov < 40.0f)
				{
					zoomFov = 40.0f;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
			}

			fov_x = zoomFov;
			break;
		case ZOOM_DISRUPTOR:
			if (!cg.predictedPlayerState.zoomLocked)
			{
				if (zoomFov > 50)
				{ //Now starting out at nearly half zoomed in
					zoomFov = 50;
				}
				zoomFov -= cg.frametime * 0.035f;//0.075f;

				if (zoomFov < MAX_ZOOM_FOV)
				{
					zoomFov = MAX_ZOOM_FOV;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
				else
				{	// Still zooming
					static int zoomSoundTime = 0;

					if (zoomSoundTime < cg.time || zoomSoundTime > cg.time + 10000)
					{
						trap_S_StartSound(cg.refdef.vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, cgs.media.disruptorZoomLoop);
						zoomSoundTime = cg.time + 300;
					}
				}
			}

			fov_x = zoomFov;
			break;
		case ZOOM_NONE:
			f = ( cg.gameTime - cg.predictedPlayerState.zoomTime ) * ( 1.0f / ZOOM_OUT_TIME );

			if ( f < 1.0f ) {
				fov_x = zoomFov + f * (fov_x - zoomFov);
				// original:
				// fov_x = f * fov_x;
			} else {
				zoomFov = 80;
			}
			break;
		}
	}

	// don't allow without cg_widescreen so people don't use it to
	// stretch disruptor zoom mask
    if ( cg_widescreen.integer && cg_fovAspectAdjust.integer &&
		cg.refdef.width * 3 > cg.refdef.height * 4 )
	{
        // 4:3 screen with fov_x must fit INTO widescreen
        float width = cg.refdef.height * (4.0f / 3.0f);

        x = width / tanf( DEG2RAD( 0.5f * fov_x ) );
        fov_x = RAD2DEG( 2 * atan2f( cg.refdef.width, x ) );
        fov_y = RAD2DEG( 2 * atan2f( cg.refdef.height, x ) );
    }
	else
	{
        x = cg.refdef.width / tanf( DEG2RAD( 0.5f * fov_x ) );
        fov_y = RAD2DEG( 2 * atan2f( cg.refdef.height, x ) );
	}

	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / (float) (1000 * WAVE_FREQUENCY * M_PI * 2);
		v = WAVE_AMPLITUDE * sinf( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}


	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if (cg.predictedPlayerState.zoomMode != ZOOM_NONE)
	{
		cg.zoomSensitivity = zoomFov/cgFov;
	}
	else if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0f;
	}

	return inwater;
}


/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void )
{
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3 * ( 1.0f - ((float)t / maxTime) );

	if (cg.snap->ps.damageType == 0)
	{ //pure health
		ent.customShader = cgs.media.viewPainShader;
		ent.shaderRGBA[0] = 180 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 50 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else if (cg.snap->ps.damageType == 1)
	{ //pure shields
		ent.customShader = cgs.media.viewPainShader_Shields;
		ent.shaderRGBA[0] = 50 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else
	{ //shields and health
		ent.customShader = cgs.media.viewPainShader_ShieldsAndHealth;
		ent.shaderRGBA[0] = 180 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0f - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	trap_R_AddRefEntityToScene( &ent );
}

#ifdef UNUSED
qboolean CheckOutOfConstrict(float curAng)
{
	float degrees_negative, degrees_positive;

	float angle_ideal = cg.constrictValue;
	float angle_current = curAng;

	float angle_dif = 0;

	if (angle_current < 0)
	{
		angle_current += 360;
	}
	if (angle_current > 360)
	{
		angle_current -= 360;
	}

	if (cg.doConstrict <= cg.time)
	{
		return qfalse;
	}

	if (angle_ideal <= angle_current)
	{
		degrees_negative = (angle_current - angle_ideal);

		degrees_positive = (360 - angle_current) + angle_ideal;
	}
	else
	{
		degrees_negative = angle_current + (360 - angle_ideal);

		degrees_positive = (angle_ideal - angle_current);
	}

	if (degrees_negative < degrees_positive)
	{
		angle_dif = degrees_negative;
	}
	else
	{
		angle_dif = degrees_positive;
	}

	if (angle_dif > 60)
	{
		return qtrue;
	}

	return qfalse;
}
#endif // UNUSED


/*
=================
CG_CalcSpectatorView

Calculate third person camera angles for spectator
=================
*/
static void CG_CalcSpectatorView() {
	static float	thirdPersonRangePos = 1;

	if (cg.spec.following && cg.spec.mode == SPECMODE_FREEANGLES)
	{
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );
		PM_UpdateViewAngles2( cg.refdefViewAngles, cg.spec.delta_angles, &cmd );

		thirdPersonRangePos -= cmd.forwardmove * cg.frametime / (1000.0f * 127.0f);

		// don't allow it too close or camera damping makes it jumpy
		if (thirdPersonRangePos < 0.3f) {
			thirdPersonRangePos = 0.3f;
		}

		cg.spec.thirdPersonRange = 80.0f * thirdPersonRangePos * thirdPersonRangePos;
	} else {
		cg.spec.thirdPersonRange = cg_thirdPersonRange.value;
	}
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues( void ) {
	const playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap_getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/

	// fall vector
	if ( ps->fallingToDeath ) {
		if ( !cg.hasFallVector ) {
			VectorCopy(cg.snap->ps.origin, cg.fallVector);
			cg.hasFallVector = qtrue;
		}
	} else {
		if ( cg.hasFallVector ) {
			cg.hasFallVector = qfalse;
			VectorClear(cg.fallVector);
		}
	}

	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		cg.hasFallVector = qfalse;
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabsf( sinf( ( ps->bobCycle & 127 ) * (float) ( M_PI / 127 ) ) );
	cg.xyspeed = sqrtf( ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1] );

	if (cg.xyspeed > 270)
	{
		cg.xyspeed = 270;
	}

	VectorCopy( ps->origin, cg.refdef.vieworg );
	VectorCopy( ps->viewangles, cg.refdefViewAngles );

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson ) {
		CG_CalcSpectatorView();
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// reset third person camera damping
		cam.lastTime = 0;
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye reletive to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.gameTime ) {
			continue;
		}
		if ( t - cg.gameTime >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		/*
		if ( ( t - cg.serverTime ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
		*/
	}
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

void CG_UpdateSoundTrackers()
{
	int		i;

	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		centity_t *cent = &cg_entities[ cg.snap->entities[ i ].number ];

		if (cent->currentState.eFlags & EF_SOUNDTRACKER)
		{ //keep sound for this entity updated in accordance with its attached entity at all times
			if (cent->currentState.trickedentindex == cg.snap->ps.clientNum)
			{ //this is actually the player, so center the sound origin right on top of us
				VectorCopy(cg.refdef.vieworg, cent->lerpOrigin);
				trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
			}
			else
			{
				trap_S_UpdateEntityPosition( cent->currentState.number, cg_entities[cent->currentState.trickedentindex].lerpOrigin );
			}
		}
	}
}

//=========================================================================

/*
================================
Screen Effect stuff starts here
================================
*/
#define	CAMERA_DEFAULT_FOV			90.0f
#define MAX_SHAKE_INTENSITY			16.0f

cgscreffects_t cgScreenEffects;

void CG_SE_UpdateShake( vec3_t origin, vec3_t angles )
{
	vec3_t	moveDir;
	float	intensity_scale, intensity;
	int		i;

	if ( cgScreenEffects.shake_duration <= 0 )
		return;

	if ( cg.serverTime > ( cgScreenEffects.shake_start + cgScreenEffects.shake_duration ) )
	{
		cgScreenEffects.shake_intensity = 0;
		cgScreenEffects.shake_duration = 0;
		cgScreenEffects.shake_start = 0;
		return;
	}

	cgScreenEffects.FOV = CAMERA_DEFAULT_FOV;
	cgScreenEffects.FOV2 = CAMERA_DEFAULT_FOV;

	//intensity_scale now also takes into account FOV with 90.0 as normal
	intensity_scale = 1.0f - ( (float) ( cg.serverTime - cgScreenEffects.shake_start ) / (float) cgScreenEffects.shake_duration ) * (((cgScreenEffects.FOV+cgScreenEffects.FOV2)/2.0f)/90.0f);

	intensity = cgScreenEffects.shake_intensity * intensity_scale;

	for ( i = 0; i < 3; i++ )
	{
		moveDir[i] = ( crandom() * intensity );
	}

	//Move the camera
	VectorAdd( origin, moveDir, origin );

	for ( i=0; i < 2; i++ ) // Don't do ROLL
		moveDir[i] = ( crandom() * intensity );

	//Move the angles
	VectorAdd( angles, moveDir, angles );
}

void CG_SE_UpdateMusic(void)
{
	if (cgScreenEffects.music_volume_multiplier < 0.1f)
	{
		cgScreenEffects.music_volume_multiplier = 1.0f;
		return;
	}

	if (cgScreenEffects.music_volume_time < cg.time)
	{
		if (cgScreenEffects.music_volume_multiplier != 1.0f || cgScreenEffects.music_volume_set)
		{
			char musMultStr[512];

			cgScreenEffects.music_volume_multiplier += 0.1f;
			if (cgScreenEffects.music_volume_multiplier > 1.0f)
			{
				cgScreenEffects.music_volume_multiplier = 1.0f;
			}

			Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
			trap_Cvar_Set("s_musicMult", musMultStr);

			if (cgScreenEffects.music_volume_multiplier == 1.0f)
			{
				cgScreenEffects.music_volume_set = qfalse;
			}
			else
			{
				cgScreenEffects.music_volume_time = cg.time + 200;
			}
		}

		return;
	}

	if (!cgScreenEffects.music_volume_set)
	{ //if the volume_time is >= cg.time, we should have a volume multiplier set
		char musMultStr[512];

		Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
		trap_Cvar_Set("s_musicMult", musMultStr);
		cgScreenEffects.music_volume_set = qtrue;
	}
}

/*
=================
CG_CalcScreenEffects

Currently just for screen shaking (and music volume management)
=================
*/
void CG_CalcScreenEffects(void)
{
	CG_SE_UpdateShake(cg.refdef.vieworg, cg.refdefViewAngles);
	CG_SE_UpdateMusic();
}

void CGCam_Shake( float intensity, int duration )
{
	if ( intensity > MAX_SHAKE_INTENSITY )
		intensity = MAX_SHAKE_INTENSITY;

	cgScreenEffects.shake_intensity = intensity;
	cgScreenEffects.shake_duration = duration;
	cgScreenEffects.shake_start = cg.serverTime;
}

void CGCam_SetMusicMult( float multiplier, int duration )
{
	if (multiplier < 0.1f)
	{
		multiplier = 0.1f;
	}

	if (multiplier > 1.0f)
	{
		multiplier = 1.0f;
	}

	cgScreenEffects.music_volume_multiplier = multiplier;
	cgScreenEffects.music_volume_time = cg.time + duration;
	cgScreenEffects.music_volume_set = qfalse;
}

/*
================================
Screen Effect stuff ends here
================================
*/

/*
=================
CG_SeekFrame

Returns if rendering current frame should be skipped
=================
*/
static qboolean CG_SeekFrame( void ) {
	qboolean skipFrame = qfalse;

	if (!cg.seekTime) {
		return skipFrame;
	}

	if (cg.seekTime > cg.serverTime) {
		// engine evaluates fixedtime ms of game time in next
		// frame. 200 is a limit imposed by the engine.
		trap_Cvar_Set( "fixedtime", va( "%d", cg.seekTime - cg.serverTime ) );
		if (cg.fastSeek) {
			if ( cg.savedmaxfps[0] == '\0' ) {
				trap_Cvar_VariableStringBuffer( "com_maxfps", cg.savedmaxfps, sizeof( cg.savedmaxfps ) );
				trap_Cvar_Set( "com_maxfps", "0" );
			}
			skipFrame = cg_fastSeek.integer;
		}
	} else {
		trap_Cvar_Set( "fixedtime", "0" );
		if ( cg.savedmaxfps[0] ) {
			trap_Cvar_Set( "com_maxfps", cg.savedmaxfps );
		}
		cg.seekTime = 0;
	}

	return skipFrame;
}

/*
=================
CG_TimeBias

Bias serverTime so that its value is low enough to fit into a single
precision float (at least for 4h39m after CGame initialization).
=================
*/
static int CG_TimeBias( int serverTime ) {
	static int	bias = 0;

	if (cg.time == 0 && serverTime != 0)
	{
		char buf[2];

		trap_Cvar_VariableStringBuffer( "cg_fixServerTime", buf, sizeof(buf) );

		if (atoi(buf)) {
			// give it some headroom to avoid negative time values
			bias = 4096 - serverTime;
			// may help with client synchronization in some cases
			bias += serverTime & 4095;
		}
	}
	else if (cgs.unpauseTime > serverTime)
	{
		bias = cg.time - serverTime;
	}

	return serverTime + bias;
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;

	cg.time = CG_TimeBias( serverTime );
	cg.serverTime = serverTime;
	cg.demoPlayback = demoPlayback;

	if (!cg.gameTime || cgs.unpauseTime <= serverTime) {
		cg.gameTime = serverTime;
	}

	if (CG_SeekFrame()) {
		return;
	}

	if (cg.snap && cg_ui_myteam.integer != cg.snap->ps.persistant[PERS_TEAM])
	{
		trap_Cvar_Set ( "ui_myteam", va("%i", cg.snap->ps.persistant[PERS_TEAM]) );
	}

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	trap_FX_AdjustTime( cg.time, cg.refdef.vieworg, cg.refdef.viewaxis );

	CG_RunLightStyles();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	trap_ROFF_UpdateEntities();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	if (cg.snap && cg.snap->ps.saberLockTime > cg.serverTime)
	{
		trap_SetUserCmdValue( cg.weaponSelect, 0.01f, cg.forceSelect, cg.itemSelect );
	}
	else if (cg.snap && cg.snap->ps.usingATST)
	{
		trap_SetUserCmdValue( cg.weaponSelect, 0.2f, cg.forceSelect, cg.itemSelect );
	}
	else
	{
		trap_SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity, cg.forceSelect, cg.itemSelect );
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// decide on third person view
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
		cg.renderingThirdPerson = qfalse;
	else if (cg.snap->ps.zoomMode != ZOOM_NONE)
		cg.renderingThirdPerson = qfalse;
	else if (cg.snap->ps.stats[STAT_HEALTH] <= 0)
		cg.renderingThirdPerson = qtrue;
	else if (cg.predictedPlayerState.usingATST)
		cg.renderingThirdPerson = qtrue;
	else if (cg.predictedPlayerState.forceHandExtend == HANDEXTEND_KNOCKDOWN)
		cg.renderingThirdPerson = qtrue;
	else if (cg.predictedPlayerState.fallingToDeath)
		cg.renderingThirdPerson = qtrue;
	else if (cg.predictedPlayerState.weapon == WP_SABER)
		cg.renderingThirdPerson = (qboolean) !cg_fpls.integer;
	else
		cg.renderingThirdPerson = (qboolean) !!cg_thirdPerson.integer;

	// build cg.refdef
	inwater = CG_CalcViewValues();

	CG_CalcScreenEffects();

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		// CG_AddParticles ();
		CG_AddLocalEntities();
	}
	CG_AddViewWeapon( &cg.predictedPlayerState );

	if ( !cg.hyperspace)
	{
		trap_FX_AddScheduledEffects();
	}

	CG_PlayGameStateSounds();

	// add buffered sounds
	CG_PlayBufferedSounds();
#ifdef MISSIONPACK
	// play buffered voice chats
	CG_PlayBufferedVoiceChats();
#endif
	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// if there are any entities flagged as sound trackers and attached to other entities, update their sound pos
	CG_UpdateSoundTrackers();

	if (cg.hasFallVector)
	{
		vec3_t lookAng;

		VectorSubtract(cg.snap->ps.origin, cg.refdef.vieworg, lookAng);
		VectorNormalize(lookAng);
		vectoangles(lookAng, lookAng);

		VectorCopy(cg.fallVector, cg.refdef.vieworg);
		AnglesToAxis(lookAng, cg.refdef.viewaxis);
	}

	// update audio positions
	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.serverTime - cg.oldServerTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldServerTime = cg.serverTime;
		CG_AddLagometerFrameInfo();
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	// actually issue the rendering calls
	CG_DrawActive( stereoView );

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}

	if (cg.queueMacroscan && !demoPlayback) {
		cg.queueMacroscan = qfalse;
		trap_SendConsoleCommand("ui_macroscan 0\n");
	}
}

