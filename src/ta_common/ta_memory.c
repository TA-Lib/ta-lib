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
 *  JC       Jim Schimandle (From a Dr. Dobbs article)
 *  MF       Mario Fortier
 *  JS       Jon Sudul
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  ------------------------------------------------------------------
 *  110199 MF    First version. This code was took from the work
 *               performed by Jim Schimandle (Dr Dobbs August 1990
 *               in the article "ENCAPSULATING C MEMORY ALLOCATION").
 *               Many modifications have been performed (mainly
 *               changing of names). I did add some code for detecting
 *               out-of-bound access to the allocated memory and
 *               for protecting multithread access.
 *  022801 MF    Add libHandle, now allocate "global variables".
 *  012504 MF,JS Allow printf in mem_tag_err only when enabled(Bug#881950)
 */

/*  Memory management utilities
 *
 *  Description
 *
 *   ta_memory.c contains routines to protect the programmer
 *   from errors in calling memory allocation/free routines.
 *   Within this library, the programmer must use the memory calls
 *   defined here instead of the standard malloc/free function.
 *
 *   When these calls are used, the allocation routines in this module
 *   add a data structure to the front of the allocated memory blocks
 *   which tags them as legal memory blocks.
 *
 *   When the free routine is called, the memory block to
 *   be freed is checked for correct tag.  If the block
 *   is not legal, the memory list is dumped to stderr and
 *   the program is terminated.
 *
 *  Compilation Options
 *
 *   TA_MEM_LIST   Link all allocated memory blocks onto
 *         an internal list. The list can be
 *         displayed using TA_MemDisplay().
 *
 *   TA_MEM_WHERE   Save the file/line number of allocated
 *         blocks in the header.
 *         Requires that the compilier supports
 *         __FILE__ and __LINE__ preprocessor
 *         directives.
 *         Also requires that the __FILE__ string
 *         have a static or global scope.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ta_memory.h"
#include "ta_system.h"
#include "ta_global.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
TA_FILE_INFO;

#define TA_MEMTAG    0xDCF4DCF4   /* Value for mh_tag */
#define TA_MEMTAG_H  0xE35AE35A   /* Value for out-of-bound Header  */
#define TA_MEMTAG_T  0xC2D1C2D1   /* Value for out-of-bound Trailer */

typedef struct memnod         /* Memory block header info   */
   {
   unsigned int   mh_tag ;   /* Special ident tag      */
   size_t      mh_size ;   /* Size of allocation block   */
#if defined(TA_MEM_LIST)
   struct memnod   *mh_next ;   /* Next memory block      */
   struct memnod   *mh_prev ;   /* Previous memory block   */
#endif
#if defined(TA_MEM_WHERE)
   char      *mh_file ;   /* File allocation was from   */
   unsigned int   mh_line ;   /* Line allocation was from   */
#endif
   unsigned int outOfBoundInt;
} TA_MEMHDR ;

#define TA_ALIGN_SIZE sizeof(double)
#define TA_HDR_SIZE sizeof(TA_MEMHDR)
#define TA_RESERVE_SIZE (((TA_HDR_SIZE+(TA_ALIGN_SIZE-1))/TA_ALIGN_SIZE) \
                        *TA_ALIGN_SIZE)

#define TA_CLIENT_2_HDR(a) ((TA_MEMHDR *) (((char *) (a)) - TA_RESERVE_SIZE))
#define TA_HDR_2_CLIENT(a) ((void *) (((char *) (a)) + TA_RESERVE_SIZE))

#define TA_MEMTAG_H_PTR(p) (&p->outOfBoundInt)
#define TA_MEMTAG_T_PTR(p) ((unsigned int *)(((char *)p) + TA_RESERVE_SIZE + p->mh_size))

typedef struct
{
   unsigned long   mem_size;   /* Amount of memory used */
   unsigned long   init_mem_size;
   #if !defined( TA_SINGLE_THREAD )
      TA_Sema mutexMemSema;
   #endif
   #if defined(TA_MEM_LIST)
      TA_MEMHDR *memlist;   /* List of memory blocks */
   #endif
} TA_MemoryGlobal;

/**** Local functions declarations.    ****/
static TA_RetCode mem_tag_err( TA_MEMHDR *p, char *fil, int lin);
#if defined(TA_MEM_LIST)
static void mem_list_add( TA_MemoryGlobal *global, TA_MEMHDR *p ); /* Add block to list */
static void mem_list_delete( TA_MemoryGlobal *global, TA_MEMHDR *p );
#define Mem_Tag_Err(a) mem_tag_err((a),fil,lin)
void internalMemDisplay( TA_MemoryGlobal *global, FILE *fp );
#else
#define Mem_Tag_Err(a) mem_tag_err((a),__FILE__,__LINE__)
#endif

