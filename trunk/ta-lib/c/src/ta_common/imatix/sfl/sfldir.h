/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfldir.h
    Title:      Directory access functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/02  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/18

    Synopsis:   The directory access functions provide a portable interface
                to the system's file directory structure.   In general these
                functions are modelled around the UNIX opendir and readdir
                functions, but they are also similar to the DOS interface.
                These functions can fail on SVr4 if the <dirent.h> file
                does not match the C library.  Recompile with the switch
                -D _USE_BSD_DIRENT and they should work a bit better.
                Tested on: MS-DOS (Turbo-C), Windows (MSVC 4.0), UNIX
                (Linux, IBM AIX, SunOS).  OS/2 port was done by Ewen McNeill
                <ewen@naos.co.nz>.  DJGPP and DRDOS LFN by Rob Judd
                <judd@alphalink.com.au>.  Changes for Win32 by Will Menninger
                <willus@netcom.com>.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLDIR_INCLUDED                /*  Allow multiple inclusions        */
#define SFLDIR_INCLUDED

#if !(defined (NAME_MAX))               /*  Posix uses NAME_MAX              */
#   if !(defined (MAXNAMLEN))           /*  BSD uses MAXNAMLEN               */
#       if (defined (__WINDOWS__))
#           define MAXNAMLEN 259        /*  MSVC uses 259                    */
#       elif (defined (__MS_DOS__) && !defined LFN) /* Support for DRDOS LFN */
#           define MAXNAMLEN 12         /*  DOS uses 8.3                     */
#       else
#           define MAXNAMLEN 255        /*  And for everyone else, 255       */
#       endif
#   endif
#   define NAME_MAX         MAXNAMLEN
#endif

#define UID_CACHE_MAX       10          /*  Max. different uid's we cache    */
#define GID_CACHE_MAX       10          /*  Max. different gid's we cache    */

/*  DOS-ish file attributes, provided as an alternative to the UNIX-ish      */
/*  file mode bits.  Both fields are always filled-out as far as possible.   */
/*  These bits correspond to the normal DOS values.                          */

#define ATTR_RDONLY         0x01        /*  Read only file                   */
#define ATTR_HIDDEN         0x02        /*  Hidden file                      */
#define ATTR_SYSTEM         0x04        /*  System file                      */
#define ATTR_SUBDIR         0x10        /*  Subdirectory                     */
#define ATTR_MASK           0x17        /*  All bits together                */

/*  For portability we need to define types for the various fields that      */
/*  stat() returns.  If the compiler complains about these definitions,      */
/*  you need to add conditional definitions accordingly.  SMOP.              */
/*  For now, I assume that DOES_UID defines the stat types correctly.        */
/*  Note that prelude.h already defines uid_t and gid_t.                     */

#if (!defined (DOES_UID) && !defined (__DJGPP__) && !defined(__BORLANDC__))
typedef unsigned short      mode_t;
typedef unsigned short      nlink_t;
typedef long                off_t;
#endif

/*  Microsoft tends to use _stat instead of stat.                            */

#if (defined (_MSC_VER))
#   define stat _stat
#endif

/*  BeOS does not define S_IEXEC so we build this mask ourselves             */

#if (!defined (S_IEXEC))
#    define S_IEXEC  00100  /*  Owner may execute                            */
#endif

/*  We define DEFAULT_DIR as the default current directory, so that we       */
/*  can call open_dir() with a null or empty directory argument.  On most    */
/*  systems this is ".".                                                     */

#if (defined (__VMS__))
#   define DEFAULT_DIR      " "
#else
#   define DEFAULT_DIR      "."
#endif

/*  Under SVr4 it can happen that the <dirent.h> file does not match the     */
/*  C library.  Typically the library readdir() function returns the BSD     */
/*  structure while the <dirent.h> file defines dirent using the System V    */
/*  standards.  This is weird but apparently quite common.  Solution: at     */
/*  compile-time, force the switch -D _USE_BSD_DIRENT, and we define our     */
/*  own BSD-like structure.  Hey, I hate second-guessing the include files   */
/*  but if it's broke, you gotta fix it.  This problem appears at least on   */
/*  SunOS, to our knowledge.                                                 */

