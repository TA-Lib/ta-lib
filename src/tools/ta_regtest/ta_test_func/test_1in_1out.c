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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  020203 MF   First version.
 *
 */

/* Description:
 *
 *     Test functions which have the following
 *     characterisic:
 *      - have one input and one output
 *      - there is no optional parameters
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"
#include "server_verify.h"
#include "test_codegen.h"
#include "ta_stream_frame.h"
#include "../../ta_alloc_check.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef enum {
  TA_HT_DCPERIOD_TEST,
  TA_HT_DCPHASE_TEST,
  TA_HT_TRENDLINE_TEST,
  TA_HT_TRENDMODE_TEST,
  TA_SIN_TEST
} TA_TestId;

typedef struct
{
   TA_Integer  doRangeTestFlag;

   TA_TestId   theFunction;
   TA_Integer  unstablePeriod;

   TA_Integer startIdx;
   TA_Integer endIdx;

   TA_RetCode expectedRetCode;

   TA_Integer oneOfTheExpectedOutRealIndex0;
   TA_Real    oneOfTheExpectedOutReal0;

   TA_Integer expectedBegIdx;
   TA_Integer expectedNbElement;
} TA_Test;

typedef struct
{
   const TA_Test *test;
   const TA_Real *price;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

/**** Local variables definitions.     ****/

static TA_Test tableTest[] =
{
   /********************************/
   /* Some Hilbert Transform Tests */
   /********************************/
   { 1, TA_HT_TRENDMODE_TEST, 0, 0, 251, TA_SUCCESS,      0,  1.0, 63,  252-63 }, /* First Value */

   { 1, TA_HT_TRENDLINE_TEST, 0, 0, 251, TA_SUCCESS,      0,  88.257, 63,  252-63 }, /* First Value */
   { 0, TA_HT_TRENDLINE_TEST, 0, 0, 251, TA_SUCCESS,      0,  88.257, 63,  252-63 },
   { 0, TA_HT_TRENDLINE_TEST, 0, 0, 251, TA_SUCCESS, 252-66, 109.69, 63,  252-63 },
   { 0, TA_HT_TRENDLINE_TEST, 0, 0, 251, TA_SUCCESS, 252-65, 110.18, 63,  252-63 },
   { 0, TA_HT_TRENDLINE_TEST, 0, 0, 251, TA_SUCCESS, 252-64, 110.46, 63,  252-63 }, /* Last Value */

   { 1, TA_HT_DCPHASE_TEST, 0, 0, 251, TA_SUCCESS,      0, 22.1495, 63,  252-63 }, /* First Value */
   { 0, TA_HT_DCPHASE_TEST, 0, 0, 251, TA_SUCCESS, 252-66, -31.182, 63,  252-63 },
   { 0, TA_HT_DCPHASE_TEST, 0, 0, 251, TA_SUCCESS, 252-65, 23.2691, 63,  252-63 },
   { 0, TA_HT_DCPHASE_TEST, 0, 0, 251, TA_SUCCESS, 252-64, 47.2765, 63,  252-63 }, /* Last Value */

   { 1, TA_HT_DCPERIOD_TEST, 0, 0, 251, TA_SUCCESS,      0, 15.5527, 32,  252-32 }, /* First Value */
   { 0, TA_HT_DCPERIOD_TEST, 0, 0, 251, TA_SUCCESS, 252-33, 18.6140, 32,  252-32 },  /* Last Value */

   /*********************************/
   /* Trigonometric and Vector Math */
   /*********************************/
   { 1, TA_SIN_TEST, 0, 0, 251, TA_SUCCESS, 0, -0.38371, 0,  252 }, /* First Value */
   { 0, TA_SIN_TEST, 0, 0, 251, TA_SUCCESS, 251, 0.870319, 0,  252 }  /* Last Value */

};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
/* Each elementary math function is the C math library's routine of that
 * name, bar by bar. A committed value cannot say so: the library's result
 * differs in the last bit from one host to the next. The bound allows a build
 * that vectorizes the call through another routine of the same library; a
 * build whose batch loop is a kernel of its own is held by the kernel's
 * comparator instead, which has margin over what that kernel measures.
 */
typedef TA_RetCode (*ElemFunc)( int, int, const double[], int *, int *, double[] );

