/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflproc.c
    Title:      Process control functions
    Package:    Standard Function Library (SFL)

    Written:    1996/09/09  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/04/01

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory-handling functions        */
#include "sflstr.h"                     /*  String-handling functions        */
#include "sflfile.h"                    /*  File access functions            */
#include "sflnode.h"                    /*  Memory node functions            */
#include "sfldir.h"                     /*  Directory access functions       */
#include "sflcons.h"                    /*  Prototypes for functions         */
#include "sflsymb.h"                    /*  Symbol table handling            */
#include "sfltok.h"                     /*  Token-handling functions         */
#include "sflenv.h"                     /*  Environment handling functions   */
#include "sflprint.h"                   /*  snprintf functions               */

#include "sflproc.h"                    /*  Prototypes for functions         */
#include "sflprocx.h"                   /*  Extra sflproc functions          */

/*  Global variables                                                         */

int  process_errno = 0;                 /*  Last process exit code           */
Bool process_compatible = TRUE;         /*  Try to be compatible             */


/*  ---------------------------------------------------------------------[<]-
    Function: process_create_full

    Synopsis: Creates a subprocess and returns a PROCESS identifying the new
    process.  Optionally redirects standard input, output, error file handles
    to supplied file handles, changes working directory, and environment.
    In some operating systems can also optinally change the root directory
    (chroot()), and the uid/gid with which the new process runs.
    All information required to start the new process is specified in a
    PROCESS_DATA structure.  Where elements are not specified, they remain the
    same as the current process.  The following elements can be specified
    in the PROCESS_DATA structure:
    <Table>
    filename      File to execute, can include arguments if argv is NULL.
    argv_[]       List of arguments; argv [0] is filename; ends in a NULL.
    path          Search path (environments PATH is used if NULL).
    shell         Shell to use if useshell is TRUE (default is OS specific)
    searchext     Array of extensions to search when looking for filename
    searchpath    Flag: TRUE indicates path should be searched for filename
    useshell      Flag: TRUE indicates program should be started via a shell
    createdaemon  Flag: TRUE indicates a (separate) daemon should be started
    wait          Flag: TRUE indicates wait for process to finish
    delay         Amount of time to wait around for errors to happen (unix)
    rootdir       Root directory for new process (chroot()) if not NULL
    workdir       Working directory; if NULL, remains in current directory.
    in            File handle to use for standard input; -2 = no redirection.
    out           File handle to use for standard output; -2 = no redirection.
    err           File handle to use for standard error; -2 = no redirection.
    no_handles    Number of file handles to pass to child process (default: 3)
    envv_[]       Whole environment for new process; if NULL current env used
    envadd        Strings to add into current environment (if envv NULL).
    envrm         Keys to remove from current environment (if envv NULL).
    username      user name under which to run process
    groupname     groupname associated with user name
    password      required if calling process is not privileged
    </Table>
    If argv is NULL, parses the filename argument into words delimited by
    whitespace and builds the necessary argv table automatically.  Use this
    feature to execute a command with arguments, specified as one string.
    To search for the program in the path set searchpath to TRUE, and
    optionally supply a path to search.  To run shell builtins set useshell
    to TRUE.
    The envv list consists of strings in the form "name=value", ending in a
    NULL pointer.  If the envv argument is null, the environment of the
    current process is passed, with additions from envadd (if not NULL),
    and the keys listed in envrm removed (if not NULL).  If envv is not null
    then the envv environment is used as is.
    The child process may optionally start in a new root directory and with
    a different user/group (if supported by the operating system).  If rootdir,
    workdir, user, and group are all non null, then they are processed in the
    order: rootdir, workdir, group, user, to ensure all changes take place.
    Note that in this instance workdir is relative to the new root directory.
    Under DOS, Windows, and OS/2, the rootdir may be used to specify a change
    to a new drive letter (processed by _chdir() or _chdir2()).
    If the child command detects an error
    at startup, it may exit with an error status.  The sleep allows this
    error to be collected by calling process_status() after this call.  
    Returns child process id, or 0 if there was an error.  The PROCESS_DATA
    structure contains the following fields used for output:
    <Table>
    pid           Process identifier (as returned by function)
    returncode    Return code from sub process (only set if wait is TRUE)
    error         Code indicating error that occured (like libc errno)
    </Table>
    Under VMS, the filename must have been defined as a command before the
    calling process was started; the path is disregarded.
    Under Windows and OS/2 processing of the #! line is emulated, and the
    interpreter mentioned in the #! line will be invoked on the script.
    Under OS/2 the filename can be the name of a CMD script, and this will
    be run with the interpreter specified in the first line (EXTPROC line,
    or "'/'*!" line; or failing that with the default command interpreter.
    ---------------------------------------------------------------------[>]-*/

