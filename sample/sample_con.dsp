# Microsoft Developer Studio Project File - Name="sample_con" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sample_con - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sample_con.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sample_con.mak" CFG="sample_con - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sample_con - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "sample_con - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sample_con - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zd /O2 /I "../ffmpeg/libavutil" /D "NDEBUG" /D "ASO_INTERLEAVE1" /D "TCPMP" /D "EMULATE_INTTYPES" /D inline=__inline /D "NO_LANG" /D "NO_PLUGINS" /D "LC_ONLY_DECODER" /D "FIXED_POINT" /D "COMMON_EXPORTS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /out:"../release/sample_con.exe"

!ELSEIF  "$(CFG)" == "sample_con - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sample_con___Win32_Debug"
# PROP BASE Intermediate_Dir "sample_con___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../ffmpeg/libavutil" /D "_DEBUG" /D "TCPMP" /D "EMULATE_INTTYPES" /D inline=__inline /D "NO_LANG" /D "NO_PLUGINS" /D "LC_ONLY_DECODER" /D "FIXED_POINT" /D "COMMON_EXPORTS" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /debug /machine:I386 /out:"../debug/sample_con.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sample_con - Win32 Release"
# Name "sample_con - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Group "PCM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\pcm\pcm_soft.c
# End Source File
# Begin Source File

SOURCE=..\common\pcm\pcm_soft.h
# End Source File
# End Group
# Begin Group "Blit"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\blit\blit_soft.c
# End Source File
# Begin Source File

SOURCE=..\common\blit\blit_soft.h
# End Source File
# End Group
# Begin Group "CPU"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\cpu\cpu.c
# End Source File
# Begin Source File

SOURCE=..\common\cpu\cpu.h
# End Source File
# Begin Source File

SOURCE=..\common\cpu\x86.asm

!IF  "$(CFG)" == "sample_con - Win32 Release"

# Begin Custom Build
IntDir=.\Release
InputPath=..\common\cpu\x86.asm
InputName=x86

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "sample_con - Win32 Debug"

# Begin Custom Build
IntDir=.\Debug
InputPath=..\common\cpu\x86.asm
InputName=x86

"$(IntDir)/$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -o "$(IntDir)/$(InputName).obj" $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Overlay"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\overlay\overlay_console.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\win32\association_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\context_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\dmo_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\file_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\mem_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\multithread_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\platform_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\str_win32.c
# End Source File
# Begin Source File

SOURCE=..\common\win32\waveout_win32.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\advanced.c
# End Source File
# Begin Source File

SOURCE=..\common\audio.c
# End Source File
# Begin Source File

SOURCE=..\common\buffer.c
# End Source File
# Begin Source File

SOURCE=..\common\buffer.h
# End Source File
# Begin Source File

SOURCE=..\common\codec.c
# End Source File
# Begin Source File

SOURCE=..\common\color.c
# End Source File
# Begin Source File

SOURCE=..\common\context.c
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode.c
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode.h
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_arm.c
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_arm.h
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_mips.c
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_mips.h
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_sh3.c
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_sh3.h
# End Source File
# Begin Source File

SOURCE=..\common\dyncode\dyncode_x86.c
# End Source File
# Begin Source File

SOURCE=..\common\equalizer.c
# End Source File
# Begin Source File

SOURCE=..\common\flow.c
# End Source File
# Begin Source File

SOURCE=..\common\format.c
# End Source File
# Begin Source File

SOURCE=..\common\format_base.c
# End Source File
# Begin Source File

SOURCE=..\common\format_subtitle.c
# End Source File
# Begin Source File

SOURCE=..\common\id3tag.c
# End Source File
# Begin Source File

SOURCE=..\common\idct.c
# End Source File
# Begin Source File

SOURCE=..\common\node.c
# End Source File
# Begin Source File

SOURCE=..\common\nulloutput.c
# End Source File
# Begin Source File

SOURCE=..\common\overlay.c
# End Source File
# Begin Source File

SOURCE=..\common\platform.c
# End Source File
# Begin Source File

SOURCE=..\common\player.c
# End Source File
# Begin Source File

SOURCE=..\common\playlist.c
# End Source File
# Begin Source File

SOURCE=..\common\playlist.h
# End Source File
# Begin Source File

SOURCE=..\common\probe.c
# End Source File
# Begin Source File

SOURCE=..\common\rawaudio.c
# End Source File
# Begin Source File

SOURCE=..\common\str.c
# End Source File
# Begin Source File

SOURCE=..\common\streams.c
# End Source File
# Begin Source File

SOURCE=..\common\timer.c
# End Source File
# Begin Source File

SOURCE=..\common\tools.c
# End Source File
# Begin Source File

SOURCE=..\common\helper_video.c
# End Source File
# Begin Source File

SOURCE=..\common\helper_base.c
# End Source File
# Begin Source File

SOURCE=..\common\video.c
# End Source File
# Begin Source File

SOURCE=..\common\vlc.c
# End Source File
# Begin Source File

SOURCE=..\common\waveout.c
# End Source File
# End Group
# Begin Group "Splitter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\splitter\mov.c
# End Source File
# Begin Source File

SOURCE=..\splitter\mov.h
# End Source File
# End Group
# Begin Group "AAC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\aac\libpaac\aac_imdct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\analysis.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\bits.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\bits.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\cfft.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\cfft.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\cfft_tab.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\common.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\common.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\decoder.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\decoder.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\drc.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\drc.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\drm_dec.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\drm_dec.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\error.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\error.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\filtbank.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\filtbank.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\fixed.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\hcr.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\huffman.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\huffman.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ic_predict.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ic_predict.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\iq_table.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\is.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\is.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\kbd_win.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\lt_predict.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\lt_predict.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\mdct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\mdct.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\mdct_tab.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\mp4.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\mp4.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ms.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ms.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\output.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\output.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\pns.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\pns.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ps_dec.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ps_dec.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ps_syntax.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ps_tables.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\pulse.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\pulse.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\rvlc.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\rvlc.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_dct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_dct.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_dec.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_dec.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_e_nf.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_e_nf.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_fbt.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_fbt.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_hfadj.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_hfadj.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_hfgen.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_hfgen.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_huff.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_huff.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_noise.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_qmf.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_qmf.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_qmf_c.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_syntax.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_syntax.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_tf_grid.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sbr_tf_grid.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\sine_win.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\specrec.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\specrec.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr_fb.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr_fb.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr_ipqf.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr_ipqf.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\ssr_win.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\structs.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\syntax.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\syntax.h
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\tns.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\aac\faad2\libfaad\tns.h
# End Source File
# End Group
# Begin Group "libavcodec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\avcodec.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\bitstream.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\bitstream.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\cabac.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\cabac.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\dsputil.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\dsputil.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\error_resilience.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\golomb.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\h264.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\h264idct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\jrevdct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\mem.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\mpeg12.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\mpegvideo.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\parser.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\simple_idct.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\utils.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavcodec\vp3dsp.c
# ADD CPP /w /W0
# End Source File
# End Group
# Begin Group "libavutil"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ffmpeg\libavutil\avutil.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\common.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\integer.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\integer.h
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\mathematics.c
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\rational.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\ffmpeg\libavutil\rational.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\ffmpeg\ffmpeg.c
# End Source File
# Begin Source File

SOURCE=.\sample_con.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
