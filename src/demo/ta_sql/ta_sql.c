/* Simple application for accessing an SQL database using TA-LIB. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ta_libc.h"

/*
#if defined (_MSC_VER)
#include <windows.h>
#endif
*/

typedef enum
{
   DISPLAY_SYMBOLS,
   DISPLAY_CATEGORIES,
   DISPLAY_HISTORIC_DATA
} Action;

void fatal_error_handler(void)
{
#if defined (_MSC_VER) && 0  /* disabled for now since TA_FatalReportToBuffer is missing */
   char buffer[TA_FATAL_ERROR_BUF_SIZE];
   TA_FatalReportToBuffer( buffer, TA_FATAL_ERROR_BUF_SIZE);
   OutputDebugString(buffer);
#else
   TA_FatalReport( stderr );
#endif
}

void print_usage( char *str ) 
{
   printf( "\n" );
   printf( "ta_sql V%s - Query stock market data SQL database\n", TA_GetVersionString() );
   printf( "\n" );
   printf( "Usage: ta_sql -c <location> <user> <pass> <category (query)>\n" );
   printf( "       ta_sql -s <location> <user> <pass> <category (query)> <given category> [<symbol (query)>]\n" );
   printf( "       ta_sql -d <location> <user> <pass> <category> <symbol> <info>\n" );
   printf( "\n" );
   printf( "  -c  Display all supported categories\n" );
   printf( "  -s  Display all symbols for a given category\n" );
   printf( "  -d  Query and display quotes data\n" );
   printf( "  Add 'z' to use TA_REPLACE_ZERO_PRICE_BAR flag\n" );
   printf( "\n" );
   printf( "  Quotes output is \"Date,Open,High,Low,Close,Volume\"\n" );
   printf( "\n" );
   printf( "  Examples:\"\n" );
   printf( "    ta_sql -c mysql://localhost/db guest \"\" \"SELECT * FROM cat\"\n" );
   printf( "    ta_sql -s mysql://localhost/db guest \"\" \"SELECT * FROM cat\" NL.EURONEXT.STOCKS\n" );
   printf( "    ta_sql -dz mysql://localhost/db guest \"\" \"SELECT * FROM cat\" MSFT \"SELECT ...\"\n" );
   printf( "\n" );
   printf( "  Online help: http://www.ta-lib.org\n" );
   printf( "\n" );
   printf( "Error: [%s]\n", str );
}

void print_error( TA_RetCode retCode )
{
   TA_RetCodeInfo retCodeInfo;

   TA_SetRetCodeInfo( retCode, &retCodeInfo );
   printf( "\nError %d=%s:[%s]\n", retCode,
           retCodeInfo.enumStr,
           retCodeInfo.infoStr );
}

int print_data( TA_UDBase *udb,
                TA_AddDataSourceParam dsParam,
                TA_Period period )
{
   TA_RetCode retCode;
   TA_History *history = NULL;
   unsigned int i;

   retCode = TA_AddDataSource( udb, &dsParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   /* Get the historical data. */
   retCode = TA_HistoryAlloc( udb, 
                              dsParam.category, 
                              dsParam.symbol,
                              period, 0, 0, TA_ALL,
                              &history );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   if( !history || history->nbBars == 0 )
   {
      printf( "No data available\n" );
   }
   else
   {
      for( i=0; i < history->nbBars; i++ )
      {
         printf( "%04u-%02u-%02u", TA_GetYear(&history->timestamp[i]),
                                   TA_GetMonth(&history->timestamp[i]),
                                   TA_GetDay(&history->timestamp[i]) );
         if( history->open )
            printf( ",%.2f", history->open[i] );
         if( history->high )
            printf( ",%.2f", history->high[i] );
         if( history->low )
            printf( ",%.2f", history->low[i] );
         if( history->close )
            printf( ",%.2f", history->close[i] );
         if( history->volume )
            printf( ",%d", history->volume[i] );
         printf( "\n" );
      }
   }

   /* Get rid of the historical data and return. */
   TA_HistoryFree( history );

   return 0;
}


int print_categories( TA_UDBase *udb,
                      TA_AddDataSourceParam dsParam)
{
   TA_RetCode retCode;
   unsigned int i;
   TA_StringTable *table;

   /* info not needed for getting just categories, but the driver insists on it */
   dsParam.info = "";  
   retCode = TA_AddDataSource( udb, &dsParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }
   
   retCode = TA_CategoryTableAlloc( udb, &table );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   } 

   for( i=0; i < table->size; i++ )
   {
      printf( "%s\n", table->string[i] );
   }

   TA_CategoryTableFree( table );

   return 0;
}


