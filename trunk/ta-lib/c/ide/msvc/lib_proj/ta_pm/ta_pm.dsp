# Microsoft Developer Studio Project File - Name="ta_pm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ta_pm - Win32 CSD Single Thread Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ta_pm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ta_pm.mak" CFG="ta_pm - Win32 CSD Single Thread Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ta_pm - Win32 CDR Multithread DLL Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_pm - Win32 CMD Multithread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_pm - Win32 CSD Single Thread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_pm - Win32 CSR Single Thread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_pm - Win32 CMR Multithread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_pm - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ta_pm - Win32 CDR Multithread DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_pm___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Intermediate_Dir "ta_pm___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cdr\ta_pm"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cdr.lib"

!ELSEIF  "$(CFG)" == "ta_pm - Win32 CMD Multithread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_pm___Win32_CMD_Multithread_Debug"
# PROP BASE Intermediate_Dir "ta_pm___Win32_CMD_Multithread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\ta_pm"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cmd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cmd.lib"

!ELSEIF  "$(CFG)" == "ta_pm - Win32 CSD Single Thread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_pm___Win32_CSD_Single_Thread_Debug"
# PROP BASE Intermediate_Dir "ta_pm___Win32_CSD_Single_Thread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\ta_pm"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csd.lib"

!ELSEIF  "$(CFG)" == "ta_pm - Win32 CSR Single Thread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_pm___Win32_CSR_Single_Thread_Release"
# PROP BASE Intermediate_Dir "ta_pm___Win32_CSR_Single_Thread_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\ta_pm"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csr.lib"

!ELSEIF  "$(CFG)" == "ta_pm - Win32 CMR Multithread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_pm___Win32_CMR_Multithread_Release0"
# PROP BASE Intermediate_Dir "ta_pm___Win32_CMR_Multithread_Release0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\ta_pm"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_pm\ta_source" /I "..\..\..\..\src\ta_pm\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_ascii" /I "..\..\..\..\src\ta_pm\ta_history" /I "..\..\..\..\src\ta_pm\ta_source\ta_simulator" /I "..\..\..\..\src\ta_pm\ta_fileindex" /I "..\..\..\..\src\ta_pm\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_cmr.lib"

!ELSEIF  "$(CFG)" == "ta_pm - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_pm___Win32_Profiling"
# PROP BASE Intermediate_Dir "ta_pm___Win32_Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ta_pm___Win32_Profiling"
# PROP Intermediate_Dir "ta_pm___Win32_Profiling"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_pm\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_pm" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_pm_csr.lib"

!ENDIF 

# Begin Target

# Name "ta_pm - Win32 CDR Multithread DLL Release"
# Name "ta_pm - Win32 CMD Multithread Debug"
# Name "ta_pm - Win32 CSD Single Thread Debug"
# Name "ta_pm - Win32 CSR Single Thread Release"
# Name "ta_pm - Win32 CMR Multithread Release"
# Name "ta_pm - Win32 Profiling"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_datalog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_instrument.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_pm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_pmarray.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_pmreport.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_pmstring.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_pm\ta_pmvalue.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
