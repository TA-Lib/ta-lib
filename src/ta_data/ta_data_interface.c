/* TA-LIB Copyright (c) 1999-2005, Mario Fortier
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
 *  MF       Mario Fortier
 *  PK       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  110199 MF      First version.
 *  011204 MF      Now use addDataSourceParamPriv and check for
 *                 new SPLIT/VALUE adjust flags.
 *  013104 MF      Add possibility to "name" data sources + add 
 *                 validation of start/end parameters.
 *  032004 MF      TA_END_OF_INDEX now use only to exit prematurely
 *                 while building the index. In normal operation, the 
 *                 data source driver should not have to return it.
 *  032704 MF      TA_AddDataSource location param will adapt to
 *                 platform when the driver set TA_LOCATION_IS_PATH.
 *  062105 PK      Added (partial) support for end-of-period
 *                 timestamp logic.
 *  070305 MF,PK   Fix #1229243 memory leak in some failure scenario.
 */

/* Decription:
 *    Provides all the user entry point for the "unified database".
 */

/**** Headers ****/
#include <string.h>
#include "ta_data.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_data_udb.h"
#include "ta_global.h"
#include "ta_magic_nb.h"
#include "ta_history.h"
#include "ta_adddatasourceparam_priv.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

/* The hidden data put in a TA_CategoryTable. */
typedef struct
{
   unsigned int magicNb;
   TA_UDBasePriv *privUDB;
} TA_CategoryTableHiddenData;

/* The hidden data put in a TA_SymbolTable. */
typedef struct
{
   unsigned int magicNb;
   TA_UDBasePriv *privUDB;
} TA_SymbolTableHiddenData;

typedef struct
{
   /* One instance of this struct exists for each added data source. */

   /* Keep a copy of the parameters who were used
    * to initialize this data source.
    */
   TA_AddDataSourceParamPriv *addDataSourceParamPriv;

   /* Initialize by the data source driver. */
   TA_DataSourceHandle *sourceHandle;

   /* This list is just a "container" for efficient allocation purpose. The
    * elements are used directly from the TA_DriverHandle->categoryHandle
    * pointer, but are owned here.
    */
   TA_List *listCategoryHandle;

   /* Allocation Strategy: This structure owns all its sub-elements. */
} TA_DataSource;

/* The following are for managing the global variables. */
static TA_RetCode TA_DataGlobalInit    ( void **globalToAlloc );
static TA_RetCode TA_DataGlobalShutdown( void *globalAllocated );

const TA_GlobalControl TA_DataGlobalControl =
{
   TA_DATA_GLOBAL_ID,
   TA_DataGlobalInit,
   TA_DataGlobalShutdown
};

typedef struct
{
  /* Only one instance of this structure will exist. */

  #if !defined( TA_SINGLE_THREAD )
  TA_Sema sema; /* To protect this structure integrity. */
  #endif

  /* Keep track of which data source has been initialized. */
  unsigned char *dataSourceInitFlag;

  /* Keep track of the allocated unified database. */
  TA_List *listUDBase; /* List of TA_UDBasePriv. */
} TA_DataGlobal;

/**** Local functions declarations.    ****/
static TA_DataSource *allocDataSourceForGlobal( TA_UDBasePriv *privUDB,
                                                TA_AddDataSourceParamPriv *addDataSourceParamPriv,
                                                TA_DataSourceHandle *sourceHandle,
                                                TA_List *listCategoryHandle );

static TA_UDB_Category *addCategory( TA_UDBasePriv *privUDB, TA_String *string );
static TA_UDB_Symbol *addSymbol( TA_UDBasePriv *privUDB,
                                 TA_Dict *dictUDBSymbol,
                                 TA_UDB_Driver *driverHandle,
                                 TA_DataSourceHandle *sourceHandle,
                                 TA_CategoryHandle *categoryHandle,
                                 TA_SymbolHandle *symbolHandle );
static TA_RetCode stringTableFree( TA_StringTable *table, unsigned int freeInternalStringOnly );

static TA_RetCode closeAllDataSource( TA_UDBasePriv *privUDB );

static TA_RetCode closeAllUDBase( TA_DataGlobal *global );
static TA_RetCode shutdownAllSourceDriver( TA_DataGlobal *global );

static void freeSymbolData( void *toBeFreed );
static void freeCategoryData( void *toBeFreed );

static TA_RetCode processCategoryAndSymbols( TA_UDBasePriv *privUDB,
                                             const TA_DataSourceDriver *driver,
                                             const TA_AddDataSourceParamPriv *addDataSourceParamPriv,
                                             TA_DataSourceHandle *sourceHandle,
                                             TA_CategoryHandle   *categoryHandle );

/* Function to alloc/free a TA_AddDataSourceParamPriv. */
static TA_AddDataSourceParamPriv *TA_AddDataSourceParamPrivAlloc( const TA_AddDataSourceParam *param, TA_SourceFlag flags );
static TA_RetCode TA_AddDataSourceParamPrivFree( TA_AddDataSourceParamPriv *toBeFreed );

/* Mechanism for easily deleting lists. */
static TA_RetCode freeListAndElement( TA_List *list,
                                      TA_RetCode (*freeFunc)( void *toBeFreed ));

