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
 *   Implements logging and tracing capability.
 *   See header file for details on how to use that module.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include "ta_global.h"
#include "ta_trace.h"
#include "ta_libc.h"
#include "ta_list.h"
#include "ta_dict.h"
#include "ta_memory.h"
#include "ta_system.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/

#if defined(TA_DEBUG)
   /* Size of circular buffer for code trace. */
   #define TA_CODE_TRACE_SIZE   15
#else
   /* Size of circular buffer for code trace. */
   #define TA_CODE_TRACE_SIZE   10
#endif

typedef struct
{
	const char *funcname;
	const char *filename;
	unsigned int lineNb;
	unsigned int repetition; /* Keep count of consecutive passage at this position. */
} TA_TracePosition;

typedef struct
{
   TA_TracePosition position;
	char *str;
	const char *date;
	const char *time;
	unsigned int param1;
	unsigned int param2;
   unsigned int type;
} TA_FatalErrorLog;

typedef struct
{
   #if !defined( TA_SINGLE_THREAD )
      /* If multithread, access to variables related to the
       * call stack and code trace are protected by callSema.
 	    */
      TA_Sema callSema;

      /* If multithread, access to variables related to the
       * logging of the fatal error is protected by fatalSema.
 	    */
      TA_Sema fatalSema;
   #endif

   #ifdef TA_DEBUG
      #if defined( TA_SINGLE_THREAD )	  
	      /* Maintain a call stack using TA_TRACE_BEGIN and TA_TRACE_RETURN.
	       * This functionality makes no sense if multithread.
	       */
	      TA_List *callStack;
      #endif

	   /* Maintain a complete dictionary of the function being called.
	    * Used to evaluate the code coverage and do some profiling.
	    * The element of the dictionary are TA_TracePosition.
       * The key of each entry are the merge of the line number merge with
       * the filename.
       */
	   TA_Dict *functionCalled; 
   #endif

   /* Maintain a code trace by using the TA_TRACE_BEGIN.
	 * Tracing is put in a circular buffer.
	 * When TA_DEBUG is defined, trace is also done for the
	 * TA_TRACE_RETURN and TA_TRACE_CHECKPOINT.
	 */
	TA_TracePosition codeTrace[TA_CODE_TRACE_SIZE];
	unsigned int posForNextTrace;

   /* Log the first occurence of a fatal error. */
	TA_FatalErrorLog fatalError;
   unsigned int fatalErrorRecorded;

	/* User of the library can install its own exception handler
	 * to intercept the fatal error.
	 * Will be NULL if no handler installed.
    */
   TA_FatalHandler userHandler;
} TA_TraceGlobal;


/**** Local functions declarations.    ****/
static void printFatalError( TA_FatalErrorLog *log, FILE *out );
static void internalCheckpoint( TA_TraceGlobal *global,
                                TA_String *key,
                                const char *funcname,
                                const char *filename,
                                unsigned int lineNb );

#ifdef TA_DEBUG
static void freeTracePosition( void *dataToBeFreed );
static TA_TracePosition *newTracePosition( 
                                           const char *funcname, 
                                           const char *filename,
                                           unsigned int lineNb );
#endif

static void printTracePosition( TA_TracePosition *tracePosition, FILE *out );

static TA_RetCode TA_TraceGlobalShutdown( void *globalAllocated );
static TA_RetCode TA_TraceGlobalInit( void **globalToAlloc );

/**** Local variables definitions.     ****/
const TA_GlobalControl TA_TraceGlobalControl =
{
   TA_TRACE_GLOBAL_ID,
   TA_TraceGlobalInit,
   TA_TraceGlobalShutdown
};

