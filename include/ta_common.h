
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
#ifndef TA_COMMON_H
#define TA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <limits.h>
#include <float.h>

#ifndef TA_DEFS_H
   #include "ta_defs.h"
#endif

/* Some functions to get the version of the TA-LIB being link. 
 * The string format is "0.0.0" for "Major.Minor.Build"
 *
 * Major increments indicates an "Highly Recommended" update
 *
 * Minor increments indicates arbitrary milestones in the
 * development of the next major version or bug fixes
 * that an end-user should consider.
 *
 * Build increments is for minor bug fixes and transitional
 * development. Only contributors should care about these
 * builds.
 * 
 * A stable release will have a format "x.y.0" where the
 * build number is zero.
 *
 * Stable means:
 *   - All functions to which you can link are suppose to
 *     work as documented.
 *   - This version did pass the regression tests (ta_regtest)
 *     on all supported platform.
 *
 * It is strongly suggested that end-user use only stable version.
 */
const char  *TA_GetVersionString( void );

unsigned int TA_GetVersionMajor ( void );
unsigned int TA_GetVersionMinor ( void );
unsigned int TA_GetVersionBuild ( void );

/* Misc. declaration used throughout the library code. */
typedef double       TA_Real;
typedef int          TA_Integer;

typedef struct {
   long date;
   long time;
} TA_Timestamp;

typedef enum
{
  /* The history can have a precision of up to 1
   * second per bar.
   */
  TA_1SEC    = 1,
  TA_1MIN    = 60,
  TA_5MIN    = 300,
  TA_10MIN   = 600,
  TA_15MIN   = 900,
  TA_30MIN   = 1800,
  TA_HOURLY  = 3600,

  TA_DAILY      = 32700,
  TA_WEEKLY     = 32710,
  TA_MONTHLY    = 32720,
  TA_QUARTERLY  = 32730,
  TA_YEARLY     = 32740
} TA_Period;

typedef struct
{
   unsigned int nbBars; /* Nb of element into the following arrays. */
   TA_Period period;    /* Amount of time between each bar. */

   /* The arrays containing data. Unused array are set to NULL. */
   const TA_Timestamp *timestamp;
   const TA_Real      *open;
   const TA_Real      *high;
   const TA_Real      *low;
   const TA_Real      *close;
   const TA_Integer   *volume;
   const TA_Integer   *openInterest;

   /* Hidden data for internal use by the TA-LIB. Do not modify. */
   void *hiddenData;
} TA_History;

/* General purpose structure giving access to tables/index by name. */
typedef struct
{
    unsigned int size;    /* Number of string. */
    const char **string;  /* Pointer to the strings. */

   /* Hidden data for internal use by the TA-LIB. Do not modify. */
   void *hiddenData;
} TA_StringTable;

/* End-user can get additional information related to a TA_RetCode. 
 *
 * Example:
 *        TA_RetCodeInfo info;
 *
 *        retCode = TA_Initialize( ... );
 *
 *        if( retCode != TA_SUCCESS )
 *        {
 *           TA_SetRetCodeInfo( retCode, &info );
 *           printf( "Error %d(%s): %s\n",
 *                   retCode,
 *                   info.enumStr,
 *                   info.infoStr );
 *        }
 *
 * Would display:
 *        "Error 1(TA_LIB_NOT_INITIALIZE): TA_Initialize was not sucessfully called"
 */
typedef struct
{
   const char *enumStr; /* Like "TA_IP_SOCKETERROR"     */
   const char *infoStr; /* Like "Error creating socket" */      
} TA_RetCodeInfo;

/* Info is always returned, even when 'theRetCode' is invalid. */
void TA_SetRetCodeInfo( TA_RetCode theRetCode, TA_RetCodeInfo *retCodeInfo );
 
/* The following functions are used for using TA_Timestamp. */
unsigned int TA_GetYear   ( const TA_Timestamp *timestamp ); /* [1600..9999] */
unsigned int TA_GetMonth  ( const TA_Timestamp *timestamp ); /* [1..12] */
unsigned int TA_GetDay    ( const TA_Timestamp *timestamp ); /* [1..31] */

unsigned int TA_GetHour   ( const TA_Timestamp *timestamp ); /* [1..23] */
unsigned int TA_GetMin    ( const TA_Timestamp *timestamp ); /* [1..59] */
unsigned int TA_GetSec    ( const TA_Timestamp *timestamp ); /* [1..59] */

typedef enum
{ 
   TA_SUNDAY   = 0,
   TA_MONDAY   = 1,
   TA_TUESDAY  = 2,
   TA_WEDNESDAY= 3,
   TA_THURSDAY = 4,
   TA_FRIDAY   = 5,
   TA_SATURDAY = 6 
} TA_DayOfWeek;

