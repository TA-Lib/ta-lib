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
 *  060102 MF   First version.
 *  030404 MF   Change TA_Transaction timestamp to execTimestamp
 */

/* Description:
 *    Test the Performance Measurement module.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ta_libc.h"
#include "ta_common.h"
#include "ta_test_priv.h"
#include "ta_system.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum
{
   TA_KEY_TYPE_INTEGER,
   TA_KEY_TYPE_CAT,
   TA_KEY_TYPE_SYM,
   TA_KEY_TYPE_CATSYM,
   NB_TA_KEY_TYPE
} TA_KEY_TYPE;

typedef struct
{
   /* The TA_PMValue parameters */
   TA_PMValueId id;
   TA_PMGroup   grp;

   /* The expected output */
   TA_RetCode   expectedRetCode;
   TA_Real      expectedValue;
} TA_PMValueIdCheck;

typedef struct
{
   /* The TA_PMArrayAlloc parameters. */
   TA_PMArrayId id;
   TA_PMGroup   grp;
   TA_Period    period;

   /* The expected output */
   const TA_Timestamp  date;
   TA_RetCode   expectedRetCode;
   TA_Real      expectedValue;
} TA_PMArrayIdCheck;

typedef struct
{
   /* The list of TA_Transaction */
   TA_Transaction *inputs;

   /* The parameter of the TA_PMAlloc */
   const TA_Timestamp  startDate;
   const TA_Timestamp  endDate;
   TA_Real             initialCapital;

   /* The list of TA_PMValueId to verify. */
   TA_PMValueIdCheck    *toCheck;
} TA_PMValueIdTest;

typedef struct
{
   /* The list of TA_Transaction */
   TA_Transaction *inputs;

   /* The parameter of the TA_PMAlloc */
   const TA_Timestamp  startDate;
   const TA_Timestamp  endDate;
   TA_Real             initialCapital;

   /* The list of TA_PMArrayId to verify. */
   TA_PMArrayIdCheck    *toCheck;
} TA_PMArrayIdTest;

/**** Local functions declarations.    ****/
static ErrorNumber checkPMvalues( TA_PM *pm,
                                  int nbLongTrade,
                                  int nbShortTrade,
                                  TA_Real longProfit,
                                  TA_Real shortProfit );

static ErrorNumber test_onetransaction_only( TA_KEY_TYPE keyTypeTest );

static ErrorNumber test_emptytradelog( void );

static ErrorNumber test_onetrade_only( TA_KEY_TYPE keyTypeTest,
                                       TA_TransactionType transactionType, 
                                       unsigned int winningTrade );

static ErrorNumber checkNoHang( TA_PM *pm );

static ErrorNumber test_report ( TA_PM *pm, unsigned int doDisplay );
static ErrorNumber test_valueId( TA_PMValueIdTest *test );
static ErrorNumber test_arrayId( TA_PMArrayIdTest *test );

/**** Local variables definitions.     ****/
static TA_Timestamp timestampNow;
static TA_Timestamp timestampNowPlusOneYear;


/* Instruments used throughout the tests. */
static TA_Instrument id1_1; /* Using a user key */
static TA_Instrument id1_2;

static TA_Instrument id2_1; /* Using a category / symbol strings. */
/*static TA_Instrument id2_2;*/

static TA_Instrument id3_1; /* Using a category only. */
static TA_Instrument id3_2; 

static TA_Instrument id4_1; /* Using only a symbol string */
static TA_Instrument id4_2;

/* List of transactions.
 * Date are longs: YYYYMMDD
 */


/* 5 transactions, same intsrument, same year. */
static TA_Transaction table1_1[] = {
   {TA_LONG_ENTRY, &id2_1, {20020101,0},  150, 10.00, 0, 0, 0 },
   {TA_LONG_ENTRY, &id2_1, {20020102,0},   50, 10.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id2_1, {20020103,0},  100, 12.00, 0, 0, 0 },
   {TA_LONG_ENTRY, &id2_1, {20020104,0},  200, 15.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id2_1, {20020108,0},  200, 14.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id2_1, {20020109,0},  100, 13.00, 0, 0, 0 },
   {-1,(TA_Instrument *)NULL, {0,0}, 0, 0.00, 0, 0, 0}
};

/* 9 transactions, 3 instruments, all on the same day.  */
static TA_Transaction table2_1[] = {
   {TA_LONG_ENTRY, &id1_1, {20020101,0},  101, 11.00, 0, 0, 0 },
   {TA_LONG_ENTRY, &id1_2, {20020101,0},  102, 12.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id1_1, {20020101,0},  100, 14.00, 0, 0, 0 },
   {TA_LONG_ENTRY, &id2_1, {20020101,0},  103, 13.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id1_2, {20020101,0},   49, 11.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id2_1, {20020101,0},  103, 16.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id1_2, {20020101,0},   51, 11.50, 0, 0, 0 },
   {TA_LONG_EXIT,  &id1_1, {20020101,0},    1, 17.00, 0, 0, 0 },
   {TA_LONG_EXIT,  &id1_2, {20020101,0},    2, 12.00, 0, 0, 0 },
   {-1,(TA_Instrument *)NULL, {0,0}, 0, 0.00, 0, 0, 0}
};

/* 9 transactions, 3 instruments, on 2 days. 
 * Using category or symbol keys only.
 */

static TA_Transaction table2_2[] = {
   {TA_LONG_ENTRY,  &id3_1, {20020101,0},  101, 11.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id3_2, {20020101,0},  102, 12.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_1, {20021231,0},  100, 14.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id4_1, {20020101,0},  103, 13.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},   49, 11.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id4_1, {20021231,0},  103, 16.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},   51, 11.50, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_1, {20021231,0},    1, 17.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},    2, 12.00, 0, 0, 0 },
   {-1,(TA_Instrument *)NULL, {0,0}, 0, 0.00, 0, 0, 0}
};

