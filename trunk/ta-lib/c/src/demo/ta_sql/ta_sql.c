/* Simple application for accessing an SQL database using TA-LIB. */

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
} CommandLineAction;

static const CommandLineAction tableAction[] = 
{ {"-c",  DISPLAY_CATEGORIES,    TA_DAILY,    },
  {"-s",  DISPLAY_SYMBOLS,       TA_DAILY,    },
  {"-d",  DISPLAY_HISTORIC_DATA, TA_DAILY,    },
  {"-dd", DISPLAY_HISTORIC_DATA, TA_DAILY,    },
  {"-dw", DISPLAY_HISTORIC_DATA, TA_WEEKLY,   },
  {"-dm", DISPLAY_HISTORIC_DATA, TA_MONTHLY,  },
  {"-dq", DISPLAY_HISTORIC_DATA, TA_QUARTERLY },
  {"-dy", DISPLAY_HISTORIC_DATA, TA_YEARLY    },
  {"-d1", DISPLAY_HISTORIC_DATA, TA_1MIN      },
  {"-d5", DISPLAY_HISTORIC_DATA, TA_5MINS     },
  {"-d10",DISPLAY_HISTORIC_DATA, TA_10MINS    },
  {"-d15",DISPLAY_HISTORIC_DATA, TA_15MINS    },
  {"-d30",DISPLAY_HISTORIC_DATA, TA_30MINS    },
  {"-d1H",DISPLAY_HISTORIC_DATA, TA_1HOUR     }
};
#define NB_ACTION_SWITCH (sizeof(tableAction)/sizeof(CommandLineAction))

typedef struct
{
  const char *string;
  unsigned int addDataSourceFlags;
  unsigned int historyAllocFlags;
} CommandLineOption;

static const CommandLineOption tableOption[] =
{
  {"-z", TA_REPLACE_ZERO_PRICE_BAR, 0},
  {"-t", 0, TA_USE_TOTAL_VOLUME | TA_USE_TOTAL_OPENINTEREST },
  {"-i", 0, TA_ALLOW_INCOMPLETE_PRICE_BARS },
  {"-f", 0, TA_DISABLE_PRICE_VALIDATION }
};
#define NB_OPTION_SWITCH (sizeof(tableOption)/sizeof(CommandLineOption))

void fatal_error_handler(void)
{
   TA_FatalReport( stderr );
}

void print_usage( char *str ) 
{
   printf( "\n" );
   printf( "ta_sql V%s - SQL for market data\n", TA_GetVersionString() );
   printf( "\n" );
   printf( "Usage: ta_sql -c    <opt> <loc> <catsql>\n" );
   printf( "       ta_sql -s    <opt> <loc> <catsql> <cat> <symsql>\n" );
   printf( "       ta_sql -d<p> <opt> <loc> <catsql> <cat> <symsql> <sym> <infosql> [period]\n" );
   printf( "\n" );
   printf( "    Specify one of the following action switch:\n" );
   printf( "      -c     Display all supported categories\n" );
   printf( "      -s     Display all symbols for a given category\n" );
   printf( "      -d<p>  Display market data for the specified <p> period. Use\n" );
   printf( "             \"d,w,m,q,y\" for \"daily,weekly,monthly,quarterly,yearly\"\n" );
   printf( "             \"1,5,10,15,30,1H\" for \"1,5,10,15,30mins,1hour\"\n" );
   printf( "\n" );
   printf( "    <opt> are optional switches:\n" );
   printf( "      -z TA_REPLACE_ZERO_PRICE_BAR flag for TA_AddDataSource.\n" );
   printf( "      -t TA_USE_TOTAL_VOLUME and OPENINTEREST flag for TA_HistoryAlloc.\n" );
   printf( "      -i TA_ALLOW_INCOMPLETE_PRICE_BARS flag for TA_HistoryAlloc.\n" );
   printf( "      -f TA_DISABLE_PRICE_VALIDATION flag for TA_HistoryAlloc.\n" );
   printf( "\n" );
   printf( "      -u=<str>  Specify the username for TA_AddDataSource.\n" );
   printf( "      -p=<str>  Specify the password for TA_AddDataSource.\n" );
   printf( "\n" );
   printf( "    <loc>     is the TA_AddDataSource location parameter.\n" );   
   printf( "    <catsql>  is the TA_AddDataSource category parameter.\n" );
   printf( "    <cat>     is a string of a given category.\n" );
   printf( "    <symsql>  is the TA_AddDataSource symbol parameter.\n" );
   printf( "    <sym>     is a string of a given symbol.\n" );
   printf( "    <infosql> is the TA_AddDataSource info parameter.\n" );
   printf( "    [period]  optional value in sec specifying the database period." );
   printf( "\n" );
   printf( "  Market data output is \"Date,Open,High,Low,Close,Volume\"\n" );
   printf( "  or \"Date,Time,Open,High,Low,Close,Volume\" for intraday data.\n" );
   printf( "\n" );
   printf( "  Check http://ta-lib.org/d_source/d_sql.html for usage examples.\n" );
   printf( "\n" );
   printf( "  For help, try the mailing list at http://ta-lib.org\n" );
   printf( "\n" );
   if( str) printf( "Error: [%s]\n", str );  
}

void print_error( TA_RetCode retCode )
{
   TA_RetCodeInfo retCodeInfo;

   TA_SetRetCodeInfo( retCode, &retCodeInfo );
   printf( "\nError %d=%s:[%s]\n", retCode,
           retCodeInfo.enumStr,
           retCodeInfo.infoStr );
}

