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

// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
extern menuDef_t *menuScoreboard;
#endif


void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (targetNum == -1) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i\n", targetNum, atoi( test ) ) );
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("%s" S_COLOR_WHITE " (%i %i %i) : %i\n", cgs.mapname, (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
		(int)cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f( void ) {
#ifdef MISSIONPACK
	CG_BuildSpectatorString();
#endif
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			cg.numScores = 0;
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.serverTime;
	}
}
#ifdef MISSIONPACK
extern menuDef_t *menuScoreboard;
void Menu_Reset();			// FIXME: add to right include file

static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}
#endif // MISSIONPACK

static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30 );
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30 );
}


static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}
#ifdef MISSIONPACK
static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5\n");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6\n");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (GT_Flag(cgs.gametype)) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/
#endif // MISSIONPACK

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/

/*
==================
CG_Players_f
==================
*/
static void CG_Players_f( void ) {
	int				len;
	int				maxNameLen;
	int				i;
	int				teamMask = -1;

	if (!Q_stricmp(CG_Argv(1), "?")) {
		CG_Printf(
			"Flag legend:\n"
			S_COLOR_CYAN "Bot" S_COLOR_WHITE " - bot\n"
			S_COLOR_MAGENTA "  H" S_COLOR_WHITE " - player uses handicap\n"
			S_COLOR_RED "R  " S_COLOR_WHITE " - referee\n"
			);
		return;
	}

	if (trap_Argc() > 1) {
		teamMask = 0;
	}

	for (i = trap_Argc() - 1; i >= 1; i--) {
		team_t team = BG_TeamFromString(CG_Argv(i));

		if (team == TEAM_NUM_TEAMS) {
			return;
		}

		teamMask |= (1 << team);
	}

	maxNameLen = STRLEN("Name");
	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (cgs.clientinfo[i].infoValid ) {
			len = Q_PrintStrlen(cgs.clientinfo[i].name);
			if (len > maxNameLen) {
				maxNameLen = len;
			}
		}
	}
	if (maxNameLen > MAX_NAME_LEN) {
		maxNameLen = MAX_NAME_LEN;
	}

	CG_Printf("Num Flg Team Name%s\n", Spaces(maxNameLen - STRLEN("Name")));
	CG_Printf("--- --- ---- %s\n", Dashes(maxNameLen));

	for (i = 0 ; i < cgs.maxclients ; i++) {
		if (cgs.clientinfo[i].infoValid &&
			((1 << cgs.clientinfo[i].team) & teamMask))
		{
			char		name[MAX_NETNAME];
			const char	*flagColor[3] = { S_COLOR_WHITE, S_COLOR_WHITE, S_COLOR_WHITE };
			int			flag[3] = { ' ', ' ', ' '};

			if (cgs.clientinfo[i].botSkill) {
				flagColor[0] = flagColor[1] = flagColor[2] = S_COLOR_CYAN;
				flag[0] = 'B';
				flag[1] = 'o';
				flag[2] = 't';
			} else {
				if (cgs.clientinfo[i].referee) {
					flagColor[0] = S_COLOR_RED;
					flag[0] = 'R';
				}
				if (cgs.clientinfo[i].handicap > 0 && cgs.clientinfo[i].handicap < 100) {
					flagColor[2] = S_COLOR_MAGENTA;
					flag[2] = 'H';
				}
			}

			Q_strncpyz(name, cgs.clientinfo[i].name, sizeof(name));
			Q_CleanStr(name);

			CG_Printf("%3d %s%c%s%c%s%c " S_COLOR_WHITE "%-4.4s %s\n", i,
				flagColor[0], flag[0], flagColor[1], flag[1], flagColor[2], flag[2],
				BG_TeamName(cgs.clientinfo[i].team, CASE_NORMAL), name);
		}
	}
}

/*
==================
CG_Seek_f
==================
*/
static void CG_Seek_f( void ) {
	enum { SEEK_FORWARD, SEEK_BACKWARD, SEEK_TIME } type;

	const char	*arg;
	int			msec;

	if (!cg.demoPlayback) {
		CG_Printf( "You can seek only during demo playback.\n" );
		return;
	}

	if (cg.seekTime) {
		return;
	}

	arg = CG_Argv(1);

	if (arg[0] == '+') {
		type = SEEK_FORWARD;
		arg++;
	} else if (arg[0] == '-') {
		type = SEEK_BACKWARD;
		arg++;
	} else if (isdigit(arg[0])) {
		type = SEEK_TIME;
	} else {
		CG_Printf("usage: seek [+|-][minutes:]seconds\n");
		return;
	}

	// not validating
	msec = 1000 * atoi(arg);
	arg = strchr(arg, ':' );
	if (arg) {
		msec *= 60;
		msec += 1000 * atoi(++arg);
	}

	switch (type) {
	case SEEK_FORWARD:
		cg.seekTime = cg.serverTime + msec;
		cg.fastSeek = qfalse;
		break;
	case SEEK_BACKWARD:
		msec = cg.serverTime - msec - cgs.levelStartTime;
		if (msec < 0) {
			msec = 0;	// prevent looping infinitely
		}
		trap_SendConsoleCommand(va("ui_seek %d\n", msec / 1000));
		break;
	case SEEK_TIME:
		if (cgs.levelStartTime + msec >= cg.serverTime) {
			cg.seekTime = cgs.levelStartTime + msec;
			cg.fastSeek = qtrue;
		} else {
			if (msec < 0) {
				msec = 0;	// prevent looping infinitely
			}
			trap_SendConsoleCommand(va("ui_seek %d\n", msec / 1000));
		}
		break;
	}
}