static TA_Transaction table2_3[] = {
   {TA_LONG_ENTRY,  &id3_1, {20020101,0},  100, 11.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id3_1, {20020101,0},    1, 11.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id3_2, {20020101,0},   49, 12.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id3_2, {20020101,0},   51, 12.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id3_2, {20020101,0},    2, 12.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_1, {20021231,0},  100, 14.00, 0, 0, 0 },
   {TA_LONG_ENTRY,  &id4_1, {20020101,0},  103, 13.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},   49, 11.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id4_1, {20021231,0},  103, 16.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},   51, 11.50, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_1, {20021231,0},    1, 17.00, 0, 0, 0 },
   {TA_LONG_EXIT,   &id3_2, {20021231,0},    2, 12.00, 0, 0, 0 },
   {-1,(TA_Instrument *)NULL, {0,0}, 0, 0.00, 0, 0, 0}
};

/* 5 transactions, short trades. */
static TA_Transaction table3_1[] = {
   {TA_SHORT_ENTRY, &id1_1, {20021231,0},  3, 10.00, 0, 0, 0 },
   {TA_SHORT_EXIT,  &id1_1, {20021231,0},  1,  9.00, 0, 0, 0 },
   {TA_SHORT_EXIT,  &id1_1, {20021231,0},  2,  9.00, 0, 0, 0 },
   {TA_SHORT_ENTRY, &id1_1, {20021231,0},  3, 10.00, 0, 0, 0 },
   {TA_SHORT_EXIT,  &id1_1, {20021231,0},  2,  9.00, 0, 0, 0 },
   {TA_SHORT_EXIT,  &id1_1, {20021231,0},  1,  9.00, 0, 0, 0 },
   {TA_SHORT_ENTRY, &id1_1, {20020101,0},  3, 10.00, 0, 0, 0 },
   {TA_SHORT_EXIT,  &id1_1, {20021231,0},  3,  9.00, 0, 0, 0 },
   {-1,(TA_Instrument *)NULL, {0,0}, 0, 0.00, 0, 0, 0}
};

/* List of TA_PMValueId to check */
static TA_PMValueIdCheck toCheck1[] = {
   {TA_PM_TOTAL_NB_OF_TRADE, TA_PM_ALL_TRADES, TA_SUCCESS, 5},
   {-1,0,0,0.0}
};

static TA_PMValueIdCheck toCheck2[] = {
   {TA_PM_TOTAL_NB_OF_TRADE,  TA_PM_ALL_TRADES, TA_SUCCESS, 6},
   {TA_PM_STARTING_CAPITAL,   TA_PM_ALL_TRADES, TA_SUCCESS, 50000},
   {TA_PM_ENDING_CAPITAL,     TA_PM_ALL_TRADES, TA_SUCCESS, 50540.50},
   {TA_PM_TOTAL_NET_PROFIT,   TA_PM_ALL_TRADES, TA_SUCCESS, 540.50 },
   {TA_PM_PROFIT_FACTOR,      TA_PM_ALL_TRADES, TA_SUCCESS, 8.2550336 },
   {TA_PM_PERCENT_PROFITABLE, TA_PM_ALL_TRADES, TA_SUCCESS, 50.0 },
   {TA_PM_RATE_OF_RETURN,     TA_PM_ALL_TRADES, TA_SUCCESS, 1.08397 },
   {TA_PM_ANNUALIZED_RETURN,  TA_PM_ALL_TRADES, TA_SUCCESS, 1.08397 },
   {TA_PM_ANNUALIZED_COMPOUNDED_RETURN, TA_PM_ALL_TRADES, TA_SUCCESS, 1.08397 },
   {TA_PM_NB_WINNING_TRADE,   TA_PM_ALL_TRADES, TA_SUCCESS, 3 },
   {TA_PM_GROSS_PROFIT,       TA_PM_ALL_TRADES, TA_SUCCESS, 615 },
   {TA_PM_AVG_PROFIT,         TA_PM_ALL_TRADES, TA_SUCCESS, 205.0 },
   {TA_PM_AVG_PROFIT_PERCENT, TA_PM_ALL_TRADES, TA_SUCCESS, 34.965035 },
   {TA_PM_LARGEST_PROFIT,     TA_PM_ALL_TRADES, TA_SUCCESS,  309.0 },
   {TA_PM_LARGEST_PROFIT_PERCENT, TA_PM_ALL_TRADES, TA_SUCCESS, 54.545 },
   {TA_PM_NB_LOSING_TRADE, TA_PM_ALL_TRADES, TA_SUCCESS, 3.0 },
   {TA_PM_GROSS_LOSS, TA_PM_ALL_TRADES, TA_SUCCESS, -74.5 },
   {TA_PM_AVG_LOSS, TA_PM_ALL_TRADES, TA_SUCCESS, -24.83333 },
   {TA_PM_AVG_LOSS_PERCENT, TA_PM_ALL_TRADES, TA_SUCCESS, -4.166},
   {TA_PM_LARGEST_LOSS,  TA_PM_ALL_TRADES, TA_SUCCESS, -49.0 },
   {TA_PM_LARGEST_LOSS_PERCENT, TA_PM_ALL_TRADES, TA_SUCCESS, -8.333 },
   {-1,0,0,0.0}
};

static TA_PMValueIdCheck toCheck3[] = {
   {TA_PM_TOTAL_NB_OF_TRADE, TA_PM_ALL_TRADES, TA_SUCCESS, 4},
   {-1,0,0,0.0}
};


/* List of TA_PMArrayId to check */
static TA_PMArrayIdCheck toCheckArray1[] = {
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020101,0}, TA_SUCCESS, 100.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020102,0}, TA_SUCCESS, 100.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020103,0}, TA_SUCCESS, 300.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020104,0}, TA_SUCCESS, 300.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020107,0}, TA_SUCCESS, 300.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020108,0}, TA_SUCCESS, 600.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20020109,0}, TA_SUCCESS, 400.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_DAILY,  {20021231,0}, TA_SUCCESS, 400.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_WEEKLY, {20020104,0}, TA_SUCCESS, 300.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_WEEKLY, {20020111,0}, TA_SUCCESS, 400.0},
   {TA_PM_ARRAY_EQUITY, TA_PM_ALL_TRADES, TA_WEEKLY, {20030103,0}, TA_SUCCESS, 400.0},
   {-1,0,0,{0,0},0,0}
};


