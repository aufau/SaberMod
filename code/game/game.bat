@set include=
@set savedpath=%path%
@set path=%path%;..\..\..\bin

del /q vm
if not exist vm\ mkdir vm
cd vm
set cc=..\..\..\bin\lcc -A -DQ3_VM -DNDEBUG -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1

copy ..\g_syscalls.asm .
@if errorlevel 1 goto quit

%cc%  ../g_main.c
@if errorlevel 1 goto quit

%cc%  ../bg_misc.c
@if errorlevel 1 goto quit
%cc%  ../bg_lib.c
@if errorlevel 1 goto quit
%cc%  ../bg_pmove.c
@if errorlevel 1 goto quit
%cc%  ../bg_saber.c
@if errorlevel 1 goto quit
%cc%  ../bg_slidemove.c
@if errorlevel 1 goto quit
%cc%  ../bg_panimate.c
@if errorlevel 1 goto quit
%cc%  ../bg_weapons.c
@if errorlevel 1 goto quit
%cc%  ../q_math.c
@if errorlevel 1 goto quit
%cc%  ../q_shared.c
@if errorlevel 1 goto quit

%cc%  ../ai_main.c
@if errorlevel 1 goto quit
%cc%  ../ai_util.c
@if errorlevel 1 goto quit
%cc%  ../ai_wpnav.c
@if errorlevel 1 goto quit

%cc%  ../g_active.c
@if errorlevel 1 goto quit

%cc%  ../g_bot.c
@if errorlevel 1 goto quit
%cc%  ../g_client.c
@if errorlevel 1 goto quit
%cc%  ../g_cmds.c
@if errorlevel 1 goto quit
%cc%  ../g_combat.c
@if errorlevel 1 goto quit
%cc%  ../g_items.c
@if errorlevel 1 goto quit
%cc%  ../g_log.c
@if errorlevel 1 goto quit
%cc%  ../g_mem.c
@if errorlevel 1 goto quit
%cc%  ../g_misc.c
@if errorlevel 1 goto quit
%cc%  ../g_missile.c
@if errorlevel 1 goto quit
%cc%  ../g_mover.c
@if errorlevel 1 goto quit
%cc%  ../g_object.c
@if errorlevel 1 goto quit
%cc%  ../g_saga.c
@if errorlevel 1 goto quit
%cc%  ../g_session.c
@if errorlevel 1 goto quit
%cc%  ../g_spawn.c
@if errorlevel 1 goto quit
%cc%  ../g_svcmds.c
@if errorlevel 1 goto quit
%cc%  ../g_target.c
@if errorlevel 1 goto quit
%cc%  ../g_team.c
@if errorlevel 1 goto quit
%cc%  ../g_trigger.c
@if errorlevel 1 goto quit
%cc%  ../g_utils.c
@if errorlevel 1 goto quit
%cc%  ../g_weapon.c
@if errorlevel 1 goto quit
%cc%  ../w_force.c
@if errorlevel 1 goto quit
%cc%  ../w_saber.c
@if errorlevel 1 goto quit
%cc%  ../g_stats.c
@if errorlevel 1 goto quit
%cc%  ../g_dimensions.c
@if errorlevel 1 goto quit
%cc%  ../g_unlagged.c
@if errorlevel 1 goto quit
%cc%  ../g_debug.c
@if errorlevel 1 goto quit
%cc%  ../g_referee.c
@if errorlevel 1 goto quit

..\..\..\bin\q3asm -f ../game
@if errorlevel 1 goto quit

:quit
@set path=%savedpath%
@set savedpath=

cd ..
