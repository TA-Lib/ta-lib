/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmysql.c
    Title:      mysql Database interface
    Package:    SFL

    Written:    1999/09/09  Pascal Antonnaux <pascal@imatix.com>
    Revised:    1999/11/14

    Synopsis:   Defines structures and constants for the mysql interface.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/
#include "prelude.h"
#include "sflsymb.h"
#include "sflcons.h"
#include "sflmem.h"
#include "sfldbio.h"

#ifdef DBIO_MYSQL
#include "sflmysql.h"

/*- Global definition -------------------------------------------------------*/

typedef struct {
    COMMON_DBIO_CTX
    char  dbname   [50];
    char  user     [50];
    char  password [50]; 
    char  host     [100];
    long  port;
    MYSQL *db_handle;
} MYSQL_CONNECT_CTX;


/*  ---------------------------------------------------------------------[<]-
    Function: dbio_mysql_connect

    Synopsis: Connect to a mysql database.
    ---------------------------------------------------------------------[>]-*/

void *
dbio_mysql_connect (char *name, char *user, char *pwd, char *host)
{
    MYSQL_CONNECT_CTX
        *handle = NULL;
    MYSQL
        *mysql;


    mysql = mysql_connect (NULL, host, user, pwd);
    if (mysql)
      {
        if (mysql_select_db (mysql, name) != 0)
          {
            mysql_close (mysql);
            mysql = NULL;
          }
      }
    if (mysql)
      {
        handle = mem_alloc (sizeof (MYSQL_CONNECT_CTX));
        memset (handle, 0,  sizeof (MYSQL_CONNECT_CTX));
        handle-> db_handle = mysql;
        handle-> port      = MYSQL_PORT;
        strcpy (handle-> dbname, name);
        if (user && strused (user))
            strcpy (handle-> user,     user);
        if (pwd  && strused (pwd))
            strcpy (handle-> password, pwd);
        if (host && strused (host))
            strcpy (handle-> host,     host);
      }
    else
        coprintf ("Error on Connect to %s in MySql", name);
    
    return ((void *)handle);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_mysql_disconnect

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
dbio_mysql_disconnect (void *context)
{
    MYSQL_CONNECT_CTX
        *ctx;
    if (context)
      {
        ctx = (MYSQL_CONNECT_CTX *)context;
        if (ctx-> db_handle)
          {
            mysql_close (ctx-> db_handle);
            ctx-> db_handle = NULL;
          }
        mem_free (ctx);
      }
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_mysql_commit

    Synopsis: Mysql don't support COMMIT/ROLLBACK
    ---------------------------------------------------------------------[>]-*/

int
dbio_mysql_commit (void *context)
{
    int
        feedback = TRUE;

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_mysql_rollback

    Synopsis: Mysql don't support COMMIT/ROLLBACK
    ---------------------------------------------------------------------[>]-*/

int
dbio_mysql_rollback (void *context)
{
    int
        feedback = TRUE;

    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: alloc_mysql_handle

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

MYSQLHANDLE *
alloc_mysql_handle    (char *table_name, void *context)
{
    MYSQLHANDLE
        *handle = NULL;                 /* mysql Context handle                */
    MYSQL_CONNECT_CTX
        *temp_ctx,
        *ctx;
    if (context)
      {
        ctx = (MYSQL_CONNECT_CTX *)context;
        temp_ctx = dbio_mysql_connect (ctx-> dbname, ctx-> user, ctx->password,
                                       ctx->host);
        if (temp_ctx)
          {
            handle = mem_alloc (sizeof (MYSQLHANDLE));
            if (handle)
              {
                memset (handle, 0, sizeof (MYSQLHANDLE));
                handle-> db_handle = temp_ctx-> db_handle;
                if (table_name && strused (table_name))
                    handle-> table_name = mem_strdup (table_name);
                mem_free (temp_ctx);
              }
            else
              {
                coprintf ("DBIO MYSQL: Error in alloc_mysql_handle (Memory Allocation)");
                dbio_mysql_disconnect (temp_ctx);
              }
          }
      }
    return (handle);
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_mysql_connection

    Synopsis: Reconnect to a mysql database.
    ---------------------------------------------------------------------[>]-*/

void
set_mysql_connection  (void *context)
{
}

#endif
