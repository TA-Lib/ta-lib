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
 *  052101 MF   First version.
 *  060302 MF   Add better retry/timeout mechanism.
 */

/* Description:
 *    Provides functionality for fetching data from internet.
 *    
 *    On WIN32, both the WinInet or libCurl library are
 *    supported. WinInet is the prefered choice.
 *
 *    On all unix platform, libCurl is used.
 */

/* #define DEBUG_PRINTF 1 */

/**** Headers ****/
#if !defined( USE_WININET ) && !defined( USE_LIBCURL )
   /* If the user does not specify its preference in
    * the makefile options, use the default choices.
    */
   #if defined( WIN32 )
      /* Win32 platform use WinInet.lib by default. */
      #define USE_WININET
   #else
      /* All non-win32 platform use libCurl.lib by default. */
      #define USE_LIBCURL
   #endif
#endif

/* Just check for unsupported setup */
#if defined(LIBCURL) && defined(USE_WININET)
#error You must choose bettween LIBCURL and WinInet!
#endif

#if defined(USE_WININET) && !defined(WIN32)
#error WinInet can be used only on win32 platform.
#endif

#include <string.h>

#include "ta_common.h"
#include "ta_trace.h"
#include "ta_magic_nb.h"
#include "ta_system.h"
#include "ta_global.h"
#include "ta_memory.h"
#include "ta_network.h"

#if defined( USE_WININET )
   #include "windows.h"
   #include "wininet.h"   
#endif

#if defined( USE_LIBCURL )
   #include "curl/curl.h"
#endif

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define BUFFER_SIZE 1500

typedef struct
{
   unsigned int magicNb;
   TA_Libc *libHandle;

   /* Variables describing where and how to fetch the data. */
   const char *webSiteAddr;
   const char *webSitePage;
   const char *proxyName;
   const char *proxyPort;
   
   /* Final result. 0 if success. OS dependent. */
   unsigned int finalErrorCode; 

   /* Final result from TA-LIB. */
   TA_RetCode finalRetCode;

} TA_WebPageHiddenData;

typedef struct
{
   /* Indicate if fundamental initialization succeed,
    * if not, all subsequent network operations will
    * failed gracefuly.
    */
   unsigned int initialized; 

   #if defined( USE_WININET )
   /* Indicate if a connection to internet
    * is currently active. Protected by
    * a mutexSema.
    */
   unsigned int connected;

   /* Will be different of NULL when connected.
    * must be freed at shutdown.
    */
   HINTERNET hInternet;
   #endif

   #if defined( USE_LIBCURL )
   CURL *curlHandle;
   #endif

   #if !defined( TA_SINGLE_THREAD )
   /* Serialize the web access for the time being,
    * until appropriate stress testing is done.
    *
    * On Win32/WinInet, this mutex is also used to protect
    * the variable 'connected' to make sure only one task
    * proceed to initialize the internet connection.
    */
   TA_Sema mutexSema;
   #endif

} TA_NetworkGlobal;

/**** Local functions declarations.    ****/
TA_RetCode internalWebPageAlloc( TA_Libc       *libHandle,
                                 const char    *webSiteAddr,
                                 const char    *webSitePage,
                                 const char    *proxyName,
                                 const char    *proxyPort,
                                 TA_WebPage   **webPageAllocated );

static TA_RetCode TA_NetworkGlobalInit    ( TA_Libc *libHandle, void **globalToAlloc );
static TA_RetCode TA_NetworkGlobalShutdown( TA_Libc *libHandle, void *globalAllocated );

#if defined( USE_WININET )
static TA_RetCode fetchUsingWinInet( TA_Libc *libHandle,
                                     TA_NetworkGlobal *global,
                                     TA_WebPage       *webPage );

static TA_RetCode buildListDataWinInet( TA_Libc *libHandle,
                                        TA_WebPage *webPage,
                                        HINTERNET hRessource );

