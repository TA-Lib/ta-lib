/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *  AC       Angelo Ciceri
 *  AB       Anatoliy Belsky
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  063001 MF   First version (initial framework only).
 *  090404 MF   Add test_candlestick
 *  110206 AC   Change volume and open interest to double
 *  122506 MF   Add MININDEX,MAXINDEX,MINMAX and MINMAXINDEX.
 *  101812 AB   Add AVGDEV.
 *  101912 AB   Add IMI.
 */

/* Description:
 *    Perform regression testing of the TA-LIB.
 */

/**** Headers ****/
#ifdef WIN32
   #include "windows.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "test_codegen.h"
#include "codegen_pipe.h"
#include "server_verify.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
int nbProfiledCall;
double timeInProfiledCall;
double worstProfiledCall;
int insufficientClockPrecision;
int doExtensiveProfiling;

/* CSV list of function names to test (NULL = test all) */
static const char *functionFilter = NULL;
static int doCodegenTest = 0;
static int codegenOnly = 0;
static int doFuzz064 = 0;
static const char *codegenLanguageFilter = NULL;

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber testTAFunction_ALL( void );
static ErrorNumber test_with_simulator( void );
static void printUsage(void);
static ErrorNumber test_codegen_with_simulator( void );
static int svLanguageEnabled( const char *filter, const char *name );

/**** Local variables definitions.     ****/
/* None */

/* Exact-token match against the --language=CSV filter (NULL = all).
 * Same semantics as language_matches_filter in test_codegen.c.
 */
static int svLanguageEnabled( const char *filter, const char *name )
{
   char filterCopy[1024];
   char *token;
   if( filter == NULL ) return 1;
   strncpy(filterCopy, filter, sizeof(filterCopy) - 1);
   filterCopy[sizeof(filterCopy) - 1] = '\0';
   token = strtok(filterCopy, ",");
   while( token != NULL )
   {
      if( strcmp(name, token) == 0 ) return 1;
      token = strtok(NULL, ",");
   }
   return 0;
}

