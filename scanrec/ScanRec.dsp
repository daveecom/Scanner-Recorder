# Microsoft Developer Studio Project File - Name="ScanRec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ScanRec - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ScanRec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ScanRec.mak" CFG="ScanRec - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ScanRec - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ScanRec - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "ScanRec - Win32 RelDbg" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Windows/ScanRec", MIAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ScanRec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WIN32" /D "_WINDOWS" /FAs /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib version.lib msacm32.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "ScanRec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WIN32" /D "_WINDOWS" /FAs /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib version.lib msacm32.lib /nologo /subsystem:windows /incremental:no /map /debug /machine:I386

!ELSEIF  "$(CFG)" == "ScanRec - Win32 RelDbg"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ScanRec_"
# PROP BASE Intermediate_Dir "ScanRec_"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "RelDbg"
# PROP Intermediate_Dir "RelDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WIN32" /D "_WINDOWS" /FAs /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WIN32" /D "_WINDOWS" /FAs /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib version.lib msacm32.lib /nologo /subsystem:windows /map /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib winmm.lib version.lib msacm32.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "ScanRec - Win32 Release"
# Name "ScanRec - Win32 Debug"
# Name "ScanRec - Win32 RelDbg"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\Adar.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\Adar.rc
# End Source File
# Begin Source File

SOURCE=.\Adarclok.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\AdarIndx.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\Adarmmio.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\AdarOpts.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\AdarPch.cpp
# ADD CPP /Yc"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\Adarrec.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\Adarsql.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\AdarTime.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE=.\Adarutl.cpp
# ADD CPP /Yu"AdarPch.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\Program Files\MSDEV\include\MMREG.H"

!IF  "$(CFG)" == "ScanRec - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 RelDbg"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\Program Files\MSDEV\include\MMSYSTEM.H"

!IF  "$(CFG)" == "ScanRec - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 RelDbg"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\..\..\Program Files\MSDEV\include\MSACM.H"

!IF  "$(CFG)" == "ScanRec - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ScanRec - Win32 RelDbg"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Version.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\adar.h
# End Source File
# Begin Source File

SOURCE=.\AdarPch.h
# End Source File
# Begin Source File

SOURCE=.\Version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\about1.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\CloseButton.bmp
# End Source File
# Begin Source File

SOURCE=.\colon.bmp
# End Source File
# Begin Source File

SOURCE=.\digit0.bmp
# End Source File
# Begin Source File

SOURCE=.\digit1.bmp
# End Source File
# Begin Source File

SOURCE=.\digit2.bmp
# End Source File
# Begin Source File

SOURCE=.\digit3.bmp
# End Source File
# Begin Source File

SOURCE=.\digit4.bmp
# End Source File
# Begin Source File

SOURCE=.\digit5.bmp
# End Source File
# Begin Source File

SOURCE=.\digit6.bmp
# End Source File
# Begin Source File

SOURCE=.\digit7.bmp
# End Source File
# Begin Source File

SOURCE=.\digit8.bmp
# End Source File
# Begin Source File

SOURCE=.\digit9.bmp
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\ExitButton.bmp
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\FileButton.bmp
# End Source File
# Begin Source File

SOURCE=.\Icon1.ico
# End Source File
# Begin Source File

SOURCE=.\icon3.ico
# End Source File
# Begin Source File

SOURCE=.\icon4.ico
# End Source File
# Begin Source File

SOURCE=.\icon5.ico
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\icon5.ico
# End Source File
# Begin Source File

SOURCE=.\iconrec.ico
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\Panel.bmp
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\RecordButton.bmp
# End Source File
# Begin Source File

SOURCE=.\scanrec.bmp
# End Source File
# Begin Source File

SOURCE=.\ScanrecButtons\StopButton.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\scanrec.wav
# End Source File
# Begin Source File

SOURCE=.\test.txt
# End Source File
# End Target
# End Project