static TA_RetCode TA_SetReceiveTimeout( TA_Libc *libHandle,
                                        unsigned long receiveTimeout,
                                        HINTERNET hWebPage );
#endif

#if defined( USE_LIBCURL )
static TA_RetCode fetchUsingLibCurl( TA_Libc *libHandle,
                                     TA_NetworkGlobal *global,
                                     TA_WebPage       *webPage );
static TA_RetCode rfc1945StatusToRetCode( unsigned int httpErrorCode );

size_t libcurlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);
#endif


/**** Local variables definitions.     ****/
TA_FILE_INFO;

const TA_GlobalControl TA_NetworkGlobalControl =
{
   TA_NETWORK_GLOBAL_ID,
   TA_NetworkGlobalInit,
   TA_NetworkGlobalShutdown
};

/**** Global functions definitions.   ****/
TA_RetCode TA_WebPageAlloc( TA_Libc       *libHandle,
                            const char    *webSiteAddr,
                            const char    *webSitePage,
                            const char    *proxyName,
                            const char    *proxyPort,
                            TA_WebPage   **webPageAllocated,
                            unsigned int   nbAttempt )
{
   TA_RetCode retCode;
   unsigned int i,j;

   /* Make sure there is at least one attempt. */
   if( nbAttempt == 0 )
      nbAttempt = 1;

   retCode = TA_SUCCESS;

   for( i=0; i < nbAttempt; i++ )
   {
      if( i > 0 )
      {
         /* Some delay before a new attempt. 
          * We do not want to irritate the server.
          * Wait up to 20 seconds.
          */
         j = i*2;
         if( j > 20 ) j = 20;
         TA_Sleep( j ); 
      }

      /* On data retreival problems, keep retrying
       * many times. TA_INTERNET_READ_DATA_FAILED means
       * that the server can be reached, but somehow the 
       * transmission of the data was interupted. So it is
       * worth to give multiple re-try immediatly.
       *
       * All other type of failure will retry
       * only "nbAttempt" times.
       */
      retCode = TA_INTERNET_READ_DATA_FAILED;
      for( j=0; (retCode == TA_INTERNET_READ_DATA_FAILED) && (j < 100); j++ )
      {
         retCode = internalWebPageAlloc( libHandle,
                                         webSiteAddr,
                                         webSitePage,
                                         proxyName,
                                         proxyPort,
                                         webPageAllocated );
         if( retCode == TA_INTERNET_READ_DATA_FAILED )
            TA_Sleep( 1 ); /* 1 second */
      }

      if( retCode == TA_SUCCESS )
         return TA_SUCCESS;
   }

   return retCode;
}


TA_RetCode TA_WebPageFree( TA_WebPage *webPage )
{
   TA_Libc *libHandle;
   TA_WebPageHiddenData *hiddenData;

   if( webPage ) 
   {      
      hiddenData = webPage->hiddenData;
      if( !hiddenData )
         return TA_INTERNAL_ERROR(32);

      if( hiddenData->magicNb != TA_WEBPAGE_MAGIC_NB )
         return TA_BAD_OBJECT;

      libHandle = hiddenData->libHandle;
      if( !libHandle )
         return TA_INTERNAL_ERROR(33);

      /* The object is validated, can start to free ressources
       * from this point.
       */

      /* Free private data */
      TA_Free( libHandle, hiddenData );

      /* Free public data. */
      if( webPage->content )
         TA_StreamFree( webPage->content );

      TA_Free( libHandle, webPage );
   }

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
static TA_RetCode TA_NetworkGlobalInit( TA_Libc *libHandle, void **globalToAlloc )
{
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif
   TA_NetworkGlobal *global;

   if( !globalToAlloc )
      return TA_BAD_PARAM;

   *globalToAlloc = NULL;

   #if defined( USE_LIBCURL )
      #if defined(WIN32)
         if( curl_global_init(CURL_GLOBAL_WIN32) != CURLE_OK )
            return TA_LIBCURL_GLOBAL_INIT_FAILED;
      #else
         if( curl_global_init(CURL_GLOBAL_NOTHING) != CURLE_OK )
            return TA_LIBCURL_GLOBAL_INIT_FAILED;
      #endif
   #endif

   global = (TA_NetworkGlobal *)TA_Malloc( libHandle, sizeof( TA_NetworkGlobal ) );
   if( !global )
      return TA_ALLOC_ERR;

   memset( global, 0, sizeof( TA_NetworkGlobal ) );

   #if defined( USE_LIBCURL )
      global->curlHandle = curl_easy_init();
      if( global->curlHandle == NULL )
      {
         TA_Free( libHandle, global );
         return TA_LIBCURL_INIT_FAILED;
      }
   #endif

   #if !defined( TA_SINGLE_THREAD )
      /* Initialize the mutex in a non-block state. */
      retCode = TA_SemaInit( &global->mutexSema, 1 );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( libHandle, global );
         return retCode;
      }
   #endif

   global->initialized = 1;

   /* Success, return the allocated memory to the caller. */
   *globalToAlloc = global;
   return TA_SUCCESS;
}

