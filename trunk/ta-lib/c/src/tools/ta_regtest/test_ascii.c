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
 *  070401 MF   First version.
 *
 */

/* Description:
 *         Regression testing of the functionality provided
 *         by the ASCII data source.
 *
 *         Indirectly, this will exercise the TA_FileIndex and
 *         TA_ReadOp objects.
 *
 * This test assume that the software is running from
 * the directory ta-lib/c/bin.
 *
 * This test assume that the following directory structure
 * exist in src/tools/ta_regtest/tstdir:
 *
 *  a\x.txt
 *  B\XYZ.TXT
 *  AA\X1.TXT
 *  AB\X2.TXT
 *  AAC\ASD.TX0
 *  AAC\ASD.TX
 *  AAC\ASD.TX1
 *  AAC\ASDF.TX1
 *  L1\L2\z1.DAT
 *  L1\L2\L3\z2.DAT
 *  L1\L22\L31\z3.DAT
 *  L1\L22\L32\z4.DAT
 *  L1\L22\L32\Z4\z5.DAT
 *
 * The content of the file is irrelevant.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ta_test_priv.h"
#include "sfl.h"
#include "ta_trace.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   const char *category;
   const char *symbol;
} TA_CatSym;

typedef struct
{
   unsigned int testId;
   unsigned int linkToNext; /* Boolean */
   const char *theLocationString;
   TA_CatSym  **theListOfExpectedCatSym; /* All expected category/symbol. */
   TA_RetCode expectedRetCode;           /* Some are expected to fail. */
} TA_FileGlobingTest;

typedef struct
{
   TA_CatSym **theListOfExpectedCatSym;
   unsigned int nbSymbolNotFound;
   unsigned int nbSymbolFound;
} TA_ForEachCatSymData;


static void TA_VerifyEachSymbolInCatSymTable( TA_UDBase *unifiedDatabase,
                                              const char *category,
                                              const char *symbol,
                                              void *opaqueData );

typedef struct
{
   TA_CatSym symbolToFind;
   unsigned int found; /* boolean */
} TA_IndexSearchData;

static void TA_SearchInIndexForCatSym( TA_UDBase *unifiedDatabase,
                                       const char *category,
                                       const char *symbol,
                                       void *opaqueData );

/**** Local functions declarations.    ****/
static int test_file_globing( TA_UDBase *uDBase, const TA_FileGlobingTest *tstData );
static int test_parsing_equivalence( void );

/**** Local variables definitions.     ****/

/* List of possible Category/Symbol combination */
TA_CatSym ZZdotOTHERdotOTHERcommaX    = {"ZZ.OTHER.OTHER","X"};
TA_CatSym ZZdotOTHERdotOTHERcommaXYZ  = {"ZZ.OTHER.OTHER","XYZ"};
TA_CatSym ZZdotOTHERdotOTHERcommaX1   = {"ZZ.OTHER.OTHER","X1"};
TA_CatSym ZZdotOTHERdotOTHERcommaX2   = {"ZZ.OTHER.OTHER","X2"};

TA_CatSym ZZdotOTHERdotOTHERcommaASDdotTX0  = {"ZZ.OTHER.OTHER","ASD.TX0"};
TA_CatSym ZZdotOTHERdotOTHERcommaASDdotTX   = {"ZZ.OTHER.OTHER","ASD.TX" };
TA_CatSym ZZdotOTHERdotOTHERcommaASDdotTX1  = {"ZZ.OTHER.OTHER","ASD.TX1" };
TA_CatSym ZZdotOTHERdotOTHERcommaASDFdotTX1 = {"ZZ.OTHER.OTHER","ASDF.TX1"};

TA_CatSym AcommaX   = {"A","X"};
TA_CatSym BcommaXYZ = {"B","XYZ"};
TA_CatSym AAcommaX1 = {"AA","X1"};
TA_CatSym ABcommaX2 = {"AB","X2"};

TA_CatSym AdotOTHERdotOTHERcommaXdotTXT   = {"A.OTHER.OTHER","X.TXT"};
TA_CatSym BdotOTHERdotOTHERcommaXYZdotTXT = {"B.OTHER.OTHER","XYZ.TXT"};
TA_CatSym AAdotOTHERdotOTHERcommaX1dotTXT = {"AA.OTHER.OTHER","X1.TXT"};
TA_CatSym ABdotOTHERdotOTHERcommaX2dotTXT = {"AB.OTHER.OTHER","X2.TXT"};

