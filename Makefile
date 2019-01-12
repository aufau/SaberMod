# This file is part of SaberMod - Star Wars Jedi Knight II: Jedi Outcast mod.

# Copyright (C) 2015-2019 Witold Pilat <witold.pilat@gmail.com>

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

.SUFFIXES:

AS	= bin/q3asm
LCC	= bin/q3lcc
RCC	= bin/q3rcc
CPP	= bin/q3cpp
LBURG   = bin/lburg

tools = $(AS) $(LCC) $(RCC) $(CPP) $(LBURG)

CFLAGS		= -ggdb3 -fvisibility=hidden -pipe
INCLUDES	= -Icode/cgame -Icode/game -Icode/ui
DEFS		= -DJK2AWARDS
VERSION		:= $(shell git describe --always --tags --dirty)
ifeq ($(.SHELLSTATUS), 0)
	DEFS += -DGIT_VERSION=\"$(VERSION)\"
endif

ALL_CFLAGS := $(CFLAGS) $(INCLUDES) $(DEFS)
ALL_CFLAGS += -fPIC
ALL_CFLAGS += -Wall -Wextra -Wno-unknown-pragmas		\
-Wno-missing-field-initializers -Wno-unused-parameter
LCC_CFLAGS := $(LCFLAGS) $(INCLUDES) $(DEFS)
LCC_CFLAGS += -S -Wf-target=bytecode -Wf-g -DQ3_VM
ifneq ($(DEBUG_VM), 1)
	LCC_CFLAGS += -DNDEBUG
endif

ifeq ($(findstring -m32, $(ALL_CFLAGS)), -m32)
	ARCH := i386
else
	ARCH := $(shell uname -m | sed -e s/i.86/i386/ | sed -e s/x86_64/amd64/ )
endif

# Set V=1 to print full compiler commands.
ifeq ($(V),1)
	echo_cmd=@:
	Q=
else
	echo_cmd=@echo
	Q=@
endif

# Sources

srcs_game = g_main ai_main ai_util ai_wpnav bg_lib bg_misc bg_pmove		\
bg_panimate bg_slidemove bg_weapons bg_saber g_active g_bot g_client	\
g_cmds g_combat g_items g_log g_mem g_misc g_missile g_mover g_object	\
g_saga g_session g_spawn g_svcmds g_target g_team g_trigger g_utils		\
g_weapon w_force w_saber q_math q_shared g_syscalls g_stats				\
g_dimensions g_unlagged g_debug g_referee

srcs_cgame = cg_main cg_consolecmds cg_draw cg_drawtools cg_effects	\
cg_ents cg_event cg_info cg_light cg_localents cg_marks cg_players	\
cg_playerstate cg_predict cg_saga cg_scoreboard cg_servercmds		\
cg_snapshot cg_turret cg_view cg_weaponinit cg_weapons fx_blaster	\
fx_bowcaster fx_bryarpistol fx_demp2 fx_disruptor fx_flechette		\
fx_heavyrepeater fx_rocketlauncher bg_slidemove bg_weapons			\
bg_panimate bg_pmove bg_lib bg_misc bg_saber q_math q_shared		\
cg_syscalls

srcs_ui = ui_main ui_atoms ui_force ui_shared ui_gameinfo bg_misc	\
bg_weapons bg_lib q_math q_shared ui_syscalls ui_macroscan

dep_game	:= $(srcs_game:%=out/mod/%.d)
dep_cgame	:= $(srcs_cgame:%=out/mod/%.d)
dep_ui		:= $(srcs_ui:%=out/mod/%.d)

asm_game	:= $(srcs_game:%=out/mod/%.asm)
asm_cgame	:= $(srcs_cgame:%=out/mod/%.asm)
asm_ui		:= $(srcs_ui:%=out/mod/%.asm)

obj_game	:= $(srcs_game:%=out/mod/%.o)
obj_cgame	:= $(srcs_cgame:%=out/mod/%.o)
obj_ui		:= $(srcs_ui:%=out/mod/%.o)

name		:= SaberMod

