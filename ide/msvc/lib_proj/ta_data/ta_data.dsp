# Microsoft Developer Studio Project File - Name="ta_data" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ta_data - Win32 DLL Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ta_data.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ta_data.mak" CFG="ta_data - Win32 DLL Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ta_data - Win32 CDR Multithread DLL Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 CMD Multithread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 CSD Single Thread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 CSR Single Thread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 CMR Multithread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 Profiling" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_data - Win32 DLL Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ta_data - Win32 CDR Multithread DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_data___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Intermediate_Dir "ta_data___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cdr\ta_data"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /WX /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cdr.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 CMD Multithread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_data___Win32_CMD_Multithread_Debug"
# PROP BASE Intermediate_Dir "ta_data___Win32_CMD_Multithread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\ta_data"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W3 /WX /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "TA_FUNC_NO_RANGE_CHECK" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmd.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 CSD Single Thread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_data___Win32_CSD_Single_Thread_Debug"
# PROP BASE Intermediate_Dir "ta_data___Win32_CSD_Single_Thread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\ta_data"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /WX /Gm /GX /Zi /Od /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csd.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 CSR Single Thread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_data___Win32_CSR_Single_Thread_Release"
# PROP BASE Intermediate_Dir "ta_data___Win32_CSR_Single_Thread_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\ta_data"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /WX /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csr.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 CMR Multithread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_data___Win32_CMR_Multithread_Release0"
# PROP BASE Intermediate_Dir "ta_data___Win32_CMR_Multithread_Release0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\ta_data"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /WX /O1 /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmr.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 Profiling"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_data___Win32_Profiling"
# PROP BASE Intermediate_Dir "ta_data___Win32_Profiling"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ta_data___Win32_Profiling"
# PROP Intermediate_Dir "ta_data___Win32_Profiling"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /WX /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_csr.lib"

!ELSEIF  "$(CFG)" == "ta_data - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_data___Win32_DLL_Debug"
# PROP BASE Intermediate_Dir "ta_data___Win32_DLL_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ta_data___Win32_DLL_Debug"
# PROP Intermediate_Dir "ta_data___Win32_DLL_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "TA_FUNC_NO_RANGE_CHECK" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /WX /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_data\ta_source\ta_readop" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_data" /I "..\..\..\..\src\ta_data\ta_source" /I "..\..\..\..\src\ta_data\ta_source\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_ascii" /I "..\..\..\..\src\ta_data\ta_history" /I "..\..\..\..\src\ta_data\ta_source\ta_simulator" /I "..\..\..\..\src\ta_data\ta_fileindex" /I "..\..\..\..\src\ta_data\ta_source\ta_yahoo" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "TA_FUNC_NO_RANGE_CHECK" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_data_cmd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_data_for_dll_debug.lib"

!ENDIF 

# Begin Target

# Name "ta_data - Win32 CDR Multithread DLL Release"
# Name "ta_data - Win32 CMD Multithread Debug"
# Name "ta_data - Win32 CSD Single Thread Debug"
# Name "ta_data - Win32 CSR Single Thread Release"
# Name "ta_data - Win32 CMR Multithread Release"
# Name "ta_data - Win32 Profiling"
# Name "ta_data - Win32 DLL Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_ascii\ta_ascii.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_ascii\ta_ascii_handle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_fileindex\ta_build_index.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\tools\gen_data\ta_daily_ref_0.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_data_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_fileindex\ta_fileindex.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_fileindex\ta_fileindex_priv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_fileindex\ta_fileindex_utils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_history\ta_history_builder.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_history\ta_historycheck.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\tools\gen_data\ta_intra_ref_0.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\tools\gen_data\ta_mrg_0.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_network.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_fileindex\ta_parse_path.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_history\ta_period.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_readop\ta_readop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_readop\ta_readop_do.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_readop\ta_readop_estalloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_readop\ta_readop_optimize.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_report.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_simulator\ta_simulator.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_source.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_token.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_webfetch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo_handle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo_historical.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo_idx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_yahoo\ta_yahoo_market.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_adddatasourceparam_priv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_data_udb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_network.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_readop\ta_readop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_source\ta_source.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_data\ta_token.h
# End Source File
# End Group
# End Target
# End Project
