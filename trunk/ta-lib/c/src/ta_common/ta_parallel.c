/* TA-LIB Copyright (c) 1999-2000, Mario Fortier
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
 *  111800 MF   First version.
 *
 */

/* Description:
 *     Basic support for local parallelism. Most of the work is done with
 *     macros in 'ta_parallel.h'.
 *
 *     For the time being, the only type of parallelism supported is for
 *     multiprocessor (SMP system).
 */

/**** Headers ****/
#include <stddef.h>
#include <stdio.h>
#include "ta_parallel.h"
#include "ta_system.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   TA_ThreadFunction newThread;
   void *newThreadArgs;
   TA_BarrierSync *bs;
   TA_Sema threadGotParameters;
   TA_Libc *libHandle;
} TA_ThreadFrameParam;

/**** Local functions declarations.    ****/
void threadFrame( void *args );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_BarrierSyncInit( TA_Libc *libHandle, TA_BarrierSync *bs )
{
   TA_RetCode retCode;

   if( libHandle == NULL )
      return TA_BAD_PARAM;

   TA_ASSERT( libHandle, bs != NULL );

   bs->libHandle = libHandle;
   bs->nbThread = 0;

   /* Initialize the barrier to an unblock state. */
   retCode = TA_SemaInit( &(bs->barrierSema), 1 );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, bs );
      return retCode;
   }

   /* Initialize mutex (unblock state). */
   retCode = TA_SemaInit( &(bs->mutexSema), 1 );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, bs );
      return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_BarrierSyncDestroy( TA_BarrierSync *bs )
{
   TA_RetCode retCode;
   TA_Libc *libHandle;

   if( bs != NULL )
      return TA_BAD_PARAM;

   libHandle = bs->libHandle;

   /* Just to be on the safe side, make sure there is no
    * thread left to be joined.
    */
   if( bs->nbThread > 0 )
   {
      retCode = TA_BarrierSyncWaitAllDone( bs );
      if( retCode != TA_SUCCESS )
      {
         TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
         return retCode;
      }
   }

   /* Make sure no one is left in the 'bs' critical section. */
   retCode = TA_SemaWait( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }
   retCode = TA_SemaPost( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   /* At this point it is safe to destroy all the 'bs' semaphores. */
   retCode = TA_SemaDestroy( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   retCode = TA_SemaDestroy( &(bs->barrierSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_BarrierSyncThreadAdd( TA_BarrierSync *bs )
{
   TA_RetCode retCode;
   unsigned int temp;
   TA_Libc *libHandle;

   if( bs != NULL )
      return TA_BAD_PARAM;

   libHandle = bs->libHandle;

   retCode = TA_SemaWait( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   /*** Begin critical section. ***/
   temp = ++(bs->nbThread);
   if( temp == 1 )
   {
      /* First thread being launch, change the state of the barrier to block. */
      retCode = TA_SemaDec( &(bs->barrierSema) );
      if( retCode != TA_SUCCESS )
      {
         TA_SemaPost( &(bs->mutexSema) );
         TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
         return retCode;
      }
   }
   /*** End critical section. ***/

   retCode = TA_SemaPost( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_BarrierSyncThreadDone( TA_BarrierSync *bs )
{
   TA_RetCode retCode;
   unsigned int temp;
   TA_Libc *libHandle;

   if( bs != NULL )
      return TA_BAD_PARAM;

   libHandle = bs->libHandle;

   retCode = TA_SemaWait( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   /*** Begin critical section. ***/
   TA_ASSERT( libHandle, bs->nbThread >= 1 );
   temp = --(bs->nbThread);
   if( temp == 0 )
   {
      /* Last thread being done, change the state of the barrier to unblock. */
      retCode = TA_SemaInc( &(bs->barrierSema), NULL );
      if( retCode != TA_SUCCESS )
      {
         TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
         return retCode;
      }
   }

   /*** End critical section. ***/

   retCode = TA_SemaPost( &(bs->mutexSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   return TA_SUCCESS;
}


TA_RetCode TA_BarrierSyncWaitAllDone( TA_BarrierSync *bs )
{
   TA_RetCode retCode;
   TA_Libc *libHandle;
   
   if( bs != NULL )
      return TA_BAD_PARAM;

   libHandle = bs->libHandle;

   retCode = TA_SemaDec( &(bs->barrierSema) );
   if( retCode != TA_SUCCESS )
   {
      TA_FATAL( libHandle, NULL, retCode, TA_GetLastError(libHandle) );
      return retCode;
   }

   return TA_SUCCESS;
}


void TA_PAR_EXEC_F( TA_BarrierSync *bs, TA_ThreadFunction newThread, void *args )
{
   TA_RetCode retCode;
   TA_Sema threadGotParameters;
   TA_ThreadFrameParam threadParam;
   TA_Libc *libHandle;

   if( bs != NULL )
      return;

   libHandle = bs->libHandle;

   TA_BarrierSyncThreadAdd( bs );

   /* Initialize semaphore in block state. */
   retCode = TA_SemaInit( &threadGotParameters, 0 );

   if( retCode != TA_SUCCESS )
      return;

   /* For safe multi-thread execution, and to avoid memory allocation,
    * the main thread will be block until the new thread have a chance
    * to run and make a copy of the 'args' pointer.
    * 
    * All this synchronization and copy is done within the threadFrame.
    * Consequently, the newThread does not have to bother about all this.
    */
   threadParam.newThread           = newThread;
   threadParam.newThreadArgs       = args;
   threadParam.bs                  = bs;
   threadParam.threadGotParameters = threadGotParameters;

   /* Call the threadFrame function who is going to take care of
    * all the coordination for a safe execution of the newThread.
    */
   retCode = TA_ThreadExec( threadFrame, &threadParam );

   if( retCode != TA_SUCCESS )
      return;

   /* Semaphore will get posted when the threadFrame made a local copy of the
    * parameters.
    */
   retCode = TA_SemaWait( &threadGotParameters );
   if( retCode != TA_SUCCESS )
      return;

   retCode = TA_SemaDestroy( &threadGotParameters );
   if( retCode != TA_SUCCESS )
      return;
}


/**** Local functions definitions.     ****/
void threadFrame( void *args )
{
   TA_RetCode retCode;
   TA_ThreadFunction newThread;
   void *newThreadArgs;
   TA_BarrierSync *bs;
   TA_ThreadFrameParam *param;
   TA_Sema threadGotParameters;
   TA_Libc *libHandle;

   /* This code is the new thread. */

   /* This threadFrame take care of the code 
    * surrounding the user provided entry and
    * exit point.
    */

   /* Get a pointer on the argument being
    * pass to the user entry point.
    * Once a copy of the args is done,
    * signal to the main thread that
    * it can release the 'param' structure.
    */
   param = (TA_ThreadFrameParam *)args;
   newThread = param->newThread;
   newThreadArgs = param->newThreadArgs;
   bs = param->bs;
   threadGotParameters = param->threadGotParameters;

   retCode = TA_SemaPost( &threadGotParameters );
   if( retCode != TA_SUCCESS )
   {
      #if defined( WIN32 )      
         _endthread();
      #else
         pthread_exit();
      #endif

      return;
   }

   /* Call the user provided "thread" entry point. */
   (*newThread)(newThreadArgs);

   TA_BarrierSyncThreadDone( bs );

   #if defined( WIN32 )      
      _endthread();
   #else
      pthread_exit();
   #endif
}

