/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmysql.h
    Title:      MYSQL Database interface - header file
    Package:    SFL

    Written:    1999/09/09  Pascal Antonnaux <pascal@imatix.com>
    Revised:    2000/02/11  Jonathan Schultz

    Synopsis:   Defines structures and constants for the mysql interface.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#if defined (DBIO_MYSQL)

#ifndef _SFLMYSQL_INCLUDED
#define _SFLMYSQL_INCLUDED

#include "mysql.h"                      /*  Main MySQL header file           */
#include "mysqld_error.h"               /*  Error numbers                    */


typedef struct {
    COMMON_DBIO_CTX
    MYSQL     *db_handle;
    MYSQL_RES *result ;
    char  *table_name;                  /*  Table name                       */
} MYSQLHANDLE;

#ifdef __cplusplus
extern "C" {
#endif

void      *dbio_mysql_connect    (char *db_name, char *user, char *pwd,
                                  char *host);
void       dbio_mysql_disconnect (void *context);
int        dbio_mysql_commit     (void *context);
int        dbio_mysql_rollback   (void *context);
MYSQLHANDLE *alloc_mysql_handle  (char *table_name, void *context);
void       free_mysql_handle     (MYSQLHANDLE *handle);
void       set_mysql_connection  (void *context);

#ifdef __cplusplus
}
#endif

#endif
#endif