/* List of tests for TA_PMValue  */
static TA_PMValueIdTest pmValueIdTests[] = {
   {table1_1, {20020101,0}, {20021231,0},  1000, toCheck1},
   {table2_1, {20020101,0}, {20021231,0}, 50000, toCheck2},
   {table2_2, {20020101,0}, {20021231,0}, 50000, toCheck2},
   {table2_3, {20020101,0}, {20021231,0}, 50000, toCheck2},
   {table3_1, {20021231,0}, {20021231,0}, 100, toCheck3}
};
#define NB_PMVALUEID_TEST (sizeof(pmValueIdTests)/sizeof(TA_PMValueIdTest))

/* List of tests for TA_PMArray  */
static TA_PMArrayIdTest pmArrayIdTests[] = {
   {table1_1, {20020101,0}, {20021231,0},  100, toCheckArray1}
};
#define NB_PMARRAYID_TEST (sizeof(pmArrayIdTests)/sizeof(TA_PMArrayIdTest))

/**** Global functions definitions.   ****/
ErrorNumber test_pm( void )
{
   ErrorNumber errorNumber;
   TA_UDBase *udb;
   unsigned int i, j;

   printf( "Testing Performance Measurement\n" );

   /* Side Note:
    * Why all these allocLib/freeLib in this function?
    *   Each time freeLib is being called, it is verified
    *   that all ressource has been freed. So that's a good
    *   way to verify for any potential memory leak.
    */

   errorNumber = allocLib( &udb );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;    

   /* Initialize some globals used throughout these tests. */
   TA_SetTimeNow( &timestampNow );
   TA_SetDateNow( &timestampNow );
   TA_NextWeekday( &timestampNow );
   TA_TimestampCopy( &timestampNowPlusOneYear, &timestampNow );
   TA_NextYear( &timestampNowPlusOneYear );
   TA_PrevDay( &timestampNowPlusOneYear );

   /* Using a user defined kkey */
   memset( &id1_1, 0, sizeof(TA_Instrument) );
   memset( &id1_2, 0, sizeof(TA_Instrument) );
   id1_1.userKey = 12;
   id1_2.userKey =  9;

   /* Using a category / symbol strings. */
   memset( &id2_1, 0, sizeof(TA_Instrument) );
   id2_1.catString = "AB";
   id2_1.symString = "CE";

   /* Using a category only. */
   memset( &id3_1, 0, sizeof(TA_Instrument) );
   memset( &id3_2, 0, sizeof(TA_Instrument) );
   id3_1.catString = "ABCD";
   id3_1.symString = NULL;
   id3_2.catString = "EFGH";
   id3_2.symString = NULL;

   /* Using only a symbol string */
   memset( &id4_1, 0, sizeof(TA_Instrument) );
   memset( &id4_2, 0, sizeof(TA_Instrument) );
   id4_1.catString = NULL;
   id4_1.symString = "A";
   id4_2.catString = NULL;
   id4_2.symString = "B";

   /* Test limit cases with empty TA_TradeLog */
   errorNumber = test_emptytradelog();
   if( errorNumber != TA_TEST_PASS )
   {
      printf( "Failed: Empty trade log cases\n" );   
      return errorNumber;
   }

   errorNumber = freeLib( udb );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Test with only one TA_Transaction.
    * Repeat all tests for each possible
    * TA_Instrument key type.
    */
   for( i=0; i < NB_TA_KEY_TYPE; i++ )
   {
      errorNumber = allocLib( &udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;    
      errorNumber = test_onetransaction_only( (TA_KEY_TYPE)i );
      if( errorNumber != TA_TEST_PASS )
      {
         printf( "Failed: one transaction cases (key=%d,errorNumber=%d)\n", (TA_KEY_TYPE)i, errorNumber );
         return errorNumber;
      }
      errorNumber = freeLib( udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;
   }

   /* Tests with two TA_Transaction for the
    * same given TA_Instrument.
    *
    * Repeat the test for each combination
    * of:
    *  - TA_Instrument key type
    *  - long and short trade.
    *  - winning and losing trade
    */
   for( i=0; i <= 1; i++ )
   {
      /* 0 = test a loosing trade
       * 1 = test a winning trade
       */   
      for( j=0; j < NB_TA_KEY_TYPE; j++ )
      {
         /* Test Long */
         errorNumber = allocLib( &udb );
         if( errorNumber != TA_TEST_PASS )
            return errorNumber;    
         errorNumber = test_onetrade_only( (TA_KEY_TYPE)j, TA_LONG_ENTRY, i );
         if( errorNumber != TA_TEST_PASS )
         {
            printf( "Failed: one trade only (key=%d,type=%d,winning=%d)\n", (TA_KEY_TYPE)j, TA_LONG_ENTRY, i );
            return errorNumber;
         }
         errorNumber = freeLib( udb );
         if( errorNumber != TA_TEST_PASS )
            return errorNumber;

         /* Test Short */
         errorNumber = allocLib( &udb );
         if( errorNumber != TA_TEST_PASS )
            return errorNumber;    
         errorNumber = test_onetrade_only( (TA_KEY_TYPE)j, TA_SHORT_ENTRY, i );
         if( errorNumber != TA_TEST_PASS )
         {
            printf( "Failed: one trade only (key=%d,type=%d,winning=%d)\n", (TA_KEY_TYPE)j, TA_SHORT_ENTRY, i );
            return errorNumber;
         }
         errorNumber = freeLib( udb );
         if( errorNumber != TA_TEST_PASS )
            return errorNumber;
      }
   }

   /* Test TA_PMValueId using a list of tests
    * defined in static variables.
    */
   for( i=0; i < NB_PMVALUEID_TEST; i++ )
   {
      errorNumber = allocLib( &udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;
      errorNumber = test_valueId( &pmValueIdTests[i] );
      if( errorNumber != TA_TEST_PASS )
      {
         printf( "Failed: test_valueId #%d\n", i);
         return errorNumber;
      }
      errorNumber = freeLib( udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;
   }

   /* Test TA_PMArrayId using a list of tests
    * defined in static variables.
    */
   for( i=0; i < NB_PMARRAYID_TEST; i++ )
   {
      errorNumber = allocLib( &udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;
      errorNumber = test_arrayId( &pmArrayIdTests[i] );
      if( errorNumber != TA_TEST_PASS )
      {
         printf( "Failed: test_arrayId #%d\n", i);
         return errorNumber;
      }
      errorNumber = freeLib( udb );
      if( errorNumber != TA_TEST_PASS )
         return errorNumber;
   }

   return TA_TEST_PASS;
}


/**** Local functions definitions.     ****/
static ErrorNumber test_emptytradelog( void )
{
   TA_TradeLog *tradeLog;
   TA_RetCode retCode;

   /* Allocate an empty TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( (retCode != TA_SUCCESS) || (tradeLog == NULL) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAlloc bad retCode! [%d]\n", retCode );
      return TA_PM_EMPTY_TA_TRADE_LOG_TESTS_0;
   }

   /* Verify invalid parameters. */
   retCode = TA_TradeLogAlloc( NULL );
   if( retCode != TA_BAD_PARAM )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAlloc bad retCode! [%d]\n", retCode );
      return TA_PM_EMPTY_TA_TRADE_LOG_TESTS_2;
   }

   /* Free the empty TA_TradeLog */
   retCode = TA_TradeLogFree( tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAlloc bad retCode! [%d]\n", retCode );
      return TA_PM_EMPTY_TA_TRADELOGFREE_FAILED;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_onetransaction_only( TA_KEY_TYPE keyTypeTest )
{
   TA_RetCode retCode;
   TA_Instrument  instrument;
   TA_Transaction       transaction;
   TA_TradeLog   *tradeLog;
   ErrorNumber errorNumber;
   TA_PM *allocatedPM;

   memset( &transaction, 0, sizeof(TA_Transaction) );

   /* Allocate an empty TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( (retCode != TA_SUCCESS) || (tradeLog == NULL) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAlloc bad retCode! [%d]\n", retCode );
      return TA_PM_EMPTY_TA_TRADE_LOG_TESTS_0;
   }

   /* Add one TA_Transaction */
   switch( keyTypeTest )
   {
   case TA_KEY_TYPE_INTEGER:
      instrument.catString = NULL;
      instrument.symString = NULL;
      instrument.userKey = 0x12345432;
      break;
   case TA_KEY_TYPE_CAT:
      instrument.catString = "CATONLY";
      instrument.symString = NULL;
      instrument.userKey = 0;
      break;
   case TA_KEY_TYPE_SYM:
      instrument.catString = NULL;
      instrument.symString = "S";
      instrument.userKey = 0;
      break;
   case TA_KEY_TYPE_CATSYM:
      instrument.catString = "CATABCDEFGHIJKLMNOPQRSTUVWXYZ";
      instrument.symString = "SYM012345678901234567890";
      instrument.userKey = 0;      
      break;
   default:
      return TA_PM_ERR_INVALID_KEY_TYPE;
   }

   transaction.id        = &instrument;
   transaction.price     = 34.45;
   transaction.quantity  = 8765;
   TA_TimestampCopy( &transaction.execTimestamp, &timestampNow );
   transaction.type      = TA_LONG_ENTRY;

   retCode = TA_TradeLogAdd( tradeLog, &transaction );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAdd bad retCode %d\n", retCode );
      return TA_PM_TRADELOGADD_ONE_TRADE_FAILED;
   }

   /* Create a TA_PM */
   retCode = TA_PMAlloc(
                         &timestampNow,
                         &timestampNow, 
                         1000, &allocatedPM );
   if( retCode != TA_SUCCESS )
   {
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMAlloc bad retCode %d\n", retCode );
      return TA_PM_TRADELOGADD_ONE_TRADE_FAILED_1;
   }                    
   retCode = TA_PMAddTradeLog( allocatedPM, tradeLog );
   if( retCode != TA_SUCCESS )
   {
      TA_PMFree( allocatedPM );
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMAddTradeLog bad retCode %d\n", retCode );
      return TA_PM_TRADELOGADD_ONE_TRADE_FAILED_2;
   }                    

   /* Verify the NB of TRADE */
   errorNumber = checkPMvalues( allocatedPM, 0, 0, 0, 0 );

   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   errorNumber = checkNoHang( allocatedPM );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Building a report should work */
   errorNumber = test_report( allocatedPM, 0 );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Clean-up and exit */
   retCode = TA_PMFree( allocatedPM );
   if( retCode != TA_SUCCESS )
   {
      TA_PMFree( allocatedPM );
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMFree bad retCode %d\n", retCode );
      return TA_PM_TRADELOGADD_ONE_TRADE_FAILED_3;
   }                    


   retCode = TA_TradeLogFree( tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogFree bad retCode %d\n", retCode );
      return TA_PM_TRADELOGFREE_ONE_TRADE_FAILED;      
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_onetrade_only( TA_KEY_TYPE keyTypeTest,
									            TA_TransactionType transactionType, 
                                       unsigned int winningTrade )
{
   TA_RetCode retCode;
   TA_Instrument  instrument;
   TA_Transaction transaction;
   TA_TradeLog   *tradeLog;
   TA_PM         *allocatedPM;
   ErrorNumber errorNumber;

   memset( &transaction, 0 ,sizeof(TA_Transaction) );

   /* Allocate a TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( (retCode != TA_SUCCESS) || (tradeLog == NULL) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAlloc bad retCode! [%d]\n", retCode );
      return TA_PM_EMPTY_TA_TRADE_LOG_TESTS_0;
   }

   /* Add the entry TA_Transaction */
   switch( keyTypeTest )
   {
   case TA_KEY_TYPE_INTEGER:
      instrument.catString = NULL;
      instrument.symString = NULL;
      instrument.userKey = 0;
      break;
   case TA_KEY_TYPE_CAT:
      instrument.catString = "CATONLY";
      instrument.symString = NULL;
      instrument.userKey = 0;
      break;
   case TA_KEY_TYPE_SYM:
      instrument.catString = "SYMONLY";
      instrument.symString = "";
      instrument.userKey = 0;
      break;
   case TA_KEY_TYPE_CATSYM:
      instrument.catString = "C";
      instrument.symString = "S";
      instrument.userKey = 0;
      break;
   default:
      return TA_PM_ERR_INVALID_KEY_TYPE;
   }

   transaction.id        = &instrument;
   transaction.price     = 10.00;
   transaction.quantity  = 1234;
   TA_TimestampCopy( &transaction.execTimestamp, &timestampNowPlusOneYear );
   transaction.type      = transactionType;

   retCode = TA_TradeLogAdd( tradeLog, &transaction );
   if( retCode != TA_SUCCESS )
   {
      printf( "Failed: TA_TradeLogAdd bad retCode %d\n", retCode );
      return TA_PM_2TRADETST_TRADELOGADD_1;
   }

   /* Set the corresponsing exit transaction type.
    * Also make the exit price either winning
    * or loosing.
    */
   if( transactionType == TA_LONG_ENTRY )
   {
      transaction.type = TA_LONG_EXIT;
      if( winningTrade )
         transaction.price = 12.00;
      else
         transaction.price = 9.50;
   }
   else if( transactionType == TA_SHORT_ENTRY )
   {
      transaction.type = TA_SHORT_EXIT;
      if( winningTrade )
         transaction.price = 9.25;
      else
         transaction.price = 11.00;
   }
   else
      return TA_PM_2TRADETST_BAD_TRADE_TYPE;

   /* Add the exit transaction. */
   retCode = TA_TradeLogAdd( tradeLog, &transaction );
   if( retCode != TA_SUCCESS )
   {
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogAdd bad retCode %d\n", retCode );
      return TA_PM_2TRADETST_TRADELOGADD_2;
   }

   /* Create a TA_PM */
   retCode = TA_PMAlloc( &timestampNow, &timestampNowPlusOneYear,
                         10000, &allocatedPM );
   if( retCode != TA_SUCCESS )
   {
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMAlloc bad retCode %d\n", retCode );
      return TA_PM_2TRADETST_PMALLOC_FAILED;
   }                    
   retCode = TA_PMAddTradeLog( allocatedPM, tradeLog );
   if( retCode != TA_SUCCESS )
   {
      TA_PMFree( allocatedPM );
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMAddTradeLog bad retCode %d\n", retCode );
      return TA_PM_2TRADETST_PMADDTRADELOG_FAILED;
   }                    

   /* Verify the NB of TRADE and the net profit */
   if( transactionType == TA_LONG_ENTRY )
   {
      if( winningTrade )
         errorNumber = checkPMvalues( allocatedPM, 1, 0, 2468, 0 );
      else
         errorNumber = checkPMvalues( allocatedPM, 1, 0, -617, 0 );
   }
   else if( transactionType == TA_SHORT_ENTRY )
   {
      if( winningTrade )
         errorNumber = checkPMvalues( allocatedPM, 0, 1, 0, 925.5 );
      else
         errorNumber = checkPMvalues( allocatedPM, 0, 1, 0, -1234);
   }
   else
      errorNumber = TA_PM_UNKNOWN_TRANSACTION_TYPE;

   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   errorNumber = checkNoHang( allocatedPM );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Building a report should work */
   errorNumber = test_report( allocatedPM, 0 );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Clean-up and exit */
   retCode = TA_PMFree( allocatedPM );
   if( retCode != TA_SUCCESS )
   {
      TA_PMFree( allocatedPM );
      TA_TradeLogFree( tradeLog );
      printRetCode( retCode );
      printf( "Failed: TA_PMFree bad retCode %d\n", retCode );
      return TA_PM_2TRADETST_PMFREE_FAILED;
   }                    

   retCode = TA_TradeLogFree( tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      printf( "Failed: TA_TradeLogFree bad retCode %d\n", retCode );
      return TA_PM_TRADELOGFREE_ONE_TRADE_FAILED;      
   }

   return TA_TEST_PASS;
}

static ErrorNumber checkNoHang( TA_PM *pm )
{
   int i, j, k;
   TA_Real firstValue, secondValue;
   TA_RetCode retCode;

   /* Call 5 of the value at random. */
   for( k=0; k < 5; k++ )
   {
      i = rand()%TA_PM_NB_VALUEID;
      j = rand()%TA_PM_NB_GROUP;

      firstValue = 1.1111;
      retCode = TA_PMValue( pm, i, j, &firstValue );
      if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
      {
         printRetCode( retCode );
         printf( "checkNoHang rand return error for first call (%d,%d,%d)\n", i, j, retCode );
         return TA_PM_ERR_CHECK_NO_HANG_1;
      }
      secondValue = 2.2222;
      retCode = TA_PMValue( pm, i, j, &secondValue );
      if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
      {
         printRetCode( retCode );
         printf( "checkNoHang rand return error for second call (%d,%d,%d)\n", i, j, retCode );
         return TA_PM_ERR_CHECK_NO_HANG_2;
      }
      if( (retCode != TA_VALUE_NOT_APPLICABLE) && (firstValue != secondValue) )
      {
         printf( "checkNoHang rand values not consistent (%d,%d,%g,%g)\n", i, j, firstValue, secondValue );
         return TA_PM_ERR_CHECK_NO_HANG_3;
      }
   }

   /* Now systematically go through all the possible
    * values.
    *
    * The goal is just to try to break things by possibly
    * causing hanging, bad pointer access or memory leak.
    *
    * The call is done twice. The same value should be
    * always returned.
    */
   for( i=0; i < TA_PM_NB_VALUEID; i++ )
   {
      for( j=0; j < TA_PM_NB_GROUP; j++ )
      {
         firstValue = 3.3333;
         retCode = TA_PMValue( pm, i, j, &firstValue );
         if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
         {
            printRetCode( retCode );
            printf( "checkNoHang return error for first call (%d,%d,%d)\n", i, j, retCode );
            return TA_PM_ERR_CHECK_NO_HANG_4;
         }
         secondValue = 4.4444;
         retCode = TA_PMValue( pm, i, j, &secondValue );
         if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
         {
            printRetCode( retCode );
            printf( "checkNoHang return error for second call (%d,%d,%d)\n", i, j, retCode );
            return TA_PM_ERR_CHECK_NO_HANG_5;
         }
         if( (retCode != TA_VALUE_NOT_APPLICABLE) && (firstValue != secondValue) )
         {
            printf( "checkNoHang values not consistent (%d,%d,%g,%g)\n", i, j, firstValue, secondValue );
            return TA_PM_ERR_CHECK_NO_HANG_6;
         }
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber checkPMvalues( TA_PM *pm, 
                                  int nbLongTrade,
                                  int nbShortTrade,
                                  TA_Real longNetProfit,
                                  TA_Real shortNetProfit )
{
   TA_RetCode retCode;
   TA_Real pmReadNbShortTrade, pmReadNbLongTrade, pmReadNbTotalTrade;
   TA_Real tempReal1, tempReal2;

   tempReal1 = tempReal2 = 0.0;

   /* Check all the TA_PM_TOTAL_NB_OF_TRADE. */
   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NB_OF_TRADE,
                         TA_PM_LONG_TRADES,
                         &pmReadNbLongTrade );
   if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value)=(%d,%d,%d,%g)\n",
               TA_PM_TOTAL_NB_OF_TRADE,
               TA_PM_LONG_TRADES,
               retCode,
               pmReadNbLongTrade );
      return TA_PM_CHECKVALUE_FAILED_0;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NB_OF_TRADE,
                         TA_PM_SHORT_TRADES,
                         &pmReadNbShortTrade );
   if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value)=(%d,%d,%d,%g)\n",
               TA_PM_TOTAL_NB_OF_TRADE,
               TA_PM_SHORT_TRADES,
               retCode,
               pmReadNbShortTrade );
      return TA_PM_CHECKVALUE_FAILED_1;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NB_OF_TRADE,
                         TA_PM_ALL_TRADES,
                         &pmReadNbTotalTrade );
   if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value)=(%d,%d,%d,%g)\n",
               TA_PM_TOTAL_NB_OF_TRADE,
               TA_PM_ALL_TRADES,
               retCode,
               pmReadNbTotalTrade );
      return TA_PM_CHECKVALUE_FAILED_2;
   }

   if( ((nbShortTrade+nbLongTrade) != pmReadNbTotalTrade) ||
       (nbShortTrade != pmReadNbShortTrade) ||
       (nbLongTrade != pmReadNbLongTrade) )
   {
      printRetCode( retCode );
      printf( "Failed: invalid nb of trade (short,pmshort,long,pmlong,total,pmtotal)=(%d,%d,%d,%d,%d,%d)\n",
              nbShortTrade, (int)pmReadNbShortTrade,
              nbLongTrade, (int)pmReadNbLongTrade,
              (nbShortTrade+nbLongTrade), (int)pmReadNbTotalTrade );
      return TA_PM_CHECKVALUE_FAILED_3;
   }

   /* Check all the value related to net profits.
    *  shortNetProfit+longNetProfit = (TA_PM_TOTAL_NET_PROFIT, TA_PM_ALL_TRADES)
    *  shortNetProfit               = (TA_PM_TOTAL_NET_PROFIT, TA_PM_SHORT_TRADES)
    *  longNetProfit                = (TA_PM_TOTAL_NET_PROFIT, TA_PM_LONG_TRADES)
    *  shortNetProfit = (TA_PM_PROFIT, TA_PM_SHORT_TRADES)+(TA_PM_LOSS, TA_PM_SHORT_TRADES)
    *  longNetProfit  = (TA_PM_PROFIT, TA_PM_LONG_TRADES)+(TA_PM_LOSS, TA_PM_LONG_TRADES)
    */
   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NET_PROFIT,
                         TA_PM_ALL_TRADES,
                         &tempReal1 );
                            
   if( ((retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE)) ||
       (tempReal1 != (shortNetProfit+longNetProfit)) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g)\n",
               TA_PM_TOTAL_NET_PROFIT,
               TA_PM_ALL_TRADES,
               retCode,
               tempReal1,
               shortNetProfit+longNetProfit );

      return TA_PM_CHECKVALUE_FAILED_4;
   }
   
   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NET_PROFIT,
                         TA_PM_LONG_TRADES,
                         &tempReal1 );
   if( ((retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE)) || (tempReal1 != longNetProfit) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g)\n",
               TA_PM_TOTAL_NET_PROFIT,
               TA_PM_LONG_TRADES,
               retCode,
               tempReal1,
               longNetProfit );

      return TA_PM_CHECKVALUE_FAILED_5;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_TOTAL_NET_PROFIT,
                         TA_PM_SHORT_TRADES,
                         &tempReal2 );
   if( ((retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE)) || (tempReal2 != shortNetProfit) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g)\n",
               TA_PM_TOTAL_NET_PROFIT,
               TA_PM_SHORT_TRADES,
               retCode,
               tempReal1,
               shortNetProfit );

      return TA_PM_CHECKVALUE_FAILED_6;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_GROSS_PROFIT,
                         TA_PM_SHORT_TRADES,
                         &tempReal1 );
   if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g)\n",
               TA_PM_GROSS_PROFIT,
               TA_PM_SHORT_TRADES,
               retCode,
               tempReal1,
               tempReal2 );

      return TA_PM_CHECKVALUE_FAILED_7;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_GROSS_LOSS,
                         TA_PM_SHORT_TRADES,
                         &tempReal2 );
   if( ((retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE)) || (shortNetProfit != (tempReal1+tempReal2)))
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g,%g)\n",
               TA_PM_GROSS_LOSS,
               TA_PM_SHORT_TRADES,
               retCode,
               tempReal1,
               tempReal2,
               shortNetProfit );

      return TA_PM_CHECKVALUE_FAILED_8;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_GROSS_PROFIT,
                         TA_PM_LONG_TRADES,
                         &tempReal1 );
   if( (retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE) )
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g)\n",
               TA_PM_GROSS_PROFIT,
               TA_PM_LONG_TRADES,
               retCode,
               tempReal1,
               tempReal2 );

      return TA_PM_CHECKVALUE_FAILED_9;
   }

   retCode = TA_PMValue( pm,
                         TA_PM_GROSS_LOSS,
                         TA_PM_LONG_TRADES,
                         &tempReal2 );
   if( ((retCode != TA_SUCCESS) && (retCode != TA_VALUE_NOT_APPLICABLE)) || (longNetProfit != (tempReal1+tempReal2)))
   {
      printRetCode( retCode );
      printf( "Failed: TA_PMValue (valueId,grpId,retCode,value,value)=(%d,%d,%d,%g,%g,%g)\n",
               TA_PM_GROSS_LOSS,
               TA_PM_SHORT_TRADES,
               retCode,
               tempReal1,
               tempReal2,
               longNetProfit );

      return TA_PM_CHECKVALUE_FAILED_10;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_report( TA_PM *pm, unsigned int doDisplay )
{
   TA_PMReport *pmReport;
   TA_RetCode retCode;

   if( doDisplay)
   {
      retCode = TA_PMReportToFile( pm, stdout );
      if( retCode != TA_SUCCESS )
      {
         printf( "Failed: TA_PMValueToFile retCode = %d\n", retCode );
         return TA_PM_VALUE_TO_FILE_FAILED;
      }
   }
   else
   {
      /* Just alloc and free for test purpose. */
      retCode = TA_PMReportAlloc( pm, &pmReport );
      if( retCode != TA_SUCCESS )
      {
         printf( "Failed: TA_PMReportAlloc retCode = %d\n", retCode );
         return TA_PM_REPORT_ALLOC_FAILED;
      }

      TA_PMReportFree( pmReport );
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_valueId( TA_PMValueIdTest *test )
{
   unsigned int  i;
   TA_TradeLog  *tradeLog;
   TA_PM        *pm;
   ErrorNumber   errorNumber;
   TA_RetCode    retCode;
   TA_Real       theValue;
   const char   *tempStr;

   /* Allocate and build the TA_TradeLog */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( retCode != TA_SUCCESS )
      return TA_PM_TEST_VALUE_ID_FAILED_0;

   /* Add all the transaction. For simplicity, make these
    * all the same instrument.
    */
   #define TA_SAFETY_NET_LIMIT 100
   i = 0;
   while( ((int)test->inputs[i].type != -1) && (i<TA_SAFETY_NET_LIMIT) )
   {
      retCode = TA_TradeLogAdd( tradeLog, &test->inputs[i] );
      if( retCode != TA_SUCCESS )
      {
         printRetCode( retCode );
         TA_TradeLogFree( tradeLog );
         return TA_PM_TEST_VALUE_ID_FAILED_1;
      }
      i++;
   }
   if( i >= TA_SAFETY_NET_LIMIT )
   {
      printf( "Failed: Number of transaction exceed %d limit\n", TA_SAFETY_NET_LIMIT );
      return TA_PM_TEST_VALUE_ID_FAILED_2;
   }
   #undef TA_SAFETY_NET_LIMIT

   /* Build the TA_PM */
   retCode = TA_PMAlloc( &test->startDate, &test->endDate, 
                         test->initialCapital, &pm );

   if( retCode != TA_SUCCESS )   
   {
      printRetCode( retCode );
      TA_TradeLogFree( tradeLog );
      return TA_PM_TEST_VALUE_ID_FAILED_3;
   }

   /* Add the trade log to that PM */
   retCode = TA_PMAddTradeLog( pm, tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_VALUE_ID_FAILED_4;
   }

   /* Test the report feature. Again just to detect
    * software hanging/bad pointer.
    */
   errorNumber = test_report( pm, 0 );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   /* Check the requested TA_PMArrayId */
   #define TA_SAFETY_NET_LIMIT 30
   i=0;
   while( (((int)test->toCheck[i].id) != -1) && (i<TA_SAFETY_NET_LIMIT) )
   {
      switch( test->toCheck[i].grp )
      {
      case TA_PM_ALL_TRADES:
         tempStr = "TA_PM_ALL_TRADES";
         break;
      case TA_PM_SHORT_TRADES:
         tempStr = "TA_PM_SHORT_TRADES";
         break;
      case TA_PM_LONG_TRADES:
         tempStr = "TA_PM_LONG_TRADES";
         break;
      default:
         tempStr = "Invalid Group Id";
      }

      retCode = TA_PMValue( pm, 
                            test->toCheck[i].id,
                            test->toCheck[i].grp,
                            &theValue );

      if( retCode != test->toCheck[i].expectedRetCode )
      {
         printRetCode( test->toCheck[i].expectedRetCode );
         printRetCode( retCode );
         printf( "Failed: TA_PMValue expectedRetCode != retCode (%d != %d)\n",
                  test->toCheck[i].expectedRetCode, retCode );
         printf( "Failed: For %d:%s %d:%s\n", test->toCheck[i].id,
                                              TA_PMValueIdString(test->toCheck[i].id),
                                              test->toCheck[i].grp, tempStr );
         return TA_PM_TEST_VALUE_ID_FAILED_5;
      }

      if( !TA_REAL_EQ(theValue,test->toCheck[i].expectedValue,0.01) )
      {
         printf( "Failed: TA_PMValue expectedValue != theValue (%f != %f)\n",
                  test->toCheck[i].expectedValue, theValue );
         printf( "Failed: For %d:%s %d:%s\n", test->toCheck[i].id,
                                              TA_PMValueIdString(test->toCheck[i].id),
                                              test->toCheck[i].grp, tempStr );
         return TA_PM_TEST_VALUE_ID_FAILED_6;
      }

      i++;
   }

   if( i >= TA_SAFETY_NET_LIMIT )
   {
      printf( "Failed: Number of checks exceed %d limit\n", TA_SAFETY_NET_LIMIT );
      return TA_PM_TEST_VALUE_ID_FAILED_7;
   }
   #undef TA_SAFETY_NET_LIMIT

   /* Check for any potential software hanging/bad pointer. */
   errorNumber = checkNoHang( pm );
   if( errorNumber != TA_TEST_PASS )
      return errorNumber;

   retCode = TA_PMFree( pm );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_VALUE_ID_FAILED_9;
   }

   /* Free up everything */
   retCode = TA_TradeLogFree( tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_VALUE_ID_FAILED_8;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_arrayId( TA_PMArrayIdTest *test )
{
   unsigned int  i, j;
   TA_TradeLog  *tradeLog;
   TA_PM        *pm;
   TA_RetCode    retCode;
   TA_Real       theValue;
   TA_PMArray    *pmArray;
   const char   *tempStr;

   /* Allocate and build the TA_TradeLog */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( retCode != TA_SUCCESS )
      return TA_PM_TEST_ARRAY_ID_FAILED_0;

   /* Add all the transaction. For simplicity, make these
    * all the same instrument.
    */
   #define TA_SAFETY_NET_LIMIT 100
   i = 0;
   while( ((int)test->inputs[i].type != -1) && (i<TA_SAFETY_NET_LIMIT) )
   {
      retCode = TA_TradeLogAdd( tradeLog, &test->inputs[i] );
      if( retCode != TA_SUCCESS )
      {
         printRetCode( retCode );
         TA_TradeLogFree( tradeLog );
         return TA_PM_TEST_ARRAY_ID_FAILED_1;
      }
      i++;
   }
   if( i >= TA_SAFETY_NET_LIMIT )
   {
      printf( "Failed: Number of transaction exceed %d limit\n", TA_SAFETY_NET_LIMIT );
      return TA_PM_TEST_ARRAY_ID_FAILED_2;
   }
   #undef TA_SAFETY_NET_LIMIT

   /* Build the TA_PM */
   retCode = TA_PMAlloc( &test->startDate, &test->endDate, 
                         test->initialCapital, &pm );

   if( retCode != TA_SUCCESS )   
   {
      printRetCode( retCode );
      TA_TradeLogFree( tradeLog );
      return TA_PM_TEST_ARRAY_ID_FAILED_3;
   }

   /* Add the trade log to that PM */
   retCode = TA_PMAddTradeLog( pm, tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_ARRAY_ID_FAILED_4;
   }

   /* Check the requested TA_PMArrayId */
   #define TA_SAFETY_NET_LIMIT 30
   i=0;
   while( (((int)test->toCheck[i].id) != -1) && (i<TA_SAFETY_NET_LIMIT) )
   {
      switch( test->toCheck[i].grp )
      {
      case TA_PM_ALL_TRADES:
         tempStr = "TA_PM_ALL_TRADES";
         break;
      case TA_PM_SHORT_TRADES:
         tempStr = "TA_PM_SHORT_TRADES";
         break;
      case TA_PM_LONG_TRADES:
         tempStr = "TA_PM_LONG_TRADES";
         break;
      default:
         tempStr = "Invalid Group Id";
      }

      retCode = TA_PMArrayAlloc( pm, 
                                 test->toCheck[i].id,
                                 test->toCheck[i].grp,
                                 test->toCheck[i].period,
                                 &pmArray );

      if( (retCode != test->toCheck[i].expectedRetCode) || (pmArray == NULL) )
      {
         printRetCode( test->toCheck[i].expectedRetCode );
         printRetCode( retCode );
         printf( "Failed: TA_PMArrayAlloc expectedRetCode != retCode (%d != %d)\n",
                  test->toCheck[i].expectedRetCode, retCode );
         printf( "Failed: For %d:%s %d:%s Date[%d]\n", test->toCheck[i].id,
                                              "",
                                              test->toCheck[i].grp, tempStr,
                                              (int)test->toCheck[i].date.date );
         return TA_PM_TEST_ARRAY_ID_FAILED_5;
      }


      if( retCode == TA_SUCCESS )
      {
         /* Find the value to be verified */
         for( j=0; j < pmArray->nbData; j++ )
         {
            if( TA_TimestampEqual( &pmArray->timestamp[j], &test->toCheck[i].date ) )
            {
               theValue = pmArray->data[j];
               if( !TA_REAL_EQ(theValue,test->toCheck[i].expectedValue,0.01) )
               {
                  printf( "Failed: TA_PMArray expectedValue != theValue (%f != %f)\n",
                           test->toCheck[i].expectedValue, theValue );
                  printf( "Failed: For %d:%s %d:%s, Date[%d]\n", test->toCheck[i].id,
                                                       "",
                                                       test->toCheck[i].grp, tempStr,
                                                       (int)test->toCheck[i].date.date );
                  return TA_PM_TEST_ARRAY_ID_FAILED_6;
               }
               break;
            }
         }

         if( j >= pmArray->nbData )
         {
            printf( "Failed: The expected date [%d] has not been found\n", (int)test->toCheck[i].date.date );
            return TA_PM_TEST_ARRAY_WITH_INVALID_DATE;
         }

         TA_PMArrayFree( pmArray );
      }

      i++;
   }

   if( i >= TA_SAFETY_NET_LIMIT )
   {
      printf( "Failed: Number of checks exceed %d limit\n", TA_SAFETY_NET_LIMIT );
      return TA_PM_TEST_ARRAY_ID_FAILED_7;
   }
   #undef TA_SAFETY_NET_LIMIT

   /* Free up everything */
   retCode = TA_PMFree( pm );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_ARRAY_ID_FAILED_9;
   }

   retCode = TA_TradeLogFree( tradeLog );
   if( retCode != TA_SUCCESS )
   {
      printRetCode( retCode );
      return TA_PM_TEST_ARRAY_ID_FAILED_8;
   }

   return TA_TEST_PASS;
}