static TA_RetCode freeUDBDriverArray( void *toBeFreed );
static TA_RetCode freeCategoryHandle( void *toBeFreed );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_UDBaseAlloc( TA_UDBase **newUDBase )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_RetCode finalRetCode;
   TA_DataGlobal *global;
   TA_UDBasePriv *privUDB;
   TA_StringCache *stringCache;
   TA_String *string;

   TA_TRACE_BEGIN(TA_UDBaseAlloc);

   /* Verify parameters. */
   if( !newUDBase )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   *newUDBase = NULL;

   /* Get the pointer on the global variables. */
   retCode = TA_GetGlobal(  &TA_DataGlobalControl, (void *)&global );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Alloc the hidden implementation of a TA_UDBase */
   privUDB = (TA_UDBasePriv *) TA_Malloc( sizeof( TA_UDBasePriv ));

   if( !privUDB )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   memset( privUDB, 0, sizeof( TA_UDBasePriv ) );
   privUDB->magicNb   = TA_UDBASE_MAGIC_NB;

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaInit( &privUDB->sema, 1 );
   if( retCode != TA_SUCCESS )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   /* Initialize the list of data source.
    * Each time a data source is added with TA_AddDataSource, it
    * is added to this list.
    */
   privUDB->listDataSource = TA_ListAlloc();
   if( !privUDB->listDataSource )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Initialize the dictCategory. One entry in the dictionary
    * will exist for each category.
    */
   privUDB->dictCategory = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeCategoryData );

   if( !privUDB->dictCategory )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Initialize the Default category structure. */
   stringCache = TA_GetGlobalStringCache();
   if( !stringCache )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   string = TA_StringAlloc( stringCache, TA_DEFAULT_CATEGORY );
   if( !string )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Add the default category to the dictionary. */
   privUDB->defaultCategory = addCategory( privUDB, string );
   TA_StringFree( stringCache, string );

   if( !privUDB->defaultCategory )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Add the UDBase to the global list. */
   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &global->sema );

   if( retCode != TA_SUCCESS )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   finalRetCode = TA_ListAddTail( global->listUDBase, (void *)privUDB );

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaPost( &global->sema );

   if( retCode != TA_SUCCESS )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   /* Check if an error occured within the critical section. */
   if( finalRetCode != TA_SUCCESS )
   {
      TA_UDBaseFree( (TA_UDBase *)privUDB );
      TA_TRACE_RETURN( finalRetCode );
   }

   /* Everything is fine, return the new unified database
    * to the caller.
    */
   *newUDBase = (TA_UDBase *)privUDB;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_UDBaseFree( TA_UDBase *toBeFreed )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_RetCode finalRetCode;
   TA_DataGlobal *global;
   TA_UDBasePriv *privUDB;

   /* Return immediately if apparently already freed. */
   privUDB = (TA_UDBasePriv *)toBeFreed;
   if( !privUDB )
      return TA_SUCCESS;

   /* Check integrity. */
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;

   retCode = TA_GetGlobal( &TA_DataGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   TA_TRACE_BEGIN( TA_UDBaseFree );

   /* Will change if an error happen. */
   finalRetCode = TA_SUCCESS;

   /* Remove from the global list. */
   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &global->sema );

   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   else
   {
   #endif

      retCode = TA_ListRemoveEntry( global->listUDBase, (void *)privUDB );

      if( retCode != TA_SUCCESS )
         finalRetCode = retCode;

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaPost( &global->sema );

      if( retCode != TA_SUCCESS )
         finalRetCode = retCode;
   }
   #endif

   #if !defined( TA_SINGLE_THREAD )
   /* No one else is suppose to access this data at this
    * point. But just in case... prevent the other to
    * be able to get the semaphore while we are shutting
    * down the unified database.
    */
   retCode = TA_SemaWait( &privUDB->sema );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   #endif

   retCode = closeAllDataSource( privUDB );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   
   retCode = TA_ListFree( privUDB->listDataSource );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;

   retCode = TA_DictFree( privUDB->dictCategory );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;

   #if !defined( TA_SINGLE_THREAD )
   /* Destroy the semaphore... too bad if someone was wrongly
    * waiting on it! That other thread will somehow be failed
    * or hang forever.
    */
   retCode = TA_SemaDestroy( &privUDB->sema );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   #endif

   TA_Free( privUDB );

   TA_TRACE_RETURN( finalRetCode );
}

