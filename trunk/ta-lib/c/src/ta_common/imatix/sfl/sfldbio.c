/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfldbio.c
    Title:      Database interface 
    Package:    SFL

    Written:    1999/03/31  Pascal Antonnaux <pascal@imatix.com>
    Revised:    1999/11/14

    Synopsis:   Defines structures and constants for the db interface.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"
#include "sflmem.h"
#include "sflsymb.h"
#include "sflcons.h"
#include "sfllist.h"
#include "sflstr.h"
#include "sfldbio.h"

#if defined (DBIO_ODBC)
#    include "sflodbc.h"
#endif
#if defined (DBIO_ORACLE)
#    include "sflora.h"
#endif
#if defined (DBIO_DB2)
#    include "sfldb2.h"
#endif
#if defined (DBIO_MYSQL)
#    include "sflmysql.h"
#endif
#if defined (DBIO_XML)
#    include "sflxmlf.h"
#endif

/*- Definition --------------------------------------------------------------*/

#define DB_TYPE_NAME_NONE      "none"
#define DB_TYPE_NAME_ODBC      "odbc"
#define DB_TYPE_NAME_ORACLE    "ora"
#define DB_TYPE_NAME_DB2       "db2"
#define DB_TYPE_NAME_INFORMIX  "infor"
#define DB_TYPE_NAME_SYBASE    "syb"
#define DB_TYPE_NAME_SQLSERV   "mssql"
#define DB_TYPE_NAME_MYSQL     "mysql"
#define DB_TYPE_NAME_POSTGRESS "post"
#define DB_TYPE_NAME_XML       "xml"

/*- Structure ---------------------------------------------------------------*/

typedef struct {
  COMMON_DBIO_CTX
} DBIO_CTX;

typedef struct _dbio_table_handle {
  struct _dbio_table_handle
       *next, *prev;                    /* Linked list pointer               */
  void *handle;                         /* Pointer to table handle           */
} DBIO_TABLE_HANDLE;

/*- Global variables --------------------------------------------------------*/

DBIO_ERR
     dbio_error;                        /* Global DB error structure         */
long
    current_connect_id [DB_TYPE_MAX];   /* Current connection ID             */
static long
    default_connect_id [DB_TYPE_MAX],   /* Default connection ID             */
    max_connect_id     [DB_TYPE_MAX];   /* Maximum ID value                  */
static Bool
    initialise = FALSE;                 /* Indicate if global variable
                                           initialised                       */
static void *
    default_connect_ctx [DB_TYPE_MAX];  /* Default connection context        */

static SYMTAB
    *connect_cache = NULL;              /* Cache of connection handle        */
LIST
    table_cache;                        /* Cache of table connect handle     */
#if !(defined (DBIO_NONE))
static char
    buffer [LINE_MAX + 1];              /* Working buffer                    */

static char *db_type_name [] = {
   DB_TYPE_NAME_NONE,
   DB_TYPE_NAME_ODBC,
   DB_TYPE_NAME_ORACLE,
   DB_TYPE_NAME_DB2,
   DB_TYPE_NAME_INFORMIX,
   DB_TYPE_NAME_SYBASE,
   DB_TYPE_NAME_SQLSERV,
   DB_TYPE_NAME_MYSQL,
   DB_TYPE_NAME_POSTGRESS,
   DB_TYPE_NAME_XML
   };
#endif


/*- Local function declaration ----------------------------------------------*/

#if !defined (DBIO_NONE)
static Bool  add_connect_context (void *context, char *db_name, dbyte db_type,
                                   Bool def);
static void *get_connect_context  (char *db_name, dbyte db_type);
static Bool  add_table_context    (void *context);
static void  free_table_context   (long connect_id);
#endif
static void  initialise_global    (void);

/*  -------------------------------------------------------------------------
    Function: initialise_global
    Synopsis: Initialise global variable.
    -------------------------------------------------------------------------*/
