/* TA-LIB Copyright (c) 1999-2006, Mario Fortier
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
 *  120804 AC   Creation           
 *
 */

/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
/* All code within this section is automatically
 * generated by gen_code. Any modification will be lost
 * next time gen_code is run.
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */    #include "TA-Lib-Core.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (NAMESPACE(TA_RetCode)TA_INTERNAL_ERROR)
/* Generated */    namespace TA { namespace Lib {
/* Generated */ #elif defined( _JAVA )
/* Generated */    #include "ta_defs.h"
/* Generated */    #define TA_INTERNAL_ERROR(Id) (NAMESPACE(TA_RetCode)TA_INTERNAL_ERROR)
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
/* Generated */ int Core::CDLSTALLEDPATTERN_Lookback( void )
/* Generated */ 
/* Generated */ #elif defined( _JAVA )
/* Generated */ public int CDLSTALLEDPATTERN_Lookback(  )
/* Generated */ 
/* Generated */ #else
/* Generated */ int TA_CDLSTALLEDPATTERN_Lookback( void )
/* Generated */ 
/* Generated */ #endif
/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/
{
   /* insert local variable here */

/**** START GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/
/* Generated */ /* No parameters to validate. */
/**** END GENCODE SECTION 2 - DO NOT DELETE THIS LINE ****/

   /* insert lookback code here. */
    return max( max( TA_CANDLEAVGPERIOD(TA_BodyLong), TA_CANDLEAVGPERIOD(TA_BodyShort) ),
                max( TA_CANDLEAVGPERIOD(TA_ShadowVeryShort), TA_CANDLEAVGPERIOD(TA_Near) )
            ) + 2;
}

/**** START GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
/*
 * TA_CDLSTALLEDPATTERN - Stalled Pattern
 * 
 * Input  = Open, High, Low, Close
 * Output = int
 * 
 */
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum class Core::TA_RetCode Core::CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                                      int    endIdx,
/* Generated */                                                      cli::array<double>^ inOpen,
/* Generated */                                                      cli::array<double>^ inHigh,
/* Generated */                                                      cli::array<double>^ inLow,
/* Generated */                                                      cli::array<double>^ inClose,
/* Generated */                                                      [Out]int%    outBegIdx,
/* Generated */                                                      [Out]int%    outNbElement,
/* Generated */                                                      cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public TA_RetCode CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      double       inOpen[],
/* Generated */                                      double       inHigh[],
/* Generated */                                      double       inLow[],
/* Generated */                                      double       inClose[],
/* Generated */                                      MInteger     outBegIdx,
/* Generated */                                      MInteger     outNbElement,
/* Generated */                                      int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                  int    endIdx,
/* Generated */                                  const double inOpen[],
/* Generated */                                  const double inHigh[],
/* Generated */                                  const double inLow[],
/* Generated */                                  const double inClose[],
/* Generated */                                  int          *outBegIdx,
/* Generated */                                  int          *outNbElement,
/* Generated */                                  int           outInteger[] )
/* Generated */ #endif
/**** END GENCODE SECTION 3 - DO NOT DELETE THIS LINE ****/
{
   /* Insert local variables here. */
    ARRAY_LOCAL(BodyLongPeriodTotal,3);
	ARRAY_LOCAL(NearPeriodTotal,3);
	double BodyShortPeriodTotal, ShadowVeryShortPeriodTotal;
    int i, outIdx, totIdx, BodyLongTrailingIdx, BodyShortTrailingIdx, ShadowVeryShortTrailingIdx, NearTrailingIdx, 
        lookbackTotal;

/**** START GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */ 
/* Generated */    /* Validate the requested output range. */
/* Generated */    if( startIdx < 0 )
/* Generated */       return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_START_INDEX;
/* Generated */    if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */       return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_END_INDEX;
/* Generated */ 
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    /* Verify required price component. */
/* Generated */    if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */       return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA)*/
/* Generated */    #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */    if( !outInteger )
/* Generated */       return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */ 
/* Generated */    #endif /* !defined(_MANAGED) && !defined(_JAVA) */
/* Generated */ #endif /* TA_FUNC_NO_RANGE_CHECK */
/* Generated */ 
/**** END GENCODE SECTION 4 - DO NOT DELETE THIS LINE ****/

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */

   lookbackTotal = LOOKBACK_CALL(CDLSTALLEDPATTERN)();

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
      VALUE_HANDLE_DEREF_TO_ZERO(outNbElement);
      return NAMESPACE(TA_RetCode)TA_SUCCESS;
   }

   /* Do the calculation using tight loops. */
   /* Add-up the initial period, except for the last value. */
   BodyLongPeriodTotal[2] = 0;
   BodyLongPeriodTotal[1] = 0;
   BodyLongPeriodTotal[0] = 0;
   BodyLongTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyLong);
   BodyShortPeriodTotal = 0;
   BodyShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyShort);
   ShadowVeryShortPeriodTotal = 0;
   ShadowVeryShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryShort);
   NearPeriodTotal[2] = 0;
   NearPeriodTotal[1] = 0;
   NearPeriodTotal[0] = 0;
   NearTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_Near);
   
   i = BodyLongTrailingIdx;
   while( i < startIdx ) {
        BodyLongPeriodTotal[2] += TA_CANDLERANGE( TA_BodyLong, i-2 );
        BodyLongPeriodTotal[1] += TA_CANDLERANGE( TA_BodyLong, i-1 );
        i++;
   }
   i = BodyShortTrailingIdx;
   while( i < startIdx ) {
        BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i );
        i++;
   }
   i = ShadowVeryShortTrailingIdx;
   while( i < startIdx ) {
        ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i-1 );
        i++;
   }
   i = NearTrailingIdx;
   while( i < startIdx ) {
        NearPeriodTotal[2] += TA_CANDLERANGE( TA_Near, i-2 );
        NearPeriodTotal[1] += TA_CANDLERANGE( TA_Near, i-1 );
        i++;
   }
   i = startIdx;

   /* Proceed with the calculation for the requested range.
    * Must have:
    * - three white candlesticks with consecutively higher closes
    * - first candle: long white
    * - second candle: long white with no or very short upper shadow opening within or near the previous white real body
    * and closing higher than the prior candle
    * - third candle: small white that gaps away or "rides on the shoulder" of the prior long real body (= it's at 
    * the upper end of the prior real body)
    * The meanings of "long", "very short", "short", "near" are specified with TA_SetCandleSettings;
    * outInteger is negative (-1 to -100): stalled pattern is always bearish;
    * the user should consider that stalled pattern is significant when it appears in uptrend, while this function 
    * does not consider it
    */
   outIdx = 0;
   do
   {
        if( TA_CANDLECOLOR(i-2) == 1 &&                                             // 1st white
            TA_CANDLECOLOR(i-1) == 1 &&                                             // 2nd white
            TA_CANDLECOLOR(i) == 1 &&                                               // 3rd white
            inClose[i] > inClose[i-1] && inClose[i-1] > inClose[i-2] &&             // consecutive higher closes
            TA_REALBODY(i-2) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal[2], i-2 ) &&  // 1st: long real body
            TA_REALBODY(i-1) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal[1], i-1 ) &&  // 2nd: long real body
                                                                                    // very short upper shadow 
            TA_UPPERSHADOW(i-1) < TA_CANDLEAVERAGE( TA_ShadowVeryShort, ShadowVeryShortPeriodTotal, i-1 ) &&
                                                                                    // opens within/near 1st real body
            inOpen[i-1] > inOpen[i-2] &&                                                    
            inOpen[i-1] <= inClose[i-2] + TA_CANDLEAVERAGE( TA_Near, NearPeriodTotal[2], i-2 ) &&
            TA_REALBODY(i) < TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal, i ) &&       // 3rd: small real body
                                                                                    // rides on the shoulder of 2nd real body
            inOpen[i] >= inClose[i-1] - TA_REALBODY(i) - TA_CANDLEAVERAGE( TA_Near, NearPeriodTotal[1], i-1 )
          )
            outInteger[outIdx++] = -100;
        else
            outInteger[outIdx++] = 0;
        /* add the current range and subtract the first range: this is done after the pattern recognition 
         * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
         */
        for (totIdx = 2; totIdx >= 1; --totIdx) {
            BodyLongPeriodTotal[totIdx] += TA_CANDLERANGE( TA_BodyLong, i-totIdx ) 
                                         - TA_CANDLERANGE( TA_BodyLong, BodyLongTrailingIdx-totIdx );
            NearPeriodTotal[totIdx] += TA_CANDLERANGE( TA_Near, i-totIdx ) 
                                     - TA_CANDLERANGE( TA_Near, NearTrailingIdx-totIdx );
        }
        BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx );
        ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i-1 ) 
                                    - TA_CANDLERANGE( TA_ShadowVeryShort, ShadowVeryShortTrailingIdx-1 );
        i++; 
        BodyLongTrailingIdx++;
        BodyShortTrailingIdx++;
        ShadowVeryShortTrailingIdx++;
        NearTrailingIdx++;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   VALUE_HANDLE_DEREF(outNbElement) = outIdx;
   VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;

   return NAMESPACE(TA_RetCode)TA_SUCCESS;
}

