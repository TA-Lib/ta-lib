/* Links against the SHARED library's import library and calls a handful of
 * entry points. Nothing else in this project ever links the Windows DLL -- the
 * dist tests check that ta-lib.dll and ta-lib.lib EXIST, and the Python wrapper
 * static-links ta-lib-static.lib -- so a prototype missing TA_LIB_API produces a
 * DLL without that symbol and no gate notices. That shipped once already:
 * TA_GetVersionString was absent from the DLL from 2002 until 0.7.1 (#57).
 *
 * Not a functional test. It fails to LINK when an entry point is missing from
 * the DLL, which is the failure no other job can see.
 */
#include <stdio.h>
#include "ta_libc.h"

int main( void )
{
   const char *version;
   double in[64], out[64];
   int i, outBeg = 0, outNb = 0;

   /* #57: absent from the DLL for 24 years. */
   version = TA_GetVersionString();
   if( version == NULL )
   {
      printf( "FAIL: TA_GetVersionString returned NULL\n" );
      return 1;
   }

   /* #400: re-exported with TA_LIB_API, so the DLL carries them for the first
    * time. Inert since #388 -- linking is the whole point, not the answer. */
   if( TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT ) != TA_SUCCESS ||
       TA_GetCompatibility() != TA_COMPATIBILITY_DEFAULT )
   {
      printf( "FAIL: the compatibility pair linked but did not behave\n" );
      return 1;
   }

   if( TA_Initialize() != TA_SUCCESS )
   {
      printf( "FAIL: TA_Initialize\n" );
      return 1;
   }

   for( i = 0; i < 64; i++ )
      in[i] = (double)i;

   if( TA_SMA( 0, 63, in, 5, &outBeg, &outNb, out ) != TA_SUCCESS || outNb != 60 )
   {
      printf( "FAIL: TA_SMA through the DLL (outNb=%d)\n", outNb );
      TA_Shutdown();
      return 1;
   }

   TA_Shutdown();
   printf( "ok: linked and ran against the DLL, version %s\n", version );
   return 0;
}
