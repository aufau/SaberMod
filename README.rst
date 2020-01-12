============
SaberMod 2.0
============

This is a *Star Wars Jedi Knight II: Jedi Outcast* 1.04 mod targeting
all competitive communities. It is developed by *SaberMod team* (see
Authors_) and draws inspiration from all great id tech 3 games and
mods, trying to improve on usability, stability and user experience
with *no-gimmicks* approach. Main goal of the mod is to promote and
improve competitive aspect of the game.

Source code is hosted on GitHub_ and based on `JK2 SDK GPL`_ - an
updated JK2 1.04 SDK.

What's New
==========

This is just a list of new features that require explanation. Refer to
CHANGELOG.rst and Git commit history for full log.

Client-Side
-----------

Console Commands
................

follow [first|second]
  Follow first or second best player on the server. Can be used to
  stay on spectator team in Tournament gametype.

motd
  Show server Message of the Day.

nextspecmode/prevspecmode
  Cycle through spectator modes when following another
  player. Currently available modes are:
  1. Follow another player from his POV. (default)
  2. You can freely control camera angles and range.

players [?|team]
  List players connected to server with some additional
  info. Optionally limit to members of *team*.

timein
  Resume paused game.

timeout
  Pause game for 60 seconds.

ragequit
  Reserved for God, do not use.

ready
  Toggle ready status during warmup.

seek [+|-][mm:]ss
  During demo playback seeks to *mm:ss* server time. With *+* or *-*
  prefix seeks *mm* minutes and *ss* seconds forwards or backwards.

Call Vote
.........

abort
  Abort the match and go back to warmup.

matchmode <0|1>
  Enable/Disable match mode - restrict spectator chat and require
  matching clientside. In round-based gametypes dead players may only
  follow their teammates.

mode [mode]
  Switch to one of admin-defined game modes.

nk
  No Kick. Disables kicking completely and turns on friendly fire.

wk
  With Kick. Kicking mechanics depends on *g_kickMethod* value. Also
  turns off friendly fire.

poll <question>
  Ask an arbitrary question to players.

remove <player>
  Remove *player* to spectator team.

shuffle
  Randomize teams.

teamsize <size>
  Set maximum team size to *size*. 0 means unlimited. No players will
  be removed.

Call Team Vote
..............

forfeit
  Forfeit the game.

Referee Commands
................

These commands can be used only by a registered referee or server
console (rcon).

abort
  Abort the match and go back to warmup.

allready
  Make all players ready in warmup or during intermission.

announce <message|motd>
  Print *message* or ingame message of the day on everyone's screen.

forceteam <player|all> <team>
  Move players between teams.

help
  List referee commands

(un)lockteam <teams>
  Prevent players from joining *teams*.

(un)pause [seconds]
  Pause match for a specified number of seconds or until unpaused.

referee <player>
  Make player a referee without losing own referee status.

unreferee <player|all>
  Remove referee status.

CGame Cvars
...........

handicap <x>
  Lower your max health to x and damage to x%.

cg_autoSave <bitmask>
  Automatically save game record.

  | 1: Save screenshot of the final scoreboard
  | 2: Save demo of the match

cg_autoSaveFormat <format>
  Format is a template for auto demo/screenshot file name, where
  following tokens will be replaced with current values:

  | [date] - Date in yyyy-mm-dd format
  | [time] - Time in hh-mm format
  | [gametype] - Server gametype
  | [name] - Player name
  | [server] - Server name
  | [map] - Map name

cg_chatBeep <0|1>
  Turn on/off chat beep.

cg_crosshairColor <hex>
  Force crosshair color using hexadecimal rgb color code. Fourth
  position can be used to set transparency. Eg ``#00ff00ff`` is green.

cg_crosshairIndicators <bitmask>
  Draw crosshair indicators. Sum values from the following list:

  =====================  =====================
  1 - Movement arrows    2 - Speedometer
  =====================  =====================

cg_crosshairIndicatorsSpec <bitmask>
  Same as above but only when following another player.

cg_damagePlums <0|1>
  When you hit an enemy, draw a small damage plum coming out of his
  torso. Works only if server has *g_damagePlums* enabled.

cg_darkenDeadBodies <0|1>
  Darken dead bodies outside of duel too.

cg_drawClock <0|1>
  Draw clock showing your local time.

cg_drawFollow <0|1>
  Draw large "Following <playername>" message.

cg_drawRewards <0|1>
  Draw rewards for outstanding moves. Requires ent's "Jedi Knight
  Rewards 2" assets.

cg_drawTimer <0|1|2>
  Draw game timer. 1 - count up, 2 - count down.

cg_duelGlow <0|1>
  Turn on/off duel glow.