static TA_RetCode TA_MemoryGlobalInit    ( void **globalToAlloc );
static TA_RetCode TA_MemoryGlobalShutdown( void *globalAllocated );

/**** Local variables definitions.     ****/
const TA_GlobalControl TA_MemoryGlobalControl =
{
   TA_MEMORY_GLOBAL_ID,
   TA_MemoryGlobalInit,
   TA_MemoryGlobalShutdown
};

/**** Global functions definitions.   ****/
TA_RetCode TA_MemInit( unsigned int memoryAlreadyInUse )
{
   TA_RetCode retCode;
   TA_MemoryGlobal *global;

   /* "Getting" the global will allocate/initialize the global for
    * this module.
    */
   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add the memory who was obtained BEFORE this module got initialized. */
   global->mem_size += memoryAlreadyInUse;

   /* When shutdown will be called, we expect that the memory
    * is back to the initial memory size! Else there is a memory
    * leak.
    */ 
   global->init_mem_size = global->mem_size;

   return TA_SUCCESS;
}

/************************************************************************/
/**** Functions accessed only through macros ****************************/
/************************************************************************/

/*----------------------------------------------------------------------
 *+
 *  TA_PrivAlloc
 *  Allocate a memory block
 *
 *  Usage
 *
 *   void *
 *   TA_PrivAlloc(
 *   size_t      size
 *   )
 *
 *  Parameters
 *
 *   size      Size of block in bytes to allocate
 *
 *  Return Value
 *
 *   Pointer to allocated memory block
 *   NULL if not enough memory
 *
 *  Description
 *
 *   TA_PrivAlloc() makes a protected call to malloc()
 *
 *  Notes
 *
 *   Access this routine using the malloc() macro in mshell.h
 *
 *-
 */

void *
TA_PrivAlloc(
#if defined(TA_MEM_WHERE)
size_t      size,
char      *fil,
int      lin
#else
size_t      size
#endif
)
{
   /* Note: This function must NOT use the tracing functionality from ta_trace.h
	*       unless authorize by TA_IsTraceEnabled.
	*/
   TA_RetCode retCode;
   TA_MEMHDR      *p ;
   TA_MemoryGlobal *global;

   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return NULL;


   /* Allocate memory block */
   /* --------------------- */
   #if !defined( TA_SINGLE_THREAD )
      TA_SemaWait( &global->mutexMemSema );
   #endif
   p = malloc(TA_RESERVE_SIZE + (sizeof(unsigned int)*2) + size ) ;

   if (p == NULL)
   {
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaPost( &global->mutexMemSema );
      #endif
      return NULL ;
   }

   /* Init header */
   /* ----------- */
   p->mh_tag = TA_MEMTAG ;
   p->mh_size = size ;
   #if defined(TA_MEM_WHERE)
      p->mh_file = fil ;
      p->mh_line = lin ;
   #endif

   /* Init header/trailer for detecting out-of-bound access. */
   *TA_MEMTAG_H_PTR(p) = TA_MEMTAG_H;
   *TA_MEMTAG_T_PTR(p) = TA_MEMTAG_T;

   #if defined(TA_MEM_LIST)
      mem_list_add(global,p) ;
   #endif

   global->mem_size += size ;
   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->mutexMemSema );
   #endif

   /* Return pointer to client data */
   /* ----------------------------- */
   return TA_HDR_2_CLIENT(p) ;
}

/*----------------------------------------------------------------------
 *+
 *  TA_PrivRealloc
 *  Reallocate a memory block
 *
 *  Usage
 *
 *   void *
 *   TA_PrivRealloc(
 *   void      *ptr,
 *   size_t       size
 *   )
 *
 *  Parameters
 *
 *   ptr      Pointer to current block
 *   size      Size to adjust block to
 *
 *  Return Value
 *
 *   Pointer to new memory block
 *   NULL if memory cannot be reallocated
 *
 *  Description
 *
 *   TA_PrivRealloc() makes a protected call to realloc().
 *
 *  Notes
 *
 *   Access this routine using the realloc() macro in mshell.h
 *
 *-
 */

void *
TA_PrivRealloc(
#if defined(TA_MEM_WHERE)
void      *ptr,
size_t      size,
char      *fil,
int      lin
#else
void      *ptr,
size_t      size
#endif
)