cls_pk3		:= $(name)-$(VERSION).pk3
cls_doc		:= README.rst LICENSE.txt
cls_assets := SOURCE.txt mv.info ui/jk2mpingame.txt						\
ui/jk2mp/menudef.h ui/jk2mp/ingame_about.menu							\
ui/jk2mp/ingame_join.menu ui/jk2mp/ingame_callvote.menu					\
ui/jk2mp/ingame_controls.menu ui/jk2mp/createserver.menu				\
ui/jk2mp/ingame_player.menu ui/jk2mp/ingame_setup_modoptions.menu		\
ui/jk2mp/ingame_setup_modoptions2.menu ui/jk2mp/gameinfo.txt			\
ui/jk2mp/setup.menu ui/jk2mp/ingame.menu								\
ui/jk2mp/ingame_setup_original.menu ui/jk2mp/rules_games.menu			\
strip/SABERMOD2_INGAME.sp strip/SABERMOD2_MENUS.sp						\
shaders/sabermod.shader_mv shaders/sabermod.shader						\
gfx/2d/crosshairj.tga gfx/2d/crosshairarrow.tga							\
gfx/menus/download.tga gfx/menus/missing.tga							\
gfx/menus/menu_buttonback_new.jpg gfx/menus/menu_buttonback2_new.jpg	\
scripts/arenas.txt scripts/duel.arena

svs_zip		:= $(name)-$(VERSION).zip
svs_assets := server.cfg reset.cfg modes/ scripts/arenas.txt	\
scripts/duel.arena
svs_doc := README.rst LICENSE.txt CHANGELOG.rst cvar-calculator.html	\
assets/SOURCE.txt

# Targets

all	: vm shared
vm	: game cgame ui
shared	: gameshared cgameshared uishared
game	: base/vm/jk2mpgame.qvm version
cgame	: base/vm/cgame.qvm version
ui	: base/vm/ui.qvm
gameshared	: base/jk2mpgame_$(ARCH).so version
cgameshared	: base/cgame_$(ARCH).so version
uishared	: base/ui_$(ARCH).so
tools	: $(tools)
assets : base/
	$(echo_cmd) "CREATE assets.pk3"
	$(Q)set -e; ${RM} base/assets.pk3; pushd assets; zip		\
	../base/assets.pk3 $(cls_assets); zip -r ../base/assets.pk3	\
	$(svs_assets); popd
clientside : cgame ui | base/
	$(echo_cmd) "CREATE $(name).pk3"
	$(Q)set -e; pushd base; $(RM) $(cls_pk3); zip $(cls_pk3)			\
	vm/cgame.qvm vm/cgame.map vm/ui.qvm vm/ui.map; popd; zip			\
	base/$(cls_pk3) $(cls_doc); pushd assets; zip ../base/$(cls_pk3)	\
	$(cls_assets); popd
serverside : clientside game | base/
	$(echo_cmd) "CREATE $(name).zip"
	$(eval tmp := $(shell mktemp -d))
	$(eval svs := $(tmp)/$(name))
	$(Q)set -e; $(RM) base/$(svs_zip); mkdir -p $(svs)/doc; cp			\
	$(svs_doc) $(svs)/doc; cp base/$(cls_pk3) $(svs); mkdir				\
	$(svs)/vm; cp base/vm/jk2mpgame.qvm base/vm/jk2mpgame.map			\
	$(svs)/vm; pushd assets; cp -r $(svs_assets) $(svs); popd; pushd	\
	$(tmp); zip -r $(svs_zip) $(name); popd; cp $(tmp)/$(svs_zip)		\
	base/; $(RM) -r $(tmp)

help	:
	@echo 'Targets:'
	@echo '  all (default)  - Build all targets'
	@echo '  vm             - Build QVM targets in base/vm/'
	@echo '  shared         - Build shared libraries in base/'
	@echo '  clientside     - Create clientside .pk3 release'
	@echo '  serverside     - Create serverside .zip release'
	@echo '  assets         - Create assets.pk3 for development'
	@echo '  game/cgame/ui  - Build game/cgame/ui QVM target'
	@echo '  gameshared/..  - Build game/cgame/ui shared libraries'
	@echo '  tools          - Build q3asm and q3lcc in bin/'
	@echo 'Cleaning targets:'
	@echo '  clean          - Same as vmclean sharedclean'
	@echo '  vmclean        - Remove QVM and intermediate files'
	@echo '  sharedclean    - Remove shared libraries and intermediate files'
	@echo '  toolsclean     - Remove q3asm and q3lcc'
	@echo '  depclean       - Remove generated dependency files'
	@echo '  distclean      - Remove all generated files'
version : FORCE
	@touch code/game/bg_version.h

FORCE	:
.PHONY	: vm game cgame ui shared gameshared cgameshared uishared tools version FORCE

