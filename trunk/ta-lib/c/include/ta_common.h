
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
#ifndef TA_COMMON_H
#define TA_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <limits.h>
#include <float.h>

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

/* min/max value for a TA_Integer */
#define TA_INTEGER_MIN ((TA_Integer)INT_MIN+1)
#define TA_INTEGER_MAX ((TA_Integer)INT_MAX)

/* min/max value for a TA_Real 
 *
 * Use fix value making sense in the
 * context of TA-Lib (avoid to use DBL_MIN
 * and DBL_MAX standard macro because they
 * are troublesome with some compiler).
 */
#define TA_REAL_MIN ((TA_Real)-3e+37)
#define TA_REAL_MAX ((TA_Real)3e+37)

/* A value outside of the min/max range 
 * indicates an undefined or default value.
 */
#define TA_INTEGER_DEFAULT ((TA_Integer)INT_MIN)
#define TA_REAL_DEFAULT    ((TA_Real)-4e+37)

typedef struct {
   long date;
   long time;
} TA_Timestamp;

typedef unsigned int TA_Libc;

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

typedef enum
{
    /****** TA-LIB Error Code *****/
    /*   0 */  TA_SUCCESS,            /* No error */
    /*   1 */  TA_LIB_NOT_INITIALIZE, /* TA_Initialize was not sucessfully called */
    /*   2 */  TA_BAD_PARAM,          /* A parameter is out of range */
    /*   3 */  TA_ALLOC_ERR,          /* Possibly out-of-memory */
    /*   4 */  TA_ACCESS_FAILED,      /* File system access failed */
    /*   5 */  TA_NO_DATA_SOURCE,     /* No data was added */
    /*   6 */  TA_SYMBOL_NOT_FOUND,   /* Symbol not found in this category */
    /*   7 */  TA_CATEGORY_NOT_FOUND, /* Category not found in the data base */
    /*   8 */  TA_KEY_NOT_FOUND,
    /*   9 */  TA_INDEX_FILE_NOT_ACCESSIBLE,
    /*  10 */  TA_INDEX_NOT_VALID,
    /*  11 */  TA_INVALID_FIELD,
    /*  12 */  TA_INVALID_PATH,
    /*  13 */  TA_INTERNAL_ERR,
    /*  14 */  TA_FATAL_ERR,
    /*  15 */  TA_NO_NEW_DATA,
    /*  16 */  TA_NOT_SUPPORTED,
    /*  17 */  TA_END_OF_INDEX,
    /*  18 */  TA_ENOUGH_DATA,
    /*  19 */  TA_MISSING_FIELD,
    /*  20 */  TA_REDUNDANT_FIELD,
    /*  21 */  TA_INVALID_DATE,
    /*  22 */  TA_INVALID_PRICE,
    /*  23 */  TA_GROUP_NOT_FOUND,
    /*  24 */  TA_FUNC_NOT_FOUND,
    /*  25 */  TA_INVALID_HANDLE,
    /*  26 */  TA_INVALID_PARAM_HOLDER,
    /*  27 */  TA_INVALID_PARAM_HOLDER_TYPE,
    /*  28 */  TA_INVALID_PARAM_FUNCTION,
    /*  29 */  TA_INPUT_NOT_ALL_INITIALIZE,
    /*  30 */  TA_OUTPUT_NOT_ALL_INITIALIZE,
    /*  31 */  TA_OUT_OF_RANGE_START_INDEX,
    /*  32 */  TA_OUT_OF_RANGE_END_INDEX,
    /*  33 */  TA_BAD_OBJECT,
    /*  34 */  TA_MEM_LEAK,
    /*  35 */  TA_ADDR_NOT_FOUND,
    /*  36 */  TA_SOCKET_LIB_INIT_ERR,
    /*  37 */  TA_END_OF_STREAM,
    /*  38 */  TA_BAD_STREAM_CRC,
    /*  39 */  TA_UNSUPPORTED_STREAM_VERSION,
    /*  40 */  TA_BAD_STREAM_HEADER_CRC,
    /*  41 */  TA_BAD_STREAM_HEADER,
    /*  42 */  TA_BAD_STREAM_CONTENT,
    /*  43 */  TA_BAD_YAHOO_IDX_HDR,
    /*  44 */  TA_UNSUPORTED_YAHOO_IDX_VERSION,
    /*  45 */  TA_BAD_YAHOO_IDX_INDICATOR_AF,
    /*  46 */  TA_BAD_YAHOO_IDX_INDICATOR_EB,
    /*  47 */  TA_BAD_YAHOO_IDX_INDICATOR_F2,
    /*  48 */  TA_NO_INTERNET_CONNECTION,
    /*  49 */  TA_INTERNET_ACCESS_FAILED,
    /*  50 */  TA_INTERNET_OPEN_FAILED,
    /*  51 */  TA_INTERNET_NOT_OPEN_TRY_AGAIN,
    /*  52 */  TA_INTERNET_SERVER_CONNECT_FAILED,
    /*  53 */  TA_INTERNET_OPEN_REQUEST_FAILED,
    /*  54 */  TA_INTERNET_SEND_REQUEST_FAILED,
    /*  55 */  TA_INTERNET_READ_DATA_FAILED,
    /*  56 */  TA_UNSUPPORTED_COUNTRY,
    /*  57 */  TA_BAD_HTML_SYNTAX,
    /*  58 */  TA_PERIOD_NOT_AVAILABLE,
    /*  59 */  TA_FINISH_TABLE,
    /*  60 */  TA_INVALID_SECURITY_EXCHANGE,
    /*  61 */  TA_INVALID_SECURITY_SYMBOL,
    /*  62 */  TA_INVALID_SECURITY_COUNTRY,
    /*  63 */  TA_INVALID_SECURITY_TYPE,
    /*  64 */  TA_MISSING_DATE_OR_TIME_FIELD,
    /*  65 */  TA_OBJECT_NOT_EQUAL,
    /*  66 */  TA_INVALID_LIST_TYPE,
    /*  67 */  TA_YAHOO_IDX_EXPIRED,
    /*  68 */  TA_YAHOO_IDX_UNAVAILABLE_1, /* Local cache does not have a Yahoo! index */
    /*  69 */  TA_YAHOO_IDX_FAILED,
    /*  70 */  TA_LIBCURL_GLOBAL_INIT_FAILED, /* libCurl shared library missing? */
    /*  71 */  TA_LIBCURL_INIT_FAILED,        /* libCurl shared library missing? */
    /*  72 */  TA_INSTRUMENT_ID_BAD,          /* TA_Instrument not initialized. */
    /*  73 */  TA_TRADE_LOG_NOT_INITIALIZED,  /* TA_TradeLog corrupted or not initialized */
    /*  74 */  TA_BAD_TRADE_TYPE,
    /*  75 */  TA_BAD_START_DATE,
    /*  76 */  TA_BAD_END_DATE,
    /*  77 */  TA_INTERNET_SET_RX_TIMEOUT_FAILED,
    /*  78 */  TA_NO_TRADE_LOG,
    /*  79 */  TA_ENTRY_TRANSACTION_MISSING,
    /*  80 */  TA_INVALID_VALUE_ID,
    /*  81 */  TA_BAD_STARTING_CAPITAL,
    /*  82 */  TA_TRADELOG_ALREADY_ADDED, /* TA_TradeLog already added to a TA_PM. When that TA_PM is freed, the TA_TradeLog can be added again. */
    /*  83 */  TA_YAHOO_IDX_UNAVAILABLE_2, /* Possibly timeout of internet connection */
    /*  84 */  TA_YAHOO_IDX_UNAVAILABLE_3, /* Failed to find a Yahoo! index */
    /*  85 */  TA_NO_WEEKDAY_IN_DATE_RANGE,
    /*  86 */  TA_VALUE_NOT_APPLICABLE,    /* This PM value is not applicable to these trades. */
    /*  87 */  TA_DATA_GAP, /* Data source returned data with gaps */

    /****** IP Error Code *****/
    /* 700 */  TA_IP_NOSOCKETS = 700,  /* Sockets not supported      */
    /* 701 */  TA_IP_BADHOST,          /* Host not known             */
    /* 702 */  TA_IP_BADSERVICE,       /* Service or port not known  */
    /* 703 */  TA_IP_BADPROTOCOL,      /* Invalid protocol specified */
    /* 704 */  TA_IP_SOCKETERROR,      /* Error creating socket      */
    /* 705 */  TA_IP_CONNECTERROR,     /* Error making connection    */
    /* 706 */  TA_IP_BINDERROR,        /* Error binding socket       */
    /* 707 */  TA_IP_LISTENERROR,      /* Error preparing to listen  */
    /* 708 */  TA_IP_WRITE_ERROR,      /* Error writing to socket    */
    /* 709 */  TA_IP_READ_ERROR,       /* Error reading from socket  */
    /* 710 */  TA_IP_UNKNOWN_ERR,

    /****** HTTP Error Code (RFC1945) *****/
    /* 800 */  TA_HTTP_NO_HEADER = 800, /* HTTP header not found.     */
    /* 801 */  TA_HTTP_SC_301,          /* Moved Permanently          */
    /* 802 */  TA_HTTP_SC_302,          /* Moved Temporarily          */
    /* 803 */  TA_HTTP_SC_304,          /* Not Modified               */
    /* 804 */  TA_HTTP_SC_400,          /* Bad Request                */
    /* 805 */  TA_HTTP_SC_401,          /* Unauthorized               */
    /* 806 */  TA_HTTP_SC_403,          /* Forbidden                  */
    /* 807 */  TA_HTTP_SC_404,          /* Not Found                  */
    /* 808 */  TA_HTTP_SC_500,          /* Internal Server Error      */
    /* 809 */  TA_HTTP_SC_501,          /* Not Implemented            */
    /* 810 */  TA_HTTP_SC_502,          /* Bad Gateway                */
    /* 811 */  TA_HTTP_SC_503,          /* Service Unavailable        */
    /* 821 */  TA_HTTP_SC_UNKNOWN,      /* Unknown error code.        */ 

    /****** TA-LIB Internal Error Code *****/
    /* 5000 */ TA_INTERNAL_ERROR = 5000, /* Internal Error - Contact TA-Lib.org */

    TA_UNKNOWN_ERR = 0xFFFF
} TA_RetCode;