{
   /* Note: This function must NOT use the tracing functionality from ta_trace.h
	*       unless authorize by TA_IsTraceEnabled.
	*/
   TA_RetCode retCode;
   TA_MEMHDR      *p ;
   TA_MemoryGlobal *global;

   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return NULL;

   /* Convert client pointer to header pointer */
   /* ---------------------------------------- */
   p = TA_CLIENT_2_HDR(ptr) ;

   /* Check for valid block */
   /* --------------------- */
   if (p->mh_tag != TA_MEMTAG)
   {
      Mem_Tag_Err(p) ;
      return NULL ;
   }

   /* Invalidate header */
   /* ----------------- */
   p->mh_tag = ~TA_MEMTAG ;

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaWait( &global->mutexMemSema );
   #endif

   global->mem_size -= p->mh_size ;

   #if defined(TA_MEM_WHERE)
      mem_list_delete(global,p) ;   /* Remove block from list */
   #endif

   /* Reallocate memory block */
   /* ----------------------- */
   p = (TA_MEMHDR *) realloc(p, TA_RESERVE_SIZE + (2*sizeof(unsigned int)) + size);

   if (p == NULL)
   {
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaPost( &global->mutexMemSema );
      #endif
      return NULL ;
   }

   /* Update header */
   /* ------------- */
   p->mh_tag = TA_MEMTAG ;
   p->mh_size = size ;
   #if defined(TA_MEM_LIST)
      p->mh_file = fil ;
      p->mh_line = lin ;
   #endif

   #if defined(TA_MEM_WHERE)
      mem_list_add(global,p) ;   /* Add block to list */
   #endif

   global->mem_size += size ;
   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->mutexMemSema );
   #endif

   /* Init header/trailer for detecting out-of-bound access. */
   *TA_MEMTAG_H_PTR(p) = TA_MEMTAG_H;
   *TA_MEMTAG_T_PTR(p) = TA_MEMTAG_T;

   /* Return pointer to client data */
   /* ----------------------------- */
   return TA_HDR_2_CLIENT(p) ;
}

/*----------------------------------------------------------------------
 *+
 *  TA_PrivStrdup
 *  Save a string in dynamic memory
 *
 *  Usage
 *
 *   char *
 *   TA_PrivStrdup(
 *   char      *str
 *   )
 *
 *  Parameters
 *
 *   str      String to save
 *
 *  Return Value
 *
 *   Pointer to allocated string
 *   NULL if not enough memory
 *
 *  Description
 *
 *   TA_PrivStrdup() saves the specified string in dynamic memory.
 *
 *  Notes
 *
 *   Access this routine using the strdup() macro in mshell.h
 *
 *-
 */

char *
TA_PrivStrdup(
#if defined(TA_MEM_WHERE)
char      *str,
char      *fil,
int      lin
#else
char      *str
#endif
)

{
   /* Note: This function must NOT use the tracing functionality from ta_trace.h
	*       unless authorize by TA_IsTraceEnabled.
	*/
char * s ;

   #if defined(TA_MEM_WHERE)
      s = TA_PrivAlloc( strlen(str)+1, fil, lin) ;
   #else
      s = TA_PrivAlloc( strlen(str)+1) ;
   #endif

   if (s != NULL)
      strcpy(s, str) ;

   return s ;
}

/*----------------------------------------------------------------------
 *+
 *  TA_PrivFree
 *  Free a memory block
 *
 *  Usage
 *
 *   void
 *   TA_PrivFree(
 *   void      *ptr
 *   )
 *
 *  Parameters
 *
 *   ptr      Pointer to memory to free
 *
 *  Return Value
 *
 *   None
 *
 *  Description
 *
 *   TA_PrivFree() frees the specified memory block. The
 *   block must be allocated using TA_PrivAlloc(), TA_PrivRealloc()
 *   or TA_PrivStrdup().
 *
 *  Notes
 *
 *   Access this routine using the free() macro in mshell.h
 *
 *-
 */

void
TA_PrivFree(
#if defined(TA_MEM_WHERE)
void      *ptr,
char      *fil,
int      lin
#else
void      *ptr
#endif
)

