#ifndef TA_SYSTEM_H
#define TA_SYSTEM_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_STREAM_H
   #include "ta_stream.h"
#endif

#ifdef WIN32
   #include <process.h>
#endif

#include <stdio.h>
#include <stdarg.h>

/* This files defined the interface to functions that must be adapted
 * to the particular OS on which the library is running.
 *
 * This allows TA-LIB to make abstraction of the Operating System when
 * using files, directory, thread, semaphore etc...
 *
 * (There is no intent to make that abstraction looks like a "Swiss knife".
 *  Only the functionality needed in the context of the TA-LIB is going to
 *  be implemented).
 */

/* Get the list of files/directories in a certain directory.
 *
 * The pattern can include wildcard used by this filesystem.
 */
typedef struct
{
   unsigned int nbFile;
   unsigned int nbDirectory;
   TA_List *listOfDirectory; /* List of TA_String. */
   TA_List *listOfFile;      /* List of TA_String. */
} TA_Directory;

TA_RetCode TA_DirectoryAlloc( const char *path,
                              TA_Directory **directory );

TA_RetCode TA_DirectoryFree( TA_Directory *directory );

/* Identify if the specified character is a seperator
 * used to seperates the directories in a path.
 *
 * Return 1 if this is a seperator character, else 0.
 */
int TA_IsSeparatorChar( int c );

/* Return the wild characters used by the file system. */
int TA_WildCharASCII( void );
int TA_WildASCII( void );

/* Return separator used to build path. */
int TA_SeparatorASCII( void );

/* REplace the separator depending of the platform. */
void TA_AdjustPath( char *path );

/* Return some information system specific. */
int TA_IsFileSystemCaseSensitive( void );
int TA_NbProcessor( void );

/* When an operation fails, the operating system may provides additional
 * information on the reason of the failure of the most recent OS call.
 * This information can be extracted by calling TA_GetLastError().
 * Note: the error code can be misleading if you are in a multithread
 *       situation.
 */
int TA_GetLastError( void );

/* Simplified thread functionality.
 *   A thread is created and started by calling TA_ThreadExec.
 *   The function pointed by 'newThreadEntryPoint' can simply return
 *   when execution is completed. Cannot be more simple than that...
 *
 *  Example:
 *     void printThread( void *string )
 *     {
 *        unsigned int i;
 *        for( i=0; i < 10; i++ )
 *        {
 *           TA_Sleep( rand() % 2 );
 *           puts( (char *)string );
 *        }
 *
 *     }
 *
 *     {
 *        ....
 *        TA_ThreadExec( printThread, (void *)"Thread #1" );
 *        TA_ThreadExec( printThread, (void *)"Thread #2" );
 *        TA_ThreadExec( printThread, (void *)"Thread #3" );
 *        ...
 *     }
 *
 */
#if defined( WIN32 )
   typedef void (*TA_ThreadFunction)( void *args );
#else
   typedef void *(*TA_ThreadFunction)( void *args );
#endif

TA_RetCode TA_ThreadExec( TA_ThreadFunction newThreadEntryPoint, void *args );
TA_RetCode TA_ThreadExit( void );

/* Sleep the current thread (in seconds).
 * A value of zero causes the thread to relinquish the remainder of its
 * time slice to any other thread of equal priority that is ready to run.
 */
void TA_Sleep( unsigned int seconds );

/* Support for counting semaphore. */

#if !defined( TA_SINGLE_THREAD )

/* Bitmap for TA_Sema flags */
#define TA_SEMA_INITIALIZED 0x00000001

#if defined( WIN32 )
   #include <windows.h>
   typedef struct {
      HANDLE theSema;
      unsigned int flags; /* 1 when initialized. */
   } TA_Sema;
#else
   #include <pthread.h>
   #include <semaphore.h>
   typedef struct {
      sem_t theSema;
      unsigned int flags; /* 1 when initialized. */
   } TA_Sema;
#endif

TA_RetCode TA_SemaInit( TA_Sema *sema, unsigned int initialValue );
TA_RetCode TA_SemaDestroy( TA_Sema *sema );

TA_RetCode TA_SemaWait( TA_Sema *sema );
TA_RetCode TA_SemaPost( TA_Sema *sema );

TA_RetCode TA_SemaInc( TA_Sema *sema, unsigned int *prevCount );
TA_RetCode TA_SemaDec( TA_Sema *sema );

#endif

/* Simple support for fast sequential read only access of files.
 *
 * These functions attempts to be memory and speed efficient
 * depending of the operating system.
 *
 * TA_FileSeqOpen : Return TA_Success and set the fileHandle on success.
 *
 * TA_FileSeqRead: Return a pointer on the data read. The number of byte is
 *             returned through the parameter 'nbByteRead'. Return NULL
 *             when there is no further byte to read.
 *             The returned pointer is valid only until the next call to
 *             TA_FileSeqRead. If the data is still needed beyond that
 *             tiemframe, you must proceed to a local copy.
 *
 * TA_FileSeqClose: Free ressources that were allocated for this file handle.
 *
 * To avoid memory allocation, it is assumed that "path" is a valid ptr until
 * the file is closed.
 */
typedef unsigned int TA_FileHandle; /* Hidden implementation. */

TA_RetCode TA_FileSeqOpen( const char *path, TA_FileHandle **handle );
const char *TA_FileSeqRead( TA_FileHandle *handle, unsigned int *nbByteRead );
TA_RetCode TA_FileSeqClose( TA_FileHandle *handle );
unsigned int TA_FileSize( TA_FileHandle *handle );

/* Like TA_FileSeqOpen, but work with a stream instead. Following this TA_FileSeqRead,
 * TA_FileSeqClose and TA_FileSize can be called as if it was a real file.
 *
 * This is a speed efficient way to access a stream sequentially (because the
 * data is returned in "chunks" and often a stream will be compose of just a
 * few large block).
 */
TA_RetCode TA_FileSeqOpenFromStream( TA_Stream *stream, TA_FileHandle **handle );

/* Equivalent to printf, but allows to output to a buffer and/or a FILE. 
 * The function will stop to write in the buffer once fill up.
 *
 * Example 1:  output to stdout and a buffer
 *     TA_PrintfVar outp;
 *     memset( outp, 0, sizeof(outp) );
 *     outp.file   = stdio; 
 *     outp.buffer = buffer; 
 *     outp.size   = sizeof(buffer); 
 *     TA_Printf( outp, ... );
 *
 * Example 2:  output to a buffer only
 *     TA_PrintfVar outp;
 *     memset( outp, 0, sizeof(outp) );
 *     outp.buffer = buffer; 
 *     outp.size   = sizeof(buffer); 
 *     TA_Printf( outp, ... );
 *
 * Example 3:  output to a file only
 *     TA_PrintfVar outp;
 *     memset( outp, 0, sizeof(outp) );
 *     outp = fopen( ... );
 *     TA_Printf( outp, ... );
 *
 */
typedef struct
{
   FILE *file;
   char *buffer;
   int   size;
} TA_PrintfVar;

void TA_Printf( TA_PrintfVar *outp, char *format, ... );

#endif