TA_DayOfWeek TA_GetDayOfTheWeek    ( const TA_Timestamp *timestamp ); /* [0..6] */
unsigned int TA_GetDayOfTheYear    ( const TA_Timestamp *timestamp ); /* [0..365] '0' is January 1 */
unsigned int TA_GetWeekOfTheYear   ( const TA_Timestamp *timestamp ); /* [0..52]  '0' if first week incomplete, else start at '1' */
unsigned int TA_GetQuarterOfTheYear( const TA_Timestamp *timestamp ); /* [0..3] */


TA_RetCode TA_GetDate( const TA_Timestamp *timestamp,
                       unsigned int *year,
                       unsigned int *month,
                       unsigned int *day );

TA_RetCode TA_GetTime( const TA_Timestamp *timestamp,
                       unsigned int *hour,
                       unsigned int *min,
                       unsigned int *sec );

TA_RetCode TA_SetDate( unsigned int year,
                       unsigned int month,
                       unsigned int day,
                       TA_Timestamp *timestamp );

TA_RetCode TA_SetTime( unsigned int hour,
                       unsigned int min,
                       unsigned int sec,
                       TA_Timestamp *timestamp );

/* With TA_SetDate, the following translation are done when
 * the 'year' parameter is less than 100.
 *
 *   00 to 10 becomes 2000 to 2010
 *   11 to 99 becomes 1911 to 1999
 *
 * It is strongly suggest to make all your source 4 digits.
 */

/* Default is: Jan 1st 1900, 00:00:00 */
TA_RetCode TA_SetDefault( TA_Timestamp *timestamp );

/* Set the timestamp with the current computer host date.
 * Time is left untouch.
 */
TA_RetCode TA_SetDateNow( TA_Timestamp *timestamp );

/* Set the timestamp with the current computer host time.
 * Date is left untouch.
 */
TA_RetCode TA_SetTimeNow( TA_Timestamp *timestamp );

/* A valid timestamp must have the year/month/day/hour/min/sec correctly
 * initialized with the TA_SetXXXXX() functions.
 *
 * In case of doubt, the following function allows to validate a timestamp.
 */
TA_RetCode TA_TimestampValidate( const TA_Timestamp *timestamp );

