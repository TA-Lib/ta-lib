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
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  100304 AC   Creation           
 *
 */

/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
/* All code within this section is automatically
 * generated by gen_code. Any modification will be lost
 * next time gen_code is run.
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */    #using <mscorlib.dll>
/* Generated */    #include "Core.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (TA_INTERNAL_ERROR)
/* Generated */    namespace TA { namespace Lib {
/* Generated */ #else
/* Generated */    #include <string.h>
/* Generated */    #include <math.h>
/* Generated */    #include "ta_func.h"
/* Generated */    #include "ta_trace.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #ifndef TA_UTILITY_H
/* Generated */    #include "ta_utility.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #ifndef TA_MEMORY_H
/* Generated */    #include "ta_memory.h"
/* Generated */ #endif
/* Generated */ 
/* Generated */ #define TA_PREFIX(x) TA_##x
/* Generated */ #define INPUT_TYPE   double
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ int Core::CDLEVENINGSTAR_Lookback( double        optInPenetration )  /* From 0 to TA_REAL_MAX */
/* Generated */ 
/* Generated */ #else
/* Generated */ int TA_CDLEVENINGSTAR_Lookback( double        optInPenetration )  /* From 0 to TA_REAL_MAX */
/* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert lookback code here. */
    (void) optInPenetration;
    return max( TA_CANDLEAVGPERIOD(TA_BodyShort), TA_CANDLEAVGPERIOD(TA_BodyLong) ) + 2;
}

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/*
 * TA_CDLEVENINGSTAR - Evening Star
 * 
 * Input  = Open, High, Low, Close
 * Output = int
 * 
 * Optional Parameters
 * -------------------
 * optInPenetration:(From 0 to TA_REAL_MAX)
 *    Percentage of penetration of a candle within another candle
 * 
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum Core::TA_RetCode Core::CDLEVENINGSTAR( int    startIdx,
/* Generated */                                             int    endIdx,
/* Generated */                                             double       inOpen __gc [],
/* Generated */                                             double       inHigh __gc [],
/* Generated */                                             double       inLow __gc [],
/* Generated */                                             double       inClose __gc [],
/* Generated */                                             double        optInPenetration, /* From 0 to TA_REAL_MAX */
/* Generated */                                             [OutAttribute]Int32 *outBegIdx,
/* Generated */                                             [OutAttribute]Int32 *outNbElement,
/* Generated */                                             int           outInteger __gc [] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_CDLEVENINGSTAR( int    startIdx,
/* Generated */                               int    endIdx,
/* Generated */                               const double inOpen[],
/* Generated */                               const double inHigh[],
/* Generated */                               const double inLow[],
/* Generated */                               const double inClose[],
/* Generated */                               double        optInPenetration, /* From 0 to TA_REAL_MAX */
/* Generated */                               int          *outBegIdx,
/* Generated */                               int          *outNbElement,
/* Generated */                               int           outInteger[] )
/* Generated */ #endif
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
    double BodyShortPeriodTotal, BodyLongPeriodTotal, BodyShortPeriodTotal2;
    int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */ 
/* Generated */    /* Validate the requested output range. */
/* Generated */    if( startIdx < 0 )
/* Generated */       return TA_OUT_OF_RANGE_START_INDEX;
/* Generated */    if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */       return TA_OUT_OF_RANGE_END_INDEX;
/* Generated */ 
/* Generated */    /* Validate the parameters. */
/* Generated */    /* Verify required price component. */
/* Generated */    if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */       return TA_BAD_PARAM;
/* Generated */ 
/* Generated */    if( optInPenetration == TA_REAL_DEFAULT )
/* Generated */       optInPenetration = 3.000000e-1;
/* Generated */    else if( (optInPenetration < 0.000000e+0) ||/* Generated */  (optInPenetration > 3.000000e+37) )
/* Generated */       return TA_BAD_PARAM;
/* Generated */ 
/* Generated */    if( outInteger == NULL )
/* Generated */       return TA_BAD_PARAM;
/* Generated */ 
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = TA_CDLEVENINGSTAR_Lookback(optInPenetration);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNbElement = 0;
      return TA_SUCCESS;
   }

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   BodyLongPeriodTotal = 0;
   BodyShortPeriodTotal = 0;
   BodyShortPeriodTotal2 = 0;
   BodyLongTrailingIdx = startIdx -2 - TA_CANDLEAVGPERIOD(TA_BodyLong);
   BodyShortTrailingIdx = startIdx -1 - TA_CANDLEAVGPERIOD(TA_BodyShort);
   
   i = BodyLongTrailingIdx;
   while( i < startIdx-2 ) {
        BodyLongPeriodTotal += TA_CANDLERANGE( TA_BodyLong, i );
        i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx-1 ) {
        BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i );
        BodyShortPeriodTotal2 += TA_CANDLERANGE( TA_BodyShort, i+1 );
        i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - first candle: long white real body
    * - second candle: star (short real body gapping up)
    * - third candle: black real body that moves well within the first candle's real body
    * The meaning of "short" and "long" is specified with TA_SetCandleSettings
    * The meaning of "moves well within" is specified with optInPenetration and "moves" should mean the real body should
    * not be short ("short" is specified with TA_SetCandleSettings) - Greg Morris wants it to be long, someone else want
    * it to be relatively long
    * outInteger is negative (-1 to -100): evening star is always bearish; 
    * the user should consider that an evening star is significant when it appears in an uptrend, 
    * while this function does not consider the trend
    */
   outIdx = 0;
   do
   {
        if( TA_REALBODY(i-2) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal, i-2 ) &&         // 1st: long
            TA_CANDLECOLOR(i-2) == 1 &&                                                             //           white
            TA_REALBODY(i-1) <= TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal, i-1 ) &&      // 2nd: short
            TA_REALBODYGAPUP(i-1,i-2) &&                                                            //            gapping up
            TA_REALBODY(i) > TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal2, i ) &&          // 3rd: longer than short
            TA_CANDLECOLOR(i) == -1 &&                                                              //          black real body
            inClose[i] < inClose[i-2] - TA_REALBODY(i-2) * optInPenetration                         //               closing well within 1st rb
          )
            outInteger[outIdx++] = -100;
        else
            outInteger[outIdx++] = 0;
        /* add the current range and subtract the first range: this is done after the pattern recognition 
         * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
         */
        BodyLongPeriodTotal += TA_CANDLERANGE( TA_BodyLong, i-2 ) - TA_CANDLERANGE( TA_BodyLong, BodyLongTrailingIdx );
        BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i-1 ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx );
        BodyShortPeriodTotal2 += TA_CANDLERANGE( TA_BodyShort, i ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx+1 );
        i++; 
        BodyLongTrailingIdx++;
        BodyShortTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNbElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}

