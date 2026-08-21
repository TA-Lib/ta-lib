/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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
static int doFuzz064 = 0;
static int doXlangHash = 0;
static const char *codegenLanguageFilter = NULL;
static unsigned int randSeed = 0;   /* 0 = pick one from the clock */

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
         else if( strncmp(argv[i], "--language=", 11) == 0 )
         {
            codegenLanguageFilter = argv[i] + 11;
         }
         else if( strcmp(argv[i], "--fuzz-064") == 0 )
         {
            doFuzz064 = 1;
         }
         else if( strcmp(argv[i], "--xlang-hash") == 0 )
         {
            doXlangHash = 1;
         }
         else if( strncmp(argv[i], "--seed=", 7) == 0 )
         {
            randSeed = (unsigned int)strtoul( argv[i] + 7, NULL, 10 );
         }
         else if( strcmp(argv[i], "--no-guarded") == 0 )
         {
            extern int g_hideGuarded;
            g_hideGuarded = 1;
         }
         else
         {
            printUsage();
            return TA_REGTEST_BAD_USER_PARAM;
         }
      }
   }

   /* Some tests are using randomness: the range sweeps pick their startIdx and
    * sizes with rand(), so each run covers a different subset. Printed so a
    * failure can be replayed with --seed=N. */
   if( randSeed == 0 )
      randSeed = (unsigned int)time( NULL );
   printf( "Random seed: %u  (replay with --seed=%u)\n", randSeed, randSeed );
   srand( randSeed );

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

   /* Opt-in cross-language BITWISE parity gate (issue #113). Self-contained:
    * init the lib (the in-process C golden), run the gate, done. Must run from
    * the bin/ directory so the language servers launch (like --fuzz-064). */
   if( doXlangHash )
   {
      ErrorNumber xlangRet;
      if( TA_Initialize() != TA_SUCCESS )
      {
         printf( "TA_Initialize failed\n" );
         return TA_TESTUTIL_INIT_FAILED;
      }
      TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );
      xlangRet = xlang_hash( functionFilter, codegenLanguageFilter );
      TA_Shutdown();
      return xlangRet;
   }

   /* Test utility like List/Stack/Dictionary/Memory Allocation etc... */
   retValue = test_internals();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed an internal test with code=%d\n", retValue );
      return retValue;
   }

   /* A server that will not start is only a legitimate skip when the caller
    * did not ask for that language. `--language=csharp` on a box without the
    * .NET SDK must fail loudly: "green because nothing ran" is the failure
    * mode every gate in this file exists to prevent.
    *
    * CODEGEN_PIPE_SUPPORTED excludes Windows, where codegen_pipe_open is an
    * unconditional-failure stub: there EVERY server is unavailable by design,
    * the C reference tests are the whole contract, and hard-failing would break
    * `--codegen --language=<anything>` for every Windows developer. */