{
   /* Note: This function must NOT use the tracing functionality from ta_trace.h
	*       unless authorize by TA_IsTraceEnabled.
	*/
   TA_RetCode retCode;
   TA_MEMHDR       *p ;
   TA_MemoryGlobal *global;
   #if defined( TA_DEBUG )
      unsigned int size;
   #endif

   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return;

#if defined(TA_MEM_WHERE)
   if( (ptr == (TA_MEMHDR *)0x0) || (ptr == (TA_MEMHDR *)ULONG_MAX) )
   {
      if( TA_IsTraceEnabled() )
      {
         TA_FATAL_NO_RET(  "Invalid Memory Block", (unsigned long)ptr, (unsigned int)lin );
      }
	   return;
   }
#endif

   /* Convert client pointer to header pointer */
   /* ---------------------------------------- */
   p = TA_CLIENT_2_HDR(ptr) ;

   /* Check for valid block */
   /* --------------------- */
   if ( (p->mh_tag != TA_MEMTAG) ||
        (*TA_MEMTAG_H_PTR(p) != TA_MEMTAG_H) ||
        (*TA_MEMTAG_T_PTR(p) != TA_MEMTAG_T))
   {
      Mem_Tag_Err(p) ;
      return ;
   }

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaWait( &global->mutexMemSema );
   #endif

   /* Invalidate header */
   /* ----------------- */
   p->mh_tag = ~TA_MEMTAG ;

   global->mem_size -= p->mh_size ;
   #if defined( TA_DEBUG )
      size = TA_RESERVE_SIZE + (sizeof(unsigned int)*2) + p->mh_size;
   #endif

   #if defined(TA_MEM_LIST)
      mem_list_delete(global,p) ;   /* Remove block from list */
   #endif

   #if defined( TA_DEBUG )
      /* Forcing everything to zero may allow
       * to detect usage of already freed memory.
       */
      memset( p, 0, size );
   #endif

   /* Free memory block */
   /* ----------------- */
   free(p) ;

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->mutexMemSema );
   #endif
}

/************************************************************************/
/**** Functions accessed directly ***************************************/
/************************************************************************/

/*----------------------------------------------------------------------
 *+
 *  TA_MemUsed
 *  Return amount of memory currently allocated
 *
 *  Usage
 *
 *   unsigned long
 *   TA_MemUsed(
 *   )
 *
 *  Parameters
 *
 *   None.
 *
 *  Description
 *
 *   TA_MemUsed() returns the number of bytes currently allocated
 *   using the memory management system. The value returned is
 *   simply the sum of the size requests to allocation routines.
 *   It does not reflect any overhead required by the memory
 *   management system.
 *
 *  Notes
 *
 *   None
 *
 *-
 */
unsigned long TA_MemUsed( void )
{
   TA_RetCode retCode;
   TA_MemoryGlobal *global;

   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return 0;

   return global->mem_size;
}

/*----------------------------------------------------------------------
 *+
 *  TA_MemDisplay
 *  Display memory allocation list
 *
 *  Usage
 *
 *   void
 *   TA_MemDisplay(
 *   FILE      *fp
 *   )
 *
 *  Parameters
 *
 *   fp      File to output data to
 *
 *  Description
 *
 *   TA_MemDisplay() displays the contents of the memory
 *   allocation list.
 *
 *   This function is a no-op if TA_MEM_LIST is not defined.
 *
 *  Notes
 *
 *   None
 *
 *-
 */

void TA_MemDisplay( FILE *fp )
{
   /* Note: This function must NOT use the tracing functionality from ta_trace.h
	*       unless authorize by TA_IsTraceEnabled.
	*/
#if defined(TA_MEM_LIST)
   TA_RetCode retCode;
   TA_MemoryGlobal *global;

   retCode = TA_GetGlobal(  &TA_MemoryGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return;

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaWait( &global->mutexMemSema );
   #endif

   internalMemDisplay( global, fp );

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->mutexMemSema );
   #endif
#else
   fprintf(fp, "Memory list not compiled (TA_MEM_LIST not defined)\n") ;
#endif
}

/**** Local functions definitions.     ****/

#if defined(TA_MEM_LIST)
void internalMemDisplay( TA_MemoryGlobal *global, FILE *fp )
{
   
   TA_MEMHDR      *p ;
   int      idx ;

   #if defined(TA_MEM_WHERE)
      fprintf(fp, "Index  Size  Address  File(Line) - total size %lu\n", global->mem_size) ;
   #else
                /* 012345678901234567890 */
      fprintf(fp, "Index  Size  Address  - total size %lu\n", global->mem_size) ;
   #endif

   idx = 0 ;
   p = global->memlist ;
   #define MAX_DISPLAY_LINE 200
   while (p != NULL && (idx < MAX_DISPLAY_LINE) )
   {
      /* Unused? ((const char *)p)+TA_RESERVE_SIZE+(sizeof(unsigned int)*2) */
      fprintf(fp, "%5d%6lu %08lX", idx++, (unsigned long)p->mh_size, (unsigned long)(TA_HDR_2_CLIENT(p)) ) ;
      #if defined(TA_MEM_WHERE)
         fprintf(fp, "  %s(%d)", p->mh_file, p->mh_line) ;
      #endif
      if (p->mh_tag != TA_MEMTAG)
      {
         fprintf(fp, " INVALID") ;
      }
      fprintf(fp, "\n") ;
      p = p->mh_next ;
   }

   if( (p != NULL) && (idx >= MAX_DISPLAY_LINE) )
   {
      fprintf( fp, "Memory dump stopped. Too much alloc to display.\n" );
   }
}
#endif
/************************************************************************/
/**** Memory list manipulation functions ********************************/
/************************************************************************/