int print_data( TA_UDBase *udb, TA_HistoryAllocParam *haParam )
{
   TA_RetCode retCode;
   TA_History *history;
   unsigned int i;
   int intraday = (haParam->period < TA_DAILY);

   history = NULL;
   retCode = TA_HistoryAlloc( udb, haParam, &history );                              
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
         if (intraday)
         {
             printf( ",%02u:%02u:%02u", TA_GetHour(&history->timestamp[i]),
                                        TA_GetMin(&history->timestamp[i]),
                                        TA_GetSec(&history->timestamp[i]) );
         }
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


int print_categories( TA_UDBase *udb )                      
{
   TA_RetCode retCode;
   unsigned int i;
   TA_StringTable *table = NULL;
   
   retCode = TA_CategoryTableAlloc( udb, &table );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   } 

   for( i=0; i < table->size; i++ )
      printf( "%s\n", table->string[i] );

   TA_CategoryTableFree( table );
   return 0;
}


int print_symbols( TA_UDBase *udb, const char *category )
{
   TA_RetCode retCode;
   unsigned int i;
   TA_StringTable *table = NULL;

   retCode = TA_SymbolTableAlloc( udb, category, &table );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   } 

   for( i=0; table && i < table->size; i++ )
      printf( "%s\n", table->string[i] );

   TA_SymbolTableFree( table );  
   return 0;
}

int main( int argc, char *argv[] )
{
   Action theAction;
   TA_UDBase *udb;
   TA_RetCode retCode;

   TA_InitializeParam    initParam;
   TA_AddDataSourceParam dsParam;
   TA_HistoryAllocParam  haParam;

   int firstParam, nbParam, retValue;
   unsigned int i;

   if( argc <= 1 )
   {
      print_usage( NULL );
      return -1;
   }

   /* Initialize defaults */
   theAction = UNKNOWN_ACTION;
   memset( &initParam, 0, sizeof(TA_InitializeParam) );
   memset( &dsParam, 0, sizeof(TA_AddDataSourceParam) );
   memset( &haParam, 0, sizeof(TA_HistoryAllocParam) );
   
   /* Check for the action switch and identify what needs to be done. */
   if( strlen(argv[1]) > 1 )
   {
      for( i=0; i < NB_ACTION_SWITCH; i++ )
      {
        if( strcmp( tableAction[i].string, argv[1]) == 0 )
        {
           theAction = tableAction[i].theAction;
           haParam.period = tableAction[i].period;
           break;
        }
      }    
   }
   if( theAction == UNKNOWN_ACTION )
   {
      print_usage( "Action switch not recognized" );
      return -1;
   }

   /* Handle optional switch. */
   firstParam = 2; /* Will become index on first non-switch parameter. */
   while( firstParam < argc )
   {
      if( argv[firstParam][0] != '-' ) break;

      if( strlen(argv[firstParam]) >= 4 && 
          (argv[firstParam][2] == '=') )
      {
         if( argv[firstParam][1] == 'u' )
            dsParam.username = &argv[firstParam][3];
         else if( argv[firstParam][1] == 'p' )
            dsParam.password = &argv[firstParam][3];
         else
         {
            print_usage( "Unrecognized optional switch" );
            return -1;
         }
      }
      else
      {
         for( i=0; i < NB_OPTION_SWITCH; i++ )
         {
            if( strcmp(argv[firstParam],tableOption[i].string) == 0 )
            {
               dsParam.flags |= tableOption[i].addDataSourceFlags;
               haParam.flags |= tableOption[i].historyAllocFlags;
               break;
            }                  
         }
         if( i == NB_OPTION_SWITCH )
         {
            print_usage( "Unrecognized optional switch" );
            return -1;
         }
      } 
      firstParam++;
   }

   /* Check the number of required parameter after the switches. */
   nbParam = argc - firstParam;
   if( ((theAction == DISPLAY_CATEGORIES)    && (nbParam < 2)) || 
       ((theAction == DISPLAY_SYMBOLS)       && (nbParam < 4)) ||
       ((theAction == DISPLAY_HISTORIC_DATA) && (nbParam < 6)))
   {
      print_usage( "Missing Parameters" );
      return -1;
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

   /* Set the parameters for TA-Lib. */
   haParam.field = TA_ALL;
   dsParam.id = TA_SQL;
   dsParam.location = argv[firstParam];
   dsParam.info = "";
   if( nbParam > 1 )
      dsParam.category = argv[firstParam+1];
   if( nbParam > 2 )
      haParam.category = argv[firstParam+2];
   if( nbParam > 3 )
      dsParam.symbol = argv[firstParam+3];
   if( nbParam > 4 )
      haParam.symbol = argv[firstParam+4];
   if( nbParam > 5 )
      dsParam.info = argv[firstParam+5];
   if( nbParam > 6 )
      dsParam.period = atoi(argv[firstParam+6]);

   /* Add the data source. */
   retCode = TA_AddDataSource( udb, &dsParam );
   if( retCode != TA_SUCCESS )
   {
      print_error( retCode );
      return -1;
   }

   /* Perform the requested action. */
   switch( theAction )
   {
   case DISPLAY_CATEGORIES:
       retValue = print_categories(udb);
       break;

   case DISPLAY_SYMBOLS:
      retValue = print_symbols(udb, haParam.category);
      break;

   case DISPLAY_HISTORIC_DATA:       
       retValue = print_data(udb, &haParam);
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
