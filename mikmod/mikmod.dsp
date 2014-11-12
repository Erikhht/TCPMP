# Microsoft Developer Studio Project File - Name="mikmod" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mikmod - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mikmod.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mikmod.mak" CFG="mikmod - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mikmod - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mikmod - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mikmod - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MIKMOD_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zd /O2 /I "libmikmod/include" /D "NDEBUG" /D "_USRDLL" /D "MIKMOD_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../release/mikmod.plg"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "mikmod - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MIKMOD_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "libmikmod/include" /D "_DEBUG" /D "_USRDLL" /D "MIKMOD_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../debug/mikmod.plg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "mikmod - Win32 Release"
# Name "mikmod - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\mikmod.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\mikmod.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "libmikmod"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libmikmod\drivers\drv_nos.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_669.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_amf.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_asy.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_dsm.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_far.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_gdm.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_imf.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_it.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_m15.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_med.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_mod.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_mtm.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_okt.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_s3m.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_stm.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_stx.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_ult.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_uni.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\loaders\load_xm.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mdriver.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mloader.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mlreg.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mlutil.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\mmio\mmerror.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\mmio\mmio.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mplayer.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\munitrk.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\mwav.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\npertab.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\sloader.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\virtch.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\virtch2.c
# End Source File
# Begin Source File

SOURCE=.\libmikmod\playercode\virtch_common.c
# End Source File
# End Group
# End Target
# End Project