/**** Global functions definitions.   ****/
int main( int argc, char **argv )
{
#ifdef WIN32
	LARGE_INTEGER QPFrequency;
#endif
   double freq;

   ErrorNumber retValue;

   insufficientClockPrecision = 0;
   timeInProfiledCall = 0.0;
   worstProfiledCall = 0.0;
   nbProfiledCall = 0;
   doExtensiveProfiling = 0;

   printf( "\n" );
   printf( "ta_regtest V%s - Regression Tests of TA-Lib code\n", TA_GetVersionString() );
   printf( "\n" );

   {
      int i;
      for( i = 1; i < argc; i++ )
      {
         if( (argv[i][0] == '-') && (argv[i][1] == 'p') && (argv[i][2] == '\0') )
         {
            doExtensiveProfiling = 1;
         }
         else if( strncmp(argv[i], "--function=", 11) == 0 )
         {
            functionFilter = argv[i] + 11;
         }
         else if( strcmp(argv[i], "--codegen") == 0 )
         {
            doCodegenTest = 1;
         }
         else if( strncmp(argv[i], "--codegen=", 10) == 0 )
         {
            doCodegenTest = 1;
            codegenLanguageFilter = argv[i] + 10;
         }
         else if( strcmp(argv[i], "--codegen-only") == 0 )
         {
            doCodegenTest = 1;
            codegenOnly = 1;
         }
         else if( strncmp(argv[i], "--language=", 11) == 0 )
         {
            codegenLanguageFilter = argv[i] + 11;
         }
         else if( strcmp(argv[i], "--fuzz-064") == 0 )
         {
            doFuzz064 = 1;
         }
         else if( strcmp(argv[i], "--no-guarded") == 0 )
         {
            extern int g_hideGuarded;
            g_hideGuarded = 1;
         }
         else if( strcmp(argv[i], "--no-unguarded") == 0 )
         {
            extern int g_hideUnguarded;
            g_hideUnguarded = 1;
         }
         else
         {
            printUsage();
            return TA_REGTEST_BAD_USER_PARAM;
         }
      }
   }

   /* Some tests are using randomness. */
   srand( (unsigned)time( NULL ) );

   /* Opt-in bit-exact differential fuzz vs released v0.6.4 (ta_064_serve).
    * Self-contained: init the lib, run the fuzz, done — skips the rest. */
   if( doFuzz064 )
   {
      ErrorNumber fuzzRet;
      if( TA_Initialize() != TA_SUCCESS )
      {
         printf( "TA_Initialize failed\n" );
         return TA_TESTUTIL_INIT_FAILED;
      }
      TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );
      fuzzRet = fuzz_ref064( functionFilter );
      TA_Shutdown();
      return fuzzRet;
   }

   /* Test utility like List/Stack/Dictionary/Memory Allocation etc... */
   retValue = test_internals();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed an internal test with code=%d\n", retValue );
      return retValue;
   }

   /* Test abstract interface.
    * When codegen mode is active, also verify each call against the server.
    */
   {
      CodegenPipe abstractPipe;
      int abstractPipeOpen = 0;
      /* Only spawn the C server for abstract verification when C is actually in
       * the language filter. Under --language=rust the C server is not built
       * (regtest.py compiles servers with --backend=rust), so spawning it would
       * execvp a missing binary and surface as a confusing first-call pipe EOF.
       * The Rust abstract parity below has the matching guard for "rust". */
      if( doCodegenTest &&
          ( codegenLanguageFilter == NULL || strstr(codegenLanguageFilter, "c") != NULL ) )
      {
         /* Build server path relative to the ta_regtest executable,
          * so it works regardless of the current working directory. */
         char serverPath[1024];
         {
            const char *self = argv[0];
            const char *lastSlash = strrchr(self, '/');
            if( lastSlash ) {
               int dirLen = (int)(lastSlash - self + 1);
               snprintf(serverPath, sizeof(serverPath), "%.*sta_codegen_serve_c",
                        dirLen, self);
            } else {
               snprintf(serverPath, sizeof(serverPath), "./ta_codegen_serve_c");
            }
         }
         const char *const serverArgv[] = {serverPath, NULL};
         if( codegen_pipe_open(&abstractPipe, serverArgv) == TA_TEST_PASS )
         {
            test_abstract_set_server(&abstractPipe);
            abstractPipeOpen = 1;
            printf( "  (with server verification)\n" );
         }
         else
         {
            printf( "  (server not available, C only)\n" );
         }
      }
      retValue = test_abstract();
      test_abstract_set_server(NULL);
      if( abstractPipeOpen )
         codegen_pipe_close(&abstractPipe);
   }

   /* Cross-language abstract parity (dev / merge-loop gate via --codegen):
    * run the full ta_abstract path against the Rust server, comparing to the C
    * reference. Two complementary passes:
    *   1. test_abstract_server_metadata — the metadata RPCs (TA_GetFuncInfo + the
    *      param-info getters), which the dynamic path below does NOT exercise.
    *   2. test_abstract — the dynamic-dispatch path (abstract_call /
    *      abstract_get_lookback / TA_FunctionDescriptionXML), comparing output
    *      VALUES (not just metadata) for every function.
    * NOTE: the autotools dist nightly runs ta_regtest with no args, so this block
    * only runs under --codegen/--codegen-only (build.py regtest / regtest.py),
    * not the dist verification path. */
   if( retValue == TA_TEST_PASS && doCodegenTest &&
       ( codegenLanguageFilter == NULL || strstr(codegenLanguageFilter, "rust") != NULL ) )
   {
      CodegenPipe rustAbstractPipe;
      char rustServerPath[1024];
      const char *self = argv[0];
      const char *lastSlash = strrchr(self, '/');
      if( lastSlash ) {
         int dirLen = (int)(lastSlash - self + 1);
         snprintf(rustServerPath, sizeof(rustServerPath), "%.*sta_codegen_serve_rust",
                  dirLen, self);
      } else {
         snprintf(rustServerPath, sizeof(rustServerPath), "./ta_codegen_serve_rust");
      }
      {
         const char *const rustArgv[] = {rustServerPath, NULL};
         if( codegen_pipe_open(&rustAbstractPipe, rustArgv) == TA_TEST_PASS )
         {
            ErrorNumber e;
            printf( "Testing Abstract metadata parity (Rust server vs C)\n" );
            test_abstract_set_server(&rustAbstractPipe);
            e = test_abstract_server_metadata(functionFilter);
            /* Full dynamic-dispatch path (abstract_call / abstract_get_lookback /
             * TA_FunctionDescriptionXML) against the Rust server, comparing output
             * VALUES to the C for every function. */
            if( e == TA_TEST_PASS )
            {
               printf( "Testing Abstract dynamic dispatch (Rust server vs C)\n" );
               e = test_abstract();
            }
            test_abstract_set_server(NULL);
            codegen_pipe_close(&rustAbstractPipe);
            if( retValue == TA_TEST_PASS && e != TA_TEST_PASS )
               retValue = e;
         }
         else
         {
            printf( "  (Rust server not available, skipping Rust abstract parity)\n" );
         }
      }
   }

   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: Abstract interface Tests (error number = %d)\n", retValue );
      return retValue;
   }

   /* Perform all regresstions tests (except when ta_regtest is executed for profiling only). */
   if( !doExtensiveProfiling )
   {
      if( !codegenOnly )
      {
         /* When codegen mode is active, also verify hand-written tests
          * against every available language server (C, Rust, Java, .NET),
          * honoring --language=CSV. The Java/.NET launch commands are
          * relative to the bin directory (same as test_codegen.c).
          */
         CodegenPipe svPipes[SV_MAX_PIPES];
         int nbSvPipes = 0;
         if( doCodegenTest )
         {
            char svPathC[1024];
            char svPathRust[1024];
            const char *self = argv[0];
            const char *lastSlash = strrchr(self, '/');
            if( lastSlash ) {
               int dirLen = (int)(lastSlash - self + 1);
               snprintf(svPathC, sizeof(svPathC), "%.*sta_codegen_serve_c",
                        dirLen, self);
               snprintf(svPathRust, sizeof(svPathRust), "%.*sta_codegen_serve_rust",
                        dirLen, self);
            } else {
               snprintf(svPathC, sizeof(svPathC), "./ta_codegen_serve_c");
               snprintf(svPathRust, sizeof(svPathRust), "./ta_codegen_serve_rust");
            }
            const char *const svArgvC[]      = {svPathC, NULL};
            const char *const svArgvRust[]   = {svPathRust, NULL};
            const char *const svArgvJava[]   = {"java", "-cp", "ta_codegen_java",
                                                "TaCodegenServe", NULL};
            const char *const svArgvDotnet[] = {"dotnet",
                                                "ta_codegen_dotnet/TaCodegenServe.dll", NULL};
            const struct { const char *lang; const char *const *argvSv; } svServers[] = {
               { "c",      svArgvC },
               { "rust",   svArgvRust },
               { "java",   svArgvJava },
               { "dotnet", svArgvDotnet },
            };
            unsigned int svIdx;
            for( svIdx = 0; svIdx < sizeof(svServers)/sizeof(svServers[0]); svIdx++ )
            {
               if( !svLanguageEnabled(codegenLanguageFilter, svServers[svIdx].lang) )
                  continue;
               if( codegen_pipe_open(&svPipes[nbSvPipes], svServers[svIdx].argvSv) == TA_TEST_PASS )
                  nbSvPipes++;
               else
                  printf( "  (%s server not available for hand-written test verification)\n",
                          svServers[svIdx].lang );
            }
            if( nbSvPipes > 0 )
            {
               CodegenPipe *pipes[SV_MAX_PIPES];
               int p;
               for( p = 0; p < nbSvPipes; p++ )
                  pipes[p] = &svPipes[p];
               server_verify_init(pipes, nbSvPipes);
               printf( "  (hand-written tests verified against %d language server(s))\n",
                       nbSvPipes );
            }
         }

         retValue = test_with_simulator();

         if( nbSvPipes > 0 )
         {
            int p;
            server_verify_shutdown();
            for( p = 0; p < nbSvPipes; p++ )
               codegen_pipe_close(&svPipes[p]);
         }

         if( retValue != TA_TEST_PASS )
            return retValue;
      }

      if( doCodegenTest )
      {
         retValue = test_codegen_with_simulator();
         if( retValue != TA_TEST_PASS )
            return retValue;
      }

      if( insufficientClockPrecision != 0 )
      {
   	   printf( "\nWarning: Code profiling not supported for this platform.\n" );
      }
      else if( nbProfiledCall > 0 )
      {
         printf( "\nNumber profiled function call       = %d function calls", nbProfiledCall );

#ifdef WIN32
         QueryPerformanceFrequency(&QPFrequency);
         freq = (double)QPFrequency.QuadPart;
         printf( "\nTotal execution time                = %g milliseconds", (timeInProfiledCall/freq)*1000.0 );
         printf( "\nWorst single function call          = %g milliseconds", (worstProfiledCall/freq)*1000.0 );
         printf( "\nAverage execution time per function = %g microseconds\n", ((timeInProfiledCall/freq)*1000000.0)/((double)nbProfiledCall) );
#else
         freq = (double)CLOCKS_PER_SEC;
         printf( "\nTotal execution time                = %g milliseconds", timeInProfiledCall/freq/1000.0 );
         printf( "\nWorst single function call          = %g milliseconds", worstProfiledCall/freq/1000.0 );
         printf( "\nAverage execution time per function = %g microseconds\n", (timeInProfiledCall/freq/1000000.0)/((double)nbProfiledCall) );
#endif
      }
      printf( "\n* All tests succeeded. Enjoy the library. *\n" );
   }


   return TA_TEST_PASS; /* Everything succeed !!! */
}

