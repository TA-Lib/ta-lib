# Microsoft Developer Studio Project File - Name="ta_abstract" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ta_abstract - Win32 CSD Single Thread Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ta_abstract.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ta_abstract.mak" CFG="ta_abstract - Win32 CSD Single Thread Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ta_abstract - Win32 CDR Multithread DLL Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_abstract - Win32 CMD Multithread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_abstract - Win32 CSD Single Thread Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_abstract - Win32 CSR Single Thread Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ta_abstract - Win32 CMR Multithread Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ta_abstract - Win32 CDR Multithread DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_abstract___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Intermediate_Dir "ta_abstract___Win32_CDR_Multithread_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cdr\ta_abstract"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cdr.lib"

!ELSEIF  "$(CFG)" == "ta_abstract - Win32 CMD Multithread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_abstract___Win32_CMD_Multithread_Debug"
# PROP BASE Intermediate_Dir "ta_abstract___Win32_CMD_Multithread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmd\ta_abstract"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MTd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_DEBUG" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cmd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cmd.lib"

!ELSEIF  "$(CFG)" == "ta_abstract - Win32 CSD Single Thread Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ta_abstract___Win32_CSD_Single_Thread_Debug"
# PROP BASE Intermediate_Dir "ta_abstract___Win32_CSD_Single_Thread_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csd\ta_abstract"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_DEBUG" /D "TA_SINGLE_THREAD" /D "WIN32" /D "_DEBUG" /D "_MBCS" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_csd.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_csd.lib"

!ELSEIF  "$(CFG)" == "ta_abstract - Win32 CSR Single Thread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_abstract___Win32_CSR_Single_Thread_Release"
# PROP BASE Intermediate_Dir "ta_abstract___Win32_CSR_Single_Thread_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\csr\ta_abstract"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /W3 /O2 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "TA_SINGLE_THREAD" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_csr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_csr.lib"

!ELSEIF  "$(CFG)" == "ta_abstract - Win32 CMR Multithread Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ta_abstract___Win32_CMR_Multithread_Release0"
# PROP BASE Intermediate_Dir "ta_abstract___Win32_CMR_Multithread_Release0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\lib"
# PROP Intermediate_Dir "..\..\..\..\temp\cmr\ta_abstract"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\..\..\include" /I "..\..\..\..\src\ta_common" /I "..\..\..\..\src\ta_abstract\frames" /I "..\..\..\..\src\ta_abstract" /D "_LIB" /D "WIN32" /D "NDEBUG" /D "_MBCS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cmr.lib"
# ADD LIB32 /nologo /out:"..\..\..\..\lib\ta_abstract_cmr.lib"

!ENDIF 

# Begin Target

# Name "ta_abstract - Win32 CDR Multithread DLL Release"
# Name "ta_abstract - Win32 CMD Multithread Debug"
# Name "ta_abstract - Win32 CSD Single Thread Debug"
# Name "ta_abstract - Win32 CSR Single Thread Release"
# Name "ta_abstract - Win32 CMR Multithread Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\ta_abstract.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_APO_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_ATR_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_BBANDS_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_CCI_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\ta_def_ui.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_DEMA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_EMA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\ta_group_idx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MACD_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MACDFIX_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MAX_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MEDPRICE_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MIN_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_MOM_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_OBV_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_PPO_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_ROC_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_ROCR_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_RSI_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_SAR_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_SMA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_STDDEV_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_STOCH_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_TEMA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_TRANGE_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_TRIX_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_TYPPRICE_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_VAR_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_WCLPRICE_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\frames\ta_WMA_frame.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_a.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_b.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_c.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_d.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_e.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_f.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_g.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_h.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_i.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_j.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_k.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_l.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_m.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_n.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_o.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_p.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_q.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_r.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_s.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_t.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_u.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_v.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_w.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_x.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_y.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\ta_abstract\tables\table_z.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
