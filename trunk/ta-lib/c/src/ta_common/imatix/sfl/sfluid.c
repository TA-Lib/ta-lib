/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfluid.c
    Title:      Process user id (uid) and group id (gid) functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/03  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/20

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"                     /*  String functions                 */
#include "sfluid.h"                     /*  Prototypes for functions         */


/*  Local constants and function prototypes                                  */

#define UID_CACHE_MAX       10          /*  Max. different uid's we cache    */
#define GID_CACHE_MAX       10          /*  Max. different gid's we cache    */

#define REAL_ID             0           /*  Arguments for get_uid/get_gid    */
#define EFFECTIVE_ID        1

#if (defined (DOES_UID))                /*  Only if uid/gid implemented      */
#   if (!defined (__OS2__))             /*  But not needed under OS/2        */
static uid_t  get_uid (int type);
#   if (!defined (__VMS__))             /*  No gid under OpenVMS             */
static gid_t  get_gid (int type);
#   endif
#   endif
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: get_uid_name

    Synopsis:
    Get user name from passwd file.  We optimise by keeping a table of uids
    and names in memory.  Note that this will cause problems if the program
    stays running when the passwd file has been changed.  Returns a string
    containing the translated user name, or "<none>" if the uid could not
    be translated.  Under MS-DOS the uid must be zero.  The returned string
    is in a static area that is _not_ overwritten with each call, but which
    should be treated as read-only, and unstable: i.e. the value returned
    by one call to get_uid_name may change as a result of a later call.  If
    you need persistent strings, use strdupl() after each call.
    ---------------------------------------------------------------------[>]-*/

