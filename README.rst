========
SaberMod
========

This is a *Star Wars Jedi Knight II: Jedi Outcast* 1.04 mod targeting
competitive community. It is developed by *SaberMod team* (see
Authors_) and draws inspiration from all id Tech 3 based games and
mods, trying to improve on usability, stability and user experience
with *no-gimmicks* approach. Our first goal is reimplementing and
improving on the features of popular *league mod* by XycaleTh.

This code is based on `JK2 SDK GPL`_ - an updated JK2 1.04 SDK.

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

Callvote
........

remove <player>
  Remove `player` to spectator team.

teamsize <size>
  Set maximum team size to `size`. 0 means unlimited. No players will
  be removed.

Server-Side
-----------

Server Commands
...............

remove <player> [time]
  remove `player` to spectator team for at least `time` seconds.

Cvars
.....

New and modified cvars with default values.

teamsize
  See callvote_ teamsize.

g_allowVote 1
  0 / 1 - disable / enable all votes
  Moreover you can decide what votes should be available using
  bitmask: TODO-RELEASE

g_maxGameClients 0
  Removed. Use teamsize instead.

g_restrictChat 0
  Prevent spectators from speaking to players and all clients from
  speaking to dueling players.

g_spawnShield 25
  Ammount of shield player gets on spawn.

g_teamsizeMin 2
  Minimum votable teamsize

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
original, non-free JK2 1.04 release.

Remaining parts of JK2 SDK GPL are licensed under GPLv2 as free
software. Read LICENSE.txt and README-raven.txt to learn
more. According to the license, among other things, you are obliged to
distribute full source code of your mod alongsid it, or at least a
written offer to ship it (eg a HTTP download link inside a .pk3
file). Moreovery any mod using patches from this repository **has to**
be released under GPLv2.

Q3ASM is Copyright (c) id Software and ioquake3 developers.

Authors
-------

* id Software (c) 1999-2000
* Raven Software (c) 2000-2002
* SaberMod team (c) 2015-2016

  + Witold *fau* Piłat <witold.pilat@gmail.com> (c) 2015-2016
  + Dziablo (c) 2015-2016

.. _`JK2 SDK GPL`: https://github.com/aufau/jk2sdk-gpl