/**** Global functions definitions.   ****/
TA_RetCode TA_TraceInit( void )
{
   TA_RetCode retCode;
   TA_TraceGlobal *global;

   /* "Getting" the global will allocate/initialize the global for
    * this module. 
	 * Note: All this will guarantee that TA_TraceGlobalShutdown
	 *       will get called when TA_Shutdown is called by the
	 *       library user.
    */
   retCode = TA_GetGlobal(  &TA_TraceGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

void TA_PrivTraceCheckpoint( const char *funcname,
                             const char *filename,
                             unsigned int lineNb )
{
   TA_TraceGlobal *global;
   TA_RetCode retCode;

   #if defined( TA_DEBUG )
      TA_String *key;
      TA_StringCache *stringCache;
   #endif

   /* Get access to the global. */
   retCode = TA_GetGlobal( &TA_TraceGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return;

   #if !defined( TA_DEBUG )
      internalCheckpoint( global, NULL, funcname, filename, lineNb );
   #else
      /* Build a unique string representing the position. */
      stringCache = TA_GetGlobalStringCache();
      key = TA_StringValueAlloc( stringCache, filename, lineNb );
      if( !key )
         return;

      internalCheckpoint( global, key, funcname, filename, lineNb );

      TA_StringFree( stringCache, key );
   #endif
}

#ifdef TA_DEBUG
TA_RetCode TA_PrivTraceReturn( const char *funcname,
                               const char *filename,
                               unsigned int lineNb,
                               TA_RetCode retCode )
{
   #if defined( TA_SINGLE_THREAD )
   TA_TracePosition *tracePosition;
   TA_TraceGlobal *global;
   TA_RetCode localRetCode;
   #endif

   TA_PrivTraceCheckpoint( funcname, filename, lineNb );

   /* If debugging a single threaded, maintain a calling stack. */
   #if defined( TA_DEBUG ) && defined( TA_SINGLE_THREAD )
      if( !TA_IsTraceEnabled() )
      {
         /* Disable tracing within tracing! */
         TA_TraceDisable();

         /* Get access to the global. */
         localRetCode = TA_GetGlobal(  &TA_TraceGlobalControl, (void **)&global );
         if( localRetCode != TA_SUCCESS )
            return retCode;

         tracePosition = TA_ListRemoveTail( global->callStack );

         if( tracePosition )
         {
            --tracePosition->repetition;
            if( tracePosition->repetition == 0 )
               TA_Free(  tracePosition );
            else
               TA_ListAddTail( global->callStack, tracePosition );
         }

         TA_TraceEnable();
      }
   #endif

	return retCode;
}
#endif

void TA_PrivTraceBegin( const char *funcname,
                        const char *filename, 
                        unsigned int lineNb )
{
   #if defined( TA_DEBUG) && defined( TA_SINGLE_THREAD )
   unsigned int newTracePositionNeeded; /* Boolean */
   TA_TracePosition *tracePosition;
   TA_TraceGlobal *global;
   TA_RetCode retCode;
   #endif

   /* Entry point of a function are "checkpoint" for code
    * coverage.
    */
   TA_PrivTraceCheckpoint( funcname, filename, lineNb );

   /* If debugging a single thread, maintain a call stack. */
   #if defined( TA_DEBUG ) && defined( TA_SINGLE_THREAD )
      if( TA_IsTraceEnabled() )
      {
         /* Disable tracing within tracing! */
         TA_TraceDisable();

         /* Get access to the global. */
         retCode = TA_GetGlobal( &TA_TraceGlobalControl, (void **)&global );
         if( retCode != TA_SUCCESS )
            return;

         tracePosition = TA_ListRemoveTail( global->callStack );
         newTracePositionNeeded = 1;
         if( tracePosition )
         {
             /* Check if last trace in the stack is the same function. */
             if( (tracePosition->filename == filename) &&
                 (tracePosition->funcname == funcname) )
            {
               /* Same fucntion, so just increment the repetition. */
               tracePosition->repetition++;
               newTracePositionNeeded = 0;
            }
            else /* Not the same function, put back this trace. */
               TA_ListAddTail( global->callStack, tracePosition );
         }

         /* New function, so add the trace to the stack. */
         if( newTracePositionNeeded )
         {
            tracePosition = newTracePosition( funcname, filename, lineNb );
            if( tracePosition )      
               TA_ListAddTail( global->callStack, (void *)tracePosition );
         }

         /* Re-enable tracing. */
         TA_TraceDisable();
      }
   #endif
}

void TA_PrivError( unsigned int type, const char *str,
                   const char *filename, const char *date,
                   const char *time, int line,
                   unsigned int j, unsigned int k )
{
   TA_RetCode retCode;
   TA_TraceGlobal *global;
   unsigned int length;

   retCode = TA_GetGlobal( &TA_TraceGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return;

   /* If a fatal error already got handled, return
    * immediatly.
    */
   if( global->fatalErrorRecorded )
      return;

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaWait( &global->callSema );
      if( retCode != TA_SUCCESS )
         return;
   #endif

   /* Double-check inside the critical section. */
   if( global->fatalErrorRecorded )
   {
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaPost( &global->callSema );
      #endif
      return;
   }

   global->fatalErrorRecorded = 1;

   /* Log the fatal error. */
   global->fatalError.position.filename = filename;
   global->fatalError.position.funcname = NULL;
   global->fatalError.position.lineNb = line;
   global->fatalError.position.repetition = 1;

   global->fatalError.str = NULL;
   if( str ) 
   {
      length = strlen( str ) + 1;
      if( length != 0 )
      {
         global->fatalError.str = (char *)TA_Malloc( length );
         memcpy( global->fatalError.str, str, length );
      }
   }

   global->fatalError.date = date;
   global->fatalError.time = time;
   global->fatalError.param1 = j;
   global->fatalError.param2 = k;
   global->fatalError.type = type;

   /* Call the user handler. */
   if( global->userHandler )
      global->userHandler();

   #if !defined( TA_SINGLE_THREAD )
   TA_SemaPost( &global->callSema );
   #endif
}

void TA_FatalReport( FILE *out )
{
   TA_TraceGlobal *global;
   TA_RetCode retCode;
   unsigned int i, pos;
   TA_TracePosition *tracePosition;

   retCode = TA_GetGlobal(  &TA_TraceGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
       return;

   if( global->fatalErrorRecorded )
      printFatalError( &global->fatalError, out );
   else
      fprintf( out, "No fatal error" ); 

   /* Output the calling sequence. */
   fprintf( out, "Execution Sequence:\n" );
   pos = global->posForNextTrace;

   for( i=0; i < TA_CODE_TRACE_SIZE; i++ )
   {    
      tracePosition = &global->codeTrace[pos];
      if( tracePosition && tracePosition->repetition )
         printTracePosition( tracePosition, out );
      pos++;
      if( pos >= TA_CODE_TRACE_SIZE )
         pos = 0;
   }
}


TA_RetCode TA_SetFatalErrorHandler( TA_FatalHandler handler )
{
   TA_TraceGlobal *global;
   TA_RetCode retCode;

   retCode = TA_GetGlobal(  &TA_TraceGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
       return retCode;

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaWait( &global->callSema );
      if( retCode != TA_SUCCESS )
         return retCode;
   #endif

   global->userHandler = handler;

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->callSema );
   #endif

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
static void printFatalError( TA_FatalErrorLog *log, FILE *out )
{
   unsigned int type;
   static char *errorType[] = { "Fatal", "Assert", "Debug Assert", "Warning", "Unknown" };

   type = log->type;

   if( type >= (sizeof(errorType)/sizeof(char *)) )
      type = (sizeof(errorType)/sizeof(char *)) - 1;

   fprintf( out, "*** Internal %s Error ***\n", errorType[type]);
	if( log->position.filename )
	{
       fprintf( out, "File:[%s]\n", log->position.filename );
       fprintf( out, "Line:[%d]    ", log->position.lineNb );
   }

	if( log->date && log->time )
       fprintf( out, "Comp:[%s %s]\n", log->date, log->time );

   if( log->position.funcname )
       fprintf( out, "Func:[%s]\n", log->position.funcname );
	
	if( log->str )
       fprintf( out, "Desc:[%s]\n", log->str );

   fprintf( out, "Info:[0x%08X,0x%08X]\n", log->param1, log->param2 );
}

static TA_RetCode TA_TraceGlobalInit( void **globalToAlloc )
{
   TA_TraceGlobal *global;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( !globalToAlloc )
      return TA_BAD_PARAM;

   *globalToAlloc = NULL;

   global = TA_Malloc( sizeof( TA_TraceGlobal ) );
   if( !global )
      return TA_ALLOC_ERR;

   memset( global, 0, sizeof( TA_TraceGlobal ) );

   #if !defined( TA_SINGLE_THREAD )
      /* Initialize the mutexes in a non-block state. */
      retCode = TA_SemaInit( &global->callSema, 1 );
      if( retCode != TA_SUCCESS )
      {
         TA_Free(  global );
         return retCode;
      }
      retCode = TA_SemaInit( &global->fatalSema, 1 );
      if( retCode != TA_SUCCESS )
      {
         TA_SemaDestroy( &global->callSema );
         TA_Free(  global );
         return retCode;
      }
   #endif

   #ifdef TA_DEBUG   
      #if defined( TA_SINGLE_THREAD )
         /* When single threaded, maintain a calling stack. */      

         global->callStack = TA_ListAlloc();
         if( !global->callStack )
         {
            TA_Free(  global );
            return TA_ALLOC_ERR;
         }
      #endif

      /* All function call and checkpoint are maintained in a dictionary. */
      global->functionCalled = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeTracePosition );
      if( !global->functionCalled )
      {
         #if !defined( TA_SINGLE_THREAD )
            TA_SemaDestroy( &global->callSema );
            TA_SemaDestroy( &global->fatalSema );      
         #else
            TA_ListFree( global->callStack );
         #endif      
         TA_Free(  global );
         return TA_ALLOC_ERR;
      }
   #endif

   /* Success, return the allocated memory to the caller. */
   *globalToAlloc = global;

   return TA_SUCCESS;
}

static TA_RetCode TA_TraceGlobalShutdown( void *globalAllocated )
{
   TA_TraceGlobal *global;
   TA_RetCode retCode = TA_SUCCESS;

   global = (TA_TraceGlobal *)globalAllocated;

   if( !global )
      return retCode;

   if( global->fatalError.str )
      TA_Free(  global->fatalError.str );

   #if !defined( TA_SINGLE_THREAD )      
      retCode = TA_SemaDestroy( &global->fatalSema );
      retCode = TA_SemaDestroy( &global->callSema );
   #endif

   #ifdef TA_DEBUG
      #if defined( TA_SINGLE_THREAD )
         if( global->callStack )
            TA_ListFree( global->callStack );
      #endif

      if( global->functionCalled )
         TA_DictFree( global->functionCalled );
   #endif

   TA_Free(  global );

   return retCode;
}

static void internalCheckpoint( TA_TraceGlobal *global,
                                TA_String  *key,
                                const char *funcname,
                                const char *filename,
                                unsigned int lineNb )
{
   #if !defined( TA_SINGLE_THREAD )      
   TA_RetCode retCode;
   #endif

   #ifdef TA_DEBUG
   TA_TracePosition *tracePosition;
   #endif

   /* Make sure there is no tracing while tracing!
    * In rare occasion, this may prevent to record
    * some tracing in a multithread environment.
    * We can live with that compromise.
    */   
   if( !TA_IsTraceEnabled() )
      return;

   #if !defined( TA_SINGLE_THREAD )                   
      retCode = TA_SemaWait( &global->callSema );
      if( retCode != TA_SUCCESS )
         return;   
   #endif

   #ifdef TA_DEBUG
   /* If this position is already in the dictionary, just
    * increment the 'repetition' counter, else create
    * a new entry in the dictionary.
    */
   TA_TraceDisable();
   tracePosition = TA_DictGetValue_S( global->functionCalled, TA_StringToChar(key) );
   TA_TraceEnable();

   if( tracePosition )
      tracePosition->repetition++;
   else
   {
      tracePosition = newTracePosition( funcname, filename, lineNb );

      if( !tracePosition )
      {
         #if !defined( TA_SINGLE_THREAD )
            TA_SemaPost( &global->callSema );
         #endif
         return;
      }
      TA_TraceDisable();
      TA_DictAddPair_S( global->functionCalled, key, (void *)tracePosition );
      TA_TraceEnable();
   }
   /* Trace position are never deleted, until the library is shutdown.
    * Make a copy of it in the circular buffer.
    */
   global->codeTrace[global->posForNextTrace] = *tracePosition;
   #else
      (void)key; /* Used only when doing debug. */
      global->codeTrace[global->posForNextTrace].filename   = filename;
      global->codeTrace[global->posForNextTrace].funcname   = funcname;
      global->codeTrace[global->posForNextTrace].lineNb     = lineNb;
      global->codeTrace[global->posForNextTrace].repetition = 1;
   #endif

   /* Move to the next entry in the circular buffer. */
   global->posForNextTrace++;
   if( global->posForNextTrace >= TA_CODE_TRACE_SIZE )
      global->posForNextTrace = 0;

   #if !defined( TA_SINGLE_THREAD )
   TA_SemaPost( &global->callSema );
   #endif
}

#ifdef TA_DEBUG
static TA_TracePosition *newTracePosition( const char *funcname, 
                                           const char *filename,
                                           unsigned int lineNb )
{
   TA_TracePosition *tracePosition;

   tracePosition = (TA_TracePosition *)TA_Malloc( sizeof(TA_TracePosition) );
   if( !tracePosition )
       return NULL;

   tracePosition->filename = filename;
   tracePosition->funcname = funcname;
   tracePosition->lineNb = lineNb;
   tracePosition->repetition = 1;

   return tracePosition;
}

static void freeTracePosition( void *dataToBeFreed )
{
   TA_Free(  dataToBeFreed );
}
#endif

static void printTracePosition( TA_TracePosition *tracePosition, FILE *out )
{
   const char *filename;
   int seperatorChar;

   if( !tracePosition )
      return;

   /* Strip off the path. */
   if( !tracePosition->filename )
      filename = NULL;
   else
   {
      seperatorChar = TA_SeparatorASCII();
      filename = strrchr( tracePosition->filename, seperatorChar );
      if( !filename )
         filename = tracePosition->filename;

      if( filename[0] == seperatorChar && seperatorChar != '\0' )
         filename++;
   }

   fprintf( out, "(%03d)Line:[%04d][%s,%s]\n",
              tracePosition->repetition,
              tracePosition->lineNb,
              tracePosition->funcname?tracePosition->funcname:"(null)",
              filename?filename:"(null)" );
}