int print_symbols( TA_UDBase *udb,
                   TA_AddDataSourceParam dsParam,
                   const char *category,
                   const char *symbol)
{
   TA_RetCode retCode;
   unsigned int i;
   TA_StringTable *table = NULL;

   /* info not needed for getting just categories, but the driver insists on it */
   dsParam.info = "";  
   dsParam.symbol = symbol;
   retCode = TA_AddDataSource( udb, &dsParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   retCode = TA_SymbolTableAlloc( udb, category, &table );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   } 

   for( i=0; table && i < table->size; i++ )
   {
      printf( "%s\n", table->string[i] );
   }

   TA_SymbolTableFree( table );
  
   return 0;
}

int main( int argc, char *argv[] )
{
   Action theAction;
   TA_InitializeParam initParam;
   TA_UDBase *udb;
   TA_RetCode retCode;
   TA_Period period;
   TA_AddDataSourceParam dsParam;
   TA_SourceFlag flags;

   int retValue;

   /* Verify that there is the minimum required number 
    * of parameters.
    */
   if( argc < 6 )
   {
      print_usage( "Missing parameters" );
      return -1;
   }

   /* Verify that there are not too many parameters */
   if( argc > 8 )
   {
      print_usage( "Too Many parameters" );
      return -1;
   }

   /* Daily data by default. */
   period = TA_DAILY;
   flags = TA_NO_FLAGS;

   /* Check for the switch, and identify what needs to be done. */
   if ( strlen(argv[1]) > 1 && argv[1][strlen(argv[1])-1] == 'z') {
      flags |= TA_REPLACE_ZERO_PRICE_BAR;
   }
   if( strcmp( "-c", argv[1] ) == 0 )
      theAction = DISPLAY_CATEGORIES;
   else if( strcmp( "-s", argv[1] ) == 0 )
      theAction = DISPLAY_SYMBOLS;
   else if( strncmp( "-d", argv[1], 2 ) == 0 )
      theAction = DISPLAY_HISTORIC_DATA;
   else if( strncmp( "-dd", argv[1], 3 ) == 0 )
      theAction = DISPLAY_HISTORIC_DATA;
   else if( strncmp( "-dw", argv[1], 3 ) == 0 )
   {
      theAction = DISPLAY_HISTORIC_DATA;
      period = TA_WEEKLY;
   }
   else if( strncmp( "-dm", argv[1], 3 ) == 0 )
   {
      theAction = DISPLAY_HISTORIC_DATA;
      period = TA_MONTHLY;
   }
   else if( strncmp( "-dq", argv[1], 3 ) == 0 )
   {
      theAction = DISPLAY_HISTORIC_DATA;
      period = TA_QUARTERLY;
   }
   else if( strncmp( "-dy", argv[1], 3 ) == 0 )
   {
      theAction = DISPLAY_HISTORIC_DATA;
      period = TA_YEARLY;
   }
   else
   {
      print_usage( "Switch not recognized" );
      return -1;
   }

   /* Verify that there is at least a symbol and category 
    * on the command line (when applicable).
    */
   switch( theAction )
   {
   case DISPLAY_SYMBOLS:
      if( argc < 7 )
      {
         print_usage( "Symbol string missing" );
         return -1;
      }
      break;
   case DISPLAY_HISTORIC_DATA:
      if( argc < 8 )
      {
         print_usage( "Info string missing" );
         return -1;
      }
      break;
   case DISPLAY_CATEGORIES:
      /* Nothing to do. */
      break;
   }

   /* Initialize TA-LIB and create an unified database. */
   memset( &initParam, 0, sizeof( TA_InitializeParam ) );
   initParam.userLocalDrive = ".";

   retCode = TA_Initialize( &initParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   retCode = TA_SetFatalErrorHandler(fatal_error_handler);
   retCode = TA_UDBaseAlloc( &udb );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      TA_Shutdown();
      return -1;
   }

   memset( &dsParam, 0, sizeof( TA_AddDataSourceParam ) );
   dsParam.id = TA_SQL;

   dsParam.flags = flags;
   dsParam.location = argv[2];
   dsParam.username = argv[3];
   dsParam.password = argv[4];
   dsParam.category = argv[5];

   /* Do 'theAction' after making an uppercase copy
    * of the symbol and category.
    */
   switch( theAction )
   {
   case DISPLAY_HISTORIC_DATA:       
       dsParam.symbol = argv[6];
       dsParam.info = argv[7];
       retValue = print_data(udb, dsParam, period);
       break;

   case DISPLAY_SYMBOLS:
      retValue = print_symbols(udb, dsParam, argv[6], (argc>7? argv[7] : NULL));
       break;

   case DISPLAY_CATEGORIES:
       retValue = print_categories(udb, dsParam);
       break;

   default:
       retValue = -1;
       break;
   }

   /* Clean-up and exit. */
   TA_UDBaseFree( udb );
   TA_Shutdown();
   return retValue;
}