static TA_RetCode TA_NetworkGlobalShutdown( TA_Libc *libHandle, void *globalAllocated )
{
   TA_NetworkGlobal *global;
   TA_RetCode retCode = TA_SUCCESS;

   global = (TA_NetworkGlobal *)globalAllocated;

   if( !global )
      return retCode;

   if( global->initialized )
   {
      #if defined( USE_WININET )
         if( global->hInternet )
            InternetCloseHandle( global->hInternet);
      #endif

      #if defined( USE_LIBCURL )
         curl_global_cleanup();
      #endif

      global->initialized = 0;
   }

   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaDestroy( &global->mutexSema );
   #endif

   TA_Free( libHandle, global );

   return retCode;
}

#if defined( USE_LIBCURL )
static TA_RetCode fetchUsingLibCurl( TA_Libc *libHandle,
                                     TA_NetworkGlobal *global,
                                     TA_WebPage       *webPage )
{
   TA_PROLOG;

   TA_RetCode retCode;
   TA_WebPageHiddenData *webPageHidden;
   const char *string1, *string2, *string3;
   unsigned int urlLength;
   char *urlString;
   CURLcode retValue;
   long curlInfo;

   TA_TRACE_BEGIN( libHandle, fetchUsingLibCurl );

   TA_ASSERT( libHandle, webPage != NULL );

   webPageHidden = (TA_WebPageHiddenData *)webPage->hiddenData;   
  
   /* Open an internet session. */

   /* Create the URL. */
   string1 = "http://";
   urlLength = strlen( string1 ) + 1;
   if( webPageHidden->webSiteAddr )
   {
      string2 = webPageHidden->webSiteAddr;
      urlLength += strlen( string2 );
   }
   else
      string2 = NULL;

   if( webPageHidden->webSitePage )
   {
      string3 = webPageHidden->webSitePage;
      urlLength += strlen( string3 ) + 1;
   }
   else
      string3 = NULL;

   urlString = TA_Malloc( libHandle, urlLength );
   if( !urlString )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   sprintf( urlString, "%s%s%s%s", string1? string1:"",
                             string2? string2:"", 
                             string3 && string3[0] != '/'? "/":"", 
                             string3? string3:"" );
   
   /* Serialize the request until a stress testing 
    * application proove that libcurl is multi-thread safe.
    */
   #if !defined( TA_SINGLE_THREAD )
      retCode = TA_SemaWait( &global->mutexSema );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( libHandle, urlString );
         TA_TRACE_RETURN( retCode );
      }
   #endif

   /* Specify URL to get */
   curl_easy_setopt(global->curlHandle, CURLOPT_URL, urlString );
 
   /* Send all data to the callback function  */
   curl_easy_setopt(global->curlHandle, CURLOPT_WRITEFUNCTION,
                    libcurlWriteMemoryCallback);
 
   /* Specify the opaque data ptr for the callback function */
   curl_easy_setopt(global->curlHandle, CURLOPT_FILE, (void *)webPage);

   /* Fetch it. */
   retValue = curl_easy_perform(global->curlHandle);

   if( retValue == CURLE_OK )
      retCode = TA_SUCCESS;
   else
   {
      retValue = curl_easy_getinfo( global->curlHandle, CURLINFO_HTTP_CODE, &curlInfo );
      if( retValue == CURLE_OK )
         retCode = rfc1945StatusToRetCode( curlInfo );
      else
         retCode = TA_HTTP_SC_UNKNOWN;
   }

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &global->mutexSema  );
   #endif

   /* Free the url. */
   TA_Free( libHandle, urlString );

   TA_TRACE_RETURN( retCode );
}

