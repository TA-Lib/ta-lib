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


/* GENERATED FILE -- do not edit. Produced by ta_codegen
 * (generator/src/backends/c_examples.rs) from ta_codegen/input/.
 * MF,CC
 *
 * Every function page's C sample, run on the synthetic series: the call
 * answers a non-empty range whose values are finite.
 */

#include <math.h>
#include <stdio.h>
#include <ta_libc.h>

static int failures = 0;

static int check( const char *name, int ok )
{
   if( !ok )
   {
      failures++;
      printf( "  FAIL: %s example\n", name );
   }
   return ok;
}

static int all_finite( const double *a, int n )
{
   int i;
   for( i = 0; i < n; i++ )
      if( !isfinite( a[i] ) )
         return 0;
   return 1;
}

static int no_nan( const double *a, int n )
{
   int i;
   for( i = 0; i < n; i++ )
      if( isnan( a[i] ) )
         return 0;
   return 1;
}

static int example_AC( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AC( 0, n - 1, high, low, 5, 34, 5, &outBegIdx, &outNBElement, out );
   if( !check( "AC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ACCBANDS( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double upperBand[252], middleBand[252], lowerBand[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ACCBANDS( 0, n - 1, high, low, close, 20,
                &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
   if( !check( "ACCBANDS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
   if( !check( "ACCBANDS", all_finite( upperBand, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ACOS( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -1 + 2 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ACOS( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "ACOS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ACOS", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AD( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AD( 0, n - 1, high, low, close, volume, &outBegIdx, &outNBElement, out );
   if( !check( "AD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AD", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ADD( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ADD( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
   if( !check( "ADD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ADD", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ADOSC( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ADOSC( 0, n - 1, high, low, close, volume, 3, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ADOSC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ADOSC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ADR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ADR( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
   if( !check( "ADR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ADR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ADX( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ADX( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "ADX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ADX", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ADXR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ADXR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "ADXR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ADXR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ALLIGATOR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double jaw[252], teeth[252], lips[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ALLIGATOR( 0, n - 1, high, low, 13, 8, 8, 5, 5, 3, &outBegIdx, &outNBElement, jaw, teeth, lips );
   if( !check( "ALLIGATOR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last jaw = %f\n", outNBElement, outBegIdx, jaw[outNBElement - 1] );
   if( !check( "ALLIGATOR", all_finite( jaw, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ALMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ALMA( 0, n - 1, series, 9, 6.0, 0.85, &outBegIdx, &outNBElement, out );
   if( !check( "ALMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ALMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AO( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AO( 0, n - 1, high, low, 5, 34, &outBegIdx, &outNBElement, out );
   if( !check( "AO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_APO( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_APO( 0, n - 1, series, 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, out );
   if( !check( "APO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "APO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AROON( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double aroonDown[252], aroonUp[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AROON( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, aroonDown, aroonUp );
   if( !check( "AROON", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last aroonDown = %f\n", outNBElement, outBegIdx, aroonDown[outNBElement - 1] );
   if( !check( "AROON", all_finite( aroonDown, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AROONOSC( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AROONOSC( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
   if( !check( "AROONOSC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AROONOSC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ASI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ASI( 0, n - 1, open, high, low, close, 3.0, &outBegIdx, &outNBElement, out );
   if( !check( "ASI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ASI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ASIN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -1 + 2 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ASIN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "ASIN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ASIN", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ATAN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ATAN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "ATAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ATAN", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ATR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ATR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "ATR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ATR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AVGDEV( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AVGDEV( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "AVGDEV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AVGDEV", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_AVGPRICE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_AVGPRICE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "AVGPRICE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "AVGPRICE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_BBANDS( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double upperBand[252], middleBand[252], lowerBand[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_BBANDS( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA,
              &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
   if( !check( "BBANDS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
   if( !check( "BBANDS", all_finite( upperBand, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_BBW( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_BBW( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
   if( !check( "BBW", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "BBW", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_BETA( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 100 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = 100 + 10 * sin(0.1 * i + 0.7); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_BETA( 0, n - 1, series0, series1, 5, &outBegIdx, &outNBElement, out );
   if( !check( "BETA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "BETA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_BOP( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_BOP( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "BOP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "BOP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CCI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CCI( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "CCI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CCI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CDL2CROWS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL2CROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL2CROWS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3BLACKCROWS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3BLACKCROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3BLACKCROWS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3INSIDE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3INSIDE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3INSIDE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3LINESTRIKE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3LINESTRIKE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3LINESTRIKE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3OUTSIDE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3OUTSIDE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3OUTSIDE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3STARSINSOUTH( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3STARSINSOUTH( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3STARSINSOUTH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDL3WHITESOLDIERS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDL3WHITESOLDIERS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDL3WHITESOLDIERS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLABANDONEDBABY( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLABANDONEDBABY( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
   if( !check( "CDLABANDONEDBABY", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLADVANCEBLOCK( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLADVANCEBLOCK( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLADVANCEBLOCK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLBELTHOLD( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLBELTHOLD( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLBELTHOLD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLBREAKAWAY( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLBREAKAWAY( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLBREAKAWAY", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLCLOSINGMARUBOZU( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLCLOSINGMARUBOZU( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLCLOSINGMARUBOZU", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLCONCEALBABYSWALL( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLCONCEALBABYSWALL( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLCONCEALBABYSWALL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLCOUNTERATTACK( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLCOUNTERATTACK( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLCOUNTERATTACK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLDARKCLOUDCOVER( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLDARKCLOUDCOVER( 0, n - 1, open, high, low, close, 0.5, &outBegIdx, &outNBElement, out );
   if( !check( "CDLDARKCLOUDCOVER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLDOJI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLDOJI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLDOJISTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLDOJISTAR( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLDOJISTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLDRAGONFLYDOJI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLDRAGONFLYDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLDRAGONFLYDOJI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLENGULFING( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLENGULFING( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLENGULFING", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLEVENINGDOJISTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLEVENINGDOJISTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
   if( !check( "CDLEVENINGDOJISTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLEVENINGSTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLEVENINGSTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
   if( !check( "CDLEVENINGSTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLGAPSIDESIDEWHITE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLGAPSIDESIDEWHITE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLGAPSIDESIDEWHITE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLGRAVESTONEDOJI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLGRAVESTONEDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLGRAVESTONEDOJI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHAMMER( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHAMMER( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHAMMER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHANGINGMAN( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHANGINGMAN( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHANGINGMAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHARAMI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHARAMI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHARAMI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHARAMICROSS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHARAMICROSS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHARAMICROSS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHIGHWAVE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHIGHWAVE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHIGHWAVE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHIKKAKE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHIKKAKE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHIKKAKE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHIKKAKEMOD( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHIKKAKEMOD( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHIKKAKEMOD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLHOMINGPIGEON( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLHOMINGPIGEON( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLHOMINGPIGEON", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLIDENTICAL3CROWS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLIDENTICAL3CROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLIDENTICAL3CROWS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLINNECK( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLINNECK( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLINNECK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLINVERTEDHAMMER( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLINVERTEDHAMMER( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLINVERTEDHAMMER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLKICKING( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLKICKING( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLKICKING", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLKICKINGBYLENGTH( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLKICKINGBYLENGTH( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLKICKINGBYLENGTH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLLADDERBOTTOM( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLLADDERBOTTOM( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLLADDERBOTTOM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLLONGLEGGEDDOJI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLLONGLEGGEDDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLLONGLEGGEDDOJI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLLONGLINE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLLONGLINE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLLONGLINE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLMARUBOZU( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLMARUBOZU( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLMARUBOZU", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLMATCHINGLOW( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLMATCHINGLOW( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLMATCHINGLOW", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLMATHOLD( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLMATHOLD( 0, n - 1, open, high, low, close, 0.5, &outBegIdx, &outNBElement, out );
   if( !check( "CDLMATHOLD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLMORNINGDOJISTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLMORNINGDOJISTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
   if( !check( "CDLMORNINGDOJISTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLMORNINGSTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLMORNINGSTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
   if( !check( "CDLMORNINGSTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLONNECK( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLONNECK( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLONNECK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLPIERCING( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLPIERCING( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLPIERCING", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLRICKSHAWMAN( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLRICKSHAWMAN( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLRICKSHAWMAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLRISEFALL3METHODS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLRISEFALL3METHODS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLRISEFALL3METHODS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSEPARATINGLINES( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSEPARATINGLINES( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSEPARATINGLINES", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSHOOTINGSTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSHOOTINGSTAR( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSHOOTINGSTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSHORTLINE( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSHORTLINE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSHORTLINE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSPINNINGTOP( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSPINNINGTOP( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSPINNINGTOP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSTALLEDPATTERN( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSTALLEDPATTERN( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSTALLEDPATTERN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLSTICKSANDWICH( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLSTICKSANDWICH( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLSTICKSANDWICH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLTAKURI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLTAKURI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLTAKURI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLTASUKIGAP( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLTASUKIGAP( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLTASUKIGAP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLTHRUSTING( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLTHRUSTING( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLTHRUSTING", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLTRISTAR( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLTRISTAR( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLTRISTAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLUNIQUE3RIVER( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLUNIQUE3RIVER( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLUNIQUE3RIVER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLUPSIDEGAP2CROWS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLUPSIDEGAP2CROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLUPSIDEGAP2CROWS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CDLXSIDEGAP3METHODS( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CDLXSIDEGAP3METHODS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "CDLXSIDEGAP3METHODS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_CE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double outLong[252], outShort[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CE( 0, n - 1, high, low, close, 22, 3.0, &outBegIdx, &outNBElement, outLong, outShort );
   if( !check( "CE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last outLong = %f\n", outNBElement, outBegIdx, outLong[outNBElement - 1] );
   if( !check( "CE", all_finite( outLong, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CEIL( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -3 + 6 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CEIL( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "CEIL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CEIL", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CG( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CG( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "CG", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CG", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CHOP( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CHOP( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "CHOP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CHOP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CHOPTR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CHOPTR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "CHOPTR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CHOPTR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CKSP( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double highStop[252], lowStop[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CKSP( 0, n - 1, high, low, close, 10, 1.0, 9, &outBegIdx, &outNBElement, highStop, lowStop );
   if( !check( "CKSP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last highStop = %f\n", outNBElement, outBegIdx, highStop[outNBElement - 1] );
   if( !check( "CKSP", all_finite( highStop, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CMF( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CMF( 0, n - 1, high, low, close, volume, 20, &outBegIdx, &outNBElement, out );
   if( !check( "CMF", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CMF", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CMO( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CMO( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "CMO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CMO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CMOU( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CMOU( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "CMOU", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CMOU", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_COPPOCK( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_COPPOCK( 0, n - 1, series, 10, 11, 14, &outBegIdx, &outNBElement, out );
   if( !check( "COPPOCK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "COPPOCK", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CORREL( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 100 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = 100 + 10 * sin(0.1 * i + 0.7); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CORREL( 0, n - 1, series0, series1, 30, &outBegIdx, &outNBElement, out );
   if( !check( "CORREL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CORREL", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_COS( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -6.3 + 12.6 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_COS( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "COS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "COS", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_COSH( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_COSH( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "COSH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "COSH", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CRSI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CRSI( 0, n - 1, series, 3, 2, 100, &outBegIdx, &outNBElement, out );
   if( !check( "CRSI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CRSI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CTI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CTI( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "CTI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CTI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CUMSUM( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CUMSUM( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "CUMSUM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CUMSUM", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_CVI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_CVI( 0, n - 1, high, low, 10, 10, &outBegIdx, &outNBElement, out );
   if( !check( "CVI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "CVI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DEMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DEMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "DEMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "DEMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DIV( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = 1 + 5 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DIV( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
   if( !check( "DIV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "DIV", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DONCHIAN( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double upperBand[252], middleBand[252], lowerBand[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DONCHIAN( 0, n - 1, high, low, 20, &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
   if( !check( "DONCHIAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
   if( !check( "DONCHIAN", all_finite( upperBand, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DOSC( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DOSC( 0, n - 1, series, 14, 5, 3, 9, &outBegIdx, &outNBElement, out );
   if( !check( "DOSC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "DOSC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DPO( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DPO( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "DPO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "DPO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_DX( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_DX( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "DX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "DX", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_EFI( void )
{
   int n = 252;
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_EFI( 0, n - 1, close, volume, 13, &outBegIdx, &outNBElement, out );
   if( !check( "EFI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "EFI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_EMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_EMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "EMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "EMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_EMV( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_EMV( 0, n - 1, high, low, volume, 14, 10000.0, &outBegIdx, &outNBElement, out );
   if( !check( "EMV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "EMV", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ER( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ER( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ER", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ERI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double bullPower[252], bearPower[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ERI( 0, n - 1, high, low, close, 13, &outBegIdx, &outNBElement, bullPower, bearPower );
   if( !check( "ERI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last bullPower = %f\n", outNBElement, outBegIdx, bullPower[outNBElement - 1] );
   if( !check( "ERI", all_finite( bullPower, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_EXP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_EXP( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "EXP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "EXP", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_FISHER( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double fisher[252], trigger[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_FISHER( 0, n - 1, high, low, 10, &outBegIdx, &outNBElement, fisher, trigger );
   if( !check( "FISHER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last fisher = %f\n", outNBElement, outBegIdx, fisher[outNBElement - 1] );
   if( !check( "FISHER", all_finite( fisher, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_FLOOR( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -3 + 6 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_FLOOR( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "FLOOR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "FLOOR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_FOSC( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_FOSC( 0, n - 1, series, 5, &outBegIdx, &outNBElement, out );
   if( !check( "FOSC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "FOSC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_FRACTAL( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   int swingHigh[252], swingLow[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_FRACTAL( 0, n - 1, high, low, 2, 2, &outBegIdx, &outNBElement, swingHigh, swingLow );
   if( !check( "FRACTAL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last swingHigh = %d\n", outNBElement, outBegIdx, swingHigh[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_FRAMA( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_FRAMA( 0, n - 1, high, low, 16, &outBegIdx, &outNBElement, out );
   if( !check( "FRAMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "FRAMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HA( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double haOpen[252], haHigh[252], haLow[252], haClose[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HA( 0, n - 1, open, high, low, close,
          &outBegIdx, &outNBElement, haOpen, haHigh, haLow, haClose );
   if( !check( "HA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last haOpen = %f\n", outNBElement, outBegIdx, haOpen[outNBElement - 1] );
   if( !check( "HA", all_finite( haOpen, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HMA( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "HMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "HMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_DCPERIOD( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_DCPERIOD( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "HT_DCPERIOD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "HT_DCPERIOD", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_DCPHASE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_DCPHASE( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "HT_DCPHASE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "HT_DCPHASE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_PHASOR( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double inPhase[252], quadrature[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_PHASOR( 0, n - 1, series, &outBegIdx, &outNBElement, inPhase, quadrature );
   if( !check( "HT_PHASOR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last inPhase = %f\n", outNBElement, outBegIdx, inPhase[outNBElement - 1] );
   if( !check( "HT_PHASOR", all_finite( inPhase, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_SINE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double sine[252], leadSine[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_SINE( 0, n - 1, series, &outBegIdx, &outNBElement, sine, leadSine );
   if( !check( "HT_SINE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last sine = %f\n", outNBElement, outBegIdx, sine[outNBElement - 1] );
   if( !check( "HT_SINE", all_finite( sine, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_TRENDLINE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_TRENDLINE( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "HT_TRENDLINE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "HT_TRENDLINE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_HT_TRENDMODE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_HT_TRENDMODE( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "HT_TRENDMODE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_IBS( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_IBS( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "IBS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "IBS", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ICHIMOKU( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double tenkanSen[252], kijunSen[252], senkouSpanA[252], senkouSpanB[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ICHIMOKU( 0, n - 1, high, low, 9, 26, 52,
                &outBegIdx, &outNBElement, tenkanSen, kijunSen, senkouSpanA, senkouSpanB );
   if( !check( "ICHIMOKU", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last tenkanSen = %f\n", outNBElement, outBegIdx, tenkanSen[outNBElement - 1] );
   if( !check( "ICHIMOKU", all_finite( tenkanSen, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_IMI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_IMI( 0, n - 1, open, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "IMI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "IMI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KAMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KAMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "KAMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "KAMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KC( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double upperBand[252], middleBand[252], lowerBand[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KC( 0, n - 1, high, low, close, 20, 10, 2.0,
          &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
   if( !check( "KC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
   if( !check( "KC", all_finite( upperBand, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KDJ( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double k[252], d[252], j[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KDJ( 0, n - 1, high, low, close, 9, 3, TA_MAType_RMA, 3, TA_MAType_RMA,
           &outBegIdx, &outNBElement, k, d, j );
   if( !check( "KDJ", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last k = %f\n", outNBElement, outBegIdx, k[outNBElement - 1] );
   if( !check( "KDJ", all_finite( k, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KST( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double kst[252], kstSignal[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KST( 0, n - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
           &outBegIdx, &outNBElement, kst, kstSignal );
   if( !check( "KST", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last kst = %f\n", outNBElement, outBegIdx, kst[outNBElement - 1] );
   if( !check( "KST", all_finite( kst, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KSTEXT( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double kst[252], kstSignal[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KSTEXT( 0, n - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9, TA_MAType_SMA, TA_MAType_SMA,
              &outBegIdx, &outNBElement, kst, kstSignal );
   if( !check( "KSTEXT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last kst = %f\n", outNBElement, outBegIdx, kst[outNBElement - 1] );
   if( !check( "KSTEXT", all_finite( kst, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KURTOSIS( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KURTOSIS( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "KURTOSIS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "KURTOSIS", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_KVO( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double kvo[252], kvoSignal[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_KVO( 0, n - 1, high, low, close, volume, 34, 55, 13, &outBegIdx, &outNBElement, kvo, kvoSignal );
   if( !check( "KVO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last kvo = %f\n", outNBElement, outBegIdx, kvo[outNBElement - 1] );
   if( !check( "KVO", all_finite( kvo, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LINEARREG( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LINEARREG( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "LINEARREG", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LINEARREG", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LINEARREG_ANGLE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LINEARREG_ANGLE( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "LINEARREG_ANGLE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LINEARREG_ANGLE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LINEARREG_INTERCEPT( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LINEARREG_INTERCEPT( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "LINEARREG_INTERCEPT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LINEARREG_INTERCEPT", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LINEARREG_SLOPE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LINEARREG_SLOPE( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "LINEARREG_SLOPE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LINEARREG_SLOPE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 0.1 + 9.9 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "LN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LN", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_LOG10( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 0.1 + 9.9 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_LOG10( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "LOG10", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "LOG10", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MA( 0, n - 1, series, 30, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
   if( !check( "MA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MACD( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double macd[252], macdSignal[252], macdHist[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MACD( 0, n - 1, series, 12, 26, 9, &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
   if( !check( "MACD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last macd = %f\n", outNBElement, outBegIdx, macd[outNBElement - 1] );
   if( !check( "MACD", all_finite( macd, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MACDEXT( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double macd[252], macdSignal[252], macdHist[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MACDEXT( 0, n - 1, series, 12, TA_MAType_SMA, 26, TA_MAType_SMA, 9, TA_MAType_SMA,
               &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
   if( !check( "MACDEXT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last macd = %f\n", outNBElement, outBegIdx, macd[outNBElement - 1] );
   if( !check( "MACDEXT", all_finite( macd, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MACDFIX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double macd[252], macdSignal[252], macdHist[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MACDFIX( 0, n - 1, series, 9, &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
   if( !check( "MACDFIX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last macd = %f\n", outNBElement, outBegIdx, macd[outNBElement - 1] );
   if( !check( "MACDFIX", all_finite( macd, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MACDV( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double macdv[252], macdvSignal[252], macdvHist[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MACDV( 0, n - 1, high, low, close, 12, 26, 9,
             &outBegIdx, &outNBElement, macdv, macdvSignal, macdvHist );
   if( !check( "MACDV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last macdv = %f\n", outNBElement, outBegIdx, macdv[outNBElement - 1] );
   if( !check( "MACDV", all_finite( macdv, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MAMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double mama[252], fama[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MAMA( 0, n - 1, series, 0.5, 0.05, &outBegIdx, &outNBElement, mama, fama );
   if( !check( "MAMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last mama = %f\n", outNBElement, outBegIdx, mama[outNBElement - 1] );
   if( !check( "MAMA", all_finite( mama, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MARKETFI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MARKETFI( 0, n - 1, high, low, volume, &outBegIdx, &outNBElement, out );
   if( !check( "MARKETFI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MARKETFI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MASSI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MASSI( 0, n - 1, high, low, 9, 25, &outBegIdx, &outNBElement, out );
   if( !check( "MASSI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MASSI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MAVP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double periods[252]; /* one period per bar */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) periods[i] = 5 + (i % 10); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MAVP( 0, n - 1, series, periods, 2, 30, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
   if( !check( "MAVP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MAVP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MAX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MAX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "MAX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MAX", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MAXINDEX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MAXINDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "MAXINDEX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_MCGD( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MCGD( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MCGD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MCGD", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MEDIAN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MEDIAN( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "MEDIAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MEDIAN", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MEDPRICE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MEDPRICE( 0, n - 1, high, low, &outBegIdx, &outNBElement, out );
   if( !check( "MEDPRICE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MEDPRICE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MFI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MFI( 0, n - 1, high, low, close, volume, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MFI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MFI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MIDPOINT( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MIDPOINT( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MIDPOINT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MIDPOINT", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MIDPRICE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MIDPRICE( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MIDPRICE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MIDPRICE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MIN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MIN( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "MIN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MIN", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MININDEX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   int out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MININDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "MININDEX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_MINMAX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double min[252], max[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MINMAX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, min, max );
   if( !check( "MINMAX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last min = %f\n", outNBElement, outBegIdx, min[outNBElement - 1] );
   if( !check( "MINMAX", all_finite( min, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MINMAXINDEX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   int minIdx[252], maxIdx[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MINMAXINDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, minIdx, maxIdx );
   if( !check( "MINMAXINDEX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last minIdx = %d\n", outNBElement, outBegIdx, minIdx[outNBElement - 1] );
   TA_Shutdown();
   return 0;
}

static int example_MINUS_DI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MINUS_DI( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MINUS_DI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MINUS_DI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MINUS_DM( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MINUS_DM( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
   if( !check( "MINUS_DM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MINUS_DM", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MOM( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MOM( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "MOM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MOM", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_MULT( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_MULT( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
   if( !check( "MULT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "MULT", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_NATR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_NATR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "NATR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "NATR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_NVI( void )
{
   int n = 252;
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_NVI( 0, n - 1, close, volume, &outBegIdx, &outNBElement, out );
   if( !check( "NVI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "NVI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_OBV( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_OBV( 0, n - 1, series, volume, &outBegIdx, &outNBElement, out );
   if( !check( "OBV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "OBV", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PERCENTB( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PERCENTB( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
   if( !check( "PERCENTB", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PERCENTB", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PERCENTILE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PERCENTILE( 0, n - 1, series, 100, 50.0, &outBegIdx, &outNBElement, out );
   if( !check( "PERCENTILE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PERCENTILE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PERCENTRANK( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PERCENTRANK( 0, n - 1, series, 100, &outBegIdx, &outNBElement, out );
   if( !check( "PERCENTRANK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PERCENTRANK", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PLUS_DI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PLUS_DI( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "PLUS_DI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PLUS_DI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PLUS_DM( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PLUS_DM( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
   if( !check( "PLUS_DM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PLUS_DM", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PPO( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PPO( 0, n - 1, series, 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, out );
   if( !check( "PPO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PPO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PSO( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PSO( 0, n - 1, high, low, close, 8, 5, &outBegIdx, &outNBElement, out );
   if( !check( "PSO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PSO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PVI( void )
{
   int n = 252;
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PVI( 0, n - 1, close, volume, &outBegIdx, &outNBElement, out );
   if( !check( "PVI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PVI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PVO( void )
{
   int n = 252;
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PVO( 0, n - 1, volume, 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, out );
   if( !check( "PVO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PVO", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_PVT( void )
{
   int n = 252;
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_PVT( 0, n - 1, close, volume, &outBegIdx, &outNBElement, out );
   if( !check( "PVT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "PVT", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_QSTICK( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_QSTICK( 0, n - 1, open, close, 10, &outBegIdx, &outNBElement, out );
   if( !check( "QSTICK", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "QSTICK", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_RMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_RMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "RMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "RMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ROC( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ROC( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ROC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ROC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ROCP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ROCP( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ROCP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ROCP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ROCR( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ROCR( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ROCR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ROCR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ROCR100( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ROCR100( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
   if( !check( "ROCR100", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ROCR100", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ROGERSSATCHELL( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ROGERSSATCHELL( 0, n - 1, open, high, low, close, 10, 252.0, &outBegIdx, &outNBElement, out );
   if( !check( "ROGERSSATCHELL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ROGERSSATCHELL", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_RSI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_RSI( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "RSI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "RSI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_RVI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_RVI( 0, n - 1, series, 14, 10, &outBegIdx, &outNBElement, out );
   if( !check( "RVI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "RVI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_RVIR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_RVIR( 0, n - 1, high, low, 14, 10, &outBegIdx, &outNBElement, out );
   if( !check( "RVIR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "RVIR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_RVOL( void )
{
   int n = 252;
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_RVOL( 0, n - 1, volume, 20, &outBegIdx, &outNBElement, out );
   if( !check( "RVOL", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "RVOL", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SAR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SAR( 0, n - 1, high, low, 0.02, 0.2, &outBegIdx, &outNBElement, out );
   if( !check( "SAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SAR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SAREXT( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SAREXT( 0, n - 1, high, low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
              &outBegIdx, &outNBElement, out );
   if( !check( "SAREXT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SAREXT", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SHARPE( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SHARPE( 0, n - 1, series, 20, 0.0, 252.0, &outBegIdx, &outNBElement, out );
   if( !check( "SHARPE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SHARPE", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SI( void )
{
   int n = 252;
   double open[252]; /* your data, oldest first */
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) open[i] = 100 + 10 * sin(0.1 * i - 0.05); }
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SI( 0, n - 1, open, high, low, close, 3.0, &outBegIdx, &outNBElement, out );
   if( !check( "SI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SIN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -6.3 + 12.6 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SIN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "SIN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SIN", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SINH( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SINH( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "SINH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SINH", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SKEW( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SKEW( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "SKEW", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SKEW", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "SMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SMI( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double smi[252], smiSignal[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SMI( 0, n - 1, high, low, close, 13, 2, 25, 9, &outBegIdx, &outNBElement, smi, smiSignal );
   if( !check( "SMI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last smi = %f\n", outNBElement, outBegIdx, smi[outNBElement - 1] );
   if( !check( "SMI", all_finite( smi, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SORTINO( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SORTINO( 0, n - 1, series, 20, 0.0, 252.0, &outBegIdx, &outNBElement, out );
   if( !check( "SORTINO", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SORTINO", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SQRT( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SQRT( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "SQRT", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SQRT", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SQZMOM( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double momentum[252];
   int squeeze[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SQZMOM( 0, n - 1, high, low, close, 20, 2.0, 20, 2.0, 1.5, 1.0,
              &outBegIdx, &outNBElement, momentum, squeeze );
   if( !check( "SQZMOM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last momentum = %f\n", outNBElement, outBegIdx, momentum[outNBElement - 1] );
   if( !check( "SQZMOM", all_finite( momentum, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_STC( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_STC( 0, n - 1, series, 23, 50, 10, &outBegIdx, &outNBElement, out );
   if( !check( "STC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "STC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_STDDEV( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_STDDEV( 0, n - 1, series, 5, 1.0, &outBegIdx, &outNBElement, out );
   if( !check( "STDDEV", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "STDDEV", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_STOCH( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double slowK[252], slowD[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_STOCH( 0, n - 1, high, low, close, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA,
             &outBegIdx, &outNBElement, slowK, slowD );
   if( !check( "STOCH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last slowK = %f\n", outNBElement, outBegIdx, slowK[outNBElement - 1] );
   if( !check( "STOCH", all_finite( slowK, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_STOCHF( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double fastK[252], fastD[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_STOCHF( 0, n - 1, high, low, close, 5, 3, TA_MAType_SMA,
              &outBegIdx, &outNBElement, fastK, fastD );
   if( !check( "STOCHF", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last fastK = %f\n", outNBElement, outBegIdx, fastK[outNBElement - 1] );
   if( !check( "STOCHF", all_finite( fastK, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_STOCHRSI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double fastK[252], fastD[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_STOCHRSI( 0, n - 1, series, 14, 5, 3, TA_MAType_SMA, &outBegIdx, &outNBElement, fastK, fastD );
   if( !check( "STOCHRSI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last fastK = %f\n", outNBElement, outBegIdx, fastK[outNBElement - 1] );
   if( !check( "STOCHRSI", all_finite( fastK, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SUB( void )
{
   int n = 252;
   double series0[252]; /* your data, oldest first */
   double series1[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series0[i] = 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) series1[i] = -5 + 10 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SUB( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
   if( !check( "SUB", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SUB", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SUM( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SUM( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "SUM", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SUM", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SUPERTREND( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double supertrend[252];
   int trend[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SUPERTREND( 0, n - 1, high, low, close, 10, 3.0, &outBegIdx, &outNBElement, supertrend, trend );
   if( !check( "SUPERTREND", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last supertrend = %f\n", outNBElement, outBegIdx, supertrend[outNBElement - 1] );
   if( !check( "SUPERTREND", all_finite( supertrend, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SWAK_2PHP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SWAK_2PHP( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "SWAK_2PHP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SWAK_2PHP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SWAK_BP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SWAK_BP( 0, n - 1, series, 20, 0.1, &outBegIdx, &outNBElement, out );
   if( !check( "SWAK_BP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SWAK_BP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SWAK_BUTTER( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SWAK_BUTTER( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "SWAK_BUTTER", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SWAK_BUTTER", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SWAK_GAUSS( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SWAK_GAUSS( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "SWAK_GAUSS", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SWAK_GAUSS", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_SWAK_HP( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_SWAK_HP( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
   if( !check( "SWAK_HP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "SWAK_HP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_T3( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_T3( 0, n - 1, series, 5, 0.7, &outBegIdx, &outNBElement, out );
   if( !check( "T3", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "T3", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TAN( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -1.2 + 2.4 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TAN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "TAN", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TAN", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TANH( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = -3 + 6 * i / 251.0; }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TANH( 0, n - 1, series, &outBegIdx, &outNBElement, out );
   if( !check( "TANH", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TANH", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TEMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TEMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "TEMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TEMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TRANGE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TRANGE( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "TRANGE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TRANGE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TRIMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TRIMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "TRIMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TRIMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TRIX( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TRIX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "TRIX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TRIX", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TSF( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TSF( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
   if( !check( "TSF", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TSF", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TSI( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TSI( 0, n - 1, series, 25, 13, &outBegIdx, &outNBElement, out );
   if( !check( "TSI", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TSI", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_TYPPRICE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_TYPPRICE( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "TYPPRICE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "TYPPRICE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ULTOSC( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ULTOSC( 0, n - 1, high, low, close, 7, 14, 28, &outBegIdx, &outNBElement, out );
   if( !check( "ULTOSC", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ULTOSC", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VAR( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VAR( 0, n - 1, series, 5, 1.0, &outBegIdx, &outNBElement, out );
   if( !check( "VAR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "VAR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VHF( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VHF( 0, n - 1, series, 28, &outBegIdx, &outNBElement, out );
   if( !check( "VHF", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "VHF", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VIDYA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VIDYA( 0, n - 1, series, 12, 9, &outBegIdx, &outNBElement, out );
   if( !check( "VIDYA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "VIDYA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VORTEX( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double plusVi[252], minusVi[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VORTEX( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, plusVi, minusVi );
   if( !check( "VORTEX", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last plusVi = %f\n", outNBElement, outBegIdx, plusVi[outNBElement - 1] );
   if( !check( "VORTEX", all_finite( plusVi, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VWAP( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VWAP( 0, n - 1, high, low, close, volume, &outBegIdx, &outNBElement, out );
   if( !check( "VWAP", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "VWAP", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_VWMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double volume[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) volume[i] = 10000 + 100 * i + 2000 * sin(0.3 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_VWMA( 0, n - 1, series, volume, 30, &outBegIdx, &outNBElement, out );
   if( !check( "VWMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "VWMA", no_nan( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_WAD( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_WAD( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "WAD", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "WAD", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_WAVETREND( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double wt1[252], wt2[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_WAVETREND( 0, n - 1, high, low, close, 10, 21, 4, &outBegIdx, &outNBElement, wt1, wt2 );
   if( !check( "WAVETREND", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last wt1 = %f\n", outNBElement, outBegIdx, wt1[outNBElement - 1] );
   if( !check( "WAVETREND", all_finite( wt1, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_WCLPRICE( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_WCLPRICE( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
   if( !check( "WCLPRICE", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "WCLPRICE", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_WILLR( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double close[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) close[i] = 100 + 10 * sin(0.1 * i) + 0.8 * sin(0.7 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_WILLR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
   if( !check( "WILLR", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "WILLR", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_WMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_WMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "WMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "WMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ZIGZAG( void )
{
   int n = 252;
   double high[252]; /* your data, oldest first */
   double low[252]; /* your data, oldest first */
   double zigZag[252];
   int trend[252], pivotIdx[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) high[i] = 101 + 10 * sin(0.1 * i); }
   { int i; for( i = 0; i < n; i++ ) low[i] = 99 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ZIGZAG( 0, n - 1, high, low, 5.0, 1, &outBegIdx, &outNBElement, zigZag, trend, pivotIdx );
   if( !check( "ZIGZAG", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last zigZag = %f\n", outNBElement, outBegIdx, zigZag[outNBElement - 1] );
   if( !check( "ZIGZAG", all_finite( zigZag, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

static int example_ZLEMA( void )
{
   int n = 252;
   double series[252]; /* your data, oldest first */
   double out[252];
   int outBegIdx, outNBElement;
   { int i; for( i = 0; i < n; i++ ) series[i] = 100 + 10 * sin(0.1 * i); }
   outBegIdx = -1; outNBElement = 0;

   TA_Initialize();
   TA_ZLEMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
   if( !check( "ZLEMA", outNBElement > 0 ) ) { TA_Shutdown(); return 1; }
   printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
   if( !check( "ZLEMA", all_finite( out, outNBElement ) ) ) { TA_Shutdown(); return 1; }
   TA_Shutdown();
   return 0;
}

int main( void )
{
   example_AC();
   example_ACCBANDS();
   example_ACOS();
   example_AD();
   example_ADD();
   example_ADOSC();
   example_ADR();
   example_ADX();
   example_ADXR();
   example_ALLIGATOR();
   example_ALMA();
   example_AO();
   example_APO();
   example_AROON();
   example_AROONOSC();
   example_ASI();
   example_ASIN();
   example_ATAN();
   example_ATR();
   example_AVGDEV();
   example_AVGPRICE();
   example_BBANDS();
   example_BBW();
   example_BETA();
   example_BOP();
   example_CCI();
   example_CDL2CROWS();
   example_CDL3BLACKCROWS();
   example_CDL3INSIDE();
   example_CDL3LINESTRIKE();
   example_CDL3OUTSIDE();
   example_CDL3STARSINSOUTH();
   example_CDL3WHITESOLDIERS();
   example_CDLABANDONEDBABY();
   example_CDLADVANCEBLOCK();
   example_CDLBELTHOLD();
   example_CDLBREAKAWAY();
   example_CDLCLOSINGMARUBOZU();
   example_CDLCONCEALBABYSWALL();
   example_CDLCOUNTERATTACK();
   example_CDLDARKCLOUDCOVER();
   example_CDLDOJI();
   example_CDLDOJISTAR();
   example_CDLDRAGONFLYDOJI();
   example_CDLENGULFING();
   example_CDLEVENINGDOJISTAR();
   example_CDLEVENINGSTAR();
   example_CDLGAPSIDESIDEWHITE();
   example_CDLGRAVESTONEDOJI();
   example_CDLHAMMER();
   example_CDLHANGINGMAN();
   example_CDLHARAMI();
   example_CDLHARAMICROSS();
   example_CDLHIGHWAVE();
   example_CDLHIKKAKE();
   example_CDLHIKKAKEMOD();
   example_CDLHOMINGPIGEON();
   example_CDLIDENTICAL3CROWS();
   example_CDLINNECK();
   example_CDLINVERTEDHAMMER();
   example_CDLKICKING();
   example_CDLKICKINGBYLENGTH();
   example_CDLLADDERBOTTOM();
   example_CDLLONGLEGGEDDOJI();
   example_CDLLONGLINE();
   example_CDLMARUBOZU();
   example_CDLMATCHINGLOW();
   example_CDLMATHOLD();
   example_CDLMORNINGDOJISTAR();
   example_CDLMORNINGSTAR();
   example_CDLONNECK();
   example_CDLPIERCING();
   example_CDLRICKSHAWMAN();
   example_CDLRISEFALL3METHODS();
   example_CDLSEPARATINGLINES();
   example_CDLSHOOTINGSTAR();
   example_CDLSHORTLINE();
   example_CDLSPINNINGTOP();
   example_CDLSTALLEDPATTERN();
   example_CDLSTICKSANDWICH();
   example_CDLTAKURI();
   example_CDLTASUKIGAP();
   example_CDLTHRUSTING();
   example_CDLTRISTAR();
   example_CDLUNIQUE3RIVER();
   example_CDLUPSIDEGAP2CROWS();
   example_CDLXSIDEGAP3METHODS();
   example_CE();
   example_CEIL();
   example_CG();
   example_CHOP();
   example_CHOPTR();
   example_CKSP();
   example_CMF();
   example_CMO();
   example_CMOU();
   example_COPPOCK();
   example_CORREL();
   example_COS();
   example_COSH();
   example_CRSI();
   example_CTI();
   example_CUMSUM();
   example_CVI();
   example_DEMA();
   example_DIV();
   example_DONCHIAN();
   example_DOSC();
   example_DPO();
   example_DX();
   example_EFI();
   example_EMA();
   example_EMV();
   example_ER();
   example_ERI();
   example_EXP();
   example_FISHER();
   example_FLOOR();
   example_FOSC();
   example_FRACTAL();
   example_FRAMA();
   example_HA();
   example_HMA();
   example_HT_DCPERIOD();
   example_HT_DCPHASE();
   example_HT_PHASOR();
   example_HT_SINE();
   example_HT_TRENDLINE();
   example_HT_TRENDMODE();
   example_IBS();
   example_ICHIMOKU();
   example_IMI();
   example_KAMA();
   example_KC();
   example_KDJ();
   example_KST();
   example_KSTEXT();
   example_KURTOSIS();
   example_KVO();
   example_LINEARREG();
   example_LINEARREG_ANGLE();
   example_LINEARREG_INTERCEPT();
   example_LINEARREG_SLOPE();
   example_LN();
   example_LOG10();
   example_MA();
   example_MACD();
   example_MACDEXT();
   example_MACDFIX();
   example_MACDV();
   example_MAMA();
   example_MARKETFI();
   example_MASSI();
   example_MAVP();
   example_MAX();
   example_MAXINDEX();
   example_MCGD();
   example_MEDIAN();
   example_MEDPRICE();
   example_MFI();
   example_MIDPOINT();
   example_MIDPRICE();
   example_MIN();
   example_MININDEX();
   example_MINMAX();
   example_MINMAXINDEX();
   example_MINUS_DI();
   example_MINUS_DM();
   example_MOM();
   example_MULT();
   example_NATR();
   example_NVI();
   example_OBV();
   example_PERCENTB();
   example_PERCENTILE();
   example_PERCENTRANK();
   example_PLUS_DI();
   example_PLUS_DM();
   example_PPO();
   example_PSO();
   example_PVI();
   example_PVO();
   example_PVT();
   example_QSTICK();
   example_RMA();
   example_ROC();
   example_ROCP();
   example_ROCR();
   example_ROCR100();
   example_ROGERSSATCHELL();
   example_RSI();
   example_RVI();
   example_RVIR();
   example_RVOL();
   example_SAR();
   example_SAREXT();
   example_SHARPE();
   example_SI();
   example_SIN();
   example_SINH();
   example_SKEW();
   example_SMA();
   example_SMI();
   example_SORTINO();
   example_SQRT();
   example_SQZMOM();
   example_STC();
   example_STDDEV();
   example_STOCH();
   example_STOCHF();
   example_STOCHRSI();
   example_SUB();
   example_SUM();
   example_SUPERTREND();
   example_SWAK_2PHP();
   example_SWAK_BP();
   example_SWAK_BUTTER();
   example_SWAK_GAUSS();
   example_SWAK_HP();
   example_T3();
   example_TAN();
   example_TANH();
   example_TEMA();
   example_TRANGE();
   example_TRIMA();
   example_TRIX();
   example_TSF();
   example_TSI();
   example_TYPPRICE();
   example_ULTOSC();
   example_VAR();
   example_VHF();
   example_VIDYA();
   example_VORTEX();
   example_VWAP();
   example_VWMA();
   example_WAD();
   example_WAVETREND();
   example_WCLPRICE();
   example_WILLR();
   example_WMA();
   example_ZIGZAG();
   example_ZLEMA();
   printf( "ta_examples: %d examples, %d failures\n", 243, failures );
   return failures == 0 ? 0 : 1;
}
