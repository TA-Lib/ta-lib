/* This file contains examples of usage of the
 * Performance Measurements module.
 *
 * The first example shows how to build a TA_PM and 
 * display all its measurements.
 *
 * The second example goes further by displaying a
 * trade-by-trade reports and shows also how the
 * excursion information (e.g. MAE) is handled.
 *
 * Info: http://ta-lib.org
 */
#include <stdio.h>
#include <stdlib.h>
#include "ta_libc.h"

static void doExample1(FILE *out);
static void doExample2(FILE *out);

static TA_Instrument msft;

int main( int argc, char **argv )
{
   /* Initialize TA-Lib once. */
   if( TA_Initialize(NULL) != TA_SUCCESS )
      exit(-1);

   /* Initialize the instrument once.
    * Here I am using the string "MSFT" to 
    * make the instrument unique.
    */
   msft.catString = NULL;
   msft.symString = "MSFT";
   msft.userKey   = 0;

   printf( "*******************\n" );
   printf( "**** EXAMPLE 1 ****\n" );
   printf( "*******************\n" );
   doExample1(stdout);

   printf( "*******************\n" );
   printf( "**** EXAMPLE 2 ****\n" );
   printf( "*******************\n" );
   doExample2(stdout);

   TA_Shutdown();

   return 0;   
}

/*************/
/* EXAMPLE 1 */
/*************/
static TA_Transaction trans_example1[] =
{
   /* See TA_Transaction in ta_pm.h */
   {TA_LONG_ENTRY, &msft, {20010101},  2,45.0},
   {TA_LONG_EXIT,  &msft, {20011231},  2,45.0},
   {TA_LONG_ENTRY, &msft, {20021025},100,50.0},
   {TA_LONG_EXIT,  &msft, {20021025}, 50,51.0},
   {TA_LONG_EXIT,  &msft, {20021026}, 50,25.0},
   {TA_SHORT_ENTRY,&msft, {20021101},100,51.0},
   {TA_SHORT_EXIT, &msft, {20021105},100,50.0},
   {TA_LONG_ENTRY, &msft, {20030101},  2,45.0},
   {TA_LONG_EXIT,  &msft, {20030101},  2,45.0}
};

#define NB_TRANS_EXAMPLE1 (sizeof(trans_example1)/sizeof(TA_Transaction))

void doExample1(FILE *out)
{
   TA_RetCode retCode;
   TA_TradeLog   *tradeLog;
   TA_PM         *allocatedPM;
   TA_Timestamp  begOfYear, endOfYear;
   int i;

   /* Allocate a TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( retCode != TA_SUCCESS )
      exit(-2);

   /* Add all the transaction */
   for( i=0; i < NB_TRANS_EXAMPLE1; i++ )
   {
      retCode = TA_TradeLogAdd( tradeLog, &trans_example1[i] );
      if( retCode != TA_SUCCESS )
         exit(-4);
   }

   /* Once all the transactions are logged, you
    * can perform measurements with a TA_PM.
    */

   /* Create a TA_PM covering a year. */
   TA_SetDate( 2002,  1,  1, &begOfYear );
   TA_SetTime( 0, 0, 0, &begOfYear );
   TA_SetDate( 2002, 12, 31, &endOfYear );
   TA_SetTime( 23, 59, 59, &endOfYear );

   retCode = TA_PMAlloc( &begOfYear, &endOfYear, 10000, &allocatedPM );
   if( retCode != TA_SUCCESS )
      exit(-5);

   /* Add all the trade log (there is only one here) */
   retCode = TA_PMAddTradeLog( allocatedPM, tradeLog );
   if( retCode != TA_SUCCESS )
      exit(-6);

   /* Output a report on stdout  */
   TA_PMReportToFile( allocatedPM, out );

   /* Clean-up */
   TA_PMFree( allocatedPM );
   TA_TradeLogFree( tradeLog );
}

/*************/
/* EXAMPLE 2 */
/*************/
static TA_Transaction trans_example2[] =
{
   /* See TA_Transaction in ta_pm.h */
   {TA_LONG_ENTRY, &msft, {20010101},  2,45.0},
   {TA_LONG_EXIT,  &msft, {20011231},  2,45.0},
   {TA_LONG_ENTRY, &msft, {20021025},100,50.0},
   {TA_LONG_EXIT,  &msft, {20021025}, 50,51.0},
   {TA_LONG_EXIT,  &msft, {20021026}, 50,25.0},
   {TA_SHORT_ENTRY,&msft, {20021101},100,51.0},
   {TA_SHORT_EXIT, &msft, {20021105},100,50.0},
   {TA_LONG_ENTRY, &msft, {20030101},  2,45.0},
   {TA_LONG_EXIT,  &msft, {20030101},  2,45.0}
};

#define NB_TRANS_EXAMPLE2 (sizeof(trans_example2)/sizeof(TA_Transaction))

void doExample2(FILE *out)
{
   TA_RetCode       retCode;
   TA_TradeLog     *tradeLog;
   TA_PM           *allocatedPM;
   TA_Timestamp     begOfYear, endOfYear;
   TA_TradeReport  *tradeReport;
   int i;

   /* Allocate a TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( retCode != TA_SUCCESS )
      exit(-2);

   /* Add all the transaction */
   for( i=0; i < NB_TRANS_EXAMPLE2; i++ )
   {
      retCode = TA_TradeLogAdd( tradeLog, &trans_example2[i] );
      if( retCode != TA_SUCCESS )
         exit(-4);
   }

   /* Once all the transactions are logged, you
    * can perform measurements with a TA_PM.
    */

   /* Create a TA_PM covering a year. */
   TA_SetDate( 2002,  1,  1, &begOfYear );
   TA_SetTime( 0, 0, 0, &begOfYear );
   TA_SetDate( 2002, 12, 31, &endOfYear );
   TA_SetTime( 23, 59, 59, &endOfYear );
   retCode = TA_PMAlloc( &begOfYear, &endOfYear, 10000, &allocatedPM );
   if( retCode != TA_SUCCESS )
      exit(-5);

   /* Add all the trade log (there is only one here) */
   retCode = TA_PMAddTradeLog( allocatedPM, tradeLog );   
   if( retCode != TA_SUCCESS )
      exit(-6);

   /* Output the reports on stdout  */
   TA_PMReportToFile( allocatedPM, out );

   retCode = TA_TradeReportAlloc( allocatedPM, &tradeReport );
   if( retCode == TA_SUCCESS )
   {
      TA_TradeReportToFile( tradeReport, out );
      TA_TradeReportFree( tradeReport );
   }

   /* Clean-up */
   TA_PMFree( allocatedPM );
   TA_TradeLogFree( tradeLog );
}