/**** START GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/
/* Generated */ 
/* Generated */ #define  USE_SINGLE_PRECISION_INPUT
/* Generated */ #if !defined( _MANAGED ) && !defined( _JAVA )
/* Generated */    #undef   TA_PREFIX
/* Generated */    #define  TA_PREFIX(x) TA_S_##x
/* Generated */ #endif
/* Generated */ #undef   INPUT_TYPE
/* Generated */ #define  INPUT_TYPE float
/* Generated */ #if defined( _MANAGED )
/* Generated */ enum class Core::TA_RetCode Core::CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                                      int    endIdx,
/* Generated */                                                      cli::array<float>^ inOpen,
/* Generated */                                                      cli::array<float>^ inHigh,
/* Generated */                                                      cli::array<float>^ inLow,
/* Generated */                                                      cli::array<float>^ inClose,
/* Generated */                                                      [Out]int%    outBegIdx,
/* Generated */                                                      [Out]int%    outNbElement,
/* Generated */                                                      cli::array<int>^  outInteger )
/* Generated */ #elif defined( _JAVA )
/* Generated */ public TA_RetCode CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                      int    endIdx,
/* Generated */                                      float        inOpen[],
/* Generated */                                      float        inHigh[],
/* Generated */                                      float        inLow[],
/* Generated */                                      float        inClose[],
/* Generated */                                      MInteger     outBegIdx,
/* Generated */                                      MInteger     outNbElement,
/* Generated */                                      int           outInteger[] )
/* Generated */ #else
/* Generated */ TA_RetCode TA_S_CDLSTALLEDPATTERN( int    startIdx,
/* Generated */                                    int    endIdx,
/* Generated */                                    const float  inOpen[],
/* Generated */                                    const float  inHigh[],
/* Generated */                                    const float  inLow[],
/* Generated */                                    const float  inClose[],
/* Generated */                                    int          *outBegIdx,
/* Generated */                                    int          *outNbElement,
/* Generated */                                    int           outInteger[] )
/* Generated */ #endif
/* Generated */ {
/* Generated */     ARRAY_LOCAL(BodyLongPeriodTotal,3);
/* Generated */ 	ARRAY_LOCAL(NearPeriodTotal,3);
/* Generated */ 	double BodyShortPeriodTotal, ShadowVeryShortPeriodTotal;
/* Generated */     int i, outIdx, totIdx, BodyLongTrailingIdx, BodyShortTrailingIdx, ShadowVeryShortTrailingIdx, NearTrailingIdx, 
/* Generated */         lookbackTotal;
/* Generated */  #ifndef TA_FUNC_NO_RANGE_CHECK
/* Generated */     if( startIdx < 0 )
/* Generated */        return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_START_INDEX;
/* Generated */     if( (endIdx < 0) || (endIdx < startIdx))
/* Generated */        return NAMESPACE(TA_RetCode)TA_OUT_OF_RANGE_END_INDEX;
/* Generated */     #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */     if(!inOpen||!inHigh||!inLow||!inClose)
/* Generated */        return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */     #endif 
/* Generated */     #if !defined(_MANAGED) && !defined(_JAVA)
/* Generated */     if( !outInteger )
/* Generated */        return NAMESPACE(TA_RetCode)TA_BAD_PARAM;
/* Generated */     #endif 
/* Generated */  #endif 
/* Generated */    lookbackTotal = LOOKBACK_CALL(CDLSTALLEDPATTERN)();
/* Generated */    if( startIdx < lookbackTotal )
/* Generated */       startIdx = lookbackTotal;
/* Generated */    if( startIdx > endIdx )
/* Generated */    {
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outBegIdx);
/* Generated */       VALUE_HANDLE_DEREF_TO_ZERO(outNbElement);
/* Generated */       return NAMESPACE(TA_RetCode)TA_SUCCESS;
/* Generated */    }
/* Generated */    BodyLongPeriodTotal[2] = 0;
/* Generated */    BodyLongPeriodTotal[1] = 0;
/* Generated */    BodyLongPeriodTotal[0] = 0;
/* Generated */    BodyLongTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyLong);
/* Generated */    BodyShortPeriodTotal = 0;
/* Generated */    BodyShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_BodyShort);
/* Generated */    ShadowVeryShortPeriodTotal = 0;
/* Generated */    ShadowVeryShortTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_ShadowVeryShort);
/* Generated */    NearPeriodTotal[2] = 0;
/* Generated */    NearPeriodTotal[1] = 0;
/* Generated */    NearPeriodTotal[0] = 0;
/* Generated */    NearTrailingIdx = startIdx - TA_CANDLEAVGPERIOD(TA_Near);
/* Generated */    i = BodyLongTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         BodyLongPeriodTotal[2] += TA_CANDLERANGE( TA_BodyLong, i-2 );
/* Generated */         BodyLongPeriodTotal[1] += TA_CANDLERANGE( TA_BodyLong, i-1 );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = BodyShortTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = ShadowVeryShortTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i-1 );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = NearTrailingIdx;
/* Generated */    while( i < startIdx ) {
/* Generated */         NearPeriodTotal[2] += TA_CANDLERANGE( TA_Near, i-2 );
/* Generated */         NearPeriodTotal[1] += TA_CANDLERANGE( TA_Near, i-1 );
/* Generated */         i++;
/* Generated */    }
/* Generated */    i = startIdx;
/* Generated */    outIdx = 0;
/* Generated */    do
/* Generated */    {
/* Generated */         if( TA_CANDLECOLOR(i-2) == 1 &&                                             // 1st white
/* Generated */             TA_CANDLECOLOR(i-1) == 1 &&                                             // 2nd white
/* Generated */             TA_CANDLECOLOR(i) == 1 &&                                               // 3rd white
/* Generated */             inClose[i] > inClose[i-1] && inClose[i-1] > inClose[i-2] &&             // consecutive higher closes
/* Generated */             TA_REALBODY(i-2) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal[2], i-2 ) &&  // 1st: long real body
/* Generated */             TA_REALBODY(i-1) > TA_CANDLEAVERAGE( TA_BodyLong, BodyLongPeriodTotal[1], i-1 ) &&  // 2nd: long real body
/* Generated */                                                                                     // very short upper shadow 
/* Generated */             TA_UPPERSHADOW(i-1) < TA_CANDLEAVERAGE( TA_ShadowVeryShort, ShadowVeryShortPeriodTotal, i-1 ) &&
/* Generated */                                                                                     // opens within/near 1st real body
/* Generated */             inOpen[i-1] > inOpen[i-2] &&                                                    
/* Generated */             inOpen[i-1] <= inClose[i-2] + TA_CANDLEAVERAGE( TA_Near, NearPeriodTotal[2], i-2 ) &&
/* Generated */             TA_REALBODY(i) < TA_CANDLEAVERAGE( TA_BodyShort, BodyShortPeriodTotal, i ) &&       // 3rd: small real body
/* Generated */                                                                                     // rides on the shoulder of 2nd real body
/* Generated */             inOpen[i] >= inClose[i-1] - TA_REALBODY(i) - TA_CANDLEAVERAGE( TA_Near, NearPeriodTotal[1], i-1 )
/* Generated */           )
/* Generated */             outInteger[outIdx++] = -100;
/* Generated */         else
/* Generated */             outInteger[outIdx++] = 0;
/* Generated */         for (totIdx = 2; totIdx >= 1; --totIdx) {
/* Generated */             BodyLongPeriodTotal[totIdx] += TA_CANDLERANGE( TA_BodyLong, i-totIdx ) 
/* Generated */                                          - TA_CANDLERANGE( TA_BodyLong, BodyLongTrailingIdx-totIdx );
/* Generated */             NearPeriodTotal[totIdx] += TA_CANDLERANGE( TA_Near, i-totIdx ) 
/* Generated */                                      - TA_CANDLERANGE( TA_Near, NearTrailingIdx-totIdx );
/* Generated */         }
/* Generated */         BodyShortPeriodTotal += TA_CANDLERANGE( TA_BodyShort, i ) - TA_CANDLERANGE( TA_BodyShort, BodyShortTrailingIdx );
/* Generated */         ShadowVeryShortPeriodTotal += TA_CANDLERANGE( TA_ShadowVeryShort, i-1 ) 
/* Generated */                                     - TA_CANDLERANGE( TA_ShadowVeryShort, ShadowVeryShortTrailingIdx-1 );
/* Generated */         i++; 
/* Generated */         BodyLongTrailingIdx++;
/* Generated */         BodyShortTrailingIdx++;
/* Generated */         ShadowVeryShortTrailingIdx++;
/* Generated */         NearTrailingIdx++;
/* Generated */    } while( i <= endIdx );
/* Generated */    VALUE_HANDLE_DEREF(outNbElement) = outIdx;
/* Generated */    VALUE_HANDLE_DEREF(outBegIdx)    = startIdx;
/* Generated */    return NAMESPACE(TA_RetCode)TA_SUCCESS;
/* Generated */ }
/* Generated */ 
/* Generated */ #if defined( _MANAGED )
/* Generated */ }} // Close namespace TA.Lib
/* Generated */ #endif
/**** END GENCODE SECTION 5 - DO NOT DELETE THIS LINE ****/

