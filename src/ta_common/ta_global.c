/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
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
 *  112400 MF   First version.
 *
 */

/* Description:
 *   Provides initialization / shutdown functionality for all modules.
 *
 *   It is also responsible for the global variable of all modules.
 *   The globals are going to be allocated only for the modules
 *   link and used at least once.
 *
 *   This whole mechanism makes sure that there is no "true" global
 *   but only allocated data. The fact of not having "true" global
 *   allows to make the TA-LIB totally re-entrant and easily used
 *   as a shared library.
 *
 *   Another important advantage to centralize the initialization
 *   of the global is to safely keep track and free all ressources
 *   from one point when TA_Shutdown is called.
 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include "ta_common.h"
#include "ta_magic_nb.h"
#include "ta_system.h"
#include "ta_global.h"
#include "ta_string.h"
#include "ta_memory.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
extern TA_RetCode TA_TraceInit( TA_Libc *libHandle );

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/

/* TA_ModuleControl will be allocated for each module. */
typedef struct
{
  #if !defined( TA_SINGLE_THREAD )
  TA_Sema sema;
  #endif
  unsigned int initialize;
  const TA_GlobalControl * control;
  void *global;
} TA_ModuleControl;

/* This is the hidden implementation of TA_Libc. */
typedef struct
{
   unsigned int magicNb; /* Unique identifier of this object. */
   TA_StringCache *dfltCache;
   TA_ModuleControl moduleControl[TA_NB_GLOBAL_ID];

   unsigned int traceEnabled;
   unsigned int stdioEnabled;
   FILE *stdioFile;

   const char *localCachePath;

} TA_LibcPriv;

/**** Global functions definitions.   ****/
int TA_IsTraceEnabled( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   if( !libHandle )
      return 0;

   privLibHandle = (TA_LibcPriv *)libHandle;

   return privLibHandle->traceEnabled;
}

void TA_TraceEnable( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   if( !libHandle )
      return;

   privLibHandle = (TA_LibcPriv *)libHandle;

   privLibHandle->traceEnabled = 1;
}

void TA_TraceDisable( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   if( !libHandle )
      return;

   privLibHandle = (TA_LibcPriv *)libHandle;

   privLibHandle->traceEnabled = 0;
}

TA_StringCache *TA_GetGlobalStringCache( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   /* Strings allocation use a "cache" mechanism. There is
    * one global cache allocated for each instance of the
    * library (for each TA_Libc).
    * This cache is currently shared by all modules.
    */
   if( !libHandle )
      return NULL;

   privLibHandle = (TA_LibcPriv *)libHandle;

   return privLibHandle->dfltCache;
}