# QVM Targets

run_as = $(AS) $(ASFLAGS) -vq3 -m -o $@

base/vm/jk2mpgame.qvm : $(asm_game) $(AS) code/game/game.q3asm | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_game)
base/vm/cgame.qvm : $(asm_cgame) $(AS) code/cgame/cgame.q3asm | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_cgame)
base/vm/ui.qvm : $(asm_ui) $(AS) code/ui/ui.q3asm | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_ui)

# BAT Script Helpers

code/game/game.q3asm : Makefile
	$(echo_cmd) "CREATE $@"
	$(file > $@,-o "jk2mpgame.qvm" $(srcs_game))
code/cgame/cgame.q3asm : Makefile
	$(echo_cmd) "CREATE $@"
	$(file > $@,-o "cgame.qvm" $(srcs_cgame))
code/ui/ui.q3asm : Makefile
	$(echo_cmd) "CREATE $@"
	$(file > $@,-o "ui.qvm" $(srcs_ui))

# Shared Object Targets

run_ld = $(CC) -shared $(ALL_CFLAGS) $^ -o $@

base/jk2mpgame_$(ARCH).so : $(obj_game) | base/
	$(echo_cmd) "LD $@"
	$(Q)$(run_ld)
base/cgame_$(ARCH).so : $(obj_cgame) | base/
	$(echo_cmd) "LD $@"
	$(Q)$(run_ld)
base/ui_$(ARCH).so : $(obj_ui) | base/
	$(echo_cmd) "LD $@"
	$(Q)$(run_ld)

# Pattern rules

.PRECIOUS : %/

%/ :
	$(echo_cmd) "MKDIR $@"
	$(Q)mkdir -p $@

define dep_template =
out/mod/%.d : code/$(1)/%.c | out/mod/
	@set -e; $(RM) $$@; gcc -MM $$< -MF $$@ -MT 'out/mod/$$*.o out/mod/$$*.asm $$@';
endef

$(eval $(call dep_template,game))
$(eval $(call dep_template,cgame))
$(eval $(call dep_template,ui))

out/mod/g_syscalls.asm : code/game/g_syscalls.asm
	$(Q)cp -f $< $@
out/mod/cg_syscalls.asm : code/cgame/cg_syscalls.asm
	$(Q)cp -f $< $@
out/mod/ui_syscalls.asm : code/ui/ui_syscalls.asm
	$(Q)cp -f $< $@

define asm_template =
out/mod/%.asm : code/$(1)/%.c $(LCC) | out/mod/
	$(echo_cmd) "LCC $$<"
	$(Q)$(LCC) -c $(LCC_CFLAGS) $$< -o $$@
endef

$(eval $(call asm_template,game))
$(eval $(call asm_template,cgame))
$(eval $(call asm_template,ui))

define obj_template =
out/mod/%.o : code/$(1)/%.c | out/mod/
	$(echo_cmd) "CC $$<"
	$(Q)$(CC) -c $(ALL_CFLAGS) $$< -o $$@
endef

$(eval $(call obj_template,game))
$(eval $(call obj_template,cgame))
$(eval $(call obj_template,ui))

# Header files dependencies

-include $(dep_game) $(dep_cgame) $(dep_ui)

#
# Tools
#

TOOLSDIR = code/tools

TOOLS_DEFS := -DARCH_STRING=\"$(ARCH)\"
TOOLS_CFLAGS = -O2 -Wno-unused-result
TOOLS_CFLAGS += $(TOOLS_DEFS)

srcs_asm = q3asm cmdlib
srcs_lcc = lcc bytecode
srcs_rcc = alloc bind bytecode dag dagcheck decl enode error event	\
expr gen init inits input lex list main null output prof profio simp	\
stmt string sym symbolic trace tree types
srcs_cpp = cpp lex nlist tokens macro eval include hideset getopt	\
unix
srcs_lburg = lburg gram

obj_asm		:= $(srcs_asm:%=out/asm/%.o)
obj_lcc		:= $(srcs_lcc:%=out/lcc/%.o)
obj_rcc		:= $(srcs_rcc:%=out/rcc/%.o)
obj_cpp		:= $(srcs_cpp:%=out/cpp/%.o)
obj_lburg	:= $(srcs_lburg:%=out/lburg/%.o)

