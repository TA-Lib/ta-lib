/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflproc.h
    Title:      Process control functions
    Package:    Standard Function Library (SFL)

    Written:    1996/09/09  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/10/17

    Synopsis:   Provides functions to create and manage processes.  The main
                set of functions lets you create, monitor, and end processes.
                A secondary function lets you run the current process as a
                background process.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLPROC_INCLUDED                /*  Allow multiple inclusions        */
#define SFLPROC_INCLUDED

/*  Type definitions                                                         */

#if (defined (WIN32))
typedef struct {
    HANDLE process;
    DESCR *envd;                        /*  Environment data                 */
} PROC_HANDLE;
typedef PROC_HANDLE *PROCESS;           /*  Process ID type                  */
#define NULL_PROCESS NULL               /*    and null process               */

#elif (defined (__VMS__))
typedef struct {
    long id;
    long status;
} PROC_HANDLE;
typedef PROC_HANDLE *PROCESS;           /*  Process ID type                  */
#define NULL_PROCESS NULL               /*    and null process               */

#else
typedef pid_t        PROCESS;           /*  Process ID type                  */
#   define NULL_PROCESS 0               /*    and null process               */
#endif

#define NULL_HANDLE  -2                 /*  File handle set to nothing       */

/*  Process creation data structure -- contains all the information needed   */
/*  to create a new process, including I/O redirection, changing directories */
/*  changing UID/GID.  The uid/gid values are applicable only to unix.       */
/*  The rootdir option does a chroot() under unix, and a _chdir() under DOS, */
/*  Windows, and OS/2 (which will also change drive).                        */
/*                                                                           */
/*  The macro PROCESS_DATA_INIT can be used to initialised this struct       */
/*  at variable declaration time.                                            */

typedef struct {
    /*  Program name and arguments ----------------------------------------- */
    /*  The filename string can contain arguments (which will be parsed)     */
    /*  if argv == NULL.  If argv != NULL filename will not be parsed for    */
    /*  arguments.                                                           */
    /*  The searchext field is relevant only to DOS-like systems, and lists  */
    /*  extensions to try adding to the file to find it.  The default is     */
    /*  operating system specific, covering standard executable extensions.  */
    const char *filename;
          char **argv;
    const char *path;                   /*  Override PATH, if not null       */
    const char *shell;                  /*  Override default shell, if ! null*/
    const char **searchext;             /*  Extensions to try, to find file  */
    Bool       searchpath;              /*  Look in path for filename        */
    Bool       useshell;                /*  Invoked via shell                */
    Bool       createdaemon;            /*  Create daemon process (detached) */
    Bool       wait;                    /*  Wait for process to finish       */
    int        delay;                   /*  ms to wait to see if exec worked */

    /*  Directories to change to ------------------------------------------- */
    const char *rootdir;                /*  Dir to chroot() to, if not NULL  */
    const char *workdir;                /*  Work dir; done after any chroot()*/

    /*  I/O Redirection ---------------------------------------------------- */
    /*  A handle of NULL_HANDLE means "no change".                           */
    /*  no_hanldes specifies the number of handles that should be inherited  */
    /*  by the child process, where this can be controlled (eg under unix,   */
    /*  where handles above that number will be marked to close on exec())   */
    int in;
    int out;
    int err;
    int no_handles;                     /*  File handles inherited; default 3*/


    /*  Environment -------------------------------------------------------- */
    /*  Default is to inherit the current environment, without change.  This */
    /*  can either be replaced entirely by supplying envv != NULL, or things */
    /*  can be added/removed from the current environment to get a new one.  */
    char   **envv;                      /*  Whole replacement environment    */
    SYMTAB *envadd;                     /*  Entries to add to environment    */
    SYMTAB *envrm;                      /*  Keys to remove from environment  */

    /*  Security ----------------------------------------------------------- */
    /*  If user name and group are not null, tries to run the process under  */
    /*  the specified user name.  Under Windows, the group name is the name  */
    /*  of the domain.  The password field is needed if the calling process  */
    /*  does not run as a priviliged process.                                */
    char  *username;
    char  *groupname;
    char  *password;
    Bool   preserveroot;                /*  Retain root privileges?          */

    /*  Output ------------------------------------------------------------- */
    /*  This section contains elements set by process_create_full().         */
    PROCESS  pid;
    int   returncode;                   /*  Return code from process         */
    int   error;                        /*  Error code from functionc call   */
} PROCESS_DATA;

