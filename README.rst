============
SaberMod 1.0
============

This is a *Star Wars Jedi Knight II: Jedi Outcast* 1.04 mod targeting
all competitive communities. It is developed by *SaberMod team* (see
Authors_) and draws inspiration from all great id Tech 3 games and
mods, trying to improve on usability, stability and user experience
with *no-gimmicks* approach. Main goal of the mod is to promote and
improve competitive aspect of the game.

Source code is hosted on GitHub_ and based on `JK2 SDK GPL`_ - an
updated JK2 1.04 SDK.

Changes
=======

This is just a list of changes that need explanation. Refer to
CHANGELOG.rst and Git commit history for full log.

Client-Side
-----------

Console Commands
................

players [?]
  List players connected to server with some additional info.

follow [first|second]
  Follow first or second best player on the server. Can be used to
  stay on spectator team in Tournament gametype.

Callvote
........

match <0|1>
  Enable/Disable match mode - restrict spectator chat and disable
  damage plums.

mode [mode]
  Switch to one of admin-defined game modes.

nk [mode]
  No Kick. `mode` can be 1 - no dmg, 2 - no knockback, 3 - no kicking.

wk
  With Kick - default JK2 rules.

remove <player>
  Remove `player` to spectator team.

teamsize <size>
  Set maximum team size to `size`. 0 means unlimited. No players will
  be removed.

CGame Cvars
...........

handicap <x>
  Lower your max health to x and damage to x%.

cg_camerafps <fps>
  Enable FPS-independent third person camera that behaves exactly like
  original camera running at <fps> frames per second and in perfect
  conditions. Setting this to your `com_maxfps` value seamlessly fixes
  camera warping in many scenarios: unstable fps, unstable connection,
  overloaded server, local server, high velocity movement, demo
  playback. 0 restores original behaviour.

cg_chatBeep <0|1>
  Turn on/off chat beep.

cg_crosshairColor <hex>
  Force crosshair color using hexadecimal rgb color code. Fourth
  position can be used to set transparency. Eg `#00ff00ff` is green.

cg_damagePlums <0|1>
  When you hit an enemy, draw a small damage plum coming out of his
  torso. Works only if server has `g_damagePlums` enabled.

cg_darkenDeadBodies <0|1>
  Darken dead bodies outside of duel too.

cg_drawRewards <0|1>
  Draw rewards for outstanding moves. Requires ent's "Jedi Knight
  Rewards 2" assets.

cg_drawClock <0|1>
  Draw clock showing your local time.

cg_drawTimer <0|1|2>
  Draw game timer. 1 - count up, 2 - count down.

cg_duelGlow <0|1>
  Turn on/off duel glow.

cg_followKiller <0|1>
  When player you are following dies, switch to his killer.

cg_followPowerup <0|1>
  Automatically follow flag and powerup carriers.

cg_privateDuel <0|1>
  Hide all other players and entities when duelling. Available only
  on server running JK2MV 1.2 or newer.

Spectating
..........

As spectator `+use` button makes you change followed player using
"smart cycle" mode. It will switch between duelling players, search
for a next powerup player or cycle through current team in a
scoreboard order.

As free floating spectator you can target a player with your crosshair
and press `+attack` button to start following him.

Server-Side
-----------

Server Commands
...............

announce <message>
  Print `message` on everyone's screen.

forceteam <player> <team>
  Little known original command allowing admin to move players between
  teams.

(un)lockteam <teams>
  Prevent players from joining `teams`.

mode <mode|default>
  Change to `mode` or list all available modes when passed without
  arguments.

remove <player> [time]
  Remove `player` to spectator team for at least `time` seconds.

Game Cvars
..........

teamsize <size>
  See callvote_ teamsize.

dmflags <bitmask>
  Sum of values from the following list:

  =====================  =====================  =====================
  8 - No fall damage     16 - Fixed fov (80)    32 - No footsteps
  64 - No kick mode      128 - league mod YDFA
  =====================  =====================  =====================

g_allowVote <0|1|bitmask>
  0 / 1 - disable / enable all votes.

  Moreover you can decide what votes should be available by setting
  it to a sum of values from the following list:

  =====================  =====================  =====================
  2 - Map Restart        4 - Next Map           8 - Map
  16 - Gametype          32 - Kick
  128 - Do Warmup        256 - Timelimit        512 - Fraglimit
  1024 - Roundlimit      2048 - Teamsize        4096 - Remove
  8192 - WK/NK           16384 - Mode           32768 - Match Mode
  65536 - Capturelimit
  =====================  =====================  =====================

g_damagePlums <0|1>
  Allow clients with `cg_damagePlums` enabled to see damage plums.

g_infiniteAmmo <0|1>
  Players spawn with infinite ammo for all weapons.

g_instagib <0|1>
  Enable simple instagib mode for all weapons. Splash does no damage.

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
  32768 - Game Stats     65536 - Duel Stats
  =====================  =====================  =====================

g_maxGameClients <limit>
  Removed. Use teamsize instead.

g_modeDefault <mode>
  Default server mode. Read `Server Modes`_ section to learn how to
  use it properly.

