# JK2 SDK GPL Makefile (c) 2015 fau <faltec@gmail.com>
#

.SUFFIXES:

AS	= bin/q3asm
LCC	= bin/q3lcc
RCC	= bin/q3rcc
CPP	= bin/q3cpp
LBURG   = bin/lburg

tools = $(AS) $(LCC) $(RCC) $(CPP) $(LBURG)

CFLAGS		= -g -fvisibility=hidden
INCLUDES	= -Icode/cgame -Icode/game -Icode/ui
DEFS		= -DGIT_VERSION=\"$(VERSION)\" -DQAGAME
VERSION		= $(shell git describe --always --tags --dirty)

ALL_CFLAGS := $(CFLAGS) $(INCLUDES) $(DEFS) -fPIC
ALL_CFLAGS += -Wall -Wno-unused-but-set-variable -Wno-unknown-pragmas	\
-Wno-missing-braces
LCC_CFLAGS := $(LCFLAGS) $(INCLUDES) $(DEFS)
LCC_CFLAGS += -S -Wf-target=bytecode -Wf-g -DQ3_VM

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
bg_panimate bg_slidemove bg_weapons bg_saber g_active g_arenas g_bot	\
g_client g_cmds g_combat g_items g_log g_mem g_misc g_missile g_mover	\
g_object g_saga g_session g_spawn g_svcmds g_target g_team g_trigger	\
g_utils g_weapon w_force w_saber q_math q_shared g_syscalls

srcs_cgame = cg_main cg_consolecmds cg_draw cg_drawtools cg_effects	\
cg_ents cg_event cg_info cg_light cg_localents cg_marks cg_players	\
cg_playerstate cg_predict cg_saga cg_scoreboard cg_servercmds		\
cg_snapshot cg_turret cg_view cg_weaponinit cg_weapons fx_blaster	\
fx_bowcaster fx_bryarpistol fx_demp2 fx_disruptor fx_flechette		\
fx_heavyrepeater fx_rocketlauncher fx_force bg_slidemove bg_weapons	\
bg_panimate bg_pmove bg_lib bg_misc bg_saber q_math q_shared		\
ui_shared cg_newDraw cg_syscalls

srcs_ui = ui_main ui_atoms ui_force ui_shared ui_gameinfo bg_misc	\
bg_weapons bg_lib q_math q_shared ui_syscalls

dep_game	:= $(srcs_game:%=out/mod/%.d)
dep_cgame	:= $(srcs_cgame:%=out/mod/%.d)
dep_ui		:= $(srcs_ui:%=out/mod/%.d)

asm_game	:= $(srcs_game:%=out/mod/%.asm)
asm_cgame	:= $(srcs_cgame:%=out/mod/%.asm)
asm_ui		:= $(srcs_ui:%=out/mod/%.asm)

obj_game	:= $(srcs_game:%=out/mod/%.o)
obj_cgame	:= $(srcs_cgame:%=out/mod/%.o)
obj_ui		:= $(srcs_ui:%=out/mod/%.o)

pk3name		:= SaberMod
pk3			:= $(pk3name)-$(VERSION).pk3
pk3doc		:= README.rst LICENSE.txt
pk3assets := SOURCE.txt ui/jk2mp/ingame_about.menu	\
ui/jk2mp/ingame_join.menu strip/SABERMOD_INGAME.sp	\
strip/SABERMOD_MENUS.sp

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
package : cgame ui | base/
	set -e; pushd base; $(RM) $(pk3); zip -r $(pk3) vm/cgame.qvm	\
	vm/ui.qvm; popd; zip base/$(pk3) $(pk3doc); pushd assets; zip	\
	../base/$(pk3) $(pk3assets); popd
help	:
	@echo 'Targets:'
	@echo '  all (default)  - Build all targets'
	@echo '  vm             - Build QVM targets in base/vm/'
	@echo '  shared         - Build shared libraries in base/'
	@echo '  package        - Build clientside .pk3 package'
	@echo '  game/cgame/ui  - Build game/cgame/ui QVM target'
	@echo '  gameshared/..  - Build game/.. shared libraries'
	@echo '  tools          - Build q3asm and q3lcc in bin/'
	@echo 'Cleaning targets:'
	@echo '  clean          - Same as vmclean sharedclean'
	@echo '  vmclean        - Remove QVM and intermediate files'
	@echo '  sharedclean    - Remove shared libraries and intermediate files'
	@echo '  packageclean   - Remove .pk3 packages'
	@echo '  toolsclean     - Remove q3asm and q3lcc'
	@echo '  depclean       - Remove generated dependency files'
	@echo '  distclean      - Remove all generated files'
version : FORCE
	@touch code/game/bg_version.h

FORCE	:
.PHONY	: vm game cgame ui shared gameshared cgameshared uishared tools version FORCE

# QVM Targets

run_as = $(AS) $(ASFLAGS) -vq3 -o $@

base/vm/jk2mpgame.qvm : $(asm_game) $(AS) | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_game)
base/vm/cgame.qvm : $(asm_cgame) $(AS) | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_cgame)
base/vm/ui.qvm : $(asm_ui) $(AS) | base/vm/
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(run_as) $(asm_ui)

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

clean : vmclean sharedclean packageclean

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
packageclean :
	$(Q)$(RM) base/$(pk3name)-*.pk3
	$(echo_cmd) "Removed .pk3 files"
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