/* Id can be from 1 to 999 */
#define TA_INTERNAL_ERROR(Id) (TA_INTERNAL_ERROR+Id)

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
    *    TA_Libc *libHandle;
    *
    *    memset( &param, 0, sizeof( TA_InitializeParam ) );
    *    ... set only the parameter you need ...
    *    retCode = TA_Initialize( &libHandle, &param ); 
    *
    * Initializing the whole structure to zero assure
    * that the actual (or future) unused parameters
    * won't interfere.
    */
   FILE *logOutput;
   const char *userLocalDrive;
} TA_InitializeParam;

TA_RetCode TA_Initialize( TA_Libc **allocatedLibHandle,
                          const TA_InitializeParam *param );

TA_RetCode TA_Shutdown  ( TA_Libc *libHandleToFree );

/* Output the information recorded on the last occurence of a TA_FATAL_ERR. 
 * Can be output to a file or stdio.
 *
 * This output shall be usually forwarded to the library developper for
 * further analysis.
 *
 * Example:
 *    TA_RetCode retCode;
 *    retCode = TA_HistoryAlloc( libHandle, .... );
 *    if( retCode == TA_FATAL_ERR )
 *       TA_FatalReport( libHandle, stderr );
 */
void TA_FatalReport( TA_Libc *libHandle, FILE *out );

/* You can also output the log into a provided buffer.
 * TA_FATAL_ERROR_BUF_SIZE is the recommended size in byte.
 * The returned buffer will be NULL terminated.
 */
#define TA_FATAL_ERROR_BUF_SIZE 1024
void TA_FatalReportToBuffer( TA_Libc *libHandle, char *buffer, unsigned int buffferSize );

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
 *    void myFatalErrorHandler( TA_Libc *libHandle )
 *    {
 *        FILE *out;
 *
 *        nbFatalError++;
 *
 *        out = fopen( "fatal.log", "w+" );
 *        if( out )
 *        {
 *           TA_FatalReport( libHandle, out );
 *           fclose(out);
 *        }
 *    }
 *    ...
 *
 *    TA_SetFatalErrorHandler( libHandle, myFatalErrorHandler );
 */
typedef void (*TA_FatalHandler)( TA_Libc *libHandle );

TA_RetCode TA_SetFatalErrorHandler( TA_Libc *libHandle, TA_FatalHandler handler );

#ifdef __cplusplus
}
#endif

#endif
