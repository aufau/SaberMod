===========
JK2 SDK GPL
===========

This is an alternative *Star Wars Jedi Knight II: Jedi Outcast* SDK2
released under the GPLv2 license. Search for *JK2 Editing Tools 2.0*
if you'd like to get familiar with original SDK for 1.04 version
released by Raven Software in 2003, containing also tools and guides
for creating maps, models and other modifications.

Creating this repository has been made possible thanks to Raven
Software releasing JK2 source code in 2013. This branch contains an
altered subset of it, necessary for creating code mods, with few
indisputable cleanups and bug fixes.

Build
=====

Windows
-------

Currently there is no support for building VM or .dll files on
Windows. Old ``code/buildvms.bat`` batch file should work for VMs if
you can get lcc and q3asm tools (eg from *JK2 Editing Tools 2.0*) and
put them into bin/ directory.

I'd be glad to include Windows build scripts, project files etc. if
you can create and test them.

License
=======

LCC 4.1 is Copyright (c) 1991-1998 by AT&T, Christopher W. Fraser and
David R. Hanson, and available under a non-free licence. You can find
it in code/tools/lcc/COPYRIGHT. LCC version bundled with this SDK
comes from ioquake3 and it has been slightly modified by it's
developers.

Remaining parts of JK2 SDK GPL are licensed under GPLv2 as free
software. Read LICENSE.txt and readme-raven.txt to learn
more. According to it, among other things, you are obliged to
distribute full source code of your mods alongside them, or at least a
written offer to ship it (eg a HTTP download link inside a .pk3 file)

Q3ASM is Copyright (c) id Software and ioquake3 developers.

Trivia
------

Did you know that, compared to *JK2 Editing Tools 2.0* license, GPLv2
doesn't prohibit you from:

* Distributing your mod by means other than internet and even charging
  for it.

* Creating mods which infringe against third party rights, are
  libelous, defamatory, obscene, false, misleading and otherwise
  illegal and unlawful.

* Shipping them to countries to which the U.S. has embargoed goods and
  to people who are prohibited by applicable law from receiving it.

GPL - JOIN THE DARK SIDE