g_modeIdleTime <minutes>
  Reset to default mode if server has been idle for this many minutes.

g_noKick <0|1|2>
  See callvote_ nk and wk.

g_restrictChat <0|1>
  Prevent spectators from speaking to players and all clients from
  speaking to dueling players.

g_roundWarmup <seconds>
  How many seconds players get to reposition themselves at the start
  of a round.

g_spawnShield <ammount>
  Ammount of shield player gets on spawn.

g_teamForceBalance <number>
  Prevents players from joining the weaker team if difference
  is greater than `number`.

g_teamsizeMin <size>
  Minimum votable teamsize.

g_spawnItems <bitmask>
  What items will be given to players on spawn. Use following bitmask:

  ================  ================  ===============  ===============
  2 - Seeker Drone  4 - Forcefield    8 - Bacta        64 - Sentry
  ================  ================  ===============  ===============

g_spawnWeapons <bitmask>

  Controls weapons given to players on spawn using the same bitmask
  as `g_weaponDisable`. The later cvar affects only weapons and ammo
  spawned on a map. Setting this cvar to 0 restores original behaviour
  of `g_weaponDisable`.

roundlimit <limit>
  Number of rounds in a round-based match.

Round-Based Gametypes
.....................

In round-based gametypes players spawn with all available weapons and
items (controlled by `g_spawnWeapons` and `g_spawnItems` cvars),
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
file in `modes/` directory that will be executed when players
sucessfuly vote to use it. It can contain any commands altering server
behaviour, but please take following guides into consideration.

Switching to a mode from any other should always result in the same
server state. To achieve this it's best to use a "reset" config,
executed at the start of each mode config. It should contain a default
value for every possible cvar your modes are changing. Examine
included modes and `reset.cfg` as an example.

Other type of modes can change a specific rule instead of loading full
game config. For example one could create "Kicks On" mode that changes
g_noKicks value to 0. In such scenario it's be best to also include a
mode reverting to original state: "Kicks Off", or reset affected cvars
in `reset.cfg`.

Server can be configured to go back to a default mode after a period
of inactivity. To do so last lines of the main server config should
resemble following template::

  set g_modeIdleTime "10"
  set g_modeDefault "mymode"
  exec "modes/mymode"
  map ffa_bespin

Where `mymode` is the default mode.

Build
=====

Linux
-----

You will need GNU Make and GCC or Clang compiler. Type ``make`` to
build .so files in base/ and .qvm files in base/vm/ You can add
``-jN`` option to speed up the build process by running N jobs
simultaneously. Type ``make help`` to learn about other targets.

Assume your mod is called "mymod" and your main JK2 directory is
~/.jkii In order to test the mod, put .qvm files in ~/.jkii/mymod/vm/
and launch the game with ``+set fs_game mymod`` commandline parameter.

To debug your mod use generated .so files. Put them in ~/.jkii/mymod/
and launch the game with ``+set vm_game 0 +set vm_cgame 0 +set vm_ui
0`` commandline parameters. Set them back to 2 when you want to use
.qvm version again.

Windows
-------

Currently there is no support for building shared libraries on
Windows. Old ``code/buildvms.bat`` batch file should work for QVMs if
you can get lcc and q3asm tools (eg from *JK2 Editing Tools 2.0*) and
put them into bin/ directory.

I'll be glad to include Windows build scripts, project files etc. if
you can create and test them.

License
=======

LCC 4.1 is Copyright (c) 1991-1998 by AT&T, Christopher W. Fraser and
David R. Hanson, and available under a non-copyleft license. You can
find it in code/tools/lcc/COPYRIGHT. LCC version bundled with this SDK
comes from ioquake3 and it has been slightly modified by it's
developers.

Some files in `assets` directory are modified assets from the
original, non-free JK2 1.04 release and licensed under *JK2 Editing
Tools 2.0* EULA.

Remaining parts of JK2 SDK GPL are licensed under GPLv2 as free
software. Read LICENSE.txt and README-raven.txt to learn
more. According to the license, among other things, you are obliged to
distribute full source code of your mod alongside of it, or at least a
written offer to ship it (eg a HTTP download link inside a .pk3
file). Moreover, any mod using patches from this repository **has to**
be released under GPLv2.

Q3ASM is Copyright (c) id Software and ioquake3 developers.

Authors
-------

* id Software 1999-2000
* Raven Software 1999-2002
* SaberMod team 2015-2017

  + Witold *fau* Piłat <witold.pilat@gmail.com> 2015-2017
  + Dziablo 2015-2016

Thanks
------

* Daggolin (boy) - Technical discussion, sharing patches and his JK2
  modding expertise.
* Miso - Sending patches, testing, promoting SaberMod by hosting
  servers and events.
* Xycaleth - Creating League mod that was a great inspiration to
  SaberMod and sharing it's source code.
* Developers of jk2mv, mvsdk, Jedi Academy, OpenJK, ioq3, jomme, JA++
  (japp), League Mod and other open source id tech 3 mods for various
  code bugfixes.

.. _GitHub : https://github.com/aufau/SaberMod
.. _`JK2 SDK GPL`: https://github.com/aufau/jk2sdk-gpl
