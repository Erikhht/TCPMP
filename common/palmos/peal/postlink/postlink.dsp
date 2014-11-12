# Microsoft Developer Studio Project File - Name="postlink" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=postlink - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "postlink.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "postlink.mak" CFG="postlink - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "postlink - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "postlink - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "postlink - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f postlink.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "postlink.exe"
# PROP BASE Bsc_Name "postlink.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "make -f "Makefile""
# PROP Rebuild_Opt "all"
# PROP Target_File "peal-postlink.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "postlink - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "postlink___Win32_Debug"
# PROP BASE Intermediate_Dir "postlink___Win32_Debug"
# PROP BASE Cmd_Line "NMAKE /f postlink.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "postlink.exe"
# PROP BASE Bsc_Name "postlink.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "postlink___Win32_Debug"
# PROP Intermediate_Dir "postlink___Win32_Debug"
# PROP Cmd_Line "make -f "Makefile""
# PROP Rebuild_Opt "all"
# PROP Target_File "peal-postlink.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "postlink - Win32 Release"
# Name "postlink - Win32 Debug"

!IF  "$(CFG)" == "postlink - Win32 Release"

!ELSEIF  "$(CFG)" == "postlink - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\complain.cc
# End Source File
# Begin Source File

SOURCE=.\complain.h
# End Source File
# Begin Source File

SOURCE=.\elf.h
# End Source File
# Begin Source File

SOURCE=.\elf32.h
# End Source File
# Begin Source File

SOURCE=.\elf_common.h
# End Source File
# Begin Source File

SOURCE=.\got.h
# End Source File
# Begin Source File

SOURCE=.\image.cc
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\postlinker.cc
# End Source File
# Begin Source File

SOURCE=.\postlinker.h
# End Source File
# Begin Source File

SOURCE=.\relocation.cc
# End Source File
# Begin Source File

SOURCE=.\relocation.h
# End Source File
# Begin Source File

SOURCE=.\section.cc
# End Source File
# Begin Source File

SOURCE=.\section.h
# End Source File
# Begin Source File

SOURCE=.\stringtable.h
# End Source File
# Begin Source File

SOURCE=.\swap.h
# End Source File
# Begin Source File

SOURCE=.\symbol.cc
# End Source File
# Begin Source File

SOURCE=.\symbol.h
# End Source File
# Begin Source File

SOURCE=.\symboltable.cc
# End Source File
# Begin Source File

SOURCE=.\symboltable.h
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
