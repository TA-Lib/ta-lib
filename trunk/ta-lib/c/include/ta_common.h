
/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
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

/* Some functions to get the version of TA-Lib.
 *
 * Format is "Major.Minor.Build (Month Day Year Hour:Min:Sec)"
 * 
 * Example: "0.1.2 (Jan 17 2004 23:59:59)"
 *
 * Major increments indicates an "Highly Recommended" update.
 * 
 * Minor increments indicates arbitrary milestones in the
 * development of the next major version or bug fixes
 * that an end-user should seriously consider.
 *
 * Build increments is for minor bug fixes and transitional
 * development. Only contributors should care about these
 * builds.
 */
const char *TA_GetVersionString( void );

/* Get individual component of the Version string */
const char *TA_GetVersionMajor ( void );
const char *TA_GetVersionMinor ( void );
const char *TA_GetVersionBuild ( void );
const char *TA_GetVersionDate  ( void );
const char *TA_GetVersionTime  ( void );

/* Misc. declaration used throughout the library code. */
typedef double TA_Real;
typedef int    TA_Integer;

typedef struct {
   long date;
   long time;
} TA_Timestamp;

typedef enum
{
  /* The history can have a precision of up to 1
   * second per bar.
   */
  TA_1SEC   = 1,
  TA_2SECS  = 2*TA_1SEC,
  TA_3SECS  = 3*TA_1SEC,
  TA_4SECS  = 4*TA_1SEC,
  TA_5SECS  = 5*TA_1SEC,
  TA_6SECS  = 6*TA_1SEC,
  TA_10SECS = 10*TA_1SEC,
  TA_12SECS = 12*TA_1SEC,
  TA_15SECS = 15*TA_1SEC,
  TA_20SECS = 20*TA_1SEC,
  TA_30SECS = 30*TA_1SEC,

  TA_1MIN   = 60*TA_1SEC,
  TA_2MINS  = 2*TA_1MIN,
  TA_3MINS  = 3*TA_1MIN,
  TA_4MINS  = 4*TA_1MIN,
  TA_5MINS  = 5*TA_1MIN,
  TA_6MINS  = 6*TA_1MIN,
  TA_10MINS = 10*TA_1MIN,
  TA_12MINS = 12*TA_1MIN,
  TA_15MINS = 15*TA_1MIN,
  TA_20MINS = 20*TA_1MIN,
  TA_30MINS = 30*TA_1MIN,
    
  TA_1HOUR   = 60*TA_1MIN,
  TA_2HOURS  = 2*TA_1HOUR,
  TA_3HOURS  = 3*TA_1HOUR,
  TA_4HOURS  = 4*TA_1HOUR,
  TA_6HOURS  = 6*TA_1HOUR,
  TA_8HOURS  = 8*TA_1HOUR,
  TA_12HOURS = 12*TA_1HOUR,

  TA_DAILY      = 32700,
  TA_WEEKLY     = 32710,
  TA_MONTHLY    = 32720,
  TA_QUARTERLY  = 32730,
  TA_YEARLY     = 32740

} TA_Period;

/* General purpose structure containing an array of string. 
 *
 * Example of usage:
 *    void printStringTable( TA_StringTable *table )
 *    {
 *       int i;
 *       for( i=0; i < table->size; i++ )
 *          cout << table->string[i] << endl;
 *    }
 *
 */
