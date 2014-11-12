@echo off

rm -f language.tgz
rm -f debug.txt
rm -f *.log
rm -f *.tmp
rm -f *.DAT
rm -f *.prc 
rm -f *.zip
rm -f *.CAB
rm -f *.exe

del /s *.aps 2>nul
del /s *.vcl 2>nul
rm -f lang/*.bin

rm -rf obj.arm 
rm -rf obj.m68k
rm -rf objsingle.arm
rm -rf objsingle.m68k
rm -rf objplugin.m68k
rm -rf objplugin.arm

for /D %%a in (.,*) do call :clean_dir %%a
goto :eof

:clean_dir
echo %1
rm -f %1\*.dat 
rm -rf %1\armrel 
rm -rf %1\armrelgcc 
rm -rf %1\armv4rel 
rm -rf %1\armv4relgcc 
rm -rf %1\mipsrel 
rm -rf %1\sh3rel 
rm -rf %1\armdbg 
rm -rf %1\armv4dbg 
rm -rf %1\mipsdbg 
rm -rf %1\sh3dbg 
rm -rf %1\debug 
rm -rf %1\release 
rm -rf "%1\debug unicode"
rm -rf "%1\release unicode" 
rm -rf %1\emulatorrel 
rm -rf %1\emulatordbg 
rm -rf %1\x86rel 
rm -rf %1\x86dbg 
rm -rf %1\x86emrel 
rm -rf %1\x86emdbg 
rm -rf %1\mipsiirel 
rm -rf %1\mipsiidbg 
rm -rf "%1\pocket pc 2003 (armv4)" 
rm -rf "%1\smartphone 2003 (armv4)" 
rm -rf "%1\windows mobile 5.0 pocket pc sdk (armv4i)" 
rm -rf "%1\windows mobile 5.0 smartphone sdk (armv4i)" 
rm -rf "%1\x64" 

:eof