/**** Local functions definitions.     ****/
static ErrorNumber test_with_simulator( void )
{
   ErrorNumber retValue;

   /* Initialize the library. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Perform testing of each of the TA Functions. */
   retValue = testTAFunction_ALL();
   if( retValue != TA_TEST_PASS )
   {
      return retValue;
   }

   /* Clean-up and exit. */

   retValue = freeLib( );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS; /* All test succeed. */
}

extern TA_Real      TA_SREF_open_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_high_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_low_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_close_daily_ref_0_PRIV[];
extern TA_Real      TA_SREF_volume_daily_ref_0_PRIV[];

static ErrorNumber test_codegen_with_simulator( void )
{
   ErrorNumber retValue;
   TA_History history;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   history.nbBars = 252;
   history.open   = TA_SREF_open_daily_ref_0_PRIV;
   history.high   = TA_SREF_high_daily_ref_0_PRIV;
   history.low    = TA_SREF_low_daily_ref_0_PRIV;
   history.close  = TA_SREF_close_daily_ref_0_PRIV;
   history.volume = TA_SREF_volume_daily_ref_0_PRIV;

   retValue = test_codegen(&history, codegenLanguageFilter, functionFilter);
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/* Check if any CSV token in 'filter' appears as a substring in 'tags'.
 * Returns 1 if match found (or filter is NULL), 0 otherwise.
 */
static int matchesFilter(const char *filter, const char *tags)
{
   char filterCopy[1024];
   char *token;

   if( filter == NULL )
      return 1; /* No filter = run everything */

   strncpy(filterCopy, filter, sizeof(filterCopy) - 1);
   filterCopy[sizeof(filterCopy) - 1] = '\0';

   token = strtok(filterCopy, ",");
   while( token != NULL )
   {
      if( strstr(tags, token) != NULL )
         return 1;
      token = strtok(NULL, ",");
   }
   return 0;
}

static ErrorNumber testTAFunction_ALL( void )
{
   ErrorNumber retValue;
   TA_History history;

   history.nbBars = 252;
   history.open   = TA_SREF_open_daily_ref_0_PRIV;
   history.high   = TA_SREF_high_daily_ref_0_PRIV;
   history.low    = TA_SREF_low_daily_ref_0_PRIV;
   history.close  = TA_SREF_close_daily_ref_0_PRIV;
   history.volume = TA_SREF_volume_daily_ref_0_PRIV;

   printf( "Testing the TA functions\n" );

   initGlobalBuffer();

   /* Make tests for each TA functions. */
   #define DO_TEST(func,str) \
      { \
      if( matchesFilter(functionFilter, str) ) \
      { \
         printf( "%50s: Testing....", str ); \
         fflush(stdout); \
         showFeedback(); \
         TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT ); \
         retValue = func( &history ); \
         if( retValue != TA_TEST_PASS ) \
            return retValue; \
         hideFeedback(); \
         printf( "done.\n" ); \
         fflush(stdout); \
      } \
      }
   DO_TEST( test_func_1in_1out, "MATH,VECTOR,DCPERIOD/PHASE,TRENDLINE/MODE" );
   DO_TEST( test_func_ma,       "All Moving Averages" );
   DO_TEST( test_func_per_hl,   "AROON,CORREL,BETA,MIDPRICE" );
   DO_TEST( test_func_per_hlc,  "CCI,WILLR,ULTOSC,NATR" );
   DO_TEST( test_func_per_ohlc, "BOP,AVGPRICE" );
   DO_TEST( test_func_rsi,      "RSI,CMO" );
   DO_TEST( test_func_imi, "IMI" );
   DO_TEST( test_func_minmax,   "MIN,MAX,MININDEX,MAXINDEX,MINMAX,MINMAXINDEX,MIDPOINT" );
   DO_TEST( test_func_po,       "PO,APO" );
   DO_TEST( test_func_adx,      "ADX,ADXR,DI,DM,DX" );
   DO_TEST( test_func_sar,      "SAR,SAREXT" );
   DO_TEST( test_func_stoch,    "STOCH,STOCHF,STOCHRSI" );
   DO_TEST( test_func_per_hlcv, "MFI,AD,ADOSC" );
   DO_TEST( test_func_1in_2out, "PHASOR,SINE" );
   DO_TEST( test_func_per_ema,  "TRIX" );
   DO_TEST( test_func_macd,     "MACD,MACDFIX,MACDEXT" );
   DO_TEST( test_func_mom_roc,  "MOM,ROC,ROCP,ROCR,ROCR100" );
   DO_TEST( test_func_trange,   "TRANGE,ATR" );
   DO_TEST( test_func_stddev,   "STDDEV,VAR" );
   DO_TEST( test_func_avgdev,   "AVGDEV" );
   DO_TEST( test_func_bbands,   "BBANDS" );
   DO_TEST( test_func_period_boundary, "PERIOD1/BOUNDARY" );
   DO_TEST( test_candlestick,   "All Candlesticks" );

   return TA_TEST_PASS; /* All tests succeeded. */
}