TA_CatSym ZZdotAdotOTHERcommaX1 = {"ZZ.A.OTHER","X1"};
TA_CatSym ZZdotBdotOTHERcommaX2 = {"ZZ.B.OTHER","X2"};

TA_CatSym TDIdotAdotOTHERcommaX   = {"TDI.A.OTHER","X"};
TA_CatSym TDIdotBdotOTHERcommaXYZ = {"TDI.B.OTHER","XYZ"};
TA_CatSym TDIdotAAdotOTHERcommaX1 = {"TDI.AA.OTHER","X1"};
TA_CatSym TDIdotABdotOTHERcommaX2 = {"TDI.AB.OTHER","X2"};

TA_CatSym AACdotOTHERdotOTHERcommaASDdotTX   = { "AAC.OTHER.OTHER", "ASD.TX"};
TA_CatSym AACdotOTHERdotOTHERcommaASDdotTX0  = { "AAC.OTHER.OTHER", "ASD.TX0"};
TA_CatSym AACdotOTHERdotOTHERcommaASDdotTX1  = { "AAC.OTHER.OTHER", "ASD.TX1"};
TA_CatSym AACdotOTHERdotOTHERcommaASDFdotTX1 = { "AAC.OTHER.OTHER", "ASDF.TX1"};

TA_CatSym ZZdotAdotOTHERcommaX   = {"ZZ.A.OTHER", "X" };
TA_CatSym AdotOTHERdotOTHERcommaX = {"A.OTHER.OTHER", "X" };
TA_CatSym ZZdotOTHERdotAcommaX   = {"ZZ.OTHER.A", "X" };

TA_CatSym DIRdotAdotACcommaDdotTX0 = {"DIR.A.AC","D.TX0"};
TA_CatSym DIRdotAdotACcommaDdotTX  = {"DIR.A.AC","D.TX"};
TA_CatSym DIRdotAdotACcommaDdotTX1 = {"DIR.A.AC","D.TX1"};
TA_CatSym DIRdotAdotACcommaDFdotTX1= {"DIR.A.AC","DF.TX1"};

/* The expected result of each of the file globing test. */
TA_CatSym *test1Result[] = 
{
   &ZZdotOTHERdotOTHERcommaX,
   &ZZdotOTHERdotOTHERcommaXYZ,
   &ZZdotOTHERdotOTHERcommaX1,
   &ZZdotOTHERdotOTHERcommaX2,
   NULL
};

TA_CatSym *test2Result[] = 
{
   &AcommaX,
   &BcommaXYZ,
   &AAcommaX1,
   &ABcommaX2,
   NULL
};

TA_CatSym *test3Result[] = 
{
   &AdotOTHERdotOTHERcommaXdotTXT,
   &BdotOTHERdotOTHERcommaXYZdotTXT,
   &AAdotOTHERdotOTHERcommaX1dotTXT,
   &ABdotOTHERdotOTHERcommaX2dotTXT,
   &AACdotOTHERdotOTHERcommaASDdotTX, 
   &AACdotOTHERdotOTHERcommaASDdotTX0,
   &AACdotOTHERdotOTHERcommaASDdotTX1, 
   &AACdotOTHERdotOTHERcommaASDFdotTX1,
   NULL
};

TA_CatSym *test4Result[] = 
{
   &ZZdotAdotOTHERcommaX1,
   &ZZdotBdotOTHERcommaX2,
   NULL
};

TA_CatSym *test5Result[] = 
{
   &TDIdotAdotOTHERcommaX,
   &TDIdotBdotOTHERcommaXYZ,
   &TDIdotAAdotOTHERcommaX1,
   &TDIdotABdotOTHERcommaX2,
   NULL
};

TA_CatSym *test6Result[] ={ &ZZdotAdotOTHERcommaX,NULL };
TA_CatSym *test7Result[] ={ &ZZdotOTHERdotAcommaX,NULL };
TA_CatSym *test8Result[] ={ &AdotOTHERdotOTHERcommaX,NULL };

TA_CatSym *test9Result[] =
{
   &DIRdotAdotACcommaDdotTX0,
   &DIRdotAdotACcommaDdotTX,
   &DIRdotAdotACcommaDdotTX1,
   &DIRdotAdotACcommaDFdotTX1,
   NULL
};

