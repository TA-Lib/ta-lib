# Microsoft Developer Studio Project File - Name="gen_rdata" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=gen_rdata - Win32 CSD Single Thread Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gen_rdata.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gen_rdata.mak" CFG="gen_rdata - Win32 CSD Single Thread Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gen_rdata - Win32 CMD Multithread Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "gen_rdata - Win32 CSD Single Thread Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "gen_rdata - Win32 CSR Single Thread Release" (based on "Win32 (x86) Console Application")
!MESSAGE "gen_rdata - Win32 CMR Multithread Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gen_rdata - Win32 CMD Multithread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gen_rdata___Win32_CMD_Multithread_Debug"
# PROP BASE Intermediate_Dir "gen_rdata___Win32_CMD_Multithread_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\gen_rdata"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Zi /Od /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /incremental:no /debug /debugtype:both /machine:I386
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /incremental:no /debug /debugtype:both /machine:I386
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "gen_rdata - Win32 CSD Single Thread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "gen_rdata___Win32_CSD_Single_Thread_Debug"
# PROP BASE Intermediate_Dir "gen_rdata___Win32_CSD_Single_Thread_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\gen_rdata"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /Zi /Od /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /Gm /Zi /Od /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /incremental:no /debug /debugtype:both /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /incremental:no /debug /debugtype:both /machine:I386

!ELSEIF  "$(CFG)" == "gen_rdata - Win32 CSR Single Thread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gen_rdata___Win32_CSR_Single_Thread_Release"
# PROP BASE Intermediate_Dir "gen_rdata___Win32_CSR_Single_Thread_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\gen_rdata"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /O2 /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "gen_rdata - Win32 CMR Multithread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "gen_rdata___Win32_CMR_Multithread_Release0"
# PROP BASE Intermediate_Dir "gen_rdata___Win32_CMR_Multithread_Release0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\gen_rdata"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /O1 /I "../../../../include" /I "../../../../src/ta_data/ta_source/ta_yahoo" /I "../../../../src/ta_data/ta_source" /I "../../../../src/ta_data" /I "../../../../src/ta_common" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /machine:I386

!ENDIF 

# Begin Target

# Name "gen_rdata - Win32 CMD Multithread Debug"
# Name "gen_rdata - Win32 CSD Single Thread Debug"
# Name "gen_rdata - Win32 CSR Single Thread Release"
# Name "gen_rdata - Win32 CMR Multithread Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\tools\gen_rdata\gen_rdata.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo_priv.h
# End Source File
# End Group
# End Target
# End Project
