# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMMON_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zd /O2 /D "NDEBUG" /D "ZLIB_DLL" /D "FASTEST" /D "BUILDFIXED" /D "DYNAMIC_CRC_TABLE" /D "NO_ERRNO_H" /D "MMX" /D "_USRDLL" /D "COMMON_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /map /machine:I386 /out:"../release/common.dll"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "COMMON_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /D "_DEBUG" /D "ZLIB_DLL" /D "FASTEST" /D "BUILDFIXED" /D "DYNAMIC_CRC_TABLE" /D "NO_ERRNO_H" /D "MMX" /D "_USRDLL" /D "COMMON_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"../debug/common.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "common - Win32 Release"
# Name "common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\advanced.c
# End Source File
# Begin Source File

SOURCE=.\audio.c
# End Source File
# Begin Source File

SOURCE=.\bitstrm.c
# End Source File
# Begin Source File

SOURCE=.\buffer.c
# End Source File
# Begin Source File

SOURCE=.\codec.c
# End Source File
# Begin Source File

SOURCE=.\color.c
# End Source File
# Begin Source File

SOURCE=.\context.c
# End Source File
# Begin Source File

SOURCE=.\equalizer.c
# End Source File
# Begin Source File

SOURCE=.\flow.c
# End Source File
# Begin Source File

SOURCE=.\format.c
# End Source File
# Begin Source File

SOURCE=.\format_base.c
# End Source File
# Begin Source File

SOURCE=.\format_subtitle.c
# End Source File
# Begin Source File

SOURCE=.\gzip.c
# End Source File
# Begin Source File

SOURCE=.\helper_base.c
# End Source File
# Begin Source File

SOURCE=.\helper_video.c
# End Source File
# Begin Source File

SOURCE=.\id3tag.c
# End Source File
# Begin Source File

SOURCE=.\idct.c
# End Source File
# Begin Source File

SOURCE=.\node.c
# End Source File
# Begin Source File

SOURCE=.\nulloutput.c
# End Source File
# Begin Source File

SOURCE=.\overlay.c
# End Source File
# Begin Source File

SOURCE=.\parser2.c
# End Source File
# Begin Source File

SOURCE=.\platform.c
# End Source File
# Begin Source File

SOURCE=.\player.c
# End Source File
# Begin Source File

SOURCE=.\playlist.c
# End Source File
# Begin Source File

SOURCE=.\probe.c
# End Source File
# Begin Source File

SOURCE=.\rawaudio.c
# End Source File
# Begin Source File

SOURCE=.\rawimage.c
# End Source File
# Begin Source File

SOURCE=.\str.c
# End Source File
# Begin Source File

SOURCE=.\streams.c
# End Source File
# Begin Source File

SOURCE=.\tchar.c
# End Source File
# Begin Source File

SOURCE=.\timer.c
# End Source File
# Begin Source File

SOURCE=.\tools.c
# End Source File
# Begin Source File

SOURCE=.\video.c
# End Source File
# Begin Source File

SOURCE=.\vlc.c
# End Source File
# Begin Source File

SOURCE=.\waveout.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\advanced.h
# End Source File
# Begin Source File

SOURCE=.\association.h
# End Source File
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\bitgolomb.h
# End Source File
# Begin Source File

SOURCE=.\bitstrm.h
# End Source File
# Begin Source File

SOURCE=.\blit.h
# End Source File
# Begin Source File

SOURCE=.\buffer.h
# End Source File
# Begin Source File

SOURCE=.\codec.h
# End Source File
# Begin Source File

SOURCE=.\color.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\context.h
# End Source File
# Begin Source File

SOURCE=.\equalizer.h
# End Source File
# Begin Source File

SOURCE=.\err.h
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\fixed.h
# End Source File
# Begin Source File

SOURCE=.\flow.h
# End Source File
# Begin Source File

SOURCE=.\format.h
# End Source File
# Begin Source File

SOURCE=.\format_base.h
# End Source File
# Begin Source File

SOURCE=.\gzip.h
# End Source File
# Begin Source File

SOURCE=.\id3tag.h
# End Source File
# Begin Source File

SOURCE=.\idct.h
# End Source File
# Begin Source File

SOURCE=.\mem.h
# End Source File
# Begin Source File

SOURCE=.\multithread.h
# End Source File
# Begin Source File

SOURCE=.\node.h
# End Source File
# Begin Source File

SOURCE=.\nulloutput.h
# End Source File
# Begin Source File

SOURCE=.\overlay.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\platform.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\playlist.h
# End Source File
# Begin Source File

SOURCE=.\portab.h
# End Source File
# Begin Source File

SOURCE=.\probe.h
# End Source File
# Begin Source File

SOURCE=.\rawaudio.h
# End Source File
# Begin Source File

SOURCE=.\rawimage.h
# End Source File
# Begin Source File

SOURCE=.\str.h
# End Source File
# Begin Source File

SOURCE=.\streams.h
# End Source File
# Begin Source File

SOURCE=.\subtitle.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\tools.h
# End Source File
# Begin Source File

SOURCE=.\video.h
# End Source File
# Begin Source File

SOURCE=.\vlc.h
# End Source File
# Begin Source File

SOURCE=.\waveout.h
# End Source File
# End Group
# Begin Group "DynCode"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dyncode\dyncode.c
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode.h
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_arm.c
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_arm.h
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_mips.c
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_mips.h
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_sh3.c
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_sh3.h
# End Source File
# Begin Source File

SOURCE=.\dyncode\dyncode_x86.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\association_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\context_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\dmo_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\file_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\mem_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\multithread_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\node_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\platform_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\str_win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\waveout_win32.c
# End Source File
# End Group
# Begin Group "Lang"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lang\lang_en.txt

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
TargetDir=\tcpmp\release
InputPath=..\lang\lang_en.txt
InputName=lang_en