char *
get_uid_name (uid_t uid)
{
#   if (defined (DOES_UID))
    static struct uids {                /*  Table of cached uids             */
        uid_t id;
        char  *name;
    } cache [UID_CACHE_MAX];
    static int
        cache_size = 0,                 /*  Number of uid's in cache         */
        cache_oldest = 0;               /*  Oldest entry in cache            */
    int
        cache_scan;                     /*  Scan through cache               */
    struct passwd
        *passwd_entry;

    /*  First, look for uid in cache                                         */
    for (cache_scan = 0; cache_scan < cache_size; cache_scan++)
        if (cache [cache_scan].id == uid)
            return (cache [cache_scan].name);

    /*  Add new name to cache: if cache was full, kick-out oldest entry      */
    if (cache_size == UID_CACHE_MAX)
      {
        cache_scan = cache_oldest++;
        cache_oldest %= UID_CACHE_MAX;
        free (cache [cache_scan].name);
      }
    else
        cache_scan = cache_size++;

    cache [cache_scan].id = uid;
    if ((passwd_entry = getpwuid (uid)) == NULL)
        cache [cache_scan].name = "<none>";
    else
        cache [cache_scan].name = strdupl (passwd_entry-> pw_name);

    return (cache [cache_scan].name);

#   elif (defined (__MSDOS__))
    return (uid == 0? "user": "<none>");

#   endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_gid_name

    Synopsis:
    Get group name from group file.  We optimise by keeping a table of gids
    and names in memory.  Note that this will cause problems if the program
    stays running when the group file has been changed.  Returns a string
    containing the translated user name, or "<none>" if the gid could not
    be translated.  Under MS-DOS the gid must be zero.  The returned string
    is in a static area that is _not_ overwritten with each call, but which
    should be treated as read-only, and unstable: i.e. the value returned
    by one call to get_gid_name may change as a result of a later call.  If
    you need persistent strings, use strdupl() after each call.
    ---------------------------------------------------------------------[>]-*/

char *
get_gid_name (gid_t gid)
{
#   if (defined (DOES_UID))
    static struct gids {                /*  Table of cache'd gids            */
        gid_t id;
        char  *name;
    } cache [GID_CACHE_MAX];
    static int
        cache_size = 0,                 /*  Number of gid's in cache         */
        cache_oldest = 0;               /*  Oldest entry in cache            */
    int
        cache_scan;                     /*  Scan through cache               */
    struct group
        *group_entry;

    /*  First, look for gid in cache                                         */
    for (cache_scan = 0; cache_scan < cache_size; cache_scan++)
        if (cache [cache_scan].id == gid)
            return (cache [cache_scan].name);

    /*  Add new name to cache: if cache was full, kick-out oldest entry      */
    if (cache_size == GID_CACHE_MAX)
      {
        cache_scan = cache_oldest++;
        cache_oldest %= GID_CACHE_MAX;
        free (cache [cache_scan].name);
      }
    else
        cache_scan = cache_size++;

    cache [cache_scan].id = gid;
#   if (defined (__VMS__))
        cache [cache_scan].name = "<none>";
#   else
    if ((group_entry = getgrgid (gid)) == NULL)
        cache [cache_scan].name = "<none>";
    else
        cache [cache_scan].name = strdupl (group_entry-> gr_name);
#   endif

    return (cache [cache_scan].name);

#   elif (defined (__MSDOS__))
    return (gid == 0? "group": "<none>");

#   endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_uid_user

    Synopsis: This function can be used by 'setuid' programs; i.e. programs
    that run under a fixed uid such as 'root'.  Typically such programs need
    to access root resources, but user data files.  To do this they must
    switch between the 'root' uid and the 'user' uid.  This function switches
    to the real user id.  Use set_uid_root() to switch (back) to the 'root'
    uid.  See also: set_gid_user() and set_gid_root().
    ---------------------------------------------------------------------[>]-*/

int
set_uid_user (void)
{
#if (defined (DOES_UID))
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
    return (setuid (get_uid (REAL_ID)));
#   elif (defined (__OS2__))            /*  OS/2 only supports one UID       */
    return (0);
#   elif (defined (__VMS__))            /*  No setuid under OpenVMS          */
    return (0);
#   else
    return (seteuid (get_uid (REAL_ID)));
#   endif
#else
    return (0);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_uid_root

    Synopsis: This function can be used by 'setuid' programs; i.e. programs
    that run under a fixed uid such as 'root'.  Typically such programs need
    to access root resources, but user data files.  To do this they must
    switch between the 'root' uid and the 'user' uid.  This function switches
    back to the root user id.  Use set_uid_user() to switch to the 'user'
    uid.  See also: set_gid_user() and set_gid_root().
    ---------------------------------------------------------------------[>]-*/

int
set_uid_root (void)
{
#if (defined (DOES_UID))
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
    return (setuid (get_uid (EFFECTIVE_ID)));
#   elif (defined (__OS2__))            /*  OS/2 only supports one UID       */
    return (0);
#   elif (defined (__VMS__))            /*  No setuid under OpenVMS          */
    return (0);
#   else
    return (seteuid (get_uid (EFFECTIVE_ID)));
#   endif
#else
    return (0);
#endif
}


#if (defined (DOES_UID) && !defined (__OS2__))
/*  -------------------------------------------------------------------------
    Function: get_uid_id -- internal

    Synopsis: Returns the real (REAL_ID) or effective (EFFECTIVE_ID) uid.
    These values are loaded the first time that the function is called: you
    should not rely on the effective uid after changing the uid.
    -------------------------------------------------------------------------*/

static uid_t
get_uid (int type)
{
    static int
        ruid = -1,
        euid = -1;

    if (ruid == -1)
        ruid = getuid ();
    if (euid == -1)
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
        euid = getuid ();
#   else
        euid = geteuid ();
#   endif

    if (type == REAL_ID)
        return (ruid);
    else
    if (type == EFFECTIVE_ID)
        return (euid);
    else
        return (-1);
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: set_gid_user

    Synopsis: This function can be used by 'setgid' programs; i.e. programs
    that run under a fixed gid such as 'root'.  Typically such programs need
    to access root resources, but user data files.  To do this they must
    switch between the 'root' gid and the 'user' gid.  This function switches
    to the real user id.  Use set_gid_root() to switch (back) to the 'root'
    gid.  See also: set_uid_user() and set_uid_root().
    ---------------------------------------------------------------------[>]-*/

int
set_gid_user (void)
{
#if (defined (DOES_UID))
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
    return (setgid (get_gid (REAL_ID)));
#   elif (defined (__OS2__))            /*  OS/2 only supports one UID       */
    return (0);
#   elif (defined (__VMS__))            /*  No setgid under OpenVMS          */
    return (0);
#   else
    return (setegid (get_gid (REAL_ID)));
#   endif
#else
    return (0);
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_gid_root

    Synopsis: This function can be used by 'setgid' programs; i.e. programs
    that run under a fixed gid such as 'root'.  Typically such programs need
    to access root resources, but user data files.  To do this they must
    switch between the 'root' gid and the 'user' gid.  This function switches
    back to the root user id.  Use set_gid_user() to switch to the 'user'
    gid.  See also: set_gid_user() and set_gid_root().
    ---------------------------------------------------------------------[>]-*/

int
set_gid_root (void)
{
#if (defined (DOES_UID))
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
    return (setgid (get_gid (EFFECTIVE_ID)));
#   elif (defined (__OS2__))            /*  OS/2 only supports one UID       */
    return (0);
#   elif (defined (__VMS__))            /*  No setgid under OpenVMS          */
    return (0);
#   else
    return (setegid (get_gid (EFFECTIVE_ID)));
#   endif
#else
    return (0);
#endif
}


#if (defined (DOES_UID) && !defined (__OS2__) && !defined (__VMS__))
/*  -------------------------------------------------------------------------
    Function: get_gid -- internal

    Synopsis: Returns the real (REAL_ID) or effective (EFFECTIVE_ID) gid.
    These values are loaded the first time that the function is called: you
    should not rely on the effective gid after changing the gid.
    -------------------------------------------------------------------------*/

static gid_t
get_gid (int type)
{
    static int
        rgid = -1,
        egid = -1;

    if (rgid == -1)
        rgid = getgid ();
    if (egid == -1)
#   if (defined (__UTYPE_HPUX) || defined (__UTYPE_BEOS))
        egid = getgid ();
#   else
        egid = getegid ();
#   endif

    if (type == REAL_ID)
        return (rgid);
    else
    if (type == EFFECTIVE_ID)
        return (egid);
    else
        return (0);
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: set_uid_gid

    Synopsis: Sets the program's uid and gid to new values as specified
    (as names).  The program must be currently running as 'root'.  Returns
    0 if the new names could be correctly used.  Returns -1 if the specified
    user id or group id was not valid, or -2 if the process was unable to
    change to the new uid/gid as specified.  The gid may be null or empty.
    ---------------------------------------------------------------------[>]-*/

int
set_uid_gid (char *new_uid, char *new_gid)
{
#if (defined (DOES_UID))
    struct passwd
        *pwdbuf;
    struct group
        *grpbuf;

#   if (defined (__VMS__))
    return (0);
#   else
    if (new_gid && *new_gid)
      {
        if ((grpbuf = getgrnam (new_gid)) == NULL)
            return (-1);
        else
        if (setgid (grpbuf-> gr_gid) == -1)
            return (-2);
      }
    if ((pwdbuf = getpwnam (new_uid)) == NULL)
        return (-1);
    else
    if (setuid (pwdbuf-> pw_uid) == -1)
        return (-2);
#   endif
#else
    (void)new_uid;
    (void)new_gid;
#endif
    return (0);
}

/*  ---------------------------------------------------------------------[<]-
    Function: get_login

    Synopsis: Returns the identity of the currently-logged user.  The
    returned string is in a static buffer.  Returns NULL if no user is
    currently logged-in.
    ---------------------------------------------------------------------[>]-*/

char *
get_login (void)
{
#if (defined (__WINDOWS__))
#   if (defined (WIN32))
    ULONG
        user_name_max = 255;
    static char
        user_name [256];

    if (!GetUserName (user_name, &user_name_max))
        strncpy (user_name, "unknown", sizeof (user_name));
    return (user_name);
#   else
    return ("none");
#   endif
#elif (defined (__UNIX__) || defined (__OS2__) || defined (__VMS__))
    return (getlogin ());
#endif
}