#if (defined (_USE_BSD_DIRENT))
struct Dirent
  {
    unsigned long   d_fileno;           /*  File number of entry             */
    unsigned short  d_reclen;           /*  Length of this record            */
    unsigned short  d_namlen;           /*  Length of string in d_name       */
    char            d_name [255 + 1];   /*  Maximum name length              */
  };
#else
#   define Dirent   dirent              /*  We'll always refer to Dirent     */
#endif

/*  Directory stream structure - this contains private fields starting       */
/*  with '_' and public fields.  If you use the private fields, be warned    */
/*  that these may change as we see fit.  If you add strings to this block,  */
/*  be sure to check fix_dir() and free_dir().                               */

typedef struct
{
    Bool    _fixed;                     /*  TRUE if processed by fix_dir()   */

#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    DIR    *_dir_handle;                /*    a directory handle             */
    struct Dirent                       /*    and a file desc. structure,    */
           *_dir_entry;                 /*    both transient blocks          */

#elif (defined (WIN32))                 /*  Win32:                           */
    HANDLE  _dir_handle;                /*    a directory handle             */
    WIN32_FIND_DATA _dir_entry;         /*    and a file descriptor          */

#elif (defined (_MSC_VER))              /*  MSC Win16                        */
    long    _dir_handle;                /*    a directory handle             */
    struct  _find_t _dir_entry;         /*    and a file desc. structure     */

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    struct ffblk                        /*    a file desc. structure         */
            _dir_entry;

#elif (defined (__VMS__))               /*  OpenVMS V5.x and lower           */
    long   _dir_handle;                 /*    lib$find_file context          */
#endif

    /*  Public fields                                                        */
    char   *dir_name;                   /*  Directory name + sep             */
    char   *owner;                      /*  File owner name                  */
    char   *group;                      /*  File owner group name            */
    char   *file_name;                  /*  Name of the file                 */
    time_t  file_time;                  /*  Time of modification for file    */
    off_t   file_size;                  /*  Size of the file                 */
    mode_t  file_mode;                  /*  UNIX-ish permission bits         */
    byte    file_attrs;                 /*  MS-DOS-ish permission bits       */
    nlink_t file_nlink;                 /*  Number of links to file          */
} DIRST;


typedef struct _FILEINFO
{
    struct _FILEINFO                    /*  Pointer for the linked list      */
        *next,
        *prev;
    DIRST
        dir;                            /*  File information                 */
    Bool
        directory;                      /*  TRUE if file is directory        */
} FILEINFO;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

Bool      open_dir      (DIRST *dir, const char *dir_name);
Bool      read_dir      (DIRST *dir);
Bool      close_dir     (DIRST *dir);

#if 0
!!! Not needed within TA-Lib
char     *format_dir    (DIRST *dir, Bool full);
int       fix_dir       (DIRST *dir);
#endif

int       free_dir      (DIRST *dir);

char     *resolve_path  (const char *path);
char     *locate_path   (const char *root, const char *path);
char     *clean_path    (const char *path);

#if 0
!!! Not needed within TA-Lib
NODE     *load_dir_list (const char *dir_name, const char *sort);
void      sort_dir_list (NODE *filelist, const char *sort);
FILEINFO *add_dir_list  (NODE *filelist, const DIRST *dir);
void      free_dir_list (NODE *filelist);
#endif

char     *get_curdir    (void);
int       set_curdir    (const char *path);
Bool      file_matches  (const char *filename, const char *pattern);
int       make_dir      (const char *path);
int       remove_dir    (const char *path);

#if 0
!!! Not needed within TA-Lib
qbyte     dir_usage     (const char *path, Bool recurse);
qbyte     dir_files     (const char *path, Bool recurse);
#endif

#ifdef __cplusplus
}
#endif

#endif
