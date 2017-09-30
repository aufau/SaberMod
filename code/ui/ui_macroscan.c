/*
================================================================================
This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

Copyright (C) 1999-2000 Id Software, Inc.
Copyright (C) 1999-2002 Activision
Copyright (C) 2017 Witold Pilat <witold.pilat@gmail.com>

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

#include "ui_local.h"

static const char * const illegalCvars[] = {
	"com_maxfps",
	"cl_maxpackets",
	"cl_timenudge",
};

#define	MAX_CMD_BUFFER	16384
#define	MAX_CMD_LINE	1024


typedef struct {
	char	*data;
	int		maxsize;
	int		cursize;
} cmd_t;

// struct holding results and intermediate values for macro scan
// recursive functions
typedef struct uiScan_s {
	int			cmd_argc;
	char		*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted
// for auto-complete (copied from OpenJK)
	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)

	int			cmd_wait;
	cmd_t		cmd_text;
	char		cmd_text_buf[MAX_CMD_BUFFER];

	int			movementCmds;
	qboolean	illegal;
	qboolean	invalid;
} uiScan_t;

static uiScan_t ui_scan;

static void UI_Cmd_TokenizeString(const char *text_in) {
	const char	*text;
	char	*textOut;
	qboolean ignoreQuotes = qfalse;

	// clear previous args
	ui_scan.cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	Q_strncpyz( ui_scan.cmd_cmd, text_in, sizeof(ui_scan.cmd_cmd) );

	text = text_in;
	textOut = ui_scan.cmd_tokenized;

	while ( 1 ) {
		if ( ui_scan.cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *(const unsigned char* /*eurofix*/)text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return;			// all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return;		// all tokens parsed
				}
				text += 2;
			} else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings
    // NOTE TTimo this doesn't handle \" escaping
		if ( !ignoreQuotes && *text == '"' ) {
			ui_scan.cmd_argv[ui_scan.cmd_argc] = textOut;
			ui_scan.cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		ui_scan.cmd_argv[ui_scan.cmd_argc] = textOut;
		ui_scan.cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *(const unsigned char* /*eurofix*/)text > ' ' ) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return;		// all tokens parsed
		}

	}
}

void UI_Cbuf_InsertText(const char *text) {
	int		len;

	len = (int)strlen( text ) + 1;
	if ( len + ui_scan.cmd_text.cursize > ui_scan.cmd_text.maxsize ) {
		ui_scan.invalid = qtrue;
		return;
	}

	// move the existing command text
	memmove( &ui_scan.cmd_text.data[ len ], ui_scan.cmd_text.data, ui_scan.cmd_text.cursize );

	// copy the new text in
	memcpy( ui_scan.cmd_text.data, text, len - 1 );

	// add a \n
	ui_scan.cmd_text.data[ len - 1 ] = '\n';

	ui_scan.cmd_text.cursize += len;
}

static void UI_Cmd_Execute(void) {
	char **cmd_argv = ui_scan.cmd_argv;
	int cmd_argc = ui_scan.cmd_argc;
	char *cmd = cmd_argv[0];
	int ci, i;

	if (ui_scan.cmd_argc == 0) {
		return;
	}

	// movement commands
	if (cmd[0] == '+' || cmd[0] == '-') {
		ui_scan.movementCmds++;
		return;
	}

	// altering fps or network settings
	ci = Q_stricmpn(cmd, "set", 3) ? 0 : 1;

	for (i = 0; i < (int)ARRAY_LEN(illegalCvars); i++) {
		if (!Q_stricmp(cmd_argv[ci], illegalCvars[i])) {
			ui_scan.illegal = qtrue;
			return;
		}
	}

	if (!Q_stricmp(cmd_argv[0], "vstr") && cmd_argc == 2) {
		char cvar[1024];
		int len;

		trap_Cvar_VariableStringBuffer(cmd_argv[1], cvar, sizeof(cvar));
		len = strlen(cvar);
		if (len + 1 == sizeof(cvar)) {
			ui_scan.invalid = qtrue;
			return;
		}
		cvar[len] = '\n';
		cvar[len + 1] = '\0';
		UI_Cbuf_InsertText(cvar);
		return;
	}

	if (!Q_stricmp(cmd_argv[0], "exec") && cmd_argc == 2) {
		fileHandle_t f;
		char filename[MAX_QPATH];
		char *buf;
		int len;

		Q_strncpyz(filename, cmd_argv[1], sizeof(filename));
		COM_DefaultExtension(filename, sizeof(filename), ".cfg");
		len = trap_FS_FOpenFile(filename, &f, FS_READ);

		if (f && len > 0) {
			buf = (char *)BG_TempAlloc(len + 1);
			trap_FS_Read(buf, len, f);
			trap_FS_FCloseFile(f);
			buf[len] = '\0';
			UI_Cbuf_InsertText(buf);
			BG_TempFree(len);
		}

		return;
	}
}

