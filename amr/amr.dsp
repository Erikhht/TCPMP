# Microsoft Developer Studio Project File - Name="amr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=amr - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "amr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "amr.mak" CFG="amr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "amr - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "amr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "amr - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "amr_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zd /O2 /D "NDEBUG" /D inline=__inline /D "_USRDLL" /D "AMR_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../release/amr.plg"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "amr - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "AMR_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /D "_DEBUG" /D inline=__inline /D "_USRDLL" /D "AMR_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../debug/amr.plg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "amr - Win32 Release"
# Name "amr - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\amrnb.c
# End Source File
# Begin Source File

SOURCE=.\amrwb.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\amrnb.h
# End Source File
# Begin Source File

SOURCE=.\amrwb.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "26104"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\26104\interf_dec.c
# End Source File
# Begin Source File

SOURCE=.\26104\interf_dec.h
# End Source File
# Begin Source File

SOURCE=.\26104\rom_dec.h
# End Source File
# Begin Source File

SOURCE=.\26104\rom_enc.h
# End Source File
# Begin Source File

SOURCE=.\26104\sp_dec.c
# End Source File
# Begin Source File

SOURCE=.\26104\sp_dec.h
# End Source File
# Begin Source File

SOURCE=.\26104\typedef.h
# End Source File
# End Group
# Begin Group "26204"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\26204\dec_acelp.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_dtx.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_gain.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_if.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_lpc.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_main.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_rom.c
# End Source File
# Begin Source File

SOURCE=.\26204\dec_util.c
# End Source File
# Begin Source File

SOURCE=.\26204\if_rom.c
# End Source File
# End Group
# End Target
# End Project
