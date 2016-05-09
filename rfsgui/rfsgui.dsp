# Microsoft Developer Studio Project File - Name="rfsgui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=rfsgui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rfsgui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rfsgui.mak" CFG="rfsgui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rfsgui - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "rfsgui - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "rfsgui - Win32 Retail64" (based on "Win32 (x86) Application")
!MESSAGE "rfsgui - Win32 Retail32" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rfsgui - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gi /GX /O2 /D "WIN32" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /FR /YX"precomp.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "rfsgui - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /FR /YX"precomp.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x417 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /incremental:no /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rfsgui - Win32 Retail64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rfsgui___Win32_Retail64"
# PROP BASE Intermediate_Dir "rfsgui___Win32_Retail64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Retail64"
# PROP Intermediate_Dir "Retail64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gi /GX /Gy /D "WIN32" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /FR /YX"precomp.h" /FD /c
# SUBTRACT BASE CPP /Gf
# ADD CPP /nologo /MD /W3 /Gm /Zi /O2 /D "WIN32" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /D "_WIN32" /FR /FD /Wp64 /GA /GL /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x417 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:IX86 /machine:AMD64 /LTCG
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "rfsgui - Win32 Retail32"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rfsgui___Win32_Retail32"
# PROP BASE Intermediate_Dir "rfsgui___Win32_Retail32"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Retail32"
# PROP Intermediate_Dir "Retail32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /Zi /O2 /D "WIN32" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /D "_WIN32" /FR /FD /Wp64 /GA /GL /c
# ADD CPP /nologo /MD /W3 /Gm /Zi /O2 /D "WIN32" /D "_MBCS" /D "STRICT" /D _WIN32_WINNT=0x500 /D "_WIN32" /FR /FD /GA /GL /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x417 /d "NDEBUG"
# ADD RSC /l 0x417 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:IX86 /machine:AMD64 /LTCG
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:IX86 /LTCG
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "rfsgui - Win32 Release"
# Name "rfsgui - Win32 Debug"
# Name "rfsgui - Win32 Retail64"
# Name "rfsgui - Win32 Retail32"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\RFSTOOL\gtools.cpp
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\pdrivefile.cpp
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\pdrivent.cpp
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\physicaldrive.cpp
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\precomp.cpp
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\reiserfs.cpp
# End Source File
# Begin Source File

SOURCE=.\rfsgui.cpp
# End Source File
# Begin Source File

SOURCE=.\rfsgui.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\RFSTOOL\gtools.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\ntdiskspec.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\pdrivefile.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\pdrivent.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\pdriveposix.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\physicaldrive.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\precomp.h
# End Source File
# Begin Source File

SOURCE=..\RFSTOOL\reiserfs.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\rfsgui.ico
# End Source File
# End Target
# End Project
