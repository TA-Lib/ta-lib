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
 *  110199 MF   First version.
 *
 */

/* Description:
 *    Make abstraction of the operating system. These functions are not
 *    meant to be general purpose. They are written for being used in
 *    the context of the TA-LIB. So let's try to keep it simple.
 *
 *    Note: Because semaphores are used very EARLY in the initialization
 *          of the library, these TA_SemaXXXX function shall not call any
 *          other TA-LIB function to avoid dependency. Keep these simple.
 */

/**** Headers ****/

/* First, define which of the following is desired:
 *
 *   USE_WIN32_API - Optimized by using Win32 API calls.
 *
 *   USE_OSLAYER   - Use a portable operating system abstraction layer.
 *                   Use Posix for thread/semaphore, use ANSI C for file
 *                   access.
 *
 * Notice that in this file, the USE_WIN32_API is preferred to the
 * usual WIN32, the reason being that we wish sometimes to compile
 * with USE_OSLAYER even on a WIN32 platform (for test purpose).
 */
#if !defined( USE_OSLAYER ) && !defined( USE_WIN32_API )
   #if defined( WIN32 )
      #define USE_WIN32_API
   #else
      #define USE_OSLAYER
   #endif
#endif

#if defined( USE_WIN32_API )
   #include <windows.h>
   #include <process.h>
   #include <io.h>
#endif

#include <limits.h>
#include "ta_trace.h"
#include "ta_system.h"
#include "ta_memory.h"
#include "ta_global.h"
#include "ta_string.h"
#include "ta_stream.h"

#if defined( USE_OSLAYER )
   /* iMatix SFL Library is used for providing
    * some portability.
    */
   #include <stdio.h>
   #include "sfl.h"
   #if !defined( WIN32 )
   #include <pthread.h>
   #include <semaphore.h>
   #endif
#endif

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   #if defined( USE_WIN32_API )
   HANDLE        handle;
   LPVOID        allocBuffer;
   DWORD         allocBufferSize;
   #else
   FILE         *handle;
   void         *allocBuffer;
   unsigned int  allocBufferSize;
   #endif

   /* Set only when the data is from 
    * a stream instead of a file.
    */
   TA_StreamAccess *streamAccess;
   TA_Stream *stream;

   /* Keep a copy of the path/filename. */
   const char *path;
} TA_FileHandlePriv;

#define FILE_HANDLE

typedef struct
{
   #if defined( USE_WIN32_API )
   unsigned int sysInfoInitialized;
   SYSTEM_INFO sysInfo;
   #endif

   TA_StringCache *dirnameCache;
   TA_StringCache *filenameCache;
   unsigned int    lastError;
} TA_SystemGlobal;

#if !defined( USE_WIN32_API )
   /* All non-win32 API use a fix buffer size for I/O.
    * Can be fine tune here for your platform.
    */
   #define FILE_BUFFER_SIZE 2048
#endif

#ifndef MAX_PATH
   #define MAX_PATH 1024
#endif

/**** Local functions declarations.    ****/
static void stringListFree( TA_StringCache *stringCache, TA_List *list );

static TA_RetCode TA_SystemGlobalInit    ( TA_Libc *libHandle, void **globalToAlloc );
static TA_RetCode TA_SystemGlobalShutdown( TA_Libc *libHandle, void *globalAllocated );

#if defined( USE_WIN32_API )
static LPVOID allocDiskBuffer( TA_Libc *libHandle, const char *path, DWORD *nbByteAlloc );
static TA_RetCode freeDiskBuffer( TA_Libc *libHandle, LPVOID buffer, DWORD nbByteAlloc );
static void initSysInfo( TA_SystemGlobal *global );
#endif

/**** Local variables definitions.     ****/
TA_FILE_INFO;

const TA_GlobalControl TA_SystemGlobalControl =
{
   TA_SYSTEM_GLOBAL_ID,
   TA_SystemGlobalInit,
   TA_SystemGlobalShutdown
};