static TA_FileGlobingTest fileGlobingTestTable[] =
{
   /* Side Note: TA-LIB will adapt on Windows/Unix for using
    *            the correct directory seperator. So there
    *            is no problem to mix these here.
    */

   /*** Result 1 ***/

   /* Add the file individually. */
   {100, 1, "..\\src\\tools\\ta_regtest\\TSTDIR\\A\\X.TXT",          NULL, TA_SUCCESS},
   {100, 1, "..\\src\\tools\\ta_regtest\\TSTDIR\\B\\XYZ.TXT",        NULL, TA_SUCCESS},
   {100, 1, "..\\src\\tools\\ta_regtest\\TSTDIR\\AA\\X1.TXT",        NULL, TA_SUCCESS},
   {100, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\AB\\X2.TXT", test1Result, TA_SUCCESS},

   /* Add the file individually + use some wildcard directory. */
   {101, 1, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\X.TXT",          NULL, TA_SUCCESS},
   {101, 1, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\XYZ.TXT",        NULL, TA_SUCCESS},
   {101, 1, "..\\?rc\\tools\\ta_regtest\\TSTDIR\\*\\X1.TXT",         NULL, TA_SUCCESS},
   {101, 0, "..\\sr?\\tools\\ta_regtest\\TSTDIR\\*\\X2.TXT",  test1Result, TA_SUCCESS},

   /* Add the file individually + use some wildcard directory + some empty source. */
   {102, 1, "..\\src\\DO_NOT_EXIST\\XYZ.TXT",                        NULL, TA_NO_DATA_SOURCE },
   {102, 1, "..\\src\\tools\\ta_regtest\\??????\\*\\X.TXT",          NULL, TA_SUCCESS},
   {102, 1, "..\\src/DO_NOT_EXIST\\XYZ.TXT",                         NULL, TA_NO_DATA_SOURCE },
   {102, 1, "..\\src\\tools\\ta_regtest\\*\\*\\XYZ.TXT",             NULL, TA_SUCCESS},
   {102, 1, "..\\src/DO_NOT_EXIST/XYZ.TXT",                          NULL, TA_NO_DATA_SOURCE },
   {102, 1, "..\\src\\tools\\ta_regtest\\TSTDI?\\*\\X1.TXT",         NULL, TA_SUCCESS},
   {102, 1, "..\\src\\tools\\ta_regtest\\?STDIR\\*\\XBAD.TXT",       NULL, TA_NO_DATA_SOURCE },
   {102, 0, "..\\src\\tools\\ta_regtest\\?STDIR\\*\\X2.TXT",  test1Result, TA_SUCCESS},

   /* Test some simple directory/symbol '*' wildcard. */
   {110, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\*.TXT",    test1Result, TA_SUCCESS },
   {111, 0, "../src/tools/ta_regtest/TSTDIR/*/*.TXT",          test1Result, TA_SUCCESS },
   {112, 0, "../src\\tools/ta_regtest\\TSTDIR/*\\*.TXT",       test1Result, TA_SUCCESS },
   {113, 0, "..\\src/tools\\ta_regtest/TSTDIR\\*/*.TXT",       test1Result, TA_SUCCESS },

   /* These '?' should fail since there is no symbol field. */
   {130, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\????.TXT",     NULL, TA_INVALID_PATH },
   {131, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//???.TXT",      NULL, TA_INVALID_PATH },
   {132, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.TXT",        NULL, TA_INVALID_PATH },
   {133, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.?XT",        NULL, TA_INVALID_PATH },
   {134, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.?XT",        NULL, TA_INVALID_PATH },
   {135, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.T?T",        NULL, TA_INVALID_PATH },
   {136, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.TX?",        NULL, TA_INVALID_PATH },
   {137, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.?",          NULL, TA_INVALID_PATH },
   {138, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?",            NULL, TA_INVALID_PATH },
   {139, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//??",           NULL, TA_INVALID_PATH },
   {140, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*//?.???",        NULL, TA_INVALID_PATH },
   {141, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\?.???",        NULL, TA_INVALID_PATH },
   {142, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*\\????????.???", NULL, TA_INVALID_PATH },
   {143, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\*/????????.???",  NULL, TA_INVALID_PATH },

   /* Make sure that the '.' is well supported. */
   {150, 0, ".\\..\\src\\tools\\ta_regtest\\TSTDIR\\*\\*.TXT",  test1Result, TA_SUCCESS },
   {151, 0, "./..\\src\\tools\\ta_regtest\\TSTDIR\\*\\*.TXT",  test1Result, TA_SUCCESS },
   {152, 0, ".\\..\\.\\src\\tools\\ta_regtest\\TSTDIR\\*\\*.TXT",  test1Result, TA_SUCCESS },
   {153, 0, "./.././src\\tools\\ta_regtest\\TSTDIR\\*\\*.TXT",  test1Result, TA_SUCCESS },
   {154, 0, "./.././src\\tools\\ta_regtest\\TSTDIR\\*\\.\\*.TXT",  test1Result, TA_SUCCESS },
   {155, 0, "./.././src\\tools\\ta_regtest\\TSTDIR\\*/./*.TXT",  test1Result, TA_SUCCESS },
   /*** Result 2 ***/
   /* Check that CAT field is working. */
   {200, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\[CAT]\\[SYM].TXT", test2Result, TA_SUCCESS},

   /*** Result 3 ***/
   /* Check that CATC field is working. */
   {300, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\[CATC]\\[SYM]", test3Result, TA_SUCCESS},

   /*** Result 4 ***/
   /* Check that CATX field is working. */
   {400, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\A[CATX]\\[SYM].TXT", test4Result, TA_SUCCESS},

   /*** Result 5 ***/
   /* Check that CATX+CATC field combination are working. */
   {500, 0, "..\\src\\tools\\ta_regtest\\TS[CATC]R\\[CATX]\\[SYM].TXT", test5Result, TA_SUCCESS},

   /*** Result 6 ***/
   /* Test one character CATX. */
   {600, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\[CATX]\\X.TXT", test6Result, TA_SUCCESS},

   /*** Result 7 ***/
   /* Test one character CATT. */
   {700, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\[CATT]\\X.TXT", test7Result, TA_SUCCESS},

   /*** Result 8 ***/
   /* Test one character CATC. */
   {800, 0, "..\\src\\tools\\ta_regtest\\TSTDIR\\[CATC]\\X.TXT", test8Result, TA_SUCCESS},

   /*** Result 9 ***/
   /* Test the combination of all category component. */
   {900, 0, "..\\src\\tools\\ta_regtest\\TST[CATC]\\A[CATT]\\[CATX].TXT", NULL, TA_INVALID_PATH},
   {901, 0, "..\\src\\tools\\ta_regtest\\TST[CATC]\\A[CATT]\\[CATX]S[SYM]", test9Result, TA_SUCCESS}

   /* As bug are found or time is available, a lot more 
    * of systematic regression tests needs to be added.
    */
};

#define nbFileGlobingTest (sizeof(fileGlobingTestTable)/sizeof(TA_FileGlobingTest))

typedef struct
{
   const char *fileInfo;
} FieldDef;

FieldDef fieldTable[] =
{
   {"[-H][M][D][Y][O][H][L][C][V]"},   
   {"[M][D][Y][HOUR][MIN][L][O][C][H][V]"},
};

#define TA_NB_FIELD_DEF (sizeof(fieldTable)/sizeof(FieldDef))

/**** Global functions definitions.   ****/
ErrorNumber test_ascii( void )
{
   unsigned int again, i, startupDatasource;
   ErrorNumber retValue;
   int testRetValue;
   TA_UDBase *udb;

   printf( "Testing ASCII data source\n" );

   /* A "dummy" test just to verify that these two
    * utility function do not cause memory leak.
    */
   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    
   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Test directory/category/file globing. */
   for( i=0; i < nbFileGlobingTest; i++ )
   {
      retValue = allocLib( &udb );
      if( retValue != TA_TEST_PASS )
         return retValue;

      startupDatasource = i;
      do
      {
         again = 0;
         testRetValue = test_file_globing( udb, &fileGlobingTestTable[i] );

         if( testRetValue == -1 )
         {
            i++;
            again = 1;
         }
      } while( again && i < nbFileGlobingTest );

      if( i == nbFileGlobingTest )
      {
         printf( "ta_regtest reference data %d is invalid.\n", startupDatasource );
         return TA_INTERNAL_ERROR(128);
      }

      if( testRetValue != TA_TEST_PASS )
      {
         printf( "File globing test #%d failed with value %d.\n",
                 fileGlobingTestTable[i].testId, testRetValue );
         freeLib( udb );
         return testRetValue;
      }

      retValue = freeLib( udb );
      if( retValue != TA_TEST_PASS )
         return retValue;
   }

   /* Verify reading of ASCII data. */

   /* Test equivalence file. All files in the
    * ta-lib/src/tools/ta_Regtest/sampling have
    * a specific name format TST[X]\Z[S].TXT
    * 
    * [X]   represent a group of file that have ALL the same data
    *       (but the content could be presentend differently).
    *
    * [S]   is a unique id. It is further decompose in two
    *       fields 'xxx_yyy'
    *
    * 'xxx' is a unique number to make the filename unique.
    *
    * 'yyy' represent the order of the fields.
    *
    * Examples:
    *      TST001\Z001_001.TXT
    *      TST001\Z001_002.TXT
    *      TST003\Z001_002.TXT
    *
    * The idea is to test ALL file having the same [X] for
    * being equivalent with TA_HistoryAlloc.
    */
   retValue = test_parsing_equivalence();
   if( retValue != TA_SUCCESS )
   {
      printf( "ASCII Test equivalence failed test with value %d.\n",
              retValue );
      return retValue;
   }

   return TA_TEST_PASS; /* Succcess. */
}

/**** Local functions definitions.     ****/
static int test_file_globing( TA_UDBase *uDBase, const TA_FileGlobingTest *tstData )
{  
   TA_RetCode retCode;
   TA_AddDataSourceParam param;
   TA_ForEachCatSymData  catSymData;
   TA_IndexSearchData    indexSearchData;
   unsigned int i;
   TA_CatSym **catSymTablePtr;

   /* Add all datasource having the same testId.  */
   memset( &param, 0, sizeof( TA_AddDataSourceParam ) );
   param.id = TA_ASCII_FILE;
   param.location = tstData->theLocationString;
   param.info     = "[Y][O]"; /* Mandatory, but not important for the test. */

   retCode = TA_AddDataSource( uDBase, &param );
   if( retCode != tstData->expectedRetCode )
   {
      printf( "TA_AddDataSource did not behave as expected.\n" );
      printf( "Expected value: %d, returned value: %d\n",
              tstData->expectedRetCode, retCode );

      return TA_TESTASCII_UNEXPECTED_RETCODE;
   }

   /* Return to the caller for adding another data source. */
   if( tstData->linkToNext )
      return -1;

   /* TA_Report( uDBase, stdout, NULL ); */

   if( retCode == TA_SUCCESS )
   {
       /* On TA_SUCCESS, 2 verifications are done:
        * 
        * (1) Verify that all symbols in the unified database
        *     are in the list of expected symbol.
        *
        * (2) Verify that all symbols in the expected list
        *     are in the unified database.
        */
    
       /*** (1) ***/
       catSymData.nbSymbolFound    = 0;
       catSymData.nbSymbolNotFound = 0;
       catSymData.theListOfExpectedCatSym = tstData->theListOfExpectedCatSym;
       
       retCode = TA_ForEachSymbol( uDBase, TA_VerifyEachSymbolInCatSymTable, &catSymData );
       if( retCode != TA_SUCCESS )
       {
          printf( "TA_ForEachSymbol failed [%d]\n", retCode );
          return TA_TESTASCII_FOREACHSYMBOL_FAILED;
       }

       if( catSymData.nbSymbolNotFound != 0 )
          return TA_TESTASCII_CATSYM_NOT_FOUND;
       
       /*** (2) ***/
       i = 0;
       catSymTablePtr = tstData->theListOfExpectedCatSym;
       while( catSymTablePtr[i] != NULL)
       {
          indexSearchData.found = 0;
          indexSearchData.symbolToFind.category = catSymTablePtr[i]->category;
          indexSearchData.symbolToFind.symbol   = catSymTablePtr[i]->symbol;
          retCode = TA_ForEachSymbol( uDBase, TA_SearchInIndexForCatSym, &indexSearchData );
          if( retCode != TA_SUCCESS )
          {
             printf( "TA_ForEachSymbol failed [%d]\n", retCode );
             return TA_TESTASCII_SEARCHININDEX_FAILED;
          }
    
          if( !indexSearchData.found )
          {
             printf( "Can't find %s.%s in the TA-LIB index.\n", 
                     indexSearchData.symbolToFind.category,
                     indexSearchData.symbolToFind.symbol );
             return TA_TESTASCII_CANTFIND_CATSYM_IN_INDEX;
          }
    
          i++;
       }
   }   

   return TA_TEST_PASS; /* Success. */
}

static void TA_VerifyEachSymbolInCatSymTable( TA_UDBase *unifiedDatabase,
                                              const char *category,
                                              const char *symbol,
                                              void *opaqueData )
{
   TA_ForEachCatSymData *catSymData;
   TA_CatSym **catSymTablePtr;
   unsigned int i;

   (void) unifiedDatabase;

   /* Ignore CVS related directories */
   if( (lexcmp( symbol, "Entries" ) == 0) ||
       (lexcmp( symbol, "Repository" ) == 0) ||
       (lexcmp( symbol, "Root" ) == 0) ||
       (lexcmp( category, "CVS.OTHER.OTHER") == 0))
      return;

   catSymData = (TA_ForEachCatSymData *)opaqueData;
   catSymTablePtr = catSymData->theListOfExpectedCatSym;

   i = 0;
   while( catSymTablePtr[i] != NULL)
   {
      if( (lexcmp( catSymTablePtr[i]->category, category) == 0) &&
          (lexcmp( catSymTablePtr[i]->symbol, symbol ) == 0))
      {
         catSymData->nbSymbolFound++;
         return;
      }

      i++;
   }

   printf( "CatSym not found for %s,%s\n", category, symbol );
   catSymData->nbSymbolNotFound++;
}

static void TA_SearchInIndexForCatSym( TA_UDBase *unifiedDatabase,
                                       const char *category,
                                       const char *symbol,
                                       void *opaqueData )
{
   TA_IndexSearchData *searchData;

   (void) unifiedDatabase;

   searchData = (TA_IndexSearchData *)opaqueData;

   if( (lexcmp( searchData->symbolToFind.category, category ) == 0) &&
       (lexcmp( searchData->symbolToFind.symbol, symbol) == 0) )
      searchData->found = 1;
}

static int test_parsing_equivalence( void )
{
   int retValue, fieldNb;
   TA_AddDataSourceParam param;
   TA_UDBase *udb, *udbEqv;
   TA_RetCode retCode;
   TA_StringTable *catTable, *symTable;
   unsigned int i,j;
   TA_History *history;
   TA_History *refHistory;
   char buffer1024[1024];
   char buffer200[200];
   TA_Timestamp ts1, ts2;
   int period;
   TA_HistoryAllocParam histParam;

   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   /* Get the complete list of files to process. */
   memset( &param, 0, sizeof( TA_AddDataSourceParam ) );  
   param.id = TA_ASCII_FILE;
   param.location = "..\\src\\tools\\ta_regtest\\sampling\\TST[CAT]\\Z[SYM].TXT";
   param.info = "[Y]"; /* Mandatory, but not used. */
   retCode = TA_AddDataSource( udb, &param );
   if( retCode != TA_SUCCESS )
   {
      printf( "Cannot access ASCII sampling directory\n" );
      reportError( "TA_AddDataSource", retCode );
      return TA_TESTASCII_SAMPLING_FILE_NOT_FOUND;
   }

   /* TA_Report( udb, stdout, NULL ); */

   retCode = TA_CategoryTableAlloc( udb, &catTable );
   if( retCode != TA_SUCCESS )
   {      
      freeLib( udb );
      return TA_TESTASCII_CATTABLE_ALLOC_ERROR;
   }
   else
   {
      for( i=0; i < catTable->size; i++ )
      {
         retCode = TA_SymbolTableAlloc( udb, catTable->string[i], &symTable );
         if( retCode != TA_SUCCESS )
         {
            TA_CategoryTableFree( catTable );
            freeLib( udb );
            return TA_TESTASCII_SYMTABLE_ALLOC_ERROR;
         }

         refHistory = NULL;
         udbEqv = NULL;

         for( j=0; j < symTable->size; j++ )
         {
            retCode = TA_UDBaseAlloc( &udbEqv );
         
            sprintf( buffer1024, 
                     "..\\src\\tools\\ta_regtest\\sampling\\TST%s\\Z%s.TXT",
                     catTable->string[i],
                     symTable->string[j] );

            strcpy( buffer200, "Z" );
            strcat( buffer200, symTable->string[j] );

            /* Validate the very specific file name nomenclature. */
            if( !isdigit( buffer200[1] ) ||
                !isdigit( buffer200[2] ) ||
                !isdigit( buffer200[3] ) ||
                !isdigit( buffer200[5] ) ||
                !isdigit( buffer200[6] ) ||
                !isdigit( buffer200[7] ) ||
                buffer200[4] != '_'      ||
                buffer200[8] != '\0' )
            {
               printf( "Invalid file name %s\n", buffer200 );
               if( refHistory )
                  TA_HistoryFree( refHistory );
               TA_SymbolTableFree( symTable );
               TA_CategoryTableFree( catTable );
               if( udbEqv )
                  TA_UDBaseFree( udbEqv );
               freeLib( udb );
               return TA_TESTASCII_EQV_BAD_FILENAME;
            }

            /* Identify the field who is going to be used for TA_AddDataSourfce. */
            fieldNb = atoi(&buffer200[5]);
            if( (fieldNb <= 0) || (fieldNb > (int)TA_NB_FIELD_DEF) )
            {
               printf( "Invalid file name %s\n", buffer200 );
               if( refHistory )
                  TA_HistoryFree( refHistory );
               TA_SymbolTableFree( symTable );
               TA_CategoryTableFree( catTable );
               if( udbEqv )
                  TA_UDBaseFree( udbEqv );
               freeLib( udb );
               return TA_TESTASCII_EQV_BAD_FIELD_ID;
            }

            memset( &param, 0, sizeof( TA_AddDataSourceParam ) );  
            param.id = TA_ASCII_FILE;
            param.location = &buffer1024[0];
            param.info = fieldTable[fieldNb-1].fileInfo;
            param.category = catTable->string[i];
            
            /* Test #2 is specifically to test the TA_REPLACE_ZERO_PRICE_BAR flag */
            if( strcmp( catTable->string[i], "002" ) == 0 )
               param.flags = TA_REPLACE_ZERO_PRICE_BAR;

            retCode = TA_AddDataSource( udbEqv, &param );
            if( retCode != TA_SUCCESS )
            {
               printf( "TA_AddDataSource failed with %d (%s)\n", retCode, &buffer200[0] );
               if( refHistory )
                  TA_HistoryFree( refHistory );
               TA_SymbolTableFree( symTable );
               TA_CategoryTableFree( catTable );
               if( udbEqv )
                  TA_UDBaseFree( udbEqv );
               freeLib( udb );
               return TA_TESTASCII_EQV_ADDDATASOURCE;
            }

            /* Test #2 use minutes data */
            if( strcmp( catTable->string[i], "002" ) == 0 )
               period = TA_1MIN;
            else
               period = TA_DAILY;

            memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
            histParam.category = catTable->string[i];
            histParam.symbol   = &buffer200[0];
            histParam.period   = period;
            histParam.field    = TA_ALL;
            retCode = TA_HistoryAlloc( udbEqv, &histParam, &history );

            if( retCode != TA_SUCCESS )
            {
               printf( "TA_HistoryAlloc failed with %d (%s)\n", retCode, symTable->string[j] );
               if( refHistory )
                  TA_HistoryFree( refHistory );
               TA_SymbolTableFree( symTable );
               TA_CategoryTableFree( catTable );
               if( udbEqv )
                  TA_UDBaseFree( udbEqv );
               freeLib( udb );
               return TA_TESTASCII_EQV_HISTORYALLOC;
            }

            if( refHistory == NULL )
               refHistory = history;
            else
            {
               retValue = TA_TEST_PASS; /* Will change if something goes wrong */

               if( refHistory->nbBars != history->nbBars )
               {
                  printf( "Nb bar different with TST%s,Z%s (ref:%d,tst:%d)\n",
                          catTable->string[i],symTable->string[j],
                          refHistory->nbBars,
                          history->nbBars);
                  retValue = TA_TESTASCII_EQV_DIFF_NBBARS;
               }
               else if( refHistory->period != history->period )
               {
                  printf( "Period different with TST%s,%s (ref:%d,tst:%d)\n",
                          catTable->string[i],symTable->string[j],
                          refHistory->period,
                          history->period);
                  retValue = TA_TESTASCII_EQV_DIFF_PERIOD;
               }
               #define CHECK_MEMBER(param) \
               else if( (refHistory->param && !history->param) || \
                        (!refHistory->param && history->param) ) \
               { \
                  printf( "Interpretation error with TST%s,Z%s,%s (ref:%d,tst:%d)\n", \
                          catTable->string[i],symTable->string[j], \
                          #param, \
                          refHistory->param?1:0, \
                          history->param?1:0); \
                  retValue = TA_TESTASCII_EQV_DIFF_HISTORY_PTR; \
               }

               CHECK_MEMBER( open )
               CHECK_MEMBER( high )
               CHECK_MEMBER( low )
               CHECK_MEMBER( close )
               CHECK_MEMBER( volume )
               CHECK_MEMBER( openInterest )
               CHECK_MEMBER( timestamp )
               #undef CHECK_MEMBER
               else if( !TA_HistoryEqual( refHistory, history ) )
               {
                  printf( "TA_HistoryEqual sees difference with TST%s,Z%s\n",
                          catTable->string[i],symTable->string[j] );
                  retValue = TA_TESTASCII_EQV_DIFF_DATA;
               }

               if( retValue != TA_TEST_PASS )
               {
                  TA_HistoryFree( history );
                  if( refHistory )
                     TA_HistoryFree( refHistory );
                  TA_SymbolTableFree( symTable );
                  TA_CategoryTableFree( catTable );
                  if( udbEqv )
                     TA_UDBaseFree( udbEqv );
                  freeLib( udb );
                  return retValue;                  
               }

               retCode = TA_HistoryFree( history );
               if( retCode != TA_SUCCESS )
               {
                  if( refHistory )
                     TA_HistoryFree( refHistory );
                  TA_SymbolTableFree( symTable );
                  TA_CategoryTableFree( catTable );
                  if( udbEqv )
                     TA_UDBaseFree( udbEqv );
                  freeLib( udb );
                  return TA_TESTASCII_HISTORYFREE_FAILED;
               }

               /* Test #1 check ranging with start and end parameter. */
               if( strcmp( catTable->string[i], "001" ) == 0 )
               {
                  /* Test start-end */
                  TA_SetDefault(&ts1);
                  TA_SetDefault(&ts2);
                  TA_SetDate( 4567,1,23,&ts1);
                  TA_SetDate( 4568,12,31,&ts2);
                  TA_SetTime( 0, 0, 0, &ts2 );

                  memset( &histParam, 0, sizeof( TA_HistoryAllocParam ) );
                  histParam.category = catTable->string[i];
                  histParam.symbol   = &buffer200[0];
                  histParam.period   = TA_DAILY;
                  histParam.field    = TA_ALL;
                  TA_TimestampCopy( &histParam.start, &ts1 );
                  TA_TimestampCopy( &histParam.end, &ts2 );
                  retCode = TA_HistoryAlloc( udbEqv, &histParam, &history );
   
                  if( retCode != TA_SUCCESS )
                  {
                     printf( "TA_HistoryAlloc failed with %d (%s)\n", retCode, symTable->string[j] );
                     if( refHistory )
                        TA_HistoryFree( refHistory );
                     TA_SymbolTableFree( symTable );
                     TA_CategoryTableFree( catTable );
                     if( udbEqv )
                        TA_UDBaseFree( udbEqv );
                     freeLib( udb );
                     return TA_TESTASCII_EQV_HISTORYALLOC;
                  }

                  if( history->nbBars != 3 )
                  {
                     printf( "Less than 3 price bar read (%d)\n", history->nbBars );
                     TA_HistoryFree( history );
                     if( refHistory )
                        TA_HistoryFree( refHistory );
                     TA_SymbolTableFree( symTable );
                     TA_CategoryTableFree( catTable );
                     if( udbEqv )
                        TA_UDBaseFree( udbEqv );
                     freeLib( udb );
                     return TA_TESTASCII_EQV_HISTORYALLOC;
                  }

                  retCode = TA_HistoryFree( history );
                  if( retCode != TA_SUCCESS )
                  {
                     if( refHistory )
                        TA_HistoryFree( refHistory );
                     TA_SymbolTableFree( symTable );
                     TA_CategoryTableFree( catTable );
                     if( udbEqv )
                        TA_UDBaseFree( udbEqv );
                     freeLib( udb );
                     return TA_TESTASCII_HISTORYFREE_FAILED;
                  }
               }
            }
         }

         if( refHistory )
         {
            retCode = TA_HistoryFree( refHistory );
            if( retCode != TA_SUCCESS )
            {
               TA_SymbolTableFree( symTable );
               TA_CategoryTableFree( catTable );
               if( udbEqv )
                  TA_UDBaseFree( udbEqv );
               freeLib( udb );
               return TA_TESTASCII_REFHISTORYFREE_FAILED;
            }
         }

         retCode = TA_SymbolTableFree( symTable );
         if( retCode != TA_SUCCESS )
         {
            TA_CategoryTableFree( catTable );
            if( udbEqv )
               TA_UDBaseFree( udbEqv );
            freeLib( udb );
            return TA_TESTASCII_SYMTABLE_FREE_ERROR;
         }

         if( udbEqv )
         {
            retCode = TA_UDBaseFree( udbEqv );
            if( retCode != TA_SUCCESS )
            {
               printf( "TA_UDBaseFree failed with %d\n", retCode );
               TA_CategoryTableFree( catTable );
               freeLib( udb );
               return TA_TESTASCII_UDBASEFREE_FAILED;
            }
         }
      }

      retCode = TA_CategoryTableFree( catTable );
      if( retCode != TA_SUCCESS )
      {
         printf( "TA_CategoryTableFree failed with %d\n", retCode );
         freeLib( udb );
         return TA_TESTASCII_CATTABLE_FREE_ERROR;         
      }
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}
