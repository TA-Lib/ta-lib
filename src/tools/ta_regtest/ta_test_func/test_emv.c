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
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092826 MF,CC  First version (issue #465).
 */

/* Test TA_EMV (Arms Ease of Movement).
 *
 * --codegen, --xlang-hash and server_verify compare every language against
 * this library, so none of them can catch a wrong formula. The legs here can:
 *
 *   1. COMPOSITE, bitwise: TA_MEDPRICE, TA_MOM(1), TA_DIV, TA_SUB, TA_DIV,
 *      TA_DIV, TA_SMA over the reference corpus, which has no flat and no
 *      zero-volume bar. Pins operand order, component order, the SMA's
 *      summation order, its anchor and the lookback.
 *   2. GOLDENS: Achelis p. 132 (via Tulip), LEAN's spy_emv.txt, and rows on
 *      the reference corpus cross-checked against LEAN.
 *   3. DEGENERATE BARS: a flat, a zero-volume and a both bar, batch, stream
 *      and every language server.
 *   4. LOOKBACK, ALIASING, RANGE.
 *
 * Never CHECK_EXPECTED_VALUE: its absolute 0.01 band passes a constant 0
 * against values near 1e-3.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

#define EMV_MAX_BARS 300

static ErrorNumber test_emv_composite ( const TA_History *history );
static ErrorNumber test_emv_sref_rows ( const TA_History *history );
static ErrorNumber test_emv_achelis   ( void );
static ErrorNumber test_emv_lean      ( void );
static ErrorNumber test_emv_degenerate( const TA_History *history );
static ErrorNumber test_emv_underflow ( void );
static ErrorNumber test_emv_lookback  ( const TA_History *history );
static ErrorNumber test_emv_aliasing  ( const TA_History *history );
static ErrorNumber test_emv_range     ( const TA_History *history );

ErrorNumber test_func_emv( TA_History *history )
{
   ErrorNumber e;

   if( history->nbBars > EMV_MAX_BARS )
   {
      printf( "Fail: EMV test expects the %d-bar reference corpus, got %d\n",
              EMV_MAX_BARS, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   if( (e = test_emv_composite( history ))  != TA_TEST_PASS ) return e;
   if( (e = test_emv_sref_rows( history ))  != TA_TEST_PASS ) return e;
   if( (e = test_emv_achelis())             != TA_TEST_PASS ) return e;
   if( (e = test_emv_lean())                != TA_TEST_PASS ) return e;
   if( (e = test_emv_degenerate( history )) != TA_TEST_PASS ) return e;
   if( (e = test_emv_underflow())           != TA_TEST_PASS ) return e;
   if( (e = test_emv_lookback( history ))   != TA_TEST_PASS ) return e;
   if( (e = test_emv_aliasing( history ))   != TA_TEST_PASS ) return e;
   if( (e = test_emv_range( history ))      != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

/* raw[k] is bar k+1's one-bar value, from bars 0..nb-1. */
static int emv_compose_raw( const double *h, const double *l, const double *v, int nb,
                            double d, double *raw )
{
   static double mid[EMV_MAX_BARS], mom[EMV_MAX_BARS], vd[EMV_MAX_BARS];
   static double rng[EMV_MAX_BARS], box[EMV_MAX_BARS];
   static double dconst[EMV_MAX_BARS];
   TA_Integer b, k;
   int i;

   for( i = 0; i < nb; i++ )
      dconst[i] = d;

   if( TA_MEDPRICE( 0, nb-1, h, l, &b, &k, mid ) != TA_SUCCESS || b != 0 ) return 0;
   if( TA_MOM( 0, nb-1, mid, 1, &b, &k, mom ) != TA_SUCCESS || b != 1 ) return 0;
   if( TA_DIV( 0, nb-1, v, dconst, &b, &k, vd ) != TA_SUCCESS || b != 0 ) return 0;
   if( TA_SUB( 0, nb-1, h, l, &b, &k, rng ) != TA_SUCCESS || b != 0 ) return 0;
   if( TA_DIV( 0, nb-1, vd, rng, &b, &k, box ) != TA_SUCCESS || b != 0 ) return 0;
   if( TA_DIV( 0, nb-2, mom, box+1, &b, &k, raw ) != TA_SUCCESS || b != 0 ) return 0;
   return 1;
}

/* (1) Bitwise against the composition, across n, the divisor and startIdx. */
static ErrorNumber test_emv_composite( const TA_History *history )
{
   static const int    nGrid[] = { 1, 2, 14, 50, 251 };
   static const double dGrid[] = { 1.0, 10000.0, 1e8 };
   static const int    sGrid[] = { 0, 1, 13, 14, 15, 100, 251 };
   static double raw[EMV_MAX_BARS], ref[EMV_MAX_BARS], out[EMV_MAX_BARS];
   int nb = (int)history->nbBars;
   int a, c, s, j, compared = 0;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   TA_RetCode retCode;

   for( a = 0; a < (int)(sizeof(nGrid)/sizeof(nGrid[0])); a++ )
   for( c = 0; c < (int)(sizeof(dGrid)/sizeof(dGrid[0])); c++ )
   {
      int n = nGrid[a];
      double d = dGrid[c];

      if( !emv_compose_raw( history->high, history->low, history->volume, nb,
                            d, raw ) )
      {
         printf( "Fail: EMV composite: a primitive failed (n=%d D=%g)\n", n, d );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( s = 0; s < (int)(sizeof(sGrid)/sizeof(sGrid[0])); s++ )
      {
         int startIdx = sGrid[s];
         int wantBeg = startIdx > n ? startIdx : n;

         retCode = TA_EMV( startIdx, nb-1, history->high, history->low,
                           history->volume, n, d, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != wantBeg || nbElement != nb-wantBeg )
         {
            printf( "Fail: TA_EMV composite n=%d D=%g start=%d: rc=%d beg=%d nb=%d "
                    "(want %d/%d)\n", n, d, startIdx, (int)retCode, (int)begIdx,
                    (int)nbElement, wantBeg, nb-wantBeg );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         /* Started at the same bar: the running sum's anchor is part of what
          * is pinned, and a later anchor reassociates it.
          */
         if( TA_SMA( wantBeg-1, nb-2, raw, n, &refBeg, &refNb, ref ) != TA_SUCCESS
             || refBeg != wantBeg-1 || refNb != nbElement )
         {
            printf( "Fail: EMV composite SMA n=%d start=%d failed\n", n, startIdx );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         for( j = 0; j < (int)nbElement; j++ )
         {
            const double *want = &ref[j];
            if( memcmp( &out[j], want, sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_EMV composite n=%d D=%g start=%d bar %d: %.17g, "
                       "composition %.17g\n", n, d, startIdx, (int)begIdx+j,
                       out[j], *want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            compared++;
         }

         /* The sweep already sends this corpus; route only the divisors it
          * does not reach. */
         if( server_verify_active() && startIdx == 0 && d != 10000.0 )
         {
            double optIn[2];
            ErrorNumber e;
            optIn[0] = (double)n;
            optIn[1] = d;
            e = server_verify( "EMV", startIdx, nb-1, nb, retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low,
                                                   history->volume, NULL },
                               optIn, 2, (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   if( compared == 0 )
   {
      printf( "Fail: EMV composite compared nothing\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (2a) Rows on the reference corpus, D = 10,000. Each is the exact rational on
 * the binary inputs rounded to double (issue #465). The ta-lib-oracles
 * lean_serve TA_EMV arm (QuantConnect LEAN EaseOfMovementValue at
 * 5b0c9975d318, arm commit d980f73) is within 2.5e-15 relative of every row.
 * The values are near 1e-3, so the absolute floor sits well below them.
 */
typedef struct { int n; int bar; double value; } EmvRow;

static const EmvRow emvSrefRow[] = {
   {  1,   1,  0.0083633396557638243  },
   {  1,  14, -0.0068326633595620903  },
   {  1,  15,  0.0074051636607245733  },
   {  1,  50, -0.0041088965915655673  },
   {  1, 125,  0.0094624323104693046  },
   {  1, 251, -0.014748650060964954   },
   { 14,  14,  0.00072293640312622097 },
   { 14,  15,  0.00065449526062341733 },
   { 14,  50,  0.0011431697623238625  },
   { 14, 125,  0.0073545797518352616  },
   { 14, 200, -0.0029857687097996307  },
   { 14, 251, -0.0012225429060355375  }
};
#define EMV_SREF_TOL 1e-12
#define EMV_SREF_ABS 1e-18

static ErrorNumber test_emv_sref_rows( const TA_History *history )
{
   static double out[EMV_MAX_BARS];
   int nb = (int)history->nbBars;
   TA_Integer begIdx, nbElement;
   int k;

   for( k = 0; k < (int)(sizeof(emvSrefRow)/sizeof(emvSrefRow[0])); k++ )
   {
      const EmvRow *r = &emvSrefRow[k];
      double err;
      const char *mode;

      if( TA_EMV( 0, nb-1, history->high, history->low, history->volume,
                  r->n, 10000.0, &begIdx, &nbElement, out ) != TA_SUCCESS
          || begIdx != r->n )
      {
         printf( "Fail: TA_EMV reference rows n=%d: call failed\n", r->n );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( !checkOracleValue( out[r->bar - begIdx], r->value,
                             EMV_SREF_TOL, EMV_SREF_ABS, &err, &mode ) )
      {
         printf( "Fail: TA_EMV reference row n=%d bar %d: %.17g want %.17g "
                 "(%s err %.3e)\n", r->n, r->bar, out[r->bar - begIdx], r->value,
                 mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (2b) Achelis, Technical Analysis from A to Z, p. 132, as transcribed in
 * Tulip Indicators tests/atoz.txt ("emv", "#page 132"; TulipCharts/
 * tulipindicators, fetched 2026-09-28). Period 1, D = 10,000, range in
 * points. The book prints 4 decimals, so abs 5e-5 is half a unit in the last
 * place; the smallest value is 0.0014, so a sign flip, a wrong divisor or a
 * bar shift each misses by far more.
 */
static ErrorNumber test_emv_achelis( void )
{
   static const double h[18] = { 23.75,23.75,23.75,25.0625,26.25,26.875,27,26.875,28,
                                 28.1875,27.6875,27.1875,26.25,26.5,26,25.875,25.375,25.5 };
   static const double l[18] = { 23,23.1875,23.25,23.5,25,25.8125,25.875,25.75,26.3125,
                                 27.375,27.125,26,25.1875,25.375,24.875,24.3125,24.25,24.75 };
   static const double v[18] = { 125733,83819,111390,211366,240664,219933,155943,138913,
                                 226220,164528,132053,109900,138313,143421,106053,141425,
                                 96921,93208 };
   static const double want[17] = { 0.0063,0.0014,0.0578,0.0698,0.0347,0.0068,-0.0101,
                                    0.0629,0.0309,-0.0160,-0.0878,-0.0672,0.0172,
                                    -0.0530,-0.0380,-0.0326,0.0251 };
   double out[18];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_EMV( 0, 17, h, l, v, 1, 10000.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 17 )
   {
      printf( "Fail: TA_EMV Achelis: rc=%d beg=%d nb=%d\n", (int)retCode,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < 17; i++ )
   {
      if( fabs( out[i] - want[i] ) > 5e-5 )
      {
         printf( "Fail: TA_EMV Achelis bar %d: %.17g want %.4f\n", i+1, out[i], want[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( server_verify_active() )
   {
      static const double optIn[2] = { 1.0, 10000.0 };
      return server_verify( "EMV", 0, 17, 18, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ h, l, v, NULL }, optIn, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

/* (2c) QuantConnect LEAN Tests/TestData/spy_emv.txt at
 * 5b0c9975d3189efe4353e5071374b77ab9681bdb (EaseOfMovementValue, period 1,
 * scale 10,000). Printed at 9 decimals, so abs 5e-10 is half a unit in the
 * last place. The values span 1.5e-6 to 3.8e-4: a relative miss of 1e-3 on
 * the largest already fails.
 */
static ErrorNumber test_emv_lean( void )
{
   static const double h[30] = {
      63.74,64.51,64.57,64.31,63.43,62.85,62.7,63.18,62.47,64.16,
      64.38,64.89,65.25,64.69,64.26,64.51,63.46,62.69,63.52,63.52,
      63.74,64.58,65.31,65.1,63.68,63.66,62.95,63.73,64.99,65.31 };
   static const double l[30] = {
      62.63,63.85,63.81,62.62,62.73,61.95,62.06,62.69,61.54,63.21,
      63.87,64.29,64.48,63.65,63.68,63.12,62.5,61.86,62.56,62.95,
      62.63,63.39,64.72,64.21,62.51,62.55,62.15,62.97,63.64,64.6 };
   static const double v[30] = {
      32178836,36461672,51372680,42476356,29504176,33098600,30577960,35693928,
      49768136,44759968,33425504,15895085,37015388,40672116,35627200,47337336,
      43373576,57651752,32357184,27620876,42467704,44460240,52992592,40561552,
      48636228,57230032,46260856,41926492,42620976,37809176 };
   static const double want[29] = {
       0.000180107, 0.000001479,-0.000288455,-0.000091343,-0.000184902,
      -0.000004186, 0.000076189,-0.000173786, 0.000356569, 0.000067134,
       0.000175526, 0.000057206,-0.000177714,-0.000032559,-0.000045514,
      -0.000184813,-0.000101497, 0.000226967, 0.000040241,-0.000013069,
       0.000214124, 0.000114676,-0.000078991,-0.000375276, 0.000001940,
      -0.000095977, 0.000145016, 0.000305659, 0.000120182 };
   double out[30];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_EMV( 0, 29, h, l, v, 1, 10000.0, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 29 )
   {
      printf( "Fail: TA_EMV LEAN: rc=%d beg=%d nb=%d\n", (int)retCode,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < 29; i++ )
   {
      if( fabs( out[i] - want[i] ) > 5e-10 )
      {
         printf( "Fail: TA_EMV LEAN bar %d: %.17g want %.9f\n", i+1, out[i], want[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( server_verify_active() )
   {
      static const double optIn[2] = { 1.0, 10000.0 };
      return server_verify( "EMV", 0, 29, 30, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ h, l, v, NULL }, optIn, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

/* (3) Degenerate bars on the reference corpus: bar 100 flat (high = low =
 * close), bar 150 at volume 0, bar 200 both. A guarded bar answers exactly
 * +0.0 and the bar after it is measured from the guarded bar's own midpoint.
 * No other leg checks these cases against an independent reference: the
 * fuzz shapes and variant regimes that reach them only compare implementations
 * with each other.
 *
 * emvTsDegenerate[k] is bar k+1 of trading-signals 8.3.0 `new EMV(1, 10000)`
 * run on that series (node v22.21.1, 2026-09-28), as IEEE-754 bits. At n = 1
 * the batch must equal it bitwise; at n = 14, TA_SMA over it. Its own n = 14
 * re-sums each window, so it is not the reference there.
 */
static const unsigned long long emvTsDegenerate[251] = {
   0x3f8120cc72746b6dULL,0x3f8380c64fa70d38ULL,0xbf68cae584c54368ULL,0xbf68da42e4e27183ULL,
   0xbf50603994a55d1aULL,0xbf762880971c9b16ULL,0xbf8cf7595be05cdaULL,0xbf75831f03d145e4ULL,
   0x3f72c40b5cda6c63ULL,0x3f97f96c979be6cbULL,0x3f929b09ee2694d5ULL,0xbf5381851eecef79ULL,
   0xbf8cc1317d1b734fULL,0xbf7bfc911ac847dfULL,0x3f7e54e07be8d0baULL,0xbf736c570c6dea57ULL,
   0xbf727d5b55f594dbULL,0x3f73a4647600ac98ULL,0x3f658ded6e17e034ULL,0xbf84a3008500afe8ULL,
   0xbf74451f5a038036ULL,0xbf7b870b9fc852eeULL,0xbf83cd517b5af62eULL,0xbf716ebfb5aa993dULL,
   0xbf564d5198eeff71ULL,0x3f2905f55d23af08ULL,0x3fa0ece5cc967be5ULL,0x3f41c2ae452e7d59ULL,
   0x3f73aceeb9b788c5ULL,0xbf86277b97d44a29ULL,0x3f5fe2f325a4cbf9ULL,0x3f560018cb9f013dULL,
   0x3f78e2f642e418deULL,0x3f801d7b05dd59f2ULL,0xbf76c2e09b00c72dULL,0xbf85a853ab2deaf9ULL,
   0xbf721e319a602714ULL,0xbf717c5f43856ba9ULL,0x3f6e54349ebab7cbULL,0xbf7398e8190cd2c7ULL,
   0x3f857ab518c6c75fULL,0x3f7973c18b3d39b9ULL,0x3f716084d8b570b1ULL,0x3f84956f6b082f50ULL,
   0xbf330a7127e50e7bULL,0x3f76cac25128de96ULL,0xbf8141560f318788ULL,0xbf6ad7a550fb0336ULL,
   0x3f75cb7e7262acefULL,0xbf70d47d87bc1697ULL,0xbf78d1523d3a93beULL,0xbf789ee5315eb752ULL,
   0xbf7d8607c1a76170ULL,0xbf74657c97230bd4ULL,0x3f729b30bba7ec79ULL,0x3f7674030fba9b7aULL,
   0x3f784e9b87b61975ULL,0x3f8275e03d26ed85ULL,0x3f7df32f13b0b0e9ULL,0xbf3239483ce8ce47ULL,
   0xbf803c65ffd88e62ULL,0x3f8e23e9bf2b5dd3ULL,0x3f8131cd9dcf6316ULL,0xbf70edf76555fd24ULL,
   0x3f79616869a5ebc8ULL,0x3f66f78f3b4140ceULL,0xbf7592759e6087c1ULL,0xbf5235302e298731ULL,
   0xbf756ab148f62849ULL,0xbf8f5c4827a6eafdULL,0xbf80fad5386b0720ULL,0xbf8ee9d9a9f4ecceULL,
   0xbf70fe13c1bb8232ULL,0x3f765ecb6de21a4cULL,0x3f958df22fffe688ULL,0x3f8b13ea8e1873b1ULL,
   0x3f8f62ec347f2a86ULL,0x3f81a5d7257cfca1ULL,0xbf8ecea53098ad68ULL,0xbf5f77bd0fe8ba98ULL,
   0x3f7121e40ed224c9ULL,0x3f7bb976c6ca373eULL,0x3f81fe97f1c515a5ULL,0xbf8758cbc7dfc2c7ULL,
   0x3f701cf091498533ULL,0x3f85a687b5e2d8e0ULL,0x3f8ad071fe93cf6fULL,0x3f42d6f2f753788dULL,
   0x3f8907602b124448ULL,0x3fa6c211884d6673ULL,0xbf40fd59805faf89ULL,0xbf8a9825387c35c7ULL,
   0x3f79cefd64a09620ULL,0xbf85be18f48312e0ULL,0xbf7211902cf865adULL,0xbf83965303267f69ULL,
   0xbf9aee86d3e3fd5bULL,0xbf7d9b27816f1d80ULL,0x3f99e1997a9527adULL,0x0000000000000000ULL,
   0xbf61688be9ea8b8dULL,0xbf8686da708409ebULL,0xbf6e46d3655f6cfcULL,0x3f85196c9ec350adULL,
   0x3f75befc9c29ae89ULL,0x3fa14c3bfc63dd23ULL,0xbf8130eeac6a27dbULL,0x3f77bfdc256ef342ULL,
   0xbf9ac062be5e4480ULL,0xbf72c61098000c30ULL,0x3f5f6d6516babc38ULL,0x3f86d29bd6fea7c3ULL,
   0x3f8c4418e83f707aULL,0x3f608b5b9b554dbcULL,0x3f26f5f9977641c3ULL,0x3f91c8e5b4676bd0ULL,
   0x3f810b46fafe9ce1ULL,0xbf8b2ab70d302539ULL,0x3f25a5346bf47b49ULL,0x3f80ede87f986b5bULL,
   0xbf70573d07076c69ULL,0x3f67ad3af9bf2a85ULL,0x3fa3a5d0231121f1ULL,0x3f80daa2a312cda6ULL,
   0x3f83610a2a833c0dULL,0x3f802c72d32ce2eeULL,0xbf53c983b6650628ULL,0x3f83d1686fef6652ULL,
   0x3f8c076bfca246c4ULL,0x3f82745511dc8541ULL,0x3f65b3611e72b60dULL,0xbf4404f625bb38adULL,
   0xbf7b6afa938f77f1ULL,0x3f40f1b1837a0bb5ULL,0xbf51460b35d5ae00ULL,0xbf9f3a185cae6149ULL,
   0xbf650fb203d8786bULL,0xbf8aa7be51d2d31eULL,0xbf8054d9155a9eb5ULL,0xbf3ab449612d0a33ULL,
   0x3f743d0010c70cafULL,0x3f88be5fed7de671ULL,0xbf7760d58f46e8efULL,0xbf3f9e8f578e724aULL,
   0xbf8e86ae2240bd5cULL,0xbf97eb7538ab80baULL,0xbf72b3d82e5db758ULL,0x3f7d2e8d6db0e628ULL,
   0x3fa019886d4c5836ULL,0x0000000000000000ULL,0xbf89acd8e7b99d2eULL,0x3f7eda1fbd8859abULL,
   0x3f6c971c7fab5935ULL,0x3f73a9ab821c9922ULL,0x3f94b1ea9003b4c3ULL,0x3f75ea15ea15ea16ULL,
   0xbf82e52fe513f087ULL,0xbf9120ffb110ab53ULL,0xbf5f6035089266f9ULL,0x3f6e5a710ddc1527ULL,
   0xbf7044d8975e9aaeULL,0x3f490ffe4ca78e34ULL,0x3f82bb64d1546bccULL,0x3efd5c181d3f19d8ULL,
   0x3f52999a5050f56aULL,0x3f5718bc143d9204ULL,0x3f8925a063372270ULL,0xbf7b80d3398967f6ULL,
   0x3f86e3bbb1cfe542ULL,0x3f836558ac892373ULL,0x3f806ef5251e03fbULL,0x3f84b7ae206daee0ULL,
   0x3f8a369e6b785548ULL,0xbf927fe390a91eacULL,0xbf6dc5f00e7fd8c1ULL,0x3f6fe5a3a2c81caeULL,
   0xbf87b142affcd73cULL,0xbf91e1cd21a3e6dfULL,0x3f76630db757296fULL,0xbf63d18fa7d02760ULL,
   0xbf7f0c1593e880a8ULL,0xbf845e2dc9aba7afULL,0xbf87e033af6c8cd0ULL,0x3f859081572e047aULL,
   0xbf6eb19180bd3663ULL,0xbf77cbd43c9ea83eULL,0xbf5e7b848ce3ac9dULL,0xbf8c9e4de689b96fULL,
   0x3f3cbaaab98dfd95ULL,0x3f9286474f96103cULL,0xbf52c1ec45326928ULL,0xbf8673cd3f153f9eULL,
   0xbf82a1e87a360559ULL,0x3f8139ba311c5fbcULL,0xbf8dc2cae5ac0badULL,0xbf9202e25eb086b2ULL,
   0xbf6d0dfb7754b1f3ULL,0x3f5789d29aa2d4f1ULL,0x3f6151f35efd6c06ULL,0x0000000000000000ULL,
   0x3f8fbc147651b52eULL,0xbf8672a19ab98919ULL,0x3f60ab03aae9b8ebULL,0x3f2122b5c0d3d153ULL,
   0x3f60cd0bdf26cf75ULL,0xbf69ac0e0b60f278ULL,0x3f45de52aa3205a9ULL,0x3f7ce9f1a345821aULL,
   0xbf176f63c2e5d892ULL,0xbf7aeb94b1f59bdaULL,0xbf546c6d1cfbfe2aULL,0xbf7b38c8be13107cULL,
   0xbf5579c6e4e94de3ULL,0x3f65a7c9a6e4d590ULL,0x3f770b9e7b5a4f12ULL,0x3f78b188cd7a6288ULL,
   0x3f61ecbd2020b5a4ULL,0xbf7577f153e5ac30ULL,0x3f602e9e16069039ULL,0xbf657bb238275281ULL,
   0xbf467e82ec0abb16ULL,0x3f762a2ab7dddd4bULL,0x3f94820b92a3c122ULL,0x3f8c1c25ba168d8bULL,
   0x3f7636c2df25c5faULL,0xbf8e7a1f3c388425ULL,0x3f8478045833de5bULL,0xbf647c2bfb5dc119ULL,
   0xbf6c959e3df8b94aULL,0x3f22d0616b9d43bfULL,0x3f7cf0b4058ffe0cULL,0x3f93230ac05d5be2ULL,
   0x3f90e2e1ccb116d6ULL,0x3f83150c4daf43ceULL,0x3f79dc23aa732684ULL,0xbf79de05f44219ecULL,
   0xbf9a7a926e7ac6ddULL,0xbf4afeb11bb33121ULL,0xbf748e64d208b3d7ULL,0xbf813c465459cb36ULL,
   0x3f80fdaf87b64946ULL,0x3f7b960b6743a298ULL,0xbf70f5e98ee8ca64ULL,0xbef69dbaaf3b015aULL,
   0x3f388bec3e4f7e29ULL,0x3f4a05265a41dc6dULL,0xbf54fec11aee4009ULL,0x3f6ca3a9dc474020ULL,
   0xbf5de5d6e3f886adULL,0xbf4190dc265e167eULL,0xbf8e348a4d603dcdULL
};
static ErrorNumber emv_stream_check( const double *h, const double *l, const double *v,
                                     int nb, int n, const double *batch )
{
   TA_EMV_Stream *stream = NULL;
   static double fill[EMV_MAX_BARS];
   TA_Integer sBeg, sNb;
   double got, peek;
   TA_RetCode retCode;
   int i;

   retCode = TA_EMV_OpenAndFill( &stream, h, l, v, nb, n, 10000.0, &sBeg, &sNb, fill );
   if( retCode != TA_SUCCESS || sBeg != n || sNb != nb-n
       || memcmp( fill, batch, (size_t)sNb*sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_EMV_OpenAndFill degenerate n=%d: rc=%d beg=%d nb=%d\n",
              n, (int)retCode, (int)sBeg, (int)sNb );
      if( stream ) TA_EMV_Close( stream );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   TA_EMV_Close( stream );

   /* Opened on the first n+1 bars, so every guarded bar goes through Peek
    * and Update rather than the batch loop.
    */
   stream = NULL;
   retCode = TA_EMV_Open( &stream, h, l, v, n+1, n, 10000.0, &got );
   if( retCode != TA_SUCCESS || memcmp( &got, &batch[0], sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_EMV_Open degenerate n=%d: rc=%d %.17g vs %.17g\n",
              n, (int)retCode, got, batch[0] );
      if( stream ) TA_EMV_Close( stream );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = n+1; i < nb; i++ )
   {
      if( TA_EMV_Peek( stream, h[i], l[i], v[i], &peek ) != TA_SUCCESS
          || TA_EMV_Update( stream, h[i], l[i], v[i], &got ) != TA_SUCCESS
          || memcmp( &peek, &got, sizeof(double) ) != 0
          || memcmp( &got, &batch[i-n], sizeof(double) ) != 0 )
      {
         printf( "Fail: TA_EMV stream degenerate n=%d bar %d: peek %.17g update "
                 "%.17g batch %.17g\n", n, i, peek, got, batch[i-n] );
         TA_EMV_Close( stream );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   TA_EMV_Close( stream );
   return TA_TEST_PASS;
}

static ErrorNumber test_emv_degenerate( const TA_History *history )
{
   static const int guard[3] = { 100, 150, 200 };
   static const int nGrid[2] = { 1, 14 };
   static double h[EMV_MAX_BARS], l[EMV_MAX_BARS], v[EMV_MAX_BARS];
   static double out[EMV_MAX_BARS], ref[EMV_MAX_BARS], tsRaw[EMV_MAX_BARS];
   const double posZero = 0.0;
   int nb = (int)history->nbBars;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   TA_RetCode retCode;
   int a, g, j;
   ErrorNumber e;

   if( nb != 252 )
   {
      printf( "Fail: EMV degenerate leg expects the 252-bar reference corpus\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( j = 0; j < nb-1; j++ )
      memcpy( &tsRaw[j], &emvTsDegenerate[j], sizeof(double) );

   memcpy( h, history->high,   (size_t)nb*sizeof(double) );
   memcpy( l, history->low,    (size_t)nb*sizeof(double) );
   memcpy( v, history->volume, (size_t)nb*sizeof(double) );
   h[100] = l[100] = history->close[100];
   v[150] = 0.0;
   h[200] = l[200] = history->close[200];
   v[200] = 0.0;

   for( a = 0; a < 2; a++ )
   {
      int n = nGrid[a];

      retCode = TA_EMV( 0, nb-1, h, l, v, n, 10000.0, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n || nbElement != nb-n )
      {
         printf( "Fail: TA_EMV degenerate n=%d: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( TA_SMA( 0, nb-2, tsRaw, n, &refBeg, &refNb, ref ) != TA_SUCCESS
          || refBeg != n-1 || refNb != nbElement )
      {
         printf( "Fail: EMV degenerate reference SMA n=%d failed\n", n );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( j = 0; j < (int)nbElement; j++ )
      {
         if( !TA_IS_FINITE( out[j] ) || memcmp( &out[j], &ref[j], sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_EMV degenerate n=%d bar %d: %.17g, trading-signals "
                    "reference %.17g\n", n, n+j, out[j], ref[j] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      if( n == 1 )
      {
         for( g = 0; g < 3; g++ )
         {
            if( memcmp( &out[guard[g]-1], &posZero, sizeof(double) ) != 0 )
            {
               printf( "Fail: TA_EMV degenerate bar %d: %.17g, want exactly +0.0\n",
                       guard[g], out[guard[g]-1] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }

      if( (e = emv_stream_check( h, l, v, nb, n, out )) != TA_TEST_PASS )
         return e;

      if( server_verify_active() )
      {
         double optIn[2];
         optIn[0] = (double)n;
         optIn[1] = 10000.0;
         e = server_verify( "EMV", 0, nb-1, nb, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ h, l, v, NULL }, optIn, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* A nonzero range whose box ratio underflows to 0 must take the guard, not
 * divide by it. Bar 1: volume 1e-300 over D 1e20 is subnormal, and dividing
 * that by a range of 2e10 rounds to 0.
 */
static ErrorNumber test_emv_underflow( void )
{
   static const double h[3] = { 1.0, 2e10, 3e10 };
   static const double l[3] = { 0.0, 0.0,  1e10 };
   static const double v[3] = { 1.0, 1e-300, 1e40 };
   const double posZero = 0.0;
   double out[3];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;

   retCode = TA_EMV( 0, 2, h, l, v, 1, 1e20, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 || nbElement != 2 )
   {
      printf( "Fail: TA_EMV underflow: rc=%d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( i = 0; i < 2; i++ )
   {
      if( !TA_IS_FINITE( out[i] ) )
      {
         printf( "Fail: TA_EMV underflow bar %d: %.17g\n", i+1, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   if( memcmp( &out[0], &posZero, sizeof(double) ) != 0 )
   {
      printf( "Fail: TA_EMV underflow bar 1: %.17g, want exactly +0.0\n", out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( server_verify_active() )
   {
      static const double optIn[2] = { 1.0, 1e20 };
      return server_verify( "EMV", 0, 2, 3, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ h, l, v, NULL }, optIn, 2,
                            (const TA_Real*[]){ out, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

/* (4a) Lookback is n for every divisor, and a call started at n emits there. */
static ErrorNumber test_emv_lookback( const TA_History *history )
{
   static const int    nGrid[] = { 1, 2, 14, 100000 };
   static const double dGrid[] = { 1.0, 10000.0, 1e8, TA_REAL_MAX };
   static double out[EMV_MAX_BARS];
   TA_Integer begIdx, nbElement;
   int a, c, n;

   for( a = 0; a < (int)(sizeof(nGrid)/sizeof(nGrid[0])); a++ )
   for( c = 0; c < (int)(sizeof(dGrid)/sizeof(dGrid[0])); c++ )
   {
      if( TA_EMV_Lookback( nGrid[a], dGrid[c] ) != nGrid[a] )
      {
         printf( "Fail: TA_EMV_Lookback(%d, %g) = %d\n", nGrid[a], dGrid[c],
                 TA_EMV_Lookback( nGrid[a], dGrid[c] ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( a = 0; a < 2; a++ )
   {
      n = a == 0 ? 1 : 14;
      if( TA_EMV( n, n, history->high, history->low, history->volume, n, 10000.0,
                  &begIdx, &nbElement, out ) != TA_SUCCESS
          || begIdx != n || nbElement != 1 )
      {
         printf( "Fail: TA_EMV at startIdx = n = %d: beg=%d nb=%d\n", n,
                 (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( TA_EMV( n-1, n-1, history->high, history->low, history->volume, n, 10000.0,
                  &begIdx, &nbElement, out ) != TA_SUCCESS || nbElement != 0 )
      {
         printf( "Fail: TA_EMV at startIdx = endIdx = n-1 = %d emitted %d\n", n-1,
                 (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4b) outReal over each input in turn, at n = 1 and n = 14. */
static ErrorNumber test_emv_aliasing( const TA_History *history )
{
   static double ref[EMV_MAX_BARS], buf[EMV_MAX_BARS];
   int nb = (int)history->nbBars;
   TA_Integer begIdx, nbElement, refBeg, refNb;
   int which, a, n;

   for( a = 0; a < 2; a++ )
   {
      n = a == 0 ? 1 : 14;
      if( TA_EMV( 0, nb-1, history->high, history->low, history->volume, n, 10000.0,
                  &refBeg, &refNb, ref ) != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;

      for( which = 0; which < 3; which++ )
      {
         const double *h = history->high;
         const double *l = history->low;
         const double *v = history->volume;

         switch( which )
         {
         case 0: memcpy( buf, h, (size_t)nb*sizeof(double) ); h = buf; break;
         case 1: memcpy( buf, l, (size_t)nb*sizeof(double) ); l = buf; break;
         default:memcpy( buf, v, (size_t)nb*sizeof(double) ); v = buf; break;
         }

         if( TA_EMV( 0, nb-1, h, l, v, n, 10000.0, &begIdx, &nbElement, buf ) != TA_SUCCESS
             || begIdx != refBeg || nbElement != refNb
             || memcmp( buf, ref, (size_t)refNb*sizeof(double) ) != 0 )
         {
            printf( "Fail: TA_EMV aliasing input %d n=%d\n", which, n );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4c) A running sum, so a sub-range differs from the full run's slice by
 * reassociation only: CMF's class.
 */
typedef struct
{
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *volume;
   int period;
} EmvRangeParam;

static TA_RetCode emvRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   EmvRangeParam *p = (EmvRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_EMV_Lookback( p->period, 10000.0 );
   return TA_EMV( startIdx, endIdx, p->high, p->low, p->volume, p->period, 10000.0,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_emv_range( const TA_History *history )
{
   static const int nGrid[] = { 1, 14, 50 };
   EmvRangeParam param;
   ErrorNumber e;
   int a;

   param.high   = history->high;
   param.low    = history->low;
   param.volume = history->volume;

   for( a = 0; a < (int)(sizeof(nGrid)/sizeof(nGrid[0])); a++ )
   {
      param.period = nGrid[a];
      e = doRangeTestEx( emvRangeTestFunction, TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
      if( e != TA_TEST_PASS )
         return e;
   }

   return TA_TEST_PASS;
}