TA_RetCode TA_AddDataSource( TA_UDBase *unifiedDatabase,
                             const TA_AddDataSourceParam *param )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_RetCode finalRetCode;
   TA_SourceId driverIndex;
   const TA_DataSourceDriver *driver;
   TA_List *tmpListCategory;
   TA_UDBasePriv *privUDB;

   TA_DataSource  *dataSource;

   TA_DataSourceHandle *sourceHandle;
   TA_CategoryHandle   *categoryHandle;

   TA_DataGlobal *global;

   TA_AddDataSourceParamPriv *addDataSourceParamPriv;

   unsigned int again; /* Boolean */

   unsigned int categoryIndex;

   TA_DataSourceParameters dataSourceParams;

   tmpListCategory = NULL;

   /* Check parameters. */
   if( unifiedDatabase == NULL )
      return TA_BAD_PARAM;

   privUDB = (TA_UDBasePriv *)unifiedDatabase;
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;

   if( param->id >= TA_NUM_OF_SOURCE_ID )
      return TA_BAD_PARAM;

   retCode = TA_GetGlobal( &TA_DataGlobalControl, (void *)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   TA_TRACE_BEGIN( TA_AddDataSource );

   /* It is assume there is a one-to-one correspondance
    * between the "TA_SourceId" and the index
    * in the TA_gDataSourceTable
    */
   driverIndex = param->id;
   driver = &TA_gDataSourceTable[driverIndex];

   /* Initialize the data source (if not already done). */
   finalRetCode = TA_SUCCESS;
   if( global->dataSourceInitFlag[driverIndex] == 0 )
   {
      #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaWait( &global->sema );

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }

      /* Check again within the critical section. */
      if( global->dataSourceInitFlag[driverIndex] == 0 )
      {
      #endif
         if( driver->initializeSourceDriver )
         {
            retCode = driver->initializeSourceDriver();
            if( retCode != TA_SUCCESS )
               finalRetCode = retCode;
         }
         global->dataSourceInitFlag[driverIndex] = 1;
      #if !defined( TA_SINGLE_THREAD )
      }

      retCode = TA_SemaPost( &global->sema );

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
      #endif
   }

   /* Check for error that could have happen in the critical section. */
   if( finalRetCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( finalRetCode );
   }

   /* Check that if a flag is set that the data source can
    * actually handle the requested functionality.
    */
   driver->getParameters( &dataSourceParams );
   if( (param->flags           & TA_REPLACE_ZERO_PRICE_BAR) && 
      !(dataSourceParams.flags & TA_REPLACE_ZERO_PRICE_BAR) )
   {
      TA_TRACE_RETURN( TA_UNSUPPORTED_REPLACE_ZERO_PRICE_BAR );
   }

   if( (param->flags           & TA_DO_NOT_SPLIT_ADJUST) && 
      !(dataSourceParams.flags & TA_DO_NOT_SPLIT_ADJUST) )
   {
      TA_TRACE_RETURN( TA_UNSUPPORTED_DO_NOT_SPLIT_ADJUST );
   }

   if( (param->flags           & TA_DO_NOT_VALUE_ADJUST) && 
      !(dataSourceParams.flags & TA_DO_NOT_VALUE_ADJUST) )
   {
      TA_TRACE_RETURN( TA_UNSUPPORTED_DO_NOT_VALUE_ADJUST );
   }

   /* Open the source. */
   
   /* Allocate a private copy of the parameters. This function
    * will also set the default values if not provided by the
    * caller. (particularly the category string).
    */
   addDataSourceParamPriv = TA_AddDataSourceParamPrivAlloc( param, dataSourceParams.flags );
   if( !addDataSourceParamPriv )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   retCode = driver->openSource( addDataSourceParamPriv, &sourceHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
      TA_TRACE_RETURN( retCode );
   }

   /* Just check the handle to make sure. */
   if( sourceHandle == NULL )
   {
      TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Add entries to the Dict of Category.
    * Also, process symbols for each category.
    */
   if( sourceHandle->nbCategory != 0 )
   {
      /* Make sure the needed functions are provided by the driver. */
      if( !driver->getFirstCategoryHandle )
      {
         if( driver->closeSource )
            driver->closeSource( sourceHandle );
         TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(24) );
      }

      if( (sourceHandle->nbCategory > 1) && !driver->getNextCategoryHandle )
      {
         if( driver->closeSource )
            driver->closeSource( sourceHandle );
         TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(25) );
      }

      tmpListCategory = TA_ListAlloc();
      if( !tmpListCategory )
      {
         if( driver->closeSource )
            driver->closeSource( sourceHandle );
         TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      categoryIndex = 0;
      again = 1;
      while( again && (categoryIndex < sourceHandle->nbCategory) )
      {
         /* Allocate the categoryHandle. */
         categoryHandle = (TA_CategoryHandle *)TA_Malloc( sizeof(TA_CategoryHandle) );
         if( !categoryHandle )
         {
            freeListAndElement( tmpListCategory, freeCategoryHandle );
            if( driver->closeSource )
               driver->closeSource( sourceHandle );
            TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }

         /* Default values. */
         memset( categoryHandle, 0, sizeof( TA_CategoryHandle ) );

         /* Fill the categoryHandle information. */
         if( categoryIndex == 0 )
            retCode = driver->getFirstCategoryHandle( sourceHandle,
                                                      categoryHandle );
         else
            retCode = driver->getNextCategoryHandle( sourceHandle,
                                                     categoryHandle,
                                                     categoryIndex );

         categoryIndex++;
         if( retCode == TA_END_OF_INDEX )
         {
            TA_Free( categoryHandle );
            again = 0;
         }
         else if( retCode != TA_SUCCESS )
         {
            freeListAndElement( tmpListCategory, freeCategoryHandle );
            TA_Free(  categoryHandle );
            if( driver->closeSource )
               driver->closeSource( sourceHandle );
            TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
            TA_TRACE_RETURN( retCode );
         }
         else
         {
            retCode = processCategoryAndSymbols( privUDB,
                                                 driver, addDataSourceParamPriv,
                                                 sourceHandle,
                                                 categoryHandle );

            if( retCode != TA_SUCCESS )
            {
               freeListAndElement( tmpListCategory, freeCategoryHandle );
               TA_Free(  categoryHandle );
               if( driver->closeSource )
                  driver->closeSource( sourceHandle );
               TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
               TA_TRACE_RETURN( retCode );
            }

            /* Keep that category in a list. Will be assigned
             * for ownership to the TA_DataSource later.
             */
            retCode = TA_ListAddTail( tmpListCategory, categoryHandle );
            if( retCode != TA_SUCCESS )
            {
               freeListAndElement( tmpListCategory, freeCategoryHandle );
               TA_Free(  categoryHandle );
               if( driver->closeSource )
                  driver->closeSource( sourceHandle );
               TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
               TA_TRACE_RETURN( retCode );
            }
         }
      }
   }

   /* Keep track of that source in the listDataSource. */
   dataSource = allocDataSourceForGlobal( privUDB,
                                          addDataSourceParamPriv,
                                          sourceHandle,
                                          tmpListCategory );
   if( !dataSource )
   {
      freeListAndElement( tmpListCategory, freeCategoryHandle );
      if( driver->closeSource )
         driver->closeSource( sourceHandle );
      TA_AddDataSourceParamPrivFree( addDataSourceParamPriv );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CategoryTableAlloc( TA_UDBase *unifiedDatabase,
                                  TA_StringTable **table )
{
   TA_PROLOG
   TA_StringTable *stringTable;
   TA_UDBasePriv *privUDB;
   TA_StringCache *stringCache;
   unsigned int i;
   unsigned int again;
   TA_RetCode finalRetCode;
   TA_CategoryTableHiddenData *hiddenData;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( !unifiedDatabase || !table )
      return TA_BAD_PARAM;

   *table = NULL;

   privUDB = (TA_UDBasePriv *)unifiedDatabase;
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;

   TA_TRACE_BEGIN( TA_CategoryTableAlloc );

   stringTable = TA_Malloc( sizeof(TA_StringTable) );

   if( stringTable == NULL )
      TA_TRACE_RETURN( TA_ALLOC_ERR );

   stringTable->size = 0;
   stringTable->string = NULL;

   /* Handle special case where there is not even one data source added
    * (of course there is no category in that case).
    */
   if( privUDB->dictCategory == NULL )
   {
      *table = stringTable;
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   stringCache = TA_GetGlobalStringCache();

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &privUDB->sema );

   if( retCode != TA_SUCCESS )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   finalRetCode = TA_SUCCESS;

   /* Extract all 'category' from the category dictionary. */
   stringTable->size = TA_DictAccessFirst( privUDB->dictCategory );
   if( stringTable->size != 0 )
   {
      stringTable->string = (const char **)TA_Malloc( (stringTable->size) *
                                                      sizeof( const char *) );

      if( stringTable->string == NULL )
      {
         stringTableFree( stringTable, 0 );
         finalRetCode = TA_ALLOC_ERR;
      }
      else
      {
         memset( (void *)stringTable->string, 0, (stringTable->size) * sizeof( const char *) );

         again = 1;
         for( i=0; (i < stringTable->size) && again; i++ )
         {
            (stringTable->string)[i] = TA_StringToChar( TA_StringDup( stringCache, TA_DictAccessKey( privUDB->dictCategory ) ) );
            again = TA_DictAccessNext( privUDB->dictCategory );
         }
      }
   }

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaPost( &privUDB->sema );

   if( retCode != TA_SUCCESS )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   /* Check if an error occured in the critical section. */
   if( finalRetCode != TA_SUCCESS )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( finalRetCode );
   }

   /* Allocate and initialize the hidden data. */
   hiddenData = TA_Malloc( sizeof(TA_CategoryTableHiddenData) );
   if( !hiddenData )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   hiddenData->privUDB = privUDB;
   hiddenData->magicNb = TA_CATEGORY_TABLE_MAGIC_NB;
   stringTable->hiddenData = hiddenData;

   /* Return the table to the caller. */
   *table = stringTable;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CategoryTableFree( TA_StringTable *table )
{
   TA_PROLOG
   TA_UDBasePriv *privUDB;
   TA_CategoryTableHiddenData *hiddenData;
   TA_RetCode retCode;

   /* If it appears to be already freed, just return. */
   if( !table )
      return TA_SUCCESS;

   /* Make sure we are dealing with the right object */
   hiddenData = (TA_CategoryTableHiddenData *)table->hiddenData;
   if( !hiddenData )
      return TA_ALLOC_ERR;
   if( hiddenData->magicNb != TA_CATEGORY_TABLE_MAGIC_NB )
      return TA_BAD_OBJECT;
   privUDB = (TA_UDBasePriv *)hiddenData->privUDB;
   if( !privUDB )
      return TA_ALLOC_ERR;
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;

   TA_TRACE_BEGIN(  TA_CategoryTableFree );

   /* Free the hidden data. */
   memset( hiddenData, 0, sizeof( TA_CategoryTableHiddenData ) );
   TA_Free( hiddenData );
   
   /* Free the strings. */
   retCode = stringTableFree( table, 0 );

   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_SymbolTableAlloc( TA_UDBase *unifiedDatabase,
                                const char *categoryString,
                                TA_StringTable **table )
{
   TA_PROLOG
   TA_StringTable  *stringTable;
   TA_UDB_Category *categoryData;
   TA_Dict         *dictUDBSymbol;
   TA_UDBasePriv   *privUDB;
   TA_StringCache  *stringCache;
   unsigned int     i;
   unsigned int     again;
   TA_SymbolTableHiddenData *hiddenData;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode       retCode;
   #endif

   if( !table || !unifiedDatabase )
      return TA_BAD_PARAM;

   *table = NULL;

   privUDB = (TA_UDBasePriv *) unifiedDatabase;
   TA_TRACE_BEGIN(  TA_SymbolTableAlloc );

   stringTable = TA_Malloc( sizeof(TA_StringTable) );

   if( stringTable == NULL )
      TA_TRACE_RETURN( TA_ALLOC_ERR );

   stringTable->size = 0;
   stringTable->string = NULL;

   /* Handle special case where there is not even one data source added
    * (of course there is no symbol in that case).
    */
   if( privUDB->dictCategory == NULL )
   {
      *table = stringTable;
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* If categoy is NULL, use the default category. */
   if( !categoryString )
      categoryString = TA_DEFAULT_CATEGORY;

   stringCache = TA_GetGlobalStringCache();

   /* Find the symbol dictionary. */

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &privUDB->sema );

   if( retCode != TA_SUCCESS )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   categoryData = TA_DictGetValue_S( privUDB->dictCategory, categoryString );

   if( !categoryData || !(categoryData->dictUDBSymbol))
   {
      /* Category not found... */
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaPost( &privUDB->sema );
      #endif
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( TA_CATEGORY_NOT_FOUND );
   }

   dictUDBSymbol = categoryData->dictUDBSymbol;

   /* Extract all 'symbols' from the symbol dictionary. */
   stringTable->size = TA_DictAccessFirst( dictUDBSymbol );
   if( stringTable->size != 0 )
   {
      stringTable->string = (const char **)TA_Malloc( (stringTable->size) *
                                                      sizeof( const char *) );

      if( stringTable->string == NULL )
      {
         #if !defined( TA_SINGLE_THREAD )
            TA_SemaPost( &privUDB->sema );
         #endif
         stringTableFree( stringTable, 0 );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      memset( (void *)stringTable->string, 0, (stringTable->size) * sizeof( const char *) );

      again = 1;
      for( i=0; (i < stringTable->size) && again; i++ )
      {
         (stringTable->string)[i] = TA_StringToChar( TA_StringDup( stringCache, TA_DictAccessKey( dictUDBSymbol ) ) );
         again = TA_DictAccessNext( dictUDBSymbol );
      }
   }

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaPost( &privUDB->sema );

   if( retCode != TA_SUCCESS )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( retCode );
   }
   #endif

   /* Allocate and initialize the hidden data. */
   hiddenData = TA_Malloc( sizeof(TA_SymbolTableHiddenData) );
   if( !hiddenData )
   {
      stringTableFree( stringTable, 0 );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   hiddenData->privUDB = privUDB;
   hiddenData->magicNb = TA_SYMBOL_TABLE_MAGIC_NB;
   stringTable->hiddenData = hiddenData;

   /* Return the table to the caller. */
   *table = stringTable;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SymbolTableFree( TA_StringTable *table )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_UDBasePriv *privUDB;
   TA_SymbolTableHiddenData *hiddenData;

   /* If it appears to be already freed, just return. */
   if( !table )
      return TA_SUCCESS;

   /* Make sure we are dealing with the right object */
   hiddenData = (TA_SymbolTableHiddenData *)table->hiddenData;
   if( !hiddenData )
      return TA_ALLOC_ERR;
   if( hiddenData->magicNb != TA_SYMBOL_TABLE_MAGIC_NB )
      return TA_BAD_OBJECT;
   privUDB = (TA_UDBasePriv *)hiddenData->privUDB;
   if( !privUDB )
      return TA_ALLOC_ERR;
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;
    
   TA_TRACE_BEGIN( TA_SymbolTableFree );

   /* Free the hidden data. */
   memset( hiddenData, 0, sizeof( TA_SymbolTableHiddenData ) );
   TA_Free(  hiddenData );

   retCode = stringTableFree( table, 0 );

   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_ForEachSymbol( TA_UDBase *unifiedDatabase,
                             TA_ForEachSymbolFunc functionToCall,
                             void *opaqueData )
{
   TA_PROLOG
   unsigned int i;
   unsigned int j;
   TA_StringTable *tableCategory;
   TA_StringTable *tableSymbol;
   TA_RetCode retCode;
   TA_UDBasePriv *privUDB;

   if( !unifiedDatabase || !functionToCall )
      return TA_BAD_PARAM;

   privUDB = (TA_UDBasePriv *)unifiedDatabase;
   if( privUDB->magicNb != TA_UDBASE_MAGIC_NB )
      return TA_BAD_OBJECT;

   TA_TRACE_BEGIN( TA_ForEachSymbol );

   /* Get all the category to iterate. */
   retCode = TA_CategoryTableAlloc( unifiedDatabase, &tableCategory );

   if( retCode == TA_SUCCESS )
   {
      for( i=0; i < tableCategory->size; i++ )
      {
         TA_ASSERT( tableCategory->string[i] != NULL );
         /* Get all the symbols to iterate for this category. */
         retCode = TA_SymbolTableAlloc( unifiedDatabase,
                                        tableCategory->string[i],
                                        &tableSymbol );

         if( retCode == TA_SUCCESS )
         {
            for( j=0; j < tableSymbol->size; j++ )
            {
               TA_ASSERT( tableSymbol->string[j] != NULL );
               (*functionToCall)( unifiedDatabase,
                                  tableCategory->string[i],
                                  tableSymbol->string[j],
                                  opaqueData );
            }

            TA_SymbolTableFree( tableSymbol );
         }
      }

      TA_CategoryTableFree( tableCategory );
   }
   else
      return TA_INTERNAL_ERROR(27);

   return TA_SUCCESS;
}

TA_RetCode TA_HistoryAlloc( TA_UDBase           *unifiedDatabase,
                            const TA_HistoryAllocParam *param,
                            TA_History         **history )
{
   TA_PROLOG
   TA_RetCode retCode;

   TA_UDB_Category *categoryData;
   TA_UDB_Symbol   *symbolData;
   TA_History      *newHistory;
   TA_Dict         *dictUDBSymbol;
   TA_UDBasePriv   *privUDB;
   TA_StringCache  *stringCache;

   const TA_Timestamp *startLocal;
   const TA_Timestamp *endLocal;
   const char *categoryLocal;

   privUDB = (TA_UDBasePriv *)unifiedDatabase;

   TA_TRACE_BEGIN( TA_HistoryAlloc );

   if( (history == NULL) || (param->symbol == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   if( (param->period == 0) || (param->period > TA_YEARLY) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   /* startLocal and endLocal must end up NULL if the
    * start and end are not specified.
    */
   startLocal = &param->start;
   endLocal   = &param->end;

   if( (startLocal->date == 0) && (startLocal->time == 0) )
      startLocal = (const TA_Timestamp *)0;
   if( (endLocal->date == 0) && (endLocal->time == 0) )
      endLocal = (const TA_Timestamp *)0;

   /* Verify validity of start/end */
   if( startLocal )
   {
      if( param->period >= TA_DAILY )
         retCode = TA_TimestampValidateYMD( startLocal );
      else
         retCode = TA_TimestampValidate( startLocal );
   
      if( retCode != TA_SUCCESS )
         TA_TRACE_RETURN( TA_BAD_START_DATE );
   }

   if( endLocal )
   {
      if( param->period >= TA_DAILY )
         retCode = TA_TimestampValidateYMD( endLocal );
      else
         retCode = TA_TimestampValidate( endLocal );

      if( retCode != TA_SUCCESS )
         TA_TRACE_RETURN( TA_BAD_END_DATE );
   }

   *history = NULL;

   /* Handle special case where there is not even one data source added. */
   if( privUDB->dictCategory == NULL )
   {
      TA_TRACE_RETURN( TA_NO_DATA_SOURCE );
   }

   /* Look for the category. If no category provided, use the default. */
   categoryLocal = TA_DEFAULT_CATEGORY;
   if( param->category != NULL )
      categoryLocal = param->category;

   stringCache = TA_GetGlobalStringCache();

   categoryData = TA_DictGetValue_S( privUDB->dictCategory, categoryLocal );

   if( !categoryData )
   {
      TA_TRACE_RETURN( TA_CATEGORY_NOT_FOUND );
   }

   if( !categoryData->dictUDBSymbol )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(28) );
   }

   /* Look for the symbol in this category. */
   dictUDBSymbol = categoryData->dictUDBSymbol;

   symbolData = TA_DictGetValue_S( dictUDBSymbol, param->symbol );

   if( !symbolData )
   {
      TA_TRACE_RETURN( TA_SYMBOL_NOT_FOUND );
   }

   /* Leave it to the TA_History sub-module to do the rest. */
   retCode = TA_HistoryBuilder( privUDB, symbolData,
                                param->period, startLocal, endLocal,
                                param->field, param->flags, param->timeout, &newHistory );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( newHistory != NULL );

   /* History has been allocated, return this information to the caller. */
   *history = newHistory;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_HistoryFree( TA_History *history )
{
   TA_PROLOG
   TA_HistoryHiddenData *hiddenData;

   TA_TRACE_BEGIN( TA_HistoryFree );

   if( !history )       
      TA_TRACE_RETURN(TA_BAD_PARAM);

   hiddenData = (TA_HistoryHiddenData *)history->hiddenData;
   if( !hiddenData || (hiddenData->magicNb != TA_HISTORY_MAGIC_NB) )
      TA_TRACE_RETURN(TA_BAD_OBJECT);

   /* Free all block of data. 
    *
    * The meaning of setting the hiddenData->"field"
    * is to transform the corresponding history->"field" 
    * into an internal pointer instead of a pointer where
    * the allocated memory starts.
    * 
    * When hiddenData->"field" is not set, the corresponding
    * public field in TA_History is the one that should
    * be look at for being freed.
    */
   #define FREE_FIELD(x) { \
      if( hiddenData->x ) \
      { \
         TA_Free((void *)hiddenData->x); \
         hiddenData->x = NULL; \
         history->x = NULL; \
      } \
      else \
      { \
         FREE_IF_NOT_NULL( history->x ); \
      } \
   }         
   FREE_FIELD( timestamp );
   FREE_FIELD( close );
   FREE_FIELD( open );
   FREE_FIELD( high );
   FREE_FIELD( low );
   FREE_FIELD( volume );
   FREE_FIELD( openInterest );
   #undef FREE_FIELD

   stringTableFree( &history->listOfSource, 1 );

   TA_Free( (void *)history );

   TA_TRACE_RETURN( TA_SUCCESS );
}

int TA_HistoryEqual( const TA_History *history1, const TA_History *history2 )
{
   unsigned int i;

   /* Return 1 if the content of both history is
    * identical or if both history pointer are NULL.
    */
   if( !history1 && !history2 )
      return 1;

   if( !history1 && history2 )
      return 0;

   if( history1 && !history2 )
      return 0;

   if( history1->period != history2->period )
      return 0;

   if( history1->nbBars != history2->nbBars )
      return 0;

   #define CHECK_MEMBER(param) \
   { \
      if( ( history1->param && !history2->param) || \
          (!history1->param &&  history2->param) ) \
         return 0; \
      if( history1->param ) \
      { \
         for( i=0; i < history1->nbBars; i++ ) \
         { \
            if( history1->param[i] != history2->param[i] ) \
               return 0; \
         } \
      } \
   }

   CHECK_MEMBER(open);
   CHECK_MEMBER(high);
   CHECK_MEMBER(low);
   CHECK_MEMBER(close);
   CHECK_MEMBER(volume);
   CHECK_MEMBER(openInterest);
   #undef CHECK_MEMBER

   if( ( history1->timestamp && !history2->timestamp) ||
       (!history1->timestamp &&  history2->timestamp) )
      return 0;
   if( history1->timestamp )
   {
      for( i=0; i < history1->nbBars; i++ )
      {
         if( !TA_TimestampEqual(&history1->timestamp[i],&history2->timestamp[i]) )
            return 0;
      }
   }

   return 1; /* Contents is equivalent. */
}

static TA_AddDataSourceParamPriv *TA_AddDataSourceParamPrivAlloc( const TA_AddDataSourceParam *param, TA_SourceFlag flag )
{
   /* Alloc an internal copy of the TA_AddDataSourceParam . */
   TA_AddDataSourceParamPriv *dst;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   dst = (TA_AddDataSourceParamPriv *)TA_Malloc( sizeof(TA_AddDataSourceParamPriv) );
   if( !dst )
      return NULL;

   memset( dst, 0, sizeof( TA_AddDataSourceParamPriv ) );

   /* At this point, TA_AddDataSourceParamPrivFree can be safely called. */

   #define DO(func,y) \
      { \
         if(param->y) \
         { \
            dst->y = func( stringCache, param->y ); \
            if( !dst->y ) \
            { \
               TA_AddDataSourceParamPrivFree( dst ); \
               return NULL; \
            } \
         } \
         else \
            dst->y = NULL; \
      } 

   /* Same as DO but provide a default if not specified by the user. */
   #define DO_DFLT(func,y,dflt) \
      { \
         if( param->y ) \
            dst->y = func( stringCache, param->y ); \
         else \
            dst->y = func( stringCache, dflt ); \
         if( !dst->y ) \
         { \
            TA_AddDataSourceParamPrivFree( dst ); \
            return NULL; \
         } \
      }

      if( flag & TA_LOCATION_IS_PATH )
      {
         DO( TA_StringAlloc_Path, location );
      }
      else
      {
         DO( TA_StringAlloc, location );
      }
         
      DO( TA_StringAlloc,     info     );
      DO( TA_StringAlloc,     username );
      DO( TA_StringAlloc,     password );
      DO( TA_StringAlloc,     symbol   );

      DO_DFLT( TA_StringAlloc, category,   TA_DEFAULT_CATEGORY          );
      DO_DFLT( TA_StringAlloc, country,    TA_DEFAULT_CATEGORY_COUNTRY  );
      DO_DFLT( TA_StringAlloc, exchange,   TA_DEFAULT_CATEGORY_EXCHANGE );
      DO_DFLT( TA_StringAlloc, type,       TA_DEFAULT_CATEGORY_TYPE     );
      DO_DFLT( TA_StringAlloc, sourceName, TA_gDataSourceTable[param->id].defaultName );
   #undef DO
   #undef DO_FLT

   /* Copy the rest. */
   dst->flags  = param->flags;
   dst->id     = param->id;
   dst->period = param->period;
   return dst;
}

static TA_RetCode TA_AddDataSourceParamPrivFree( TA_AddDataSourceParamPriv *toBeFreed )
{
   TA_StringCache *stringCache;

   if( toBeFreed )
   {
      stringCache = TA_GetGlobalStringCache();

      /* Free all the strings that are not NULL. */
      #define DO(y) { if(toBeFreed->y) TA_StringFree( stringCache, toBeFreed->y ); }
         DO( location   );
         DO( info       );
         DO( username   );
         DO( password   );
         DO( category   );
         DO( country    );
         DO( exchange   );
         DO( type       );
         DO( symbol     );
         DO( sourceName );
      #undef DO

      TA_Free( toBeFreed );
   }

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/

static TA_RetCode closeAllUDBase( TA_DataGlobal *global )
{
   TA_List *listUDB;
   TA_UDBase *udBase;

   /* Attempts to close all UDBase belonging to this library. */
   if( !global )
      return TA_INTERNAL_ERROR(30);

   listUDB = global->listUDBase;

   udBase = (TA_UDBase *)TA_ListRemoveTail( listUDB );
   while( udBase )
   {
      TA_UDBaseFree( udBase );
      udBase = (TA_UDBase *)TA_ListRemoveTail( listUDB );
   }

   return TA_SUCCESS;
}

static TA_RetCode closeAllDataSource( TA_UDBasePriv *privUDB )
{
   TA_PROLOG
   TA_DataSource       *dataSource;
   TA_StringCache      *stringCache;
   TA_List             *listDataSource;

   const TA_DataSourceDriver *driver;
   
   TA_TRACE_BEGIN( closeAllDataSource );
   stringCache = TA_GetGlobalStringCache();

   listDataSource = privUDB->listDataSource;

   dataSource = (TA_DataSource *)TA_ListRemoveTail( listDataSource );
   while( dataSource )
   {
      TA_ASSERT( dataSource->addDataSourceParamPriv->id < TA_NUM_OF_SOURCE_ID );

      driver = &TA_gDataSourceTable[dataSource->addDataSourceParamPriv->id];

      if( driver->closeSource )
      {
         if( dataSource->sourceHandle )
            driver->closeSource( dataSource->sourceHandle );
      }

      if( dataSource->addDataSourceParamPriv )
         TA_AddDataSourceParamPrivFree( dataSource->addDataSourceParamPriv );

      if( dataSource->listCategoryHandle )
         freeListAndElement( dataSource->listCategoryHandle, freeCategoryHandle );

      TA_Free( dataSource );

      dataSource = (TA_DataSource *)TA_ListRemoveTail( listDataSource );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode shutdownAllSourceDriver( TA_DataGlobal *global )
{
   unsigned int i;
   const TA_DataSourceDriver *driver;

   for( i=0; i < TA_gDataSourceTableSize; i++)
   {
      if( global->dataSourceInitFlag[i] )
      {
         driver = &TA_gDataSourceTable[i];
         if( driver->shutdownSourceDriver )
            driver->shutdownSourceDriver();
         global->dataSourceInitFlag[i] = 0;
      }
   }

   return TA_SUCCESS;
}

/* Add an entry in the dictCategory for the specified UDBase.
 * - If an entry is already existing, no entry are added and the existing
 *   entry is returned.
 * - If the entry does not exist, it is created and returned.
 *
 * On error, NULL is returned.
 */
static TA_UDB_Category *addCategory( TA_UDBasePriv *privUDB,
                                     TA_String *string )
{
   TA_UDB_Category *category;
   TA_RetCode retCode;
   TA_Dict *dictCategory;
   TA_StringCache *stringCache;

   if( !privUDB )
      return NULL;

   /* Validate parameters. */
   TA_ASSERT_RET( string != NULL, (TA_UDB_Category *)NULL );

   dictCategory = privUDB->dictCategory;
   TA_ASSERT_RET( dictCategory != NULL, (TA_UDB_Category *)NULL );

   /* Check if the string already exist in the dictionary.
    * If yes, do not add it, simply return it.
    */
   category = (TA_UDB_Category *)TA_DictGetValue_S( dictCategory, TA_StringToChar(string) );

   if( category )
      return category;

   /* The entry does not exist, create it and add it to the
    * dictCategory.
    */
   category = (TA_UDB_Category *)TA_Malloc( sizeof( TA_UDB_Category ) );

   if( category == NULL )
      return NULL;

   TA_ListInit(  &category->listUDBDriverArray );

   retCode = TA_DictAddPair_S( dictCategory, string, category );

   if( retCode != TA_SUCCESS )
   {
      TA_Free( category );
      return NULL;
   }

   /* Initialize the TA_UDB_Category fields. */
   category->dictUDBSymbol = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeSymbolData );

   if( !category->dictUDBSymbol )
   {
      /* Can not create the symbol dict!? Clean-up and get out of here.... */
      TA_DictDeletePair_S( dictCategory, TA_StringToChar(string) );
      TA_Free( category );
      return NULL;
   }

   stringCache = TA_GetGlobalStringCache();
   category->string = TA_StringDup( stringCache, string );

   if( !category->string )
   {
      TA_DictFree( category->dictUDBSymbol );
      TA_DictDeletePair_S( dictCategory, TA_StringToChar(string) );
      TA_Free( category );
      return NULL;
   }

   return category;
}

static TA_UDB_Symbol *addSymbol( TA_UDBasePriv *privUDB,
                                 TA_Dict *dictUDBSymbol,
                                 TA_UDB_Driver *driverHandle,
                                 TA_DataSourceHandle *sourceHandle,
                                 TA_CategoryHandle *categoryHandle,
                                 TA_SymbolHandle *symbolHandle )
{
   TA_UDB_Symbol *data;   
   TA_RetCode retCode;
   TA_StringCache *stringCache;

   if( !privUDB )
      return NULL;

   /* Validate parameters. */
   TA_ASSERT_RET( dictUDBSymbol != NULL, (TA_UDB_Symbol *)NULL );
   TA_ASSERT_RET( sourceHandle != NULL, (TA_UDB_Symbol *)NULL );
   TA_ASSERT_RET( categoryHandle != NULL, (TA_UDB_Symbol *)NULL );
   TA_ASSERT_RET( symbolHandle != NULL, (TA_UDB_Symbol *)NULL );
   TA_ASSERT_RET( symbolHandle->string != NULL, (TA_UDB_Symbol *)NULL );

   stringCache = TA_GetGlobalStringCache();

   /* Check if the symbol is already in the dictionary.
    * If yes, use it, else create a new entry in the dictionary.
    */
   data = (TA_UDB_Symbol *)TA_DictGetValue_S( dictUDBSymbol, TA_StringToChar(symbolHandle->string) );

   if( data == NULL )
   {
      data = (TA_UDB_Symbol *)TA_Malloc( sizeof( TA_UDB_Symbol ) );

      if( data == NULL )
         return NULL;

      TA_ListInit( &data->listDriverHandle );

      retCode = TA_DictAddPair_S( dictUDBSymbol, symbolHandle->string, data );

      if( retCode != TA_SUCCESS )
      {
         TA_Free( data );
         return NULL;
      }

      /* Initialize the TA_UDB_Symbol fields. */
      data->string = TA_StringDup( stringCache, symbolHandle->string );

      if( data->string == NULL )
      {
         TA_DictDeletePair_S( dictUDBSymbol, TA_StringToChar(symbolHandle->string) );
         TA_Free( data );
         return NULL;
      }
   }

   /* Add the TA_UDB_Driver to the list for this symbol. */
   retCode = TA_ListNodeAddTail( &data->listDriverHandle, &driverHandle->node, driverHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_DictDeletePair_S( dictUDBSymbol, TA_StringToChar(symbolHandle->string) );
      TA_StringFree( stringCache, data->string );
      TA_Free( data );
      TA_Free( driverHandle );
      return NULL;
   }

   return data;
}

static TA_RetCode stringTableFree( TA_StringTable *table, unsigned int freeInternalStringOnly )
{
   TA_PROLOG
   unsigned int size;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( stringTableFree );

   if( table == NULL )
   {
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   stringCache = TA_GetGlobalStringCache();

   if( table->string )
   {
      size = table->size;

      while( size > 0 )
      {
         if( (table->string)[size-1] )
            TA_StringFree( stringCache, TA_StringFromChar((table->string)[size-1]) );

         size--;
      }

      TA_Free( (void *)table->string );
   }

   if( !freeInternalStringOnly )
      TA_Free( table );

   TA_TRACE_RETURN( TA_SUCCESS );
}

/* On deletion of dictCategory, this function will get called
 * for each TA_UDB_Category member of the dictionary.
 */
static void freeCategoryData( void *toBeFreed )
{
   TA_UDB_Category *categoryData = (TA_UDB_Category *)toBeFreed;
   TA_StringCache *stringCache;

   if( categoryData->string )
   {
      stringCache = TA_GetGlobalStringCache();
      TA_StringFree( stringCache, categoryData->string );
   }

   /* Delete all symbols under that category. */
   if( categoryData->dictUDBSymbol )
      TA_DictFree( categoryData->dictUDBSymbol );

   /* Delete all the TA_UDB_Driver arrays. */
   freeListAndElement( &categoryData->listUDBDriverArray, freeUDBDriverArray );

   TA_Free( categoryData );
}

/* On deletion of a symbol dictionary, this function will get called
 * for each TA_UDB_Symbol member of the dictionary.
 */
static void freeSymbolData( void *toBeFreed )
{
   TA_UDB_Symbol *symbolData = (TA_UDB_Symbol *)toBeFreed;
   TA_StringCache *stringCache;

   /* Free the category/strings handles. */
   if( symbolData->string )
   {
      stringCache = TA_GetGlobalStringCache();
      TA_StringFree( stringCache, symbolData->string );
   }

   TA_Free(  symbolData );
}

static TA_RetCode processCategoryAndSymbols( TA_UDBasePriv *privUDB,
                                             const TA_DataSourceDriver *driver,
                                             const TA_AddDataSourceParamPriv *addDataSourceParamPriv,
                                             TA_DataSourceHandle *sourceHandle,
                                             TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_UDB_Category *categoryData;
   TA_UDB_Symbol   *symbolData;
   TA_UDB_Driver *theUDBDriverArray;
   TA_UDB_Driver *currentUDBDriver;

   TA_SymbolHandle *symbolHandle;
   unsigned int nbSymbol;
   unsigned int symbolIndex;

   if( !privUDB )
      return TA_INTERNAL_ERROR(31);

   TA_TRACE_BEGIN(  processCategoryAndSymbols );

   TA_ASSERT( driver != NULL );
   TA_ASSERT( sourceHandle != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( categoryHandle->string != NULL );

   categoryData = addCategory( privUDB, categoryHandle->string );

   if( (categoryData == NULL) || (categoryData->dictUDBSymbol == NULL) )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   nbSymbol = categoryHandle->nbSymbol;
   if( nbSymbol != 0 )
   {
      /* In one shot, allocate all the TA_UDB_Driver for all the symbols for
       * this category. This array will be owned by the corresponding TA_UDB_Category.
       */
      theUDBDriverArray = (TA_UDB_Driver *)TA_Malloc( sizeof( TA_UDB_Driver ) * nbSymbol );
      if( !theUDBDriverArray )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
      memset( theUDBDriverArray, 0, sizeof( TA_UDB_Driver ) * nbSymbol );

      retCode = TA_ListAddTail( &categoryData->listUDBDriverArray, theUDBDriverArray );
      if( retCode != TA_SUCCESS )
      {
         TA_Free(  theUDBDriverArray );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* For the time being, for simplicity, nothing is removed from the
       * listUDBDriverArray, until the corresponding category is deleted.
       */
      for( symbolIndex=0; symbolIndex < nbSymbol; symbolIndex++ )
      {
         /* Initialize the TA_UDB_Driver for this symbol. */
         currentUDBDriver = &theUDBDriverArray[symbolIndex];
         currentUDBDriver->sourceHandle  = sourceHandle;
         currentUDBDriver->categoryHandle = categoryHandle;
         currentUDBDriver->addDataSourceParamPriv = addDataSourceParamPriv;

         /* Ask the driver to fill the symbolHandle information. */
         symbolHandle = &(currentUDBDriver->symbolHandle);
         if( symbolIndex == 0 )
            retCode = driver->getFirstSymbolHandle( sourceHandle,
                                                    categoryHandle,
                                                    symbolHandle );
         else
            retCode = driver->getNextSymbolHandle( sourceHandle,
                                                   categoryHandle,
                                                   symbolHandle,
                                                   symbolIndex );

         if( retCode != TA_SUCCESS )
         {
            TA_TRACE_RETURN( retCode );
         }
         else
         {
            /* Add the symbol to the unified database */            
            symbolData = addSymbol( privUDB, categoryData->dictUDBSymbol,
                                    currentUDBDriver,
                                    sourceHandle,
                                    categoryHandle,
                                    symbolHandle );
            if( symbolData == NULL )
            {
               TA_TRACE_RETURN( TA_ALLOC_ERR );
            }
         }
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeListAndElement( TA_List *list,
                                      TA_RetCode (*freeFunc)( void *toBeFreed ))
{
   TA_PROLOG
   TA_RetCode retCode;
   void *node;

   TA_TRACE_BEGIN( freeListAndElement );

   if( list != NULL )
   {
      while( (node = TA_ListRemoveTail( list )) != NULL )
      {
         retCode = freeFunc(  node );
         if( retCode != TA_SUCCESS )
         {
            TA_FATAL(  NULL, node, retCode );
         }
      }

      retCode = TA_ListFree( list );
      if( retCode != TA_SUCCESS )
      {
         TA_FATAL(  NULL, list, retCode );
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeUDBDriverArray( void *toBeFreed )
{
   /* Free all the TA_UDB_Driver of a certain category/datasource
    * combination in one shot (see TA_UDB_Category->listUDBDriverArray)
    */
   if( toBeFreed )
      TA_Free( toBeFreed );

   return TA_SUCCESS;
}

static TA_RetCode freeCategoryHandle( void *toBeFreed )
{
   if( toBeFreed )
      TA_Free( (TA_CategoryHandle *)toBeFreed );

   return TA_SUCCESS;
}

static TA_DataSource *allocDataSourceForGlobal( TA_UDBasePriv *privUDB,
                                                TA_AddDataSourceParamPriv *addDataSourceParamPriv,
                                                TA_DataSourceHandle *sourceHandle,
                                                TA_List *listCategoryHandle )
{
   TA_RetCode finalRetCode;
   TA_DataSource *dataSource;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( !privUDB )
      return NULL;

   dataSource = (TA_DataSource *)TA_Malloc( sizeof( TA_DataSource ) );

   memset( dataSource, 0, sizeof( TA_DataSource ) );

   dataSource->addDataSourceParamPriv = addDataSourceParamPriv;
   dataSource->sourceHandle = sourceHandle;
   dataSource->listCategoryHandle = listCategoryHandle;

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaWait( &privUDB->sema );
   if( retCode != TA_SUCCESS )
   {
      TA_Free(  dataSource );
      return NULL;
   }
   #endif

   finalRetCode = TA_ListAddTail( privUDB->listDataSource, dataSource );

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaPost( &privUDB->sema );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   #endif

   if( finalRetCode != TA_SUCCESS )
   {
      TA_Free(  dataSource );
      return NULL;
   }

   return dataSource;
}

static TA_RetCode TA_DataGlobalShutdown( void *globalAllocated )
{
   TA_RetCode retCode;
   TA_RetCode finalRetCode;
   TA_DataGlobal *global;

   /* Will change if an error occured. */
   finalRetCode = TA_SUCCESS;

   if( !globalAllocated )
      return TA_BAD_PARAM;

   global = (TA_DataGlobal *)globalAllocated;

   /* Free all the UDBase. */
   retCode = closeAllUDBase( global );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;
   TA_ListFree( global->listUDBase );

   /* Shutdown all the source drivers. */
   retCode = shutdownAllSourceDriver( global );
   if( retCode != TA_SUCCESS )
      finalRetCode = retCode;

   TA_Free( global->dataSourceInitFlag );
   TA_Free( globalAllocated );

   return finalRetCode;
}

static TA_RetCode TA_DataGlobalInit( void **globalToAlloc )
{
   TA_PROLOG
   TA_DataGlobal *global;

   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   TA_TRACE_BEGIN( TA_DataGlobalInit );

   /* Make some "parano" runtime sanity check of the code. */
   TA_ASSERT( TA_gDataSourceTableSize == TA_NUM_OF_SOURCE_ID );

   *globalToAlloc = NULL;

   /* Allocate the global. */
   global = (TA_DataGlobal *)TA_Malloc( sizeof( TA_DataGlobal ) );
   if( !global )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Allocate and initialize flags use to keep track of which data source
    * driver have been initialized.
    */
   global->dataSourceInitFlag = (unsigned char *)TA_Malloc( TA_gDataSourceTableSize * sizeof( unsigned char) );
   if( !global->dataSourceInitFlag )
   {
      TA_Free( global );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   memset( global->dataSourceInitFlag, 0, TA_gDataSourceTableSize * sizeof( unsigned char) );

   /* Initialize the list keeping track of the UDBase. */
   global->listUDBase = TA_ListAlloc( );
   if( !global->listUDBase )
   {
      TA_Free( global->dataSourceInitFlag );
      TA_Free( global );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaInit( &global->sema, 1 );
      if( retCode != TA_SUCCESS )
      {
         TA_ListFree( global->listUDBase );
         TA_Free( global->dataSourceInitFlag );
         TA_Free( global );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
   #endif

   /* Success! Return the global to the caller. */
   *globalToAlloc = global;
   TA_TRACE_RETURN( TA_SUCCESS );
}


