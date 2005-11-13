/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110199 MF   First version.
 *  062105 PK   Simulator can produce end-of-period timestamped data.
 *
 */

/* Description:
 *    This is the entry points of the data source driver for TA_Simulator.
 *
 *    It provides ALL the functions needed by the "TA_DataSourceDriver"
 *    structure (see ta_source.h).
 */

/**** Headers ****/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "ta_source.h"
#include "ta_simulator.h"
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_list.h"
#include "ta_data.h"
#include "ta_system.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
#define TA_REF_DAILY_NB_BARS 252 /* from ta_daily_ref_0.c */
extern TA_Timestamp TA_SREF_timestamp_daily_ref_0_PRIV [TA_REF_DAILY_NB_BARS];
extern TA_Real      TA_SREF_open_daily_ref_0_PRIV      [TA_REF_DAILY_NB_BARS];
extern TA_Real      TA_SREF_high_daily_ref_0_PRIV      [TA_REF_DAILY_NB_BARS];
extern TA_Real      TA_SREF_low_daily_ref_0_PRIV       [TA_REF_DAILY_NB_BARS];
extern TA_Real      TA_SREF_close_daily_ref_0_PRIV     [TA_REF_DAILY_NB_BARS];
extern TA_Integer   TA_SREF_volume_daily_ref_0_PRIV    [TA_REF_DAILY_NB_BARS];

#define TA_REF_INTRA_NB_BARS 33 /* from ta_intra_ref_0.c */
extern TA_Timestamp TA_SREF_timestamp_intra_ref_0_PRIV [TA_REF_INTRA_NB_BARS];
extern TA_Real      TA_SREF_open_intra_ref_0_PRIV      [TA_REF_INTRA_NB_BARS];
extern TA_Real      TA_SREF_high_intra_ref_0_PRIV      [TA_REF_INTRA_NB_BARS];
extern TA_Real      TA_SREF_low_intra_ref_0_PRIV       [TA_REF_INTRA_NB_BARS];
extern TA_Real      TA_SREF_close_intra_ref_0_PRIV     [TA_REF_INTRA_NB_BARS];

extern const char TA_MRG_0_DATA_PRIV[1000]; /* from ta_mrg_0.c */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   /* Put everything in the Data source handle.
    * This is slightly a kludge approach... 
    * but it will do since there a fix and limited
    * amount of category and symbol to handle for
    * this data source.
    */
   unsigned int categoryIter;
   unsigned int catRefIter;
   unsigned int catMrgIter;
   TA_String *ta_sim_ref_cat;
   TA_String *ta_sim_mrg_cat;
   TA_String *daily_ref_0;
   TA_String *intra_ref_0;
   TA_String *mrg_0;

   unsigned int mrgAllocated; /* boolean */
   unsigned int mrgInstance;  /* 1,2,3,4 */
   TA_History   mrgHistory;
   TA_SourceFlag flags;

} TA_PrivateHandle;