#define ABSTRACT_SERVER_MISSING(lang, filter, rv)                                  \
   do {                                                                            \
      if( (filter) != NULL && CODEGEN_PIPE_SUPPORTED ) {                           \
         printf( "  ABSTRACT ERROR: --language named %s but its server could not " \
                 "be started\n", (lang) );                                         \
         if( (rv) == TA_TEST_PASS ) (rv) = TA_ABSTRACT_SERVER_ERROR;               \
      } else {                                                                     \
         printf( "  (%s server not available, skipping %s abstract parity)\n",     \
                 (lang), (lang) );                                                 \
      }                                                                            \
   } while(0)

   /* Test abstract interface.
    * When codegen mode is active, also verify each call against the server.
    */
   {
      CodegenPipe abstractPipe;
      int abstractPipeOpen = 0;
      ErrorNumber cMetaErr = TA_TEST_PASS;
      /* Only spawn the C server for abstract verification when C is actually in
       * the language filter. Under --language=rust the C server is not built
       * (regtest.py compiles servers with --backend=rust), so spawning it would
       * execvp a missing binary and surface as a confusing first-call pipe EOF.
       * The Rust abstract parity below has the matching guard for "rust". */
      if( doCodegenTest && svLanguageEnabled( codegenLanguageFilter, "c" ) )
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
            test_abstract_set_server(&abstractPipe, "c");
            abstractPipeOpen = 1;
            printf( "  (with server verification)\n" );
            /* The control arm. C's server answers the metadata RPCs from the
             * very ta_abstract this compares against, so a failure here is a
             * defect in the comparator, not in a backend — which is what makes
             * a failure on Rust/Java/C# meaningful. It is also the only caller
             * that reaches C's own abstract_for_each_func handler. */
            cMetaErr = test_abstract_server_metadata(functionFilter);
         }
         else
         {
            printf( "  (server not available, C only)\n" );
         }
      }
      retValue = test_abstract();
      if( retValue == TA_TEST_PASS )
         retValue = cMetaErr;
      test_abstract_set_server(NULL, NULL);
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
    * only runs under --codegen (build.py regtest / regtest.py),
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
            test_abstract_set_server(&rustAbstractPipe, "rust");
            e = test_abstract_server_metadata(functionFilter);
            /* Full dynamic-dispatch path (abstract_call / abstract_get_lookback /
             * TA_FunctionDescriptionXML) against the Rust server, comparing output
             * VALUES to the C for every function. */
            if( e == TA_TEST_PASS )
            {
               printf( "Testing Abstract dynamic dispatch (Rust server vs C)\n" );
               e = test_abstract();
            }
            test_abstract_set_server(NULL, NULL);
            codegen_pipe_close(&rustAbstractPipe);
            if( retValue == TA_TEST_PASS && e != TA_TEST_PASS )
               retValue = e;
         }
         else
         {
            ABSTRACT_SERVER_MISSING("rust", codegenLanguageFilter, retValue);
         }
      }
   }

   /* Java abstract parity (issue #114): metadata RPCs (TA_GetFuncInfo + the
    * param-info getters) AND the dynamic-dispatch path (abstract_call /
    * abstract_get_lookback / TA_FunctionDescriptionXML), comparing output VALUES
    * to the C reference for every function. Java server spawns via
    * `java -cp ta_codegen_java TaCodegenServe`. */
   if( retValue == TA_TEST_PASS && doCodegenTest &&
       ( codegenLanguageFilter == NULL || strstr(codegenLanguageFilter, "java") != NULL ) )
   {
      CodegenPipe javaAbstractPipe;
      const char *const javaArgv[] = {"java", "-cp", "ta_codegen_java", "TaCodegenServe", NULL};
      if( codegen_pipe_open(&javaAbstractPipe, javaArgv) == TA_TEST_PASS )
      {
         ErrorNumber e;
         printf( "Testing Abstract metadata parity (Java server vs C)\n" );
         test_abstract_set_server(&javaAbstractPipe, "java");
         e = test_abstract_server_metadata(functionFilter);
         if( e == TA_TEST_PASS )
         {
            printf( "Testing Abstract dynamic dispatch (Java server vs C)\n" );
            e = test_abstract();
         }
         test_abstract_set_server(NULL, NULL);
         codegen_pipe_close(&javaAbstractPipe);
         if( retValue == TA_TEST_PASS && e != TA_TEST_PASS )
            retValue = e;
      }
      else
      {
         ABSTRACT_SERVER_MISSING("java", codegenLanguageFilter, retValue);
      }
   }

   /* C# abstract parity: the same two passes as Java and Rust — the metadata
    * RPCs (TA_GetFuncInfo + the param-info getters) and the dynamic-dispatch
    * path (abstract_call / abstract_get_lookback / TA_FunctionDescriptionXML),
    * comparing output VALUES to the C reference for every function.
    *
    * Without this block the C# server's abstract RPCs would have exactly zero
    * coverage and every gate would stay green: test_abstract_server_metadata()
    * and the server legs of test_abstract() both short-circuit to TA_TEST_PASS
    * when no pipe is set. The managed server spawns via
    * `dotnet ta_codegen_csharp/TaCodegenServe.dll`. */
   if( retValue == TA_TEST_PASS && doCodegenTest &&
       ( codegenLanguageFilter == NULL || strstr(codegenLanguageFilter, "csharp") != NULL ) )
   {
      CodegenPipe csharpAbstractPipe;
      const char *const csharpArgv[] = {"dotnet", "ta_codegen_csharp/TaCodegenServe.dll", NULL};
      if( codegen_pipe_open(&csharpAbstractPipe, csharpArgv) == TA_TEST_PASS )
      {
         ErrorNumber e;
         printf( "Testing Abstract metadata parity (C# server vs C)\n" );
         test_abstract_set_server(&csharpAbstractPipe, "csharp");
         e = test_abstract_server_metadata(functionFilter);
         if( e == TA_TEST_PASS )
         {
            printf( "Testing Abstract dynamic dispatch (C# server vs C)\n" );
            e = test_abstract();
         }
         test_abstract_set_server(NULL, NULL);
         codegen_pipe_close(&csharpAbstractPipe);
         if( retValue == TA_TEST_PASS && e != TA_TEST_PASS )
            retValue = e;
      }
      else
      {
         ABSTRACT_SERVER_MISSING("csharp", codegenLanguageFilter, retValue);
      }
   }
