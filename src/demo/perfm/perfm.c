/* Example of code doing a list of trade and 
 * writing a report on stdout.
 *
 * Info: http:\\ta-lib.org
 */
#include <stdio.h>
#include <stdlib.h>
#include "ta_libc.h"

static TA_Instrument msft;

static TA_Transaction trans[] =
{
   /* See TA_Transaction in ta_pm.h */
   {TA_LONG_ENTRY, &msft, {20021025},100,50.0},
   {TA_LONG_EXIT,  &msft, {20021025}, 50,51.0},
   {TA_LONG_EXIT,  &msft, {20021026}, 50,25.0},
   {TA_SHORT_ENTRY,&msft, {20021101},100,51.0},
   {TA_SHORT_EXIT, &msft, {20021105},100,50.0}
};

#define NB_TRANS (sizeof(trans)/sizeof(TA_Transaction))

int main( int argc, char **argv )
{
   TA_RetCode retCode;
   TA_TradeLog   *tradeLog;
   TA_PM         *allocatedPM;
   TA_Timestamp  begOfYear, endOfYear;
   int i;

   /* Initialize TA-Lib once. */
   retCode = TA_Initialize(NULL);
   if( retCode != TA_SUCCESS )
      exit(-1);

   /* Initialize the instrument once.
    * Here I am using the string "MSFT" to make the
    * instrument unique.
    */
   TA_InstrumentInit( &msft, NULL, "MSFT" );

   /* Allocate a TA_TradeLog. */
   retCode = TA_TradeLogAlloc( &tradeLog );
   if( retCode != TA_SUCCESS )
      exit(-2);

   /* Add all the transaction */
   for( i=0; i < NB_TRANS; i++ )
   {
      retCode = TA_TradeLogAdd( tradeLog, &trans[i] );
      if( retCode != TA_SUCCESS )
         exit(-4);
   }


   /* Once all the transactions are logged, you
    * can perform measurements with a TA_PM.
    */

   /* Create a TA_PM covering a year. */
   TA_SetDate( 2002,  1,  1, &begOfYear );
   TA_SetDate( 2002, 12, 31, &endOfYear );
   retCode = TA_PMAlloc( &begOfYear, &endOfYear, 10000, &allocatedPM );
   if( retCode != TA_SUCCESS )
      exit(-5);

   /* Add all the trade log (there is only one here) */
   retCode = TA_PMAddTradeLog( allocatedPM, tradeLog );   
   if( retCode != TA_SUCCESS )
      exit(-6);

   /* Output a report on stdout  */
   TA_PMReportToFile( allocatedPM, stdout );

   /* Clean-up and exit */
   TA_PMFree( allocatedPM );
   TA_TradeLogFree( tradeLog );
   TA_Shutdown();

   return 0;   
}


