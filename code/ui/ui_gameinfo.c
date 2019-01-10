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
// gameinfo.c
//

#include "ui_local.h"


//
// arena and bot info
//


int				ui_numBots;
static char		*ui_botInfos[MAX_BOTS];

static int		ui_numArenas;
static char		*ui_arenaInfos[MAX_ARENAS];

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( const char *buf, int max, char *infos[] ) {
	const char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) != 0 ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] )
				Info_SetValueForKey( info, key, "<NULL>" );
			else
				Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = (char *)UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas( void ) {
	int			numdirs;
	vmCvar_t	arenasFile;
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;
	const char	*type;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM );
	if( *arenasFile.string ) {
		UI_LoadArenasFromFile(arenasFile.string);
	}
	else {
		UI_LoadArenasFromFile("scripts/arenas.txt");
	}

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		Q_strcat(filename, sizeof(filename), dirptr);
		UI_LoadArenasFromFile(filename);
	}
	trap_Print( va( "%i arenas parsed\n", ui_numArenas ) );
	if (UI_OutOfMemory()) {
		trap_Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");
	}

	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type

		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "map"));
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc(Info_ValueForKey(ui_arenaInfos[n], "longname"));
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc(va("levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName));
		uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

		type = Info_ValueForKey( ui_arenaInfos[n], "type" );
		// if no type specified, it will be treated as "ffa"
		if( *type ) {
			for ( i = 0; i < GT_MAX_GAME_TYPE; i++ ) {
				if ( Q_stristr( type, gametypeShort[i] ) ) {
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << i);
				}
			}
		} else {
			uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << GT_FFA);
		}

		uiInfo.mapCount++;
		if (uiInfo.mapCount >= MAX_MAPS) {
			break;
		}
	}
}


/*
===============
UI_LoadBotsFromFile
===============
*/
static void UI_LoadBotsFromFile( const char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];
	char			*stopMark;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;

	stopMark = strstr(buf, "@STOPHERE");

	//This bot is in place as a mark for modview's bot viewer.
	//If we hit it just stop and trace back to the beginning of the bot define and cut the string off.
	//This is only done in the UI and not the game so that "test" bots can be added manually and still
	//not show up in the menu.
	if (stopMark)
	{
		int startPoint = stopMark - buf;

		while (buf[startPoint] != '{')
		{
			startPoint--;
		}

		buf[startPoint] = 0;
	}

	trap_FS_FCloseFile( f );

	COM_Compress(buf);

	ui_numBots += UI_ParseInfos( buf, MAX_BOTS - ui_numBots, &ui_botInfos[ui_numBots] );
}

/*
===============
UI_LoadBots
===============
*/
void UI_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	ui_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		UI_LoadBotsFromFile(botsFile.string);
	}
	else {
		UI_LoadBotsFromFile("botfiles/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		Q_strcat(filename, sizeof(filename), dirptr);
		UI_LoadBotsFromFile(filename);
	}
	trap_Print( va( "%i bots parsed\n", ui_numBots ) );
}


/*
===============
UI_GetBotInfoByNumber
===============
*/
char *UI_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= ui_numBots ) {
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return ui_botInfos[num];
}

#ifdef UNUSED
/*
===============
UI_GetBotInfoByName
===============
*/
char *UI_GetBotInfoByName( const char *name ) {
	int			n;
	const char	*value;

	for ( n = 0; n < ui_numBots ; n++ ) {
		value = Info_ValueForKey( ui_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return ui_botInfos[n];
		}
	}

	return NULL;
}
#endif // UNUSED

int UI_GetNumBots() {
	return ui_numBots;
}


const char *UI_GetBotNameByNumber( int num ) {
	const char *info = UI_GetBotInfoByNumber(num);
	if (info) {
		return Info_ValueForKey( info, "name" );
	}
	return "Kyle";
}

void UI_LoadModes( const char *directory ) {
	char dir[MAX_QPATH];
	char *mode;
	int nummodes;
	int i;

	Q_strncpyz(dir, directory, sizeof(dir));
	trap_GetConfigString( CS_MODES, uiInfo.modeBuf, sizeof( uiInfo.modeBuf ) );
	mode = uiInfo.modeBuf;
	nummodes = 0;

	if (dir[0]) {
		uiInfo.modeList[nummodes] = "..";
		nummodes++;
	}

	for ( i = 0; i < MAX_MODES; i++ ) {
		char *name = mode;
		int len;
		char *sep;

		mode = strchr( mode, '\\' );
		if (!mode)
			break;
		*mode++ = '\0';

		len = strlen(dir);

		if (!strncmp(name, dir, len)) {
			name += len;
			sep = strchr(name, '/');

			if (sep) {
				// mode in a sub-directory
				sep[1] = '\0';

				// don't duplicate directories
				if (!nummodes || strcmp(name, uiInfo.modeList[nummodes - 1])) {
					uiInfo.modeList[nummodes] = name;
					nummodes++;
				}
			} else {
				// mode on this directory level
				uiInfo.modeList[nummodes] = name;
				nummodes++;
			}
		}
	}

	uiInfo.modeCount = nummodes;
	Menu_SetFeederSelection(NULL, FEEDER_MODES, 0, NULL);
	Q_strncpyz(uiInfo.modeDir, dir, sizeof(uiInfo.modeDir));
}

