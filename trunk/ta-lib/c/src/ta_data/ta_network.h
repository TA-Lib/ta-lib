#ifndef TA_NETWORK_H
#define TA_NETWORK_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_STREAM_H
   #include "ta_stream.h"
#endif

/* Functionality for fetching a Web Page. */
typedef struct
{
   /* The content of the page is put in 
    * a TA_Stream. The header portion is
    * stripped from the page.
    */
   TA_Stream *content;

   /* Do not modify the following variable. */
   void *hiddenData;
} TA_WebPage;

/* Fetch the specified web page.
 *
 * Example 1  (http://www.yahoo.com):           
 *    TA_WebPage *webPage;
 *    TA_WebPageAlloc( libHandle, "www.yahoo.com",
 *                     NULL, NULL, NULL, &webPage, 1 );
 *
 * Example 2 (http://finance.yahoo.com/q?s=LNUX&d=v1):
 *    TA_WebPage *webPage;
 *    TA_WebPageAlloc( libHandle, "finance.yahoo.com",
 *                     "q?s=LNUX&d=v1", NULL, NULL, &webPage, 1 );
 *           
 * If no proxy is used, 'proxyName' and 'proxyPort' shall
 * be NULL. If 'proxyName' is specified but not 'proxyPort',
 * port 80 is used as the default. Note: When these parameters
 * are NULL on Windows, the proxy settings (if applicable) are
 * retreived for you from the registry.
 *
 * webSiteAddr can be the dotted-decimal address as well (x.x.x.x)
 */
TA_RetCode TA_WebPageAlloc( TA_Libc       *libHandle,
                            const char    *webSiteAddr,
                            const char    *webSitePage,
                            const char    *proxyName,
                            const char    *proxyPort,
                            TA_WebPage   **webPageAllocated,
                            unsigned int   nbAttempt );

/* When TA_WebPageAlloc return TA_SUCCESS, the
 * allocated web page must be eventually freed
 * with TA_WebPageFree.
 */
TA_RetCode TA_WebPageFree( TA_WebPage *webPage );

#endif

