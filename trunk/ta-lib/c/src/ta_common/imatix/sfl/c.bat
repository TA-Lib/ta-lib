@echo off
goto start
:help
echo.
echo  C script - iMatix C compile script
echo  Written: 98/01/27  Pieter Hintjens
echo  Revised: 99/07/23  Pieter Hintjens
echo.
echo    c  file1 file2...     Compile ANSI C program(s)
echo    c -l main...          Compile and link main program(s)
echo    c -L main...          Link main program(s), no compile
echo    c -S                  Report detected compiler options
echo    c -r library file...  Replace object file in library (if library
echo                          name is 'any', inserts into first .lib file
echo.
echo  You can optionally set these environment variables in autoexec.bat:
echo.
echo  CCNAME - compiler name: bc, bcc, tc, tcc, msvc, lcc, or gcc or wcc;
echo           if not defined, script assumes 'lcc'.
echo  CCDIR  - fully qualified compiler directory: if not defined, script
echo           looks in \bc, \bcc, \borlandc, \tc, \tcc, \turboc, \msdev,
echo           \msvc, \lcc, \gcc, and %WATCOM% in the current drive.
echo  INCDEF - value for compiler '-I' option; if not defined, script uses
echo           compiler include directory followed by \usr\include.
echo  LIBDIR - directory containing user libraries; if not defined, script
echo           uses \usr\lib in the current drive.
goto exit

:start
:-
:-  Clean-up symbols and directory
:-
set lib=
if exist *.map del *.map
if exist *.lst del *.lst

:-
:-  Determine compiler name and location, and directories to use
:-
    if "%CCNAME%"==""     set CCNAME=lcc
    if "%CCNAME%"=="lcc"  goto start_lcc
    if "%CCNAME%"=="bc"   goto start_bc
    if "%CCNAME%"=="bcc"  goto start_bc
    if "%CCNAME%"=="tc"   goto start_tc
    if "%CCNAME%"=="tcc"  goto start_tc
    if "%CCNAME%"=="msvc" goto start_msvc
    if "%CCNAME%"=="gcc"  goto start_gcc
    if "%CCNAME%"=="wcc"  goto start_wcc
    echo Unknown compiler '%CCNAME%' defined - aborting
    goto exit

:-
:-  Borland C/C++
:-
:start_bc
    if not "%CCDIR%"=="" goto sbc_1
    set CCDIR=\bc
    if exist %CCDIR%\bin\%CCNAME%.exe  goto stc_2
    set CCDIR=\bcc
    if exist %CCDIR%\bin\%CCNAME%.exe  goto stc_2
    set CCDIR=\borlandc
    if exist %CCDIR%\bin\%CCNAME%.exe  goto stc_2
    echo Borland C/C++ compiler not found in default directories - aborting
    goto exit
:sbc_1
    if exist %CCDIR%\bin\%CCNAME%.exe  goto stc_2
    echo Borland C/C++ compiler not found in defined directory - aborting
    goto exit

:-
:-  Turbo C/C++
:-
:start_tc
    if not "%CCDIR%"=="" goto stc_1
    set CCDIR=\tc
    if exist %CCDIR%\bin\%CCNAME%.exe goto stc_2
    set CCDIR=\tcc
    if exist %CCDIR%\bin\%CCNAME%.exe goto stc_2
    set CCDIR=\turboc
    if exist %CCDIR%\bin\%CCNAME%.exe goto stc_2
    echo Turbo C/C++ compiler not found in default directories - aborting
:stc_1
    if exist %CCDIR%\bin\%CCNAME%.exe goto stc_2
    echo Turbo C/C++ compiler not found in defined directory - aborting
    goto exit

:stc_2
    if "%INCDEF%"=="" set INCDEF=%CCDIR%\include -I\usr\include
    if "%LIBDIR%"=="" set LIBDIR=\usr\lib
    goto start_ok

:-
:-  MS Visual C/C++
:-
:start_msvc
    if not "%CCDIR%"=="" goto smsvc_1
    set CCDIR=\msdev
    if exist %CCDIR%\bin\cl.exe goto smsvc_2
    set CCDIR=\msvc
:smsvc_1
    if exist %CCDIR%\bin\cl.exe goto smsvc_2
    echo MS Visual C/C++ directory not found - aborting
    goto exit
:smsvc_2
    if "%INCDEF%"=="" set INCDEF=%CCDIR%\include /I\usr\include
    if "%LIBDIR%"=="" set LIBDIR=\usr\lib
    goto start_ok

:-
:-  lcc
:-
:start_lcc
    if not "%CCDIR%"=="" goto slcc_1
    set CCDIR=\lcc
:slcc_1
    if exist %CCDIR%\bin\lcc.exe goto slcc_2
    echo lcc directory not found or compiler not yet defined - aborting
    goto exit
:slcc_2
    if "%INCDEF%"=="" set INCDEF=%CCDIR%\include -I\usr\include
    if "%LIBDIR%"=="" set LIBDIR=\usr\lib
    goto start_ok

:-
:-  GNU C
:-
:start_gcc
    if not "%CCDIR%"=="" goto sgcc_1
    set CCDIR=\gcc
:sgcc_1
    if exist %CCDIR%\bin\gcc.exe goto sgcc_2
    echo gcc directory not found or compiler not yet defined - aborting
    goto exit
:sgcc_2
    if "%INCDEF%"=="" set INCDEF=%CCDIR%\include
    if "%LIBDIR%"=="" set LIBDIR=%CCDIR%\lib
    goto start_ok

:-
:-  WATCOM C32
:-
:start_wcc
    if not "%CCDIR%"=="" goto swcc_1
    set CCDIR=%WATCOM%
    if exist %CCDIR%\binnt\wcc386.exe goto swcc_2
    set CCDIR=\watcom
:swcc_1
    if exist %CCDIR%\binnt\wcc386.exe goto swcc_2
    echo Watcom C32 directory not found - aborting
    goto exit
:swcc_2
    if "%INCDEF%"=="" set INCDEF=%NT_INCLUDE%;%INCLUDE%
    if "%LIBDIR%"=="" set LIBDIR=%LIB%
    goto start_ok


:start_ok
    if "%1"==""  goto help
    if "%1"=="-l" goto clink
    if "%1"=="-L" goto link
    if "%1"=="-S" goto system
    if "%1"=="-r" goto replace

    set _COMP=1
    set _LINK=0
    goto process

:clink
    set _COMP=1
    set _LINK=1
    shift
    goto process

:link
    set _COMP=0
    set _LINK=1
    shift
    goto process

:system
    if %CCNAME%==msvc echo Compiling using MS Visual C/C++ in %CCDIR%
    if %CCNAME%==bc   echo Compiling using Borland C in %CCDIR%
    if %CCNAME%==bcc  echo Compiling using Borland C++ in %CCDIR
    if %CCNAME%==tc   echo Compiling using Turbo C in %CCDIR%
    if %CCNAME%==tcc  echo Compiling using Turbo C++ in %CCDIR%
    if %CCNAME%==lcc  echo Compiling using lcc in %CCDIR%
    if %CCNAME%==gcc  echo Compiling using GNU C in %CCDIR%
    if %CCNAME%==wcc  echo Compiling using Watcom C32 in %CCDIR%
    echo Linking with libraries in %LIBDIR%
    goto exit

:replace
    if .%2==. goto help
    if .%3==. goto help
    set _LIB=%2
    set _FILE=%3
    if %_LIB%==any for %%a in (*.lib) do set _LIB=%%a
    if %_LIB%==any      set _LIB=libany.lib
    if exist %_LIB%.lib set _LIB=%_LIB%.lib

:rep_loop
    if exist %_FILE%.obj set _FILE=%_FILE%.obj
    echo Replacing %_FILE% into %_LIB%
    if %CCNAME%==bc   goto rbor
    if %CCNAME%==bcc  goto rbor
    if %CCNAME%==tc   goto rbor
    if %CCNAME%==tcc  goto rbor
    if %CCNAME%==lcc  goto rlcc
    if %CCNAME%==gcc  goto rgcc
    if %CCNAME%==msvc goto rmsvc
    if %CCNAME%==wcc  goto rwcc
    goto exit

:rep_next
    shift
    if .%3==. goto rep_done
    set _FILE=%3
    goto rep_loop

:rep_done
    set _LIB=
    set _FILE=
    goto exit

:rmsvc
    if     exist %_LIB% %CCDIR%\bin\lib /nologo      %_LIB% %_FILE% >nul
    if not exist %_LIB% %CCDIR%\bin\lib /nologo /out:%_LIB% %_FILE% >nul
    goto rep_next

:rbor
    %CCDIR%\bin\tlib %_LIB%-+%_FILE% >nul
    goto rep_next

:rlcc
    %CCDIR%\bin\lcclib %_LIB% %_FILE% >nul
    goto rep_next

:rgcc
    %CCDIR%\bin\ar -r %_LIB% %_FILE%
    goto rep_next

:rwcc
    %CCDIR%\binnt\wlib -q %_LIB% -+%_FILE% >nul
    goto rep_next

:process
    if .%1==. goto help
    set _FILE=%1
    if exist %_FILE%.c set _FILE=%_FILE%.c
    if not exist %_FILE% goto notfound

    if %CCNAME%==bc   goto cborc
    if %CCNAME%==bcc  goto cborc
    if %CCNAME%==tc   goto cbortc
    if %CCNAME%==tcc  goto cbortc
    if %CCNAME%==lcc  goto clcc
    if %CCNAME%==gcc  goto cgcc
    if %CCNAME%==msvc goto cmsvc
    if %CCNAME%==wcc  goto cwcc
    goto exit

:notfound
    echo %_FILE% not found
    set _FILE=
    goto exit

:cmsvc
    if %_COMP%==0 goto msvc_link
    set CC=%CCDIR%\bin\cl
    echo Compiling %_FILE%...
    %CC% /nologo /c /Od /W3 /I%INCDEF% /D "DEBUG" /D "WIN32" %_FILE%>c.lst
    if not errorlevel 1 goto cmsvc_ok
    type c.lst
    goto error
:cmsvc_ok
    del c.lst
    set _FILE=

:msvc_link
    if %_LINK%==0 goto done
    if exist library.lst del library.lst
    for %%a in (*.lib)          do echo %%a >> library.lst
    for %%a in (%LIBDIR%\*.lib) do echo %%a >> library.lst
    echo %CCDIR%\lib\libc.lib               >> library.lst
    echo %CCDIR%\lib\libcp.lib              >> library.lst
    echo %CCDIR%\lib\kernel32.lib           >> library.lst
    echo %CCDIR%\lib\user32.lib             >> library.lst
    echo %CCDIR%\lib\gdi32.lib              >> library.lst
    echo %CCDIR%\lib\comdlg32.lib           >> library.lst
    echo %CCDIR%\lib\advapi32.lib           >> library.lst
    echo %CCDIR%\lib\netapi32.lib           >> library.lst
    echo %CCDIR%\lib\shell32.lib            >> library.lst
    echo %CCDIR%\lib\odbc32.lib             >> library.lst
    echo %CCDIR%\lib\user32.lib             >> library.lst
    echo %CCDIR%\lib\wsock32.lib            >> library.lst
    echo %CCDIR%\lib\winmm.lib              >> library.lst
    echo %CCDIR%\lib\oldnames.lib           >> library.lst
    echo %CCDIR%\lib\uuid.lib               >> library.lst
    echo %CCDIR%\lib\mpr.lib                >> library.lst

    echo Linking %1...
    %CCDIR%\bin\link /NOLOGO /STACK:8096 /SUBSYSTEM:CONSOLE %1 @library.lst
    if exist %1.map del %1.map
    if exist %1.exp del %1.exp
    del library.lst
    goto done

:cbortc
    if %_COMP%==0 goto bor_link
    echo Compiling %_FILE%...
    %CCDIR%\bin\tcc -c -f -O -Z -w -C -I%INCDEF% -ml -DDEBUG %_FILE%>c.lst
    if not errorlevel 1 goto cbortc_ok
    type c.lst
    goto error
:cbortc_ok
    del c.lst
    set _FILE=
    goto bor_link

:cborc
    if %_COMP%==0 goto bor_link
    echo Compiling %_FILE%...
    %CCDIR%\bin\bcc -c -f -O -Z -w -C -I%INCDEF% -ml -DDEBUG %_FILE%>c.lst
    if not errorlevel 1 goto cborc_ok
    type c.lst
    goto error
:cborc_ok
    del c.lst
    set _FILE=

:bor_link
    if %_LINK%==0 goto done
    if exist library.lst del library.lst
    for %%a in (*.lib)          do echo %%a+ >> library.lst
    for %%a in (%LIBDIR%\*.lib) do echo %%a+ >> library.lst
    echo %CCDIR%\lib\mathl.lib+              >> library.lst
    echo %CCDIR%\lib\emu.lib+                >> library.lst
    echo %CCDIR%\lib\cl.lib                  >> library.lst

    echo Linking %1...
    %CCDIR%\bin\tlink %CCDIR%\lib\c0l+%1,%1,,@library.lst;
    if errorlevel 1 goto lerror
    del library.lst
    goto done

:clcc
    if %_COMP%==0 goto lcc_link
    echo Compiling %_FILE%...
    set CCDEFINES=-DDEBUG -DWIN32 -D__LCC__ -D_MSC_VER
    %CCDIR%\bin\lcc -O -I%INCDEF% %CCDEFINES% %_FILE%
    if errorlevel 1 goto error
    set CCDEFINES=
    set _FILE=

:lcc_link
    if %_LINK%==0 goto done
    if exist library.lst del library.lst
    mkdir libtmp
    for %%a in (%LIBDIR%\*.lib) do copy %%a libtmp > nul
    for %%a in (*.lib)          do copy %%a libtmp > nul
    for %%a in (*.lib)          do echo %%a >> library.lst
    for %%a in (%LIBDIR%\*.lib) do echo %%a >> library.lst
    for %%a in (libtmp\*.lib)   do echo %%a >> library.lst
    echo %CCDIR%\lib\wsock32.lib            >> library.lst
    echo %CCDIR%\lib\winmm.lib              >> library.lst
    echo %CCDIR%\lib\mpr.lib                >> library.lst

    echo Linking %1...
    %CCDIR%\bin\lcclnk -s -subsystem console %1.obj @library.lst
    del library.lst
    del libtmp\*.lib
    rmdir libtmp
    goto done

:cgcc
    if %_COMP%==0 goto gcc_link
    echo Compiling %_FILE%...
    set CCDEFINES=-DDEBUG
    %CCDIR%\bin\gcc -c -O2 -I%INCDEF% %CCDEFINES% %_FILE%
    if errorlevel 1 goto error
    set CCDEFINES=
    set _FILE=
    if exist %1.map del %1.obj
    ren %1.o %1.obj

:gcc_link
    if %_LINK%==0 goto done
    if exist library.lst del library.lst
    mkdir libtmp
    for %%a in (%LIBDIR%\*.lib) do copy %%a libtmp > nul
    for %%a in (*.lib)          do copy %%a libtmp > nul
    for %%a in (*.lib)          do echo %%a >> library.lst
    for %%a in (%LIBDIR%\*.lib) do echo %%a >> library.lst
    for %%a in (libtmp\*.lib)   do echo %%a >> library.lst

    echo Linking %1...
    %CCDIR%\bin\gcc -o%1.exe %1.obj @library.lst
    del library.lst
    del libtmp\*.lib
    rmdir libtmp
    goto done

:cwcc
    if %_COMP%==0 goto wcc_link
    echo Compiling %_FILE%...
    set CCDEFINES=-DDEBUG
    %CCDIR%\binnt\wcc386 -wx -zq -i=%INCDEF% %CCDEFINES% %_FILE%
    if errorlevel 1 goto error
    set CCDEFINES=
    set _FILE=

:wcc_link
    if %_LINK%==0 goto done
    if exist library.lst del library.lst
    mkdir libtmp
    for %%a in (*.lib)          do echo %%a >> library.lst

    echo Linking %1...
    %CCDIR%\binnt\wlink option quiet debug all file %1.obj library { @library.lst }
    del library.lst
    rmdir libtmp
    goto done

:done
    shift
    if not .%1==. goto process
    set _COMP=
    set _LINK=
    goto exit

:error
    set _FILE=
    echo Compile errors in %1
    goto exit

:lerror
    echo Link errors in %1
    goto exit

:exit