/**** START GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #define  USE_SINGLE_PRECISION_INPUT
/* Generated */ #if !defined( _MANAGED )
/* Generated */    #undef   TA_PREFIX
/* Generated */    #define  TA_PREFIX(x) TA_S_##x
/* Generated */ #endif
/* Generated */ #undef   INPUT_TYPE
/* Generated */ #define  INPUT_TYPE float
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum Core::TA_RetCode Core::CDLEVENINGSTAR( int    startIdx,
/* Generated */                                             int    endIdx,
/* Generated */                                             float        inOpen __gc [],
/* Generated */                                             float        inHigh __gc [],
/* Generated */                                             float        inLow __gc [],
/* Generated */                                             float        inClose __gc [],
/* Generated */                                             double        optInPenetration, /* From 0 to TA_REAL_MAX */
/* Generated */                                             [OutAttribute]Int32 *outBegIdx,
/* Generated */                                             [OutAttribute]Int32 *outNbElement,
/* Generated */                                             int           outInteger __gc [] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_CDLEVENINGSTAR( int    startIdx,
/* Generated */                                 int    endIdx,
/* Generated */                                 const float  inOpen[],
/* Generated */                                 const float  inHigh[],
/* Generated */                                 const float  inLow[],
/* Generated */                                 const float  inClose[],
/* Generated */                                 double        optInPenetration, /* From 0 to TA_REAL_MAX */
/* Generated */                                 int          *outBegIdx,
/* Generated */                                 int          *outNbElement,
/* Generated */                                 int           outInteger[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */     double BodyShortPeriodTotal, BodyLongPeriodTotal, BodyShortPeriodTotal2;
/* Generated */     int i, outIdx, BodyShortTrailingIdx, BodyLongTrailingIdx, lookbackTotal;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return TA_OUT_OF_RANGE_START_INDEX;
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return TA_OUT_OF_RANGE_END_INDEX;
/* Generated */     if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */        return TA_BAD_PARAM;
/* Generated */     if( optInPenetration == TA_REAL_DEFAULT )
/* Generated */        optInPenetration = 3.000000e-1;
/* Generated */     else if( (optInPenetration < 0.000000e+0) ||  (optInPenetration > 3.000000e+37) )
/* Generated */        return TA_BAD_PARAM;
/* Generated */     if( outInteger == NULL )
/* Generated */        return TA_BAD_PARAM;
/* Generated */  #endif 
/* Generated */    lookbackTotal = TA_CDLEVENINGSTAR_Lookback(optInPenetration);
/* Generated */    if( startIdx < lookbackTotal )
/* Generated */       startIdx = lookbackTotal;
/* Generated */    if( startIdx > endIdx )
/* Generated */    {
/* Generated */       *outBegIdx = 0;
/* Generated */       *outNbElement = 0;
/* Generated */       return TA_SUCCESS;
/* Generated */    }
/* Generated */    BodyLongPeriodTotal = 0;
/* Generated */    BodyShortPeriodTotal = 0;
/* Generated */    BodyShortPeriodTotal2 = 0;
/* Generated */    BodyLongTrailingIdx = startIdx -2 - TA_CANDLEAVGPERIOD(TA_BodyLong);
/* Generated */    BodyShortTrailingIdx = startIdx -1 - TA_CANDLEAVGPERIOD(TA_BodyShort);
/* Generated */    i = BodyLongTrailingIdx;
/* Generated */    while( i < startIdx-2 ) {
/* Generated */         BodyLongPeriodTotal += TA_CANDLERANGE( TA_BodyLong, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = BodyShortTrailingIdx;
/* Generated */    while( i < startIdx-1 ) {
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i );
/* Generated */         BodyShortPeriodTotal2 += TA_CANDLERANGE( TA_BodyShort, i+1 );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = startIdx;
/* Generated */    outIdx = 0;
/* Generated */    do
/* Generated */    {
/* Generated */         if( TA_REALBODY(i-2) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal, i-2 ) &&         // 1st: long
/* Generated */             TA_CANDLECOLOR(i-2) == 1 &&                                                             //           white
/* Generated */             TA_REALBODY(i-1) <= TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal, i-1 ) &&      // 2nd: short
/* Generated */             TA_REALBODYGAPUP(i-1,i-2) &&                                                            //            gapping up
/* Generated */             TA_REALBODY(i) > TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal2, i ) &&          // 3rd: longer than short
/* Generated */             TA_CANDLECOLOR(i) == -1 &&                                                              //          black real body
/* Generated */             inClose[i] < inClose[i-2] - TA_REALBODY(i-2) * optInPenetration                         //               closing well within 1st rb
/* Generated */           )
/* Generated */             outInteger[outIdx++] = -100;
/* Generated */         else
/* Generated */             outInteger[outIdx++] = 0;
/* Generated */         BodyLongPeriodTotal += TA_CANDLERANGE( TA_BodyLong, i-2 ) - TA_CANDLERANGE( TA_BodyLong, BodyLongTrailingIdx );
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i-1 ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx );
/* Generated */         BodyShortPeriodTotal2 += TA_CANDLERANGE( TA_BodyShort, i ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx+1 );
/* Generated */         i++; 
/* Generated */         BodyLongTrailingIdx++;
/* Generated */         BodyShortTrailingIdx++;
/* Generated */    } while( i <= endIdx );
/* Generated */    *outNbElement = outIdx;
/* Generated */    *outBegIdx    = startIdx;
/* Generated */    return TA_SUCCESS;
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }} // Close namespace TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