size_t libcurlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
   register int bufferSize;
   TA_WebPage *webPage;
   TA_WebPageHiddenData *webPageHidden;
   TA_Libc *libHandle;
   TA_RetCode retCode;
   char *buffer;

   webPage = (TA_WebPage *)data;
   webPageHidden = (TA_WebPageHiddenData *)webPage->hiddenData;   
   libHandle = webPageHidden->libHandle;

   bufferSize = size * nmemb;
   
   /* Make a copy of the provided data. */
   buffer = TA_Malloc( libHandle, bufferSize );
   if( !buffer )
      return 0; /* Error. */

   memcpy( buffer, ptr, bufferSize );

   /* Add the data to the stream. */
   if( !webPage->content )
   {
      /* The first buffer will initiate the creation of
       * the stream.
       */
      webPage->content = TA_StreamAllocFromBuffer( libHandle,
                                                   buffer, bufferSize,
                                                   NULL, NULL );

      if( !webPage->content )
      {
         TA_Free( libHandle, buffer );
         return 0; /* Error. */
      }
   }
   else
   {
      /* Add the buffer to the stream. */
      retCode = TA_StreamAddBuffer( webPage->content, 
                                    buffer, bufferSize,
                                    NULL, NULL );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( libHandle, buffer );
         return 0; /* Error */
      }
   }

   return bufferSize;
}
#endif