typedef struct
{
    unsigned int size;    /* Number of string. */
    const char **string;  /* Pointer to the strings. */

   /* Hidden data for internal use by TA-Lib. Do not modify. */
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

unsigned int TA_GetHour   ( const TA_Timestamp *timestamp ); /* [0..23] */
unsigned int TA_GetMin    ( const TA_Timestamp *timestamp ); /* [0..59] */
unsigned int TA_GetSec    ( const TA_Timestamp *timestamp ); /* [0..59] */

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
 * It is recommended to always use 4 digits for 'year'.
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

/* Align the timestamp according to bars period. --AK--
 * Limitations: period<= TA_12HOURS
 */
TA_RetCode TA_TimestampAlign( TA_Timestamp *dest, const TA_Timestamp *src, TA_Period period );


/* Validate only the year/month/day */
TA_RetCode TA_TimestampValidateYMD( const TA_Timestamp *timestamp );

/* Validate only the hour/min/sec */
TA_RetCode TA_TimestampValidateHMS( const TA_Timestamp *timestamp );

/* Some comparison functions. */
int TA_TimestampEqual  ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampLess   ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampGreater( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampCompare( const TA_Timestamp *t1, const TA_Timestamp *t2 );

/* Comparison but only of the "date" not of the "time". */
int TA_TimestampDateEqual  ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampDateLess   ( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampDateGreater( const TA_Timestamp *t1, const TA_Timestamp *t2 );
int TA_TimestampDateCompare( const TA_Timestamp *t1, const TA_Timestamp *t2 );

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

/* Move the timestamp forward/backward by some time "delta"
 * which is specified in seconds                     --AK--
 * Approximate limitations: -30 years < delta < 30 years
 */
TA_RetCode TA_AddTimeToTimestamp( TA_Timestamp *dest, 
                                  const TA_Timestamp *src,
                                  const int delta);

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

TA_RetCode TA_TimestampDeltaIntraday( const TA_Timestamp *t1,
                                      const TA_Timestamp *t2,
                                      unsigned int *delta,
                                      TA_Period old_period,
                                      TA_Period new_period);

/* TA_Initialize() initialize the ressources used by TA-Lib. This
 * function must be called once prior to any other functions declared in
 * this file.
 *
 * TA_Shutdown() allows to free all ressources used by TA-Lib. Following
 * a shutdown, TA_Initialize() must be called again for re-using TA-Lib.
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

/* Output the information recorded on the occurence of a TA_FATAL_ERR. 
 * Can be output to a file or stdio.
 *
 * This output shall be usually forwarded to the library developper for
 * further analysis ( http://ta-lib.org ).
 *
 * TA-Lib records only the first occurence of a fatal error. It is 
 * assumed that the application will properly exit following the
 * first detection of a fatal error.
 *
 * Example:
 *    TA_RetCode retCode;
 *    retCode = TA_HistoryAlloc( .... );
 *    if( retCode == TA_FATAL_ERR )
 *    {
 *       TA_FatalReport( stderr );
 *       exit(-1);
 *    }
 */
void TA_FatalReport( FILE *out );

/* You can choose to output the fatal error information
 * into a provided buffer.
 *
 * The output will be NULL terminated. No character more than
 * bufferSize will be written.
 */
#define TA_FATAL_ERROR_BUF_SIZE 2048
void TA_FatalReportToBuffer( char *buffer, unsigned int bufferSize );

/* You can provide your own handler to intercept fatal error. 
 *
 * The handler is called by TA-Lib on the first occurent of a fatal 
 * error. You can use printf or whatever you want in the handler, but
 * no TA-Lib function other than TA_FatalReport and TA_FatalReportToBuffer
 * can be called (to avoid recursive call if the failure persist!).
 * DO NOT CALL TA_Shutdown/TA_Initialize from the handler. A fatal error
 * really means that you should exit. You just get here the opportunity
 * to record or display some information and properly free other
 * application ressources before a relatively clean exit.
 *
 * Example:
 *    Whenever a fatal error occured, we wish to append the log of the 
 *    fatal error in a file, display it to the user and exit.
 *
 *    void myFatalErrorHandler( void )
 *    {
 *        // Append to a file
 *        FILE *out;
 *        out = fopen( "fatal.log", "w+" );
 *        if( out )
 *        {
 *           TA_FatalReport( out );
 *           fclose(out);
 *        }
 *
 *        // Show to the user in a dialog that the
 *        // application will be shutdown.
 *        char myBuffer[100];
 *        TA_FatalReportToBuffer( myBuffer, 100 );
 *        myDisplayErrorDialog( myBuffer );
 *
 *        // Free any possible ressource use by 
 *        // your application. Do NOT call TA_Shutdown().
 *        ...
 *
 *        // It is recommended to exit the application now. 
 *        exit(-1);
 *    }
 *
 *    int main( )
 *    {
 *        ...
 *        TA_Initialize(...);
 *        TA_SetFatalErrorHandler( myFatalErrorHandler );
 *        ...
 *    }
 */
typedef void (*TA_FatalHandler)( void );

TA_RetCode TA_SetFatalErrorHandler( TA_FatalHandler handler );

/* Function used exclusively by TA-Lib developers to perform
 * regression testing.
 *
 * Most (if not all) TA-Lib users should ignore this function.
 *
 * The Fatal and assert error are simulated ones and the
 * stability of the application is not affected. Meaning
 * that the application do not need to exit and normal
 * operation with TA-Lib can resume.
 */
typedef enum
{
   TA_REG_TEST_FATAL_ERROR,
   TA_REG_TEST_ASSERT_FAIL,
   TA_NB_REGRESSION_TEST_ID   
} TA_RegressionTestId;

TA_RetCode TA_RegressionTest( TA_RegressionTestId id );

#ifdef __cplusplus
}
#endif

#endif