void
initialise_global (void)
{
    memset (current_connect_id,  0, sizeof (current_connect_id));
    memset (max_connect_id,      0, sizeof (max_connect_id));
    memset (default_connect_id,  0, sizeof (default_connect_id));
    memset (default_connect_ctx, 0, sizeof (default_connect_ctx));
    list_reset (&table_cache);

    initialise = TRUE;
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_connect

    Synopsis: Connect to a database server and add the connection handle in
    Connection cache. To use the connection handle, use dbio_get_handle.

    Extra parameters is used if a database serveur need extra information 
    (ex: DB2 need a COLLECTION value for this PACKEGESET)

    Return TRUE if connection is made.
    ---------------------------------------------------------------------[>]-*/

Bool
dbio_connect (char *db_name, char *user, char *pwd, char *extra,
              Bool set_default, dbyte db_type)
{
    Bool
        feedback = FALSE;
#if !(defined (DBIO_NONE))
    void
        *db_context;
#endif

    if (initialise == FALSE)
        initialise_global ();

#if !(defined (DBIO_NONE))
    /* Check if database is already connected                                */
    if (get_connect_context (db_name, db_type) != NULL)
        return (TRUE);
#endif
    switch (db_type)
      {
        case DB_TYPE_NONE:
            break;
        case DB_TYPE_ODBC:
#if defined (DBIO_ODBC)
            db_context = dbio_odbc_connect (db_name, user, pwd);
            if (db_context)
              {
                add_connect_context (db_context, db_name, DB_TYPE_ODBC,
                                      set_default);
                feedback = TRUE;
              }
#endif
            break;
        case DB_TYPE_ORACLE:
#if defined (DBIO_ORACLE)
            db_context = dbio_ora_connect (db_name, user, pwd);
            if (db_context)
              {
                add_connect_context (db_context, db_name, DB_TYPE_ORACLE,
                                      set_default);
                feedback = TRUE;
              }
#endif
            break;
        case DB_TYPE_DB2:
#if defined (DBIO_DB2)
            db_context = dbio_db2_connect (db_name, user, pwd, extra);
            if (db_context)
              {
                add_connect_context (db_context, db_name, DB_TYPE_DB2,
                                      set_default);
                feedback = TRUE;
              }
#endif
            break;
        case DB_TYPE_INFORMIX:
            break;
        case DB_TYPE_SYBASE:
            break;
        case DB_TYPE_SQLSERV:
            break;
        case DB_TYPE_MYSQL:
#if defined (DBIO_MYSQL)
            db_context = dbio_mysql_connect (db_name, user, pwd, extra);
            if (db_context)
              {
                add_connect_context (db_context, db_name, DB_TYPE_MYSQL,
                                      set_default);
                feedback = TRUE;
              }
#endif
            break;
        case DB_TYPE_POSTGRES:
            break;
        case DB_TYPE_XML:
#if defined (DBIO_XML)
#endif
            break;
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_get_handle

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void *
dbio_get_handle (dbyte db_type, char *table_name, char *connect_name,
                 long connect_id)
{
#if !defined (DBIO_NONE)
    void
        *connect_ctx = NULL;            /* Connection context structure      */
#endif
    void
        *handle      = NULL;

    switch (db_type)
      {
#if defined (DBIO_ODBC)
        case DB_TYPE_ODBC:
            if (connect_id != current_connect_id [db_type])
              {
                connect_ctx = get_connect_context (connect_name, db_type);
                if (connect_ctx)
                  {
                    free_table_context (current_connect_id [db_type]);
                    set_odbc_connection (connect_ctx);
                    current_connect_id [db_type] = 
                                      ((DBIO_CTX *)connect_ctx)-> connect_id;
                  }
              }
            handle = (void *)alloc_odbc_handle (table_name, connect_ctx);
            if (handle)
                add_table_context (handle);
            break;
#endif
#if defined (DBIO_ORACLE)
#endif
#if defined (DBIO_DB2)
        case DB_TYPE_DB2:
            if (connect_id != current_connect_id [db_type])
              {
                connect_ctx = get_connect_context (connect_name, db_type);
                if (connect_ctx)
                  {
                    free_table_context (current_connect_id [db_type]);
                    dbio_db2_disconnect (NULL);
                    set_db2_connection (connect_ctx);
                    current_connect_id [db_type] = 
                                      ((DBIO_CTX *)connect_ctx)-> connect_id;
                  }
              }
            handle = (void *)alloc_db2_handle (table_name);
            if (handle)
                add_table_context (handle);
            break;
#endif
#if defined (DBIO_INFORMIX)
#endif
#if defined (DBIO_SYBASE)
#endif
#if defined (DBIO_SQLSERV)
#endif
#if defined (DBIO_MYSQL)
        case DB_TYPE_MYSQL:
            if (connect_id != current_connect_id [db_type])
              {
                connect_ctx = get_connect_context (connect_name, db_type);
                if (connect_ctx)
                  {
                    free_table_context (current_connect_id [db_type]);
                    set_mysql_connection (connect_ctx);
                    current_connect_id [db_type] = 
                                      ((DBIO_CTX *)connect_ctx)-> connect_id;
                  }
              }
            handle = (void *)alloc_mysql_handle (table_name, connect_ctx);
            if (handle)
                add_table_context (handle);
            break;
#endif
#if defined (DBIO_POSTGRESS)
#endif
#if defined (DBIO_XML)
#endif
      }
    return (handle);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_disconnect

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void dbio_disconnect (void)
{
    SYMBOL
         *val;

    if (connect_cache)
      {
#if !(defined (DBIO_NONE))
        free_table_context (0);
#endif
        for (val = connect_cache-> symbols; val; val = val-> next)
          {
#if defined (DBIO_NONE)
            if (streq (val-> value, DB_TYPE_NAME_NONE))
              {
              }
#else
            if (FALSE) {}
#endif
#if defined (DBIO_ODBC)
            else
            if (streq (val-> value, DB_TYPE_NAME_ODBC))
                dbio_odbc_disconnect ((void *)val-> data);
#endif
#if defined (DBIO_ORACLE)
            else
            if (streq (val-> value, DB_TYPE_NAME_ORACLE))
              {
              }
#endif
#if defined (DBIO_DB2)
            else
            if (streq (val-> value, DB_TYPE_NAME_DB2))
                dbio_db2_disconnect ((void *)val-> data);
#endif
#if defined (DBIO_INFORMIX)
            else
            if (streq (val-> value, DB_TYPE_NAME_INFORMIX))
              {
              }
#endif
#if defined (DBIO_SYBASE)
            else
            if (streq (val-> value, DB_TYPE_NAME_SYBASE))
              {
              }
#endif
#if defined (DBIO_SQLSERV)
            else
            if (streq (val-> value, DB_TYPE_NAME_SQLSERV))
              {
              }
#endif
#if defined (DBIO_MYSQL)
            else
            if (streq (val-> value, DB_TYPE_NAME_MYSQL))
                dbio_mysql_disconnect ((void *)val-> data);
#endif
#if defined (DBIO_POSTGRESS)
            else
            if (streq (val-> value, DB_TYPE_NAME_POSTGRESS))
              {
              }
#endif
#if defined (DBIO_XML)
            else
            if (streq (val-> value, DB_TYPE_NAME_XML))
              {
              }
#endif
          }
        sym_delete_table (connect_cache);    
      }
}
  

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_commit

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
dbio_commit (void)
{
    SYMBOL
         *val;
    int
       feedback = TRUE;
    if (connect_cache)
      {
        for (val = connect_cache-> symbols; val; val = val-> next)
          {
#if defined (DBIO_NONE)
            if (streq (val-> value, DB_TYPE_NAME_NONE))
              {
              }
#else
            if (FALSE) {}
#endif
#if defined (DBIO_ODBC)
            else
            if (streq (val-> value, DB_TYPE_NAME_ODBC))
                dbio_odbc_commit (val-> data);
#endif
#if defined (DBIO_ORACLE)
            else
            if (streq (val-> value, DB_TYPE_NAME_ORACLE))
              {
              }
#endif
#if defined (DBIO_DB2)
            else
            if (streq (val-> value, DB_TYPE_NAME_DB2))
                dbio_db2_commit ();
#endif
#if defined (DBIO_INFORMIX)
            else
            if (streq (val-> value, DB_TYPE_NAME_INFORMIX))
              {
              }
#endif
#if defined (DBIO_SYBASE)
            else
            if (streq (val-> value, DB_TYPE_NAME_SYBASE))
              {
              }
#endif
#if defined (DBIO_SQLSERV)
            else
            if (streq (val-> value, DB_TYPE_NAME_SQLSERV))
              {
              }
#endif
#if defined (DBIO_MYSQL)
            else
            if (streq (val-> value, DB_TYPE_NAME_MYSQL))
                dbio_mysql_commit (val-> data);
#endif
#if defined (DBIO_POSTGRESS)
            else
            if (streq (val-> value, DB_TYPE_NAME_POSTGRESS))
              {
              }
#endif
#if defined (DBIO_XML)
            else
            if (streq (val-> value, DB_TYPE_NAME_XML))
              {
              }
#endif
          }
      }

#if !defined (DBIO_NONE)
    free_table_context (0);
#endif
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_rollback

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
dbio_rollback (void)
{
    SYMBOL
        *val;
    int
        feedback = TRUE;
    if (connect_cache)
      {
        for (val = connect_cache-> symbols; val; val = val-> next)
          {
#if defined (DBIO_NONE)
            if (streq (val-> value, DB_TYPE_NAME_NONE))
              {
              }
#else
            if (FALSE) {}
#endif
#if defined (DBIO_ODBC)
            else
            if (streq (val-> value, DB_TYPE_NAME_ODBC))
                dbio_odbc_rollback (val-> data);
#endif
#if defined (DBIO_ORACLE)
            else
            if (streq (val-> value, DB_TYPE_NAME_ORACLE))
              {
              }
#endif
#if defined (DBIO_DB2)
            else
            if (streq (val-> value, DB_TYPE_NAME_DB2))
                dbio_db2_rollback ();
#endif
#if defined (DBIO_INFORMIX)
            else
            if (streq (val-> value, DB_TYPE_NAME_INFORMIX))
              {
              }
#endif
#if defined (DBIO_SYBASE)
            else
            if (streq (val-> value, DB_TYPE_NAME_SYBASE))
              {
              }
#endif
#if defined (DBIO_SQLSERV)
            else
            if (streq (val-> value, DB_TYPE_NAME_SQLSERV))
              {
              }
#endif
#if defined (DBIO_MYSQL)
            else
            if (streq (val-> value, DB_TYPE_NAME_MYSQL))
                dbio_mysql_rollback (val-> data);
#endif
#if defined (DBIO_POSTGRESS)
            else
            if (streq (val-> value, DB_TYPE_NAME_POSTGRESS))
              {
              }
#endif
#if defined (DBIO_XML)
            else
            if (streq (val-> value, DB_TYPE_NAME_XML))
              {
              }
#endif
          }
      }
#if !defined (DBIO_NONE)
    free_table_context (0);
#endif
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_get_error

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

DBIO_ERR *
dbio_get_error (void)
{
    return (&dbio_error);
}


/*  ---------------------------------------------------------------------[<]-
    Function: dbio_error_code

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

int
dbio_error_code (void)
{
    return (dbio_error.code);
}

/*  ---------------------------------------------------------------------[<]-
    Function: dbio_error_message

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

char *
dbio_error_message (void)
{
    return (dbio_error.message);
}


#if !defined (DBIO_NONE)
/*  -------------------------------------------------------------------------
    Function: add_connect_context

    Synopsis: 
    -------------------------------------------------------------------------*/

static Bool
add_connect_context (void *context, char *db_name, dbyte db_type, Bool def)
{
    Bool
        feedback = FALSE;               /* Feedback value                    */
    SYMBOL
        *cache_value;                   /* Cache value                       */
    DBIO_CTX
        *ctx;
    /* Allocate connect cache if empty                                       */
    if (connect_cache == NULL)
      {
        connect_cache = sym_create_table ();
        if (connect_cache == NULL)
            return (FALSE);
      }

    /* Add connection context to cache                                       */
    sprintf (buffer, "%s_%s", db_type_name [db_type], db_name);
    cache_value = sym_assume_symbol (connect_cache, buffer,
                                     db_type_name [db_type]);
    if (cache_value)
      {
        ctx = (DBIO_CTX *)context;
        ctx-> connect_id = ++max_connect_id [db_type];
        cache_value-> data = context;
        current_connect_id [db_type] = ctx-> connect_id;
        if (def == TRUE)
          {
            if (default_connect_id [db_type] != 0)
                coprintf ("Warning: Multiple default connection for %s\n",
                           db_type_name [db_type]);
            default_connect_id  [db_type] = ctx-> connect_id;
            default_connect_ctx [db_type] = context;
          }
        feedback = TRUE;
      }
    
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: get_connect_context

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

static void *
get_connect_context  (char *db_name, dbyte db_type)
{
    void
        *context = NULL;                /* Database context structure        */
    SYMBOL
        *cache_value;                   /* Cache value                       */

    if (connect_cache == NULL)
        return (NULL);

    if (db_name == NULL || strnull (db_name))
        context = default_connect_ctx [db_type];
    else
      {
        sprintf (buffer, "%s_%s", db_type_name [db_type], db_name);
        cache_value = sym_lookup_symbol (connect_cache, buffer);
        if (cache_value)
            context = (void *)cache_value-> data;
      }
    return (context);
}

static Bool
add_table_context (void *context)
{
    DBIO_TABLE_HANDLE
        *handle = NULL;
    Bool
        feedback = FALSE;

    list_create (handle, sizeof (DBIO_TABLE_HANDLE));
    if (handle)
      {
        handle-> handle = context;
        list_relink_before (&table_cache, handle);
        feedback = TRUE;
      }
    return (feedback);
}

static void
free_table_context (long connect_id)
{
    DBIO_TABLE_HANDLE
        *next   = NULL,
        *handle = NULL;
    DBIO_CTX
        *ctx    = NULL;    

    for (handle = table_cache.next; (void *)handle != &table_cache; )
      {
        ctx = (DBIO_CTX *)handle-> handle;
        if ((connect_id == 0
        ||  connect_id == ctx-> connect_id)
        &&  ctx-> free_handle != NULL)
          {
            ctx-> free_handle (ctx);
            next = handle-> next;
            list_unlink (handle);
            mem_free (handle-> handle);
            mem_free (handle);
            handle = next;
          }
        else
            handle = handle-> next;
      }
}
#endif


dbyte
get_db_type (char *db_type_name)
{
    dbyte
        type = DB_TYPE_NONE;

    if (db_type_name == NULL)
        return (type);
     
    if (lexcmp (db_type_name, DB_TYPE_NAME_ODBC) == 0)
        type = DB_TYPE_ODBC;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_ORACLE) == 0)
        type = DB_TYPE_ORACLE;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_DB2) == 0)
        type = DB_TYPE_DB2;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_INFORMIX) == 0)
        type = DB_TYPE_INFORMIX;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_SYBASE) == 0)
        type = DB_TYPE_SYBASE;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_SQLSERV) == 0)
        type = DB_TYPE_SQLSERV;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_MYSQL) == 0)
        type = DB_TYPE_MYSQL;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_POSTGRESS) == 0)
        type = DB_TYPE_POSTGRES;
    else
    if (lexcmp (db_type_name, DB_TYPE_NAME_XML) == 0)
        type = DB_TYPE_XML;
    return (type);
}
