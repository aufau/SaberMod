========
SaberMod
========

This is a *Star Wars Jedi Knight II: Jedi Outcast* 1.04 mod targeting
competitive community. It is developed by *SaberMod team* (see
Authors_) and draws inspiration from all id Tech 3 based games and
mods, trying to improve on usability, stability and user experience
with *no-gimmicks* approach. Our first goal is reimplementing and
improving on the features of popular *league mod* by XycaleTh.

Source code is hosted on GitHub_ and based on `JK2 SDK GPL`_ - an
updated JK2 1.04 SDK.

Changes
=======

This is just a list of changes that need explanation. Refer to Git
commit history for full changelog.

Client-Side
-----------

Console Commands
................

players
  List players connected to server with some additional info.

follow [first|second]
  Follow first or second best player on the server.

Callvote
........

mode [mode]
  Switch to one of admin-defined game modes. To see a full list type
  `callvote mode`.

nk [mode]
  No Kick. `mode` can be 1 - no dmg, 2 - no knockback, 3 - no kicking

wk
  With Kick - default JK2 rules.

remove <player>
  Remove `player` to spectator team.

teamsize <size>
  Set maximum team size to `size`. 0 means unlimited. No players will
  be removed.

Cvars
.....

handicap <x>
  Lower your max health to x and damage to x%.

cg_camerafps <fps>
  Enable FPS-independent third person camera that behaves exactly like
  original camera running at <fps> frames per second and in perfect
  conditions. Setting this to your `com_maxfps` value seamlessly fixes
  camera warping in many scenarios: unstable fps, unstable connection,
  overloaded server, local server, high velocity movement, demo
  playback. 0 restores original behaviour.

cg_chatBeep 1
  Turn on/off chat beep.

cg_damagePlums 1
  When you hit an enemy, draw a small damage plum coming out of his
  torso. Works only if server has `g_damagePlums` enabled.

cg_darkenDeadBodies 0
  Darken dead bodies outside of duel too.

cg_drawRewards 1
  Draw rewards for outstanding moves. Requires ent's "Jedi Knight
  Rewards 2" assets.

cg_drawClock 0
  Draw clock showing your local time.

cg_drawTimer 2
  Count down by default. 1 reverts to original behaviour.

cg_duelGlow 1
  Turn on/off duel glow.

cg_followKiller 0
  When player you are following dies, switch to his killer.

cg_followPowerup 0
  Automatically follow flag and powerup carriers.

cg_privateDuel 0
  Hide all other players and entities when duelling.

Spectating
..........

As spectator `+use` button makes you switch followed player using
"smart cycle" mode. It will switch between duelling players you're
currently following, search for next powerup player or cycle through a
team you're following in scoreboard order.

As free floating spectator you can target a player with your crosshair
and press `+attack` button to start following him.

Server-Side
-----------

Server Commands
...............

announce <message>
  Print `message` on everyone's screen.

(un)lockteam <teams>
  Prevent players from joining `teams`.

remove <player> [time]
  Remove `player` to spectator team for at least `time` seconds.

Cvars
.....

New and modified cvars with default values.

teamsize
  See callvote_ teamsize.

dmflags 0
  Sum of values from the following list:

  =====================  =====================  =====================
  8 - No fall damage     16 - Fixed fov (80)    32 - No footsteps
  64 - No kick mode      128 - league mod YDFA
  =====================  =====================  =====================

g_allowVote 1
  0 / 1 - disable / enable all votes

  Moreover you can decide what votes should be available by setting
  it to a sum of values from the following list:

  =====================  =====================  =====================
  2 - Map Restart        4 - Next Map           8 - Map
  16 - Gametype          32 - Kick              64 - Client Kick
  128 - Do Warmup        256 - Timelimit        512 - Fraglimit
  1024 - Roundlimit      2048 - Teamsize        4096 - Remove
  8192 - WK/NK           16384 - Mode
  =====================  =====================  =====================

g_damagePlums
  Allow clients with `cg_damagePlums` enabled to see damage plums.

g_infiniteAmmo 0
  Players spawn with infinite ammo for all weapons.

g_instagib 0
  Enable simple instagib mode for all weapons. Splash does no damage.

g_log[1-4] <filename>
  You can use 4 separate log files now.

g_consoleFilter <mask>
g_logFilter[1-4] <mask>
  Filter events that should be printed in the dedicated server console
  or saved in the corresponding log file using following bit mask:

  TODO RELEASE

g_maxGameClients 0
  Removed. Use teamsize instead.

g_noKick [type]
  See callvote_ nk and wk. `type` can be 0, 1 or 2.

g_restrictChat 0
  Prevent spectators from speaking to players and all clients from
  speaking to dueling players.

g_roundWarmup 10
  How many seconds players get to reposition themselves at the start
  of a round.

g_spawnShield 25
  Ammount of shield player gets on spawn.

g_teamForceBalance <number>
  Prevents players from joining the weaker team if difference
  is greater than `number`.

g_teamsizeMin 2
  Minimum votable teamsize

g_spawnItems 0
  What items will be given to players on spawn. Use following bitmask:

  ================  ================  ===============  ===============
  2 - Seeker Drone  4 - Forcefield    8 - Bacta        64 - Sentry
  ================  ================  ===============  ===============

g_spawnWeapons 0
  Controls weapons given to players on spawn using the same bitmask
  as `g_weaponDisable`. The later cvar affects weapons and ammo
  spawned on a map, and if a game is considered saber-only. Setting
  this cvar to 0 restores original behaviour of `g_weaponDisable`.

roundlimit 0
  Number of rounds in a round-based match.

Round-based gametypes
.....................

In round-based gametypes players spawn with all available weapons and
items (controlled by `g_spawnWeapons` and `disable_*` cvars), however
there are no pickups on the map. Players gain one point for killing
an enemy and one point for each 50 damage dealt to enemy team. A round
lasts until either one team gets eliminated or a timelimit
is hit. Match ends when a roundlimit is hit.

Red Rover (g_gametype 9)
  It can be described as a FFA gametype with a twist. There are two
  teams, player who gets killed respawns in the opposing team. Round
  ends when one team gets eliminated, but the match winner is a person
  who scores most points.

Clan Arena (g_gametype 10)
  Player who dies must spectate until the end of a round.

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

* id Software (c) 1999-2000
* Raven Software (c) 2000-2002
* SaberMod team (c) 2015-2016

  + Witold *fau* Piłat <witold.pilat@gmail.com> (c) 2015-2016
  + Dziablo (c) 2015-2016

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
