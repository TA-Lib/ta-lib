/*
 *  Compile script for iMatix sources, for OS/2
 *
 *  This script will compile, link, or archive, one or more files.
 *  The syntax is a subset of the syntax of the c bourne shell script.
 *
 *  Written:    1998/01/02  Ewen McNeill <ewen@imatix.com>
 *  Revised:    1999/02/03  Ewen McNeill <ewen@imatix.com>
 *              1999/11/14  Pieter Hintjens
 *
 *  Syntax:     c filename...     Compile ANSI C program(s)
 *              c -c filename...  Compile ANSI C program(s)
 *              c -l main         Compile and link main program(s)
 *              c -L main         Link main program(s), no compile
 *              c -C              Print out full call to compiler
 *              c -S              Report name of system (OS2)
 *              c -r lib file     Replace file.o into library
 *                -v              (First arg prefix to above): be verbose
 *                -q              (First arg prefix to above): be quiet
 *
 *  Copyright:  Copyright (c) 1998-99 iMatix Corporation
 *
 *  Bug reports & questions to <ewen@imatix.com>
 *
 *  The following environment variables control its operation:
 *
 *  UTYPE       Ignored; OS2 assumed
 *  CCNAME      Name of C compiler (default: gcc)
 *  CCOPTS      Options for C compiler (overrides CCPRODLEVEL, etc)
 *  CCDEFINES   Definitions to pass to compiler (default: -DDEBUG,
 *                   unless CCPRODLEVEL=production)
 *  CCPRODLEVEL One of: debug, standard (default), production
 *              Controls amount of debugging support built in;
 *              debug:       -DEBUG -g
 *              standard:    -DEBUG -O2
 *              production:  -s -O2 -Zomf -Zcrtdll
 *  CCLIBNAME   Library name to archive into (if library name is "any";
 *              by default archives into first library found in this 
 *              case)
 *  CCLIBS      Libraries to link against, which linking.  Default is
 *              all libraries in the current directory.
 *  RANLIB      Ignored ("ar rs" always used)
 *  LINKPATH    Ignored (gcc is flexible)
 *
 *  The program will try to read the settings of these variables out of
 *  1. %ETC%\c.conf; \emx\etc\c.conf; \local\emx\etc\c.conf
 *  2. %HOME%\.c.conf
 *  3. c.conf, .c.conf
 *
 *  but variables set in the environment will override these values.
 *  The files on the same line are tried in order, and the files
 *  on different lines are read in the order 1, 2, 3, and thus the
 *  more specific files (eg, per directory) may override other more
 *  general settings.
 *  ---------------------------------------------------------------
 */
'@echo off'
setlocal

/* ------------------------------------------------------------------------ */
/* Pull in RexxUtil library, so that we can can use RxSysFileTree           */
IF RxFuncQuery('SysLoadFuncs') THEN
DO
  IF RxFuncAdd('SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs') THEN
  DO
    SAY "Cannot load RexxUtil (required for RxSysFileTree)"
    EXIT 1
  END
  CALL SysLoadFuncs
END

/* Configuration options */

/* Try to read configuration from possible files
 * The following files are tried in order:
 * %ETC%\c.conf
 * \emx\etc\c.conf
 * \local\emx\etc\c.conf
 * %HOME%\.c.conf
 * c.conf
 * .c.conf
 *
 * Possibly at some later stage we could scan the datapath for likely items.
 */

/* System configuration */
havesysconfig = 0
ETC = value('ETC', , 'OS2ENVIRONMENT')
IF ETC \= '' THEN
DO
   call loadconfig ETC'\c.conf'
   if RC == 0 then
      havesysconfig = 1
END

IF havesysconfig \= 1 THEN
DO
   call loadconfig '\emx\etc\c.conf'
   IF RC \= 0 THEN
      call loadconfig '\local\emx\etc\c.conf'
END

/* User configuration */
HOME = value('HOME', , 'OS2ENVIRONMENT')
IF HOME \= '' THEN  
   call loadconfig HOME'\.c.conf'

/* Directory configuration */
call loadconfig 'c.conf'
IF RC \= 0 THEN
   call loadconfig '.c.conf'

/*----------------------------------------------------------------------*/
/* Now apply environment settings, and defaults, over the top           */
/* Compiler defaults to gcc */

IF value('CCNAME', , 'OS2ENVIRONMENT') \= '' THEN
   CCNAME = value('CCNAME', , 'OS2ENVIRONMENT')
ELSE
   IF symbol('CCNAME') \= 'VAR' THEN
      CCNAME = 'gcc'