/**** Global functions definitions.   ****/
int TA_GetLastError( TA_Libc *libHandle )
{
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   if( !libHandle )
      return 0;

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return 0;

   return global->lastError;
}

int TA_IsSeparatorChar( int c )
{
   if( (c == '\\') || (c == '/') )
      return 1;

   return 0;
}

int TA_SeparatorASCII( void )
{
#if defined( WIN32 )
   return '\\';
#else
   return '/';
#endif
}

int TA_WildCharASCII( void )
{
   return '?';
}

int TA_WildASCII( void )
{
   return '*';
}

TA_RetCode TA_DirectoryAlloc( TA_Libc *libHandle,
                              const char *path,
                              TA_Directory **directory )
{
   #if defined( USE_WIN32_API )
   HANDLE handle;
   WIN32_FIND_DATA data;
   DWORD win32Error;
   #endif

   #if defined( USE_OSLAYER )
   DIRST dirHandle;
   const char *filePattern;
   char *basePath;
   #endif

   unsigned pathLength;

   int findNextRetCode;

   TA_Directory *dir;
   TA_String *string;
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   const char   *entryName;
   unsigned int  entryIsDirectory;

   *directory = NULL;

   if( (path == NULL) || (directory == NULL) || (libHandle == NULL))
      return TA_BAD_PARAM;

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   dir = (TA_Directory *)TA_Malloc( libHandle, sizeof( TA_Directory ) );

   if( dir == NULL )
      return TA_ALLOC_ERR;

   dir->nbFile = 0;
   dir->nbDirectory = 0;

   dir->listOfFile = TA_ListAlloc( libHandle );
   dir->listOfDirectory = TA_ListAlloc( libHandle );

   if( (dir->listOfFile == NULL) || (dir->listOfDirectory == NULL) )
   {
      TA_DirectoryFree( libHandle, dir );
      return TA_ALLOC_ERR;
   }

   /* Verify that the path is valid. */
   pathLength = strlen( path );

   if( (pathLength == 0) || (pathLength >= MAX_PATH) )
   {
      TA_DirectoryFree( libHandle, dir );
      return TA_BAD_PARAM;
   }

   /* Now get the directory from the operating system. */
   #if defined( USE_WIN32_API )

   handle = FindFirstFile( path, &data );
   if( handle == INVALID_HANDLE_VALUE )
   {
      win32Error = GetLastError();
      global->lastError = win32Error;

      if( (win32Error != ERROR_FILE_NOT_FOUND) &&
          (win32Error != ERROR_PATH_NOT_FOUND) )
      {
         TA_DirectoryFree( libHandle, dir );
         return TA_ACCESS_FAILED;
      }

      /* No files or directory... but still have to pass the result
       * to the caller.
       */
      *directory = dir;

      return TA_SUCCESS;
   }

   entryName = data.cFileName;
   entryIsDirectory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
   #endif

   #if defined( USE_OSLAYER )
   /* Split the path into the basePath and the filePattern. */
   basePath = TA_Malloc( libHandle, pathLength+1 ); 
   memcpy( basePath, path, pathLength+1 );
   filePattern = split_path_and_file( libHandle, basePath );

   if( !filePattern )
   {
      /* With no filePattern, no file can be found...
       * so return an empty directory to the caller.
       */
      *directory = dir;
      TA_Free( libHandle, basePath );
      return TA_SUCCESS;
   }

   /* Look for last separetor. */
   if( !open_dir(libHandle,&dirHandle, basePath ) )
   {
      /* Errors, or no files or no directory... but
       * still have to pass the result to the caller.
       */      
      TA_Free( libHandle, basePath );
      *directory = dir;
      return TA_SUCCESS;
   }

   entryName = dirHandle.file_name;
   entryIsDirectory = dirHandle.file_attrs & ATTR_SUBDIR;
   #endif

   do
   {
      #if defined( USE_OSLAYER )
      if( file_matches( libHandle, entryName, filePattern ) )
      {
      #endif
         if( entryIsDirectory )
         {
            if( entryName[0] != '.' )
            {
               string = TA_StringAlloc( global->dirnameCache, entryName );

               if( string == NULL )
               {
                  #if defined( USE_OSLAYER )
                     close_dir(libHandle,&dirHandle);
                     TA_Free( libHandle, basePath );
                  #endif
                  TA_DirectoryFree( libHandle, dir );
                  return TA_ALLOC_ERR;
               }

               retCode = TA_ListAddTail( dir->listOfDirectory, (void *)string );

               if( retCode != TA_SUCCESS )
               {
                  #if defined( USE_OSLAYER )
                     close_dir(libHandle,&dirHandle);
                     TA_Free( libHandle, basePath );
                  #endif
                  TA_DirectoryFree( libHandle, dir );
                  return retCode;
               }
               dir->nbDirectory++;
            }
         }
         else
         {
            string = TA_StringAlloc( global->filenameCache, entryName );

            if( string == NULL )
            {
               #if defined( USE_OSLAYER )
                  close_dir(libHandle,&dirHandle);
                  TA_Free( libHandle, basePath );
               #endif
               TA_DirectoryFree( libHandle, dir );
               return TA_ALLOC_ERR;
            }

            retCode = TA_ListAddTail( dir->listOfFile, (void *)string );

            if( retCode != TA_SUCCESS )
            {
               #if defined( USE_OSLAYER )
                  close_dir(libHandle,&dirHandle);
                  TA_Free( libHandle, basePath );
               #endif
               TA_DirectoryFree( libHandle, dir );
               return retCode;
            }
            dir->nbFile++;
         }
      #if defined( USE_OSLAYER )
      }
      #endif
      
      #if defined( USE_WIN32_API )
      findNextRetCode = FindNextFile( handle, &data );
      entryName = data.cFileName;
      entryIsDirectory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      #endif

      #if defined( USE_OSLAYER )
      findNextRetCode = read_dir( libHandle, &dirHandle );
      entryName = dirHandle.file_name;
      entryIsDirectory = dirHandle.file_attrs & ATTR_SUBDIR;
      #endif
   }
   while( findNextRetCode == TRUE );


   #if defined( USE_OSLAYER )
   TA_Free( libHandle, basePath );
   if( !close_dir(libHandle,&dirHandle) )
   {
      TA_DirectoryFree( libHandle, dir );
      return TA_UNKNOWN_ERR;
   }
   #endif

   #if defined( USE_WIN32_API )   
   if( FindClose( handle ) != TRUE )
   {
      global->lastError = GetLastError();
      TA_DirectoryFree( libHandle, dir );
      return TA_UNKNOWN_ERR;
   }
   #endif

   /* Pass the result to the caller. */
   *directory = dir;

   return TA_SUCCESS;
}