cg_fastSeek <0|1>
  Use experimental fast seeking method (see *seek* console command).

cg_fixServerTime <0|1>
  Fix various engine issues on servers running for a few days.

cg_followKiller <0|1>
  When player you are following dies, switch to his killer.

cg_followPowerup <0|1>
  Automatically follow flag and powerup carriers.

cg_fovAspectAdjust <0|1>
  Change Field Of View calculations so that they don't disadvantage
  widescreen monitors. Instead of cropping top and bottom parts of the
  screen it's extended to the sides, compared to 4:3 display. Works
  only when *cg_widescreen* is enabled.

cg_drawSpectatorHints <0|1>
  Draw extra hints on new spectator features.

cg_privateDuel <0|1>
  Hide all other players and entities when duelling. Available only
  on server running JK2MV 1.2 or newer.

cg_smoothCamera <0|1>
  Fix camera warping while maintaining original feel in following
  scenarios: unstable fps, unstable connection, overloaded server,
  local server, high velocity movement, demo playback.

cg_smoothCameraFPS <fps>
  Emulate specific fps with *cg_smoothCamera*. When this is 0, current
  com_maxfps is used instead. Useful for demo rendering.

cg_widescreen <0|1>
  Enable HUD adjustments for widescreen monitors

UI Cvars
........

ui_widescreen <0|1>
  Enable menu adjustments for widescreen monitors

Spectating
..........

As a spectator, *+use* button makes you change followed player using
"smart cycle" mode. It will switch between duelling players, search
for a next powerup player or cycle through current team in a
scoreboard order.

As a free floating spectator you can target a player with your
crosshair and press *+attack* button to start following him.

Server-Side
-----------

Console Commands
................

All `Referee Commands`_ can be used as console commands.

items [items]
  Enable/Disable items using human readable names. Type without
  argument to see usage instructions.

mode <mode|default>
  Change to *mode* or list all available modes when passed without
  arguments.

players [team]
  Print various informations about players. Optionally filter by team.

referee <password>
  Become a referee using password provided by server admin.

remove <player|all> [time]
  Remove *player* to spectator team for at least *time* seconds.

spawnitems [items]
  Enable/Disable spawning items using human readable names. Type
  without argument to see usage instructions.

shuffle
  Randomize teams.

Game Cvars
..........

teamsize <size>
  See `Call Vote`_ teamsize.

roundlimit <limit>
  Number of rounds in a round-based match.

duel_fraglimit
  Removed. Use roundlimit instead.

dmflags <bitmask>
  Sum of values from the following list:

  =====================  =====================  =====================
  1 - Fix jump height    2 - CJK Disruptor      4 - Go through team
  8 - No fall damage     16 - Limit FOV (97)    32 - No footsteps
  64 - No kick mode      128 - league mod YDFA
  =====================  =====================  =====================

bot_nochat <0|1>
  Prevent bots from sending chat messages.

g_allowRefVote <0|1|bitmask>
  Control what commands are available to referees. Uses the same
  bitmask as g_allowVote below.

g_allowTeamVote <bitmask>
  What team votes should be allowed. Team votes are called with
  *callteamvote* command.

  =====================  =====================  =====================
  2 - Team Leader        4 - Forfeit Match
  =====================  =====================  =====================

g_allowVote <0|1|bitmask>
  0 / 1 - disable / enable all votes.

  Moreover you can decide what votes should be available by setting
  it to a sum of values from the following list:

  =====================  =====================  =====================
  2 - Map Restart        4 - Next Map           8 - Map
  16 - Gametype          32 - Kick              64 - Shuffle
  128 - Do Warmup        256 - Timelimit        512 - Fraglimit
  1024 - Roundlimit      2048 - Teamsize        4096 - Remove
  8192 - WK/NK           16384 - Mode           32768 - Match Mode
  65536 - Capturelimit   131072 - Poll          262144 - Referee
  524288 - Abort
  =====================  =====================  =====================

g_antiWarp <0|1|2>
  Prevention system against players who are warping or using lag scripts.

  | 1: Draw icon above warping player's head.
  | 2: Forcefully prevent players from warping for others. This
       setting makes game almost unplayable for a warping player and
       may hurt legitimate players who have bad connection.

  Refer to *g_antiWarpTime* cvar description for more details.

g_antiWarpTime <msec>
  Tune when player is considered as warping and g_antiWarp preventive
  actions are taken against him. Default setting is 1000 and it only
  marks players with interrupted connection. To prevent warping and
  lag scripts it should be set as low as possible so that legitimate
  players are not affected.

g_damagePlums <0|1>
  Allow clients with *cg_damagePlums* enabled to see damage plums.

g_dismember <percentage>
  Chance to dismemeber player killed with a lightsaber.

