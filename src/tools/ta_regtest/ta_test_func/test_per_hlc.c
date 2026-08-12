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
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     First version.
 *  061904 MF     Add test to detect cumulative errors in CCI algorithm
 *                when some values were close to zero (epsilon).
 *  021106 MF     Add tests for ULTOSC.
 *  042206 MF     Add tests for NATR
 *  120507 MF     Add tests for ACCBANDS
 *  070626 MF,CC  Add uniform-input CCI regression test (issue #7 / SF bug #107):
 *                identical prices over the period must yield exactly 0.0, not a
 *                spurious value from dividing sub-epsilon floating-point residue.
 *
 */

/* Description:
 *
 *     Test functions which have the following characteristic:
 *      - the input arrays are high, low and close.
 *      - the only parameter is a period, if any (WAD takes none).
 *
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum {
TA_CCI_TEST,
TA_WILLR_TEST,
TA_ULTOSC_TEST,
TA_NATR_TEST,
TA_ACCBANDS_TEST,
TA_WAD_TEST
} TA_TestId;

typedef struct
{
   TA_Integer doRangeTestFlag;

   TA_TestId  theFunction;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_Integer optInTimePeriod1;
   TA_Integer optInTimePeriod2;
   TA_Integer optInTimePeriod3;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /*****************/
   /* ACCBANDS TEST */
   /*****************/
   { 1, TA_ACCBANDS_TEST, 0, 251, 14, 0, 0, TA_SUCCESS,       0,  93.8575, 13,  252-13 },

   /****************/
   /* NATR TEST    */
   /****************/
   /* TODO Analyze further why NATR requires a very large unstable period.
    * for now, just disable range testing.
    */
   { 0, TA_NATR_TEST, 0, 251, 14, 0, 0, TA_SUCCESS,       0,  3.9321, 14,  252-14 },
   { 0, TA_NATR_TEST, 0, 251, 14, 0, 0, TA_SUCCESS,       1,  3.7576, 14,  252-14 },
   { 0, TA_NATR_TEST, 0, 251, 14, 0, 0, TA_SUCCESS,  252-15,  3.0229, 14,  252-14 },

   /****************/
   /* ULTOSC TEST  */
   /****************/
   { 0, TA_ULTOSC_TEST, 0, 251, 7, 14, 28, TA_SUCCESS,       0,   47.1713, 28,  252-28 },
   { 0, TA_ULTOSC_TEST, 0, 251, 7, 14, 28, TA_SUCCESS,       1,   46.2802, 28,  252-28 },
   { 1, TA_ULTOSC_TEST, 0, 251, 7, 14, 28, TA_SUCCESS,  252-29,   40.0854, 28,  252-28 },


   /****************/
   /* WILLR TEST   */
   /****************/
   { 0, TA_WILLR_TEST, 13, 251, 14, 0, 0, TA_SUCCESS,   1,   -66.9903,  13,  252-13 }, /* First Value */
   { 1, TA_WILLR_TEST,  0, 251, 14, 0, 0, TA_SUCCESS,   0,   -90.1943,  13,  252-13 },
   { 0, TA_WILLR_TEST,  0, 251, 14, 0, 0, TA_SUCCESS, 112,        0.0,  13,  252-13 },

   { 0, TA_WILLR_TEST,  24, 24, 14, 0, 0, TA_SUCCESS, 0,    -89.2857,  24,  1 },
   { 0, TA_WILLR_TEST,  25, 25, 14, 0, 0, TA_SUCCESS, 0,    -97.2602,  25,  1 },
   { 0, TA_WILLR_TEST,  26, 26, 14, 0, 0, TA_SUCCESS, 0,    -71.5482,  26,  1 },

   { 0, TA_WILLR_TEST, 251, 251, 14, 0, 0, TA_SUCCESS,      0,    -59.1515, 251,  1 },
   { 0, TA_WILLR_TEST,  14,  251, 14, 0, 0, TA_SUCCESS, 252-15,   -59.1515, 14,  252-14 },

   /****************/
   /*   CCI TEST  */
   /****************/

   /* The following two should always be identical. */
   { 0, TA_CCI_TEST, 186,187,  2, 0, 0, TA_SUCCESS,   1, 0.0, 186,  2 },
   { 0, TA_CCI_TEST, 187,187,  2, 0, 0, TA_SUCCESS,   0, 0.0, 187,  1 },

   /* Test period 2, 5 and 11 */
   { 0, TA_CCI_TEST, 0, 251,  2, 0, 0, TA_SUCCESS,  0, 66.666, 1,  252-1 },
   { 1, TA_CCI_TEST, 0, 251,  5, 0, 0, TA_SUCCESS,  0, 18.857, 4,  252-4 },

   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  0,   87.927,  10,  252-10 }, /* First Value */
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  1,   180.005, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  2,  143.5190963, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  3,  -113.8669783, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  4,  -111.064497, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  5,  -26.77393309, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  6,  -70.77933765, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  7,  -83.15662884, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  8,  -41.14421073, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS,  9,  -49.63059589, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 10,  -86.45142995, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 11,  -105.6275799, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 12,  -157.698269, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 13,  -190.5251436, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 14,  -142.8364298, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 15,  -122.4448056, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 16,  -79.95100041, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 17,  22.03829204, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 18,  7.765575065, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 19,  32.38905945, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 20,  -0.005587727, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 21,  43.84607294, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 22,  40.35152301, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 23,  92.89237535, 10,  252-10 },
   { 0, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 24,  113.4778681, 10,  252-10 },
   { 1, TA_CCI_TEST, 0, 251, 11, 0, 0, TA_SUCCESS, 252-11,  -169.65514, 10,  252-10 }, /* Last Value */

   /************/
   /* WAD TEST */
   /************/
   /* No parameter; lookback 0, so the whole requested range comes back and
    * out[k] is the line at bar startIdx+k. Values recomputed independently
    * from the corpus arrays in test_data.c, not read back out of the
    * implementation.
    *
    * CHECK_EXPECTED_VALUE is adequate here, unlike for MARKETFI: its band is
    * an ABSOLUTE 0.01 and WAD spans [-20.73, 45.77] on this corpus. The
    * discriminating question is WHERE to pin, not how tightly.
    *
    * The flat arm of the three-way branch -- close exactly equal to the
    * previous close -- has two occurrences worth pinning, and random fuzz
    * doubles produce neither:
    *   - the seed bar, at every startIdx, since it is measured against
    *     itself. Turning `>` into `>=` makes it accumulate 0.75 instead of
    *     nothing, so out[0] stops being exactly 0.0 -- that golden is the
    *     cheapest guard in the table and is not decoration;
    *   - bar 101, the corpus's only pair of equal consecutive closes
    *     (close[101] == close[100] == 116.0). The same mutation shifts
    *     everything from there onward again, which is why goldens sit on
    *     both sides of it. */
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,       0,    0.0,    0,  252 }, /* seed bar accumulates nothing */
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,       1,    3.410,  0,  252 },
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,      51,   -7.050,  0,  252 },
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,     100,   23.675,  0,  252 }, /* just before the equal-close bar */
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,     101,   23.675,  0,  252 }, /* the flat arm: unchanged */
   { 0, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,     125,   36.535,  0,  252 }, /* past it */
   { 1, TA_WAD_TEST, 0, 251, 0, 0, 0, TA_SUCCESS,     251,   -3.375,  0,  252 }, /* Last Value */

   /* Start-dependent: the accumulator restarts at startIdx, so a later start
    * is the same shape offset by a constant, never the same values. */
   { 0, TA_WAD_TEST, 100, 251, 0, 0, 0, TA_SUCCESS,      0,    0.0,  100, 152 },
   { 0, TA_WAD_TEST, 100, 251, 0, 0, 0, TA_SUCCESS,      1,    0.0,  100, 152 }, /* bar 101 again, now at the range head */
   { 0, TA_WAD_TEST, 251, 251, 0, 0, 0, TA_SUCCESS,      0,    0.0,  251, 1   }
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/* Issue #7 / SF bug 107: with identical prices over the period, CCI is 0/0.
 * Sub-epsilon FP residue used to slip past the exact "!= 0.0" guard and divide
 * into a spurious ~66.67; TA_IS_ZERO now returns 0.0. Cross-checks all backends
 * under --codegen. */