TA_RetCode TA_DirectoryFree( TA_Libc *libHandle, TA_Directory *directory )
{
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return retCode;

   if( directory )
   {
      stringListFree( global->filenameCache, directory->listOfFile );
      stringListFree( global->dirnameCache, directory->listOfDirectory );
      TA_Free( libHandle, directory );
   }

   return TA_SUCCESS;
}

int TA_IsFileSystemCaseSensitive( void )
{
   #if defined(__WIN32__) || defined(__MSDOS__) || defined(WIN32)
      /* MS-DOS/Windows is not case sensitive. */
      return 0;
   #else
      /* For the time being, assume yes for all
       * other platform.
       */
      return 1;
   #endif
}

int TA_NbProcessor( TA_Libc *libHandle )
{
#if defined( USE_WIN32_API )
   TA_RetCode retCode;

   TA_SystemGlobal *global;

   (void)libHandle;
   
   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return 1;

   /* Run time function checking the number of processor on that system.
    * Will eventually allows to make better decision for parallelism.
    */
   initSysInfo(global);
   return global->sysInfo.dwNumberOfProcessors;
#else
   (void)libHandle;
   
   /* For the time being... do not assume more than one
    * processor on other platform.    
    */
   return 1;
#endif

}

/* Simplified thread functionality.
 *   A thread is created/started by calling TA_ThreadExec.
 */