TA_RetCode TA_Initialize( TA_Libc **libHandle,
                          const TA_InitializeParam *param )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;
   TA_RetCode retCode;
   #if !defined( TA_SINGLE_THREAD )
   unsigned int again; /* Boolean */
   unsigned int i;
   #endif

   if( !libHandle )
      return TA_BAD_PARAM;

   *libHandle = NULL;

   /* Allocate the "global variable" used to manage the global
    * variables of all other modules...
    */
   privLibHandle = (TA_LibcPriv *)malloc( sizeof( TA_LibcPriv ) );

   if( !privLibHandle )
      return TA_ALLOC_ERR;

   memset( privLibHandle, 0, sizeof( TA_LibcPriv ) );
   privLibHandle->magicNb = TA_LIBC_PRIV_MAGIC_NB;

   if( param )
   { 
      if( param->logOutput )
      {
         /* Override someone intention to use stdin!? */
         if( param->logOutput == stdin )
            privLibHandle->stdioFile = stdout;
         else
            privLibHandle->stdioFile = param->logOutput;

         privLibHandle->stdioEnabled = 1;      
      }

      if( param->userLocalDrive )
         privLibHandle->localCachePath = param->userLocalDrive;
   }

   #if !defined( TA_SINGLE_THREAD )
      /* For multithread, allocate all semaphores
       * protecting the module's globals.
       */
      again = 1;
      for( i=0; i < TA_NB_GLOBAL_ID && again; i++ )
      {
         retCode = TA_SemaInit( &privLibHandle->moduleControl[i].sema, 1 );
         if( retCode != TA_SUCCESS )
            again = 0;
      }

      if( again == 0 )
      {
         /* Clean-up and exit. */
         for( i=0; i < TA_NB_GLOBAL_ID; i++ )
            TA_SemaDestroy( &privLibHandle->moduleControl[i].sema );

         memset( privLibHandle, 0, sizeof( TA_LibcPriv ) );

         free( privLibHandle );
         return TA_UNKNOWN_ERR;
      }

   #endif
	  
   /* Force immediatly the initialization of the memory module. 
    * Note: No call to the tracing capability are allowed in TA_MemInit.
    */
   retCode = TA_MemInit( (TA_Libc *)privLibHandle, sizeof( TA_LibcPriv ) );

   /* Force immediatly the initialization of the tracing module. */
   if( retCode == TA_SUCCESS )
      retCode = TA_TraceInit( (TA_Libc *)privLibHandle );

   if( retCode != TA_SUCCESS )
   {
      /* Clean-up and exit. */
      #if !defined( TA_SINGLE_THREAD )
      for( i=0; i < TA_NB_GLOBAL_ID; i++ )
         TA_SemaDestroy( &privLibHandle->moduleControl[i].sema );
      #endif
      memset( privLibHandle, 0, sizeof( TA_LibcPriv ) );

      free( privLibHandle );
      return TA_UNKNOWN_ERR;
   }

   /* Tracing can now be safely used by the memory module until
    * shutdown in TA_Shutdown.
    */
   privLibHandle->traceEnabled = 1;


   /*** At this point, TA_Shutdown can be called to clean-up. ***/


   /* Allocate the default string cache used throughout the library. */
   retCode = TA_StringCacheAlloc( (TA_Libc *)privLibHandle,
                                  &privLibHandle->dfltCache );

   if( retCode != TA_SUCCESS )
   {
      TA_Shutdown( (TA_Libc *)privLibHandle );
      return retCode;
   }

   /* Success! Return the info to the caller. */
   *libHandle = (TA_Libc *)privLibHandle;

   return TA_SUCCESS;
}

TA_RetCode TA_Shutdown( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;
   const TA_GlobalControl *control;
   unsigned int i;
   TA_RetCode retCode, finalRetCode;

   privLibHandle = (TA_LibcPriv *)libHandle;

   if( !privLibHandle )
      return TA_BAD_PARAM;

   if( privLibHandle->magicNb != TA_LIBC_PRIV_MAGIC_NB )
      return TA_BAD_OBJECT;

   /* Will change if an error occured at any point. */
   finalRetCode = TA_SUCCESS;

   /* Shutdown all the modules who were initialized.
    * Also destroy the corresponding semaphore.
    */
   for( i=0; i < TA_NB_GLOBAL_ID; i++ )
   {
	  /* Disable tracing when starting to shut down
	   * the tracing module. This is to avoid that
	   * the memory module starts do some tracing while we
	   * are shutting down the tracing itself!!!
	   */
	  if( i == TA_TRACE_GLOBAL_ID )
         privLibHandle->traceEnabled = 0;

      #if !defined( TA_SINGLE_THREAD )
      if( privLibHandle->moduleControl[i].sema.flags & TA_SEMA_INITIALIZED )
      {
         retCode = TA_SemaWait( &(privLibHandle->moduleControl[i].sema) );
         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
      #endif

         /* Just before shutting down the ta_memory module,
          * free the global string cache.
          */
         if( (i == TA_MEMORY_GLOBAL_ID) && (privLibHandle->dfltCache) )
         {
            retCode = TA_StringCacheFree( privLibHandle->dfltCache );
            if( retCode != TA_SUCCESS )
               finalRetCode = retCode;
         }

         if( privLibHandle->moduleControl[i].initialize )
         {
            control = privLibHandle->moduleControl[i].control;
            if( control && control->shutdown )
            {
               retCode = (*control->shutdown)( libHandle, privLibHandle->moduleControl[i].global );
               if( retCode != TA_SUCCESS )
                  finalRetCode = retCode;
            }
            privLibHandle->moduleControl[i].initialize = 0;
         }

      #if !defined( TA_SINGLE_THREAD )
         retCode = TA_SemaDestroy( &(privLibHandle->moduleControl[i].sema) );
         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
      }
      #endif
   }

   /* Initialize to all zero to make sure we invalidate that object. */
   memset( privLibHandle, 0, sizeof( TA_LibcPriv ) );

   free( privLibHandle );

   return finalRetCode;
}