/* Production level defaults to standard */
IF value('CCPRODLEVEL', , 'OS2ENVIRONMENT') \= '' THEN
   CCPRODLEVEL = value('CCPRODLEVEL', , 'OS2ENVIRONMENT')
ELSE
   IF symbol('CCPRODLEVEL') \= 'VAR' THEN
      CCPRODLEVEL = 'standard'

/* Compiler definitions defaults to -DDEBUG unless production compile */
IF value('CCDEFINES', , 'OS2ENVIRONMENT') \= '' THEN
   CCDEFINES    = value('CCDEFINES', , 'OS2ENVIRONMENT')
IF symbol('CCDEFINES') \= 'VAR' THEN
   IF CCPRODLEVEL = 'production' THEN
      CCDEFINES = ''
   ELSE
      CCDEFINES = '-DDEBUG'

IF value('CCLIBS', , 'OS2ENVIRONMENT') \= '' THEN
   CCLIBS = value('CCLIBS', , 'OS2ENVIRONMENT')

/* Standard flags; use -Zexe so that we can use unix style makefiles.        */
/* sysv-signals are used because iMatix code historically depends on it      */
/* And other useful defaults.                                                */
CCFLAGS   = '-Wall -pedantic -Zsysv-signals -Zexe'
LIBRARIES   = '-llibsfl -lsocket'         /* Default libraries required      */
ARCHIVE     = 'ar -rs'
EOBJ        = '.o'
ELIB        = '.a'

/* Now set up the CCOPTS based on production level, if it is not already set */
IF value('CCOPTS', , 'OS2ENVIRONMENT') \= '' THEN
   CCOPTS = value('CCOPTS', , 'OS2ENVIRONMENT')

IF symbol('CCOPTS') \= 'VAR' THEN
DO
   IF CCPRODLEVEL = 'debug' THEN
   DO
      CCOPTS   = CCDEFINES' 'CCFLAGS' -g'    /* Debug: -g */
   END 
   ELSE IF CCPRODLEVEL = 'production' THEN
   DO                                        /* Production: -Zomf -Zcrtdll */
      CCOPTS   = CCDEFINES' 'CCFLAGS' -O2 -s -Zomf -Zcrtdll' 
      ARCHIVE  = 'emxomfar rs'
      EOBJ     = '.obj'
      ELIB     = '.lib'
   END 
   ELSE 
   DO
      CCOPTS   = CCDEFINES' 'CCFLAGS' -O2'   /* Standard: -O2 */
   END
END

/* Upper case version of obj extension and lib extension */
EOBJU       = TRANSLATE(EOBJ)
ELIBU       = TRANSLATE(ELIB)
OBJLEN      = LENGTH(EOBJ)
LIBLEN      = LENGTH(ELIB)

/***
SAY "Compile with: "CCNAME
SAY "CCOPTS:       "CCOPTS
SAY "CCPRODLEVEL:  "CCPRODLEVEL
SAY "LIB = "ELIB"; OBJ = "EOBJ
EXIT
***/

PARSE ARG commandline
IF commandline == "" THEN
DO
  SAY "Syntax:     c filename...     Compile ANSI C program(s)"
  SAY "            c -c filename...  Compile ANSI C program(s)"
  SAY "            c -l main         Compile and link main program(s)"
  SAY "            c -L main         Link main program(s), no compile"
  SAY "            c -S              Print out operating system (OS2)"
  SAY "            c -C              Print out full call to compiler"
  SAY "            c -r lib file     Replace object file into library"
  SAY "              -v              (First arg prefix to above): be verbose"
  SAY "              -q              (First arg prefix to above): be quiet"
  endlocal
  exit 1
END

/* Default is to compile, but not link -- no longer add to library */
first   = 1
compile = 1
link    = 0
addtolib= 0
verbose = 1