static int SortServerMapsLoadName( const void *a, const void *b ) {
	const serverMapInfo *ai = (const serverMapInfo *)a;
	const serverMapInfo *bi = (const serverMapInfo *)b;
	char mapNameA[MAX_INFO_VALUE];
	char mapNameB[MAX_INFO_VALUE];

	Q_strncpyz( mapNameA, ai->mapLoadName, sizeof( mapNameA ) );
	Q_CleanStr( mapNameA );

	Q_strncpyz( mapNameB, bi->mapLoadName, sizeof( mapNameB ) );
	Q_CleanStr( mapNameB );

	return Q_stricmp( mapNameA, mapNameB );
}

static int SortServerMapsName( const void *a, const void *b ) {
	const serverMapInfo *ai = (const serverMapInfo *)a;
	const serverMapInfo *bi = (const serverMapInfo *)b;
	char mapNameA[MAX_INFO_VALUE];
	char mapNameB[MAX_INFO_VALUE];

	Q_strncpyz( mapNameA, ai->mapName, sizeof( mapNameA ) );
	Q_CleanStr( mapNameA );

	Q_strncpyz( mapNameB, bi->mapName, sizeof( mapNameB ) );
	Q_CleanStr( mapNameB );

	return Q_stricmp( mapNameA, mapNameB );
}

void UI_LoadServerMaps( void ) {
	char *p;
	int i, n;

	// UI_LoadArenas();

	for ( i = 0; i < MAX_CS_MAPS; i++ ) {
		trap_GetConfigString( CS_MAPS + i, uiInfo.serverMapBuf[i], sizeof( uiInfo.serverMapBuf[0] ) );
	}

	n = 0;
	p = uiInfo.serverMapBuf[n];

	for ( i = 0; i < MAX_SERVER_MAPS; i++ ) {
		const char *name;
		const char *longName;
		qboolean valid = qfalse;
		qboolean done = qfalse;
		int j;

		// fau - I'm ashamed of this parsing
		do {
			name = p;
			p = strchr( p, '\\' );
			if ( p ) {
				*p++ = '\0';

				longName = p;
				p = strchr( p, '\\' );

				if ( p ) {
					*p++ = '\0';
					// we have valid name and longName
					valid = qtrue;
				}
			}

			if ( !p ) {
				// otherwise try next configstring until we run out
				n++;
				if ( n >= MAX_CS_MAPS ) {
					done = qtrue;
					break;
				}
				p = uiInfo.serverMapBuf[n];
			}
		} while ( !valid );

		if ( done ) {
			break;
		}

		uiInfo.serverMapList[i].mapName = longName;
		uiInfo.serverMapList[i].mapLoadName = name;
		uiInfo.serverMapList[i].mapIndex = -1;

		// match local map
		for ( j = 0; j < uiInfo.mapCount; j++ ) {
			if ( !Q_stricmp( uiInfo.mapList[j].mapLoadName, name ) ) {
				uiInfo.serverMapList[i].mapIndex = j;
				break;
			}
		}
	}

	uiInfo.serverMapCount = i;
	uiInfo.serverMapIndex = 0;

	if ( ui_longMapName.integer ) {
		qsort( uiInfo.serverMapList, uiInfo.serverMapCount, sizeof( uiInfo.serverMapList[0] ), SortServerMapsName );
	} else {
		qsort( uiInfo.serverMapList, uiInfo.serverMapCount, sizeof( uiInfo.serverMapList[0] ), SortServerMapsLoadName );
	}
}

void UI_GetHTTPDownloads( void ) {
	char		info[MAX_INFO_VALUE];
	char		clientJK2MV[2];
	const char	*serverJK2MV;

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
	trap_Cvar_VariableStringBuffer( "JK2MV", clientJK2MV, sizeof(clientJK2MV) );
	serverJK2MV = Info_ValueForKey( info, "JK2MV" );

	if (clientJK2MV[0] != '\0' &&
		(int)trap_Cvar_VariableValue( "mv_allowdownload" ) &&
		serverJK2MV[0] != '\0' &&
		atoi( Info_ValueForKey( info, "mv_httpdownloads" ) ) )
	{
		uiInfo.httpDownloads = qtrue;
	}
	else
	{
		uiInfo.httpDownloads = qfalse;
	}
}