typedef struct
{
   const char *name;
   ElemFunc    func;
   double    (*reference)( double );
   double      divisor;   /* brings a close into the routine's domain */
} ElemRow;

static const ElemRow elemRows[] =
{
   { "ACOS",  TA_ACOS,  acos,  250.0 },
   { "ASIN",  TA_ASIN,  asin,  250.0 },
   { "ATAN",  TA_ATAN,  atan,   40.0 },
   { "COS",   TA_COS,   cos,    40.0 },
   { "COSH",  TA_COSH,  cosh,   40.0 },
   { "EXP",   TA_EXP,   exp,    40.0 },
   { "LN",    TA_LN,    log,    40.0 },
   { "LOG10", TA_LOG10, log10,  40.0 },
   { "SIN",   TA_SIN,   sin,    40.0 },
   { "SINH",  TA_SINH,  sinh,   40.0 },
   { "TAN",   TA_TAN,   tan,    40.0 },
   { "TANH",  TA_TANH,  tanh,   40.0 },
};
#define NB_ELEM_ROW (sizeof(elemRows)/sizeof(elemRows[0]))
#define ELEM_CAP 512

static ErrorNumber test_elementary_math( const TA_History *history )
{
   static double in[ELEM_CAP], out[ELEM_CAP];
   int nbBars = (int)history->nbBars;
   int nbNear = 0;
   unsigned int r;
   int i;

   if( nbBars < 1 || nbBars > ELEM_CAP )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   for( r = 0; r < NB_ELEM_ROW; r++ )
   {
      const ElemRow *row = &elemRows[r];
      int kernel = regtest_vmath_batch( row->name );
      TA_Integer begIdx = -1, nbElement = -1;
      TA_RetCode retCode;

      for( i = 0; i < nbBars; i++ )
         in[i] = history->close[i] / row->divisor;
      retCode = row->func( 0, nbBars - 1, in, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != nbBars )
      {
         printf( "\nFail: TA_%s rc=%d begIdx=%d count=%d on %d bars\n",
                 row->name, (int)retCode, (int)begIdx, (int)nbElement, nbBars );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbBars; i++ )
      {
         double want = row->reference( in[i] );
         double bound = 4.0 * DBL_EPSILON * fabs( want );
         int same;
         if( kernel )
         {
            nbNear++;
            same = codegen_vmath_near( out[i], want );
         }
         else
            same = fabs( out[i] - want ) <= bound;
         if( !same )
         {
            printf( "\nFail: TA_%s(%.17g) is %.17g, the math library answers %.17g\n",
                    row->name, in[i], out[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   /* Every row is a kernel-batch function, or none is. */
   if( nbNear != TA_VMATH_KERNEL * (int)NB_ELEM_ROW * nbBars )
   {
      printf( "\nFail: %d elementary math value(s) held by the kernel comparator, "
              "want %d\n", nbNear, TA_VMATH_KERNEL * (int)NB_ELEM_ROW * nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   return TA_TEST_PASS;
}

/* One element is one function of one value: not of its neighbour, of where the
 * range starts, of the range's length or of the tier computing it. A loop that
 * works several elements at a time can break each of those inside any
 * tolerance, and a comparison against the math library cannot tell. Ordinary
 * values are mixed with special ones, so a leak has an ordinary result to change.
 *
 * isFloat marks a value that is exactly a float, so that TA_S_ is given the
 * same number. Not the NaN: its bits through a float are the platform's.
 */
typedef struct
{
   double value;
   int    isFloat;
} ElemLaneIn;

static const ElemLaneIn elemLaneIn[] =
{
   { 0.5,   1 }, { (double)NAN,       0 },
   { 0.37,  0 }, { (double)INFINITY,  1 },
   { 0.25,  1 }, { -(double)INFINITY, 1 },
   { 0.81,  0 }, { 0.0,               1 },
   { 0.75,  1 }, { -0.0,              1 },
   { 0.12,  0 }, { DBL_MIN / 8.0,     0 },   /* subnormal */
   { 0.125, 1 }, { 1e300,             0 },
   { 0.63,  0 }, { -1.0,              1 },   /* outside LN and LOG10 */
   { 0.625, 1 }, { 2.0,               1 },   /* outside ACOS and ASIN */
   { 0.94,  0 }, { -0.0,              1 },
   { 1e300, 0 }, { -0.0,              1 },
   { (double)NAN, 0 }, { 0.0,         1 },
   { (double)INFINITY, 1 }, { -1.0,   1 },
   { 1e300, 0 }
};
/* Not a whole number of the kernel's blocks, and neither is the range starting
 * one bar later, which also moves every element to another place in its block. */
#define ELEM_LANE_N 27
/* Per function: 26 one bar later, 27 alone, 16 through TA_S_, 27 in place,
 * 27 filled, 27 opened. */
#define ELEM_LANE_CMP 150
#define ELEM_LANE_GUARD 7.25e-300

static ErrorNumber elemLaneFail( const char *name, const char *leg, int bar )
{
   printf( "\nFail: TA_%s: %s differs from the full-range call at bar %d\n",
           name, leg, bar );
   return TA_TESTUTIL_TFRR_BAD_CALCULATION;
}

static ErrorNumber test_elementary_lanes( void )
{
   static const double guard = ELEM_LANE_GUARD;
   double in[ELEM_LANE_N], full[ELEM_LANE_N + 1], out[ELEM_LANE_N + 1];
   float  inS[ELEM_LANE_N];
   const double *const inPtr[1]  = { in };
   const float  *const inSPtr[1] = { inS };
   double       *const outPtr[1] = { out };
   const int n = ELEM_LANE_N;
   int nbCmp = 0;
   unsigned int r;
   int i, t;

   if( (int)(sizeof(elemLaneIn)/sizeof(elemLaneIn[0])) != n )
      return TA_TESTUTIL_TFRR_BAD_PARAM;

   for( i = 0; i < n; i++ )
   {
      double widened;
      in[i]  = elemLaneIn[i].value;
      inS[i] = elemLaneIn[i].isFloat ? (float)in[i] : 0.25f;
      widened = (double)inS[i];
      if( elemLaneIn[i].isFloat && memcmp( &widened, &in[i], sizeof(double) ) != 0 )
      {
         printf( "\nFail: elementary math input %d (%.17g) is not a float\n", i, in[i] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   for( r = 0; r < NB_ELEM_ROW; r++ )
   {
      const ElemRow *row = &elemRows[r];
      const TA_StreamEntry  *se = NULL;
      const TA_VariantEntry *ve = NULL;
      TA_Integer beg = -1, nb = -1;
      TA_RetCode rc;
      void *stream;

      for( t = 0; t < TA_STREAM_TABLE_SIZE; t++ )
         if( strcmp( TA_StreamTable[t].name, row->name ) == 0 )
            se = &TA_StreamTable[t];
      for( t = 0; t < TA_VARIANT_TABLE_SIZE; t++ )
         if( strcmp( TA_VariantTable[t].name, row->name ) == 0 )
            ve = &TA_VariantTable[t];
      if( !se || !ve )
      {
         printf( "\nFail: TA_%s has no stream or no TA_S_ entry\n", row->name );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      full[n] = guard;
      rc = row->func( 0, n - 1, in, &beg, &nb, full );
      if( rc != TA_SUCCESS || beg != 0 || nb != n
          || memcmp( &full[n], &guard, sizeof(double) ) != 0 )
      {
         printf( "\nFail: TA_%s rc=%d begIdx=%d count=%d on %d values, or a write "
                 "past them\n", row->name, (int)rc, (int)beg, (int)nb, n );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      rc = row->func( 1, n - 1, in, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != 1 || nb != n - 1 )
         return elemLaneFail( row->name, "the range of the call starting one bar later", 1 );
      for( i = 1; i < n; i++ )
      {
         nbCmp++;
         if( memcmp( &out[i - 1], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "the call starting one bar later", i );
      }

      for( i = 0; i < n; i++ )
      {
         rc = row->func( i, i, in, &beg, &nb, out );
         nbCmp++;
         if( rc != TA_SUCCESS || beg != i || nb != 1
             || memcmp( &out[0], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "the one-element call", i );
      }

      rc = ve->single( 0, n - 1, inSPtr, NULL, &beg, &nb, outPtr, NULL );
      if( rc != TA_SUCCESS || beg != 0 || nb != n )
         return elemLaneFail( row->name, "the range of TA_S_", 0 );
      for( i = 0; i < n; i++ )
      {
         if( !elemLaneIn[i].isFloat )
            continue;
         nbCmp++;
         if( memcmp( &out[i], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "TA_S_", i );
      }

      memcpy( out, in, sizeof(in) );
      out[n] = guard;
      rc = row->func( 0, n - 1, out, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != 0 || nb != n
          || memcmp( &out[n], &guard, sizeof(double) ) != 0 )
         return elemLaneFail( row->name, "the range of the in-place call, or the "
                              "element after it,", n );
      for( i = 0; i < n; i++ )
      {
         nbCmp++;
         if( memcmp( &out[i], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "the in-place call", i );
      }

      stream = NULL;
      rc = se->openAndFill( &stream, inPtr, n, NULL, &beg, &nb, outPtr, NULL );
      if( stream )
         se->close( stream );
      if( rc != TA_SUCCESS || beg != 0 || nb != n )
         return elemLaneFail( row->name, "the range of OpenAndFill", 0 );
      for( i = 0; i < n; i++ )
      {
         nbCmp++;
         if( memcmp( &out[i], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "OpenAndFill", i );
      }

      /* Open computes into one slot: the last bar has to be the last store. */
      for( i = 0; i < n; i++ )
      {
         stream = NULL;
         rc = se->open( &stream, inPtr, i + 1, NULL, outPtr, NULL );
         if( stream )
            se->close( stream );
         nbCmp++;
         if( rc != TA_SUCCESS || memcmp( &out[0], &full[i], sizeof(double) ) != 0 )
            return elemLaneFail( row->name, "the value Open reports", i );
      }
   }

   if( nbCmp != (int)NB_ELEM_ROW * ELEM_LANE_CMP )
   {
      printf( "\nFail: %d elementary math lane comparison(s), want %d\n",
              nbCmp, (int)NB_ELEM_ROW * ELEM_LANE_CMP );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   return TA_TEST_PASS;
}

ErrorNumber test_func_1in_1out( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   for( i=0; i < NB_TEST; i++ )
   {
      /* Re-initialize all the unstable period to zero. */
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

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

   retValue = test_elementary_math( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_elementary_lanes();
   if( retValue != TA_TEST_PASS )
      return retValue;

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
   TA_Integer *intBuffer;
   int size, i;

   (void)outputNb;
   (void)outputBufferInt;

   *isOutputInteger = 0;

   testParam = (TA_RangeTestParam *)opaqueData;

   switch( testParam->test->theFunction )
   {
   case TA_HT_DCPERIOD_TEST:
      retCode = TA_HT_DCPERIOD( startIdx,
                                endIdx,
                                testParam->price,
                                outBegIdx,
                                outNbElement,
                                outputBuffer );
      *lookback = TA_HT_DCPERIOD_Lookback();
      break;
   case TA_HT_DCPHASE_TEST:
      retCode = TA_HT_DCPHASE( startIdx,
                               endIdx,
                               testParam->price,
                               outBegIdx,
                               outNbElement,
                               outputBuffer );

      *lookback = TA_HT_DCPHASE_Lookback();
      break;
   case TA_HT_TRENDLINE_TEST:
      retCode = TA_HT_TRENDLINE( startIdx,
                                 endIdx,
                                 testParam->price,
                                 outBegIdx,
                                 outNbElement,
                                 outputBuffer );
      *lookback = TA_HT_TRENDLINE_Lookback();
      break;
   case TA_HT_TRENDMODE_TEST:
      /* Trendmode returns integers, but this test
       * is comparing real, so a translation is done
       * here.
       */
      #define PRE_SENTINEL  ((TA_Integer)0xABABFEDC)
      #define POST_SENTINEL ((TA_Integer)0xEFABCDFF)
      #define ALLOC_INT_BUFFER(varSize)  \
      { \
         intBuffer = TA_Malloc(sizeof(TA_Integer)*(varSize+2)); \
         TA_TOOL_CHECK_ALLOC(intBuffer); \
         intBuffer[0]      = PRE_SENTINEL; \
         intBuffer[varSize+1] = POST_SENTINEL; \
      }

      size = endIdx-startIdx+1; \
      ALLOC_INT_BUFFER(size);
      retCode = TA_HT_TRENDMODE( startIdx,
                                 endIdx,
                                 testParam->price,
                                 outBegIdx,
                                 outNbElement,
                                 &intBuffer[1] );
      *lookback = TA_HT_TRENDMODE_Lookback();

      #define FREE_INT_BUFFER( destBuffer, varNbElement ) \
      { \
         if( intBuffer[0] != PRE_SENTINEL ) \
         { \
            retCode = TA_INTERNAL_ERROR(138); \
         } \
         else if( intBuffer[size+1] != POST_SENTINEL ) \
         { \
            retCode = TA_INTERNAL_ERROR(139); \
         } \
         else \
         { \
            for( i=0; i < varNbElement; i++ ) \
               destBuffer[i] = (double)intBuffer[i+1]; \
         } \
         TA_Free( intBuffer ); \
      }

      FREE_INT_BUFFER( outputBuffer, *outNbElement );
      break;
   case TA_SIN_TEST:
      retCode = TA_SIN( startIdx,
                        endIdx,
                        testParam->price,
                        outBegIdx,
                        outNbElement,
                        outputBuffer );
      *lookback = TA_SIN_Lookback();
      break;
   default:
      retCode = TA_INTERNAL_ERROR(132);
   }

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
   const TA_Real *referenceInput;

   TA_Integer *intBuffer;
   TA_Integer svIntOutput[MAX_NB_TEST_ELEMENT];
   int size, i;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->close,  history->nbBars );

   /* Change the input to MEDPRICE for some tests. */
   switch( test->theFunction )
   {
   case TA_HT_DCPERIOD_TEST:
   case TA_HT_DCPHASE_TEST:
   case TA_HT_TRENDLINE_TEST:
   case TA_HT_TRENDMODE_TEST:
      TA_MEDPRICE( 0, history->nbBars-1, history->high, history->low,
                   &outBegIdx, &outNbElement, gBuffer[0].in );

      /* Will be use as reference */
      TA_MEDPRICE( 0, history->nbBars-1, history->high, history->low,
                   &outBegIdx, &outNbElement, gBuffer[1].in );

      referenceInput = gBuffer[1].in;
      break;
   default:
      referenceInput = history->close;
   }

   /* Make a simple first call. */
   size = (test->endIdx-test->startIdx)+1;

   switch( test->theFunction )
   {
   case TA_HT_DCPERIOD_TEST:
      retCode = TA_HT_DCPERIOD( test->startIdx,
                                test->endIdx,
                                gBuffer[0].in,
                                &outBegIdx,
                                &outNbElement,
                                gBuffer[0].out0 );
      break;

   case TA_HT_DCPHASE_TEST:
      retCode = TA_HT_DCPHASE( test->startIdx,
                               test->endIdx,
                               gBuffer[0].in,
                               &outBegIdx,
                               &outNbElement,
                               gBuffer[0].out0 );
      break;
   case TA_HT_TRENDLINE_TEST:
      retCode = TA_HT_TRENDLINE( test->startIdx,
                                 test->endIdx,
                                 gBuffer[0].in,
                                 &outBegIdx,
                                 &outNbElement,
                                 gBuffer[0].out0 );
      break;
   case TA_HT_TRENDMODE_TEST:
      ALLOC_INT_BUFFER(size);
      retCode = TA_HT_TRENDMODE( test->startIdx,
                                 test->endIdx,
                                 gBuffer[0].in,
                                 &outBegIdx,
                                 &outNbElement,
                                 &intBuffer[1] );
      /* Save integer output for server_verify before FREE_INT_BUFFER frees intBuffer. */
      for( i=0; i < outNbElement && i < MAX_NB_TEST_ELEMENT; i++ )
         svIntOutput[i] = intBuffer[i+1];
      FREE_INT_BUFFER( gBuffer[0].out0, outNbElement );
      break;
   case TA_SIN_TEST:
      retCode = TA_SIN( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
	   break;
   default:
      retCode = TA_INTERNAL_ERROR(133);
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, referenceInput, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   if( server_verify_active() )
   {
      const char *funcName;
      switch( test->theFunction )
      {
      case TA_HT_DCPERIOD_TEST:  funcName = "HT_DCPERIOD";  break;
      case TA_HT_DCPHASE_TEST:   funcName = "HT_DCPHASE";   break;
      case TA_HT_TRENDLINE_TEST: funcName = "HT_TRENDLINE";  break;
      case TA_HT_TRENDMODE_TEST: funcName = "HT_TRENDMODE";  break;
      case TA_SIN_TEST:          funcName = "SIN";            break;
      default:                   funcName = "UNKNOWN";        break;
      }

      if( test->theFunction == TA_HT_TRENDMODE_TEST )
      {
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               NULL, 0,
                               NULL, (const TA_Integer*[]){ &svIntOutput[0], NULL });
      }
      else
      {
         int nearBefore = server_verify_vmath_comparisons();
         errNb = server_verify(funcName, test->startIdx, test->endIdx, history->nbBars,
                               retCode, outBegIdx, outNbElement,
                               (const TA_Real*[]){ gBuffer[0].in, NULL },
                               NULL, 0,
                               (const TA_Real*[]){ gBuffer[0].out0, NULL }, NULL);
         if( errNb == TA_TEST_PASS &&
             ( server_verify_vmath_comparisons() > nearBefore )
             != ( server_verify_vmath_pipes( funcName ) > 0 ) )
         {
            printf( "\nFail: TA_%s: %d server value(s) held by the kernel comparator "
                    "on %d pipe(s) that call for it\n", funcName,
                    server_verify_vmath_comparisons() - nearBefore,
                    server_verify_vmath_pipes( funcName ) );
            return TA_SV_ROUTED_VACUOUS;
         }
      }
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output
    * are the same buffer.
    */
   switch( test->theFunction )
   {
   case TA_HT_DCPERIOD_TEST:
      retCode = TA_HT_DCPERIOD( test->startIdx,
                                test->endIdx,
                                gBuffer[0].in,
                                &outBegIdx,
                                &outNbElement,
                                gBuffer[0].in
                              );
      break;

   case TA_HT_DCPHASE_TEST:
      retCode = TA_HT_DCPHASE( test->startIdx,
                               test->endIdx,
                               gBuffer[0].in,
                               &outBegIdx,
                               &outNbElement,
                               gBuffer[0].in
                              );
      break;
   case TA_HT_TRENDLINE_TEST:
      retCode = TA_HT_TRENDLINE( test->startIdx,
                                 test->endIdx,
                                 gBuffer[0].in,
                                 &outBegIdx,
                                 &outNbElement,
                                 gBuffer[0].in
                                );
      break;
   case TA_HT_TRENDMODE_TEST:
      ALLOC_INT_BUFFER(size);
      retCode = TA_HT_TRENDMODE( test->startIdx,
                                 test->endIdx,
                                 gBuffer[0].in,
                                 &outBegIdx,
                                 &outNbElement,
                                 &intBuffer[1]
                                );
      FREE_INT_BUFFER( gBuffer[0].in, outNbElement );
      break;
   case TA_SIN_TEST:
      retCode = TA_SIN( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].in
                        );
      break;
   default:
      retCode = TA_INTERNAL_ERROR(134);
   }

   /* The previous call should have the same output
    * as this call.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.price = referenceInput;

   if( test->doRangeTestFlag )
   {
      switch( test->theFunction )
      {
      case TA_HT_DCPERIOD_TEST:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_HT_DCPERIOD,
                              (void *)&testParam, 1, 0 );
         break;

      case TA_HT_DCPHASE_TEST:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_HT_DCPHASE,
                              (void *)&testParam, 1, 360 );
         break;

      case TA_HT_TRENDLINE_TEST:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_HT_TRENDLINE,
                              (void *)&testParam, 1, 0 );
         break;

      case TA_HT_TRENDMODE_TEST:
         errNb = doRangeTest( rangeTestFunction,
                              TA_FUNC_UNST_HT_TRENDMODE,
                              (void *)&testParam, 1, 0 );
         break;

      default:
         errNb = doRangeTest( rangeTestFunction,
                              TA_TEST_UNST_NONE,
                              (void *)&testParam, 1, 0 );
      }
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}

