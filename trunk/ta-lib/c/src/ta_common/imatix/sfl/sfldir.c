/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfldir.c
    Title:      Directory access functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/02  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfldate.h"                    /*  Date handling functions          */
#include "sfluid.h"                     /*  Uid/gid functions                */
#include "sflstr.h"                     /*  String functions                 */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflnode.h"                    /*  Linked-list functions            */
#include "sflfile.h"                    /*  File-access functions            */
#include "sflcons.h"                    /*  Console display functions        */
#include "sflprint.h"                   /*  snprintf functions               */
#include "sfldir.h"                     /*  Prototypes for functions         */


#include "ta_memory.h"
#include "ta_trace.h"

/*  Static variables used globally in this source file only                  */

#if 0
!!! Not used within TA-Lib
static char
    *sort_key;
#endif

TA_FILE_INFO;

/*  Function prototypes                                                      */

static Bool   populate_entry     (TA_Libc *libHandle,DIRST *dir);
static Bool  path_delimiter      (char delim);


#if 0
!!! Not needed within TA-Lib

static time_t get_six_months_ago (void);
static char  *format_mode        (DIRST *dir);
static char  *format_name        (DIRST *dir, Bool full);
static char  *format_time        (DIRST *dir);

static Bool  compare_dir         (LIST *p1, LIST *p2);
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: open_dir

    Synopsis:
    Creates a directory stream and returns the first entry in the directory.
    The order of entries is arbitrary, and it is undefined whether you will
    get entries like '.' and '..' or not.  Returns TRUE if something was
    found, else FALSE.  If TRUE, fills-in the values in the directory stream
    block.  Use the same directory stream block for the read_dir and
    close_dir() functions.

    You must supply a DIRST block when calling open_dir().  If you do not
    supply a dir_name (i.e. it is NULL or ""), open_dir() assumes you want
    to read from the current working directory.  You must call close_dir()
    after an open_dir(), even if it fails.

    The strings in DIRST all point to static areas that may change after a
    further call to read_dir.  If you need persistent data (i.e. because
    you want to collect a set of DIRSTs and then sort them, call fix_dir()
    after each call to open_dir and read_dir.  You should then call
    free_dir() to release each DIRST when you are finished.
    ---------------------------------------------------------------------[>]-*/

Bool
open_dir (
    TA_Libc *libHandle,
    DIRST *dir,
    const char *dir_name)
{
    char
        *dir_spec,                      /*  Directory to search through      */
        *dir_spec_end;                  /*  Points to NULL in dir_spec       */
    
    TA_ASSERT_RET (libHandle,dir != NULL,FALSE);
    memset (dir, 0, sizeof (DIRST));

    /*  Copy and prepare the directory specification                         */
    dir_spec = TA_Malloc( libHandle, NAME_MAX );
    if (dir_name == NULL || *dir_name == 0)
        strcpy (dir_spec, DEFAULT_DIR);
    else
        strcpy (dir_spec, dir_name);

#if (defined (MSDOS_FILESYSTEM))
    strconvch (dir_spec, '/', '\\');
#endif
    /*  Remove a trailing slash from the directory name                      */
    dir_spec_end = dir_spec + strlen (dir_spec);
    if (dir_spec_end [-1] == PATHEND)
      {
        dir_spec_end [-1] = '\0';
        dir_spec_end--;
      }

    /*  Open directory stream or find first directory entry                  */
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    if (strnull (dir_spec))
        strcpy (dir_spec, "/");

    #if (defined (__OS2__))
       if (dir_spec_end [-1] == ':')                /*  Special case d: to d:\  */
           strcat (dir_spec, "\\");
    #endif         
    
    if ((dir-> _dir_handle = opendir (dir_spec)) == NULL)
#elif (defined (WIN32))
    strcat (dir_spec, "\\*");
    if ((dir-> _dir_handle = FindFirstFile (dir_spec, &dir-> _dir_entry))
                           == INVALID_HANDLE_VALUE)

#elif (defined (_MSC_VER))
    strcat (dir_spec, "\\*.*");
    if ((dir-> _dir_handle = _dos_findfirst (dir_spec, _A_NORMAL | _A_SUBDIR,
                                             &dir-> _dir_entry)) != 0)

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    strcat (dir_spec, "\\*.*");
    if (findfirst (dir_spec, &dir-> _dir_entry, 255 - FA_LABEL) == -1)
#endif
      {
        TA_Free (libHandle, dir_spec);
        return (FALSE);                 /*  Could not open directory         */
      }

    /*  Save the directory name in directory stream structure                */
#if (defined (__MSDOS__) || defined (__OS2__))
    *dir_spec_end = '\0';               /*  Kill the \*.* again              */
#endif
    dir-> dir_name = dir_spec;          /*  Now owned by DIRST structure     */

#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    /*  Under UNIX & VMS we still need to fetch the first file entry         */
    return (read_dir (libHandle,dir));

#elif (defined (WIN32))
    /*  Under Win32 we have read an entry, so return those values            */
    return (populate_entry (libHandle,dir));

#elif (defined (_MSC_VER))
    /*  Under MSC we have read an entry, so return those values              */
    return (populate_entry (libHandle,dir));

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    /*  Under Borland C we have read an entry, so return those values        */
    return (populate_entry (libHandle,dir));
#else

    return (FALSE);                     /*  Directory access not supported   */
#endif
}


/*  -------------------------------------------------------------------------
 *  populate_entry -- internal
 *
 *  Sets the various public fields in the directory stream from the system-
 *  specific values in the private fields.  Returns TRUE if okay, FALSE if
 *  there was a problem getting the file status information.
 */

static Bool
populate_entry (TA_Libc *libHandle,DIRST *dir)
{

#if (defined (WIN32))
    static char
        *default_user = "user";
#else
    char
        *full_path;                    /*  Full path name of the file        */
    struct stat
        stat_buf;                      /*  File status structure             */
    int
        rc;
#endif

    TA_ASSERT_RET (libHandle,dir != NULL,FALSE);
    if (dir-> _fixed)
        free_dir (libHandle,dir);                /*  De-allocate old strings if reqd   */

    /*  Get name of file from directory structure                            */
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    dir-> file_name = dir-> _dir_entry-> d_name;

#elif (defined (WIN32))
    dir-> file_name = dir-> _dir_entry.cFileName;

#elif (defined (_MSC_VER))
    dir-> file_name = dir-> _dir_entry.name;

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    dir-> file_name = dir-> _dir_entry.ff_name;
#endif

#if (defined (__UNIX__) || defined (MSDOS_FILESYSTEM))
    /*  If the filename is . or .., skip this entry and do the next.  We     */
    /*  use a little bit of a recursive call to make this code simple.       */

    if (dir-> file_name [0] == '.' && (dir-> file_name [1] == '\0'
    || (dir-> file_name [1] == '.' &&  dir-> file_name [2] == '\0')))
    {
        return (read_dir (libHandle,dir));
    }
#endif

    /*  Prepare full path and get file status information.  Most systems have
     *  some kind of stat() function; we call this, even if similar info is
     *  already available in the _dir_entry structures.  Except for Win32
     *  where we use the native Win32 API to support compilers which may not
     *  have all the C wrapper functions (e.g. RSXNT).
     */

#if (defined (WIN32))
    /*  Get file info using Win32 calls                                      */
    {
        unsigned long thi, tlo;
        double   dthi, dtlo;
        double   secs_since_1601;
        double   delta = 11644473600.;
        double   two_to_32 = 4294967296.;

        thi = dir-> _dir_entry.ftLastWriteTime.dwHighDateTime;
        tlo = dir-> _dir_entry.ftLastWriteTime.dwLowDateTime;
        dthi = (double) thi;
        dtlo = (double) tlo;
        secs_since_1601 = (dthi * two_to_32 + dtlo) / 1.0e7;

        dir-> file_time = (unsigned long) (secs_since_1601 - delta);
        dir-> file_size = dir->_dir_entry.nFileSizeLow;
        dir-> file_mode = 0;
        if (dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            dir-> file_mode |= S_IFDIR;
        else
            dir-> file_mode |= S_IFREG;
        if (!(dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
            dir-> file_mode |= S_IREAD;
        if (!(dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
            dir-> file_mode |= S_IWRITE;
        dir-> file_nlink = 1;
        dir-> owner = NULL;
        dir-> group = NULL;
    }

#else
    full_path = xstrcpy (libHandle, NULL, dir-> dir_name, "/", dir-> file_name, NULL);
    rc = stat (full_path , &stat_buf);
    TA_Free (libHandle, full_path);
    if (rc == -1)
    {
        return (FALSE);
    }

    dir-> file_time  = stat_buf.st_mtime;   /*  Modification time            */
    dir-> file_size  = stat_buf.st_size;    /*  Size in bytes                */
    dir-> file_mode  = stat_buf.st_mode;    /*  UNIX-ish permissions         */
    dir-> file_nlink = stat_buf.st_nlink;   /*  Number of links to file      */

    #if 0 
    !!! Not needed within TA-Lib
    /*  Get owner and group names                                            */
    dir-> owner = get_uid_name (stat_buf.st_uid);
    dir-> group = get_gid_name (stat_buf.st_gid);
    #endif
#endif

    /*  Prepare DOS-ish permission bits                                      */
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    dir-> file_attrs = 0;
    if ((stat_buf.st_mode & S_IREAD)  == 0)
        dir-> file_attrs |= ATTR_HIDDEN;
    if ((stat_buf.st_mode & S_IWRITE) == 0)
        dir-> file_attrs |= ATTR_RDONLY;
    if ((stat_buf.st_mode & S_IFDIR)  != 0)
        dir-> file_attrs |= ATTR_SUBDIR;

#elif (defined (WIN32))
    dir-> file_attrs = 0;
    if (dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        dir-> file_attrs |= ATTR_SUBDIR;
    if (dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        dir-> file_attrs |= ATTR_RDONLY;
    if (dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        dir-> file_attrs |= ATTR_HIDDEN;
    if (dir-> _dir_entry.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        dir-> file_attrs |= ATTR_SYSTEM;
    dir-> owner = dir-> group = default_user;

#elif (defined (_MSC_VER))
    dir-> file_attrs = (byte) dir-> _dir_entry.attrib & ATTR_MASK;

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    dir-> file_attrs = (byte) dir-> _dir_entry.ff_attrib & ATTR_MASK;
#endif

    return (TRUE);                      /*  No errors                        */
}


/*  ---------------------------------------------------------------------[<]-
    Function: read_dir

    Synopsis:
    Reads the next entry from the directory stream.  Returns TRUE if there
    was more data to read; returns FALSE if there was nothing more to read.
    Updates the fields in the directory structure when it returns TRUE.
    The strings in DIRST all point to static areas that may change after a
    further call to read_dir.  If you need persistent data (i.e. because
    you want to collect a set of DIRSTs and then sort them, call fix_dir()
    after each call to open_dir and read_dir.  You should then call
    free_dir() to release each DIRST when you are finished.
    ---------------------------------------------------------------------[>]-*/

Bool
read_dir (
    TA_Libc *libHandle,
    DIRST *dir)
{
    TA_ASSERT_RET (libHandle,dir != NULL,FALSE);
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    if ((dir-> _dir_entry =
        (struct Dirent *) readdir (dir-> _dir_handle)) != NULL)
        return (populate_entry (libHandle,dir));
    else

#elif (defined (WIN32))
    if (FindNextFile (dir-> _dir_handle, &dir-> _dir_entry))
        return (populate_entry (libHandle,dir));
    else

#elif (defined (_MSC_VER))
    if (_dos_findnext (&dir-> _dir_entry) == 0)
        return (populate_entry (libHandle,dir));
    else

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    if (findnext (&dir-> _dir_entry) == 0)
        return (populate_entry (libHandle,dir));
    else
#endif
    {
        return (FALSE);
    }
}


/*  ---------------------------------------------------------------------[<]-
    Function: close_dir

    Synopsis:
    Close the directory stream, and free any allocated memory.  You should
    call this function when you are done reading a directory, or you will
    get memory leaks.  Returns TRUE if okay, FALSE if there was an error.
    ---------------------------------------------------------------------[>]-*/

Bool
close_dir (
    TA_Libc *libHandle,
    DIRST *dir)
{
    Bool
        rc;

    TA_ASSERT_RET (libHandle,dir != NULL,FALSE);

    TA_Free (libHandle, dir-> dir_name);          /*  Free dynamically-allocated name  */
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    TA_ASSERT_RET (libHandle,dir-> _dir_handle,FALSE);
    rc = (closedir (dir-> _dir_handle) == 0);

#elif (defined (WIN32))
    rc = (Bool)FindClose (dir-> _dir_handle);

#elif (defined (_MSC_VER))
    rc = TRUE;                          /*  No function to close a dir       */

#elif (defined (__TURBOC__) || defined (__DJGPP__))
    rc = TRUE;                          /*  No function to close a dir       */
#else

    rc = FALSE;                         /*  Directory access not supported   */
#endif

    return (rc);
}


#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: format_dir

    Synopsis:
    Formats the directory entry information using the same conventions as
    the UNIX 'ls -l' command.  Returns a static buffer that contains the
    the formatted string.  If the full argument is TRUE, adds cosmetic
    hints to indicate the file type; for instance '/' if the file is a
    directory, '*' if it is executable.
    ---------------------------------------------------------------------[>]-*/

char *
format_dir (
    DIRST *dir,
    Bool full)
{
    static char
        buffer [LINE_MAX];              /*  Formatted directory entry        */

    TA_ASSERT_RET (libHandle,dir != NULL,FALSE);
    snprintf (buffer, sizeof (buffer), 
                      "%s %3d %-8.8s %-8.8s %8ld %s %s",
                      format_mode (dir),
                      dir-> file_nlink,
                      dir-> owner,
                      dir-> group,
                      (long) dir-> file_size,
                      format_time (dir),
                      format_name (dir, full)
           );
    return (buffer);
}
#endif


#if 0
!!! Not needed within TA-Lib
/*  -------------------------------------------------------------------------
 *  format_mode -- internal
 *
 *  Returns a static string holding the ASCII representation of the UNIX-ish
 *  permission bits for the directory entry specified.
 */

static char *
format_mode (
    DIRST *dir)
{
    static char
        buffer [11],                    /*  duuugggooo + null                */
        *bufptr;                        /*  Pointer into buffer              */
    dbyte
        mode = dir-> file_mode;         /*  File mode bits                   */
    int
        field;                          /*  User, group, other               */

    bufptr = buffer;
    *bufptr++ = ((mode & S_IFDIR) ? 'd' : '-');
    for (field = 0; field < 3; field++)
      {
        *bufptr++ = ((mode & (S_IREAD  >> (field * 3))) ? 'r' : '-');
        *bufptr++ = ((mode & (S_IWRITE >> (field * 3))) ? 'w' : '-');
        *bufptr   = ((mode & (S_IEXEC  >> (field * 3))) ? 'x' : '-');

#if (defined (S_ISUID))
        if (field == 0 && (mode & S_ISUID))
            *bufptr = (*bufptr == 'x' ? 's' : 'S');
        else
        if (field == 1 && (mode & S_ISGID))
            *bufptr = (*bufptr == 'x' ? 's' : 'S');
#   if (defined (S_ISVTX))
        else
        if (field == 2 && (mode & S_ISVTX))
            *bufptr = (*bufptr == 'x' ? 't' : 'T');
#   endif
#endif
        ++bufptr;
    }
    *bufptr = '\0';
    return (buffer);
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  -------------------------------------------------------------------------
 *  format_name -- internal
 *
 *  Returns a static string holding the cleaned-up filename for the directory
 *  entry specified.  Replaces any control characters by an octal string
 *  and appends '/' if the entry is a directory, or '*' if it is executable,
 *  if the full argument is TRUE.
 */

static char *
format_name (DIRST *dir, Bool full)
{
    static char
        buffer [NAME_MAX + 1],
        *fromptr,
        *bufptr;

    bufptr = buffer;
    for (fromptr = dir-> file_name; *fromptr; bufptr++, fromptr++)
      {
        *bufptr = *fromptr;
        if ((byte) *bufptr < ' ')
          {
            sprintf (bufptr, "\\%03o", (byte) *fromptr);
            bufptr += strlen (bufptr) - 1;
          }
      }
    if (full)
      {
        if (dir-> file_mode & S_IFDIR)
           *bufptr++ = '/';
        else
        if (dir-> file_mode & S_IEXEC)
           *bufptr++ = '*';
      }
    *bufptr = '\0';
    return (buffer);
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  -------------------------------------------------------------------------
 *  format_time -- internal
 *
 *  Returns a static string holding the ASCII representation of the date and
 *  time for the specified directory entry.  The format of the date and time
 *  depends on whether the file is older than 6 months, roughly:
 *
 *      Oct  9  1995        if older than 6 months
 *      Jan 12 09:55        if not older than 6 months
 */

static char *
format_time (DIRST *dir)
{
    static char
        buffer [13],                    /*  Returned string                  */
        *months = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
    static time_t
        six_months_ago = 0;
    struct tm
        *file_time;

    if (six_months_ago == 0)
        six_months_ago = get_six_months_ago ();

    file_time = safe_localtime (&dir-> file_time);
    strncpy (buffer, months + file_time-> tm_mon * 4, 4);

    if (dir-> file_time < six_months_ago
    ||  dir-> file_time > time (NULL))
        snprintf (buffer + 3, sizeof (buffer) - 3, 
                              " %2d  %4d", file_time-> tm_mday,
                                           file_time-> tm_year + 1900);
    else
        snprintf (buffer + 3, sizeof (buffer) - 3,
                              " %2d %02d:%02d", file_time-> tm_mday,
                                                file_time-> tm_hour,
                                                file_time-> tm_min);
    return (buffer);
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  -------------------------------------------------------------------------
 *  get_six_months_ago -- internal
 *
 *  Returns system clock 6 months ago.
 */

static time_t
get_six_months_ago ()
{
    int
        count,
        year,
        month;
    long
        days;
    time_t
        time_val;
    struct tm
        *time_now;

    time_val = time (NULL);
    time_now = safe_localtime (&time_val);
    month    = time_now-> tm_mon + 1;
    year     = time_now-> tm_year;

    days = 0;
    for (count = 0; count < 6; count++)
      {
        if (--month < 1)                /*  Start with last month            */
          {
            month = 12;
            year--;
          }
        switch (month)
          {
            case 2:                     /*  Feb                              */
                days += leap_year (year)? 29: 28;
                break;
            case 1:                     /*  Jan                              */
            case 3:                     /*  Mar                              */
            case 5:                     /*  May                              */
            case 7:                     /*  Jul                              */
            case 8:                     /*  Aug                              */
            case 10:                    /*  Oct                              */
            case 12:                    /*  Dec                              */
                days++;
            case 4:                     /*  Apr                              */
            case 6:                     /*  Jun                              */
            case 9:                     /*  Sep                              */
            case 11:                    /*  Nov                              */
                days += 30;
        }
    }
    /*  60 * 60 * 24 = 86400 sec / day                                       */
    return (time (NULL) - (long) (86400L * days));
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: fix_dir

    Synopsis:
    Converts all strings in the DIRST into permenant values, by allocating
    heap memory for each string.  You must call this function if you intend
    to keep a set of DIRSTs, for searching or sorting.  You do not need to
    call fix_dir() if you handle each call to read_dir() independently.
    If you use fix_dir(), you must call free_dir() for each DIRST when you
    terminate.  Returns 0 if okay, -1 if there was insufficient memory or
    another fatal error.
    ---------------------------------------------------------------------[>]-*/

int
fix_dir (DIRST *dir)
{
    char
        *owner,
        *group,
        *file_name;

    /*  Allocate each string                                                 */
    owner     = mem_strdup (dir-> owner);
    group     = mem_strdup (dir-> group);
    file_name = mem_strdup (dir-> file_name);

    /*  If all okay, assign new strings and indicate everything okay         */
    if (owner && group && file_name)
      {
        dir-> owner     = owner;
        dir-> group     = group;
        dir-> file_name = file_name;
        dir-> _fixed    = TRUE;
        return (0);
      }
    else
      {
        /*  Otherwise patch things back the way they were                    */
        if (owner)
            mem_free (owner);
        if (group)
            mem_free (group);
        if (file_name)
            mem_free (file_name);
        return (-1);
      }
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: free_dir

    Synopsis:
    Frees all strings used in the DIRST.  You should call this function to
    free space allocated by fix_dir().  If you try to call free_dir() for a
    DIRST that was not fixed, you will get an error feedback, and (if you
    compiled with DEBUG defined) an assertion fault.  Returns 0 if okay,
    -1 if there was an error.  After a call to free_dir(), do not try to
    access the strings in DIRST; they are all set to point to an empty
    string.
    ---------------------------------------------------------------------[>]-*/

int
free_dir (TA_Libc *libHandle, DIRST *dir)
{
    static char
        *empty = "";

    TA_ASSERT_RET (libHandle,dir-> _fixed,FALSE);

    if (dir-> _fixed)
      {
        /*  Free allocated strings                                           */
        TA_Free (libHandle,dir-> owner);
        TA_Free (libHandle,dir-> group);
        TA_Free (libHandle,dir-> file_name);

        /*  Now point the strings to an empty string                         */
        dir-> owner     = empty;
        dir-> group     = empty;
        dir-> file_name = empty;

        /*  And mark the DIRST as no longer 'fixed'                          */
        dir-> _fixed    = FALSE;
        return (0);
      }
    else
        return (-1);
}

#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: load_dir_list

    Synopsis:
    Loads and sorts the contents of a directory.  Returns a NODE pointer to
    a linked list containing the directory entries.  Each node is a FILEINFO
    structure (mapped onto a NODE structure for purposes of manipulating the
    linked list).  You can ask for the directory list to be sorted in various
    ways; in this case subdirectory entries are always sorted first.  To
    specify the sort order you pass a string consisting of one or more of
    these characters, which are then used in order:
    <TABLE>
    n      Sort by ascending name.
    N      Sort by descending name.
    x      Sort by ascending extension.
    X      Sort by descending extension.
    t      Sort by ascending time and date.
    T      Sort by descending time and date.
    s      Sort by ascending size.
    S      Sort by descending size.
    </TABLE>
    If the sort string is NULL, no sort is carried out.
    ---------------------------------------------------------------------[>]-*/

NODE *
load_dir_list (
    const char *dir_name,
    const char *sort)
{
    NODE
        *file_list;                     /*  File list head                   */
    DIRST
        dir;
    Bool
        rc;
    int
        nbr_files = 0;

    file_list = mem_alloc (sizeof (NODE));
    if (!file_list)
        return (NULL);

    node_reset (file_list);             /*  Initialise file list             */

    /* Load directory                                                        */
    rc = open_dir (&dir, dir_name);
    while (rc)
      {
        add_dir_list (file_list, &dir);
        nbr_files++;
        rc = read_dir (&dir);
      }
    close_dir (&dir);

    if (sort && nbr_files > 1)
        sort_dir_list (file_list, sort);
    return (file_list);
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: free_dir_list

    Synopsis:
    Frees all FILELIST blocks in the specified linked list.
    ---------------------------------------------------------------------[>]-*/

void
free_dir_list (NODE *file_list)
{
    TA_ASSERT_NO_RET (libHandle,file_list);

    while (file_list-> next != file_list)
      {
        free_dir (&((FILEINFO *) file_list-> next)-> dir);
        node_destroy (file_list-> next);
      }
    mem_free (file_list);
}
#endif


#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: sort_dir_list

    Synopsis: Sorts the directory list as specified.  Returns the number of
    items in the list.  To specify the sort order you pass a string holding
    one or more of these characters, which are then used in order:
    <TABLE>
    n      Sort by ascending name.
    N      Sort by descending name.
    x      Sort by ascending extension.
    X      Sort by descending extension.
    t      Sort by ascending time and date.
    T      Sort by descending time and date.
    s      Sort by ascending size.
    S      Sort by descending size.
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

void
sort_dir_list (NODE *file_list, const char *sort)
{
    TA_ASSERT_NO_RET (libHandle,file_list);
    sort_key = (char *) sort;
    list_sort (file_list, compare_dir);
}
#endif

#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: add_dir_list

    Synopsis: Adds a node to the specified directory list.  Returns the
    address of the new node, or NULL if there was insufficient memory.
    ---------------------------------------------------------------------[>]-*/

FILEINFO *
add_dir_list (NODE *file_list, const DIRST *dir)
{
    FILEINFO
        *file_info;

    file_info = (FILEINFO *) node_create (file_list-> prev, sizeof (FILEINFO));
    if (file_info)
      {
        memcpy  (&file_info-> dir, dir, sizeof (DIRST));
        fix_dir (&file_info-> dir);
        file_info-> directory = (dir-> file_attrs & ATTR_SUBDIR) != 0;
      }
    return (file_info);
}
#endif

#if 0
!!! not needed within TA-Lib
/*  -------------------------------------------------------------------------
 *  compare_dir -- internal
 *
 *  Compare two file info structures and return TRUE if SWAP is needed.
 *  Uses the global variable sort_key to determine sorting method.
 */
static Bool
compare_dir (LIST *p1, LIST *p2)
{
    Bool
        end  = FALSE,
        swap = FALSE;
    char
        *ext1,
        *ext2,
        *p;
    int
        comp;
    long
        date1,
        date2,
        time1,
        time2;

    TA_ASSERT_RET (libHandle,p1,FALSE);
    TA_ASSERT_RET (libHandle,p2,FALSE);

    if (((FILEINFO *) p1)-> directory != ((FILEINFO *) p2)-> directory)
        return (((FILEINFO *) p2)-> directory);

    for (p = sort_key;
            *p   != '\0'
         && end  == FALSE
         && swap == FALSE;
         p++)
      {
        switch (*p)
          {
            case 'n':
            case 'N':
                comp = lexcmp (((FILEINFO *) p1)-> dir.file_name,
                               ((FILEINFO *) p2)-> dir.file_name);
                if ((*p == 'n' && comp > 0)
                ||  (*p == 'N' && comp < 0))
                    swap = TRUE;
                else
                if (comp != 0)
                    end = TRUE;
                break;
            case 't':
            case 'T':
                /* Use date and time and not time_t for compare to minute,
                   not second                                                */
                date1 = timer_to_date (((FILEINFO *) p1)-> dir.file_time);
                date2 = timer_to_date (((FILEINFO *) p2)-> dir.file_time);
                if ((*p == 't' && date1 > date2)
                ||  (*p == 'T' && date1 < date2))
                    swap = TRUE;
                else
                if (date1 != date2)
                    end = TRUE;
                else
                  {
                    time1 = timer_to_time
                                (((FILEINFO *) p1)-> dir.file_time) / 10000;
                    time2 = timer_to_time
                                (((FILEINFO *) p2)-> dir.file_time) / 10000;
                    if ((*p == 't' && time1 > time2)
                    ||  (*p == 'T' && time1 < time2))
                        swap = TRUE;
                    else
                    if (time1 != time2)
                        end = TRUE;
                  }
                break;
            case 's':
            case 'S':
                if ((*p == 's' && ((FILEINFO *) p1)-> dir.file_size >
                                  ((FILEINFO *) p2)-> dir.file_size)
                ||  (*p == 'S' && ((FILEINFO *) p1)-> dir.file_size <
                                  ((FILEINFO *) p2)-> dir.file_size))
                    swap = TRUE;
                else
                if (((FILEINFO *) p1)-> dir.file_size
                !=  ((FILEINFO *) p2)-> dir.file_size)
                    end = TRUE;
                break;
            case 'x':
            case 'X':
                ext1 = strchr (((FILEINFO *) p1)-> dir.file_name, '.');
                ext2 = strchr (((FILEINFO *) p2)-> dir.file_name, '.');
                if (ext1 && ext2)
                  {
                    comp = lexcmp (ext1, ext2);
                    if ((*p == 'x' && comp > 0)
                    ||  (*p == 'X' && comp < 0))
                        swap = TRUE;
                    else
                    if (comp != 0)
                        end = TRUE;
                  }
                break;

          }
      }
    return (swap);
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: resolve_path

    Synopsis: Accepts a path consisting of zero or more directory names and
    optionally a filename plus extension. Removes '.' and '..' if they occur
    in the path. '..' is resolved by also removing the preceding directory
    name, if any.  Returns a freshly-allocated string containing the
    resulting path.  The caller must free this string using TA_Free() when
    finished with it.  The returned path may be empty.  Under OS/2 and DOS,
    treats '\' and '/' both as directory separators. A '..' directory at the
    start of the path resolves into nothing. If the input path started with
    '/', the returned path also does, else it does not.  For compatibility
    with DOS-based systems, '...' is treated as '../..', '....' as
    '../../..', and so on.
    ---------------------------------------------------------------------[>]-*/

char *
resolve_path (
    TA_Libc *libHandle,
    const char *old_path)
{
#if (defined (__UNIX__) || defined (MSDOS_FILESYSTEM) || defined (__VMS__))
    char
        *new_path,                      /*  Newly-allocated path             */
        *new_ptr,                       /*  Pointer into new_path            */
        last_char = '/';                /*  Start of path counts as delim    */
    int
        nbr_dots;                       /*  Size of '..', '...' specifier    */

    TA_ASSERT_RET (libHandle,old_path,NULL);

    new_path = TA_MallocCopy( libHandle,old_path,strlen(old_path)+1);
    new_ptr  = new_path;
    while (*old_path)
      {
        if (path_delimiter (last_char) && *old_path == '.')
          {
            /*  Handle one or more dots followed by a path delimiter         */
            nbr_dots = 0;               /*  Count number of dots             */
            while (old_path [nbr_dots] == '.')
                nbr_dots++;

            if (path_delimiter (old_path [nbr_dots]))
              {
                old_path += nbr_dots;   /*  Skip past dots                   */
                if (*old_path)
                    old_path++;         /*    and past / if any              */

                /*  Now backtrack in new path, dropping directories as       */
                /*  many times as needed (0 or more times)                   */
                while (nbr_dots > 1)
                  {
                    if (new_ptr > new_path + 1)
                      {
                        new_ptr--;      /*  Drop delimiter                   */
                        while (new_ptr > new_path)
                          {
                            if (path_delimiter (*(new_ptr - 1)))
                                break;  /*    and end after delimiter        */
                            new_ptr--;
                          }
                      }
                    else
                        break;          /*  At start of name - finish        */
                    nbr_dots--;
                  }
              }
            else
                /*  Handle '.something'                                      */
                last_char = *new_ptr++ = *old_path++;
          }
        else
            last_char = *new_ptr++ = *old_path++;
      }

    *new_ptr = '\0';                    /*  Terminate string nicely          */
    return (new_path);
#else

    return (TA_MallocCopy(libHandle,old_path,strlen(old_path)+1));     /*  Path resolution not supported    */
#endif
}

static Bool
path_delimiter (char delim)
{
#if (defined (MSDOS_FILESYSTEM))
    if (delim == '\\' || delim == '/' || delim == '\0')
        return (TRUE);
    else
#elif (defined (__UNIX__) || defined (__VMS__))
    if (delim == '/' || delim == '\0')
        return (TRUE);
    else
#endif
        return (FALSE);
}


#if 0
!!! Not needed within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: locate_path

    Synopsis: Accepts a root directory and a path and locates the path with
    respect to the root.  If the path looks like an absolute directory,
    returns the path after cleaning it up.  Otherwise appends the path to
    the root, and returns the result.  In any case, the resulting directory
    does not need to exist.  Cleans-up the returned path by appending a '/'
    if necessary, and resolving any '..' subpaths.  The returned value is
    held in a freshly-allocated string which the caller must free using
    mem_free() when finished with..
    ---------------------------------------------------------------------[>]-*/

char *
locate_path (
    TA_Libc *libHandle,
    const char *root,
    const char *path)
{
#if (defined (__UNIX__) || defined (MSDOS_FILESYSTEM) || defined (__VMS__))
    char
        *new_path,                      /*  Returned path value              */
        *resolved;                      /*  and after .. resolution          */

    TA_ASSERT_RET (libHandle,root,NULL);
    TA_ASSERT_RET (libHandle,path,NULL);

#if (defined (MSDOS_FILESYSTEM))
    /*  Under MSDOS, OS/2, or Windows we have a full path if we have any of:
     *  /directory
     *  D:/directory
     *  the variations of those with backslashes.
     */
    if (path [0] == '\\'   || path [0] == '/'
    || (isalpha (path [0]) && path [1] == ':'
    && (path [2] == '\\'   || path [2] == '/')))

#else
    /*  Under UNIX or VMS we have a full path if the path starts with the
     *  directory marker.
     */
    if (path [0] == PATHEND)
#endif
        new_path = mem_strdup (path);   /*  Use path as supplied             */
    else
      {
        xstrcpy_debug ();
        if (path_delimiter (strlast (root)))
            new_path = xstrcpy (NULL, root, path, NULL);
        else
            new_path = xstrcpy (NULL, root, "/", path, NULL);
      }
    resolved = resolve_path (libHandle,new_path);
    mem_free (new_path);
    /*  Append slash if necessary                                            */
    if (!path_delimiter (strlast (resolved)))
      {
        new_path = resolved;
        xstrcpy_debug ();
        resolved = xstrcpy (NULL, new_path, "/", NULL);
        mem_free (new_path);
      }
    return (resolved);
#else

    return (mem_strdup (path));         /*  Unknown system                   */
#endif
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: clean_path

    Synopsis: Returns a clean directory name; i.e. resolves the path,
    removes a trailing slash unless the name consists just of '/'; on a
    MS-DOS file system, cleans-up a directory name consisting of a disk
    specifier.  The cleaned-up directory path is in a static area that is
    overwritten by each call.
    ---------------------------------------------------------------------[>]-*/

char *
clean_path (
    TA_Libc *libHandle,
    const char *path)
{

#if (defined (__UNIX__) || defined (MSDOS_FILESYSTEM) || defined (__VMS__))
    static char
        new_path [PATH_MAX + 1];        /*  Returned path value              */
    char
        *slash;

   (void)libHandle;

    strncpy (new_path, path, PATH_MAX);
    new_path [PATH_MAX] = '\0';
#   if (defined (MSDOS_FILESYSTEM))
    /*  For DOS filesystems, use only back slashes                           */
    strconvch (new_path, '/', '\\');
#   endif
    slash = strrchr (new_path, PATHEND);    /*  Find last slash              */

#   if (defined (MSDOS_FILESYSTEM))
    /*  If slash is last character in string, maybe squash it                */
    if (slash && slash [1] == '\0')
      {
        if (slash > new_path && slash [-1] != ':')
            *slash = '\0';
      }
    else                                /*  Turn X: into X:\                 */
    if (new_path [1] == ':' && new_path [2] == '\0')
      {
        new_path [2] = '\\';
        new_path [3] = '\0';
      }
#   else
    /*  If slash is last character in string, maybe squash it                */
    if (slash && slash [1] == '\0')
        if (slash > new_path)
            *slash = '\0';
#   endif
    return (new_path);
#else
   (void)libHandle;
    return ((char *) path);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_curdir

    Synopsis: Returns a buffer containing the current working directory.
    This buffer is allocated using the mem_alloc() function and should be
    freed using mem_free() when no longer needed.  Returns NULL if there
    was insufficient memory to allocate the buffer, or if the system does
    not provide the current working directory information.  Under Windows,
    replaces backslash characters by the UNIX-like slash.  Under OpenVMS,
    returns directory name in POSIX format.  Note that the directory name
    always ends in a slash (e.g. 'C:/' or 'C:/subdir/').
    ---------------------------------------------------------------------[>]-*/

char *
get_curdir (TA_Libc *libHandle)
{
    char
        *curdir;                        /*  String we get from the OS        */

    curdir = TA_Malloc(libHandle,PATH_MAX + 1);

#if (defined (__UNIX__) || defined (__OS2__))
    getcwd (curdir, PATH_MAX);

#elif (defined (__VMS__))
    getcwd (curdir, PATH_MAX, 0);

#elif (defined (WIN32))
    GetCurrentDirectory (PATH_MAX, curdir);
    strconvch (curdir, '\\', '/');

#elif (defined (MSDOS_FILESYSTEM))
    getcwd (curdir, PATH_MAX);
    strconvch (curdir, '\\', '/');

#else
    strclr (curdir);
#endif

    /*  The directory must always end in a slash                             */
    if (strlast (curdir) != '/')
        strcat (curdir, "/");

    return (curdir);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_curdir

    Synopsis: Sets the current working directory as specified.  Returns 0
    if the directory path was found; -1 if there was an error.  Under
    Windows, replaces '/' by '\' before changing directory, and switches
    to the specified disk if the path starts with a letter and ':'.  Does
    nothing if the path is NULL or empty.
    ---------------------------------------------------------------------[>]-*/

int
set_curdir (
    TA_Libc *libHandle,
    const char *path)
{
    int
        feedback = 0;

#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    (void)libHandle;
    if (path && *path)
        feedback = chdir (path);

#elif (defined (MSDOS_FILESYSTEM))
    char
        *copy_path = TA_MallocCopy(libHandle,path,strlen(path)+1);

    if (path == NULL || *path == '\0')
        return (0);                     /*  Do nothing if path is empty      */

    /*  MS-DOS compilers generally require a two-step process                */
    strconvch (copy_path, '/', '\\');

#   if (defined (WIN32))
    /* The drive letter does not need to be changed separately in Win32.    */
    feedback = !SetCurrentDirectory (copy_path);

#   elif (defined (__TURBOC__))
    feedback = chdir (copy_path);
    if (feedback == 0 && isalpha (path [0]) && path [1] == ':')
        setdisk (toupper (path [0]) - 'A');

#   elif (defined (__LCC__))
    feedback = chdir (copy_path);
    if (feedback == 0 && isalpha (path [0]) && path [1] == ':')
        chdrive (toupper (path [0]) - 'A' + 1);

#   elif (defined (_MSC_VER))
    feedback = _chdir (copy_path);
    if (feedback == 0 && isalpha (path [0]) && path [1] == ':')
        _chdrive (toupper (path [0]) - 'A' + 1);

#   endif
    TA_Free (libHandle,copy_path);
#else
    feedback = -1;
#endif

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: file_matches

    Synopsis: Returns TRUE if the filename matches the pattern.  The pattern
    is a character string that can contain these 'wildcard' characters: ?,*

    This function was modified for TA-LIB:
      - Is now case insensitive on all platform.
      - Support '*' as being a zero-length trailing wildcard.
    ---------------------------------------------------------------------[>]-*/

Bool
file_matches (
    TA_Libc *libHandle,
    const char *filename,
    const char *pattern)
{   
    char
        *pattern_ptr,                   /*  Points to pattern                */
        *filename_ptr;                  /*  Points to filename               */

   (void)libHandle;

    filename_ptr = (char *) filename;   /*  Start comparing file name        */
    pattern_ptr  = (char *) pattern;    /*  Start comparing file name        */
    FOREVER
      {
        /*  If we came to the end of the pattern and the filename, we have   */
        /*  successful match.                                                */
        if (*pattern_ptr == '\0' && *filename_ptr == '\0')
            return (TRUE);              /*  Have a match                     */

        /* If the pattern is '*' and this is the end of the
         * filename_ptr, there is a match since '*' length
         * can be zero character.
         */
        if( *pattern_ptr == '*' && *filename_ptr == '\0' )
           return(TRUE);

        /*  Otherwise, end of either is a failed match                       */
        if (*pattern_ptr == '\0' || *filename_ptr == '\0')
            return (FALSE);             /*  Match failed                     */

        /*  If the pattern character is '?', then we matched a char          */
        if (*pattern_ptr == '?' ||  toupper (*pattern_ptr) == toupper (*filename_ptr))        
          {
            pattern_ptr++;
            filename_ptr++;
          }
        else
        /*  If we have a '*', match as much of the filename as we can        */
        if (*pattern_ptr == '*')
          {
            pattern_ptr++;              /*  Try to match following char      */
            while (*filename_ptr && toupper (*filename_ptr) != toupper (*pattern_ptr))
                filename_ptr++;
          }
        else
            return (FALSE);             /*  Match failed                     */
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: make_dir

    Synopsis: Create a new directory.  Returns 0 if the directory was created;
    -1 if there was an error.  Under Windows and OpenVMS, accepts directory
    names with '/'.  Will create multiple levels of directory if required.
    ---------------------------------------------------------------------[>]-*/

int
make_dir (
    TA_Libc *libHandle,
    const char *path_to_create)
{
    char
        *path,
        *slash;
    int
        rc = 0;

    path = TA_MallocCopy( libHandle, path_to_create, strlen(path_to_create)+1 ); /*  Working copy                     */
#if (defined (MSDOS_FILESYSTEM))
    strconvch (path, '/', '\\');

    /*  Handle \\system\drive specially                                      */
    if (strprefixed (path, "\\\\"))     /*  Network drive name?              */
      {
        slash = strchr (path + 2, '\\');
        if (slash)
            slash = strchr (slash + 1, '\\');
      }
    else
#endif
    slash = strchr (path + 1, PATHEND);

    /*  Create each component of directory as required                       */
    FOREVER                             /*  Create any parent directories    */
      {
        if (slash)
            *slash = '\0';              /*  Cut at slash                     */

        if (!file_is_directory (libHandle,path))
          {
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
            rc = mkdir (path, 0775);    /*  User RWE Group RWE World RE      */

#elif (defined (WIN32))
            if (CreateDirectory (path, NULL))
                rc = 0;
            else
                rc = -1;

#elif (defined (MSDOS_FILESYSTEM))
#   if (defined (__DJGPP__))
            rc = mkdir (path, 0775);    /*  User RWE Group RWE World RE      */
#   else
            rc = mkdir (path);          /*  Protection?  What's that?        */
#   endif
#else
            rc = -1;                    /*  Not a known system               */
#endif
            if (rc)                     /*  End if error                     */
                break;
          }
        if (slash == NULL)              /*  End if last directory            */
            break;
       *slash = PATHEND;                /*  Restore path name                */
        slash = strchr (slash + 1, PATHEND);
      }
    TA_Free(libHandle,path);
    return (rc);
}


/*  ---------------------------------------------------------------------[<]-
    Function: remove_dir

    Synopsis: remove a directory.  Returns 0 if the directory could be
    removed; -1 if there was an error.  Under MS-DOS and OpenVMS accepts
    a directory name in UNIX format, i.e. containing '/' delimiters.  The
    directory must be empty to be removed.
    ---------------------------------------------------------------------[>]-*/

int
remove_dir (
    TA_Libc *libHandle,
    const char *path)
{
#if (defined (__UNIX__) || defined (__VMS_XOPEN) || defined (__OS2__))
    /*  Check that directory exists                                          */
    if (!file_is_directory (libHandle,path))
        return (-1);

    return (rmdir (path));

#elif (defined (MSDOS_FILESYSTEM))
    int
        rc = 0;
    char
        *copy_path;

    /*  Check that directory exists                                          */
    if (!file_is_directory (libHandle,path))
        return (-1);

    copy_path = TA_MallocCopy(libHandle,path,strlen(path)+1);
    if (copy_path)
      {
        strconvch (copy_path, '/', '\\');
#   if (defined (WIN32))
        if (RemoveDirectory (copy_path))
            rc = 0;
        else
          {
#if 0
!!! No output please!
LPVOID lpMsgBuf;
 
FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
    (LPTSTR) &lpMsgBuf,
    0,
    NULL 
);
coprintf (lpMsgBuf);
#endif           
            rc = -1;
            }
#   else
        rc = rmdir (copy_path);
#   endif
        TA_Free( libHandle, copy_path);
      }
    return (rc);
#else
    return (-1);
#endif
}

#if 0
!!! Not used within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: dir_usage

    Synopsis: Calculates the amount of disk space used by a directory, and
    optionally all directories below that.  If the total size is greater
    than 4Gb, returns an unspecified value.  Returns 0 if there was an error.
    ---------------------------------------------------------------------[>]-*/

qbyte
dir_usage (const char *path, Bool recurse)
{
    DIRST
        dir;
    qbyte
        usage = 0;
    char
        *full_dir;

    if (open_dir (&dir, path))
    do
      {
        if ((dir.file_attrs & ATTR_HIDDEN) != 0)
            ;   /*  Do nothing                                               */
        else
        if (recurse
        && (dir.file_attrs & ATTR_SUBDIR) != 0)
          {
            full_dir = locate_path (path, dir.file_name);
            usage += dir_usage (full_dir, TRUE);
            TA_Free(libHandle,full_dir);
          }
        else
            usage += dir.file_size;
      }
    while (read_dir (&dir));
    close_dir (&dir);
    return (usage);
}
#endif

#if 0
!!! Not used within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: dir_files

    Synopsis: Calculates the number of files in a directory and optionally
    all directories below that.  Excludes directories from the count (thus,
    a directory containing only '.' and '..' contains 0 files.  Returns 0
    if there was an error.  Excludes hidden files.
    ---------------------------------------------------------------------[>]-*/

qbyte
dir_files (const char *path, Bool recurse)
{
    DIRST
        dir;
    qbyte
        files = 0;
    char
        *full_dir;

    if (open_dir (&dir, path))
    do
      {
        if ((dir.file_attrs & ATTR_HIDDEN) != 0)
            ;   /*  Do nothing                                               */
        else
        if (recurse
        && (dir.file_attrs & ATTR_SUBDIR) != 0)
          {
            full_dir = locate_path (path, dir.file_name);
            files += dir_files (full_dir, TRUE);
            TA_Free(libHandle,full_dir);
          }
        else
            files++;
      }
    while (read_dir (&dir));
    close_dir (&dir);
    return (files);
}
#endif
