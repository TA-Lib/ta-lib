/* TA-LIB MySQL Data Source Driver Copyright (c) 2003, Pawel A. Konieczny
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  PK       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  101703 PK   First version.
 *
 */

/* Description:
 *    This is the entry points of the data source driver for MySQL database.
 *    It depend on the mysql++ library
 *
 *    It provides ALL the functions needed by the "TA_DataSourceDriver"
 *    structure (see ta_source.h).
 */

/**** Headers ****/
extern "C" {
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ta_source.h"
#include "ta_mysql.h"
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_list.h"
#include "ta_data.h"
#include "ta_system.h"
#include "ta_fileindex.h"
#include "ta_global.h"
}
#include "ta_mysql_handle.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
TA_RetCode TA_MYSQL_ParseLocation(const char location[],       
                                  char host[],
                                  unsigned int *port,
                                  char database[]);

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_MYSQL_InitializeSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN(  TA_MYSQL_InitializeSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_ShutdownSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN(  TA_MYSQL_ShutdownSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetParameters( TA_DataSourceParameters *param )
{
   TA_PROLOG

   TA_TRACE_BEGIN( TA_MYSQL_GetParameters );

   memset( param, 0, sizeof( TA_DataSourceParameters ) );

   // No additional parameters supported yet. TODO
   param->flags = TA_NO_FLAGS;

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_MYSQL_OpenSource( const TA_AddDataSourceParamPriv *param,
                                TA_DataSourceHandle **handle )
{
   TA_PROLOG
   TA_DataSourceHandle *tmpHandle;
   TA_PrivateMySQLHandle *privData;
   char *host, *dbase;
   unsigned port;
   TA_RetCode retCode;

   *handle = NULL;

   TA_TRACE_BEGIN( TA_MYSQL_OpenSource );

   /* 'info' and 'location' are mandatory. */
   if( (!param->info) || (!param->location) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   /* get parameters for MySQL connection */
   host = (char*)TA_Malloc(strlen(TA_StringToChar(param->location)));
   if (host == NULL)
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   dbase = (char*)TA_Malloc(strlen(TA_StringToChar(param->location)));
   if (dbase == NULL)
   {
      TA_Free(host);
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   retCode = TA_MYSQL_ParseLocation(TA_StringToChar(param->location), host, &port, dbase);
   if( retCode != TA_SUCCESS )
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_TRACE_RETURN( retCode );
   }

   /* Allocate and initialize the handle. This function will also allocate the
    * private handle (opaque data).
    */
   tmpHandle = TA_MYSQL_DataSourceHandleAlloc(param);

   if( tmpHandle == NULL )
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   privData = (TA_PrivateMySQLHandle *)(tmpHandle->opaqueData);

   /* Establish the connection with the MySQL server */
   try 
   {
      privData->con = new Connection(
         dbase, 
         host, 
         TA_StringToChar(param->username), 
         TA_StringToChar(param->password), 
         port);
   }
   catch (...)
   {
      TA_Free(host);
      TA_Free(dbase);
      TA_MYSQL_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( TA_ACCESS_FAILED );
   }
   TA_Free(host);
   TA_Free(dbase);

   /* Now build the symbols index. */
   retCode = TA_MYSQL_BuildSymbolsIndex( tmpHandle );

   if( retCode != TA_SUCCESS )
   {
      TA_MYSQL_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Set the total number of distinct categories. */
   tmpHandle->nbCategory = TA_ListSize(privData->theCategoryIndex);

   *handle = tmpHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_CloseSource( TA_DataSourceHandle *handle )
{
   TA_PROLOG

   TA_TRACE_BEGIN(  TA_MYSQL_CloseSource );

   /* Free all ressource used by this handle. */
   if( handle )
      TA_MYSQL_DataSourceHandleFree( handle );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privData;
   TA_List               *categoryIndex;
   TA_MySQLCategoryNode  *categoryNode;

   TA_TRACE_BEGIN(  TA_MYSQL_GetFirstCategoryHandle );

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateMySQLHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(49) );
   }

   categoryIndex = privData->theCategoryIndex;

   if( !categoryIndex )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(50) );
   }

   categoryNode = (TA_MySQLCategoryNode*)TA_ListAccessHead(categoryIndex);
   if( !categoryNode || !categoryNode->category )
   {
      TA_TRACE_RETURN( (TA_RetCode)TA_INTERNAL_ERROR(51) ); /* At least one category must exist. */
   }

   /* Set the categoryHandle. */
   categoryHandle->string = categoryNode->category;
   categoryHandle->nbSymbol = 0;
   categoryHandle->opaqueData = categoryHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetNextCategoryHandle( TA_DataSourceHandle *handle,
                                           TA_CategoryHandle   *categoryHandle,
                                           unsigned int index )
{
   TA_PROLOG

   //TA_PrivateAsciiHandle *privData;
   //TA_FileIndex     *fileIndex;
   //TA_String        *string;

   TA_TRACE_BEGIN(  TA_MYSQL_GetNextCategoryHandle );

#if 0
   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(52) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(53) );
   }

   /* Get the next category from the fileIdnex. */
   string = TA_FileIndexNextCategory( fileIndex );

   if( !string )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the categoryHandle. */
   categoryHandle->string = string;
   categoryHandle->nbSymbol = TA_FileIndexNbSymbol( fileIndex );
   categoryHandle->opaqueData = NULL; /* Not needed... */

   TA_TRACE_RETURN( TA_SUCCESS );
#endif
   TA_TRACE_RETURN( TA_END_OF_INDEX );
}

TA_RetCode TA_MYSQL_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle   *categoryHandle,
                                          TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG
   //TA_RetCode retCode;
   //TA_PrivateAsciiHandle *privData;
   //TA_FileIndex *fileIndex;
   //TA_FileInfo *sourceInfo;

   TA_TRACE_BEGIN(  TA_MYSQL_GetFirstSymbolHandle );

#if 0
   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(54) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex || !categoryHandle->string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(55) );
   }

   /* Make sure the current category is the one requested. */
   retCode = TA_FileIndexSelectCategory( fileIndex, categoryHandle->string );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(56) );
   }

   /* Get the first symbol in this category. */
   sourceInfo = TA_FileIndexFirstSymbol( fileIndex );

   if( !sourceInfo )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Parano sanity check: the string of the requested categoryHandle should
    * correspond to the string of this sourceInfo.
    */
   if( strcmp( TA_StringToChar( TA_FileInfoCategory( sourceInfo ) ),
               TA_StringToChar( categoryHandle->string ) ) != 0 )
   {
      TA_FATAL(  NULL, 0, 0 );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = TA_FileInfoSymbol( sourceInfo );
   symbolHandle->opaqueData = sourceInfo;

#endif
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                         TA_CategoryHandle   *categoryHandle,
                                         TA_SymbolHandle     *symbolHandle,
                                         unsigned int index )
{
   TA_PROLOG
   //TA_RetCode retCode;
   //TA_PrivateAsciiHandle *privData;
   //TA_FileIndex *fileIndex;
   //TA_FileInfo *sourceInfo;

   TA_TRACE_BEGIN(  TA_MYSQL_GetNextSymbolHandle );

#if 0
   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(57) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex || !categoryHandle->string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(58) );
   }

   /* Make sure the current category is the one requested. */
   retCode = TA_FileIndexSelectCategory( fileIndex, categoryHandle->string );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(59) );
   }

   /* Get the next symbol in this category. */
   sourceInfo = TA_FileIndexNextSymbol( fileIndex );

   if( !sourceInfo )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Parano sanity check: the string of the requested categoryHandle should
    * correspond to the string of this sourceInfo.
    */
   if( strcmp( TA_StringToChar( TA_FileInfoCategory( sourceInfo ) ),
               TA_StringToChar( categoryHandle->string ) ) != 0 )
   {
      TA_FATAL(  NULL, 0, 0 );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = TA_FileInfoSymbol( sourceInfo );
   symbolHandle->opaqueData = sourceInfo;

#endif
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_GetHistoryData( TA_DataSourceHandle *handle,
                                    TA_CategoryHandle   *categoryHandle,
                                    TA_SymbolHandle     *symbolHandle,
                                    TA_Period            period,
                                    const TA_Timestamp  *start,
                                    const TA_Timestamp  *end,
                                    TA_Field             fieldToAlloc,
                                    TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG
   //const char *path;
   //TA_FileInfo *sourceInfo;
   //TA_FileHandle *fileHandle;
   //TA_RetCode retCode;
   //TA_PrivateAsciiHandle *privateHandle;

   TA_TRACE_BEGIN(  TA_MYSQL_GetHistoryData );

#if 0
   TA_ASSERT( handle != NULL );

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   TA_ASSERT( privateHandle != NULL );

   TA_ASSERT( paramForAddData != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   sourceInfo = (TA_FileInfo *)symbolHandle->opaqueData;

   TA_ASSERT( sourceInfo != NULL );

   /* If the requested period is too precise for the
    * period that can be provided by this data source,
    * simply return without error.
    * Since no data has been added, the TA-LIB will ignore
    * this data source.
    */
   if( period < privateHandle->readOpInfo->period )
   {
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* If all fields are requested, provides all the fields
    * that were specified when this data source was added.
    */
   if( fieldToAlloc == TA_ALL )
      fieldToAlloc = privateHandle->readOpInfo->fieldProvided;

   /* When the TA_REPLACE_ZERO_PRICE_BAR flag is set, we must
    * always process the close.
    */
   if( !(fieldToAlloc & TA_CLOSE) &&
        (privateHandle->param->flags & TA_REPLACE_ZERO_PRICE_BAR) )
   {   
      fieldToAlloc |= TA_CLOSE;
   }

   /* Get the path of the file. */
   path = TA_FileInfoPath( sourceInfo );

   if( !path )
   {
      TA_FATAL(  "Building path failed", 0, 0 );
   }

   /* Open the file for sequential read only (optimized read) */
   retCode = TA_FileSeqOpen( path, &fileHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Optimize the readOp according to the requested field. */
   retCode = TA_ReadOp_Optimize( privateHandle->readOpInfo,
                                 privateHandle->readOpInfo->period,
                                 fieldToAlloc );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Read all the price bar. */
   retCode = TA_ReadOp_Do( fileHandle,                           
                           privateHandle->readOpInfo,
                           privateHandle->readOpInfo->period,
                           start, end, 200,
                           fieldToAlloc, paramForAddData, NULL );

   if( retCode != TA_SUCCESS )
   {
      TA_FileSeqClose( fileHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Read completed... clean-up and return. */
   retCode = TA_FileSeqClose( fileHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

#endif
   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/

/* Extract host, port, and database name from location URL 
 * IN: location string
 * OUT: host (array must be preallocated to the size at least the length of location)
 * OUT: port (0 when not specified)
 * OUT: database (array must be preallocated to the size at least the length of location)
 */
TA_RetCode TA_MYSQL_ParseLocation(const char location[],       
                                  char host[],
                                  unsigned int *port,
                                  char database[])
{
   TA_PROLOG
   const char *b;

   TA_TRACE_BEGIN( TA_MYSQL_ParseLocation );

   /* check whether location starts with mysql: scheme */ 
   if (strncmp(location, "mysql:/", 7) != 0)
   {
      TA_TRACE_RETURN( TA_INVALID_PATH );
   }

   b = &location[7];  /* begin of position to parse */
   host[0] = '\0';
   if (*b == '/')
   {
      /* host name specified */
      const char *e = strchr(b+1, '/');  /* end of host part */
      const char *p = strchr(b+1, ':');  /* position of port number */

      if (!e)  /* no database name */
      {
         TA_TRACE_RETURN( TA_INVALID_PATH );
      }

      if (p && p < e)  /* port specified */
      {
         *port = (unsigned)atoi(p+1);
         e = p;
      }
      else
      {
         *port = 0;
      }
      b++;
      memcpy(host, b, e - b);
      host[e - b] = '\0';
      b = strchr(b, '/') + 1;
   }
   strcpy(database, b);

   TA_TRACE_RETURN( TA_SUCCESS );
}