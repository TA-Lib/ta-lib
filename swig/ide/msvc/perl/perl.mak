# Microsoft Developer Studio Generated NMAKE File, Based on perl.dsp
!IF "$(CFG)" == ""
CFG=perl - Win32 Release
!MESSAGE No configuration specified. Defaulting to perl - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "perl - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "perl.mak" CFG="perl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "perl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
OUTDIR=.\..\..\..\temp\perl\wrap
INTDIR=.\..\..\..\temp\perl\wrap

ALL : "..\..\..\temp\perl\wrap\ta_libc_wrap.c" "..\..\..\temp\perl\wrap\TA.pm" "..\..\..\lib\perl\ta.dll"


CLEAN :
	-@erase "$(INTDIR)\ta_libc_wrap.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ta.exp"
	-@erase "$(OUTDIR)\ta.lib"
	-@erase "..\..\..\lib\perl\ta.dll"
	-@erase "..\..\..\temp\perl\wrap\TA.pm"
	-@erase "..\..\..\temp\perl\wrap\ta_libc_wrap.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

F90=df.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\perl.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\..\..\c\lib\ta_libc_cdr.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib $(PERL5_INCLUDE)/$(PERL5_LIB) /nologo /dll /pdb:none /machine:I386 /out:"..\..\..\lib\perl\ta.dll" /implib:"$(OUTDIR)\ta.lib" 
LINK32_OBJS= \
	"$(INTDIR)\ta_libc_wrap.obj"

"..\..\..\lib\perl\ta.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /MD /W3 /O1 /I "..\..\..\..\c\include" /I "$(PERL5_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSWIN32" /D "_CONSOLE" /D "NO_STRICT" /D "PERL_MSVCRT_READFIX" /D "PERL_CAPI" /Fp"$(INTDIR)\perl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("perl.dep")
!INCLUDE "perl.dep"
!ELSE 
!MESSAGE Warning: cannot find "perl.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "perl - Win32 Release"
SOURCE="$(INTDIR)\ta_libc_wrap.c"
CPP_SWITCHES=/nologo /MD /W3 /O1 /I "..\..\..\..\c\include" /I "$(PERL5_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MSWIN32" /D "_CONSOLE" /D "NO_STRICT" /D "PERL_MSVCRT_READFIX" /D "PERL_CAPI" /Fp"$(INTDIR)\perl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\ta_libc_wrap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=..\..\..\src\interface\ta_libc.swg
InputDir=..\..\..\src\interface
InputPath=..\..\..\src\interface\ta_libc.swg
InputName=ta_libc
USERDEP__TA_LI="..\..\..\src\interface\perl.pm"	

"..\..\..\temp\perl\wrap\ta_libc_wrap.c"	"..\..\..\temp\perl\wrap\TA.pm" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__TA_LI)
	<<tempfile.bat 
	@echo off 
	echo In order to function correctly, please ensure the following environment variables are correctly set: 
	echo PERL5_INCLUDE: %PERL5_INCLUDE% 
	echo PERL5_LIB: %PERL5_LIB% 
	echo Make also sure that swig and perl are on search path: 
	echo %PATH% 
	echo on 
	swig -perl -module "Finance::TA" -proxy -Fmicrosoft -o ..\..\..\temp\perl\wrap\$(InputName)_wrap.c -I..\..\..\..\c\include -I$(InputDir) $(InputPath) 
	perl -pe "s/^\x25(OWNER|ITERATORS)/our \x25\1/" ..\..\..\temp\perl\wrap\TA.pm >..\..\..\lib\perl\Finance\TA.pm 
	type ..\..\..\src\interface\perl.pm >>..\..\..\lib\perl\Finance\TA.pm
<< 
	

!ENDIF 