g_infiniteAmmo <0|1>
  Players spawn with infinite ammo for all weapons.

g_ingameMotd <message|none>
  Ingame message of the day shown to all players. May contain ``\n``
  for newline and ``\\`` for backslash.

g_instagib <0|1>
  Enable simple instagib mode for all weapons. Splash does no damage.

g_kickMethod <method>
  Choose one of following force kick methods:

  =====================  =====================  =====================
  0 - No effect          1 - Basejk             2 - No damage
  3 - League Mod
  =====================  =====================  =====================

g_log[1-4] <filename>
  You can use 4 separate log files now.

g_consoleFilter <mask>

g_logFilter[1-4] <mask>
  Filter events that should be printed in the dedicated server console
  or saved in the corresponding log file using following bit mask:

  =====================  =====================  =====================
  1 - Game Status        2 - Client Connect     4 - Client Begin
  8 - Userinfo Change    16 - Client Rename     32 - Client Spawn
  64 - Private Duel      128 - Obituary         256 - Say
  512 - Say Team         1024 - Tell            2048 - Voice Tell
  4096 - Item Pickup     8192 - Flag            16384 - Weapon Stats
  32768 - Game Stats     65536 - Duel Stats     131072 - Vote
  262144 - Referee Cmds
  =====================  =====================  =====================

g_macroscan <0|1>
  Enable scanning for and disabling binds that may give unfair
  advantage. Works only on players using SaberMod Clientside.

g_maxGameClients <limit>
  Removed. Use teamsize instead.

g_modeDefault <mode>
  Default server mode. Read `Server Modes`_ section to learn how to
  use it properly.

g_modeDefaultMap <map>
  Map for default mode. Leave blank to not change map.

g_modeIdleTime <minutes>
  Reset to default mode if server has been idle for this many minutes.

g_pushableItems <mask>
  What types of items should be movable with force push and pull:

  =====================  =====================  =====================
  2 - Weapon             4 - Ammo               8 - Armor
  16 - Health            32 - Powerup           64 - Holdable
  =====================  =====================  =====================

g_refereePassword <password>
  Allow players who know password to become referees using *referee*
  console command. When this cvar is empty (default), *referee*
  console command cannot be used to become a referee.

g_requireClientside <0|1>
  Allow only players with matching clientside to join the game.

g_restrictChat <0|1>
  Prevent spectators from speaking to players and all clients from
  speaking to dueling players.

g_restrictSpectator <0|1>
  Dead players may only follow their teammates.

g_roundWarmup <seconds>
  How many seconds players get to reposition themselves at the start
  of a round.

g_spawnItems <bitmask>
  What items will be given to players on spawn. Use following bitmask:

  =====================  =====================  =====================
  2 - Seeker Drone       4 - Forcefield         8 - Bacta
  32 - Binoculars        64 - Sentry
  =====================  =====================  =====================

g_spawnShield <ammount>
  Ammount of shield player gets on spawn.

g_spawnWeapons <bitmask>
  Controls weapons given to players on spawn using the same bitmask
  as *g_weaponDisable*. The later cvar affects only weapons and ammo
  spawned on a map. Setting this cvar to 0 restores original behaviour
  of *g_weaponDisable*.

g_teamForceBalance <number>
  Prevents players from joining the weaker team if difference
  is greater than *number*.

g_teamsizeMin <size>
  Minimum votable teamsize.

g_timeoutDuration <seconds>
  Duration of a player timeout.

g_timeoutLimit <number>
  Maximum number of times a player is allowed to call a timeout.

g_unlagged <0|1|2>
  Experimental "unlagged" disruptor hit detection. 2 accounts for
  doors and other movers too at some server performance penalty.

g_unlaggedMaxPing <msec>
  Maximum lag compensation. Unlagged has subjective, counter-intuitive
  side effects. For example a player can be hit some time after he hid
  behind an obstacle. This cvar's value limits time period in which
  this can happen, adding extra hit detection delay for players with
  pings higher than *msec*.

g_warmup <0|1>
  SaberMod has a new warmup system. All players must ready up with
  *ready* command before a match can start. Old *g_warmupTime* Cvar is
  no longer used. Setting this cvar to 0 disables warmup alltogether.

g_vampiricDamage <ratio>
  Players become vampires and they drain health when damaging
  enemy. *ratio* value determines how much health is drained for 1
  point of damage. It may be set to a negative value.

g_voteCooldown <seconds>
  How long a player has to wait before he can call another vote.

Round-Based Gametypes
.....................

In round-based gametypes players spawn with all available weapons and
items (controlled by *g_spawnWeapons8 and *g_spawnItems* cvars),
however there are no pickups on the map. Players gain one point for
killing an enemy and one point for each 50 damage dealt to the enemy
team. A round lasts until either one team is eliminated or a timelimit
is hit. Match ends when a roundlimit is hit.

