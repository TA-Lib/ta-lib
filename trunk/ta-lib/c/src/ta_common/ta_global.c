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
 *  112400 MF   First version.
 *
 */

/* Description:
 *   Provides initialization / shutdown functionality for all modules.
 *
 *   Since not all module are used/link in the application,
 *   the ta_common simply provides the mechanism for the module
 *   to optionnaly "register" its initialization/shutdown
 *   function.
 *
 *   This whole mechanism helps the initialization of "module globals"
 *   while actually not using true globals. The fact of not having "true"
 *   globals helps to make TA-LIB re-entrant.
 *
 *   This also allows to clearly define the shutdown sequence.
 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ta_common.h"
#include "ta_magic_nb.h"
#include "ta_system.h"
#include "ta_global.h"
#include "ta_string.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_func.h"

/**** External functions declarations. ****/
extern TA_RetCode TA_TraceInit( void );

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/

/* The entry point for all globals */
TA_LibcPriv theGlobals;
TA_LibcPriv *TA_Globals = &theGlobals;

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
int TA_IsTraceEnabled( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   return TA_Globals->traceEnabled;
}

void TA_TraceEnable( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */
   TA_Globals->traceEnabled = 1;
}

void TA_TraceDisable( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */
   TA_Globals->traceEnabled = 0;
}

TA_StringCache *TA_GetGlobalStringCache( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */
   return TA_Globals->dfltCache;
}

TA_RetCode TA_Initialize( const TA_InitializeParam *param )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */
   TA_RetCode retCode;
   #if !defined( TA_SINGLE_THREAD )
   unsigned int again; /* Boolean */
   unsigned int i;
   #endif

   /* Initialize the "global variable" used to manage the global
    * variables of all other modules...
    */
   memset( TA_Globals, 0, sizeof( TA_LibcPriv ) );
   TA_Globals->magicNb = TA_LIBC_PRIV_MAGIC_NB;

   if( param )
   { 
      if( param->logOutput )
      {
         /* Override someone intention to use stdin!? */
         if( param->logOutput == stdin )
            TA_Globals->stdioFile = stdout;
         else
            TA_Globals->stdioFile = param->logOutput;

         TA_Globals->stdioEnabled = 1;      
      }

      if( param->userLocalDrive )
         TA_Globals->localCachePath = param->userLocalDrive;
   }

   #if !defined( TA_SINGLE_THREAD )
      /* For multithread, allocate all semaphores
       * protecting the module's globals.
       */
      again = 1;
      for( i=0; i < TA_NB_GLOBAL_ID && again; i++ )
      {
         retCode = TA_SemaInit( &(TA_Globals->moduleControl[i].sema), 1 );
         if( retCode != TA_SUCCESS )
            again = 0;
      }

      if( again == 0 )
      {
         /* Clean-up and exit. */
         for( i=0; i < TA_NB_GLOBAL_ID; i++ )
            TA_SemaDestroy( &(TA_Globals->moduleControl[i].sema) );

         memset( TA_Globals, 0, sizeof( TA_LibcPriv ) );

         return TA_INTERNAL_ERROR(4);
      }

   #endif
	  
   /* Force immediatly the initialization of the memory module. 
    * Note: No call to the tracing capability are allowed in TA_MemInit.
    */
   retCode = TA_MemInit( sizeof( TA_LibcPriv ) );

   /* Force immediatly the initialization of the tracing module. */
   if( retCode == TA_SUCCESS )
      retCode = TA_TraceInit();

   if( retCode != TA_SUCCESS )
   {
      /* Clean-up and exit. */
      #if !defined( TA_SINGLE_THREAD )
      for( i=0; i < TA_NB_GLOBAL_ID; i++ )
         TA_SemaDestroy( &(TA_Globals->moduleControl[i].sema) );
      #endif
      memset( TA_Globals, 0, sizeof( TA_LibcPriv ) );

      return TA_INTERNAL_ERROR(5);
   }

   /* Tracing can now be safely used by the memory module until
    * shutdown in TA_Shutdown.
    */
   TA_Globals->traceEnabled = 1;


   /*** At this point, TA_Shutdown can be called to clean-up. ***/


   /* Allocate the default string cache used throughout the library. */
   retCode = TA_StringCacheAlloc( &(TA_Globals->dfltCache) );

   if( retCode != TA_SUCCESS )
   {
      TA_Shutdown();
      return retCode;
   }

   /* Seeds random generator */
   srand( (unsigned)time( NULL ) );

   return TA_SUCCESS;
}

