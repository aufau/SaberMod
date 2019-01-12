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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#include "../ui/ui_shared.h"

#ifdef MISSIONPACK
// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#endif

qboolean CG_WorldCoordToScreenCoord(const vec3_t worldCoord, float *x, float *y);
qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle );

static const vec4_t	bluehudtint = {0.5, 0.5, 1.0, 1.0};
static const vec4_t	redhudtint = {1.0, 0.5, 0.5, 1.0};
const float	*hudTintColor;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

extern float zoomFov; //this has to be global client-side

const char * const showPowersName[] =
{
	"HEAL2",//FP_HEAL
	"JUMP2",//FP_LEVITATION
	"SPEED2",//FP_SPEED
	"PUSH2",//FP_PUSH
	"PULL2",//FP_PULL
	"MINDTRICK2",//FP_TELEPTAHY
	"GRIP2",//FP_GRIP
	"LIGHTNING2",//FP_LIGHTNING
	"DARK_RAGE2",//FP_RAGE
	"PROTECT2",//FP_PROTECT
	"ABSORB2",//FP_ABSORB
	"TEAM_HEAL2",//FP_TEAM_HEAL
	"TEAM_REPLENISH2",//FP_TEAM_FORCE
	"DRAIN2",//FP_DRAIN
	"SEEING2",//FP_SEE
	"SABER_OFFENSE2",//FP_SABERATTACK
	"SABER_DEFENSE2",//FP_SABERDEFEND
	"SABER_THROW2",//FP_SABERTHROW
	NULL
};


qhandle_t MenuFontToHandle(font_t iMenuFont)
{
#ifdef MISSIONPACK
	switch (iMenuFont)
	{
		case FONT_SMALL:	return cgDC.Assets.qhSmallFont;
		case FONT_MEDIUM:	return cgDC.Assets.qhMediumFont;
		case FONT_LARGE:	return cgDC.Assets.qhBigFont;
	}

	return cgDC.Assets.qhMediumFont;
#else
	switch (iMenuFont)
	{
		case FONT_SMALL:	return cgs.media.qhSmallFont;
		case FONT_MEDIUM:	return cgs.media.qhMediumFont;
		case FONT_LARGE:	return cgs.media.qhBigFont;
	}

	return cgs.media.qhMediumFont;
#endif
}

float CG_Text_Width(const char *text, float scale, font_t iMenuFont)
{
	qhandle_t iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_StrLenPixels(text, iFontIndex, scale);
}

float CG_Text_Height(const char *text, float scale, font_t iMenuFont)
{
	qhandle_t iFontIndex = MenuFontToHandle(iMenuFont);

	return trap_R_Font_HeightPixels(iFontIndex, scale);
}

#include "../qcommon/qfiles.h"	// for STYLE_BLINK etc
void CG_Text_Paint(float x, float y, float scale, const vec4_t color, const char *text, float adjust, int limit, int style, font_t iMenuFont)
{
	int iStyle = (int)MenuFontToHandle(iMenuFont);

	switch (style)
	{
	case  ITEM_TEXTSTYLE_NORMAL:										break;	// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyle |= STYLE_BLINK;		break;	// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyle |= STYLE_BLINK;		break;	// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyle |= STYLE_DROPSHADOW;	break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyle |= STYLE_DROPSHADOW;	break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyle |= STYLE_DROPSHADOW;	break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyle |= STYLE_DROPSHADOW;	break;	// JK2 drop shadow ( need a color for this )
	}

	trap_R_Font_DrawString(x, y, text, color, iStyle, limit, scale);
}

/*
qboolean CG_WorldCoordToScreenCoord(vec3_t worldCoord, int *x, int *y)

  Take any world coord and convert it to a 2D virtual 640x480 screen coord
*/
/*
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y)
{
	int	xcenter, ycenter;
	vec3_t	local, transformed;

//	xcenter = cg.refdef.width / 2;//gives screen coords adjusted for resolution
//	ycenter = cg.refdef.height / 2;//gives screen coords adjusted for resolution

	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = 640 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn
	ycenter = 480 / 2;//gives screen coords in virtual 640x480, to be adjusted when drawn

	VectorSubtract (worldCoord, cg.refdef.vieworg, local);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);

	// Make sure Z is not negative.
	if(transformed[2] < 0.01)
	{
		return qfalse;
	}
	// Simple convert to screen coords.
	float xzi = xcenter / transformed[2] * (90.0/cg.refdef.fov_x);
	float yzi = ycenter / transformed[2] * (90.0/cg.refdef.fov_y);

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vec3_t worldCoord, int *x, int *y )
{
	float	xF, yF;
	qboolean retVal = CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF );
	*x = (int)xF;
	*y = (int)yF;
	return retVal;
}
*/

/*
================
CG_DrawZoomMask

================
*/
static void CG_DrawZoomMask( void )
{
	vec4_t		color1;
	float		level;
	static qboolean	flip = qtrue;

//	int ammo = cg_entities[0].gent->client->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
	float cx, cy;
//	int val[5];
	float max, fi;

	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if ( cg.predictedPlayerState.zoomMode == ZOOM_BINOCULARS )
	{
		// zoom level
		level = (float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		CG_WideScreenMode(qfalse);

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap_R_SetColor( colorTable[CT_WHITE] );
		trap_R_DrawStretchPic( 34, 48, 570, 362, 0, 0, 1, 1, cgs.media.binocularStatic );

		// Black out the area behind the numbers
		trap_R_SetColor( colorTable[CT_BLACK]);
		trap_R_DrawStretchPic( 212, 367, 200, 40, 0, 0, 1, 1, cgs.media.whiteShader );

		CG_WideScreenMode(qtrue);

		// Numbers should be kind of greenish
		color1[0] = 0.2f;
		color1[1] = 0.4f;
		color1[2] = 0.2f;
		color1[3] = 0.3f;
		trap_R_SetColor( color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		{
			// shift to match angles in original cgame
			const float	angle = AngleSubtract(cg.refdefViewAngles[YAW], -23.5f);
			int			val = (int)((angle + 180) / 10) * 10;
			const float	off = (angle + 180) - val;
			const float	segmentW = 100; // 10 * max off
			const float middle = cgs.screenWidth * 0.5f;
			float		x;
			int			i;

			// make sure we don't loop forever in case of nan/inf off value
			for ( i = -5; i <= 5; i++ )
			{
				val -= 10;
				if ( val < 0 ) {
					val += 360;
				}

				x = middle + i * segmentW + off * 10;
				if ( x + segmentW < cgs.screenWidth * 0.3f ) {
					continue;
				}
				if ( x > cgs.screenWidth * 0.7f ) {
					break;
				}

				CG_DrawNumField( x, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( x + 80, 376, 6, 6, cgs.media.whiteShader );
			}
		}

		CG_WideScreenMode(qfalse);

		trap_R_DrawStretchPic( 212, 367, 200, 28, 0, 0, 1, 1, cgs.media.binocularOverlay );

		color1[0] = sinf( cg.time * 0.01f ) * 0.5f + 0.5f;
		color1[0] = color1[0] * color1[0];
		color1[1] = color1[0];
		color1[2] = color1[0];
		color1[3] = 1.0f;

		trap_R_SetColor( color1 );

		CG_WideScreenMode(qtrue);

		CG_DrawPic( 82 * cgs.screenXFactorInv, 94, 16, 16, cgs.media.binocularCircle );

		CG_WideScreenMode(qfalse);

		// Flickery color
		color1[0] = 0.7f + crandom() * 0.1f;
		color1[1] = 0.8f + crandom() * 0.1f;
		color1[2] = 0.7f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );

		trap_R_DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, cgs.media.binocularMask );

		trap_R_DrawStretchPic( 4, 282 - level, 16, 16, 0, 0, 1, 1, cgs.media.binocularArrow );

		// The top triangle bit randomly flips
		if ( flip )
		{
			trap_R_DrawStretchPic( 330, 60, -26, -30, 0, 0, 1, 1, cgs.media.binocularTri );
		}
		else
		{
			trap_R_DrawStretchPic( 307, 40, 26, 30, 0, 0, 1, 1, cgs.media.binocularTri );
		}

		if ( random() > 0.98f && ( cg.time & 1024 ))
		{
			flip = (qboolean)!flip;
		}

		CG_WideScreenMode(qtrue);
	}
	else if ( cg.predictedPlayerState.zoomMode == ZOOM_DISRUPTOR )
	{
		float xOffset = 0.5f * (cgs.screenWidth - 640);

		// disruptor zoom mode
		level = (float)(50.0f - zoomFov) / 50.0f;//(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork.
		level *= 103.0f;

		// Draw target mask
		CG_FillRect( 0, 0, xOffset, 480, colorTable[CT_BLACK] );
		CG_FillRect( xOffset + 640, 0, xOffset, 480, colorTable[CT_BLACK] );
		trap_R_SetColor( colorTable[CT_WHITE] );
		CG_DrawPic( xOffset, 0, 640, 480, cgs.media.disruptorMask );


		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			color1[0] = 1.0f;
			color1[1] = 1.0f;
			color1[2] = 1.0f;
			color1[3] = 0.7f + sinf( cg.time * 0.01f ) * 0.3f;

			trap_R_SetColor( color1 );
		}

		// Draw rotating insert
		CG_DrawRotatePic2( 0.5f * cgs.screenWidth, 240, 640, 480, -level, cgs.media.disruptorInsert );

		// Increase the light levels under the center of the target
//		CG_DrawPic( 198, 118, 246, 246, cgs.media.disruptorLight );

		// weirdness.....converting ammo to a base five number scale just to be geeky.
/*		val[0] = ammo % 5;
		val[1] = (ammo / 5) % 5;
		val[2] = (ammo / 25) % 5;
		val[3] = (ammo / 125) % 5;
		val[4] = (ammo / 625) % 5;

		color1[0] = 0.2f;
		color1[1] = 0.55f + crandom() * 0.1f;
		color1[2] = 0.5f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap_R_SetColor( color1 );

		for ( int t = 0; t < 5; t++ )
		{
			cx = 320 + sin( (t*10+45)/57.296f ) * 192;
			cy = 240 + cos( (t*10+45)/57.296f ) * 192;

			CG_DrawRotatePic2( cx, cy, 24, 38, 45 - t * 10, trap_R_RegisterShader( va("gfx/2d/char%d",val[4-t] )));
		}
*/
		//max = ( cg_entities[0].gent->health / 100.0f );

		max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max;
		if ( max > 1.0f )
		{
			max = 1.0f;
		}

		color1[0] = (1.0f - max) * 2.0f;
		color1[1] = max * 1.5f;
		color1[2] = 0.0f;
		color1[3] = 1.0f;

		// If we are low on health, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
		{
			VectorClear( color1 );
		}

		if ( color1[0] > 1.0f )
		{
			color1[0] = 1.0f;
		}

		if ( color1[1] > 1.0f )
		{
			color1[1] = 1.0f;
		}

		trap_R_SetColor( color1 );

		max *= 58.0f;

		for (fi = 18.5f; fi <= 18.5f + max; fi+= 3 ) // going from 15 to 45 degrees, with 5 degree increments
		{
			cx = 320 + sinf( (fi+90.0f) * (1.0f / 57.296f) ) * 190;
			cy = 240 + cosf( (fi+90.0f) * (1.0f / 57.296f) ) * 190;

			CG_DrawRotatePic2( xOffset + cx, cy, 12, 24, 90 - fi, cgs.media.disruptorInsertTick );
		}

		if ( cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT )
		{
			trap_R_SetColor( colorTable[CT_WHITE] );

			// draw the charge level
			max = ( cg.gameTime - cg.predictedPlayerState.weaponChargeTime ) / ( 50.0f * 30.0f ); // bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.

			if ( max > 1.0f )
			{
				max = 1.0f;
			}

			CG_DrawPicExt(xOffset + 257, 435, 134 * max, 34, 0, 0, max, 1, cgs.media.disruptorChargeShader);
		}
//		trap_R_SetColor( colorTable[CT_WHITE] );
//		CG_DrawPic( 0, 0, 640, 480, cgs.media.disruptorMask );

	}
}