static void printUsage(void)
{
      printf( "Usage: ta_regtest [-p] [--function=NAME[,NAME,...]]\n" );
      printf( "\n" );
      printf( "   No parameter needed for regression testing.\n" );
      printf( "\n" );
      printf( "   This tool will execute a series of tests to\n" );
      printf( "   make sure that the library is behaving as\n" );
      printf( "   expected.\n");
      printf( "\n" );
      printf( "   ** Must be run from the 'bin' directory.\n" );
      printf( "\n" );
      printf( "   OPTIONS:\n" );
      printf( "    -p Only generate profiling data on stdout. This is\n" );
      printf( "       intended only for the TA-Lib developers. It is\n" );
      printf( "       not further documented for general use.\n" );
      printf( "\n" );
      printf( "    --function=NAME[,NAME,...]\n" );
      printf( "       Only run test groups whose tags contain at least\n" );
      printf( "       one of the given names (substring match).\n" );
      printf( "       Example: --function=RSI,BBANDS\n" );
      printf( "\n" );
      printf( "    --codegen[=LANG[,LANG,...]]\n" );
      printf( "       After normal tests, verify ta_codegen output against C reference.\n" );
      printf( "       Languages: rust, c, java, dotnet (default: all)\n" );
      printf( "       Example: --codegen=rust,java\n" );
      printf( "\n" );
      printf( "    --codegen-only\n" );
      printf( "       Run ONLY codegen verification; skip the normal C test suite.\n" );
      printf( "       Combine with --language and --function to narrow the run.\n" );
      printf( "       Example: --codegen-only --language=rust --function=SMA\n" );
      printf( "\n" );
      printf( "    --language=LANG[,LANG,...]\n" );
      printf( "       Filter which language servers to test with --codegen / --codegen-only.\n" );
      printf( "       Valid values: rust, c, java, dotnet (default: all)\n" );
      printf( "       Example: --language=c,rust\n" );
      printf( "\n" );
      printf( "       Requires language server binaries in the bin directory.\n" );
      printf( "       Build with: ta_codegen build\n" );
      printf( "\n" );
      printf( "   On success, the exit code is 0.\n" );
      printf( "   On failure, the exit code is a number that can be\n" );
      printf( "   found in c/src/tools/ta_regtest/ta_error_number.h\n" );
}