#if defined( USE_WININET )
static TA_RetCode fetchUsingWinInet( TA_Libc *libHandle,
                                     TA_NetworkGlobal *global,
                                     TA_WebPage       *webPage )
{
   TA_PROLOG;

   TA_RetCode retCode;
   DWORD retDWORD;
   LPCTSTR lpszProxyName;
   TA_WebPageHiddenData *webPageHidden;
   HINTERNET hSession, hWebPage;

   TA_TRACE_BEGIN( libHandle, fetchUsingWinInet );

   webPageHidden = (TA_WebPageHiddenData *)webPage->hiddenData;   

   retCode = TA_SUCCESS; /* Will change if an error occured. */
   if( !global->connected )
   {
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaWait( &global->mutexSema );
      #endif

         /* Check again if connected within the mutex. To avoid
          * multiple task to initialize at the same time.
          */
         if( !global->connected )
         {                  
            /* Open and verify connection with a known URL. */
            retDWORD = InternetAttemptConnect(0);
            if( retDWORD != ERROR_SUCCESS )
               retCode = TA_NO_INTERNET_CONNECTION;
            else
            {
               /* needed?
                if( InternetCheckConnection((LPCTSTR)NULL, FLAG_ICC_FORCE_CONNECTION, 0 ) != 0 )
                  retCode = TA_INTERNET_ACCESS_FAILED;
                */

               /* Ok.. the last step is to get an handle used for
                * retreiving the Web pages.
                */
               if( webPageHidden->proxyName )
               {           
                  lpszProxyName = TA_Malloc( libHandle, 100 + strlen( webPageHidden->proxyName ) );
                  if( !lpszProxyName )
                     retCode = TA_ALLOC_ERR;
                  else
                     sprintf( (char *)lpszProxyName, "http=http://%s:%s",
                              webPageHidden->proxyName,
                              webPageHidden->proxyPort? "80":webPageHidden->proxyPort );                      
               }
               else
                  lpszProxyName = NULL;
               
               if( retCode == TA_SUCCESS ) 
               {
                  global->hInternet = InternetOpen( "TA-LIB",
                                                    INTERNET_OPEN_TYPE_PRECONFIG,
                                                     lpszProxyName, NULL, 0 );
               }

               if( lpszProxyName )
                  TA_Free( libHandle, (void *)lpszProxyName );

            }

            if( !global->hInternet )
               webPageHidden->finalErrorCode = GetLastError();
            else
            {
               /* Success. Set a variable to avoid to connect again.
                * This variable will also make sure that the hInternet
                * is freed upon shutdown.
                */
               global->connected = 1; /* Success */
            }
         }
      #if !defined( TA_SINGLE_THREAD )
         TA_SemaPost( &global->mutexSema  );
      #endif

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }            
   }

   if( !global->hInternet )
   {
      /* May be returned in multi-thread situation if
       * the inital thread did not yet complete or succeed
       * to establish the internet access.
       */
      TA_TRACE_RETURN( TA_INTERNET_NOT_OPEN_TRY_AGAIN );
   }
  
   /* Open an internet session. */
   hSession = InternetConnect( global->hInternet,
                               webPageHidden->webSiteAddr,
                               INTERNET_DEFAULT_HTTP_PORT,
                               NULL, NULL,
                               (unsigned long)INTERNET_SERVICE_HTTP,
                               0, 0 );

   if( !hSession )
   {
      /* Did not work.... mmmm.... always try a second time. */
      hSession = InternetConnect( global->hInternet,
                                  webPageHidden->webSiteAddr,
                                  INTERNET_DEFAULT_HTTP_PORT,
                                  NULL, NULL,
                                  (unsigned long)INTERNET_SERVICE_HTTP,
                                  0, 0 );
   }

   if( !hSession )
   {
      TA_TRACE_RETURN( TA_INTERNET_SERVER_CONNECT_FAILED );
   }
   else
   {
      hWebPage = HttpOpenRequest( hSession, "GET",
                                 webPageHidden->webSitePage,
                                 NULL,
                                 NULL,
                                 NULL,
                                 INTERNET_FLAG_NEED_FILE|
                                 INTERNET_FLAG_CACHE_IF_NET_FAIL|                                 
                                 INTERNET_FLAG_NO_UI|
                                 INTERNET_FLAG_NO_COOKIES|
                                 INTERNET_FLAG_RESYNCHRONIZE,
                                 0 );
      if( !hWebPage )
         retCode = TA_INTERNET_OPEN_REQUEST_FAILED;
      else
      {         
         retCode = TA_SetReceiveTimeout( libHandle,
                                         10000 /* 10 seconds */,
                                         hWebPage );
         if( retCode == TA_SUCCESS )
         {
            if( !HttpSendRequest( hWebPage, 0, 0, 0, 0 ) )
               retCode = TA_INTERNET_SEND_REQUEST_FAILED;
            else
               retCode = buildListDataWinInet( libHandle, webPage, hWebPage );
         }

         InternetCloseHandle( hWebPage );
      }
      
      InternetCloseHandle( hSession );
   }

   TA_TRACE_RETURN( retCode );
}
#endif