TA_RetCode TA_Shutdown( void )
{
   /* Note: Keep that function simple.
    *       No tracing and no assert.
    */
   const TA_GlobalControl *control;
   unsigned int i;
   TA_RetCode retCode, finalRetCode;

   if( TA_Globals->magicNb != TA_LIBC_PRIV_MAGIC_NB )
      return TA_LIB_NOT_INITIALIZE;

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
         TA_Globals->traceEnabled = 0;

      #if !defined( TA_SINGLE_THREAD )
      if( TA_Globals->moduleControl[i].sema.flags & TA_SEMA_INITIALIZED )
      {
         retCode = TA_SemaWait( &(TA_Globals->moduleControl[i].sema) );
         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
      #endif

         /* Just before shutting down the ta_memory module,
          * free the global string cache.
          */
         if( (i == TA_MEMORY_GLOBAL_ID) && (TA_Globals->dfltCache) )
         {
            retCode = TA_StringCacheFree( TA_Globals->dfltCache );
            if( retCode != TA_SUCCESS )
               finalRetCode = retCode;
         }

         if( TA_Globals->moduleControl[i].initialize )
         {
            control = TA_Globals->moduleControl[i].control;
            if( control && control->shutdown )
            {
               retCode = (*control->shutdown)( TA_Globals->moduleControl[i].global );
               if( retCode != TA_SUCCESS )
                  finalRetCode = retCode;
            }
            TA_Globals->moduleControl[i].initialize = 0;
         }

      #if !defined( TA_SINGLE_THREAD )
         retCode = TA_SemaDestroy( &(TA_Globals->moduleControl[i].sema) );
         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
      }
      #endif
   }

   /* Initialize to all zero to make sure we invalidate that object. */
   memset( TA_Globals, 0, sizeof( TA_LibcPriv ) );

   return finalRetCode;
}

/* This function return a pointer on the global variable for
 * a particular module.
 * If the global variables are NOT initialized, this function will
 * call the corresponding 'init' function for this module.
 */
TA_RetCode TA_GetGlobal( const TA_GlobalControl * const control,
                         void **global )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   TA_GlobalModuleId id;
   TA_RetCode retCode, finalRetCode;
   const TA_GlobalControl * const locControl = control;

   /* Validate parameters */
   if( !control || !global )
      return TA_FATAL_ERR;

   *global = NULL;

   id = control->id;
   if( id >= TA_NB_GLOBAL_ID )
      return TA_FATAL_ERR;

   if( TA_Globals->moduleControl[id].initialize )
   {
      /* This module is already initialized, just return the global. */
      *global = TA_Globals->moduleControl[id].global;
      return TA_SUCCESS;
   }

   /* This module did not yet get its global initialized. Let's do it. */

   /* Will change if anything goes wrong in the following critical section. */
   finalRetCode = TA_SUCCESS;

   #if !defined( TA_SINGLE_THREAD )
   if( !(TA_Globals->moduleControl[id].sema.flags&TA_SEMA_INITIALIZED) )
      return TA_FATAL_ERR;

   retCode = TA_SemaWait( &(TA_Globals->moduleControl[id].sema) );
   if( retCode != TA_SUCCESS )
      return TA_FATAL_ERR;

   /* Check again if initialize AFTER we got the semaphore. */
   if( !TA_Globals->moduleControl[id].initialize )
   {
   #endif

      /* The module needs to be initialized. Call the corresponding
       * 'init' function.
       */
      if( !TA_Globals->moduleControl[id].control )
         TA_Globals->moduleControl[id].control = locControl;         

      if( locControl->init )
      {
         retCode = (*locControl->init)( &(TA_Globals->moduleControl[id].global) );

         if( retCode != TA_SUCCESS )
            finalRetCode = retCode;
         else
            TA_Globals->moduleControl[id].initialize = 1;
      }

   #if !defined( TA_SINGLE_THREAD )
   }
   retCode = TA_SemaPost( &(TA_Globals->moduleControl[id].sema) );
   if( retCode != TA_SUCCESS )
      finalRetCode = TA_FATAL_ERR;
   #endif

   /* Verify if an error occured inside the critical section. */
   if( finalRetCode != TA_SUCCESS )
      return finalRetCode;

   if( TA_Globals->moduleControl[id].initialize )
   {
      /* Ok, everything is now alloc/initialized at this point, simply
       * return the pointer on the "global variable" for this module.
       */
      *global = TA_Globals->moduleControl[id].global;
   }
   else
      return TA_FATAL_ERR;

   return TA_SUCCESS;
}

FILE *TA_GetStdioFilePtr( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */

   if( TA_Globals->stdioEnabled )
      return TA_Globals->stdioFile;

   return NULL;
}

const char *TA_GetLocalCachePath( void )
{
   /* Note: Keep that function simple.
    *       No tracing, no stdio and no assert.
    */
   if( TA_Globals->localCachePath )
      return TA_Globals->localCachePath;

   return NULL;
}

/**** Local functions definitions.     ****/
/* None */