Red Rover (g_gametype 9)
  It can be described as FFA with a twist. There are two teams, player
  who gets killed respawns in the opposing team. Round ends when one
  team is eliminated, but the match winner is a person who scores most
  points.

Clan Arena (g_gametype 10)
  Player who dies must spectate until the end of a round. When one
  team is eliminated, round is over. Team who hits the round limit
  first wins the match.

Server Modes
............

Server administrator can configure a number of custom game "modes",
players will be able to choose from. A mode is technically a config
file in ``modes/`` directory that will be executed when players
sucessfuly vote to use it. It can contain any commands altering server
behaviour, but please take following guides into consideration.

Switching to a mode from any other should always result in the same
server state. To achieve this it's best to use a "reset" config,
executed at the start of each mode config. It should contain a default
value for every possible cvar your modes are changing. Examine
included modes and ``reset.cfg`` as an example.

Server can be configured to go back to a default mode after a period
of inactivity. To do so last lines of the main server config should
resemble following template::

  set g_modeIdleTime "10"
  set g_modeDefault "mymode"
  exec "modes/mymode"
  map ffa_bespin

Where *mymode* is the default mode.

Building SaberMod
=================

SaberMod has several build scripts, but only GNU Make script is
complete and used for official releases. other scripts produce
different QVM files from the same source code.

GNU Make
--------

Only requirements are GNU Make, Git and GCC or Clang
compiler. Makefile has several targets, type ``make <target>`` to
build selected target. Most useful are:

- **shared** build shared module libraries that should be used for
  debugging and development.
- **assets** create assets.pk3 - an amalgamation of all SaberMod
  clientside and serverside assets which is usef!ul for development, as
  they will not be loaded from outside of a pk3.
- **serverside** create a full release zip package.

Type ``make help`` to learn about remaining targets.

Microsoft Visual Studio
-----------------------

Install Git command line program on your system. It can be done from
Visual Studio Team Explorer tab. Use *Clone Git Repository* and
provide address of your SaberMod repo. *Build All* action compiles
.dll modules. Other targets can be compiled from *CMake Targets View*
in *Project Explorer*.

CMake
-----

Install CMake for your operating system (Linux, Windows, MacOS and
others) and use ``CMakelists.txt`` script from the main SaberMod
directory.

Batch Scripts
-------------

Copy lcc.exe and q3asm.exe tools from *JK2 Editing Tools 2.0* into
bin/ directory. Run ``code/buildvms.bat`` batch script to generate
.qvm files.

License
=======

LCC 4.1 is Copyright (c) 1991-1998 by AT&T, Christopher W. Fraser and
David R. Hanson, and available under a non-copyleft license. You can
find it in ``code/tools/lcc/COPYRIGHT``. LCC version bundled with this
SDK comes from ioquake3 and it has been slightly modified by its
developers.

Some files in ``assets`` directory are modified assets from the
original, non-free JK2 1.04 release and licensed under *JK2 Editing
Tools 2.0* EULA.

Remaining parts of JK2 SDK GPL are licensed under GPLv2 as free
software. Read LICENSE.txt and README-raven.txt to learn
more. According to the license, among other things, you are obliged to
distribute full source code of your mod alongside of it, or at least a
written offer to ship it (eg a HTTP download link inside a .pk3
file). Moreover, any mod using patches from this repository **must**
be released under GPLv2 or a compatible license.

Q3ASM is Copyright (c) id Software and ioquake3 developers.

Authors
-------

* id Software 1999-2000
* Raven Software 1999-2002
* SaberMod developers 2015-2020

  + Witold *fau* Piłat <witold.pilat@gmail.com> 2015-2020
  + Dziablo 2015-2016

Thanks
------

* Miso - Sending patches, testing, promoting SaberMod by hosting
  servers and events.
* Daggolin (boy) - Technical discussion, sharing patches and his JK2
  modding expertise.
* Xycaleth - Creating League mod that was a great inspiration to
  SaberMod and sharing its source code.
* ouned - Engine and modding expertise.
* Bucky, God, Kameleon, michl, PowTech, Tr!force - Providing valuable
  programming input, review, ideas and patches.
* Developers of jk2mv, mvsdk, Jedi Academy, OpenJK, ioq3, jomme, JA++
  (japp), League Mod and other open source id tech 3 mods for various
  code bugfixes.
* Players who help testing and improving SaberMod on a daily basis.

.. _GitHub : https://github.com/aufau/SaberMod
.. _`JK2 SDK GPL`: https://github.com/aufau/jk2sdk-gpl