#if defined( USE_WININET )
static TA_RetCode TA_SetReceiveTimeout( TA_Libc  *libHandle,
                                        DWORD     newTimeout, /* milliseconds */
                                        HINTERNET hWebPage )
{
   BOOL status;

   /* Set the timeout who is going to affect calls to
    * InternetReadFile when retreiving data.
    */

   #if 0
   /* Code for debugging */
	unsigned char *optionString;
	DWORD optionStringLength = 0;
   DWORD receiveTimeout;

	status = InternetQueryOption( hWebPage, 
                                 INTERNET_OPTION_RECEIVE_TIMEOUT,
			                        NULL, &optionStringLength );
	optionString = TA_Malloc( libHandle, optionStringLength + 1);
	optionString[optionStringLength] = 0;
	InternetQueryOption( hWebPage, INTERNET_OPTION_RECEIVE_TIMEOUT,
			               &receiveTimeout, &optionStringLength );
   /*printf("GET INTERNET_OPTION_RECEIVE_TIMEOUT = %u\n", receiveTimeout );*/
	TA_Free( libHandle, (void *) optionString );
   #endif

   status = InternetSetOption( hWebPage,
                               INTERNET_OPTION_RECEIVE_TIMEOUT,
                               &newTimeout,
                               sizeof(DWORD) );

	if( !status )
      return TA_INTERNET_SET_RX_TIMEOUT_FAILED;

   return TA_SUCCESS;
}
#endif

#if defined( USE_WININET )
static TA_RetCode buildListDataWinInet( TA_Libc *libHandle, 
                                        TA_WebPage *webPage,
                                        HINTERNET hRessource )
{
   TA_PROLOG;

   LPSTR    lpszData;      /* buffer for the data */
   DWORD    dwSize;        /* size of the data available */
   DWORD    dwDownloaded;  /* size of the downloaded data */
   TA_RetCode retCode;
   int again;
   int i;
   BOOL status;

   TA_TRACE_BEGIN( libHandle, buildListDataWinInet );

   /* This loop handles reading the data.   */
   again = 1;
   while( again )
   {
      /* The call to InternetQueryDataAvailable determines the amount
 		 * of data available to download.
       */
      status = FALSE;
      for( i = 0; (status == FALSE) && (i < 2); i++ )
         status = InternetQueryDataAvailable(hRessource,&dwSize,0,0);

	   if( !status )
	   {
		   /*printf("buildListDataWinInet(): InternetQueryDataAvailable() TIMED OUT %d TIMES!\n", i );*/
		   TA_TRACE_RETURN( TA_INTERNET_READ_DATA_FAILED );
	   }
      else if( dwSize == 0 )
      {
         again = 0; /* We got all the data, exit. */
      }
      else
      {    
         /* Allocate a buffer of the size returned by
          * InternetQueryDataAvailable.
          */
         lpszData = TA_Malloc( libHandle, dwSize );

         if( !lpszData )
         {
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }

         /* Read the data from the HINTERNET handle. */
         if(!InternetReadFile(hRessource,(LPVOID)lpszData,dwSize,&dwDownloaded))
         {
            again = 0;
         }
         else
         {
            if( !webPage->content )
            {
               /* The first buffer will initiate the creation of
                * the stream. If there is any failure, while adding
                * more data to this webPage->content, the partially
                * filled stream will be free when TA_WebPageFree
                * will do the clean-up in internalWebPageAlloc.
                */
               webPage->content = TA_StreamAllocFromBuffer( libHandle,
                                                           (unsigned char *)lpszData, dwSize,
                                                           NULL, NULL );

               if( !webPage->content )
               {
                  TA_Free( libHandle, lpszData );
                  TA_TRACE_RETURN( TA_ALLOC_ERR );
               } 
            }
            else
            {
               /* Add the buffer to the stream. */
               retCode = TA_StreamAddBuffer( webPage->content, (unsigned char *)lpszData, dwSize, NULL, NULL );
               if( retCode != TA_SUCCESS )
               {
                  TA_Free( libHandle, lpszData );
                  TA_TRACE_RETURN( TA_ALLOC_ERR );
               }
            }

            /* Check the size of the remaining data.  If it is zero, exit. */
            if (dwDownloaded == 0)
            {
               again = 0;
            }
         }
      }
   }

   /* Everything suceed!  */
   TA_TRACE_RETURN( TA_SUCCESS );
}
#endif