static void UI_Cmd_ExecuteString(const char *text) {
	UI_Cmd_TokenizeString(text);
	UI_Cmd_Execute();
}

static void UI_Cbuf_Execute(void)
{
	int		i;
	char	*text;
	char	line[MAX_CMD_LINE];
	int		quotes;

	while (ui_scan.cmd_text.cursize)
	{
		// find a \n or ; line break
		text = ui_scan.cmd_text.data;

		quotes = 0;
		for (i=0 ; i< ui_scan.cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n' || text[i] == '\r' )
				break;
		}

		if( i > MAX_CMD_LINE - 1) {
			i = MAX_CMD_LINE - 1;
		}

		memcpy(line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if (i == ui_scan.cmd_text.cursize)
			ui_scan.cmd_text.cursize = 0;
		else
		{
			i++;
			ui_scan.cmd_text.cursize -= i;
			memmove (text, text+i, ui_scan.cmd_text.cursize);
		}

// execute the command line

		UI_Cmd_ExecuteString (line);
	}
}

static void UI_Cbuf_Init(void) {
	ui_scan.cmd_text.data = ui_scan.cmd_text_buf;
	ui_scan.cmd_text.maxsize = MAX_CMD_BUFFER;
	ui_scan.cmd_text.cursize = 0;
}

/*
=================
UI_MacroScan_f

Scan for illegal macros
=================
*/
void UI_MacroScan_f() {
	qboolean developer = trap_Cvar_VariableValue("developer");
	qboolean quiet = (qboolean)!!atoi(UI_Argv(1));
	qboolean disabled = qfalse;
	char kb[1024];
	int start, end;
	int i;

	if (developer || !quiet) {
		Com_Printf("Starting macro scan...\n");

		start = trap_Milliseconds();
	}

	for (i = 0; i < MAX_KEYS; i++) {
		trap_Key_GetBindingBuf(i, kb, sizeof(kb));

		memset(&ui_scan, 0, sizeof(ui_scan));
		UI_Cbuf_Init();
		UI_Cbuf_InsertText(kb);
		UI_Cbuf_Execute();

		if (ui_scan.movementCmds > 1) {
			disabled = qtrue;
			trap_Key_SetBinding(i, "");
			continue;
		}

		if (ui_scan.movementCmds > 0 && (i == A_MWHEELUP || i == A_MWHEELDOWN)) {
			disabled = qtrue;
			trap_Key_SetBinding(i, "");
			continue;
		}

		if (ui_scan.invalid) {
			disabled = qtrue;
			trap_Key_SetBinding(i, "");
			continue;
		}
	}

	if (developer || !quiet) {
		end = trap_Milliseconds();

		Com_Printf("Macro scan completed in %dms.", end - start);

		if (disabled) {
			Com_Printf(" Some of your binds have been disabled.\n");
		} else {
			Com_Printf("\n");
		}
	}
}
