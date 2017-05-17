2.0
---

* Unlagged Disruptor hit detection - lag compensation technique that
  allows playing instagib comfortably even with very high ping.
* New votes: Shuffle and Poll.
* Voting: callvote cooldown, ability to change vote.
* Callvote menu: Rules section, maps section: select from server maps
  rather than your own; download icon next to maps you're missing.
* 4 force kick effects to choose from: no effect, basejk, no damage,
  league mod.
* Crosshair: cg_crosshairColor, new "dot" crosshair (9).
* Ingame "Message of the Day", can be seen from ingame about menu.
* Fixed various issues with servers running for a few days or more.
* "players" game console command prints basic information about
  players and their fps and packets.
* Set original (97) FOV limit for all players with dmflags 16.
* Polished Clan Arena gametype and other previous features.
* Bug fixes and performance improvements - notably faster map restart.

1.0
---

* Clan Arena gametype.
* Votable game modes configured by server admin.
* Transparent duels - dueling players don't collide with others.
* Private duels - makes other players invisible (JK2MV 1.2+ servers).
* FPS-independent camera (cg_camerafps).
* Support for ent's "Jedi Knight Rewards 2.0".
* Voting: human-readable Gametype, Match Mode, Capturelimit.
* Improved logging: consistent format, timestamps, up to 4 logfiles
  with events customizable by bitmask cvars.
* Server commands: (un)lockteam, announce, say, tell, mode.
* cg_followKiller, cg_followPowerup, cg_drawTimer 2 (count down),
  cg_darkenDeadBodies, cg_drawClock, cg_duelGlow, cg_chatBeep,
  cg_privateDuel, cg_camerafps - most available in setup menu.
* g_instagib, g_spawnWeapons, g_spawnItems, g_infiniteAmmo,
  g_logFilter, g_consoleFilter, g_consoleFilter, g_modeDefault.
* League mod compatibility dmflag.
* Removed hi-res fonts as they are part of JK2MV now.
* Exploits blocked: speed hack, health reading, saber stealing,
  charged jump.
* Bugs fixed: glowing weapon, broken ydfa, rocket lock bug, saber lock
  bug, invisible saber, saber flying through the map on respawn,
  forcefield warping, 3-player duel and more.

Alpha
-----

* End-game statistics tuned for gametype.
* Detailed scoreboard tuned for gametype.
* Damage plums.
* Voting: No Kicks / With Kicks, Remove (removes to spectator),
  Teamsize, Roundlimit.
* Red Rover gametype.
* Smart follow button (+use).
* High resolution fonts.
* Smart name matching in game commands (tell, remove etc).
* /players client console command
* g_damagePlums, g_restrictChat, g_spawnShield, g_allowVote (uses
  bitmask now), teamsize, roundlimit, g_roundWarmup, g_teamsizeMin.
* Multiple bug fixes, optimizations and adjustments.