/* Some comparison functions. */
int TA_TimestampEqual  ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampLess   ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampGreater( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampCompare( const TA_Timestamp *t1, const TA_Timestamp *t2 );

/* Move the timestamp to the next day. Will continue to
 * the next month and/or year if needed.
 */
TA_RetCode TA_NextDay( TA_Timestamp *timestamp );

/* Move the timestamp one year forward. 
 * 
 * This is exactly 365 days in the future. There
 * is no guarantee that it will be the exact
 * same month / day of the month because of
 * leap year.
 */
TA_RetCode TA_NextYear( TA_Timestamp *timestamp );

/* Move the timestamp to the previous year. 
 * 
 * This is exactly 365 days in the past. There
 * is no guarantee that it will be the exact
 * same month / day of the month because of
 * leap years.
 */
TA_RetCode TA_PrevYear( TA_Timestamp *timestamp );

/* Move the timestamp to the previous day. Will adjust to
 * the previous month and/or year if needed.
 */
TA_RetCode TA_PrevDay( TA_Timestamp *timestamp );

/* Move the timestamp to the next week-day.
 * After Friday comes Monday...
 */
TA_RetCode TA_NextWeekday( TA_Timestamp *timestamp );

/* Move the timestamp to the previous week-day.
 * Before Monday is Friday...
 */
TA_RetCode TA_PrevWeekday( TA_Timestamp *timestamp );

/* Jump forward to the last day of the current month.
 * Won't move if already on the last day of the month.
 */
TA_RetCode TA_JumpToEndOfMonth( TA_Timestamp *timestamp );

/* Jump backward to the first day of the current month.
 * Won't move if already on the first day of the month.
 */
TA_RetCode TA_BackToBeginOfMonth( TA_Timestamp *timestamp );

/* Jump forward to the last day of the quarter.
 * Won't move if already on the last day of the
 * quarter.
 *   Quarter 0:  3/31
 *   Quarter 1:  6/30
 *   Quarter 2:  9/30
 *   Quarter 3: 12/31
 */
TA_RetCode TA_JumpToEndOfQuarter( TA_Timestamp *timestamp );

/* Jump backward to the first day of the current quarter.
 * Won't move if already on the first day of the
 * quarter.
 *   Quarter 0:  1/1
 *   Quarter 1:  4/1
 *   Quarter 2:  7/1
 *   Quarter 3: 10/1
 */
TA_RetCode TA_BackToBeginOfQuarter( TA_Timestamp *timestamp );

/* Jump forward to the last day of the year.
 * (December 31)
 */
TA_RetCode TA_JumpToEndOfYear( TA_Timestamp *timestamp );

/* Jump backward to the first day of the year.
 * (January 1st).
 */
TA_RetCode TA_BackToBeginOfYear( TA_Timestamp *timestamp );

/* Jump forward to the specified day of the week. 
 * Won't move if already on the specified day of the week.
 * Time component of the timestamp is not modified.
 */
TA_RetCode TA_JumpToDayOfWeek( TA_Timestamp *timestamp,
                               TA_DayOfWeek  dayOfWeek );

/* Jump backward to the specified day of the week.
 * Won't move if already on the specified day of the week.
 * Time component of the timestamp is not modified.
 */
TA_RetCode TA_BackToDayOfWeek( TA_Timestamp *timestamp,
                               TA_DayOfWeek  dayOfWeek );

/* Make a copy of the value of a timestamp. */
TA_RetCode TA_TimestampCopy( TA_Timestamp *dest, const TA_Timestamp *src );

/* Evaluate how many complete period can fit between
 * two timestamps (timestamp days are inclusive).
 *
 * Example:
 *    TA_TimestampMaxDeltaWeek( t1, t2, &delta );
 *    will calculate the number of complete week
 *    between t1 and t2 (first last date inclusive).
 *
 * The chronological order of t1 and t2 is not important.
 * The result is always positive and written in the 
 * variable pointed by 'delta'.
 *    
 * Because partial period are not accounted for, the
 * caller should often consider to add '2' to be on
 * the safe side.
 */
TA_RetCode TA_TimestampDeltaWeek ( const TA_Timestamp *t1,
                                   const TA_Timestamp *t2,
                                   unsigned int *delta );

TA_RetCode TA_TimestampDeltaMonth( const TA_Timestamp *t1,
                                   const TA_Timestamp *t2,
                                   unsigned int *delta );

TA_RetCode TA_TimestampDeltaYear ( const TA_Timestamp *t1,
                                   const TA_Timestamp *t2,
                                   unsigned int *delta );
           
TA_RetCode TA_TimestampDeltaDay( const TA_Timestamp *t1,
                                 const TA_Timestamp *t2,
                                 unsigned int *delta );

TA_RetCode TA_TimestampDeltaWeekday( const TA_Timestamp *t1,
                                     const TA_Timestamp *t2,
                                     unsigned int *delta );

TA_RetCode TA_TimestampDeltaQuarter( const TA_Timestamp *t1,
                                     const TA_Timestamp *t2,
                                     unsigned int *delta );

/* TA_Initialize() initialize the ressources used by the TA-LIB. This
 * function must be called once prior to any other functions declared in
 * this file.
 *
 * TA_Shutdown() allows to free all ressources used by the TA-LIB. Following
 * a shutdown, TA_Initialize() must be called again for re-using the TA-LIB.
 *
 * TA_Shutdown() should be called prior to exiting the application code.
 */
typedef struct
{
   /* The whole structure should be first initialize
    * with zero, and only the relevant parameter
    * to your application needs to be set.
    *
    * The safest way is to ALWAYS do something like:
    *    TA_InitializeParam param;
    *
    *    memset( &param, 0, sizeof( TA_InitializeParam ) );
    *    ... set only the parameter you need ...
    *    retCode = TA_Initialize( &param ); 
    *
    * Initializing the whole structure to zero assure
    * that the actual (or future) unused parameters
    * won't interfere.
    */
   FILE *logOutput;
   const char *userLocalDrive;
} TA_InitializeParam;

TA_RetCode TA_Initialize( const TA_InitializeParam *param );

TA_RetCode TA_Shutdown( void );

/* Output the information recorded on the last occurence of a TA_FATAL_ERR. 
 * Can be output to a file or stdio.
 *
 * This output shall be usually forwarded to the library developper for
 * further analysis.
 *
 * Example:
 *    TA_RetCode retCode;
 *    retCode = TA_HistoryAlloc( .... );
 *    if( retCode == TA_FATAL_ERR )
 *       TA_FatalReport( stderr );
 */
void TA_FatalReport( FILE *out );

/* You can also output the log into a provided buffer.
 * TA_FATAL_ERROR_BUF_SIZE is the recommended size in byte.
 * The returned buffer will be NULL terminated.
 */
#define TA_FATAL_ERROR_BUF_SIZE 1024
void TA_FatalReportToBuffer( char *buffer, unsigned int buffferSize );

/* You can provide your own handler to intercept fatal error. 
 * You can use printf or whatever you want in the handler, but
 * no TA-LIB function other than TA_FatalReport can be called
 * within this handler (to avoid recursive call if the failure
 * persist!). 
 *
 * Example:
 *    Whenever a fatal error occured, we wish to increment a global
 *    variable and append the log of the fatal error in a file.
 *
 *    int nbFatalError = 0;
 *    ...
 *
 *    void myFatalErrorHandler( void )
 *    {
 *        FILE *out;
 *
 *        nbFatalError++;
 *
 *        out = fopen( "fatal.log", "w+" );
 *        if( out )
 *        {
 *           TA_FatalReport( out );
 *           fclose(out);
 *        }
 *    }
 *    ...
 *
 *    TA_SetFatalErrorHandler( myFatalErrorHandler );
 */
typedef void (*TA_FatalHandler)( void );

TA_RetCode TA_SetFatalErrorHandler( TA_FatalHandler handler );

#ifdef __cplusplus
}
#endif

#endif