/**** Local functions declarations.    ****/
static void freePrivateHandle( TA_PrivateHandle *privData );
static TA_RetCode addSimMrgData( TA_PrivateHandle *privData,
                                 TA_ParamForAddData  *paramForAddData );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_SIMULATOR_InitializeSourceDriver( void )
{
   TA_PROLOG

   TA_TRACE_BEGIN(  TA_SIMULATOR_InitializeSourceDriver );

   /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_ShutdownSourceDriver( void )
{
   TA_PROLOG

   TA_TRACE_BEGIN(  TA_SIMULATOR_ShutdownSourceDriver );

   /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetParameters( TA_DataSourceParameters *param )
{
   TA_PROLOG

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetParameters );
   memset( param, 0, sizeof( TA_DataSourceParameters ) );

   /* All parameters are zero */
   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_SIMULATOR_OpenSource( const TA_AddDataSourceParamPriv *param,
                                    TA_DataSourceHandle **handle )
{
   TA_PROLOG
   TA_DataSourceHandle *tmpHandle;
   TA_PrivateHandle *privData;
   TA_StringCache *stringCache;

   *handle = NULL;

   TA_TRACE_BEGIN(  TA_SIMULATOR_OpenSource );

   stringCache = TA_GetGlobalStringCache();

   if( !stringCache )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(91) );
   }

   /* Allocate and initialize the handle. This function will also allocate the
    * private handle (opaque data).
    */
   tmpHandle = (TA_DataSourceHandle *)TA_Malloc( sizeof( TA_DataSourceHandle ) );

   if( tmpHandle == NULL )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( tmpHandle, 0, sizeof( TA_DataSourceHandle ) );

   privData = (TA_PrivateHandle *)TA_Malloc( sizeof( TA_PrivateHandle ) );
   if( !privData )
   {
      TA_Free(  tmpHandle );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( privData, 0, sizeof( TA_PrivateHandle ) );

   tmpHandle->opaqueData = privData;

   /* Copy some parameters in the private handle. */
   privData->categoryIter = 0;   
   privData->catRefIter   = 0;
   privData->catMrgIter   = 0;

   if( param->info == NULL )
   {
      privData->mrgInstance = 0;
      tmpHandle->nbCategory = 1;
   }
   else
   {
      privData->mrgInstance = atoi( TA_StringToChar( param->info ) );

      if( (privData->mrgInstance < 1) || (privData->mrgInstance > 4) )
      {
         TA_Free(  tmpHandle );
         TA_Free(  privData );
         TA_TRACE_RETURN( TA_BAD_PARAM );
      }

      tmpHandle->nbCategory = 2;

      /* Allocate the data. */
      
   }

   /* Pre-allocate all the string used in this data source. */
   privData->ta_sim_ref_cat = TA_StringAlloc(stringCache, "TA_SIM_REF");
   privData->ta_sim_mrg_cat = TA_StringAlloc(stringCache, "TA_SIM_MRG");
   privData->daily_ref_0    = TA_StringAlloc(stringCache, "DAILY_REF_0");
   privData->intra_ref_0    = TA_StringAlloc(stringCache, "INTRA_REF_0");
   privData->mrg_0          = TA_StringAlloc(stringCache, "MRG_0");

   if( !privData->ta_sim_ref_cat ||
       !privData->ta_sim_mrg_cat ||
       !privData->daily_ref_0 ||
       !privData->intra_ref_0 ||
       !privData->mrg_0 )
   {
      freePrivateHandle( privData );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   privData->flags = param->flags;

   /* Everything is fine, return the handle to the caller. */
   *handle = tmpHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_CloseSource( TA_DataSourceHandle *handle )
{
   TA_PROLOG

   TA_PrivateHandle *privData;

   TA_TRACE_BEGIN(  TA_SIMULATOR_CloseSource );

   /* Free all ressource used by this handle. */
   if( handle )
   {
      privData = (TA_PrivateHandle *)handle->opaqueData;

      if( privData )
         freePrivateHandle( privData );

      TA_Free(  handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                                TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG

   TA_PrivateHandle *privData;

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetFirstCategoryHandle );

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(92) );
   }

   categoryHandle->nbSymbol = 2;
   categoryHandle->string = privData->ta_sim_ref_cat;
   categoryHandle->opaqueData = 0;

   privData->categoryIter = 1;   

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetNextCategoryHandle( TA_DataSourceHandle *handle,
                                               TA_CategoryHandle   *categoryHandle,
                                               unsigned int index )
{
   TA_PROLOG
   TA_PrivateHandle *privData;

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetNextCategoryHandle );

   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(93) );
   }

   /* No category left. */
   if( privData->categoryIter >= 2 )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the categoryHandle. */
   categoryHandle->string = privData->ta_sim_mrg_cat;
   categoryHandle->nbSymbol = 1;
   categoryHandle->opaqueData = (void *)1;

   privData->categoryIter++;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                              TA_CategoryHandle   *categoryHandle,
                                              TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG
   TA_PrivateHandle *privData;

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetFirstSymbolHandle );

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateHandle *)(handle->opaqueData);

   if( !privData || !categoryHandle->string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(94) );
   }

   /* Get the first symbol in this category. */
   switch( (unsigned long)categoryHandle->opaqueData )
   {
   case 0: /* This is TA_SIM_REF */
      privData->catRefIter = 1;
      symbolHandle->string = privData->daily_ref_0;
      symbolHandle->opaqueData = (void *)0;      
      break;
   case 1: /* This is TA_SIM_MRG */
      privData->catMrgIter = 1;
      symbolHandle->string = privData->mrg_0;
      symbolHandle->opaqueData = (void *)0;      
      break;
   default:
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(95) );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                             TA_CategoryHandle   *categoryHandle,
                                             TA_SymbolHandle     *symbolHandle,
                                             unsigned int index )
{
   TA_PROLOG
   TA_PrivateHandle *privData;

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetNextSymbolHandle );

   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(96) );
   }
   /* Get the first symbol in this category. */
   switch( (unsigned long)categoryHandle->opaqueData )
   {
   case 0: /* This is TA_SIM_REF */
      if( privData->catRefIter == 1 )
      {
         symbolHandle->string = privData->intra_ref_0;
         symbolHandle->opaqueData = (void *)1;
         privData->catRefIter = 2;
      }
      else
      {
         TA_TRACE_RETURN( TA_END_OF_INDEX );
      }
      break;
   case 1: /* This is TA_SIM_MRG */
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   default:
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(97) );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_SIMULATOR_GetHistoryData( TA_DataSourceHandle *handle,
                                        TA_CategoryHandle   *categoryHandle,
                                        TA_SymbolHandle     *symbolHandle,
                                        TA_Period            period,
                                        const TA_Timestamp  *start,
                                        const TA_Timestamp  *end,
                                        TA_Field             fieldToAlloc,
                                        TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG
   TA_PrivateHandle *privateHandle;
   TA_RetCode retCode;
   TA_Timestamp *timestamp;
   TA_Real *open, *high, *low, *close;
   TA_Integer *volume;
   unsigned int i;

   (void)fieldToAlloc;
   (void)end;
   (void)start;
   (void)period;

   TA_TRACE_BEGIN(  TA_SIMULATOR_GetHistoryData );

   TA_ASSERT( handle != NULL );

   privateHandle = (TA_PrivateHandle *)handle->opaqueData;
   TA_ASSERT( privateHandle != NULL );
   TA_ASSERT( paramForAddData != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   retCode = TA_INTERNAL_ERROR(98);

   /* Note: start/end index are currently ignored
    *       in this data source.
    */

   /* Identify the category. */
   switch( (unsigned long)categoryHandle->opaqueData )
   {
   case 0: /* This is TA_SIM_REF */
      switch( (unsigned long)symbolHandle->opaqueData )
      {
      case 0: /* DAILY_REF_0 */
          timestamp = (TA_Timestamp *)NULL;
          open = high = low = close = (TA_Real *)NULL;
          volume = (TA_Integer *)NULL;

          #define TA_ALLOC_COPY( varName, varType, varSize) { \
             varName = TA_Malloc( sizeof( varType ) * varSize ); \
             if( !varName ) \
             { \
                FREE_IF_NOT_NULL( open         ); \
                FREE_IF_NOT_NULL( high         ); \
                FREE_IF_NOT_NULL( low          ); \
                FREE_IF_NOT_NULL( close        ); \
                FREE_IF_NOT_NULL( volume       ); \
                TA_TRACE_RETURN( TA_ALLOC_ERR ); \
             } \
             memcpy( varName, TA_SREF_##varName##_daily_ref_0_PRIV, sizeof( varType )*varSize ); }

             TA_ALLOC_COPY( open,      TA_Real,      TA_REF_DAILY_NB_BARS );
             TA_ALLOC_COPY( high,      TA_Real,      TA_REF_DAILY_NB_BARS );
             TA_ALLOC_COPY( low,       TA_Real,      TA_REF_DAILY_NB_BARS );
             TA_ALLOC_COPY( close,     TA_Real,      TA_REF_DAILY_NB_BARS );
             TA_ALLOC_COPY( volume,    TA_Integer,   TA_REF_DAILY_NB_BARS );         
         #undef TA_ALLOC_COPY

         /* Set the timestamp. */
         timestamp = TA_Malloc( sizeof( TA_Timestamp ) * TA_REF_DAILY_NB_BARS );
         if( !timestamp )
         {
            TA_Free(  open   );
            TA_Free(  high   );
            TA_Free(  low    );
            TA_Free(  close  );
            TA_Free(  volume );
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }

         for( i=0; i < TA_REF_DAILY_NB_BARS; i++ )
            TA_TimestampCopy( &timestamp[i], &TA_SREF_timestamp_daily_ref_0_PRIV[i] );

         if( privateHandle->flags & TA_SOURCE_USES_END_OF_PERIOD )
         {
             for( i=0; i < TA_REF_DAILY_NB_BARS; i++ )
                 TA_NextDay( &timestamp[i] );
         }

          retCode = TA_HistoryAddData( paramForAddData,
                                       TA_REF_DAILY_NB_BARS, TA_DAILY,                                      
                                       timestamp,
                                       open, high,                                      
                                       low, close,
                                       volume, NULL );                                      
          break;

      case 1: /* INTRA_REF_0 */
          /* Allocate the rest. */
          timestamp = (TA_Timestamp *)NULL;
          open = high = low = close = (TA_Real *)NULL;

          #define TA_ALLOC_COPY( varName, varType, varSize) { \
                  varName = TA_Malloc( sizeof( varType ) * varSize ); \
                  if( !varName ) \
                  { \
                     TA_Free(  timestamp ); \
                     FREE_IF_NOT_NULL( open      ); \
                     FREE_IF_NOT_NULL( high      ); \
                     FREE_IF_NOT_NULL( low       ); \
                     FREE_IF_NOT_NULL( close     ); \
                     TA_TRACE_RETURN( TA_ALLOC_ERR ); \
                  } \
                  memcpy( varName, TA_SREF_##varName##_daily_ref_0_PRIV, sizeof( varType )*varSize ); }

         TA_ALLOC_COPY( open,      TA_Real,      TA_REF_INTRA_NB_BARS );
         TA_ALLOC_COPY( high,      TA_Real,      TA_REF_INTRA_NB_BARS );
         TA_ALLOC_COPY( low,       TA_Real,      TA_REF_INTRA_NB_BARS );
         TA_ALLOC_COPY( close,     TA_Real,      TA_REF_INTRA_NB_BARS );
         #undef TA_ALLOC_COPY

         /* Set the timestamp. */
         timestamp = (TA_Timestamp *)TA_Malloc( sizeof( TA_Timestamp ) * TA_REF_INTRA_NB_BARS );
         if( !timestamp )
         {
            FREE_IF_NOT_NULL( open  );
            FREE_IF_NOT_NULL( high  );
            FREE_IF_NOT_NULL( low   );
            FREE_IF_NOT_NULL( close );
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }

         for( i=0; i < TA_REF_INTRA_NB_BARS; i++ )
         {
            if( privateHandle->flags & TA_SOURCE_USES_END_OF_PERIOD )
               TA_AddTimeToTimestamp( &timestamp[i], &TA_SREF_timestamp_intra_ref_0_PRIV[i], TA_10MINS );
            else
               TA_TimestampCopy( &timestamp[i], &TA_SREF_timestamp_intra_ref_0_PRIV[i] );
         }

         retCode = TA_HistoryAddData( paramForAddData,
                                      TA_REF_INTRA_NB_BARS, TA_10MINS,
                                      timestamp,
                                      open, high,                                      
                                      low, close,                                      
                                      NULL, NULL );
                                      
         break;
      }
      break;

   case 1: /* This is TA_SIM_MRG */
      retCode = addSimMrgData( privateHandle, 
                               paramForAddData );
      break;
   }

   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/
static void freePrivateHandle( TA_PrivateHandle *privData )
{
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   if( stringCache )
   {
      if( privData->ta_sim_ref_cat )
         TA_StringFree( stringCache, privData->ta_sim_ref_cat );

      if( privData->ta_sim_mrg_cat )
         TA_StringFree( stringCache, privData->ta_sim_mrg_cat );

      if( privData->daily_ref_0 )
         TA_StringFree( stringCache, privData->daily_ref_0 );

      if( privData->intra_ref_0 )
         TA_StringFree( stringCache, privData->intra_ref_0 );

      if( privData->mrg_0 )
         TA_StringFree( stringCache, privData->mrg_0 );
   }

   FREE_IF_NOT_NULL( privData->mrgHistory.open );
   FREE_IF_NOT_NULL( privData->mrgHistory.high );
   FREE_IF_NOT_NULL( privData->mrgHistory.low );
   FREE_IF_NOT_NULL( privData->mrgHistory.close );
   FREE_IF_NOT_NULL( privData->mrgHistory.volume );
   FREE_IF_NOT_NULL( privData->mrgHistory.openInterest );
   FREE_IF_NOT_NULL( privData->mrgHistory.timestamp );

   TA_Free(  privData );    
}


static TA_RetCode addSimMrgData( TA_PrivateHandle *privData,
                                 TA_ParamForAddData  *paramForAddData )
{
   (void)paramForAddData;
   (void)privData;
#if 0
   /* Allocate the data if not already done. */
   if( !privData->mrgAllocated )
   {
      memset( privData->mrgHistory, 0, sizeof( TA_History ) );
      privData->mrgHistory.open         = TA_Malloc( sizeof( TA_Real ) * 1000 ) );
      privData->mrgHistory.high         = TA_Malloc( sizeof( TA_Real ) * 1000 ) );
      privData->mrgHistory.low          = TA_Malloc( sizeof( TA_Real ) * 1000 ) );
      privData->mrgHistory.close        = TA_Malloc( sizeof( TA_Real ) * 1000 ) );
      privData->mrgHistory.volume       = TA_Malloc( sizeof( TA_Integer ) * 1000 ) );
      privData->mrgHistory.openInterest = TA_Malloc( sizeof( TA_Integer ) * 1000 ) );
      privData->mrgHistory.timestamp    = TA_Malloc( sizeof( TA_Timestamp ) * 1000 ) );

      if( !privData->mrgHistory.open         ||
          !privData->mrgHistory.high         ||
          !privData->mrgHistory.low          ||
          !privData->mrgHistory.close        ||
          !privData->mrgHistory.volume       ||
          !privData->mrgHistory.openInterest ||
          !privData->mrgHistory.timestamp )
      {
         FREE_IF_NOT_NULL( privData->mrgHistory.open );
         FREE_IF_NOT_NULL( privData->mrgHistory.high );
         FREE_IF_NOT_NULL( privData->mrgHistory.low );
         FREE_IF_NOT_NULL( privData->mrgHistory.close );
         FREE_IF_NOT_NULL( privData->mrgHistory.volume );
         FREE_IF_NOT_NULL( privData->mrgHistory.openInterest );
         FREE_IF_NOT_NULL( privData->mrgHistory.timestamp );
         return TA_ALLOC_ERR;
      }
   }
#endif

   return TA_SUCCESS;
}