#undef ABSTRACT_SERVER_MISSING

   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: Abstract interface Tests (error number = %d)\n", retValue );
      return retValue;
   }

   /* Perform all regresstions tests (except when ta_regtest is executed for profiling only). */
   if( !doExtensiveProfiling )
   {
      {
         /* When codegen mode is active, also verify hand-written tests
          * against every available language server (C, Rust, Java, C#),
          * honoring --language=CSV. The Java/C# launch commands are
          * relative to the bin directory (same as test_codegen.c).
          */
         CodegenPipe svPipes[SV_MAX_PIPES];
         const char *svPipeLang[SV_MAX_PIPES] = {0};
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
            const char *const svArgvCsharp[] = {"dotnet",
                                                "ta_codegen_csharp/TaCodegenServe.dll", NULL};
            const struct { const char *lang; const char *const *argvSv; } svServers[] = {
               { "c",      svArgvC },
               { "rust",   svArgvRust },
               { "java",   svArgvJava },
               { "csharp", svArgvCsharp },
            };
            unsigned int svIdx;
            for( svIdx = 0; svIdx < sizeof(svServers)/sizeof(svServers[0]); svIdx++ )
            {
               if( !svLanguageEnabled(codegenLanguageFilter, svServers[svIdx].lang) )
                  continue;
               if( codegen_pipe_open(&svPipes[nbSvPipes], svServers[svIdx].argvSv) == TA_TEST_PASS )
               {
                  svPipeLang[nbSvPipes] = svServers[svIdx].lang;
                  nbSvPipes++;
               }
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
               server_verify_init(pipes, svPipeLang, nbSvPipes);
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
   history.openInterest = NULL;   /* the test series carries no OI; honor the
                                     "unused arrays are NULL" contract so readers
                                     (test_variants.c build_regime) can trust it */

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
/* Does any '*'-suffixed element of `tags` prefix `token`?
 *
 * A tag element ending in '*' is a PREFIX CLAIM: the group covers every
 * function whose name starts with it. The plain substring rule asks whether the
 * TAG contains the user's token, so reaching a family of functions otherwise
 * means spelling every member into the tag -- which is what issue #137 did for
 * the composite group, and it goes stale the moment a new member lands. It went
 * stale here: all 61 CDL* names were unreachable, so `--function=CDLDOJI` ran
 * the candlestick group zero times and still printed "All tests succeeded".
 */
static int tagPrefixMatches(const char *tags, const char *token)
{
   const char *p = tags;

   while( *p )
   {
      const char *star = strchr(p, '*');
      const char *start;
      size_t len;

      if( star == NULL )
         return 0;
      start = star;
      while( start > tags && start[-1] != ',' )
         start--;
      len = (size_t)(star - start);
      if( len > 0 && strncmp(token, start, len) == 0 )
         return 1;
      p = star + 1;
   }
   return 0;
}

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
      if( tagPrefixMatches(tags, token) )
         return 1;
      token = strtok(NULL, ",");
   }
   return 0;
}

static ErrorNumber testTAFunction_ALL( void )
{
   ErrorNumber retValue;
   TA_History history;
   int nbGroupsRun = 0;

   history.nbBars = 252;
   history.open   = TA_SREF_open_daily_ref_0_PRIV;
   history.high   = TA_SREF_high_daily_ref_0_PRIV;
   history.low    = TA_SREF_low_daily_ref_0_PRIV;
   history.close  = TA_SREF_close_daily_ref_0_PRIV;
   history.volume = TA_SREF_volume_daily_ref_0_PRIV;
   history.openInterest = NULL;   /* the test series carries no OI; honor the
                                     "unused arrays are NULL" contract so readers
                                     (test_variants.c build_regime) can trust it */

   printf( "Testing the TA functions\n" );

   initGlobalBuffer();

   /* Make tests for each TA functions. */
   #define DO_TEST(func,str) \
      { \
      if( matchesFilter(functionFilter, str) ) \
      { \
         nbGroupsRun++; \
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
   DO_TEST( test_func_1in_1out, "MATH,VECTOR,DCPERIOD/PHASE,TRENDLINE/MODE,"
                                "HT_DCPERIOD,HT_DCPHASE,HT_TRENDLINE,HT_TRENDMODE,MEDPRICE" );
   DO_TEST( test_func_ma,       "All Moving Averages,"
                                "SMA,EMA,WMA,DEMA,TEMA,TRIMA,KAMA,MAMA,T3,MA" );
   DO_TEST( test_func_per_hl,   "AROON,AROONOSC,CORREL,BETA,MIDPRICE" );
   DO_TEST( test_func_per_hlc,  "CCI,WILLR,ULTOSC,NATR,ACCBANDS,WAD" );
   DO_TEST( test_func_per_ohlc, "BOP,AVGPRICE" );
   DO_TEST( test_func_rsi,      "RSI,CMO" );
   DO_TEST( test_func_imi, "IMI" );
   DO_TEST( test_func_minmax,   "MIN,MAX,MININDEX,MAXINDEX,MINMAX,MINMAXINDEX,MIDPOINT" );
   DO_TEST( test_func_po,       "PO,APO,PPO" );
   DO_TEST( test_func_adx,      "ADX,ADXR,DI,DM,DX,PLUS_DI,PLUS_DM,MINUS_DI,MINUS_DM" );
   DO_TEST( test_func_sar,      "SAR,SAREXT" );
   DO_TEST( test_func_stoch,    "STOCH,STOCHF,STOCHRSI" );
   DO_TEST( test_func_per_hlcv, "MFI,AD,ADOSC" );
   DO_TEST( test_func_per_cv,   "NVI,PVI" );
   DO_TEST( test_func_1in_2out, "PHASOR,SINE,HT_PHASOR,HT_SINE" );
   DO_TEST( test_func_per_ema,  "TRIX" );
   DO_TEST( test_func_macd,     "MACD,MACDFIX,MACDEXT" );
   DO_TEST( test_func_mom_roc,  "MOM,ROC,ROCP,ROCR,ROCR100" );
   DO_TEST( test_func_trange,   "TRANGE,ATR" );
   DO_TEST( test_func_stddev,   "STDDEV,VAR" );
   DO_TEST( test_func_avgdev,   "AVGDEV" );
   DO_TEST( test_func_bbands,   "BBANDS" );
   DO_TEST( test_func_period_boundary, "PERIOD1/BOUNDARY" );
   DO_TEST( test_func_mavp,     "MAVP/GROUPING" );
   DO_TEST( test_func_s_overflow, "MATH,ADD,SUB,MULT,DIV" );
   /* CDL* is a prefix claim: this one group covers every candlestick, so
    * naming any of them reaches it without the tag listing all 61. */
   DO_TEST( test_candlestick,   "All Candlesticks,CDL*" );
   /* The tag is what --function= substring-matches, so every function the
    * group covers must appear in it: --function=VWMA matched nothing before
    * VWMA was named here (issue #137). */
   /* Composite functions -- indicators defined as an exact arithmetic
    * composition of shipped primitives. Split by file (composite1, composite2,
    * ...) rather than by indicator: one file per group is the convention here,
    * and a single file had grown past 4000 lines. Each carries its own tag, so
    * --function= reaches the members of whichever file they live in. */
   DO_TEST( test_func_composite1, "PVO,VWMA,CMF,HMA,EFI,QSTICK,AO,AC,SUM" );
   DO_TEST( test_func_composite2, "SMI" );
   DO_TEST( test_func_marketfi, "MARKETFI" );
   DO_TEST( test_func_cmf,       "CMF" );
   DO_TEST( test_func_cmou,      "CMOU" );
   DO_TEST( test_func_variants,  "TA_S_,VARIANT" );
   DO_TEST( test_candle_precision, "CDLDOJI,CANDLE,VARIANT,PRECISION" );
   DO_TEST( test_func_rolling_extremum,
            "MIN,MAX,MINMAX,MIDPOINT,MIDPRICE,WILLR,ROLLING,BLOCKSCAN" );
   DO_TEST( test_func_legacy,    "LEGACY,064,FROZEN" );
   DO_TEST( test_func_stream_finite,
            "SMA,MINUS_DI,MA,MAVP,BBANDS,STOCH,CDLDOJI,STREAM,FINITE" );

   /* A filter that matched nothing must not read as success. The group tags are
    * hand-maintained and cover far fewer names than the library exports, so a
    * plausible `--function=SMA` selected zero groups and still printed "All
    * tests succeeded" -- the failure mode that hid the candlestick gap above.
    *
    * Only fail when these groups were the whole job: --codegen, --xlang-hash
    * and --fuzz-064 filter by REAL function name and legitimately run for names
    * no group tag carries (scripts/synth_gate.py drives --function=SYNTH that
    * way). There, zero groups here is expected, not an error. */
   if( functionFilter != NULL && nbGroupsRun == 0 &&
       !doCodegenTest && !doXlangHash && !doFuzz064 )
   {
      printf( "\nFAILED: --function=%s matched no test group, so nothing ran.\n",
              functionFilter );
      printf( "        The filter is a substring match against the GROUP TAG in\n" );
      printf( "        ta_regtest.c's DO_TEST list, not against the function name.\n" );
      printf( "        Some functions have no hand-written group at all (the plain\n" );
      printf( "        vector math, LINEARREG*, TSF, OBV, TYPPRICE, WCLPRICE); they\n" );
      printf( "        are covered by the systematic sweeps, which enumerate every\n" );
      printf( "        function through ta_abstract -- reach those with --codegen or\n" );
      printf( "        --xlang-hash. Otherwise add the name to its group's tag.\n" );
      return TA_REGTEST_FILTER_MATCHED_NOTHING;
   }

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
      printf( "       Languages: rust, c, java, csharp (default: all)\n" );
      printf( "       Example: --codegen=rust,java\n" );
      printf( "\n" );
      printf( "    --language=LANG[,LANG,...]\n" );
      printf( "       Filter which language servers to test with --codegen.\n" );
      printf( "       Valid values: rust, c, java, csharp (default: all)\n" );
      printf( "       Example: --language=c,rust\n" );
      printf( "\n" );
      printf( "       Requires language server binaries in the bin directory.\n" );
      printf( "       Build with: ta_codegen build\n" );
      printf( "\n" );
      printf( "    --xlang-hash\n" );
      printf( "       Cross-language BITWISE parity gate (issue #113): diff each\n" );
      printf( "       language server against the in-process C library on\n" );
      printf( "       seed-generated inputs, comparing full-precision output hashes\n" );
      printf( "       with no tolerance. Honors --function and --language (rust today).\n" );
      printf( "       Run from the bin directory (needs the language servers).\n" );
      printf( "\n" );
      printf( "    --seed=N\n" );
      printf( "       Seed the range sweeps, replaying a previous run. Each run\n" );
      printf( "       otherwise picks its own seed and prints it.\n" );
      printf( "\n" );
      printf( "   On success, the exit code is 0.\n" );
      printf( "   On failure, the exit code is a number that can be\n" );
      printf( "   found in c/src/tools/ta_regtest/ta_error_number.h\n" );
}