TA_RetCode internalWebPageAlloc( TA_Libc       *libHandle,
                                 const char    *webSiteAddr,
                                 const char    *webSitePage,
                                 const char    *proxyName,
                                 const char    *proxyPort,
                                 TA_WebPage   **webPageAllocated )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_NetworkGlobal *global;
   TA_WebPage *tmpWebPage;
   TA_WebPageHiddenData *webPageHiddenData;

   if( !webSiteAddr || !webPageAllocated )
      return TA_BAD_PARAM;

   TA_TRACE_BEGIN( libHandle, TA_WebPageAlloc );

   *webPageAllocated = NULL;

   /* Get the pointer on the global variables. */
   retCode = TA_GetGlobal( libHandle, &TA_NetworkGlobalControl, (void *)&global );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Make sure the network library was initialized. */
   if( !global->initialized )
   {
      TA_TRACE_RETURN( TA_SOCKET_LIB_INIT_ERR );
   }

   /* Alloc the TA_WebPage. */
   tmpWebPage = (TA_WebPage *) TA_Malloc(libHandle, sizeof(TA_WebPage) );
   if( !tmpWebPage )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( tmpWebPage, 0, sizeof( TA_WebPage ) );

   /* Alloc the hidden implementation of a TA_WebPage. */
   webPageHiddenData = (TA_WebPageHiddenData *) TA_Malloc(libHandle, sizeof(TA_WebPageHiddenData));

   if( !webPageHiddenData )
   {
      TA_Free( libHandle, tmpWebPage );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   memset( webPageHiddenData, 0, sizeof( TA_WebPageHiddenData ) );
   webPageHiddenData->magicNb     = TA_WEBPAGE_MAGIC_NB;
   webPageHiddenData->libHandle   = libHandle;
   webPageHiddenData->webSiteAddr = webSiteAddr;
   webPageHiddenData->webSitePage = webSitePage;
   webPageHiddenData->proxyName   = proxyName;
   webPageHiddenData->proxyPort   = proxyPort;

   tmpWebPage->hiddenData = webPageHiddenData;

   /* From this point, TA_WebPageFree shall be called to clean-up. */

   #ifdef DEBUG_PRINTF
      printf( "Fetching [%s][%s]", webSiteAddr, webSitePage );
   #endif

   #if defined( USE_WININET )
      retCode = fetchUsingWinInet( libHandle, global, tmpWebPage );
   #endif

   #if defined( USE_LIBCURL )
      retCode = fetchUsingLibCurl( libHandle, global, tmpWebPage );
   #endif

   if( retCode != TA_SUCCESS )
   {
      /* If an error occured at any point in this module,
       * we are sure to free up everything by ending-up here.
       * It is assumed that all ressources are referenced
       * within the 'tmpWebPage' of course.
       */
      TA_WebPageFree( tmpWebPage );
      TA_TRACE_RETURN( retCode );
   }
   
   /* Everything is fine! Return the page to the caller. */
   *webPageAllocated = tmpWebPage; 
   TA_TRACE_RETURN( TA_SUCCESS );
}

#if defined( USE_LIBCURL )
static TA_RetCode rfc1945StatusToRetCode( unsigned int httpErrorCode )
{
   switch( httpErrorCode )
   {
   case 301:
      return TA_HTTP_SC_301;     /* Moved Permanently */
   case 302:
      return TA_HTTP_SC_302;     /* Moved Temporarily */
   case 304:
      return TA_HTTP_SC_304;     /* Not Modified      */
   case 400:
      return TA_HTTP_SC_400;     /* Bad Request       */
   case 401:
      return TA_HTTP_SC_401;     /* Unauthorized      */
   case 403:
      return TA_HTTP_SC_403;     /* Forbidden         */
   case 404:
      return TA_HTTP_SC_404;     /* Not Found         */
   case 500:
      return TA_HTTP_SC_500;     /* Internal Server Error */
   case 501:
      return TA_HTTP_SC_501;     /* Not Implemented */
   case 502:
      return TA_HTTP_SC_502;     /* Bad Gateway */
   case 503:
      return TA_HTTP_SC_503;     /* Service Unavailable */
   }

   return TA_HTTP_SC_UNKNOWN; /* Unknown error code. */ 
}
#endif