/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, const vec3_t origin, const vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;
	float			xScale, yScale;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	xScale = (float) cgs.glconfig.vidWidth / cgs.screenWidth;
	yScale = (float) cgs.glconfig.vidHeight / SCREEN_HEIGHT;
	refdef.x = x * xScale;
	refdef.y = y * yScale;
	refdef.width = w * xScale;
	refdef.height = h * yScale;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

#ifdef UNUSED
/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles )
{
	clientInfo_t	*ci;

	ci = &cgs.clientinfo[ clientNum ];

	CG_DrawPic( x, y, w, h, ci->modelIcon );

	// if they are deferred, draw a cross out
	if ( ci->deferred )
	{
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}
#endif

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, team_t team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5f * ( mins[2] + maxs[2] );
		origin[1] = 0.5f * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5f * ( maxs[2] - mins[2] );
		origin[0] = len * (1 / 0.268f);	// len / tan( fov/2 )

		angles[YAW] = 60 * sinf( cg.time * 0.0005f );

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawHUDLeftFrame1
================
*/
static void CG_DrawHUDLeftFrame1(float x, float y)
{
	// Inner gray wire frame
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDInnerLeft );
}

/*
================
CG_DrawHUDLeftFrame2
================
*/
static void CG_DrawHUDLeftFrame2(float x, float y)
{
	// Inner gray wire frame
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftFrame );		// Metal frame
}

#ifdef UNUSED
/*
================
DrawHealthArmor
================
*/
static void DrawHealthArmor(float x, float y)
{
	vec4_t calcColor;
	float	armorPercent,hold,healthPercent;
	playerState_t	*ps;

	int healthAmt;
	int armorAmt;

	ps = &cg.snap->ps;

	healthAmt = ps->stats[STAT_HEALTH];
	armorAmt = ps->stats[STAT_ARMOR];

	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}

	if (armorAmt > 100)
	{
		armorAmt = 100;
	}

	trap_R_SetColor( colorTable[CT_WHITE] );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftFrame );		// Circular black background

	//	Outer Armor circular
	Vector4Copy(colorTable[CT_GREEN], calcColor);

	hold = armorAmt-(ps->stats[STAT_MAX_HEALTH]/2);
	armorPercent = (float) hold/(ps->stats[STAT_MAX_HEALTH]/2);
	if (armorPercent <0)
	{
		armorPercent = 0;
	}
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor1 );

	// Inner Armor circular
	if (armorPercent>0)
	{
		armorPercent = 1;
	}
	else
	{
		armorPercent = (float) armorAmt/(ps->stats[STAT_MAX_HEALTH]/2);
	}
	Vector4Copy(colorTable[CT_GREEN], calcColor);
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor2 );			//	Inner Armor circular

	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armorPercent < 0.5f)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

	Vector4Copy(colorTable[CT_RED], calcColor);
	healthPercent = (float) healthAmt/ps->stats[STAT_MAX_HEALTH];
	calcColor[0] *= healthPercent;
	calcColor[1] *= healthPercent;
	calcColor[2] *= healthPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealth );

	// Make tic flash if health is at 20% of full
	if (healthPercent > 0.2f)
	{
		cg.HUDHealthFlag=qtrue;
	}
	else
	{
		if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
		{
			cg.HUDTickFlashTime = cg.time + 100;

			if (armorPercent > 0 && armorPercent < 0.5f)		// Keep the tics in sync if flashing
			{
				cg.HUDHealthFlag=cg.HUDArmorFlag;
			}
			else
			{
				if (cg.HUDHealthFlag)
				{
					cg.HUDHealthFlag = qfalse;
				}
				else
				{
					cg.HUDHealthFlag = qtrue;
				}
			}
		}
	}

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap_R_SetColor( colorTable[CT_RED] );
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealthTic );
	}

	if (cg.HUDArmorFlag)
	{
		trap_R_SetColor( colorTable[CT_GREEN] );
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmorTic );		//
	}

	trap_R_SetColor(hudTintColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeftStatic );		//

	trap_R_SetColor( colorTable[CT_RED] );
	CG_DrawNumField (x + 16, y + 40, 3, ps->stats[STAT_HEALTH], 14, 18,
		NUM_FONT_SMALL,qfalse);

	trap_R_SetColor( colorTable[CT_GREEN] );
	CG_DrawNumField (x + 18 + 14, y + 40 + 14, 3, ps->stats[STAT_ARMOR], 14, 18,
		NUM_FONT_SMALL,qfalse);

	trap_R_SetColor(hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDLeft );			// Metal frame
}
#endif // 0

/*
================
CG_DrawHealth
================
*/
static void CG_DrawHealth(float x, float y)
{
	vec4_t calcColor;
	float	healthPercent;
	playerState_t	*ps;
	int healthAmt;

	ps = &cg.snap->ps;

	healthAmt = ps->stats[STAT_HEALTH];

	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}

	Vector4Copy(colorTable[CT_HUD_RED], calcColor);
	healthPercent = (float) healthAmt/ps->stats[STAT_MAX_HEALTH];
	calcColor[0] *= healthPercent;
	calcColor[1] *= healthPercent;
	calcColor[2] *= healthPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealth );

	// Draw the ticks
	if (cg.HUDHealthFlag)
	{
		trap_R_SetColor( colorTable[CT_HUD_RED] );
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDHealthTic );
	}

	trap_R_SetColor( colorTable[CT_HUD_RED] );
	CG_DrawNumField (x + 16, y + 40, 3, ps->stats[STAT_HEALTH], 6, 12,
		NUM_FONT_SMALL,qfalse);

}

/*
================
CG_DrawArmor
================
*/
static void CG_DrawArmor(float x, float y)
{
	vec4_t			calcColor;
	float			armorPercent,hold;
	playerState_t	*ps;
	int				armor;

	ps = &cg.snap->ps;

	//	Outer Armor circular
	Vector4Copy(colorTable[CT_HUD_GREEN], calcColor);

	armor =ps->stats[STAT_ARMOR];

	if (armor> ps->stats[STAT_MAX_HEALTH])
	{
		armor = ps->stats[STAT_MAX_HEALTH];
	}

	hold = armor-(ps->stats[STAT_MAX_HEALTH]/2);
	armorPercent = (float) hold/(ps->stats[STAT_MAX_HEALTH]/2);
	if (armorPercent <0)
	{
		armorPercent = 0;
	}
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor1 );

	// Inner Armor circular
	if (armorPercent>0)
	{
		armorPercent = 1;
	}
	else
	{
		armorPercent = (float) ps->stats[STAT_ARMOR]/(ps->stats[STAT_MAX_HEALTH]/2);
	}
	Vector4Copy(colorTable[CT_HUD_GREEN], calcColor);
	calcColor[0] *= armorPercent;
	calcColor[1] *= armorPercent;
	calcColor[2] *= armorPercent;
	trap_R_SetColor( calcColor);
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmor2 );			//	Inner Armor circular

	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		// Make tic flash if inner armor is at 50% (25% of full armor)
		if (armorPercent < 0.5f)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 100;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

	if (cg.HUDArmorFlag)
	{
		trap_R_SetColor( colorTable[CT_HUD_GREEN] );
		CG_DrawPic(   x, y, 80, 80, cgs.media.HUDArmorTic );
	}

	trap_R_SetColor( colorTable[CT_HUD_GREEN] );
	CG_DrawNumField (x + 18 + 14, y + 40 + 14, 3, ps->stats[STAT_ARMOR], 6, 12,
		NUM_FONT_SMALL,qfalse);

}

/*
================
CG_DrawHUDRightFrame1
================
*/
void CG_DrawHUDRightFrame1(float x, float y)
{
	trap_R_SetColor( hudTintColor );
	// Inner gray wire frame
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDInnerRight );		//
}

/*
================
CG_DrawHUDRightFrame2
================
*/
void CG_DrawHUDRightFrame2(float x, float y)
{
	trap_R_SetColor( hudTintColor );
	CG_DrawPic(   x, y, 80, 80, cgs.media.HUDRightFrame );		// Metal frame
}

/*
================
CG_DrawAmmo
================
*/
static void CG_DrawAmmo(centity_t *cent, float x, float y)
{
	playerState_t	*ps;
	int			numColor_i;
	int			i;
	vec4_t		calcColor;
	float		value,inc,percent;

	ps = &cg.snap->ps;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon == WP_SABER )
	{
		trap_R_SetColor( colorTable[CT_WHITE] );
		// don't need to draw ammo, but we will draw the current saber style in this window
		switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
		{
		case FORCE_LEVEL_1:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle1 );
			break;
		case FORCE_LEVEL_2:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle2 );
			break;
		case FORCE_LEVEL_3:
			CG_DrawPic( x, y, 80, 40, cgs.media.HUDSaberStyle3 );
			break;
		default:
			break;
		}
		return;
	}
	else
	{
		value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	}

	if (value < 0)	// No ammo
	{
		return;
	}


	//
	// ammo
	//
/*	if (cg.oldammo < value)
	{
		cg.oldAmmoTime = cg.time + 200;
	}

	cg.oldammo = value;
*/
	// Firing or reloading?
/*	if (( pm->ps->weaponstate == WEAPON_FIRING
		&& cg.predictedPlayerState.weaponTime > 100 ))
	{
		numColor_i = CT_LTGREY;
	} */
	// Overcharged?
//	else if ( cent->gent->s.powerups & ( 1 << PW_WEAPON_OVERCHARGE ) )
//	{
//		numColor_i = CT_WHITE;
//	}
//	else
//	{
//		if ( value > 0 )
//		{
//			if (cg.oldAmmoTime > cg.time)
//			{
//				numColor_i = CT_YELLOW;
//			}
//			else
//			{
//				numColor_i = CT_HUD_ORANGE;
//			}
//		}
//		else
//		{
//			numColor_i = CT_RED;
//		}
//	}

	numColor_i = CT_HUD_ORANGE;

	trap_R_SetColor( colorTable[numColor_i] );
	if (value == INFINITE_AMMO)
		CG_DrawRotatePic2(x + 42, y + 33, 8, 16, -90, cgs.media.smallnumberShaders[8] );
	else
		CG_DrawNumField (x + 30, y + 26, 3, value, 6, 12, NUM_FONT_SMALL,qfalse);


//cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]

	inc = (float) ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_TICS;
	value =ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

	for (i=MAX_TICS-1;i>=0;i--)
	{

		if (value <= 0)	// partial tic
		{
			Vector4Copy(colorTable[CT_BLACK], calcColor);
		}
		else if (value < inc)	// partial tic
		{
			Vector4Copy(colorTable[CT_WHITE], calcColor);
			percent = value / inc;
			calcColor[0] *= percent;
			calcColor[1] *= percent;
			calcColor[2] *= percent;
		}
		else
		{
			Vector4Copy(colorTable[CT_WHITE], calcColor);
		}

		trap_R_SetColor( calcColor);
		CG_DrawPic( x + ammoTicPos[i].x,
			y + ammoTicPos[i].y,
			ammoTicPos[i].width,
			ammoTicPos[i].height,
			ammoTicPos[i].tic );

		value -= inc;
	}

}

/*
================
CG_DrawForcePower
================
*/
void CG_DrawForcePower(float x,float y)
{
	int			i;
	vec4_t		calcColor;
	float		value,inc,percent;

	inc = (float)  100 / MAX_TICS;
	value = cg.snap->ps.fd.forcePower;

	for (i=MAX_TICS-1;i>=0;i--)
	{

		if (value <= 0)	// partial tic
		{
			Vector4Copy(colorTable[CT_BLACK], calcColor);
		}
		else if (value < inc)	// partial tic
		{
			Vector4Copy(colorTable[CT_WHITE], calcColor);
			percent = value / inc;
			calcColor[0] *= percent;
			calcColor[1] *= percent;
			calcColor[2] *= percent;
		}
		else
		{
			Vector4Copy(colorTable[CT_WHITE], calcColor);
		}

		trap_R_SetColor( calcColor);
		CG_DrawPic( x + forceTicPos[i].x,
			y + forceTicPos[i].y,
			forceTicPos[i].width,
			forceTicPos[i].height,
			forceTicPos[i].tic );

		value -= inc;
	}
}

/*
================
CG_DrawHUD
================
*/
void CG_DrawHUD(centity_t	*cent)
{
#ifdef MISSIONPACK
	menuDef_t	*menuHUD = NULL;
#endif
	const char *scoreStr = NULL;

	if (cg_hudFiles.integer)
	{
		float x = 0;
		float y = SCREEN_HEIGHT-80;
		char ammoString[64];
		float weapX = x;

		UI_DrawProportionalString( x+16, y+40, va( "%i", cg.snap->ps.stats[STAT_HEALTH] ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_RED] );

		UI_DrawProportionalString( x+18+14, y+40+14, va( "%i", cg.snap->ps.stats[STAT_ARMOR] ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_GREEN] );

		if (cg.snap->ps.weapon == WP_SABER)
		{
			if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_3)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "STRONG");
				weapX += 16;
			}
			else if (cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_2)
			{
				Com_sprintf(ammoString, sizeof(ammoString), "MEDIUM");
				weapX += 16;
			}
			else
			{
				Com_sprintf(ammoString, sizeof(ammoString), "FAST");
			}
		}
		else
		{
			if (cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex] != INFINITE_AMMO)
				Com_sprintf(ammoString, sizeof(ammoString), "%i", cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex]);
			else
				Com_sprintf(ammoString, sizeof(ammoString), "INF");
		}

		UI_DrawProportionalString( cgs.screenWidth-(weapX+16+32), y+40, va( "%s", ammoString ),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_HUD_ORANGE] );

		UI_DrawProportionalString( cgs.screenWidth-(x+18+14+32), y+40+14, va( "%i", cg.snap->ps.fd.forcePower),
			UI_SMALLFONT|UI_DROPSHADOW, colorTable[CT_ICON_BLUE] );

		return;
	}

	if (GT_Team(cgs.gametype))
	{	// tint the hud items based on team
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
			hudTintColor = redhudtint;
		else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
			hudTintColor = bluehudtint;
		else // If we're not on a team for whatever reason, leave things as they are.
			hudTintColor = colorTable[CT_WHITE];
	}
	else
	{	// tint the hud items white (dont' tint)
		hudTintColor = colorTable[CT_WHITE];
	}
#ifdef MISSIONPACK
	menuHUD = Menus_FindByName("lefthud");
	if (menuHUD)
	{
		CG_DrawHUDLeftFrame1(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawArmor(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHealth(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHUDLeftFrame2(menuHUD->window.rect.x,menuHUD->window.rect.y);
	}
	else
#endif
	{ //Apparently we failed to get proper coordinates from the menu, so resort to manually inputting them.
		CG_DrawHUDLeftFrame1(0,SCREEN_HEIGHT-80);
		CG_DrawArmor(0,SCREEN_HEIGHT-80);
		CG_DrawHealth(0,SCREEN_HEIGHT-80);
		CG_DrawHUDLeftFrame2(0,SCREEN_HEIGHT-80);
	}

	//scoreStr = va("Score: %i", cgs.clientinfo[cg.snap->ps.clientNum].score);
	if ( cgs.gametype == GT_TOURNAMENT )
	{//A duel that requires more than one kill to knock the current enemy back to the queue
		//show current kills out of how many needed
		scoreStr = va("Score: %i/%i", cg.snap->ps.persistant[PERS_SCORE], cgs.fraglimit);
	}
#if 0
	else if (!GT_Team(cgs.gametype) )
	{	// This is a teamless mode, draw the score bias.
		int	scoreBias;
		char scoreBiasStr[16];

		scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores1;
		if (scoreBias == 0)
		{	// We are the leader!
			if (cgs.scores2 <= 0)
			{	// Nobody to be ahead of yet.
				Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), "");
			}
			else
			{
				scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores2;
				if (scoreBias == 0)
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (Tie)");
				}
				else
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (+%d)", scoreBias);
				}
			}
		}
		else // if (scoreBias < 0)
		{	// We are behind!
			Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (%d)", scoreBias);
		}
		scoreStr = va("Score: %i%s", cg.snap->ps.persistant[PERS_SCORE], scoreBiasStr);
	}
#endif
	else
	{	// Don't draw a bias.
		scoreStr = va("Score: %i", cg.snap->ps.persistant[PERS_SCORE]);
	}
	UI_DrawScaledProportionalString(cgs.screenWidth-101, SCREEN_HEIGHT-23, scoreStr, UI_RIGHT|UI_DROPSHADOW, colorTable[CT_WHITE], 0.7f);

	if (GT_Round(cgs.gametype) && cgs.round > 0) {
		if (cgs.gametype == GT_REDROVER) {
			scoreStr = va("Round: %i/%i", cgs.round, cgs.roundlimit);
		} else {
			scoreStr = va("Round Limit: %i", cgs.roundlimit);
		}
		UI_DrawScaledProportionalString(101, SCREEN_HEIGHT-23, scoreStr, UI_LEFT|UI_DROPSHADOW, colorTable[CT_WHITE], 0.7f);
	}
