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
	"cl_yawspeed",
	"cl_pitchspeed"
};

static const char * const illegalCmds[] = {
	"ping",
	"cvarlist",
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
	qboolean	command_ready;
} uiScan_t;

static uiScan_t ui_scan;

struct keyname_s {
	const char	*name;
	fakeAscii_t	key;
};

static const struct keyname_s keynames[MAX_KEYS] =
{
	{ "SHIFT", A_SHIFT },
	{ "CTRL", A_CTRL },
	{ "ALT", A_ALT },
	{ "CAPSLOCK", A_CAPSLOCK },
	{ "KP_NUMLOCK", A_NUMLOCK },
	{ "SCROLLLOCK", A_SCROLLLOCK },
	{ "PAUSE", A_PAUSE },
	{ "BACKSPACE", A_BACKSPACE },
	{ "TAB", A_TAB },
	{ "ENTER", A_ENTER },
	{ "KP_PLUS", A_KP_PLUS },
	{ "KP_MINUS", A_KP_MINUS },
	{ "KP_ENTER", A_KP_ENTER },
	{ "KP_DEL", A_KP_PERIOD },
	{ "KP_INS", A_KP_0 },
	{ "KP_END", A_KP_1 },
	{ "KP_DOWNARROW", A_KP_2 },
	{ "KP_PGDN", A_KP_3 },
	{ "KP_LEFTARROW", A_KP_4 },
	{ "KP_5", A_KP_5 },
	{ "KP_RIGHTARROW", A_KP_6 },
	{ "KP_HOME", A_KP_7 },
	{ "KP_UPARROW", A_KP_8 },
	{ "KP_PGUP", A_KP_9 },
	{ "CONSOLE", A_CONSOLE },
	{ "ESCAPE", A_ESCAPE },
	{ "F1", A_F1 },
	{ "F2", A_F2 },
	{ "F3", A_F3 },
	{ "F4", A_F4 },
	{ "SPACE", A_SPACE },
	{ "SEMICOLON", A_SEMICOLON },
	{ "DEL", A_DELETE },
	{ "EURO", A_EURO },
	{ "SHIFT", A_SHIFT2 },
	{ "CTRL", A_CTRL2 },
	{ "ALTGR", A_ALT2 },
	{ "F5", A_F5 },
	{ "F6", A_F6 },
	{ "F7", A_F7 },
	{ "F8", A_F8 },
	{ "CIRCUMFLEX", A_CIRCUMFLEX },
	{ "MWHEELUP", A_MWHEELUP },
	{ "MWHEELDOWN", A_MWHEELDOWN },
	{ "MOUSE1", A_MOUSE1 },
	{ "MOUSE2", A_MOUSE2 },
	{ "INS", A_INSERT },
	{ "HOME", A_HOME },
	{ "PGUP", A_PAGE_UP },
	{ "F9", A_F9 },
	{ "F10", A_F10 },
	{ "F11", A_F11 },
	{ "F12", A_F12 },
	{ "SHIFT_ENTER", A_ENTER },
	{ "END", A_END },
	{ "PGDN", A_PAGE_DOWN },
	{ "SHIFT_SPACE", A_SHIFT_SPACE },
    { "SHIFT_KP_ENTER", A_SHIFT_KP_ENTER },
	{ "MOUSE3", A_MOUSE3 },
	{ "MOUSE4", A_MOUSE4 },
	{ "MOUSE5", A_MOUSE5 },
	{ "UPARROW", A_CURSOR_UP },
	{ "DOWNARROW", A_CURSOR_DOWN },
	{ "LEFTARROW", A_CURSOR_LEFT },
	{ "RIGHTARROW", A_CURSOR_RIGHT },

	{ "JOY0", A_JOY0 },
	{ "JOY1", A_JOY1 },
	{ "JOY2", A_JOY2 },
	{ "JOY3", A_JOY3 },
	{ "JOY4", A_JOY4 },
	{ "JOY5", A_JOY5 },
	{ "JOY6", A_JOY6 },
	{ "JOY7", A_JOY7 },
	{ "JOY8", A_JOY8 },
	{ "JOY9", A_JOY9 },
	{ "JOY10", A_JOY10 },
	{ "JOY11", A_JOY11 },
	{ "JOY12", A_JOY12 },
	{ "JOY13", A_JOY13 },
	{ "JOY14", A_JOY14 },
	{ "JOY15", A_JOY15 },
	{ "JOY16", A_JOY16 },
	{ "JOY17", A_JOY17 },
	{ "JOY18", A_JOY18 },
	{ "JOY19", A_JOY19 },
	{ "JOY20", A_JOY20 },
	{ "JOY21", A_JOY21 },
	{ "JOY22", A_JOY22 },
	{ "JOY23", A_JOY23 },
	{ "JOY24", A_JOY24 },
	{ "JOY25", A_JOY25 },
	{ "JOY26", A_JOY26 },
	{ "JOY27", A_JOY27 },
	{ "JOY28", A_JOY28 },
	{ "JOY29", A_JOY29 },
	{ "JOY30", A_JOY30 },
	{ "JOY31", A_JOY31 },

	{ "AUX0", A_AUX0 },
	{ "AUX1", A_AUX1 },
	{ "AUX2", A_AUX2 },
	{ "AUX3", A_AUX3 },
	{ "AUX4", A_AUX4 },
	{ "AUX5", A_AUX5 },
	{ "AUX6", A_AUX6 },
	{ "AUX7", A_AUX7 },
	{ "AUX8", A_AUX8 },
	{ "AUX9", A_AUX9 },
	{ "AUX10", A_AUX10 },
	{ "AUX11", A_AUX11 },
	{ "AUX12", A_AUX12 },
	{ "AUX13", A_AUX13 },
	{ "AUX14", A_AUX14 },
	{ "AUX15", A_AUX15 },
	{ "AUX16", A_AUX16 },
	{ "AUX17", A_AUX17 },
	{ "AUX18", A_AUX18 },
	{ "AUX19", A_AUX19 },
	{ "AUX20", A_AUX20 },
	{ "AUX21", A_AUX21 },
	{ "AUX22", A_AUX22 },
	{ "AUX23", A_AUX23 },
	{ "AUX24", A_AUX24 },
	{ "AUX25", A_AUX25 },
	{ "AUX26", A_AUX26 },
	{ "AUX27", A_AUX27 },
	{ "AUX28", A_AUX28 },
	{ "AUX29", A_AUX29 },
	{ "AUX30", A_AUX30 },
	{ "AUX31", A_AUX31 },
};

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

	for (i = 0; i < (int)ARRAY_LEN(illegalCmds); i++) {
		if (!Q_stricmp(cmd_argv[0], illegalCmds[i])) {
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

	if (!Q_stricmp(cmd_argv[0], "ready") && cmd_argc == 1) {
		ui_scan.command_ready = qtrue;
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

Scan for illegal macros and check binds
=================
*/
void UI_MacroScan_f() {
	qboolean developer = trap_Cvar_VariableValue("developer");
	qboolean quiet = (qboolean)!!atoi(UI_Argv(1));
	qboolean disabled = qfalse;
	char kb[1024];
	int i;

	if (developer || !quiet) {
		Com_Printf("Starting macro scan...\n");
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

		if (ui_scan.command_ready) {
			char		asciiName[2] = { 0, 0 };
			const char	*name = asciiName;
			int			j;

			if (isgraph(i)) {
				asciiName[0] = i;
			}

			for (j = 0; j < (int)ARRAY_LEN(keynames); j++) {
				if (i == (int)keynames[j].key) {
					name = keynames[j].name;
					break;
				}
			}

			trap_Cvar_Set("ui_bind_ready", name);
		}
	}

	if (developer || !quiet) {
		Com_Printf("Macro scan completed.");

		if (disabled) {
			Com_Printf(" Some of your binds have been disabled.\n");
		} else {
			Com_Printf("\n");
		}
	}
}
