/* Simple command line utility for fetching a web site.
 *
 * The raw data is displayed and can be redirected to
 * a file.
 *
 * This application is just for giving a chance to
 * the fetch engine used within the TA-LIB project
 * to be more widely used and tested.
 * 
 * Info: http:\\ta-lib.org
 *
 */
#include <stdio.h>
#include "ta_libc.h"

/* Prototype */
static void print_usage( const char *str );
static void print_error( TA_RetCode retCode );

int main( int argc, char **argv )
{
   TA_Libc *libHandle;
   TA_RetCode retCode;
   int retValue;

   /* Verify the parameters. */
   if( argc != 2 )
   {
      print_usage( "Bad Parameter" );
      return -1;
   }

   /* Initialize TA-LIB. */
   retCode = TA_Initialize( &libHandle, NULL );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   /* Fetch and display the WebPage. */
   retCode = TA_WebFetch( libHandle, argv[1], stdout );

   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      retValue = -1;
   }
   else
      retValue = 0;
      
   /* Clean-up and exit. */
   TA_Shutdown( libHandle );

   return retValue;
}

static void print_usage( const char *str ) 
{
   printf( "\n" );
   printf( "WebFetch V%s - Fetch data from a specified URL\n", TA_GetVersionString() );
   printf( "\n" );
   printf( "Usage: WebFetch URL\n" );
   printf( "\n" );
   printf( "   The text content of the URL is simply displayed\n" );
   printf( "   and can be redirected to a file.\n" );
   printf( "\n" );
   printf( "   Examples: WebFetch http://www.yahoo.com\n" );
   printf( "             WebFetch finance.yahoo.com/mt\n" );
   printf( "             WebFetch www.yahoo.com >myFile.html\n" );
   printf( "\n" );
   printf( "   On failure the first string displayed is\n" );
   printf( "   always \"WebFetch\".\n" );
   printf( "\n" );
   printf( "   Exit code is 0 on success.\n" );
   printf( "   Exit code is -1 on failure.\n" );
   printf( "\n" );
   printf( "   To uninstall, just delete WebFetch.exe\n" );
   printf( "\n" );
   printf( "   WebFetch.exe is open-source and is a small\n" );
   printf( "   component of a larger project called TA-LIB.\n" );
   printf( "\n" );
   printf( "   More info at http://ta-lib.org\n" );
   printf( "\n" );
   printf( "Error: [%s]\n", str );
}

static void print_error( TA_RetCode retCode )
{
   TA_RetCodeInfo retCodeInfo;

   /* Ask TA-LIB to provide an error string
    * and the enum that can be used to find
    * the origin of the failure within the
    * source code.
    */
   TA_SetRetCodeInfo( retCode, &retCodeInfo );
   printf( "\nWebFetch Fail: %d=%s:[%s]\n",
           retCode,
           retCodeInfo.enumStr,
           retCodeInfo.infoStr );
}