#ifdef MISSIONPACK
	menuHUD = Menus_FindByName("righthud");
	if (menuHUD)
	{
		CG_DrawHUDRightFrame1(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawForcePower(menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawAmmo(cent,menuHUD->window.rect.x,menuHUD->window.rect.y);
		CG_DrawHUDRightFrame2(menuHUD->window.rect.x,menuHUD->window.rect.y);

	}
	else
#endif
	{ //Apparently we failed to get proper coordinates from the menu, so resort to manually inputting them.
		CG_DrawHUDRightFrame1(cgs.screenWidth-80,SCREEN_HEIGHT-80);
		CG_DrawForcePower(cgs.screenWidth-80,SCREEN_HEIGHT-80);
		CG_DrawAmmo(cent,cgs.screenWidth-80,SCREEN_HEIGHT-80);
		CG_DrawHUDRightFrame2(cgs.screenWidth-80,SCREEN_HEIGHT-80);
	}
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

static qboolean ForcePower_Valid(forcePowers_t i)
{
	if (FP_Selectable(i) &&
		cg.snap->ps.fd.forcePowersKnown & (1 << i))
	{
		return qtrue;
	}

	return qfalse;
}

/*
===================
CG_DrawForceSelect
===================
*/
static void CG_DrawForceSelect( void )
{
	int		i;
	int		count;
	float	smallIconSize,bigIconSize;
	float	holdX,x,y,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.forceSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid((forcePowers_t)i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	// Max number of icons on the side
	sideMax = (cgs.screenWidth - 240 - bigIconSize) / (smallIconSize + pad) / 2;

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	x = 0.5f * cgs.screenWidth;
	y = 425;

	// Background
	// length = (sideLeftIconCnt * smallIconSize) + (sideLeftIconCnt*pad) +
	//		bigIconSize + (sideRightIconCnt * smallIconSize) + (sideRightIconCnt*pad) + 12;

	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS - 1;
	}

	trap_R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS - 1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] );
			holdX -= (smallIconSize+pad);
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{
		// Current Center Icon
		if (cgs.media.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)), bigIconSize, bigIconSize, cgs.media.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i >= MAX_SHOWPOWERS)
	{
		i = FP_FIRST;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i >= MAX_SHOWPOWERS)
		{
			i = FP_FIRST;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y, smallIconSize, smallIconSize, cgs.media.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad);
		}
	}

	if ( showPowersName[cg.forceSelect] )
	{
		UI_DrawProportionalString(0.5f * cgs.screenWidth, y + 30, CG_GetStripEdString("INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
static void CG_DrawInvenSelect( void )
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	float			smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	float			holdX,x,y,y2,pad;
//	int				height;
//	float			addX;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.invenSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.itemSelect == HI_NONE)
	{
		cg.itemSelect = (holdable_t)bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = HI_NONE + 1 ; i < HI_NUM_HOLDABLE ; i++ )
	{
		if (/*CG_InventorySelectable(i) && inv_icons[i]*/
			(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) )
		{
			count++;
		}
	}

	if (!count)
	{
		y2 = 0; //err?
		UI_DrawProportionalString(0.5f * cgs.screenWidth, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, colorTable[CT_ICON_BLUE]);
		return;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	// Max number of icons on the side
	sideMax = (cgs.screenWidth - 240 - bigIconSize) / (smallIconSize + pad) / 2;

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = (int)cg.itemSelect - 1;
	if (i <= HI_NONE)
	{
		i = HI_NUM_HOLDABLE-1;
	}

	x = 0.5f * cgs.screenWidth;
	y = 410;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
//	height = smallIconSize * cg.iconHUDPercent;
//	addX = smallIconSize * 0.75f;

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i <= HI_NONE)
		{
			i = HI_NUM_HOLDABLE-1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == (int)cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);
				*/

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
//	height = bigIconSize * cg.iconHUDPercent;
	if (cgs.media.invenIcons[cg.itemSelect])
	{
		int itemNdex;
		trap_R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, cgs.media.invenIcons[cg.itemSelect] );
//		addX = bigIconSize * 0.75f;
		trap_R_SetColor(colorTable[CT_ICON_BLUE]);
		/*CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12,
			NUM_FONT_SMALL,qfalse);*/

		itemNdex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[itemNdex].classname)
		{
			static const vec4_t	textColor = { .312f, .75f, .621f, 1.0f };
			char				text[1024];

			if ( trap_SP_GetStringTextString( va("INGAME_%s",bg_itemlist[itemNdex].classname), text, sizeof( text )))
			{
				UI_DrawProportionalString(0.5f * cgs.screenWidth, y+45, text, UI_CENTER | UI_SMALLFONT, textColor);
			}
			else
			{
				UI_DrawProportionalString(0.5f * cgs.screenWidth, y+45, bg_itemlist[itemNdex].classname, UI_CENTER | UI_SMALLFONT, textColor);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i > HI_NUM_HOLDABLE-1)
	{
		i = HI_NONE + 1;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
//	height = smallIconSize * cg.iconHUDPercent;
//	addX = smallIconSize * 0.75f;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i > HI_NUM_HOLDABLE)
		{
			i = HI_NONE + 1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == (int)cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (cgs.media.invenIcons[i])
		{
			trap_R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, cgs.media.invenIcons[i] );

			trap_R_SetColor(colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);*/

			holdX += (smallIconSize+pad);
		}
	}
}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void )
{
	centity_t		*cent;
/*	playerState_t	*ps;
	vec3_t			angles;
//	vec3_t		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
*/
	cent = &cg_entities[cg.snap->ps.clientNum];
/*	ps = &cg.snap->ps;

	VectorClear( angles );

	// Do start
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	cgi_UI_MenuPaintAll();*/

	CG_DrawHUD(cent);
	/*CG_DrawArmor(cent);
	CG_DrawHealth(cent);
	CG_DrawAmmo(cent);

	CG_DrawTalk(cent);*/
}


/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( float x, float y, float w, float h, float alpha, team_t team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = .2f;
		hcolor[2] = .2f;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = .2f;
		hcolor[1] = .2f;
		hcolor[2] = 1;
	} else {
		return;
	}
//	trap_R_SetColor( hcolor );

	CG_FillRect ( x, y, w, h, hcolor );
//	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard ( float y )
{
	char temp[MAX_QPATH];

	if ( !cg_drawScores.integer )
	{
		return y;
	}

	if ( GT_Team(cgs.gametype) )
	{
		strcpy ( temp, "Red: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		Q_strcat ( temp, MAX_QPATH, " Blue: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( temp, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
	}
	else
	{
		/*
		strcpy ( temp, "1st: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );

		Q_strcat ( temp, MAX_QPATH, " 2nd: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( temp, 0.7f, FONT_SMALL ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
		*/
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}


	return y;
}

/*
================
CG_DrawEnemyInfo
================
*/
static float CG_DrawEnemyInfo ( float y )
{
	float		size;
	int			clientNum;
	const char	*title;
	clientInfo_t	*ci;

	if ( !cg_drawEnemyInfo.integer )
	{
		return y;
	}

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
	{
		return y;
	}

	if ( cgs.gametype == GT_JEDIMASTER )
	{
		//title = "Jedi Master";
		title = CG_GetStripEdString("INGAMETEXT", "MASTERY7");
		clientNum = cgs.jediMaster;

		if ( clientNum < 0 )
		{
			//return y;
//			title = "Get Saber!";
			title = CG_GetStripEdString("INGAMETEXT", "GET_SABER");


			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic( cgs.screenWidth - size - 12, y, size, size, cgs.media.weaponIcons[WP_SABER] );

			y += size;

			/*
			CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
			y += 15;
			*/

			CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

			return y + BIGCHAR_HEIGHT + 2;
		}
	}
	else if ( cg.snap->ps.duelInProgress )
	{
//		title = "Dueling";
		title = CG_GetStripEdString("INGAMETEXT", "DUELING");
		clientNum = cg.snap->ps.duelIndex;
	}
	else if ( cgs.gametype == GT_TOURNAMENT && cg.snap->ps.pm_type != PM_SPECTATOR)
	{
//		title = "Dueling";
		title = CG_GetStripEdString("INGAMETEXT", "DUELING");
		if (cg.snap->ps.clientNum == cgs.duelist1)
		{
			clientNum = cgs.duelist2;
		}
		else if (cg.snap->ps.clientNum == cgs.duelist2)
		{
			clientNum = cgs.duelist1;
		}
		else
		{
			return y;
		}
	}
	else
	{
		/*
		title = "Attacker";
		clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];

		if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum )
		{
			return y;
		}

		if ( cg.time - cg.attackerTime > ATTACKER_HEAD_TIME )
		{
			cg.attackerTime = 0;
			return y;
		}
		*/
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
		{
			return y;
		}


		title = va("%s: %i",CG_GetStripEdString("INGAMETEXT", "LEADER"), cgs.scores1);

		/*
		if (cgs.scores1 == 1)
		{
			title = va("%i kill", cgs.scores1);
		}
		else
		{
			title = va("%i kills", cgs.scores1);
		}
		*/
		clientNum = cgs.duelWinner;
	}

	ci = &cgs.clientinfo[ clientNum ];

	if ( !ci )
	{
		return y;
	}

	size = ICON_SIZE * 1.25;
	y += 5;

	if ( ci->modelIcon )
	{
		CG_DrawPic( cgs.screenWidth - size - 5, y, size, size, ci->modelIcon );
	}

	y += size;

	CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );

	y += 15;
	CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, title, 0, 0, 0, FONT_MEDIUM );

	if ( cgs.gametype == GT_TOURNAMENT && cg.snap->ps.pm_type != PM_SPECTATOR)
	{//also print their score
		char text[1024];
		y += 15;
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit );
		CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width ( text, 0.7f, FONT_MEDIUM ), y, 0.7f, colorWhite, text, 0, 0, 0, FONT_MEDIUM );
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( cgs.screenWidth - 5 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

		CG_DrawBigString( cgs.screenWidth - 5 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	if (cg_drawTimer.integer >= 2) {
		if (!cgs.timelimit) {
			return y;
		}

		msec = cgs.timelimit * 60 * 1000;
		if (!cg.warmup) {
			msec -= cg.serverTime - cgs.levelStartTime;
			// intermission or overtime
			if (msec < 0) {
				msec = -msec;
			}
		}
	} else {
		msec = MAX(0, cg.serverTime - cgs.levelStartTime);
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( cgs.screenWidth - 5 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==============
CG_DrawCountdown
==============
*/
static void CG_DrawCountdown( void )
{
	char		*s;
	int			msec;

	if (cgs.gametype != GT_CLANARENA) {
		return;
	}
	if (!cgs.timelimit) {
		return;
	}
	if (cg.warmup) {
		return;
	}

	msec = cgs.timelimit * 60 * 1000;
	msec -= cg.serverTime - cgs.levelStartTime;

	if (msec < 0 || 16000 <= msec) {
		return;
	}

	msec = MAX(0, msec);

	s = va( "%d", msec / 1000 );
	UI_DrawProportionalString( 0.5f * cgs.screenWidth, 125, s, UI_CENTER, colorRed );
}

/*
==============
CG_DrawClock
==============
*/
static void CG_DrawClock( void )
{
	char	time[sizeof("00:00")];
	qtime_t	t;

	trap_RealTime( &t );
	if (t.tm_sec & 1)
		Com_sprintf( time, sizeof(time), "%d:%02d", t.tm_hour, t.tm_min );
	else
		Com_sprintf( time, sizeof(time), "%d %02d", t.tm_hour, t.tm_min );

	CG_Text_Paint( cgs.screenWidth - 10 - CG_Text_Width (time , 0.7f, FONT_SMALL ),
		310, 0.7f, colorWhite, time, 0, 0, 0, FONT_SMALL );
}

/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	float x, w, xx;
	int h;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	float ret_y;
	int count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == (team_t)cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = cgs.screenWidth - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == (team_t)cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j < PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( (powerup_t)j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
							cg_items[ ITEM_INDEX(item) ].icon );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


static void CG_DrawPowerupIcons(int y)
{
	int j;
	int ico_size = 64;
	float xAlign = cgs.screenWidth - ico_size * 1.1f;
	//int y = ico_size/2;
	gitem_t	*item;

	if (!cg.snap)
	{
		return;
	}

	y += 16;

	for (j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if (cg.snap->ps.powerups[j] > cg.gameTime)
		{
			int secondsleft = (cg.snap->ps.powerups[j] - cg.gameTime)/1000;

			item = BG_FindItemForPowerup( (powerup_t)j );

			if (item)
			{
				int icoShader = 0;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						icoShader = cgs.media.mpiRFlagYSShader;
					}
					else
					{
						icoShader = cgs.media.mpiBFlagYSShader;
					}
				}
				else
				{
					icoShader = cg_items[ ITEM_INDEX(item) ].icon;
				}

				CG_DrawPic( xAlign, y, ico_size, ico_size, icoShader );

				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					UI_DrawProportionalString(xAlign + (ico_size/2), y-8, va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW, colorTable[CT_WHITE]);
				}

				y += (ico_size/3);
			}
		}
	}
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

	if ( GT_Team(cgs.gametype) && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}

	y = CG_DrawEnemyInfo ( y );

	y = CG_DrawMiniScoreboard ( y );

	CG_DrawPowerupIcons(y);
}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) {
#ifdef JK2AWARDS
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.serverTime;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	// play sound only
	if ( !cg.rewardShader[0] ) {
		return;
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 0.5f * (cgs.screenWidth - ICON_SIZE);
		CG_DrawPic( x + 2, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( cgs.screenWidth - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) * 0.5f;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 0.5f * (cgs.screenWidth - count * ICON_SIZE);
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x + 2, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
#endif
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.serverTime - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	if (cg.mMapChange)
	{
		s = CG_GetStripEdString("INGAMETEXT", "SERVER_CHANGING_MAPS");	// s = "Server Changing Maps";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 0.5f * (cgs.screenWidth - w), 100, s, 1.0F);

		s = CG_GetStripEdString("INGAMETEXT", "PLEASE_WAIT");	// s = "Please wait...";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 0.5f * (cgs.screenWidth - w), 200, s, 1.0F);
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.serverTime ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStripEdString("INGAMETEXT", "CONNECTION_INTERRUPTED"); // s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 0.5f * (cgs.screenWidth - w), 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = cgs.screenWidth - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, cgs.media.connectionShader );
}

#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, i;
	float	x, y;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = cgs.screenWidth - 48;
	y = SCREEN_HEIGHT - 144;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );
	x -= 1.0f; //lines the actual graph up with the background

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			CG_DrawPicExt( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			CG_DrawPicExt( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			CG_DrawPicExt( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			CG_DrawPicExt( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

#define MAX_CP_WIDTH 480

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y ) {
	char	*s;
	char	*lastLine;
	char	*lastSpace;
	qboolean	inWord;
	int			words;

	if ( cg.centerPrintLock ) {
		return;
	}

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.serverTime;
	cg.centerPrintY = y;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;

	// Wrap long lines on word boundary
	lastLine = s;
	lastSpace = NULL;
	words = 0;
	inWord = qfalse;
	while( *s ) {
		char c = *s;

		if (c == ' ' || c == '\t') {
			*s = '\0';
			inWord = qfalse;

			if (CG_Text_Width(lastLine, 1.0f, FONT_MEDIUM) > MAX_CP_WIDTH) {
				if (lastSpace) {
					*s = c;
					*lastSpace = '\0';
					lastLine = lastSpace + 1;
					lastSpace = NULL;
				} else {	// word too wide, not my problem
					lastLine = s + 1;
				}

				cg.centerPrintLines++;
			} else {
				*s = c;
				lastSpace = s;
			}
		}
		else if (c == '\n')
		{
			*s = '\0';
			inWord = qfalse;
			lastSpace = NULL;
			lastLine = s + 1;
			cg.centerPrintLines++;
		}
		else if ( !inWord )
		{
			words++;
			inWord = qtrue;
		}

		s++;
	}

	{
		const float	wpm = 200.0f;	// words per minute

		cg.centerPrintMsec = 1000 * 60 * words / wpm;
		cg.centerPrintMsec *= cg_centertime.value / DEFAULT_CENTERTIME;

		if ( cg.centerPrintMsec < 1000 * cg_centertime.value ) {
			cg.centerPrintMsec = 1000 * cg_centertime.value;
		}
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	float	x, y, h;
	int		i;
	float	*color;
	const float scale = 1.0; //0.5

	color = CG_FadeColor( cg.centerPrintTime, cg.centerPrintMsec );
	if ( !color ) {
		cg.centerPrintTime = 0;
		cg.centerPrintLock = qfalse;
		return;
	}

	trap_R_SetColor( color );

	h = CG_Text_Height(NULL, scale, FONT_MEDIUM);
	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2 + h;
	h += 6;

	start = cg.centerPrint;

	for (i = cg.centerPrintLines; i > 0; i--) {
		x = 0.5f * (cgs.screenWidth - CG_Text_Width(start, scale, FONT_MEDIUM));
		CG_Text_Paint(x, y, scale, color, start, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += h;
		start += strlen(start) + 1;
	}

	trap_R_SetColor( NULL );
}


/*
==================
CG_PrintMotd_f
==================
*/
void CG_PrintMotd_f( void ) {
	const char *motd = CG_ConfigString( CS_INGAME_MOTD );

	if (strcmp(motd, "")) {
		cg.centerPrintLock = qfalse;
		CG_CenterPrint( motd, SCREEN_HEIGHT * 0.30f );
		cg.centerPrintLock = qtrue;
	}
}

/*
================================================================================

CROSSHAIR

================================================================================
*/

/*
=================
CG_DrawCrosshairArrows
=================
*/
static void CG_DrawCrosshairIndicators(float x, float y, float w, float h) {
	int crosshairIndicators;

	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		crosshairIndicators = cg_crosshairIndicatorsSpec.integer;
	} else {
		crosshairIndicators = cg_crosshairIndicators.integer;
	}

	if (crosshairIndicators & 1)
	{
		static const int	arrow[8][4] = {
			// { left, forward, right, back }
			{ 0, 1, 0, 0 },
			{ 1, 1, 0, 0 },
			{ 1, 0, 0, 0 },
			{ 1, 0, 0, 1 },
			{ 0, 0, 0, 1 },
			{ 0, 0, 1, 1 },
			{ 0, 0, 1, 0 },
			{ 0, 1, 1, 0 }
		};

		if (!(cg.snap->ps.pm_flags & PMF_STILL))
		{
			float arrowW = 0.5f * M_SQRT2 * w;
			float arrowH = 0.5f * M_SQRT2 * h;
			float arrowX = x - arrowW;
			float arrowY = y;
			int dir;
			int i;

			dir = cg.snap->ps.movementDir;

			if (dir < 0 || 8 <= dir) {
				assert(0);
				return;
			}

			for (i = 0; i < 4; i++) {
				if (arrow[cg.snap->ps.movementDir][i]) {
					CG_DrawRotatePic(arrowX, arrowY, arrowW, arrowH,
						45 + 90 * i, cgs.media.crosshairArrow);
				}
			}
		}
	}

	if (crosshairIndicators & 2)
	{
		static vec4_t color = { 1, 1, 1, 1 };
		const font_t font = FONT_SMALL;
		const float scale = 0.5f;
		vec_t *vec = cg.snap->ps.velocity;
		float velocity = sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
		const char *s = va("%d", (int)velocity);
		float textX = x - 0.5f * CG_Text_Width(s, scale, font);
		float textY = y + h + 5;

		color[3] = M_2_PI * atan2f(velocity, 300);

		CG_Text_Paint(textX, textY, scale, color, s, 0, 0, 0, font);
	}
}

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( vec3_t worldPoint, int chEntValid ) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;

	if ( !cg_drawCrosshair.integer )
	{
		return;
	}

	if (cg.snap->ps.fallingToDeath)
	{
		return;
	}

	if ( cg.predictedPlayerState.zoomMode != ZOOM_NONE )
	{//not while scoped
		return;
	}

	if (cg.spec.following && cg.spec.mode == SPECMODE_FREEANGLES)
	{
		return;
	}

	if ( cgs.crosshairColor[3] > 0.0f )
	{
		trap_R_SetColor( cgs.crosshairColor );
	}
	else if ( cg_crosshairHealth.integer )
	{
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	}
	else
	{
		//set color based on what kind of ent is under crosshair
		if ( cg.crosshairClientNum >= ENTITYNUM_WORLD )
		{
			trap_R_SetColor( NULL );
		}
		else if (chEntValid && (cg_entities[cg.crosshairClientNum].currentState.number < MAX_CLIENTS || cg_entities[cg.crosshairClientNum].currentState.shouldtarget))
		{
			vec4_t	ecolor = {0,0,0,0};
			centity_t *crossEnt = &cg_entities[cg.crosshairClientNum];

			if ( crossEnt->currentState.number < MAX_CLIENTS )
			{
				if (GT_Team(cgs.gametype) &&
					cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team )
				{
					//Allies are green
					ecolor[0] = 0;//R
					ecolor[1] = 1;//G
					ecolor[2] = 0;//B
				}
				else
				{
					//Enemies are red
					ecolor[0] = 1;//R
					ecolor[1] = 0;//G
					ecolor[2] = 0;//B
				}

				if (cg.snap->ps.duelInProgress)
				{
					if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
					{ //grey out crosshair for everyone but your foe if you're in a duel
						ecolor[0] = 0.4f;
						ecolor[1] = 0.4f;
						ecolor[2] = 0.4f;
					}
				}
				else if (crossEnt->currentState.bolt1)
				{ //this fellow is in a duel. We just checked if we were in a duel above, so
				  //this means we aren't and he is. Which of course means our crosshair greys out over him.
					ecolor[0] = 0.4f;
					ecolor[1] = 0.4f;
					ecolor[2] = 0.4f;
				}
			}
			else if (crossEnt->currentState.shouldtarget)
			{
				//VectorCopy( crossEnt->startRGBA, ecolor );
				if ( !ecolor[0] && !ecolor[1] && !ecolor[2] )
				{
					// We really don't want black, so set it to yellow
					ecolor[0] = 1.0F;//R
					ecolor[1] = 0.8F;//G
					ecolor[2] = 0.3F;//B
				}

				if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
					(GT_Team(cgs.gametype) &&
						(team_t)crossEnt->currentState.teamowner ==	cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 0;//R
					ecolor[1] = 1;//G
					ecolor[2] = 0;//B
				}
				else if (crossEnt->currentState.teamowner == 16 ||
					(GT_Team(cgs.gametype) && crossEnt->currentState.teamowner &&
						(team_t)crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor[0] = 1;//R
					ecolor[1] = 0;//G
					ecolor[2] = 0;//B
				}
				else if (crossEnt->currentState.eType == ET_GRAPPLE)
				{
					ecolor[0] = 1;//R
					ecolor[1] = 0;//G
					ecolor[2] = 0;//B
				}
			}

			ecolor[3] = 1;

			trap_R_SetColor( ecolor );
		}
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( worldPoint && VectorLength( worldPoint ) )
	{
		if ( !CG_WorldCoordToScreenCoord( worldPoint, &x, &y ) )
		{//off screen, don't draw it
			return;
		}
	}
	else
	{
		x = 0.5f * cgs.screenWidth + cg_crosshairX.value;
		y = 0.5f * SCREEN_HEIGHT + cg_crosshairY.value;
	}

	hShader = cgs.media.crosshairShader[ CLAMP( 0, NUM_CROSSHAIRS - 1, cg_drawCrosshair.integer ) ];

	x += cg.refdef.x;
	y += cg.refdef.y;

	CG_DrawPic( x - 0.5f * w, y - 0.5f * h, w, h, hShader );
	CG_DrawCrosshairIndicators( x, y, w, h );
}

qboolean CG_WorldCoordToScreenCoord(const vec3_t worldCoord, float *x, float *y)
{
	float	xcenter, ycenter;
	vec3_t	local, transformed;
	vec3_t	vfwd;
	vec3_t	vright;
	vec3_t	vup;
	float xzi;
	float yzi;

//	xcenter = cg.refdef.width / 2;//gives screen coords adjusted for resolution
//	ycenter = cg.refdef.height / 2;//gives screen coords adjusted for resolution

	//NOTE: did it this way because most draw functions expect virtual 640x480 coords
	//	and adjust them for current resolution
	xcenter = 0.5f * cgs.screenWidth;
	ycenter = 0.5f * SCREEN_HEIGHT;

	AngleVectors (cg.refdefViewAngles, vfwd, vright, vup);

	VectorSubtract (worldCoord, cg.refdef.vieworg, local);

	transformed[0] = DotProduct(local,vright);
	transformed[1] = DotProduct(local,vup);
	transformed[2] = DotProduct(local,vfwd);

	// Make sure Z is not negative.
	if(transformed[2] < 0.01f)
	{
		return qfalse;
	}

	xzi = xcenter / transformed[2] * (90.0f / cg.refdef.fov_x);
	yzi = ycenter / transformed[2] * (90.0f / cg.refdef.fov_y);

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return qtrue;
}

/*
====================
CG_SaberClashFlare
====================
*/
void CG_SaberClashFlare( void )
{
	int				t, maxTime = 150;
	vec3_t dif;
	float x,y;
	float v, len;
	trace_t tr;

	t = cg.time - cg.saberFlashTime;

	if ( t <= 0 || t >= maxTime )
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract( cg.saberFlashPos, cg.refdef.vieworg, dif );

	if ( DotProduct( dif, cg.refdef.viewaxis[0] ) < 0.2f )
	{
		return;
	}

	CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cg.saberFlashPos, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f )
	{
		return;
	}

	len = VectorNormalize( dif );

	// clamp to a known range
	if ( len > 800 )
	{
		len = 800;
	}

	v = ( 1.0f - ((float)t / maxTime )) * ((1.0f - ( len / 800.0f )) * 2.0f + 0.35f);

	if ( CG_WorldCoordToScreenCoord( cg.saberFlashPos, &x, &y ) ) {
		static const vec4_t color = { 0.8f, 0.8f, 0.8f, 1.0f };

		trap_R_SetColor( color );

		CG_DrawPic( x - ( v * 300 ), y - ( v * 300 ),
			v * 600, v * 600, cgs.media.saberFlareShader );
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = 10;
	int starty = 10;//SCREEN_HEIGHT - icon_size*3;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode != ZOOM_NONE)
	{ //don't display over zoom mask
		return;
	}

	if (cg.snap->ps.pm_type == PM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.holocronBits & (1 << forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			starty += (icon_size+2); //+2 for spacing
			if ((starty+icon_size) >= SCREEN_HEIGHT-80)
			{
				starty = 10;//SCREEN_HEIGHT - icon_size*3;
				startx += (icon_size+2);
			}
		}

		i++;
	}
}

static qboolean CG_IsDurationPower(forcePowers_t power)
{
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}

	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	float startx = icon_size*2+16;
	float starty = SCREEN_HEIGHT - icon_size*2;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode != ZOOM_NONE)
	{ //don't display over zoom mask
		return;
	}

	if (cg.snap->ps.pm_type == PM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if ((cg.snap->ps.fd.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, cgs.media.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2); //+2 for spacing
			if ((startx+icon_size) >= cgs.screenWidth-80)
			{
				startx = icon_size*2+16;
				starty += (icon_size+2);
			}
		}

		i++;
	}

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.fd.forceRageRecoveryTime > cg.gameTime)
	{
		CG_DrawPic( startx, starty, endx, endy, cgs.media.rageRecShader);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	float	cx, cy;
	vec3_t	org;
	static	int lastvalidlockdif = 0;
	static	int oldDif = 0;
	centity_t *cent = &cg_entities[lockEntNum];
	vec4_t color={0.0f,0.0f,0.0f,0.0f};
	int dif = ( cg.gameTime - cg.snap->ps.rocketLockTime ) / ( 1200 / /*8*/16 );
	int i;

	if (!cg.snap->ps.rocketLockTime)
	{
		return;
	}

	if (cg.snap->ps.pm_type == PM_SPECTATOR)
	{
		return;
	}

	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
	{
		if (cgs.clientinfo[cg.snap->ps.rocketLockIndex].team == cgs.clientinfo[cg.snap->ps.clientNum].team)
		{
			if (GT_Team(cgs.gametype))
			{
				return;
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1)
	{
		lastvalidlockdif = dif;
	}
	else
	{
		dif = lastvalidlockdif;
	}

	if ( !cent )
	{
		return;
	}

	VectorCopy( cent->lerpOrigin, org );

	if ( CG_WorldCoordToScreenCoord( org, &cx, &cy ))
	{
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( cent->lerpOrigin, cg.refdef.vieworg ) / 1024.0f;

		if ( sz > 1.0f )
		{
			sz = 1.0f;
		}
		else if ( sz < 0.0f )
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		cy += sz * 0.5f;

		if ( dif < 0 )
		{
			oldDif = 0;
			return;
		}
		else if ( dif > 8 )
		{
			dif = 8;
		}

		// do sounds
		if ( oldDif != dif )
		{
			if ( dif == 8 )
			{
				trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
			}
			else
			{
				trap_S_StartSound( org, 0, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
			}
		}

		oldDif = dif;

		for ( i = 0; i < dif; i++ )
		{
			color[0] = 1.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 0.1f * i + 0.2f;

			trap_R_SetColor( color );

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, trap_R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if ( dif == 8 )
		{
			color[0] = color[1] = color[2] = sinf( cg.time * 0.05f ) * 0.5f + 0.5f;
			color[3] = 1.0f; // this art is additive, so the alpha value does nothing

			trap_R_SetColor( color );

			CG_DrawPic( cx - sz, cy - sz * 2, sz * 2, sz * 2, trap_R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	if ( cg_dynamicCrosshair.integer )
	{
		vec3_t d_f, d_rt, d_up;
		/*
		if ( cg.snap->ps.weapon == WP_NONE ||
			cg.snap->ps.weapon == WP_SABER ||
			cg.snap->ps.weapon == WP_STUN_BATON)
		{
			VectorCopy( cg.refdef.vieworg, start );
			AngleVectors( cg.refdefViewAngles, d_f, d_rt, d_up );
		}
		else
		*/
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vec3_t pitchConstraint;

				VectorCopy(cg.refdefViewAngles, pitchConstraint);

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdefViewAngles, pitchConstraint);
				}

				if (pitchConstraint[PITCH] > 40)
				{
					pitchConstraint[PITCH] = 40;
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			else
			{
				vec3_t pitchConstraint;

				if (cg.renderingThirdPerson)
				{
					VectorCopy(cg.predictedPlayerState.viewangles, pitchConstraint);
				}
				else
				{
					VectorCopy(cg.refdefViewAngles, pitchConstraint);
				}

				AngleVectors( pitchConstraint, d_f, d_rt, d_up );
			}
			CG_CalcMuzzlePoint(cg.snap->ps.clientNum, start);
		}

		//FIXME: increase this?  Increase when zoom in?
		VectorMA( start, 4096, d_f, end );//was 8192
	}
	else
	{
		VectorCopy( cg.refdef.vieworg, start );
		VectorMA( start, 131072, cg.refdef.viewaxis[0], end );
	}

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );

	if (trace.entityNum < MAX_CLIENTS)
	{
		if (CG_IsMindTricked(cg_entities[trace.entityNum].currentState.trickedentindex,
			cg_entities[trace.entityNum].currentState.trickedentindex2,
			cg_entities[trace.entityNum].currentState.trickedentindex3,
			cg_entities[trace.entityNum].currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			if (cg.crosshairClientNum == trace.entityNum)
			{
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}

			CG_DrawCrosshair(trace.endpos, 0);

			return; //this entity is mind-tricking the current client, so don't render it
		}
	}

	if (cg.snap->ps.pm_type != PM_SPECTATOR)
	{
		if (trace.entityNum < /*MAX_CLIENTS*/ENTITYNUM_WORLD)
		{
			CG_DrawCrosshair(trace.endpos, 1);
		}
		else
		{
			CG_DrawCrosshair(trace.endpos, 0);
		}
	}

//	if ( trace.entityNum >= MAX_CLIENTS ) {
//		return;
//	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	if ( trace.entityNum >= MAX_CLIENTS ) {
		cg.crosshairClientNum = trace.entityNum;
		cg.crosshairClientTime = cg.serverTime;
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.serverTime;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	vec4_t		tcolor;
	const char	*name;
	ct_table_t	baseColor;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	//rww - still do the trace, our dynamic crosshair depends on it

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;

	if (GT_Team(cgs.gametype))
	{
		if (cgs.clientinfo[cg.crosshairClientNum].team == TEAM_RED)
		{
			baseColor = CT_RED;
		}
		else
		{
			baseColor = CT_BLUE;
		}

		/*
		//For now instead of team-based we'll make it oriented based on which team we're on
		if (cgs.clientinfo[cg.crosshairClientNum].team == cgs.clientinfo[cg.snap->ps.clientNum].team)
		{
			baseColor = CT_GREEN;
		}
		else
		{
			baseColor = CT_RED;
		}
		*/
	}
	else
	{
		//baseColor = CT_WHITE;
		baseColor = CT_RED; //just make it red in nonteam modes since everyone is hostile and crosshair will be red on them too
	}

	if (cg.snap->ps.duelInProgress)
	{
		if (cg.crosshairClientNum != cg.snap->ps.duelIndex)
		{ //grey out crosshair for everyone but your foe if you're in a duel
			baseColor = CT_BLACK;
		}
	}
	else if (cg_entities[cg.crosshairClientNum].currentState.bolt1)
	{ //this fellow is in a duel. We just checked if we were in a duel above, so
	  //this means we aren't and he is. Which of course means our crosshair greys out over him.
		baseColor = CT_BLACK;
	}

	tcolor[0] = colorTable[baseColor][0];
	tcolor[1] = colorTable[baseColor][1];
	tcolor[2] = colorTable[baseColor][2];
	tcolor[3] = color[3]*0.5f;

	UI_DrawProportionalString(0.5f * cgs.screenWidth, 170, name, UI_CENTER, tcolor);

	// draw "press fire to follow" target hint
	if (cg_drawSpectatorHints.integer && cg.snap->ps.pm_type == PM_SPECTATOR && !cg.demoPlayback) {
		color[3] *= 0.4f;
		UI_DrawScaledProportionalString(0.5f * cgs.screenWidth, 195, CG_GetStripEdString("SABERINGAME", "CROSSHAIR_FOLLOW_HINT"), UI_CENTER, color, 0.6f);
	}

	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void)
{
	const char* s;

	if (cgs.gametype == GT_TOURNAMENT &&
		cgs.duelist1 != -1 &&
		cgs.duelist2 != -1)
	{
		char text[1024];
		int size = 64;

		Com_sprintf(text, sizeof(text), "%s" S_COLOR_WHITE " %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStripEdString("INGAMETEXT", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
		UI_DrawProportionalString( 0.5f * cgs.screenWidth, 420, text, UI_CENTER, colorWhite );

		trap_R_SetColor( colorTable[CT_WHITE] );
		if ( cgs.clientinfo[cgs.duelist1].modelIcon )
		{
			CG_DrawPic( 10, SCREEN_HEIGHT-(size*1.5f), size, size, cgs.clientinfo[cgs.duelist1].modelIcon );
		}
		if ( cgs.clientinfo[cgs.duelist2].modelIcon )
		{
			CG_DrawPic( cgs.screenWidth-size-10, SCREEN_HEIGHT-(size*1.5f), size, size, cgs.clientinfo[cgs.duelist2].modelIcon );
		}
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit );
		UI_DrawProportionalString( 42, SCREEN_HEIGHT - (size * 1.5f) + 64, text, UI_CENTER, colorWhite );

		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit );
		UI_DrawProportionalString( cgs.screenWidth - size + 22, SCREEN_HEIGHT - (size * 1.5) + 64, text, UI_CENTER, colorWhite );
	}
	else
	{
		s = CG_GetStripEdString("INGAMETEXT", "SPECTATOR");			// "SPECTATOR"
		UI_DrawProportionalString( 0.5f * cgs.screenWidth, 420, s, UI_CENTER, colorWhite );
	}

	if ( cgs.gametype == GT_TOURNAMENT )
	{
		s = CG_GetStripEdString("INGAMETEXT", "WAITING_TO_PLAY");	// "waiting to play";
		UI_DrawProportionalString( 0.5f * cgs.screenWidth, 440, s, UI_CENTER, colorWhite );
	}
	else //if ( GT_Team(cgs.gametype) )
	{
		//s = "press ESC and use the JOIN menu to play";
		s = CG_GetStripEdString("INGAMETEXT", "SPEC_CHOOSEJOIN");
		UI_DrawProportionalString( 0.5f * cgs.screenWidth, 440, s, UI_CENTER, colorWhite );
	}
}

/*
=================
CG_GetHTTPDownloads

Copy of UI_GetHTTPDownloads
=================
*/
static qboolean CG_GetHTTPDownloads( void ) {
	const char	*info;
	const char	*serverJK2MV;
	const char	*mv_httpdownloads;
	char		clientJK2MV[2];
	char		mv_allowdownload[2];

	info = CG_ConfigString( CS_SERVERINFO );
	serverJK2MV = Info_ValueForKey( info, "JK2MV" );
	mv_httpdownloads = Info_ValueForKey( info, "mv_httpdownloads" );
	trap_Cvar_VariableStringBuffer( "JK2MV", clientJK2MV, sizeof(clientJK2MV) );
	trap_Cvar_VariableStringBuffer( "mv_allowdownload", mv_allowdownload, sizeof(mv_allowdownload) );

	if (clientJK2MV[0] != '\0' && atoi( mv_allowdownload ) &&
		serverJK2MV[0] != '\0' && atoi( mv_httpdownloads ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	const char	*s;
	int		sec;
	float	offset;
	char sYes[20];
	char sNo[20];

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
//		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.serverTime - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	trap_SP_GetStringTextString("MENUS0_YES", sYes, sizeof(sYes) );
	trap_SP_GetStringTextString("MENUS0_NO",  sNo,  sizeof(sNo) );

	s = va( "VOTE(%i):%s" S_COLOR_WHITE, sec, cgs.voteString );
	CG_DrawSmallString( 4, 58, s, 1.0F );
	offset = SMALLCHAR_WIDTH * ( CG_DrawStrlen( s ) );

	if ( cgs.voteMapMissing ) {
		qhandle_t	icon = CG_GetHTTPDownloads() ? cgs.media.download : cgs.media.missing;

		offset += SMALLCHAR_WIDTH;
		CG_DrawPic( 4 + offset, 58, SMALLCHAR_HEIGHT, SMALLCHAR_HEIGHT, icon );
		offset += SMALLCHAR_HEIGHT;
	}

	s = va( " %s:%i %s:%i", sYes, cgs.voteYes, sNo, cgs.voteNo);
	CG_DrawSmallString( 4 + offset, 58, s, 1.0F );

	s = CG_GetStripEdString("INGAMETEXT", "OR_PRESS_ESC_THEN_CLICK_VOTE");	//	s = "or press ESC then click Vote";
	CG_DrawSmallString( 4, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo->team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo->team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
//		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.serverTime - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int voteIndex = 0;
			char voteIndexStr[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				voteIndexStr[voteIndex] = cgs.teamVoteString[cs_offset][i];
				voteIndex++;
				i++;
			}
			voteIndexStr[voteIndex] = 0;

			voteIndex = atoi(voteIndexStr);

			s = va("TEAMVOTE(%i):(Make %s" S_COLOR_WHITE " the new team leader) yes:%i no:%i", sec, cgs.clientinfo[voteIndex].name,
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
		else
		{
			s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
								cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	}
	CG_DrawSmallString( 4, 90, s, 1.0F );
}

static qboolean CG_DrawScoreboard() {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}


	if (menuScoreboard == NULL) {
		if ( GT_Team(cgs.gametype) ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else // MISSIONPACK
	return CG_DrawOldScoreboard();
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.serverTime;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void )
{
	const char	*s;
	float		x;

	if ( !cg_drawFollow.integer )
	{
		return qfalse;
	}

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) )
	{
		return qfalse;
	}

//	s = "following";
	s = CG_GetStripEdString("INGAMETEXT", "FOLLOWING");
	x = 0.5f * (cgs.screenWidth - CG_Text_Width(s, 1.0f, FONT_MEDIUM));
	CG_Text_Paint(x , 60, 1.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);

	s = cgs.clientinfo[ cg.snap->ps.clientNum ].name;
	x = 0.5f * (cgs.screenWidth - CG_Text_Width(s, 2.0f, FONT_MEDIUM));
	CG_Text_Paint (x, 80, 2.0f, colorWhite, s, 0, 0, 0, FONT_MEDIUM);

	return qtrue;
}

static void CG_DrawScoreboardHints( void )
{
	const char	*s;
	float		x;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return;
	}

	if ( !cg_drawSpectatorHints.integer ) {
		return;
	}

	if ( cg.demoPlayback ) {
		return;
	}

	x = 0.5f * cgs.screenWidth;

	if ( cg.snap->ps.duelInProgress )
	{
		float	width;
		float	scale;

		s = va("%s" S_COLOR_WHITE " %s %s",
			cgs.clientinfo[cg.snap->ps.clientNum].name,
			CG_GetStripEdString("INGAMETEXT", "SPECHUD_VERSUS"),
			cgs.clientinfo[cg.snap->ps.duelIndex].name);
		width = CG_Text_Width(s, 1.0f, FONT_MEDIUM);
		// never overlap health and ammo frames
		scale = cgs.screenWidth - 2 * 80 - 20;
		scale /= width;
		if ( scale > 1.0f ) {
			scale = 1.0f;
		}
		UI_DrawScaledProportionalString(x, 410, s, UI_CENTER, colorWhite, scale);

		s = CG_GetStripEdString("SABERINGAME", "DUEL_FOLLOW_HINT");
		UI_DrawProportionalString(x, 440, s, UI_CENTER, colorWhite);
	}
	else if ( GT_Team(cgs.gametype) )
	{
		s = va(CG_GetStripEdString("SABERINGAME", "TFFA_FOLLOW_HINT"),
			BG_TeamName(cg.snap->ps.persistant[PERS_TEAM], CASE_LOWER));
		UI_DrawProportionalString(x, 440, s, UI_CENTER, colorWhite);
	}
	else if ( cgs.gametype != GT_TOURNAMENT )
	{
		s = CG_GetStripEdString("SABERINGAME", "FFA_FOLLOW_HINT");
		UI_DrawProportionalString(x, 440, s, UI_CENTER, colorWhite);
	}
}

#ifdef UNUSED
static void CG_DrawTemporaryStats()
{ //placeholder for testing (draws ammo and force power)
	char s[512];

	if (!cg.snap)
	{
		return;
	}

	sprintf(s, "Force: %i", cg.snap->ps.fd.forcePower);

	CG_DrawBigString(cgs.screenWidth-164, SCREEN_HEIGHT-128, s, 1.0f);

	sprintf(s, "Ammo: %i", cg.snap->ps.ammo[weaponData[cg.snap->ps.weapon].ammoIndex]);

	CG_DrawBigString(cgs.screenWidth-164, SCREEN_HEIGHT-112, s, 1.0f);

	sprintf(s, "Health: %i", cg.snap->ps.stats[STAT_HEALTH]);

	CG_DrawBigString(8, SCREEN_HEIGHT-128, s, 1.0f);

	sprintf(s, "Armor: %i", cg.snap->ps.stats[STAT_ARMOR]);

	CG_DrawBigString(8, SCREEN_HEIGHT-112, s, 1.0f);
}
#endif

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
#if 0
	const char	*s;
	int			w;

	if (!cg_drawStatus.integer)
	{
		return;
	}

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(0.5f * (cgs.screenWidth - w), 64, s, 1.0F);
#endif
}



/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	float		w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	const char	*s;

	if (cgs.unpauseTime > cg.serverTime)
	{
		sec = (cgs.unpauseTime - cg.serverTime) / 1000;

		if (sec < 60) {
			s = va(CG_GetStripEdString("SABERINGAME", "MATCH_WILL_RESUME"), sec); // "Game will resume in %d seconds"
		} else {
			s = CG_GetStripEdString("SABERINGAME", "MATCH_PAUSED");
		}

		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(0.5f * (cgs.screenWidth - w), 24, s, 1.0F);
		return;
	}

	sec = cg.warmup;

	if (!sec) {
		return;
	}

	if (sec < 0) {
		if ((cg.snap->ps.pm_flags & PMF_FOLLOW) ||
			(cg.snap->ps.pm_type == PM_SPECTATOR) ||
			(cgs.readyClients & (1 << cg.snap->ps.clientNum)))
		{
//			s = "Waiting for players";
			s = CG_GetStripEdString("INGAMETEXT", "WAITING_FOR_PLAYERS");
		}
		else
		{
			char	bind[16];

			trap_Cvar_VariableStringBuffer("ui_bind_ready", bind, sizeof(bind));

			if (bind[0]) {
				s = va(CG_GetStripEdString("SABERINGAME", "PRESS_TO_START"), bind);
			} else {
				s = CG_GetStripEdString("SABERINGAME", "TYPE_READY");
			}
		}

		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(0.5f * (cgs.screenWidth - w), 24, s, 1.0F);
		cg.warmupCount = 0;

		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s" S_COLOR_WHITE " vs %s", ci1->name, ci2->name );
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(0.5f * (cgs.screenWidth - w), 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
		}
	} else {
		if ( GT_Round(cgs.gametype) && cgs.round > 0 ) {
			s = va("Round %i", cgs.round);
		} else {
			s = gametypeLong[cgs.gametype];
		}

		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(0.5f * (cgs.screenWidth - w), 90, 1.5f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
	}

	sec = ( sec - cg.gameTime );
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
		return;
	}
	sec /= 1000;
//	s = va( "Starts in: %i", sec + 1 );
	s = va( "%s: %i",CG_GetStripEdString("INGAMETEXT", "STARTS_IN"), sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		scale = 1.25f;
		break;
	case 1:
		scale = 1.15f;
		break;
	case 2:
		scale = 1.05f;
		break;
	default:
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(0.5f * (cgs.screenWidth - w), 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
#ifdef MISSIONPACK
/*
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif // MISSIONPACK

qboolean CG_OtherTeamHasFlag(void) {
	if (GT_Flag(cgs.gametype)) {
		team_t team = (team_t)cg.snap->ps.persistant[PERS_TEAM];
		if (team == TEAM_RED && cgs.redflag == FLAG_TAKEN) {
			return qtrue;
		} else if (team == TEAM_BLUE && cgs.blueflag == FLAG_TAKEN) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
	return qfalse;
}

qboolean CG_YourTeamHasFlag(void) {
	if (GT_Flag(cgs.gametype)) {
		team_t team = (team_t)cg.snap->ps.persistant[PERS_TEAM];
		if (team == TEAM_RED && cgs.blueflag == FLAG_TAKEN) {
			return qtrue;
		} else if (team == TEAM_BLUE && cgs.redflag == FLAG_TAKEN) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
	return qfalse;
}

void CG_DrawFlagStatus()
{
	int myFlagTakenShader = 0;
	int theirFlagShader = 0;
	team_t team;
	float startDrawPos = 2;
	float ico_size = 32;

	if (!cg.snap)
	{
		return;
	}

	if (!GT_Flag(cgs.gametype))
	{
		return;
	}

	team = (team_t)cg.snap->ps.persistant[PERS_TEAM];

	if (cgs.gametype == GT_CTY)
	{
		if (team == TEAM_RED)
		{
			myFlagTakenShader = cgs.media.mpiRFlagXShader;
			theirFlagShader = cgs.media.mpiBFlagYSShader;
		}
		else
		{
			myFlagTakenShader = cgs.media.mpiBFlagXShader;
			theirFlagShader = cgs.media.mpiRFlagYSShader;
		}
	}
	else
	{
		if (team == TEAM_RED)
		{
			myFlagTakenShader = cgs.media.mpiRFlagXShader;
			theirFlagShader = cgs.media.mpiBFlagShader;
		}
		else
		{
			myFlagTakenShader = cgs.media.mpiBFlagXShader;
			theirFlagShader = cgs.media.mpiRFlagShader;
		}
	}

	if (CG_YourTeamHasFlag())
	{
		CG_DrawPic( startDrawPos, 365, ico_size, ico_size, theirFlagShader );
		startDrawPos += ico_size+2;
	}

	if (CG_OtherTeamHasFlag())
	{
		CG_DrawPic( startDrawPos, 365, ico_size, ico_size, myFlagTakenShader );
	}
}

/*
=================
CG_DrawForceEffects
=================
*/
static void CG_DrawForceEffects( void ) {
	static int cgRageTime = 0;
	static int cgRageFadeTime = 0;
	static float cgRageFadeVal = 0;

	static int cgRageRecTime = 0;
	static int cgRageRecFadeTime = 0;
	static float cgRageRecFadeVal = 0;

	static int cgAbsorbTime = 0;
	static int cgAbsorbFadeTime = 0;
	static float cgAbsorbFadeVal = 0;

	static int cgProtectTime = 0;
	static int cgProtectFadeTime = 0;
	static float cgProtectFadeVal = 0;

	static int cgYsalTime = 0;
	static int cgYsalFadeTime = 0;
	static float cgYsalFadeVal = 0;

	vec4_t	hcolor;
	float	rageTime, rageRecTime, absorbTime, protectTime, ysalTime;

	if (cg.snap->ps.pm_type == PM_SPECTATOR)
	{
		cgRageTime = 0;
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;

		cgRageRecTime = 0;
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;

		cgAbsorbTime = 0;
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;

		cgProtectTime = 0;
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;

		cgYsalTime = 0;
		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;

		return;
	}

	if (cg.snap->ps.fd.forcePowersActive & (1 << FP_RAGE))
	{
		if (!cgRageTime)
		{
			cgRageTime = cg.time;
		}

		rageTime = (float)(cg.time - cgRageTime);

		rageTime /= 9000.0f;

		if (rageTime < 0)
		{
			rageTime = 0;
		}
		if (rageTime > 0.15f)
		{
			rageTime = 0.15f;
		}

		hcolor[3] = rageTime;
		hcolor[0] = 0.7f;
		hcolor[1] = 0;
		hcolor[2] = 0;

		if (!cg.renderingThirdPerson)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}

		cgRageFadeTime = 0;
		cgRageFadeVal = 0;
	}
	else if (cgRageTime)
	{
		if (!cgRageFadeTime)
		{
			cgRageFadeTime = cg.time;
			cgRageFadeVal = 0.15f;
		}

		rageTime = cgRageFadeVal;

		cgRageFadeVal -= (cg.time - cgRageFadeTime)*0.000005f;

		if (rageTime < 0)
		{
			rageTime = 0;
		}
		if (rageTime > 0.15f)
		{
			rageTime = 0.15f;
		}

		if (cg.snap->ps.fd.forceRageRecoveryTime > cg.gameTime)
		{
			float checkRageRecTime = rageTime;

			if (checkRageRecTime < 0.15f)
			{
				checkRageRecTime = 0.15f;
			}

			hcolor[3] = checkRageRecTime;
			hcolor[0] = rageTime*4;
			if (hcolor[0] < 0.2f)
			{
				hcolor[0] = 0.2f;
			}
			hcolor[1] = 0.2f;
			hcolor[2] = 0.2f;
		}
		else
		{
			hcolor[3] = rageTime;
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		}

		if (!cg.renderingThirdPerson && rageTime)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}
		else
		{
			if (cg.snap->ps.fd.forceRageRecoveryTime > cg.gameTime)
			{
				hcolor[3] = 0.15f;
				hcolor[0] = 0.2f;
				hcolor[1] = 0.2f;
				hcolor[2] = 0.2f;
				CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
			}
			cgRageTime = 0;
		}
	}
	else if (cg.snap->ps.fd.forceRageRecoveryTime > cg.gameTime)
	{
		if (!cgRageRecTime)
		{
			cgRageRecTime = cg.time;
		}

		rageRecTime = (float)(cg.time - cgRageRecTime);

		rageRecTime /= 9000;

		if (rageRecTime < 0.15f)//0)
		{
			rageRecTime = 0.15f;//0;
		}
		if (rageRecTime > 0.15f)
		{
			rageRecTime = 0.15f;
		}

		hcolor[3] = rageRecTime;
		hcolor[0] = 0.2f;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.2f;

		if (!cg.renderingThirdPerson)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}

		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;
	}
	else if (cgRageRecTime)
	{
		if (!cgRageRecFadeTime)
		{
			cgRageRecFadeTime = cg.time;
			cgRageRecFadeVal = 0.15f;
		}

		rageRecTime = cgRageRecFadeVal;

		cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime)*0.000005f;

		if (rageRecTime < 0)
		{
			rageRecTime = 0;
		}
		if (rageRecTime > 0.15f)
		{
			rageRecTime = 0.15f;
		}

		hcolor[3] = rageRecTime;
		hcolor[0] = 0.2f;
		hcolor[1] = 0.2f;
		hcolor[2] = 0.2f;

		if (!cg.renderingThirdPerson && rageRecTime)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}
		else
		{
			cgRageRecTime = 0;
		}
	}

	if (cg.snap->ps.fd.forcePowersActive & (1 << FP_ABSORB))
	{
		if (!cgAbsorbTime)
		{
			cgAbsorbTime = cg.time;
		}

		absorbTime = (float)(cg.time - cgAbsorbTime);

		absorbTime /= 9000;

		if (absorbTime < 0)
		{
			absorbTime = 0;
		}
		if (absorbTime > 0.15f)
		{
			absorbTime = 0.15f;
		}

		hcolor[3] = absorbTime * 0.5f;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0.7f;

		if (!cg.renderingThirdPerson)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}

		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;
	}
	else if (cgAbsorbTime)
	{
		if (!cgAbsorbFadeTime)
		{
			cgAbsorbFadeTime = cg.time;
			cgAbsorbFadeVal = 0.15f;
		}

		absorbTime = cgAbsorbFadeVal;

		cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime)*0.000005f;

		if (absorbTime < 0)
		{
			absorbTime = 0;
		}
		if (absorbTime > 0.15f)
		{
			absorbTime = 0.15f;
		}

		hcolor[3] = absorbTime * 0.5f;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0.7f;

		if (!cg.renderingThirdPerson && absorbTime)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}
		else
		{
			cgAbsorbTime = 0;
		}
	}

	if (cg.snap->ps.fd.forcePowersActive & (1 << FP_PROTECT))
	{
		if (!cgProtectTime)
		{
			cgProtectTime = cg.time;
		}

		protectTime = (float)(cg.time - cgProtectTime);

		protectTime /= 9000.0f;

		if (protectTime < 0)
		{
			protectTime = 0;
		}
		if (protectTime > 0.15f)
		{
			protectTime = 0.15f;
		}

		hcolor[3] = protectTime * 0.5f;
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		if (!cg.renderingThirdPerson)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}

		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;
	}
	else if (cgProtectTime)
	{
		if (!cgProtectFadeTime)
		{
			cgProtectFadeTime = cg.time;
			cgProtectFadeVal = 0.15f;
		}

		protectTime = cgProtectFadeVal;

		cgProtectFadeVal -= (cg.time - cgProtectFadeTime)*0.000005f;

		if (protectTime < 0)
		{
			protectTime = 0;
		}
		if (protectTime > 0.15f)
		{
			protectTime = 0.15f;
		}

		hcolor[3] = protectTime * 0.5f;
		hcolor[0] = 0;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		if (!cg.renderingThirdPerson && protectTime)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}
		else
		{
			cgProtectTime = 0;
		}
	}

	if (BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
	{
		if (!cgYsalTime)
		{
			cgYsalTime = cg.time;
		}

		ysalTime = (float)(cg.time - cgYsalTime);

		ysalTime /= 9000.0f;

		if (ysalTime < 0)
		{
			ysalTime = 0;
		}
		if (ysalTime > 0.15f)
		{
			ysalTime = 0.15f;
		}

		hcolor[3] = ysalTime * 0.5f;
		hcolor[0] = 0.7f;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		if (!cg.renderingThirdPerson)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}

		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;
	}
	else if (cgYsalTime)
	{
		if (!cgYsalFadeTime)
		{
			cgYsalFadeTime = cg.time;
			cgYsalFadeVal = 0.15f;
		}

		ysalTime = cgYsalFadeVal;

		cgYsalFadeVal -= (cg.time - cgYsalFadeTime)*0.000005f;

		if (ysalTime < 0)
		{
			ysalTime = 0;
		}
		if (ysalTime > 0.15f)
		{
			ysalTime = 0.15f;
		}

		hcolor[3] = ysalTime * 0.5f;
		hcolor[0] = 0.7f;
		hcolor[1] = 0.7f;
		hcolor[2] = 0;

		if (!cg.renderingThirdPerson && ysalTime)
		{
			CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
		}
		else
		{
			cgYsalTime = 0;
		}
	}
}

/*
=================
CG_DrawFallingToDeath
=================
*/
static void CG_DrawFallingToDeath( void ) {
	if (cg.snap->ps.fallingToDeath)
	{
		vec4_t	hcolor;
		float	fallTime;

		fallTime = (float)(cg.gameTime - cg.snap->ps.fallingToDeath);

		fallTime /= (FALL_FADE_TIME/2);

		if (fallTime < 0)
		{
			fallTime = 0;
		}
		if (fallTime > 1)
		{
			fallTime = 1;
		}

		hcolor[3] = fallTime;
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 0;

		CG_FillRect(0, 0, cgs.screenWidth, SCREEN_HEIGHT, hcolor);
	}
}

qboolean gCGHasFallVector = qfalse;
vec3_t gCGFallVector;

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
	int				inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	int				wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	int				bestTime;
	int				drawSelect = 0;
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.serverTime > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	CG_DrawForceEffects();

	if (cg.snap->ps.rocketLockIndex != MAX_CLIENTS && (cg.gameTime - cg.snap->ps.rocketLockTime) > 0)
	{
		CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
	}

	if (cg.snap->ps.holocronBits)
	{
		CG_DrawHolocronIcons();
	}
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.gameTime)
	{
		CG_DrawActivePowers();
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.pm_type == PM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair(NULL, 0);
		CG_DrawCrosshairNames();
		CG_SaberClashFlare();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
#ifdef MISSIONPACK
			if ( /*cg_drawStatus.integer*/0 ) {
				//Reenable if stats are drawn with menu system again
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#endif
			//CG_DrawTemporaryStats();

			CG_DrawAmmoWarning();

			CG_DrawCrosshairNames();

			if (cg_drawStatus.integer)
			{
				CG_DrawIconBackground();
			}

			if (inTime > wpTime)
			{
				drawSelect = 1;
				bestTime = cg.invenSelectTime;
			}
			else //only draw the most recent since they're drawn in the same place
			{
				drawSelect = 2;
				bestTime = cg.weaponSelectTime;
			}

			if (cg.forceSelectTime > bestTime)
			{
				drawSelect = 3;
			}

			switch(drawSelect)
			{
			case 1:
				CG_DrawInvenSelect();
				break;
			case 2:
				CG_DrawWeaponSelect();
				break;
			case 3:
				CG_DrawForceSelect();
				break;
			default:
				break;
			}

			if (cg_drawStatus.integer)
			{
				//Powerups now done with upperright stuff
				//CG_DrawPowerupIcons();

				CG_DrawFlagStatus();
			}

			CG_SaberClashFlare();

			if (cg_drawStatus.integer)
			{
				CG_DrawStats();
			}

			//Do we want to use this system again at some point?
			CG_DrawReward();
		}

	}

	CG_DrawFallingToDeath();
	CG_DrawVote();
	CG_DrawTeamVote();

	if ( cg_drawClock.integer) {
		CG_DrawClock();
	}
	CG_DrawLagometer();

	if (!cg_paused.integer) {
		CG_DrawUpperRight();
	}

	CG_DrawWarmup();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( cg.scoreBoardShowing) {
		CG_DrawScoreboardHints();
	} else {
		CG_DrawFollow();
		CG_DrawCenterString();
		CG_DrawCountdown();
	}
}


static void CG_DrawTourneyScoreboard() {
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.pm_type == PM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
 	CG_Draw2D();
}