/*
==================
CG_CycleSpectatorMode
==================
*/
static void CG_CycleSpectatorMode( int dir ) {
	static const char *specModeNames[SPECMODE_MAX] = {
		"Follow",
		"Free Angles"
	};

	if (cg.spec.following) {
		cg.spec.mode = (cg.spec.mode + SPECMODE_MAX + dir) % SPECMODE_MAX;

		if (cg.spec.mode == SPECMODE_FREEANGLES) {
			usercmd_t	cmd;
			int			cmdNum;

			cmdNum = trap_GetCurrentCmdNumber();
			trap_GetUserCmd(cmdNum, &cmd);

			PM_SetDeltaAngles(cg.spec.delta_angles, cg.snap->ps.viewangles, &cmd);
		}

		CG_Printf("Spectator Mode: %s\n", specModeNames[cg.spec.mode]);
	} else {
		CG_Printf("Spectator Mode: Free\n");
	}
}

/*
==================
CG_PrevSpectatorMode_f
==================
*/
static void CG_PrevSpectatorMode_f( void ) {
	CG_CycleSpectatorMode(-1);
}

/*
==================
CG_NextSpectatorMode_f
==================
*/
static void CG_NextSpectatorMode_f( void ) {
	CG_CycleSpectatorMode(1);
}


typedef struct {
	const char	*cmd;
	void		(*function)(void);
} consoleCommand_t;

static const consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
#ifdef MISSIONPACK
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
#endif
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "startOrbit", CG_StartOrbit_f },
	//{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },
	{ "players", CG_Players_f },
	{ "motd", CG_PrintMotd_f },
	{ "seek", CG_Seek_f },
	{ "prevspecmode", CG_PrevSpectatorMode_f },
	{ "nextspecmode", CG_NextSpectatorMode_f },
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	size_t		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	size_t		i;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("forcechanged");
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
#ifdef MISSIONPACK
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
#endif
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("follownext");
	trap_AddCommand ("followprev");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo

	trap_AddCommand ("ragequit");
	trap_AddCommand ("timeout");
	trap_AddCommand ("timein");
	trap_AddCommand ("referee");
}

void CG_ShutDownConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < (int)ARRAY_LEN( commands ) ; i++ ) {
		trap_RemoveCommand( commands[i].cmd );
	}

	trap_RemoveCommand ("forcechanged");
	trap_RemoveCommand ("kill");
	trap_RemoveCommand ("say");
	trap_RemoveCommand ("say_team");
	trap_RemoveCommand ("tell");
#ifdef MISSIONPACK
	trap_RemoveCommand ("vsay");
	trap_RemoveCommand ("vsay_team");
	trap_RemoveCommand ("vtell");
	trap_RemoveCommand ("vtaunt");
	trap_RemoveCommand ("vosay");
	trap_RemoveCommand ("vosay_team");
	trap_RemoveCommand ("votell");
#endif
	trap_RemoveCommand ("give");
	trap_RemoveCommand ("god");
	trap_RemoveCommand ("notarget");
	trap_RemoveCommand ("noclip");
	trap_RemoveCommand ("team");
	trap_RemoveCommand ("follow");
	trap_RemoveCommand ("follownext");
	trap_RemoveCommand ("followprev");
	trap_RemoveCommand ("levelshot");
	trap_RemoveCommand ("addbot");
	trap_RemoveCommand ("setviewpos");
	trap_RemoveCommand ("callvote");
	trap_RemoveCommand ("vote");
	trap_RemoveCommand ("callteamvote");
	trap_RemoveCommand ("teamvote");
	trap_RemoveCommand ("stats");
	trap_RemoveCommand ("teamtask");
	trap_RemoveCommand ("loaddefered");	// spelled wrong, but not changing for demo

	trap_RemoveCommand ("ragequit");
	trap_RemoveCommand ("timeout");
	trap_RemoveCommand ("timein");

	// referee commands

	trap_RemoveCommand ("referee");
	trap_RemoveCommand ("unreferee");
	trap_RemoveCommand ("lockteam");
	trap_RemoveCommand ("unlockteam");
	trap_RemoveCommand ("forceteam");
	trap_RemoveCommand ("announce");
	trap_RemoveCommand ("help");
	trap_RemoveCommand ("pause");
	trap_RemoveCommand ("unpause");
	trap_RemoveCommand ("allready");
}