"$(TargetDir)/$(InputName).txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" "$(TargetDir)"

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
TargetDir=\tcpmp\debug
InputPath=..\lang\lang_en.txt
InputName=lang_en

"$(TargetDir)/$(InputName).txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" "$(TargetDir)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\lang\lang_std.txt

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
TargetDir=\tcpmp\release
InputPath=..\lang\lang_std.txt
InputName=lang_std

"$(TargetDir)/$(InputName).txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" "$(TargetDir)"

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
TargetDir=\tcpmp\debug
InputPath=..\lang\lang_std.txt
InputName=lang_std

"$(TargetDir)/$(InputName).txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" "$(TargetDir)"

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "PalmOS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\palmos\association_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\context_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\dia.c
# End Source File
# Begin Source File

SOURCE=.\palmos\dia.h
# End Source File
# Begin Source File

SOURCE=.\palmos\file_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\filedb_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\mem_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\native.s
# End Source File
# Begin Source File

SOURCE=.\palmos\native86.c
# End Source File
# Begin Source File

SOURCE=.\palmos\node_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\pace.c
# End Source File
# Begin Source File

SOURCE=.\palmos\pace.h
# End Source File
# Begin Source File

SOURCE=.\palmos\platform_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\str_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\vfs_palmos.c
# End Source File
# Begin Source File

SOURCE=.\palmos\waveout_palmos.c
# End Source File
# End Group
# Begin Group "PlayList"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\playlist\asx.c
# End Source File
# Begin Source File

SOURCE=.\playlist\asx.h
# End Source File
# Begin Source File

SOURCE=.\playlist\m3u.c
# End Source File
# Begin Source File

SOURCE=.\playlist\m3u.h
# End Source File
# Begin Source File

SOURCE=.\playlist\pls.c
# End Source File
# Begin Source File

SOURCE=.\playlist\pls.h
# End Source File
# End Group
# Begin Group "Blit"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\blit\blit_arm_fix.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_gray.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_half.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_packed_yuv.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_rgb16.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_stretch.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_arm_yuv.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_mips_fix.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_mips_gray.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_mmx.asm

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\blit\blit_mmx.asm
InputName=blit_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\blit\blit_mmx.asm
InputName=blit_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\blit\blit_sh3_fix.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_sh3_gray.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_soft.c
# End Source File
# Begin Source File

SOURCE=.\blit\blit_soft.h
# End Source File
# Begin Source File

SOURCE=.\blit\blit_wmmx_fix.c
# End Source File
# End Group
# Begin Group "PCM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\pcm\pcm_arm.c
# End Source File
# Begin Source File

SOURCE=.\pcm\pcm_mips.c
# End Source File
# Begin Source File

SOURCE=.\pcm\pcm_sh3.c
# End Source File
# Begin Source File

SOURCE=.\pcm\pcm_soft.c
# End Source File
# Begin Source File

SOURCE=.\pcm\pcm_soft.h
# End Source File
# End Group
# Begin Group "SoftIDCT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\softidct\block.h
# End Source File
# Begin Source File

SOURCE=.\softidct\block_c.c
# End Source File
# Begin Source File

SOURCE=.\softidct\block_half.c
# End Source File
# Begin Source File

SOURCE=.\softidct\block_mips64.c
# End Source File
# Begin Source File

SOURCE=.\softidct\block_mx1.c
# End Source File
# Begin Source File

SOURCE=.\softidct\block_mx1.h
# End Source File
# Begin Source File

SOURCE=.\softidct\block_wmmx.c
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_arm.asm
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_arm.s
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_c.c
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_half.c
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_mmx.asm

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\softidct\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\softidct\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\softidct\idct_wmmx.asm
# End Source File
# Begin Source File

SOURCE=.\softidct\idct_wmmx.s
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp4x4_c.c
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp4x4_mmx.asm

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\softidct\mcomp4x4_mmx.asm
InputName=mcomp4x4_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\softidct\mcomp4x4_mmx.asm
InputName=mcomp4x4_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_arm.asm
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_arm.s
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_c.c
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_mips32.c
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_mips64.c
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_mmx.asm

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\softidct\mcomp_mmx.asm
InputName=mcomp_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\softidct\mcomp_mmx.asm
InputName=mcomp_mmx

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_wmmx.asm
# End Source File
# Begin Source File

SOURCE=.\softidct\mcomp_wmmx.s
# End Source File
# Begin Source File

SOURCE=.\softidct\softidct.c
# End Source File
# Begin Source File

SOURCE=.\softidct\softidct.h
# End Source File
# End Group
# Begin Group "CPU"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cpu\arm.asm
# End Source File
# Begin Source File

SOURCE=.\cpu\arm.s
# End Source File
# Begin Source File

SOURCE=.\cpu\cpu.c
# End Source File
# Begin Source File

SOURCE=.\cpu\cpu.h
# End Source File
# Begin Source File

SOURCE=.\cpu\mips.c
# End Source File
# Begin Source File

SOURCE=.\cpu\sh3.asm
# End Source File
# Begin Source File

SOURCE=.\cpu\x86.asm

!IF  "$(CFG)" == "common - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=.\cpu\x86.asm
InputName=x86

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=.\cpu\x86.asm
InputName=x86

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "ZLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Overlay"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\overlay\ddraw.h
# End Source File
# Begin Source File

SOURCE=.\overlay\ddrawce.h
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_console.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_ddraw.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_ddraw.h
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_ddrawce.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_direct.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_flytv.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_gapi.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_gdi.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_hires.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_raw.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_s1d13806.c
# End Source File
# Begin Source File

SOURCE=.\overlay\overlay_xscale.c
# End Source File
# End Group
# End Target
# End Project
