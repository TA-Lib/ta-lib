/* Simple application for accessing Yahoo! Web Site using TA-LIB. */

/* To see how data is fetched from Yahoo! see print_data(). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ta_libc.h"

typedef enum
{
   DISPLAY_SYMBOLS,
   DISPLAY_CATEGORIES,
   DISPLAY_HISTORIC_DATA,
   UNKNOWN_ACTION
} Action;

typedef struct
{
  const char *string;
  Action theAction;
  TA_Period period;
  int flags;
} CommandLineSwitch;

static const CommandLineSwitch tableSwitch[] = 
{ {"-c", DISPLAY_CATEGORIES,    TA_DAILY,    0},
  {"-s", DISPLAY_SYMBOLS,       TA_DAILY,    0},
  {"-d", DISPLAY_HISTORIC_DATA, TA_DAILY,    0},
  {"-dd",DISPLAY_HISTORIC_DATA, TA_DAILY,    0},
  {"-dw",DISPLAY_HISTORIC_DATA, TA_WEEKLY,   0},
  {"-dm",DISPLAY_HISTORIC_DATA, TA_MONTHLY,  0},
  {"-dq",DISPLAY_HISTORIC_DATA, TA_QUARTERLY,0},
  {"-dy",DISPLAY_HISTORIC_DATA, TA_YEARLY,   0},
  {"-u", DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-ud",DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-uw",DISPLAY_HISTORIC_DATA, TA_WEEKLY,   TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-um",DISPLAY_HISTORIC_DATA, TA_MONTHLY,  TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-uq",DISPLAY_HISTORIC_DATA, TA_QUARTERLY,TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-uy",DISPLAY_HISTORIC_DATA, TA_YEARLY,   TA_DO_NOT_SPLIT_ADJUST|TA_DO_NOT_VALUE_ADJUST},
  {"-i", DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_SPLIT_ADJUST},
  {"-id",DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_SPLIT_ADJUST},
  {"-iw",DISPLAY_HISTORIC_DATA, TA_WEEKLY,   TA_DO_NOT_SPLIT_ADJUST},
  {"-im",DISPLAY_HISTORIC_DATA, TA_MONTHLY,  TA_DO_NOT_SPLIT_ADJUST},
  {"-iq",DISPLAY_HISTORIC_DATA, TA_QUARTERLY,TA_DO_NOT_SPLIT_ADJUST},
  {"-iy",DISPLAY_HISTORIC_DATA, TA_YEARLY,   TA_DO_NOT_SPLIT_ADJUST},
  {"-z", DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_VALUE_ADJUST},
  {"-zd",DISPLAY_HISTORIC_DATA, TA_DAILY,    TA_DO_NOT_VALUE_ADJUST},
  {"-zw",DISPLAY_HISTORIC_DATA, TA_WEEKLY,   TA_DO_NOT_VALUE_ADJUST},
  {"-zm",DISPLAY_HISTORIC_DATA, TA_MONTHLY,  TA_DO_NOT_VALUE_ADJUST},
  {"-zq",DISPLAY_HISTORIC_DATA, TA_QUARTERLY,TA_DO_NOT_VALUE_ADJUST},
  {"-zy",DISPLAY_HISTORIC_DATA, TA_YEARLY,   TA_DO_NOT_VALUE_ADJUST}
};
#define NB_SWITCH (sizeof(tableSwitch)/sizeof(CommandLineSwitch))

void print_usage( char *str ) 
{
   printf( "\n" );
   printf( "ta_yahoo V%s - Fetch market data from Yahoo!\n", TA_GetVersionString() );
   printf( "\n" );
   printf( "Usage: ta_yahoo -c\n" );
   printf( "       ta_yahoo -s <category>\n" );
   printf( "       ta_yahoo -{d,u,i,z}{d,w,m,q,y} <category> <symbol>\n" );
   printf( "       ta_yahoo -{d,u,i,z}{d,w,m,q,y} DIRECT=US <Yahoo! symbol>\n" );
   printf( "\n" );
   printf( "  -c Display all supported categories.\n" );
   printf( "  -s Display all symbols for a given category.\n" );
   printf( "  -d Fetch split and dividend adjusted data.\n" );
   printf( "  -u Fetch non adjusted data.\n" );
   printf( "  -i Fetch dividend-only adjusted data.\n" );
   printf( "  -z Fetch split-only adjusted data.\n" );
   printf( "\n" );
   printf( "  {d,w,m,q,y} = \"daily,weekly,monthly,quarterly,yearly\"\n" );
   printf( "\n" );
   printf( "  Specify \"DIRECT=US\" to use Yahoo! names directly with the US\n" );
   printf( "  web site of Yahoo! instead of the TA-Lib category/symbol index.\n" );
   printf( "  Use double quote when symbol name contains special characters.\n" );
   printf( "\n" );
   printf( "  Stock output is \"Date,Open,High,Low,Close,Volume\"\n" );
   printf( "  Funds output is \"Date,Close\". Date are \"mm-dd-yyyy\"\n" );
   printf( "\n" );
   printf( "  For period greater than daily, volume is the daily volume average.\n" );
   printf( "\n" );
   printf( "  Examples: ta_yahoo -s  US.NASDAQ.FUND\n" );
   printf( "            ta_yahoo -dd US.NASDAQ.STOCK MSFT\n" );
   printf( "            ta_yahoo -ud US.NASDAQ.STOCK MSFT\n" );
   printf( "            ta_yahoo -zw US.NASDAQ.STOCK MSFT\n" );
   printf( "            ta_yahoo -dd DIRECT=US 2812.TW\n" );
   printf( "            ta_yahoo -uy DIRECT=US \"^DJI\"\n" );
   printf( "\n" );
   printf( "  This utility may creates files \"y_xx.dat\" to speed\n" );
   printf( "  up subsequent remote access. These are automatically\n" );
   printf( "  re-generated and can be safely deleted.\n" );
   printf( "\n" );
   printf( "  * Data cannot be resdistributed. You must respect\n" );
   printf( "  * Yahoo! terms of service (www.yahoo.com)\n" );
   printf( "\n" );
   printf( "  Online help: http://ta-lib.org\n" );
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
                const char *country,
                const char *category,
                const char *symbol,
                TA_Period period,
                int flags )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam addSourceParam;
   TA_History *history;
   unsigned int i;
   TA_HistoryAllocParam histParam;

   /* Setup the datasource. */
   memset( &addSourceParam, 0, sizeof( TA_AddDataSourceParam ) );
   if( strcmp(category,"DIRECT=US") == 0 )
   {
     addSourceParam.id       = TA_YAHOO_ONE_SYMBOL;
     addSourceParam.info     = symbol;
     addSourceParam.category = category;
     addSourceParam.symbol   = symbol;
   }
   else
     addSourceParam.id = TA_YAHOO_WEB;

   addSourceParam.location = country;
   addSourceParam.flags = flags;
   retCode = TA_AddDataSource( udb, &addSourceParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   /* Get the historical data. */
   memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
   histParam.category = category;
   histParam.symbol   = symbol;
   histParam.period   = period;
   histParam.field    = TA_ALL;
   retCode = TA_HistoryAlloc( udb, &histParam, &history );

   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   if( history->nbBars == 0 )
   {
      printf( "No data available" );
   }
   else
   {
      for( i=0; i < history->nbBars; i++ )
      {
         printf( "%d-%d-%d", TA_GetMonth(&history->timestamp[i]),
                             TA_GetDay(&history->timestamp[i]),
                             TA_GetYear(&history->timestamp[i]) );
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

   /* Get ride of the historical data and return. */
   TA_HistoryFree( history );

   return 0;
}


int print_categories( TA_UDBase *udb )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam addYahooParam;
   unsigned int i;
   TA_StringTable *table;
   const char *listCountry[] = {"US","CA","UK","DE","ES","FR","IT","NO","SE","DK",NULL};

   memset( &addYahooParam, 0, sizeof( TA_AddDataSourceParam ) );
   addYahooParam.id = TA_YAHOO_WEB;

   i=0;
   while( listCountry[i++] != NULL )
   {
      addYahooParam.location = listCountry[i];
      retCode = TA_AddDataSource( udb, &addYahooParam );
      if( retCode != TA_SUCCESS )
      {
         print_error( retCode );
         return -1;
      }
   }
   
   retCode = TA_CategoryTableAlloc( udb, &table );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   } 

   for( i=0; i < table->size; i++ )
   {
      if( strcmp( table->string[i], "ZZ.OTHER.OTHER" ) )
         printf( "%s\n", table->string[i] );
   }

   TA_CategoryTableFree( table );

   return 0;
}