#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_ThreadExec( TA_ThreadFunction newThreadEntryPoint, void *args )
{

    #if defined(WIN32)
    unsigned long thread_id;
    thread_id = _beginthread(newThreadEntryPoint,8192*4,(void *)args);
    if( thread_id == (unsigned long)-1 )
       return TA_UNKNOWN_ERR;
    #else    
    pthread_t thread_id;

    if( pthread_create(&thread_id, NULL,newThreadEntryPoint, args) != 0 )
       return TA_UNKNOWN_ERR;
    #endif

    return TA_SUCCESS;
}
#endif

void TA_Sleep( unsigned int seconds )
{
   #if defined( USE_WIN32_API )
      Sleep( seconds * 1000 );
   #endif

   #if defined( USE_OSLAYER )
      sleep( seconds );
   #endif
}

/* Implementation of counting semaphores. */
#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaInc( TA_Sema *sema, unsigned int *prevCount )
{
   #if defined( USE_WIN32_API )
   BOOL retValue;
   #endif

   if( sema == NULL )
      return TA_BAD_PARAM;

   /* Fail if not initialized. */
   if( !(sema->flags & TA_SEMA_INITIALIZED) )
      return TA_UNKNOWN_ERR;

   #if defined( USE_WIN32_API )
   retValue = ReleaseSemaphore( sema->theSema, 1, (LPLONG)prevCount );
   if( retValue != 0 )
      return TA_SUCCESS;
   else
      return TA_UNKNOWN_ERR;
   #endif

   #if defined( USE_OSLAYER )   
   (void)prevCount;
   if( sem_post( &sema->theSema ) != -1 )
      return TA_SUCCESS;
   else
      return TA_UNKNOWN_ERR;
   #endif
}
#endif

#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaDec( TA_Sema *sema )
{
   #if defined( USE_WIN32_API ) 
   DWORD retValue;
   #endif

   if( sema == NULL )
      return TA_BAD_PARAM;

   /* Fail if not initialized. */
   if( !(sema->flags & TA_SEMA_INITIALIZED) )
      return TA_UNKNOWN_ERR;

   #if defined( USE_WIN32_API ) 
   retValue = WaitForSingleObject( sema->theSema, INFINITE );

   if( retValue == WAIT_OBJECT_0 )
      return TA_SUCCESS;
   else
      return TA_UNKNOWN_ERR;
   #endif

   #if defined( USE_OSLAYER )
   if( sem_wait( &sema->theSema ) != -1 )
      return TA_SUCCESS;
   else
      return TA_UNKNOWN_ERR;
   #endif
}
#endif

#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaWait( TA_Sema *sema )
{
   return TA_SemaDec( sema );
}
#endif

#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaPost( TA_Sema *sema )
{
   return TA_SemaInc( sema, NULL );
}
#endif

#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaInit( TA_Sema *sema, unsigned int initialValue )
{
   if( sema == NULL )
      return TA_BAD_PARAM;

   #if defined( USE_WIN32_API )
   sema->theSema = CreateSemaphore( (LPSECURITY_ATTRIBUTES)NULL, (LONG)initialValue, (LONG)LONG_MAX, (LPCTSTR)NULL );
   
   if( sema->theSema )
   {
      sema->flags = TA_SEMA_INITIALIZED;
      return TA_SUCCESS;
   }
   else
   {
      sema->flags = 0;
      return TA_UNKNOWN_ERR;
   }
   #endif
   
   #if defined( USE_OSLAYER )
   if( sem_init( &sema->theSema, 0, initialValue ) != -1 )
   {
      sema->flags = TA_SEMA_INITIALIZED;
      return TA_SUCCESS;
   }
   else
   {
      sema->flags = 0;
      return TA_UNKNOWN_ERR;
   }  
   #endif
}
#endif