DO WHILE commandline \= ""
  PARSE VAR commandline thisarg commandline        /* Get next argument */

  IF (first = 1) & (compare(thisarg, "-v") = 0) THEN 
  DO
    verbose = 1
    PARSE VAR commandline thisarg commandline      /* Get next argument */
  END
  ELSE IF (first = 1) & (compare(thisarg, "-q") = 0) THEN 
  DO
    verbose = 0
    PARSE VAR commandline thisarg commandline      /* Get next argument */
  END

  IF (first = 1) & (compare(thisarg, "-c") = 0) THEN
  DO
    compile = 1
    link    = 0
    addtolib= 0
    PARSE VAR commandline thisarg commandline      /* Get next argument */
  END
  ELSE IF (first = 1) & (compare(thisarg, "-l") = 0) THEN
  DO
    compile = 1
    link    = 1
    addtolib= 0
    PARSE VAR commandline thisarg commandline      /* Get next argument */
  END
  ELSE IF (first = 1) & (compare(thisarg, "-L") = 0) THEN
  DO
    compile = 0
    link    = 1
    addtolib= 0
    PARSE VAR commandline thisarg commandline      /* Get next argument */
  END
  ELSE IF (first = 1) & (compare(thisarg, "-r") = 0) THEN
  DO
    compile = 0
    link    = 0
    addtolib= 1
    PARSE VAR commandline library thisarg commandline  /* Get next arg */
    IF library = 'any' THEN
       library = FindLocalLib()
  END
  ELSE IF (first = 1) & (compare(thisarg, "-C") = 0) THEN
  DO
    /* Report on the command line invocation, and exit        */
    /* We add the -D__EMX__ bit for the benefit of makedepend */
    /* And we pick up the C_INCLUDE_PATH item if defined and  */
    /* stick that on the end.                                 */
    includepath = value('C_INCLUDE_PATH',,'OS2ENVIRONMENT')
    if (includepath \= '') then includepath = "-I"includepath

    SAY CCNAME "-c" CCOPTS "-D__EMX__" includepath
    exit 0
  END
  ELSE IF (first = 1) & (compare(thisarg, "-S") = 0) THEN
  DO
    /* Report operating system -- always OS2 */
    SAY "OS2"
    exit 0
  END
  ELSE
  DO
    /* Default is to compile only -- no link, no add to library */
    compile = 1
    link    = 0
    addtolib= 0
  END

  first = 0

  IF thisarg \= "" THEN
  DO
    IF compile = 1 THEN 
    DO
      IF addtolib = 0 THEN CALL CompileToObj thisarg
                      ELSE CALL CompileToLib thisarg library
    END
    ELSE
    DO
      IF addtolib = 1 THEN CALL ArchiveObj   thisarg  library
    END

    IF link       = 1 THEN CALL LinkProgram  thisarg
  END
END

endlocal
exit 0
/* End script */

/* Utility subroutines -------------------------------------------------- */

/* CompileToObj <source>
 *
 * Compile C source file into object file.  If this fails, abort the script.
 * CCNAME and CCOPTS are REXX variables defined at the top of the script.
 */

CompileToObj:

/* Check to make sure the symbols we require are defined */
/* NOTE: commas at end of line to continue onto next line */
IF (symbol('CCNAME') \= 'VAR') | (symbol('CCOPTS') \= 'VAR') THEN
DO
   SAY "One of the required symbols is not defined."
   SAY "CCNAME is a " symbol('CCNAME')
   SAY "CCOPTS is a " symbol('CCOPTS')
   EXIT 99
END

/* Figure out the filename for the source file */
IF (right(ARG(1), 2) = '.c') | (right(ARG(1), 2 ) = '.C') THEN
DO
   sourcefile   = ARG(1)
   basefilename = left(ARG(1), (length(ARG(1)) - 2))
END
ELSE
DO
   sourcefile   = ARG(1)'.c'
   basefilename = ARG(1)
END

/* Join the strings together */
objectfile   = basefilename''EOBJ

/* List file name */
listfile     = basefilename'.lst'

IF verbose = 1 THEN SAY "Compiling "sourcefile"..."

CCNAME CCOPTS '-c' sourcefile '2>'listfile
IF (RC > 0) THEN 
DO
  'type 'listfile
  exit 2
END
ELSE
DO
  'type 'listfile
  'del 'listfile' >nul'
END

return

/* ArchiveObj <object> <library> <deleteflag>
 *
 * Archive the object specified into the library.
 * ARCHIVE is a REXX variable defined at the top of the script.
 */

ArchiveObj:

/* Check to make sure the symbols we require are defined  */
/* NOTE: commas at end of line to continue onto next line */
IF (symbol('ARCHIVE') \= 'VAR') THEN
DO
  SAY "One of the required symbols is not defined."
  SAY "ARCHIVE is a " symbol('ARCHIVE')
  EXIT 99
END

PARSE ARG object library deleteflag

/* Figure out the filename for the source file */
IF (right(object, 2) = '.c') | (right(object, 2) = '.C') |,
   (right(object, OBJLEN) = EOBJ) | (right(object, OBJLEN) = EOBJU) THEN
  basefilename = left(object, (length(object) - OBJLEN))
ELSE
  basefilename = object

/* Join the strings together to get object filename */
objectfile = basefilename''EOBJ

IF (right(library, LIBLEN) = ELIB) | (right(library, LIBLEN) = ELIBU) THEN
    libraryfile = library
ELSE
    libraryfile = library''ELIB

IF verbose = 1
  THEN SAY "Replacing object "objectfile" in library "libraryfile"..."