/* This function return a pointer on the global variable for
 * a particular module.
 * If the global variables are NOT initialized, this function will
 * call the corresponding 'init' function for this module.
 */
TA_RetCode TA_GetGlobal( TA_Libc *libHandle,
                         const TA_GlobalControl * const control,
                         void **global )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_GlobalModuleId id;
   TA_RetCode retCode, finalRetCode;
   TA_LibcPriv *privLibHandle;
   const TA_GlobalControl * const locControl = control;

   /* Validate parameters */
   if( !libHandle || !control || !global )
      return TA_FATAL_ERR;

   *global = NULL;

   id = control->id;
   if( id >= TA_NB_GLOBAL_ID )
      return TA_FATAL_ERR;

   privLibHandle = (TA_LibcPriv *)libHandle;
   if( privLibHandle->magicNb != TA_LIBC_PRIV_MAGIC_NB )
      return TA_FATAL_ERR;

   if( privLibHandle->moduleControl[id].initialize )
   {
      /* This module is already initialized, just return the global. */
      *global = privLibHandle->moduleControl[id].global;
      return TA_SUCCESS;
   }

   /* This module did not yet get its global initialized. Let's do it. */

   /* Will change if anything happen in the following critical section. */
   finalRetCode = TA_SUCCESS;

   #if !defined( TA_SINGLE_THREAD )
   if( !(privLibHandle->moduleControl[id].sema.flags&TA_SEMA_INITIALIZED) )
      return TA_FATAL_ERR;

   retCode = TA_SemaWait( &(privLibHandle->moduleControl[id].sema) );
   if( retCode != TA_SUCCESS )
      return TA_FATAL_ERR;

   /* Check again if initialize AFTER we got the semaphore. */
   if( !privLibHandle->moduleControl[id].initialize )
   {
   #endif

      /* The module needs to be initialized. Call the corresponding
       * 'init' function.
       */
      if( !privLibHandle->moduleControl[id].control )
         privLibHandle->moduleControl[id].control = locControl;         

      if( locControl->init )
      {
         retCode = (*locControl->init)( libHandle, &privLibHandle->moduleControl[id].global );

         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
         else
            privLibHandle->moduleControl[id].initialize = 1;
      }

   #if !defined( TA_SINGLE_THREAD )
   }
   retCode = TA_SemaPost( &(privLibHandle->moduleControl[id].sema) );
   if( retCode != TA_SUCCESS )
      finalRetCode = TA_FATAL_ERR;
   #endif

   /* Verify if an error occured inside the critical section. */
   if( finalRetCode != TA_SUCCESS )
      return finalRetCode;

   if( privLibHandle->moduleControl[id].initialize )
   {
      /* Ok, everything is now alloc/initialized at this point, simply
       * return the pointer on the "global variable" for this module.
       */
      *global = privLibHandle->moduleControl[id].global;
   }
   else
      return TA_FATAL_ERR;

   return TA_SUCCESS;
}

FILE *TA_GetStdioFilePtr( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   privLibHandle = (TA_LibcPriv *)libHandle;

   if( !privLibHandle )
      return NULL;

   if( privLibHandle->stdioEnabled )
      return privLibHandle->stdioFile;

   return NULL;
}

const char *TA_GetLocalCachePath( TA_Libc *libHandle )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_LibcPriv *privLibHandle;

   privLibHandle = (TA_LibcPriv *)libHandle;

   if( !privLibHandle )
      return NULL;

   if( privLibHandle->localCachePath )
      return privLibHandle->localCachePath;

   return NULL;
}

/**** Local functions definitions.     ****/
/* None */