/*
 * mem_list_add()
 * Add block to list
 */

#if defined(TA_MEM_LIST)
static void
mem_list_add(
TA_MemoryGlobal *global,
TA_MEMHDR   *p
)
{
   p->mh_next = global->memlist ;
   p->mh_prev = NULL ;
   if (global->memlist != NULL)
   {
      global->memlist->mh_prev = p ;
   }
   global->memlist = p ;
}
#endif

/*----------------------------------------------------------------------*/

/*
 * mem_list_delete()
 * Delete block from list
 */

#if defined(TA_MEM_LIST)
static void mem_list_delete( TA_MemoryGlobal *global, TA_MEMHDR *p )
{

   if (p->mh_next != NULL)
      p->mh_next->mh_prev = p->mh_prev ;

   if (p->mh_prev != NULL)
      p->mh_prev->mh_next = p->mh_next ;
   else
      global->memlist = p->mh_next ;
}
#endif

/************************************************************************/
/**** Error display *****************************************************/
/************************************************************************/

/*
 *  mem_tag_err()
 *  Display memory tag error
 */

static TA_RetCode mem_tag_err(
TA_MEMHDR *p,
char      *fil,
int        lin
)

{
   #if defined(TA_DEBUG) && defined(TA_MEM_LIST)
   FILE *fp;
   #endif

   (void)fil;

   #if defined(TA_DEBUG) && defined(TA_MEM_LIST)
      fp = TA_GetStdioFilePtr();
      if( fp )
      {
         TA_MemDisplay(fp);
      }
   #endif

   if( *TA_MEMTAG_H_PTR(p) != TA_MEMTAG_H )
   {
       TA_FATAL_RET( "Header Out-of-bound memory access", (unsigned long)p, (unsigned long)lin, TA_ALLOC_ERR );
   }
   else if( *TA_MEMTAG_T_PTR(p) != TA_MEMTAG_T)
   {
       TA_FATAL_RET( "Trailer Out-of-bound memory access", (unsigned long)p, (unsigned long)lin, TA_ALLOC_ERR );
   }

   TA_FATAL_RET( "Bad memory allocation memtag", (unsigned long)p, (unsigned long)lin, TA_ALLOC_ERR );
}


static TA_RetCode TA_MemoryGlobalInit( void **globalToAlloc )
{   
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif
   TA_MemoryGlobal *global;

   if( !globalToAlloc )
      return TA_BAD_PARAM;

   *globalToAlloc = NULL;

   global = malloc( sizeof( TA_MemoryGlobal ) );
   if( !global )
      return TA_ALLOC_ERR;

   memset( global, 0, sizeof( TA_MemoryGlobal ) );

   global->mem_size = sizeof( TA_MemoryGlobal );

   #if !defined( TA_SINGLE_THREAD )
      /* Initialize the mutex in a non-block state. */
      retCode = TA_SemaInit( &global->mutexMemSema, 1 );
      if( retCode != TA_SUCCESS )
      {
         free( global );
         return retCode;
      }
   #endif

   /* Success, return the allocated memory to the caller. */
   *globalToAlloc = global;
   return TA_SUCCESS;
}

static TA_RetCode TA_MemoryGlobalShutdown( void *globalAllocated )
{
   TA_MemoryGlobal *global;
   TA_RetCode retCode = TA_SUCCESS;
   #if defined(TA_DEBUG) && defined(TA_MEM_LIST)
   FILE *fp;
   #endif
   global = (TA_MemoryGlobal *)globalAllocated;

   if( !global )
      return retCode;

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaDestroy( &global->mutexMemSema );
   #endif

   /* Check if there is a memory leak/corruption. */
   if( global->mem_size != global->init_mem_size )
   {
      retCode = TA_MEM_LEAK;
   
      #if defined(TA_DEBUG) && defined(TA_MEM_LIST)        
        fp = TA_GetStdioFilePtr();
        if( fp )
        {
          fprintf( fp, "\n*** Memory Leak detected!\n" );
          internalMemDisplay( global, fp );
        }       
      #endif
   }

   free( global );

   return retCode;
}

