# Microsoft Developer Studio Project File - Name="ta_sql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ta_sql - Win32 Debug No Thread
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ta_sql.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ta_sql.mak" CFG="ta_sql - Win32 Debug No Thread"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ta_sql - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ta_sql - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "ta_sql - Win32 Release No Thread" (based on "Win32 (x86) Console Application")
!MESSAGE "ta_sql - Win32 Debug No Thread" (based on "Win32 (x86) Console Application")
!MESSAGE "ta_sql - Win32 DLL Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ta_sql - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\ta_sql"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "../../../../include" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ta_libc_cmr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /profile /machine:I386 /libpath:"..\..\..\..\lib"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\ta_sql"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "..\..\..\..\src\include" /I "../../../../include" /D "_CONSOLE" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ta_libc_cmd.lib wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libcpmtd.lib mysql++.lib ta_data_cmd.lib libcmtd.lib /nologo /subsystem:console /profile /debug /debugtype:both /machine:I386 /libpath:"..\..\..\..\lib" /libpath:"..\..\..\..\..\..\mysql++\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_sql___Win32_Release_No_Thread"
# PROP BASE Intermediate_Dir "ta_sql___Win32_Release_No_Thread"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\ta_sql"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "../../../../include" /D "_CONSOLE" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ta_libc_cmr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /profile /machine:I386 /libpath:"..\..\..\..\lib"
# ADD LINK32 ta_libc_csr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /profile /machine:I386 /libpath:"..\..\..\..\lib"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_sql___Win32_Debug_No_Thread"
# PROP BASE Intermediate_Dir "ta_sql___Win32_Debug_No_Thread"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\ta_sql"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "_CONSOLE" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /ZI /Od /I "../../../../include" /D "_CONSOLE" /D "TA_SINGLE_THREAD" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ta_libc_cmd.lib wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /debugtype:both /machine:I386 /libpath:"..\..\..\..\lib"
# ADD LINK32 ta_libc_csd.lib wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /profile /debug /debugtype:both /machine:I386 /libpath:"..\..\..\..\lib"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_sql___Win32_DLL_Release"
# PROP BASE Intermediate_Dir "ta_sql___Win32_DLL_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\bin"
# PROP Intermediate_Dir "..\..\..\..\temp\cdr\ta_sql"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /I "../../../../include" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../../../../include" /D "_CONSOLE" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ta_libc_cmr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /profile /machine:I386 /libpath:"..\..\..\..\lib"
# ADD LINK32 ta_libc_cdr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib /nologo /subsystem:console /profile /machine:I386 /libpath:"..\..\..\..\lib"

!ENDIF 

# Begin Target

# Name "ta_sql - Win32 Release"
# Name "ta_sql - Win32 Debug"
# Name "ta_sql - Win32 Release No Thread"
# Name "ta_sql - Win32 Debug No Thread"
# Name "ta_sql - Win32 DLL Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\demo\ta_sql\ta_sql.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Rebuild Dependency"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\lib\ta_libc_csd.lib

!IF  "$(CFG)" == "ta_sql - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\ta_libc_cmr.lib

!IF  "$(CFG)" == "ta_sql - Win32 Release"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\ta_libc_csr.lib

!IF  "$(CFG)" == "ta_sql - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\ta_libc_cmd.lib

!IF  "$(CFG)" == "ta_sql - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\lib\ta_libc_cdr.lib

!IF  "$(CFG)" == "ta_sql - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Release No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 Debug No Thread"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ta_sql - Win32 DLL Release"

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