dep_asm		:= $(srcs_asm:%=out/asm/%.d)
dep_lcc		:= $(srcs_lcc:%=out/lcc/%.d)
dep_rcc		:= $(srcs_rcc:%=out/rcc/%.d)
dep_cpp		:= $(srcs_cpp:%=out/cpp/%.d)
dep_lburg	:= $(srcs_lburg:%=out/lburg/%.d)

dep_tools	= $(dep_asm) $(dep_lcc) $(dep_rcc) $(dep_cpp) $(dep_lburg)

define run_tools_link
	$(echo_cmd) "LD $@"
	$(Q)$(CC) -o $@ $^
endef

$(AS) : $(obj_asm) | bin/
	$(run_tools_link)

$(LCC) : $(obj_lcc) $(RCC) $(CPP) | bin/
	$(echo_cmd) "LD $@"
	$(Q)$(CC) -o $@ $(obj_lcc)

$(RCC) : $(obj_rcc) | bin/
	$(run_tools_link)

$(CPP) : $(obj_cpp) | bin/
	$(run_tools_link)

$(LBURG) : $(obj_lburg) | bin/
	$(run_tools_link)

q3asmsrcdir = $(TOOLSDIR)/asm
q3lccsrcdir = $(TOOLSDIR)/lcc/src
q3lccetcdir = $(TOOLSDIR)/lcc/etc
q3cppsrcdir = $(TOOLSDIR)/lcc/cpp
q3lburgsrcdir = $(TOOLSDIR)/lcc/lburg

$(q3lccsrcdir)/dagcheck.c : $(q3lccsrcdir)/dagcheck.md $(LBURG)
	$(echo_cmd) "LBURG $<"
	$(Q)$(LBURG) $< $@

define dep_tools_template =
out/$(1)/%.d : $(2)/%.c | out/$(1)/
	@set -e; $(RM) $$@; $(CC) $(TOOLS_DEFS) -MM $$< -MF $$@ -MT 'out/$(1)/$$*.o $$@';
endef

$(eval $(call dep_tools_template,asm,$(q3asmsrcdir)))
$(eval $(call dep_tools_template,rcc,$(q3lccsrcdir)))
$(eval $(call dep_tools_template,lcc,$(q3lccetcdir)))
$(eval $(call dep_tools_template,cpp,$(q3cppsrcdir)))
$(eval $(call dep_tools_template,lburg,$(q3lburgsrcdir)))

define obj_tools_template =
out/$(1)/%.o : $(2)/%.c | out/$(1)/
	$(echo_cmd) "CC $$<"
	$(Q)$(CC) -c $(TOOLS_CFLAGS) $$< -o $$@
endef

$(eval $(call obj_tools_template,asm,$(q3asmsrcdir)))
$(eval $(call obj_tools_template,rcc,$(q3lccsrcdir)))
$(eval $(call obj_tools_template,lcc,$(q3lccetcdir)))
$(eval $(call obj_tools_template,cpp,$(q3cppsrcdir)))
$(eval $(call obj_tools_template,lburg,$(q3lburgsrcdir)))

-include $(dep_tools)

# Clean targets

.PHONY : clean vmclean asmclean objclean sharedclean toolsclean depclean

clean : vmclean sharedclean

vmclean : asmclean
	$(Q)$(RM) base/vm/*.qvm base/vm/*.map
	$(echo_cmd) "Removed .qvm and .map files"
sharedclean : objclean
	$(Q)$(RM) base/*.so
	$(echo_cmd) "Removed .so files"
asmclean :
	$(Q)$(RM) $(asm_game)
	$(Q)$(RM) $(asm_cgame)
	$(Q)$(RM) $(asm_ui)
	$(echo_cmd) "Removed .asm files"
objclean :
	$(Q)$(RM) $(obj_game)
	$(Q)$(RM) $(obj_cgame)
	$(Q)$(RM) $(obj_ui)
	$(echo_cmd) "Removed .o files"
toolsclean :
	$(Q)$(RM) $(tools)
	$(Q)$(RM) $(obj_asm) $(obj_lcc) $(obj_rcc) $(obj_cpp) $(obj_lburg)
	$(echo_cmd) "Removed tools"
depclean :
	$(Q)$(RM) $(dep_game)
	$(Q)$(RM) $(dep_cgame)
	$(Q)$(RM) $(dep_ui)
	$(Q)$(RM) $(dep_tools)
	$(echo_cmd) "Removed .d files"
distclean :
	$(Q)$(RM) -r base bin out
	$(echo_cmd) "Removed directories: bin base out"
