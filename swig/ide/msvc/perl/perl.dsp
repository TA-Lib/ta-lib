# Microsoft Developer Studio Project File - Name="perl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=perl - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "perl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "perl.mak" CFG="perl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "perl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\temp\perl\wrap"
# PROP Intermediate_Dir "..\..\..\temp\perl\wrap"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=df.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXAMPLE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O1 /I "..\..\..\..\c\include" /I "$(PERL5_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSWIN32" /D "_CONSOLE" /D "NO_STRICT" /D "PERL_MSVCRT_READFIX" /D "PERL_CAPI" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ..\..\..\..\c\lib\ta_libc_cdr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib $(PERL5_INCLUDE)/$(PERL5_LIB) /nologo /dll /pdb:none /machine:I386 /out:"..\..\..\lib\perl\ta.dll"
# Begin Target

# Name "perl - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\temp\perl\wrap\ta_libc_wrap.c
# ADD CPP /W3
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_abstract.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_data.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_func.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_libc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\c\include\ta_pm.h
# End Source File
# End Group
# Begin Group "test_perl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\tools\test_perl\runtests.pl
# End Source File
# Begin Source File

SOURCE=..\..\..\src\tools\test_perl\ta_abstract.t
# End Source File
# Begin Source File

SOURCE=..\..\..\src\tools\test_perl\ta_common.t
# End Source File
# Begin Source File

SOURCE=..\..\..\src\tools\test_perl\ta_defs.t
# End Source File
# Begin Source File

SOURCE=..\..\..\src\tools\test_perl\ta_func.t
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\src\interface\perl.pm
# End Source File
# Begin Source File

SOURCE=..\..\..\src\interface\ta_func.swg
# End Source File
# Begin Source File

SOURCE=..\..\..\src\interface\ta_libc.perl.swg
# End Source File
# Begin Source File

SOURCE=..\..\..\src\interface\ta_libc.swg
USERDEP__TA_LI="..\..\..\src\interface\perl.pm"	
# Begin Custom Build
InputDir=\Latest\TA-Lib\ta-lib\swig\src\interface
InputPath=..\..\..\src\interface\ta_libc.swg
InputName=ta_libc

BuildCmds= \
	echo In order to function correctly, please ensure the following environment variables are correctly set: \
	echo PERL5_INCLUDE: %PERL5_INCLUDE% \
	echo PERL5_LIB: %PERL5_LIB% \
	echo Make also sure that swig and perl are on search path: \
	echo %PATH% \
	echo on \
	swig -perl -module "Finance::TA" -proxy -Fmicrosoft -o ..\..\..\temp\perl\wrap\$(InputName)_wrap.c -I..\..\..\..\c\include -I$(InputDir) $(InputPath) \
	perl -pe "s/^\x25(OWNER|ITERATORS)/our \x25\1/" ..\..\..\temp\perl\wrap\TA.pm >..\..\..\lib\perl\Finance\TA.pm \
	type ..\..\..\src\interface\perl.pm >>..\..\..\lib\perl\Finance\TA.pm \
	

"..\..\..\temp\perl\wrap\$(InputName)_wrap.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"..\..\..\temp\perl\wrap\TA.pm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build
# End Source File
# End Target
# End Project