/*  Macros:
 *  FILEHANDLE_MAX      Maximum possible number of open files
 *  PROCESS_DATA_INIT   Process data structure (empty) initialisation
 */

/*  getdtablesize () is not available on all systems                         */
#if (defined (__UNIX__))
#   if (defined (__UTYPE_UNIXWARE))
#       define FILEHANDLE_MAX   sysconf (_SC_OPEN_MAX)
#   elif (defined (__UTYPE_HPUX))
#       define FILEHANDLE_MAX   FD_SETSIZE
#   elif (defined (__UTYPE_SINIX))
#       define FILEHANDLE_MAX   FD_SETSIZE
#   else
#       define FILEHANDLE_MAX   getdtablesize ()
#   endif
#elif (defined (FD_SETSIZE))
#   define FILEHANDLE_MAX       FD_SETSIZE
#else
#   define FILEHANDLE_MAX       32      /*  Arbitrary                        */
#endif

/*  Usage: PROCESS_DATA myproc = PROCESS_DATA_INIT;                          */
#define PROCESS_DATA_INIT  {\
      /*  Filename, args,   */  NULL, NULL, NULL, NULL, NULL,            \
      /*    flags           */  FALSE, FALSE, FALSE, FALSE, 1000,        \
      /*  Directories       */  NULL, NULL,                              \
      /*  I/O redirection   */  NULL_HANDLE, NULL_HANDLE, NULL_HANDLE, 3,\
      /*  Environment       */  NULL, NULL, NULL,                        \
      /*  Security          */  NULL, NULL, NULL, FALSE,                 \
      /*  Output            */  NULL_PROCESS, 0, 0                       \
    }


/*  Global variables                                                         */

extern int  process_errno;              /*  Last process exit code           */
extern Bool process_compatible;         /*  Try to be compatible             */
extern const char **sfl_default_ext;    /*  Default extensions               */

#ifdef __cplusplus
extern "C" {
#endif

PROCESS process_create_full (PROCESS_DATA *procinfo);
PROCESS process_create      (const char *file, char *argv [], const char *dir,
                             const char *in, const char *out, const char *err,
                             char *envv [], Bool wait);
int     process_setinfo     (PROCESS_DATA *procinfo, const char *in,
                             const char *out, Bool scratch_out,
                             const char *err, Bool scratch_err);
int     process_open_io     (const char *filename, char access_type);
void    process_close_io    (PROCESS_DATA *procinfo);
int     process_status      (PROCESS process_id);
int     process_kill        (PROCESS process_id);
void    process_close       (PROCESS process_id);
int     process_server      (const char *workdir, const char *lockfile,
                             int argc, char *argv [], const char *sswitch []);
Bool    process_alarm       (long delay);
char   *process_esc         (char *dest, const char *src);
char   *process_unesc       (char *dest, const char *src);
int     process_priority    (int priority);

#ifdef __cplusplus
}
#endif

/*  Return values from process_status()                                      */

#define PROCESS_RUNNING         0
#define PROCESS_ENDED_OK        1
#define PROCESS_ENDED_ERROR     2
#define PROCESS_INTERRUPTED     3

/*  Values for process_priority()                                            */

#define PRIORITY_LOW            0
#define PRIORITY_NORMAL         1
#define PRIORITY_HIGH           2

/*  Is process_create_full() implemented?                                    */
#if (defined (__UNIX__) || defined (__OS2__) || defined (WIN32))
#   define DOES_FULL_PROCESS
#else
#   undef  DOES_FULL_PROCESS
#endif


#endif