int print_symbols( TA_UDBase *udb,
                   const char *country,
                   const char *category )
{
   TA_RetCode retCode;
   TA_AddDataSourceParam addYahooParam;
   unsigned int i;
   TA_StringTable *table;

   memset( &addYahooParam, 0, sizeof( TA_AddDataSourceParam ) );
   addYahooParam.id = TA_YAHOO_WEB;
   addYahooParam.location = country;

   retCode = TA_AddDataSource( udb, &addYahooParam );
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

   for( i=0; i < table->size; i++ )
   {
      /* Display all categories... except the default one. */
      if( strcmp( table->string[i], "ZZ.OTHER.OTHER" ) )
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
   int flags;

   int retValue;
   unsigned int i, stringSize;

   char country[TA_CAT_COUNTRY_MAX_LENGTH+1];
   char category[TA_CATEGORY_MAX_LENGTH+1];
   char symbol[TA_SYMBOL_MAX_LENGTH+1];

   /* Verify that there is the minimum required number 
    * of parameters.
    */
   if( argc < 2 )
   {
      print_usage( "Missing parameters" );
      return -1;
   }

   /* Verify that there are not too many parameters */
   if( argc > 4 )
   {
      print_usage( "Too Many parameters" );
      return -1;
   }

   /* Daily data by default. */
   period    = TA_DAILY;
   theAction = UNKNOWN_ACTION;
   flags     = 0;

   /* Check for the switch, and identify what needs to be done. */
   for( i=0; (i < NB_SWITCH) && (theAction == UNKNOWN_ACTION); i++ )
   {
      if( strcmp(tableSwitch[i].string,argv[1]) == 0 )
      {
         period    = tableSwitch[i].period;
         theAction = tableSwitch[i].theAction;
         flags     = tableSwitch[i].flags;
      }
   }

   if( theAction == UNKNOWN_ACTION )
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
   case DISPLAY_HISTORIC_DATA:
      if( argc < 3 )
      {
         print_usage( "Category string missing" );
         return -1;
      }
      break;
   default:
      /* Nothing to do. */
      break;
   }

   if( theAction == DISPLAY_HISTORIC_DATA )
   {
      if( argc < 4 )
      {
         print_usage( "Symbol string missing" );
         return -1;
      }
   }

   
   /* Identify country when needed. */
   switch( theAction )
   {
   case DISPLAY_HISTORIC_DATA:
   case DISPLAY_SYMBOLS:
      /* Trap case where the user prefer the "DIRECT" approach
       * instead of the category/symbol index of TA-Lib.
       */
      if( strncmp( &argv[2][0], "DIRECT=", 7 ) == 0 )
      {
         if( strlen(&argv[2][0]) != 9 )
         {
           print_usage( "Do \"DIRECT=US\" to use the United State Yahoo! web site." );
           return -1;
         }
         i = 7;
      }
      else 
         i = 0;
      country[0] = argv[2][i];
      country[1] = argv[2][i+1];
      country[2] = '\0';
      break;
   case DISPLAY_CATEGORIES:
      country[0] = '\0';
   default:
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

   retCode = TA_UDBaseAlloc( &udb );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      TA_Shutdown();
      return -1;
   }

   /* Do 'theAction' after making an uppercase copy
    * of the symbol and category.
    */
   switch( theAction )
   {
   case DISPLAY_HISTORIC_DATA:       
       stringSize = strlen(argv[2]);
       if( stringSize > TA_CATEGORY_MAX_LENGTH )
          stringSize = TA_CATEGORY_MAX_LENGTH;
       for( i=0; i < stringSize; i++ )
          category[i] = toupper(argv[2][i]);
       category[stringSize] = '\0';

       stringSize = strlen(argv[3]);
       if( stringSize > TA_SYMBOL_MAX_LENGTH )
          stringSize = TA_SYMBOL_MAX_LENGTH;
       for( i=0; i < stringSize; i++ )
          symbol[i] = toupper(argv[3][i]);
       symbol[stringSize] = '\0';

       retValue = print_data(udb,country,category,symbol,period,flags);
       break;

   case DISPLAY_SYMBOLS:
       stringSize = strlen(argv[2]);
       if( stringSize > TA_CATEGORY_MAX_LENGTH )
          stringSize = TA_CATEGORY_MAX_LENGTH;
       for( i=0; i < stringSize; i++ )
          category[i] = toupper(argv[2][i]);
       category[stringSize] = '\0';

       retValue = print_symbols(udb,country,category);
       break;
   case DISPLAY_CATEGORIES:
       retValue = print_categories(udb);
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
