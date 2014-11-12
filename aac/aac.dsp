# Microsoft Developer Studio Project File - Name="aac" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=aac - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aac.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aac.mak" CFG="aac - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aac - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "aac - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aac - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AAC_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zd /O2 /D "NDEBUG" /D "LC_ONLY_DECODER" /D "FIXED_POINT" /D "_USRDLL" /D "AAC_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../release/aac.plg"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "aac - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AAC_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /D "_DEBUG" /D "LC_ONLY_DECODER" /D "FIXED_POINT" /D "_USRDLL" /D "AAC_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../debug/aac.plg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "aac - Win32 Release"
# Name "aac - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\faad.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\faad.h
# End Source File
# Begin Source File

SOURCE=.\faad2\include\neaacdec.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "libfaad"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\faad2\libfaad\bits.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\cfft.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\common.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\decoder.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\drc.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\drm_dec.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\filtbank.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\hcr.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\huffman.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ic_predict.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\is.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\lt_predict.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\mdct.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\mp4.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ms.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\output.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\pns.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ps_dec.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ps_syntax.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\pulse.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\rvlc.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_dct.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_dec.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_e_nf.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_fbt.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_hfadj.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_hfgen.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_huff.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_qmf.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_syntax.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\sbr_tf_grid.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\specrec.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ssr.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ssr_fb.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\ssr_ipqf.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\syntax.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\faad2\libfaad\tns.c
# ADD CPP /W1
# End Source File
# End Group
# Begin Group "libpaac"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libpaac\aac_imdct.c
# End Source File
# Begin Source File

SOURCE=.\libpaac\aac_imdct.h
# End Source File
# End Group
# End Target
# End Project