PROCESS
process_create_full (PROCESS_DATA *procinfo)
{
    /*  Implementation note: due to the size of this function, and the       */
    /*  sizeable differences between operating systems supported, the        */
    /*  implementation of this function for each operating system is in      */
    /*  a different file, and the appropriate one is included here.          */

    /*  WARNING: do not put any code here, otherwise it will prevent the     */
    /*  implementations from declaring variables.                            */

#if   (defined (__UNIX__))
#   include "sflprocu.imp"              /*  Unix implementation              */
#elif (defined (__OS2__))
#   include "sflproco.imp"              /*  OS/2 implementation              */
#elif (defined (WIN32))
#   include "sflprocw.imp"              /*  Windows (32-bit) implementation  */
#elif (defined (__VMS__))
#   include "sflprocv.imp"              /*  VMS implementation               */
#else
    return ((PROCESS) 0);               /*  Not supported on this system     */
#endif
}

 /*  ---------------------------------------------------------------------[<]-
    Function: process_create

    Synopsis: Creates a subprocess and returns a PROCESS identifying the new
    process.  Optionally directs standard input, output, and error streams
    to specified devices.  The caller can also specify environment symbols
    that the subprocess can access.  Accepts these arguments:
    <Table>
    filename    File to execute; if bare filename, searches PATH.
    argv_[]     List of arguments; argv [0] is filename; ends in a NULL.
    workdir     Working directory; if NULL, remains in current directory.
    std_in      Device to use for standard input; NULL = no redirection.
    std_out     Device to use for standard output; NULL = no redirection.
    std_err     Device to use for standard error; NULL = no redirection.
    envs_[]     List of environment symbols to define, or NULL.
    </Table>
    If argv is NULL, parses the filename argument into words delimited by
    whitespace and builds the necessary argv table automatically.  Use this
    feature to execute a command with arguments, specified as one string.
    The envv list consists of strings in the form "name=value", ending in a
    NULL pointer.  If the envv argument is null, the environment of the
    current process is passed.  Otherwise the envv environment is used.
    If the child command detects an error
    at startup, it may exit with an error status.  The sleep allows this
    error to be collected by calling process_status() after this call.  
    Returns child process id, or 0 if there was an error.
    Under VMS, the filename must have been defined as a command before the
    calling process was started; the path is disregarded.
    Under OS/2 the filename can be the name of a CMD script, and this will
    be run with the interpreter specified in the first line (EXTPROC line,
    or "'/'*!" line; or failing that with the default command interpreter.
    Under Unix, Windows, and OS/2 this function is implemented using the
    process_create_full() function.

    Known bugs: when parsing filename argument into words, does not handle
    quotes in any special way; "this text" is 2 words, '"this' and 'text"'.
    You should have passed the filename through process_esc() before adding
    any optional arguments.
    ---------------------------------------------------------------------[>]-*/

