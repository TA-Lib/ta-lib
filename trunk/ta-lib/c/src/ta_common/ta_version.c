/* Version number controlled manually.
 *
 * Should be modified only by TA-Lib.org
 */
#define MAJOR 0
#define MINOR 0
#define BUILD 6

#define VERSION_STRING "0.0.6"

/* Nothing to modify below this line. */

const char  *TA_GetVersionString( void )
{
    return VERSION_STRING;
}

unsigned int TA_GetVersionMajor( void )
{
   return MAJOR;
}

unsigned int TA_GetVersionMinor( void )
{
   return MINOR;
}

unsigned int TA_GetVersionBuild( void )
{
   return BUILD;
}