static ErrorNumber test_cci_uniform_zero( void )
{
   static const int periods[] = { 5, 8, 14, 30 };
   const int nbBars = 40;
   TA_Real high[40], low[40], close[40], out[40];
   TA_Integer outBegIdx, outNbElement;
   TA_RetCode retCode;
   int k, i;

   for( i = 0; i < nbBars; i++ )
      high[i] = low[i] = close[i] = 1.1;

   for( k = 0; k < (int)(sizeof(periods)/sizeof(periods[0])); k++ )
   {
      int period = periods[k];

      retCode = TA_CCI( 0, nbBars-1, high, low, close, period,
                        &outBegIdx, &outNbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "Fail: CCI uniform input returned retCode=%d (period %d)\n",
                 (int)retCode, period );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < outNbElement; i++ )
      {
         if( out[i] != 0.0 )
         {
            printf( "Fail: CCI uniform input period %d out[%d]=%.17g, expected 0.0 "
                    "(issue #7 / SF bug #107)\n", period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      if( server_verify_active() )
      {
         ErrorNumber errNb = server_verify( "CCI", 0, nbBars-1, nbBars,
                                retCode, outBegIdx, outNbElement,
                                (const TA_Real*[]){ high, low, close, NULL },
                                (double[]){ (double)period }, 1,
                                (const TA_Real*[]){ out, NULL }, NULL );
         if( errNb != TA_TEST_PASS )
            return errNb;
      }
   }

   return TA_TEST_PASS;
}


/* WAD external vectors. The pinned corpus values above were recomputed from
 * test_data.c, so they anchor the implementation to arithmetic but not to any
 * published source. These two do.
 *
 * Both were transcribed by Tulip Indicators 0.9.2 and both reproduce here
 * exactly. Tulip's ti_wad_start() returns 1 where this implementation returns
 * 0, so its series is this one WITHOUT the leading zero -- the offset below is
 * deliberate and is the whole reason the vectors are checked by hand rather
 * than fed through a generic harness, where a silent misalignment would be
 * indistinguishable from agreement.
 *
 * Tolerance is absolute: the published values carry three or four decimals,
 * and WAD is a running sum of price differences, so an absolute band is the
 * meaningful one. 5e-4 is half the last printed digit of the coarser vector. */
#define WAD_VECTOR_TOL 5e-4

/* Achelis, Technical Analysis from A to Z, 2nd ed., p.368
 * (Tulip tests/atoz.txt:270). 12 bars in, 11 published values. */
static const TA_Real wadBookHigh[]  =
   { 21.5, 21.625, 21.125, 22.438, 23.5, 23.25, 25, 25.625, 27.125, 28.75, 28, 30.375 };
static const TA_Real wadBookLow[]   =
   { 20.75, 21, 20.5, 20.875, 22.438, 22.438, 22.875, 23.75, 24.938, 26.875, 26.25, 27.625 };
static const TA_Real wadBookClose[] =
   { 21.25, 21.031, 20.875, 22, 22.5, 23, 24.563, 25.375, 26.875, 27.375, 27.75, 29.5 };
static const TA_Real wadBookExp[]   =
   { -0.594, -0.844, 0.281, 0.781, 1.343, 3.031, 4.656, 6.593, 7.093, 8.593, 10.468 };

/* Tulip tests/untest.txt:471. 15 bars in, 14 published values. */
static const TA_Real wadTulipHigh[]  =
   { 82.15, 81.89, 83.03, 83.30, 83.85, 83.90, 83.33, 84.30,
     84.84, 85.00, 85.90, 86.58, 86.98, 88.00, 87.87 };
static const TA_Real wadTulipLow[]   =
   { 81.29, 80.64, 81.31, 82.65, 83.07, 83.11, 82.49, 82.30,
     84.15, 84.11, 84.03, 85.39, 85.76, 87.17, 87.01 };
static const TA_Real wadTulipClose[] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const TA_Real wadTulipExp[]   =
   { -0.830, 0.980, 1.330, 1.940, 1.190, 0.700, 2.390, 2.950,
     2.310, 3.810, 4.960, 6.090, 6.970, 6.390 };

static ErrorNumber wad_check_vector( const char *tag,
                                     const TA_Real *high, const TA_Real *low,
                                     const TA_Real *close, int nbIn,
                                     const TA_Real *expected, int nbExpected )
{
   TA_Real out[32];
   TA_Integer beg, nb;
   TA_RetCode retCode;
   int i;

   retCode = TA_WAD( 0, nbIn - 1, high, low, close, &beg, &nb, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "Fail: WAD %s returned retCode=%d\n", tag, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   /* Lookback 0: one MORE value than the published vector, the leading zero. */
   if( beg != 0 || nb != nbExpected + 1 )
   {
      printf( "Fail: WAD %s beg=%d nb=%d, expected 0/%d\n",
              tag, (int)beg, (int)nb, nbExpected + 1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   if( out[0] != 0.0 )
   {
      printf( "Fail: WAD %s out[0]=%.17g, the seed bar must contribute exactly 0\n",
              tag, out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( i = 0; i < nbExpected; i++ )
   {
      double diff = out[i+1] - expected[i];
      if( diff < 0.0 ) diff = -diff;
      if( diff > WAD_VECTOR_TOL )
      {
         printf( "Fail: WAD %s at published value %d: got %.17g expected %.17g "
                 "(|diff|=%.3e > %.3e)\n",
                 tag, i, out[i+1], expected[i], diff, WAD_VECTOR_TOL );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_wad_published_vectors( void )
{
   ErrorNumber retValue;

   retValue = wad_check_vector( "book vector (Achelis p.368)",
                                wadBookHigh, wadBookLow, wadBookClose,
                                (int)(sizeof(wadBookHigh)/sizeof(TA_Real)),
                                wadBookExp,
                                (int)(sizeof(wadBookExp)/sizeof(TA_Real)) );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = wad_check_vector( "tulip vector (untest.txt wad)",
                                wadTulipHigh, wadTulipLow, wadTulipClose,
                                (int)(sizeof(wadTulipHigh)/sizeof(TA_Real)),
                                wadTulipExp,
                                (int)(sizeof(wadTulipExp)/sizeof(TA_Real)) );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/**** Global functions definitions.   ****/
ErrorNumber test_func_per_hlc( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Degenerate uniform-input CCI regression (issue #7 / SF bug #107). */
   retValue = test_cci_uniform_zero();
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed CCI uniform-input test (Code=%d)\n", retValue );
      return retValue;
   }

   /* WAD against the two published vectors (Achelis p.368 and Tulip). */
   retValue = test_wad_published_vectors();
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed WAD published-vector test (Code=%d)\n", retValue );
      return retValue;
   }

   for( i=0; i < NB_TEST; i++ )
   {
      if( (int)tableTest[i].expectedNbElement > (int)history->nbBars )
      {
         printf( "Failed Bad Parameter for Test #%d (%d,%d)\n",
                 i, tableTest[i].expectedNbElement, history->nbBars );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "Failed Test #%d (Code=%d)\n", i, retValue );
         return retValue;
      }
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All test succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/
static TA_RetCode rangeTestFunction( TA_Integer    startIdx,
                                     TA_Integer    endIdx,
                                     TA_Real      *outputBuffer,
                                     TA_Integer   *outputBufferInt,
                                     TA_Integer   *outBegIdx,
                                     TA_Integer   *outNbElement,
                                     TA_Integer   *lookback,
                                     void         *opaqueData,
                                     unsigned int  outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RetCode retCode;
   TA_RangeTestParam *testParam;
   TA_Real *dummyBuffer1, *dummyBuffer2;
   TA_Real *out1, *out2, *out3;

   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   if( testParam->test->theFunction != TA_ACCBANDS_TEST )
   {
	   out1 = out2 = out3 = NULL;
	   dummyBuffer1 = NULL;
	   dummyBuffer2 = NULL;
   }
   else
   {
	   dummyBuffer1 = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Real));
	   if( !dummyBuffer1 )
		 return TA_ALLOC_ERR;

	   dummyBuffer2 = TA_Malloc( ((endIdx-startIdx)+1)*sizeof(TA_Real));
	   if( !dummyBuffer2 )
	   {
		  TA_Free( dummyBuffer1 );
		  return TA_ALLOC_ERR;
	   }

	   switch( outputNb )
	   {
	   case 0:
		  out1 = outputBuffer;
		  out2 = dummyBuffer1;
		  out3 = dummyBuffer2;
		  break;
	   case 1:
		  out2 = outputBuffer;
		  out1 = dummyBuffer1;
		  out3 = dummyBuffer2;
		  break;
	   case 2:
		  out3 = outputBuffer;
		  out2 = dummyBuffer1;
		  out1 = dummyBuffer2;
		  break;
	   default:
		  TA_Free( dummyBuffer1 );
		  TA_Free( dummyBuffer2 );
		  return TA_BAD_PARAM;
	   }
   }

   switch( testParam->test->theFunction )
   {
   case TA_NATR_TEST:
      retCode = TA_NATR( startIdx,
                         endIdx,
                         testParam->high,
                         testParam->low,
                         testParam->close,
                         testParam->test->optInTimePeriod1,
                         outBegIdx,
                         outNbElement,
                         outputBuffer );
      *lookback = TA_NATR_Lookback( testParam->test->optInTimePeriod1 );
      break;

   case TA_CCI_TEST:
      retCode = TA_CCI( startIdx,
                        endIdx,
                        testParam->high,
                        testParam->low,
                        testParam->close,
                        testParam->test->optInTimePeriod1,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_CCI_Lookback( testParam->test->optInTimePeriod1 );
      break;
   case TA_WILLR_TEST:
      retCode = TA_WILLR( startIdx,
                          endIdx,
                          testParam->high,
                          testParam->low,
                          testParam->close,
                          testParam->test->optInTimePeriod1,
                          outBegIdx,
                          outNbElement,
                          outputBuffer );
      *lookback = TA_WILLR_Lookback( testParam->test->optInTimePeriod1 );
      break;

   case TA_ULTOSC_TEST:
      retCode = TA_ULTOSC( startIdx,
                           endIdx,
                           testParam->high,
                           testParam->low,
                           testParam->close,
                           testParam->test->optInTimePeriod1,
                           testParam->test->optInTimePeriod2,
                           testParam->test->optInTimePeriod3,
                           outBegIdx,
                           outNbElement,
                           outputBuffer );
      *lookback = TA_ULTOSC_Lookback( testParam->test->optInTimePeriod1,
                                      testParam->test->optInTimePeriod2,
                                      testParam->test->optInTimePeriod3 );
      break;
   case TA_ACCBANDS_TEST:
      retCode = TA_ACCBANDS( startIdx,
                          endIdx,
                          testParam->high,
                          testParam->low,
                          testParam->close,
                          testParam->test->optInTimePeriod1,
                          outBegIdx,
                          outNbElement,
                          out1, out2, out3 );
      *lookback = TA_ACCBANDS_Lookback( testParam->test->optInTimePeriod1 );
	  break;

   case TA_WAD_TEST:
      retCode = TA_WAD( startIdx,
                        endIdx,
                        testParam->high,
                        testParam->low,
                        testParam->close,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_WAD_Lookback();
      break;

   default:
      retCode = TA_INTERNAL_ERROR(132);
   }

   FREE_IF_NOT_NULL( dummyBuffer1 );
   FREE_IF_NOT_NULL( dummyBuffer2 );
   return retCode;
}

static TA_RetCode do_call( const TA_Test *test,
                            const double high[],
                            const double low[],
                            const double close[],
                            int *outBegIdx,
                            int *outNbElement,
                            double output[] )
{
   TA_RetCode retCode;
   TA_Real *dummyBuffer1, *dummyBuffer2;

   if( test->theFunction != TA_ACCBANDS_TEST )
   {
	   dummyBuffer1 = NULL;
	   dummyBuffer2 = NULL;
   }
   else
   {
	   dummyBuffer1 = TA_Malloc( ((test->endIdx-test->startIdx)+1)*sizeof(TA_Real));
	   if( !dummyBuffer1 )
		 return TA_ALLOC_ERR;

	   dummyBuffer2 = TA_Malloc( ((test->endIdx-test->startIdx)+1)*sizeof(TA_Real));
	   if( !dummyBuffer2 )
	   {
		  TA_Free( dummyBuffer1 );
		  return TA_ALLOC_ERR;
	   }
   }

   switch( test->theFunction )
   {
   case TA_NATR_TEST:
      retCode = TA_NATR( test->startIdx,
                         test->endIdx,
                         high, low, close,
                         test->optInTimePeriod1,
                         outBegIdx,
                         outNbElement,
                         output );
      break;

   case TA_CCI_TEST:
      retCode = TA_CCI( test->startIdx,
                        test->endIdx,
                        high, low, close,
                        test->optInTimePeriod1,
                        outBegIdx,
                        outNbElement,
                        output );
      break;

   case TA_WILLR_TEST:
      retCode = TA_WILLR( test->startIdx,
                          test->endIdx,
                          high, low, close,
                          test->optInTimePeriod1,
                          outBegIdx,
                          outNbElement,
                          output );
      break;

   case TA_ULTOSC_TEST:
      retCode = TA_ULTOSC( test->startIdx,
                           test->endIdx,
                           high, low, close,
                           test->optInTimePeriod1,
                           test->optInTimePeriod2,
                           test->optInTimePeriod3,
                           outBegIdx,
                           outNbElement,
                           output );
      break;

   case TA_ACCBANDS_TEST:
	   /* TODO: replace dummy with real for more complete tests. */
      retCode = TA_ACCBANDS( test->startIdx,
                          test->endIdx,
                          high, low, close,
                          test->optInTimePeriod1,
                          outBegIdx,
                          outNbElement,
                          dummyBuffer1, output, dummyBuffer2 );
      break;

   case TA_WAD_TEST:
      retCode = TA_WAD( test->startIdx,
                        test->endIdx,
                        high, low, close,
                        outBegIdx,
                        outNbElement,
                        output );
      break;

   default:
      retCode = TA_INTERNAL_ERROR(133);
   }

   FREE_IF_NOT_NULL( dummyBuffer1 );
   FREE_IF_NOT_NULL( dummyBuffer2 );

   return retCode;
}

static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test )
{
   TA_RetCode retCode;
   ErrorNumber errNb;
   TA_Integer outBegIdx;
   TA_Integer outNbElement;
   TA_RangeTestParam testParam;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->high,  history->nbBars );
   setInputBuffer( 1, history->low,   history->nbBars );
   setInputBuffer( 2, history->close, history->nbBars );

   /* Make a simple first call. */
   retCode = do_call( test,
                      gBuffer[0].in,
                      gBuffer[1].in,
                      gBuffer[2].in,
                      &outBegIdx,
                      &outNbElement,
                      gBuffer[0].out0 );

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   if( server_verify_active() )
   {
      const char *funcName;
      switch( test->theFunction )
      {
      case TA_CCI_TEST:
         funcName = "CCI";
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               (double[]){ (double)test->optInTimePeriod1 }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_WILLR_TEST:
         funcName = "WILLR";
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               (double[]){ (double)test->optInTimePeriod1 }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_ULTOSC_TEST:
         funcName = "ULTOSC";
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               (double[]){ (double)test->optInTimePeriod1,
                                           (double)test->optInTimePeriod2,
                                           (double)test->optInTimePeriod3 }, 3,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_WAD_TEST:
         funcName = "WAD";
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               NULL, 0,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_NATR_TEST:
         funcName = "NATR";
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                   gBuffer[2].in, NULL },
                               (double[]){ (double)test->optInTimePeriod1 }, 1,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         break;
      case TA_ACCBANDS_TEST:
         {
            /* ACCBANDS has 3 outputs; do_call only captured the middle band.
             * Make a separate call to get all 3 outputs for server verification. */
            TA_Integer svBeg, svNb;
            funcName = "ACCBANDS";
            retCode = TA_ACCBANDS( test->startIdx, test->endIdx,
                                   gBuffer[0].in, gBuffer[1].in, gBuffer[2].in,
                                   test->optInTimePeriod1,
                                   &svBeg, &svNb,
                                   gBuffer[0].out1, gBuffer[0].out0, gBuffer[0].out2 );
            errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                                  retCode, svBeg, svNb,
                                  (const TA_Real*[]){ gBuffer[0].in, gBuffer[1].in,
                                                      gBuffer[2].in, NULL },
                                  (double[]){ (double)test->optInTimePeriod1 }, 1,
                                  (const TA_Real*[]){ gBuffer[0].out1, gBuffer[0].out0,
                                                      gBuffer[0].out2, NULL }, NULL);
         }
         break;
      default:
         errNb = TA_TEST_PASS;
         break;
      }
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = do_call( test,
                      gBuffer[0].in,
                      gBuffer[1].in,
                      gBuffer[2].in,
                      &outBegIdx,
                      &outNbElement,
                      gBuffer[0].in );

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   setInputBuffer( 0, history->high,  history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = do_call( test,
                      gBuffer[0].in,
                      gBuffer[1].in,
                      gBuffer[2].in,
                      &outBegIdx,
                      &outNbElement,
                      gBuffer[1].in );

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );
   setInputBuffer( 1, history->low,   history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   retCode = do_call( test,
                      gBuffer[0].in,
                      gBuffer[1].in,
                      gBuffer[2].in,
                      &outBegIdx,
                      &outNbElement,
                      gBuffer[2].in );

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].in, 0 );
   setInputBuffer( 2, history->close, history->nbBars );

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.high  = history->high;
   testParam.low   = history->low;
   testParam.close = history->close;

   if( test->doRangeTestFlag )
   {
      switch( test->theFunction )
      {
      case TA_NATR_TEST:
		  /* Special case: Unstable period to test */
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_NATR,
                              (void *)&testParam, 1, 0 );
         break;

	  case TA_ACCBANDS_TEST:
		  /* Special case: 3 outputs to test */
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 3, 0 );
         break;

      case TA_WAD_TEST:
         /* Special case: start-dependent (#127). The accumulator restarts at
          * startIdx, so a sub-range is a constant offset from the full-range
          * line and the sweep must check the shape, not the values -- the
          * same treatment AD and OBV get in test_per_hlcv.c. */
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 1,
                              TA_DO_NOT_COMPARE );
         break;

      default:
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 1, 0 );
         break;
      }

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