ARCHIVE libraryfile objectfile
IF (RC > 0) then exit 3
IF deleteflag = 'delete' THEN
   'del 'objectfile' >nul'

return

/* CompileToLib <source> <library>
 *
 * Compile C source file into object file, then put that object file into
 * the library.  If either of these steps fail, then abort the script.
 *
 * CC, CCOPTS, and ARCHIVE are REXX variables defined at the top
 * of the script.
 */

CompileToLib:

PARSE ARG SOURCE LIBRARY

CALL CompileToObj SOURCE
IF RC > 0
   exit 2
CALL ArchiveObj   SOURCE LIBRARY  'delete'
IF RC > 0
   exit 3
return

/* LinkProgram <name>
 *
 * Link the object file name.OBJ to get name.EXE.  If the link fails, exit.
 *
 * CC, CCOPTS and LIBRARIES are REXX variables defined at the top
 * of the script.
 */

LinkProgram:

/* Check to make sure the symbols we require are defined */
/* NOTE: commas at end of line to continue onto next one */
IF (symbol('CCNAME') \= 'VAR') | (symbol('CCOPTS') \= 'VAR') |,
   (symbol('LIBRARIES') \= 'VAR') THEN
DO
  SAY "One of the required symbols is not defined."
  EXIT 99
END

basefilename = ARG(1)
IF (right(basefilename, 4) = '.exe') | (right(object, 4) = '.EXE') THEN
  basefilename = left(basefilename, (length(basefilename) - 4))

IF (right(basefilename, 2) = '.c')   | (right(basefilename, 2) = '.C') THEN
  basefilename = left(basefilename, (length(basefilename) - 2))

IF (right(basefilename, OBJLEN) = EOBJ) |,
   (right(basefilename, OBJLEN) = EOBJU) THEN
  basefilename = left(basefilename, (length(basefilename) - OBJLEN))

/* Join the strings together */
/* If we are linking with -Zexe, then the output filename shouldn't have     */
/* ".exe" extension, because this will be added automatically.               */
objectfile   = basefilename''EOBJ
IF (POS("-Zexe", CCOPTS) > 0) THEN
  executable = basefilename
ELSE
  executable = basefilename'.exe'

/* Figure out the libraries to link against */
locallibraries = '-L.'

IF symbol('CCLIBS') \= 'VAR' THEN
DO
   CALL SysFileTree '*'ELIB, 'files.', 'FO'
   DO i = 1 TO files.0
     libfilename = FILESPEC('name', files.i)
     IF libfilename \= '' THEN
     DO
       libfilename = STRIP(libfilename)    /* CMD.EXE leaves trailing spaces*/
       IF (right(libfilename, LIBLEN) = ELIB) |,
          (right(libfilename, LIBLEN) = ELIBU) THEN
           libraryfile = left(libfilename, (length(libfilename) - LIBLEN))
       ELSE
           libraryfile = libfilename

       locallibraries = locallibraries '-l'libraryfile
     END
   END

   alllibraries = locallibraries' 'LIBRARIES
END
ELSE
DO
   alllibraries = CCLIBS
END

IF verbose = 1 THEN SAY "Linking "executable"..."

CCNAME CCOPTS '-o' executable objectfile alllibraries alllibraries
IF (RC > 0) THEN exit 2
return

/* Find a local library to add ourselves into
 *
 * Strategy is: CCLIBNAME value if set, otherwise look for lib*.a
 */

FindLocalLib:
  locallib = value('CCLIBNAME',,'OS2ENVIRONMENT')
  IF (locallib = '') THEN
  DO
    /* Search for a local library to use */
    CALL SysFileTree '*'ELIB, 'files.', 'FO'
    IF files.0 > 0 THEN
      locallib = FILESPEC('name', files.1)
  END

  IF (locallib = '') THEN locallib = 'libany'ELIB

RETURN locallib

/* Load configuration from file
 * Reads in a set of name=value pairs, and skip comments beginning with #
 * from the specified file, if the file can be opened.  Returns 0 if 
 * successful, and 1 otherwise.
 */

loadconfig:
  configfilename = ARG(1)
  if stream(configfilename, 'C', 'QUERY EXISTS') = '' THEN
     return 1

  DO WHILE LINES(configfilename)
     line = LINEIN(configfilename)
     IF POS('#', line) == 0             /* Not a comment character */
     THEN
     DO
       PARSE VAR line name '=' setting
       IF name \= '' & setting \= '' THEN
       DO
/*       SAY "Setting: "name"="setting  */
         dummy = value(name, setting)           /* Store name = setting */
       END
     END
  END

RETURN 0

/* End of File */


