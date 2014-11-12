# Microsoft Developer Studio Project File - Name="speex" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=speex - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "speex.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "speex.mak" CFG="speex - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "speex - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "speex - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "speex - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "speex_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /GX /Zd /O2 /I "speex/include" /D "NDEBUG" /D inline=__inline /D "FIXED_POINT" /D "_USRDLL" /D "SPEEX_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "NDEBUG"
# ADD RSC /l 0x40e /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map /machine:I386 /out:"../release/speex.plg"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "speex - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SPEEX_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "speex/include" /D "_DEBUG" /D inline=__inline /D "FIXED_POINT" /D "_USRDLL" /D "SPEEX_EXPORTS" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40e /d "_DEBUG"
# ADD RSC /l 0x40e /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../debug/speex.plg" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "speex - Win32 Release"
# Name "speex - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\speexcodec.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\speex.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "speex"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\speex\libspeex\arch.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\bits.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\cb_search.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\cb_search.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\cb_search_arm4.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\cb_search_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\cb_search_sse.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_10_16_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_10_32_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_20_32_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_5_256_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_5_64_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\exc_8_128_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\filters.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\filters.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\filters_arm4.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\filters_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\filters_sse.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\fixed_arm4.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\fixed_arm5e.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\fixed_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\fixed_debug.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\fixed_generic.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\gain_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\gain_table_lbr.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\hexc_10_32_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\hexc_table.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\high_lsp_tables.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lbr_48k_tables.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lpc.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lpc.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lpc_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lsp.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lsp.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\lsp_tables_nb.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\ltp.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\ltp.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\ltp_arm4.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\ltp_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\ltp_sse.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\math_approx.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\math_approx.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\misc.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\misc.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\misc_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\modes.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\modes.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\nb_celp.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\nb_celp.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\quant_lsp.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\quant_lsp.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\sb_celp.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\sb_celp.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\speex.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\speex_callbacks.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\speex_header.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\stack_alloc.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\stereo.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vbr.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vbr.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vq.c
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vq.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vq_arm4.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vq_bfin.h
# End Source File
# Begin Source File

SOURCE=.\speex\libspeex\vq_sse.h
# End Source File
# End Group
# End Target
# End Project
