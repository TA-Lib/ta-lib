# Microsoft Developer Studio Project File - Name="ta_common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ta_common - Win32 CSD Single Thread Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ta_common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ta_common.mak" CFG="ta_common - Win32 CSD Single Thread Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ta_common - Win32 CDR Multithread DLL Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_common - Win32 CMD Multithread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_common - Win32 CSD Single Thread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_common - Win32 CSR Single Thread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_common - Win32 CMR Multithread Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ta_common - Win32 CDR Multithread DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_common___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Intermediate_Dir "ta_common___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cdr\ta_common"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cdr.lib"

!ELSEIF  "$(CFG)" == "ta_common - Win32 CMD Multithread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_common___Win32_CMD_Multithread_Debug"
# PROP BASE Intermediate_Dir "ta_common___Win32_CMD_Multithread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\ta_common"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cmd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cmd.lib"

!ELSEIF  "$(CFG)" == "ta_common - Win32 CSD Single Thread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_common___Win32_CSD_Single_Thread_Debug"
# PROP BASE Intermediate_Dir "ta_common___Win32_CSD_Single_Thread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\ta_common"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\src\ta_common" /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_common_csd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_common_csd.lib"

!ELSEIF  "$(CFG)" == "ta_common - Win32 CSR Single Thread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_common___Win32_CSR_Single_Thread_Release"
# PROP BASE Intermediate_Dir "ta_common___Win32_CSR_Single_Thread_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\ta_common"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_common_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_common_csr.lib"

!ELSEIF  "$(CFG)" == "ta_common - Win32 CMR Multithread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_common___Win32_CMR_Multithread_Release0"
# PROP BASE Intermediate_Dir "ta_common___Win32_CMR_Multithread_Release0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\ta_common"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common\imatix\sfl" /I "..\..\..\..\src\ta_common" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_common_cmr.lib"

!ENDIF 

# Begin Target

# Name "ta_common - Win32 CDR Multithread DLL Release"
# Name "ta_common - Win32 CMD Multithread Debug"
# Name "ta_common - Win32 CSD Single Thread Debug"
# Name "ta_common - Win32 CSR Single Thread Release"
# Name "ta_common - Win32 CMR Multithread Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\blocksort.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\BZLIB.C
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\COMPRESS.C
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\CRCTABLE.C
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\decompress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\kazlib\Dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\HUFFMAN.C
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\kazlib\list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\bzip2\randtable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sflcryp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sfldate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sfldir.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sflfile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sflstr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\imatix\sfl\sfltok.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_country_info.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_global.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_list.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_memory.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_retcode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_stream.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_string.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_system.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_timestamp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_trace.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\ta_version.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_common\trio\trionan.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