PROCESS
process_create (
    const char *filename,               /*  Name of file to execute          */
    char *argv [],                      /*  Arguments for process, or NULL   */
    const char *workdir,                /*  Working directory, or NULL       */
    const char *std_in,                 /*  Stdin device, or NULL            */
    const char *std_out,                /*  Stdout device, or NULL           */
    const char *std_err,                /*  Stderr device, or NULL           */
    char *envv [],                      /*  Environment variables, or NULL   */
    Bool  wait                          /*  Wait for process to end          */
)
{
#if (defined (__UNIX__) || defined (__OS2__) || defined (WIN32))
    PROCESS_DATA
        procinfo = PROCESS_DATA_INIT;
    PROCESS
        process = NULL_PROCESS;

    ASSERT (filename);
    if (!filename)
        return (NULL_PROCESS);

    /*  Set up information to start the new process                          */
    procinfo.filename = filename;
    procinfo.argv     = argv;
    procinfo.workdir  = workdir;
    procinfo.envv     = envv;
    procinfo.wait     = wait;

    /*  process_setinfo handles:
     *  1.  Determining if the path can be searched
     *  2.  Determining if root privileges should be preserved (if applicable)
     *  3.  Redirecting IO streams (if required)
     */
    if (process_setinfo (&procinfo, std_in, std_out, TRUE, std_err, FALSE) == 0)
      {
        process = process_create_full (&procinfo);
        process_close_io (&procinfo);
        /*  Stuff value into errno, to emulate old behaviour                 */
        errno = procinfo.error;
      }
    return (process);

#elif (defined (__VMS__))
    PROCESS
        process;                        /*  Our created process handle       */
    char
        *curdir,                        /*  Current directory                */
        *clean_filename,                /*  Unescaped filename               */
        *full_filename = NULL,
        *full_std_in   = NULL,
        *full_std_out  = NULL;
    qbyte
        process_flags;                  /*  Process creation flags           */
    int
        argn,                           /*  Argument number                  */
        rc;                             /*  Return code from lib$spawn       */
    Bool
        rebuilt_argv = FALSE;           /*  Did we rebuild argv[]?           */

    VMS_STRING (command_dsc, "");       /*  Define string descriptors        */
    VMS_STRING (std_in_dsc,  "");
    VMS_STRING (std_out_dsc, "");

    /*  If argv[] array was not supplied, build it now from filename         */
    if (!argv)
      {
        argv = tok_split (filename);
        filename = argv [0];
        rebuilt_argv = TRUE;
      }
    /*  If filename contains a path or extension, disregard them             */
    clean_filename = strrchr (filename, '/');
    if (clean_filename)
        clean_filename++;
    else
        clean_filename = (char *) filename;
    if (strchr (clean_filename, '.'))
       *strchr (clean_filename, '.') = '\0';

    /*  Rebuild full command from filename and arguments                     */
    full_filename = mem_alloc (tok_text_size ((char **) argv)
                               + strlen (clean_filename) + 1);
    strcpy (full_filename, clean_filename);
    for (argn = 1; argv [argn]; argn++)
        xstrcat (full_filename, " ", argv [argn], NULL);

    /*  Free argument table if we allocated it dynamically here              */
    if (rebuilt_argv)
        tok_free (argv);

    command_dsc.value  = full_filename;
    command_dsc.length = strlen (full_filename);

    /*  Prepare full names for stdin and stdout                              */
    curdir = get_curdir ();
    if (std_in)
      {
        if (strchr (std_in, '/'))       /*  If already with path, use as is  */
            full_std_in = mem_strdup (std_in);
        else
          {
            xstrcpy_debug ();
            full_std_in = xstrcpy (NULL, curdir, "/", std_in, NULL);
          }
        translate_to_vms  (full_std_in);
        std_in_dsc.value = full_std_in;
      }
    if (std_out)
      {
        if (strchr (std_out, '/'))      /*  If already with path, use as is  */
            full_std_out = mem_strdup (std_out);
        else
          {
            xstrcpy_debug ();
            full_std_out = xstrcpy (NULL, curdir, "/", std_out, NULL);
          }
        translate_to_vms   (full_std_out);
        std_out_dsc.value = full_std_out;
      }
    std_in_dsc.length  = std_in?  strlen (std_in_dsc.value): 0;
    std_out_dsc.length = std_out? strlen (std_out_dsc.value): 0;

    /*  If requested, change to working directory                            */
    if (workdir)
        chdir (workdir);

    /*  Prepare process flags                                                */
    if (wait)
        process_flags = 0;
    else
        process_flags = 1;              /*  Bit 1 = don't wait for child     */

    process = mem_alloc (sizeof (PROC_HANDLE));
    process-> id     = 0;
    process-> status = 0;               /*  Completion status                */

/*  char *envv [],  */                  /*  Environment variables, or NULL   */

    rc = lib$spawn (
        &command_dsc,                   /*  Command to run                   */
        std_in?  &std_in_dsc: NULL,     /*  Stdin descriptor                 */
        std_out? &std_out_dsc: NULL,    /*  Stdout+stderr                    */
        &process_flags,                 /*  Options for new process          */
        &NULL,                          /*  Process name -- generated        */
        &process-> id,                  /*  Returned process ID              */
        &process-> status);

    if (workdir)                        /*  Switch back to original dir      */
        chdir (curdir);
    mem_free (curdir);

    mem_strfree (&full_filename);       /*  Deallocate various buffers,      */
    mem_strfree (&full_std_in);         /*    if they were used              */
    mem_strfree (&full_std_out);        /*                                   */

    /*  Return process ID.  If we waited for completion, the process id      */
    /*  is always NULL.                                                      */
    if (rc != 1)                        /*  Process failed with error        */
      {
        process_close (process);
        process = NULL;
      }
    else
    if (wait)                           /*  Finished with process            */
        process_close (process);

    return (process);

#else
    return ((PROCESS) 0);               /*  Not supported on this system     */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_setinfo

    Synopsis: Accepts a pointer to a PROC_INFO block, and resets the fields in
    this block to default values as specified below (other fields are not
    changed):
    <Table>
    searchpath    True unless filename already contains a slash
    in            Std_in filename, or NULL
    out           Std_out filename, or NULL
    err           Std_err filename, or NULL
    preserveroot  True if we're running as root now
    </Table>
    Returns 0 if the PROC_INFO block was correctly initialised; returns -1 if
    there was an error opening one of the i/o streams.  Before calling this
    function you should have declared the PROC_INFO block using the syntax:
    PROCESS_DATA myproc = PROCESS_DATA_INIT;
    ---------------------------------------------------------------------[>]-*/

int
process_setinfo (
    PROCESS_DATA *procinfo,
    const char *std_in,                 /*  Stdin device, or NULL            */
    const char *std_out,                /*  Stdout device, or NULL           */
    Bool  scratch_out,                  /*  Scratch stdout file?             */
    const char *std_err,                /*  Stderr device, or NULL           */
    Bool  scratch_err                   /*  Scratch stderr file?             */
)                       
{
    const char
        *spacepos,
        *slashpos;
    char
        stdout_mode,
        stderr_mode;

    /*  Figure out if it is permissable to search the path                   */
    /*  It's permissable if the filename to be run doesn't contain a slash   */
    /*  of its own already.                                                  */
    procinfo-> searchpath = FALSE;
    if (procinfo-> filename)
      {
        spacepos = strchr (procinfo-> filename, ' ');
        if (spacepos == NULL)
            spacepos = procinfo-> filename + strlen (procinfo-> filename);
        slashpos = strchr (procinfo-> filename, '/');
#if (defined (MSDOS_FILESYSTEM))
        if (!slashpos)
            slashpos = strchr (procinfo-> filename, '\\');
#endif        
        if (slashpos == NULL || slashpos > spacepos)
            procinfo-> searchpath = TRUE;
      }
    /*  Figure out if root privilege should be retained.  If the real user   */
    /*  is root and effective user is root, then we retain root privilege    */
    /*  for the executed program.  Otherwise we want to discard it (default) */
    procinfo-> preserveroot = FALSE;
#if (defined (__UNIX__))
    if (getuid () == 0 && geteuid () == 0)
        procinfo-> preserveroot = TRUE;
#endif

    /*  Open the i/o streams if requested.  Note that process_set_io
     *  returns NULL_HANDLE if the supplied filename is null or empty.
     */
    stdout_mode = scratch_out? 'w': 'a';
    stderr_mode = scratch_err? 'w': 'a';
    procinfo-> in  = process_open_io (std_in,  'r');
    procinfo-> out = process_open_io (std_out, stdout_mode);
    procinfo-> err = process_open_io (std_err, stderr_mode);
    if (procinfo-> in  == -1
    ||  procinfo-> out == -1
    ||  procinfo-> err == -1)
      {
        process_close_io (procinfo);
        return (-1);
      }
    else
        return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_open_io

    Synopsis: Opens a file for i/o and returns a handle ready for passing to
    process_create_full().  The filename may be null or empty, in which case
    this function returns -2, which indicates a null file handle.  If there is
    an error, it returns -1.  Otherwise it returns a file handle >= 0.
    ---------------------------------------------------------------------[>]-*/

int
process_open_io (
    const char *filename,
    char access_type                    /*  r, w, a                          */
)
{
    int
        handle,
        mode = 0,
        permissions = 0;

    if (filename == NULL || *filename == '\0')
        return (-2);                    /*  Indicates a null handle          */
        
#if (defined (__UNIX__) || defined (__OS2__))
    mode = O_NOCTTY;
#endif
    if (access_type == 'r')
      {
        mode += O_RDONLY;
        permissions = 0;
      }
    else
    if (access_type == 'w')
      {
        mode += O_WRONLY | O_CREAT | O_TRUNC;
        permissions = S_IREAD | S_IWRITE;
      }
    else
    if (access_type == 'a')
      {
        mode += O_WRONLY | O_CREAT | O_APPEND;
        permissions = S_IREAD | S_IWRITE;
      }
    handle = open (filename, mode, permissions);
#if (defined (WIN32))
    /*  Under Windows, we need to move the file pointer to the end of the
     *  file ourselves, for append files, since for some reason this does
     *  not happen automatically for file handles passed to subprocesses.
     */
    if (access_type == 'a')
        lseek (handle, 0, SEEK_END);
#endif

#if (defined (DEBUG))
    if (handle < 0)
        perror ("Error opening redirection stream");
#endif
    return (handle);
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_close_io

    Synopsis: Closes any file handles opened for the process.  Use this
    function after you're finished with a process_create_full() call.
    ---------------------------------------------------------------------[>]-*/

void
process_close_io (PROCESS_DATA *procinfo)
{
    if (procinfo-> in >= 0)
      {
        close (procinfo-> in);
        procinfo-> in = NULL_HANDLE;
      }
    if (procinfo-> out >= 0)
      {
        close (procinfo-> out);
        procinfo-> out = NULL_HANDLE;
      }
    if (procinfo-> err >= 0)
      {
        close (procinfo-> err);
        procinfo-> err = NULL_HANDLE;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_status

    Synopsis: Returns status of process specified by process ID.  Returns
    one of these values, or -1 if there was an error:
    <Table>
    PROCESS_RUNNING       Process is still running.
    PROCESS_ENDED_OK      Process ended normally.
    PROCESS_ENDED_ERROR   Process ended with an error status.
    PROCESS_INTERRUPTED   Process was interrupted (killed).
    </Table>
    In the case of PROCESS_ENDED_ERROR, the global variable process_errno is
    set to the exit code returned by the process.
    ---------------------------------------------------------------------[>]-*/

int
process_status (
    PROCESS process)
{
#if (defined (__UNIX__) || defined (__OS2__))
    int
        status;
    pid_t
        return_pid;

    /*  waitpid() returns 0 if the child process is still running, or the    */
    /*  process id if it stopped.  It can also return -1 in case of error.   */
    /*  No other return value is possible.                                   */

    return_pid = waitpid (process, &status, WNOHANG | WUNTRACED);
    if (return_pid == 0)
        return (PROCESS_RUNNING);
    else
    if (return_pid == process)
      {
        if (WIFEXITED (status))        /*  Program called exit()             */
          {
            process_errno = WEXITSTATUS (status);
            if (process_errno)         /*  Treat exit (0) as normal end      */
                return (PROCESS_ENDED_ERROR);
            else
                return (PROCESS_ENDED_OK);
          }
        else
        if (WIFSIGNALED (status))       /*  Process was interrupted          */
            return (PROCESS_INTERRUPTED);
        else
            return (PROCESS_ENDED_OK);
      }
    else
        return (-1);

#elif (defined (WIN32))
    DWORD
         status;

    ASSERT (process);
    status = WaitForSingleObject (process-> process, 0);

    if (status == WAIT_TIMEOUT)
        return (PROCESS_RUNNING);
    else
    if (status == WAIT_OBJECT_0)
        return (PROCESS_ENDED_OK);
    else
    if (status == WAIT_ABANDONED)
        return (PROCESS_ENDED_ERROR);
    else
        return (-1);

#elif (defined (__VMS__))
    ASSERT (process);
    if (process-> status == 0)
        return (PROCESS_RUNNING);
    else
        return (PROCESS_ENDED_OK);

#else
    return (-1);                        /*  Not supported on this system     */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_kill

    Synopsis: Ends a process specified by a process id.  The current process
    must have the appropriate authority to stop the specified process.
    Returns zero if the process was killed, -1 if there was an error.
    ---------------------------------------------------------------------[>]-*/

int
process_kill (
    PROCESS process)
{
#if (defined (__UNIX__) || defined (__OS2__))
    int count = 5;

    ASSERT (process);

    /*  First give it a chance to gracefully exit...                         */
    kill (process, SIGTERM);
    while (process_status (process) == PROCESS_RUNNING && count--)
        sleep (1);

    /*  Then get brutal if neccessary.                                       */
    if (process_status (process) == PROCESS_RUNNING)
      {
        kill (process, SIGKILL);
        while (process_status (process) == PROCESS_RUNNING)
            sleep (1);
      }
    return (0);

#elif (defined (WIN32))
    ASSERT (process);

    TerminateProcess (process-> process, 1);
    while (process_status (process) == PROCESS_RUNNING)
        Sleep (100);

    process_close (process);
    return (0);

#elif (defined (__VMS__))
    ASSERT (process);

    sys$delprc (process-> id);
    process_close (process);
    return (0);

#else
    return (-1);                        /*  Not supported on this system     */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_close

    Synopsis: You should call this function when a process has ended
    normally, if you did not specify the wait option when calling the
    process_create() function.  On some systems, each created process
    uses some memory.  process_close() guarantees that this memory is
    correctly freed.  Does nothing if the process handle is NULL.
    ---------------------------------------------------------------------[>]-*/

void
process_close (
    PROCESS process)
{
#if (defined (WIN32))
    if (process)
      {
        mem_free (process-> envd);
        mem_free (process);
      }
#elif (defined (__VMS__))
    mem_free (process);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_server

    Synopsis: Converts the process from an interactive foreground process
    into a background process.  Depending on the system either the existing
    process becomes a background process, or a new process is started as
    a background process and the existing process terminates.
    
    Requires the original command-line arguments as passed to the main()
    function.  If a new process is started, any arguments that match any
    of the values in the (NULL terminated) 'sswitch' array will be omitted
    from the arguments to the new process.  You should specify the
    command-line switch(es) your main program uses to run as a background
    service.   If it returns, returns 0 if okay, and returns -1 if there
    was an error.

    The precise effect of this function depends on the system.  On UNIX,
    does this:
    <LIST>
    Switches the process to run as a background job, under init;
    closes all open files;
    moves to a safe, known working directory, if required;
    sets the umask for new files;
    opens stdin, stdout, and stderr to the null device;
    enforces exclusive execution through a lock file, if required;
    logs the process id in the lock file;
    ignores the hangup unwanted signals.
    </LIST>
    On OS/2, starts a new copy of the program up, detached, and running
    separately from the current process, and if successful, the existing
    process exits.

    On other systems does nothing except return 0.
    ---------------------------------------------------------------------[>]-*/

int
process_server (
    const char *workdir,                /*  Where server runs, or NULL/""    */
    const char *lockfile,               /*  For exclusive execution          */
    int   argc,                         /*  Original command-line arguments  */
    char *argv [],                      /*  Original command-line arguments  */
    const char *sswitch [])             /*  Filter these options from argv   */
{
#if (defined (__UNIX__))
    int
        fork_result,
        file_handle;
    char
        pid_buffer [10];
    struct flock
        lock_file;                      /*  flock() argument block           */
#endif
    int
        argi = 0,                       /*  Input arguments iterator         */
        argo = 0;                       /*  Output arguments iterator        */
    char
        **newargv = NULL;               /*  Array of new arguments           */

    newargv = malloc (argc * sizeof (char *));
    if (newargv == NULL)
        return (-1);

    /*  Copy the arguments across, skipping any in sswitch                   */
    for (argi = argo = 0; argi < argc; argi++)
      {
        Bool
            copy_argument = TRUE;
        int
            i = 0;

        for (i = 0; sswitch != NULL && sswitch [i] != NULL; i++)
          {
            if (strcmp (argv [argi], sswitch [i]) == 0)
                copy_argument = FALSE;
          }
        if (copy_argument)
            newargv [argo++] = argv [argi];
      }

    /*  Terminate the new argument array                                     */
    newargv [argo] = NULL;

#if (defined (__UNIX__))
    /*  We recreate our process as a child of init.  The process continues   */
    /*  to exit in the background.  UNIX calls wait() for all children of    */
    /*  the init process, so the server will exit cleanly.                   */

    fork_result = fork ();
    if (fork_result < 0)                /*  < 0 is an error                  */
        return (-1);                    /*  Could not fork                   */
    else
    if (fork_result > 0)                /*  > 0 is the parent process        */
        exit (EXIT_SUCCESS);            /*  End parent process               */

    /*  We close all open file descriptors that may have been inherited      */
    /*  from the parent process.  This is to reduce the resources we use.    */

    for (file_handle = FILEHANDLE_MAX - 1; file_handle >= 0; file_handle--)
        close (file_handle);            /*  Ignore errors                    */

    /*  We move to a safe and known directory, which is supplied as an       */
    /*  argument to this function (or not, if workdir is NULL or empty).     */

    if (workdir && strused (workdir))
        chdir (workdir);

    /*  We set the umask so that new files are given mode 750 octal          */

    umask (027);                        /*  Complement of 0750               */

    /*  We set standard input and output to the null device so that any      */
    /*  functions that assume that these files are open can still work.      */

    file_handle = open ("/dev/null", O_RDWR);    /*  stdin = handle 0        */
    dup (file_handle);                           /*  stdout = handle 1       */
    dup (file_handle);                           /*  stderr = handle 2       */

    /*  We enforce a lock on the lockfile, if specified, so that only one    */
    /*  copy of the server can run at once.  We return -1 if the lock fails. */
    /*  This locking code might be better isolated into a separate package,  */
    /*  since it is not very portable between unices.                        */

    if (lockfile && strused (lockfile))
      {
        file_handle = open (lockfile, O_RDWR | O_CREAT, 0640);
        if (file_handle < 0)
            return (-1);                /*  We could not open lock file      */
        else
          {
            lock_file.l_type = F_WRLCK;
            if (fcntl (file_handle, F_SETLK, &lock_file))
                return (-1);            /*  We could not obtain a lock       */
          }
        /*  We record the server's process id in the lock file               */
        snprintf (pid_buffer, sizeof (pid_buffer), "%6d\n", getpid ());
        write   (file_handle, pid_buffer, strlen (pid_buffer));
      }

    /*  We ignore any hangup signal from the controlling TTY                 */
    signal (SIGHUP, SIG_IGN);
    return (0);                         /*  Initialisation completed ok      */

#elif (defined (__OS2__))

    /*  Start a new copy of the program up, running detached from this
     *  program.  Quote the arguments to prevent expansion because they've
     *  already been expanded once if applicable.  The program name is taken
     *  to be argv[0] due to the lack of better options, and the path will be
     *  searched to try to find it.
     *
     *  Providing the program starts successfully, this process exits,
     *  otherwise an error is returned.
     */
#   if (defined (__EMX__))

    if (spawnvp ((P_NOWAIT | P_DETACH | P_QUOTE), newargv [0], newargv) == -1)
        return -1;
    else
        exit (EXIT_SUCCESS);

#   else
#   error Do not know how to run detached program.    
#   endif 

#else
    return (0);                         /*  Nothing to do on this system     */
#endif
}


#if (defined (__WINDOWS__))

/*  handle_timer -- internal
 *
 *  This function is called by Windows when the timer goes off.  We use this
 *  to send a SIGALRM to whoever is handling signals.  (SIGALRM is actually
 *  SIGFPE, since MSVC does not know SIGALRM, and its raise() function
 *  only works with the pathetic little group of standard signals.)
 *  We call WSACancelBlockingCall(), which *should* unblock any select() or
 *  other blocking winsock call that is in progress.  If you are waiting in
 *  a select() loop, and a timer goes off, you want to go handle it right
 *  away.  Sadly, this does not work with all (any?) winsocks.  So, a word
 *  to the wise: if you use select() and timers, under Windows you should
 *  use a select() timeout that matches the level of responsiveness that
 *  you need.  (Typically 100-500ms.)
 */
static UINT                             /*  We only want a single timer to   */
    last_timer = 0;                     /*    be active at once.             */
#   if (defined (WIN32))
void FAR PASCAL handle_timer (UINT idTimer, UINT msg,
                              DWORD dw1, DWORD dw2, DWORD dw3)
#   else
void CALLBACK handle_timer (HWND hwnd, UINT msg, UINT timerid, DWORD time)
#   endif
{
    WSACancelBlockingCall ();           /*  Force "interrupted system call"  */
    raise (SIGALRM);                    /*  Simulate UNIX signal             */
    last_timer = 0;                     /*    and note that timer has gone   */
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: process_alarm

    Synopsis: Sets a system timer to raise a SIGALRM after a specified
    interval in milliseconds.  Returns TRUE if the timer could be created
    and FALSE if there were insufficient resources, or if the system does
    not support timers.  Permits a single alarm for the current process:
    any alarm that was still pending when you called this function is
    annulled.  The implementation is system-dependent and highly
    non-portable.

    Under UNIX we use the setitimer() system function, which is clean and
    simple.

    BeOS does not support this, so we use the good old alarm() function.

    Under 16-bit Windows we use the SetTimer() call.  This does not work
    in 32-bit console applications.  Under 32-bit Windows we use the
    'multimedia' timer, which provides better resolution and does work
    in console applications.  In both these cases we cache the id of the
    last-created alarm (and kill it before each new request), to avoid
    multiple active alarms.  It is not a good idea to create too many
    concurrent timers; after 16 or so the alarms start to fail.  This is
    not supposed to happen with MM timers, but does anyway.  Under
    Windows, SIGALRM does not exist.  Since signal() only accepts one of
    a small set of fixed signals, we hijack the SIGFPE signal...  It's a
    compromise and requires that any code which expects a SIGALRM does
    not use SIGFPE.  This can be tweaked in prelude.h.

    Under OS/2 we use the alarm() function which is accurate to one
    second only.  The required accuracy of timing is not easily
    achieved, so process_alarm() rounds down to whole seconds (except if
    rounding down would give 0, in which case it will delay 1 second).
    This will probably cause problems in code applications that depends
    on sub-second timing resolution.

    Under OpenVMS 7 and later we use the setitimer() function as for
    UNIX.  Under OpenVMS 6 and earlier we use the alarm() function as
    for OS/2.  This code may be tuned to use native VMS system calls.
    ---------------------------------------------------------------------[>]-*/

Bool
process_alarm (long delay)
{
#if ((defined (__UNIX__) && !defined (__UTYPE_BEOS)) || defined (__VMS_XOPEN))
    struct itimerval
        timeout;                        /*  Timeout for setitimer            */

    /*  If the system supports interval timers, ask for a signal             */
    timeout.it_interval.tv_sec  = 0;
    timeout.it_interval.tv_usec = 0;
    timeout.it_value.tv_sec     = delay / 1000;
    timeout.it_value.tv_usec    = delay % 1000 * 1000L;
    setitimer (ITIMER_REAL, &timeout, 0);
    return (TRUE);

#elif (defined (__OS2__) || defined (__VMS__) || defined (__UTYPE_BEOS))
    /*  Since we use alarm() for our timeout, we can only time to            */
    /*  the nearest second, and alarm(0) turns off the alarm.                */
    /*  NOTE: we also have only one timer -- if alarm() is called while      */
    /*  the alarm is active, then it will be reset to the new value, and     */
    /*  only a single SIGALRM will be generated.                             */
    delay = (delay < 1000) ? 1 : (delay / 1000);
    alarm (delay);
    return (TRUE);

#elif (defined (__WINDOWS__))
#   if (defined (WIN32))
#   pragma comment (lib, "winmm")       /*  Link-in multimedia library       */
    /*  The multimedia timer gives the best accuracy, and works in console   */
    /*  applications                                                         */
    int rc;
    if (last_timer)
        __try {
            rc = timeKillEvent (last_timer);
        }
        __except (1) {
            coprintf ("timeKillEvent %d failed", last_timer);
        }
    __try {
        last_timer = timeSetEvent (delay, 50, handle_timer, 0, TIME_ONESHOT);
    }
    __except (1) {
        coprintf ("timeSetEvent %ld failed", delay);
    }
    return (TRUE);

#   else
    /*  But the normal Windows timer will do if we're in 16 bits             */
    if (last_timer)
        KillTimer ((HWND) NULL, last_timer);

    last_timer = SetTimer ((HWND) NULL, 0, (UINT) delay, handle_timer);
    return (TRUE);
#   endif

#else
    return (FALSE);                     /*  No timers - function failed      */
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_esc

    Synopsis: Escapes a directory string so that process_create() can handle
    it correctly.  If you pass a command to process_create with a directory
    name that contains spaces, it will assume that the spaces delimit the
    command from its arguments.  For instance, under Windows 95, the filename
    "C:\Program Files\Myprog.exe" will be incorrectly treated as a program
    called "C:\Program" with arguments "Files\Myprog.exe".  This function
    replaces spaces by the escape character (0x1B).  You cannot use this
    value in a filename and expect process_create() to work correctly.  On
    an EBCDIC system, the escape character (0x27) is also used.  If the
    dest argument is NULL, allocates a string using mem_alloc() and returns
    that.  Otherwise copies into the dest string and returns that.  If the
    src string is NULL, returns an empty string.
    ---------------------------------------------------------------------[>]-*/

char *
process_esc (char *dest, const char *src)
{
#if (defined (__EBCDIC__))
#   define ESC_CHAR   0x27
#else
#   define ESC_CHAR   0x1B
#endif
    /*  Copy to dest, allocate if necessary                                  */
    if (dest != src)
      {
        xstrcpy_debug ();
        dest = xstrcpy (dest, src, NULL);
      }
    strconvch (dest, ' ', ESC_CHAR);
    return (dest);
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_unesc

    Synopsis: Does the reverse translaction to process_esc().
    ---------------------------------------------------------------------[>]-*/

char *
process_unesc (char *dest, const char *src)
{
    /*  Copy to dest, allocate if necessary                                  */
    if (dest != src)
      {
        xstrcpy_debug ();
        dest = xstrcpy (dest, src, NULL);
      }
    strconvch (dest, ESC_CHAR, ' ');
    return (dest);
}


/*  ---------------------------------------------------------------------[<]-
    Function: process_priority

    Synopsis: Sets process priority as specified, to one of PRIORITY_LOW,
    PRIORITY_NORMAL, or PRIORITY_HIGH.  Currently has an effect only under
    Windows NT/95.  Returns 0 if okay, -1 if there was an error.
    ---------------------------------------------------------------------[>]-*/

int
process_priority (int priority)
{
#if (defined (WIN32))
    int
        class;

    if (priority == PRIORITY_HIGH)
        class = HIGH_PRIORITY_CLASS;
    else
    if (priority == PRIORITY_LOW)
        class = IDLE_PRIORITY_CLASS;
    else
        class = NORMAL_PRIORITY_CLASS;

    return (SetPriorityClass (GetCurrentProcess (), class));
#else
    return (0);
#endif
}