#if !defined( TA_SINGLE_THREAD )
TA_RetCode TA_SemaDestroy( TA_Sema *sema )
{
   #if defined( USE_WIN32_API )
   BOOL retValue;
   #endif

   if( sema == NULL )
      return TA_BAD_PARAM;

   if( !(sema->flags & TA_SEMA_INITIALIZED) )
      return TA_SUCCESS; /* Do nothing if not initialized. */

   #if defined( USE_WIN32_API )
   retValue = CloseHandle( sema->theSema );

   if( !retValue )
      return TA_UNKNOWN_ERR;
   else
      return TA_SUCCESS;
   #endif
   
   #if defined( USE_OSLAYER )
   sem_destroy( &sema->theSema );
   return TA_SUCCESS;
   #endif
}
#endif

/* Like TA_FileSeqOpen, but work with a stream instead. */
TA_RetCode TA_FileSeqOpenFromStream( TA_Libc *libHandle,
                                     TA_Stream *stream,
                                     TA_FileHandle **handle )
{
   TA_PROLOG;
   TA_FileHandlePriv *fileHandlePriv;

   TA_TRACE_BEGIN( libHandle, TA_FileSeqOpen );

   TA_ASSERT( libHandle, stream != NULL );
   TA_ASSERT( libHandle, handle != NULL );

   /* Allocate the private file handle. */
   fileHandlePriv = (TA_FileHandlePriv *)TA_Malloc( libHandle, sizeof( TA_FileHandlePriv ) );
   if( !fileHandlePriv )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( fileHandlePriv, 0, sizeof( TA_FileHandlePriv ) );

   /* There is NO file... */
   #if defined( USE_WIN32_API )
   fileHandlePriv->handle = INVALID_HANDLE_VALUE; 
   #endif

   #if defined( USE_OSLAYER )
   fileHandlePriv->handle = (FILE *)NULL;
   #endif

   /* ... use a stream instead. */
   fileHandlePriv->stream = stream;
   fileHandlePriv->streamAccess = TA_StreamAccessAlloc( stream );
   if( !fileHandlePriv->streamAccess )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Success! Return the info to the caller. */
   *handle = (TA_FileHandle *)fileHandlePriv;
   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_FileSeqOpen( TA_Libc *libHandle, const char *path, TA_FileHandle **handle )
{
   TA_PROLOG;
   TA_FileHandlePriv *fileHandlePriv;

   TA_TRACE_BEGIN( libHandle, TA_FileSeqOpen );

   TA_ASSERT( libHandle, path != NULL );
   TA_ASSERT( libHandle, handle != NULL );

   fileHandlePriv = (TA_FileHandlePriv *)TA_Malloc( libHandle, sizeof( TA_FileHandlePriv ) );
   if( !fileHandlePriv )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( fileHandlePriv, 0, sizeof( TA_FileHandlePriv ) );

   #if defined( USE_WIN32_API )
   fileHandlePriv->handle = CreateFile( path, GENERIC_READ, FILE_SHARE_READ,
                                        NULL, OPEN_EXISTING,
                                        FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL );

   if( fileHandlePriv->handle == INVALID_HANDLE_VALUE )
   {
      TA_FileSeqClose( libHandle, (TA_FileHandle *)fileHandlePriv );
      TA_TRACE_RETURN( TA_ACCESS_FAILED );
   }
   #else
   /* For all non-win32 platform, use standard ANSI C I/O */
   fileHandlePriv->handle = fopen( path, "rb" );

   if( fileHandlePriv->handle == 0 )
   {
      TA_FileSeqClose( libHandle, (TA_FileHandle *)fileHandlePriv );
      TA_TRACE_RETURN( TA_ACCESS_FAILED );
   }
   #endif

   /* Allocate buffer memory. */
   #if defined( USE_WIN32_API )
      fileHandlePriv->allocBuffer = allocDiskBuffer( libHandle, path, &fileHandlePriv->allocBufferSize );
      if( !fileHandlePriv->allocBuffer || (fileHandlePriv->allocBufferSize == 0) )
      {
         TA_FileSeqClose( libHandle, (TA_FileHandle *)fileHandlePriv );
         TA_TRACE_RETURN( TA_ACCESS_FAILED );
      }
   #else
      fileHandlePriv->allocBuffer = TA_Malloc( libHandle, FILE_BUFFER_SIZE );
      fileHandlePriv->allocBufferSize = FILE_BUFFER_SIZE;
      if( !fileHandlePriv->allocBuffer )
      {
         TA_FileSeqClose( libHandle, (TA_FileHandle *)fileHandlePriv );
         TA_TRACE_RETURN( TA_ACCESS_FAILED );
      }
   #endif

   /* Keep a ptr on the path. */
   fileHandlePriv->path = path;

   /* Success! Return the info to the caller. */
   *handle = (TA_FileHandle *)fileHandlePriv;
   TA_TRACE_RETURN( TA_SUCCESS );
}

unsigned int TA_FileSize( TA_Libc *libHandle, TA_FileHandle *handle )
{
   #if defined( USE_WIN32_API )
   BY_HANDLE_FILE_INFORMATION bhfi;
   #endif

   TA_FileHandlePriv *fileHandlePriv;
   unsigned int fileSize;

   TA_ASSERT_RET( libHandle, handle != NULL, 0 );

   fileHandlePriv = (TA_FileHandlePriv *)handle;

   if( fileHandlePriv->streamAccess )
   {
      /* Use the stream instead of the file. */
      fileSize = TA_StreamSizeInByte( fileHandlePriv->stream );
   }
   else
   {
      #if defined( USE_WIN32_API )
         TA_ASSERT_RET( libHandle, fileHandlePriv->handle != INVALID_HANDLE_VALUE, 0 );
         GetFileInformationByHandle( fileHandlePriv->handle, &bhfi );
         fileSize = bhfi.nFileSizeLow;
      #endif

      #if defined( USE_OSLAYER )
         TA_ASSERT_RET( libHandle, fileHandlePriv->handle != NULL, 0 );
         fileSize = get_file_size( libHandle, fileHandlePriv->path );
      #endif
   }

   return fileSize;
}

const char *TA_FileSeqRead( TA_Libc *libHandle, TA_FileHandle *handle, unsigned int *nbByteRead )
{
   #if defined( USE_WIN32_API )
   BOOL retValue;
   DWORD nbByteReadLocal;
   #else
   size_t nbByteReadLocal;
   #endif

   TA_FileHandlePriv *fileHandlePriv;
   const char *returnValue;
   TA_RetCode retCode;

   TA_ASSERT_RET( libHandle, handle != NULL, (char *)NULL );
   TA_ASSERT_RET( libHandle, nbByteRead != NULL, (char *)NULL );

   fileHandlePriv = (TA_FileHandlePriv *)handle;

   if( fileHandlePriv->streamAccess )
   {
      /* Use the stream instead of the file. 
       * Get the data chunk by chunk.
       */
      retCode = TA_StreamAccessGetBuffer( fileHandlePriv->streamAccess,
                                          &returnValue,
                                          nbByteRead );
      if( retCode != TA_SUCCESS )
         return 0;
   }  
   else
   { 
	   
      TA_ASSERT_RET( libHandle, fileHandlePriv->allocBuffer != NULL, (char *)NULL );
      TA_ASSERT_RET( libHandle, fileHandlePriv->allocBufferSize >= 128, (char *)NULL );

      *nbByteRead = 0;

      #if defined( USE_WIN32_API )
         TA_ASSERT_RET( libHandle, fileHandlePriv->handle != INVALID_HANDLE_VALUE, (char *)NULL );
         retValue = ReadFile( fileHandlePriv->handle,
                              fileHandlePriv->allocBuffer,
                              fileHandlePriv->allocBufferSize,
                              &nbByteReadLocal,
                              NULL );

         if( retValue == 0 )
            return NULL;
      #else
         TA_ASSERT_RET( libHandle, fileHandlePriv->handle != NULL, (char *)NULL );
         if( feof(fileHandlePriv->handle) || ferror(fileHandlePriv->handle) )
            return NULL;

         nbByteReadLocal = fread( fileHandlePriv->allocBuffer, 1, 
                                  fileHandlePriv->allocBufferSize,
                                  fileHandlePriv->handle );
      #endif

      *nbByteRead = nbByteReadLocal;

      if( *nbByteRead == 0 )
         return NULL;

      returnValue = fileHandlePriv->allocBuffer;
   }

   return returnValue;
}

TA_RetCode TA_FileSeqClose( TA_Libc *libHandle, TA_FileHandle *handle )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_FileHandlePriv *fileHandlePriv;
   TA_SystemGlobal *global;
   #if defined( USE_WIN32_API )
   DWORD win32Error;
   BOOL retValue;
   #endif

   TA_TRACE_BEGIN( libHandle, TA_FileSeqClose );

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( libHandle, handle != NULL );

   fileHandlePriv = (TA_FileHandlePriv *)handle;

   if( fileHandlePriv )
   {
      #if defined( USE_WIN32_API )
         if( fileHandlePriv->handle != INVALID_HANDLE_VALUE )
         {
            retValue = CloseHandle( fileHandlePriv->handle );
            if( retValue == 0 )
            {
               win32Error = GetLastError();
               global->lastError = win32Error;
               TA_FATAL( libHandle, NULL, 0, win32Error );
            }
         }
         if( fileHandlePriv->allocBuffer )
         {
            freeDiskBuffer( libHandle, fileHandlePriv->allocBuffer,
                            fileHandlePriv->allocBufferSize );
         }
      #endif

      #if defined( USE_OSLAYER )
         if( fileHandlePriv->handle != NULL )
            fclose( fileHandlePriv->handle );
         if( fileHandlePriv->allocBuffer )
            TA_Free( libHandle, fileHandlePriv->allocBuffer );
      #endif

      if( fileHandlePriv->streamAccess )
      {
         TA_StreamAccessFree( fileHandlePriv->streamAccess );
      }

      TA_Free( libHandle, fileHandlePriv );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}
/**** Local functions definitions.     ****/
static void stringListFree( TA_StringCache *stringCache, TA_List *list )
{
   TA_String *node;

   if( list == NULL )
      return;

   while( (node = (TA_String *)TA_ListRemoveHead( list )) != NULL )
   {
      TA_StringFree( stringCache, node );
   }

   TA_ListFree( list );
}

#if defined( USE_WIN32_API )
static LPVOID allocDiskBuffer( TA_Libc *libHandle, const char *path, DWORD *nbByteAlloc )
{
   BOOL retValue;
   LPCTSTR lpRootPathName; /* address of root path */
   DWORD lpSectorsPerCluster; /* address of sectors per cluster */
   DWORD lpBytesPerSector; /* address of bytes per sector */
   DWORD lpNumberOfFreeClusters; /* address of number of free clusters */
   DWORD lpTotalNumberOfClusters; /* address of total number of clusters */
   LPVOID allocatedMem;
   DWORD win32Error;
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
      return (LPVOID)NULL;

   lpRootPathName = path;

   *nbByteAlloc = 0;


   retValue = GetDiskFreeSpace( NULL/*lpRootPathName*/,
                                &lpSectorsPerCluster,
                                &lpBytesPerSector,
                                &lpNumberOfFreeClusters,
                                &lpTotalNumberOfClusters );

   if( retValue == 0 )
   {
      win32Error = GetLastError();
      global->lastError = win32Error;
      return (LPVOID)NULL;
   }

   allocatedMem = VirtualAlloc( NULL, /* No specific address needed. */
                                lpBytesPerSector, /* size of region */
                                MEM_COMMIT|MEM_RESERVE, /* type of allocation */
                                PAGE_READWRITE/* type of access protection */ );

   if( !allocatedMem )
   {
      win32Error = GetLastError();
      global->lastError = win32Error;
      return (LPVOID)NULL;
   }

   /* Success... return information to caller. */
   *nbByteAlloc = lpBytesPerSector;
   return allocatedMem;
}
#endif

#if defined( USE_WIN32_API )
static TA_RetCode freeDiskBuffer( TA_Libc *libHandle, LPVOID buffer, DWORD nbByteAlloc )
{
   TA_PROLOG;
   BOOL retValue;
   DWORD win32Error;
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   TA_TRACE_BEGIN( libHandle, freeDiskBuffer );

   retCode = TA_GetGlobal( libHandle, &TA_SystemGlobalControl, (void **)&global );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Uncommit all pages. */
   retValue = VirtualFree( buffer, nbByteAlloc, MEM_DECOMMIT );
   if( retValue == 0 )
   {
      win32Error = GetLastError();
      global->lastError = win32Error;
      TA_FATAL( libHandle, "Win32 Cannot free paged mem", nbByteAlloc, win32Error );
   }

   /* Unreserve all pages. */
   retValue = VirtualFree( buffer, 0, MEM_RELEASE );
   if( retValue == 0 )
   {
      win32Error = GetLastError();
      global->lastError = win32Error;
      TA_FATAL( libHandle, "Win32 Cannot free paged mem", 0, win32Error );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}
#endif

#if defined( USE_WIN32_API )
static void initSysInfo( TA_SystemGlobal *global )
{
   if( global->sysInfoInitialized == 0 )
   {
      GetSystemInfo( &global->sysInfo );
      global->sysInfoInitialized = 1;
   }
}
#endif

static TA_RetCode TA_SystemGlobalInit( TA_Libc *libHandle,
                                       void **globalToAlloc )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_SystemGlobal *global;

   TA_TRACE_BEGIN( libHandle, TA_SystemGlobalInit );

   if( libHandle == NULL )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   TA_ASSERT( libHandle, globalToAlloc != NULL );

   *globalToAlloc = NULL;

   global = (TA_SystemGlobal *)TA_Malloc( libHandle, sizeof( TA_SystemGlobal ) );
   if( !global )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   memset( global, 0, sizeof( TA_SystemGlobal ) );

   retCode = TA_StringCacheAlloc( libHandle, &global->dirnameCache );
   if( retCode != TA_SUCCESS )
   {
      TA_Free( libHandle, global );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   retCode = TA_StringCacheAlloc( libHandle, &global->filenameCache );
   if( retCode != TA_SUCCESS )
   {
      TA_StringCacheFree( global->dirnameCache );
      TA_Free( libHandle, global );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Success! Return the global to the caller. */
   *globalToAlloc = global;
   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode TA_SystemGlobalShutdown( TA_Libc *libHandle,
                                           void *globalAllocated )
{
   TA_PROLOG;
   TA_RetCode retCode, finalRetCode;
   TA_SystemGlobal *global;

   TA_TRACE_BEGIN( libHandle, TA_SystemGlobalShutdown );

   TA_ASSERT( libHandle, libHandle != NULL );

   /* No need to shutdown if the initialization failed. */
   if( globalAllocated == NULL )
   {
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   finalRetCode = TA_SUCCESS;

   global = (TA_SystemGlobal *)globalAllocated;

   if( global->dirnameCache )
   {
      retCode = TA_StringCacheFree( global->dirnameCache );
      TA_ASSERT( libHandle, retCode == TA_SUCCESS );
      if( retCode != TA_SUCCESS )
         finalRetCode = retCode;
   }

   if( global->filenameCache )
   {
      retCode = TA_StringCacheFree( global->filenameCache );
      TA_ASSERT( libHandle, retCode == TA_SUCCESS );
      if( retCode != TA_SUCCESS )
         finalRetCode = retCode;
   }

   TA_Free( libHandle, global );

   TA_TRACE_RETURN( finalRetCode );
}
