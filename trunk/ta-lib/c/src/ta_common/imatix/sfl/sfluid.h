/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfluid.h
    Title:      Process user id (uid) and group id (gid) functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/03  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/09/04

    Synopsis:   Provides functions to access user and group id names and
                manage the current real/effective uid's and gid's for a
                process.  These functions are only meaningful on UNIX
                systems, and partially on VMS systems, but may be used by
                portable programs that must operate under UNIX as well as
                other environments.  Some uid functions are non-portable
                between UNIX systems; this package provides a single API.
                Changes for OS/2 were done by Ewen McNeill <ewen@naos.co.nz>.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLUID_INCLUDED                /*  Allow multiple inclusions        */
#define SFLUID_INCLUDED

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

char *get_uid_name  (uid_t uid);
char *get_gid_name  (gid_t gid);
int   set_uid_user  (void);
int   set_uid_root  (void);
int   set_gid_user  (void);
int   set_gid_root  (void);
int   set_uid_gid   (char *new_uid, char *new_gid);
char *get_login     (void);


#ifdef __cplusplus
}
#endif

#endif
