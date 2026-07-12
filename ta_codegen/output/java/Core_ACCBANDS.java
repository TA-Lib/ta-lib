/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 *  071226 MF,CC  Fused single-loop rewrite: maintain the three band running
 *                sums (close for the middle band; the pointwise High/Low map for
 *                the upper/lower bands) over one shared trailing window, instead
 *                of two scratch buffers + three sma() calls. Enables streaming
 *                and is bit-identical to the prior three-SMA form (verified vs
 *                v0.6.4).
 */

   public int accbandsLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return smaLookback(optInTimePeriod) ;

   }
   public RetCode accbands( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            double inClose[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outRealUpperBand[],
                            double outRealMiddleBand[],
                            double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = smaLookback(optInTimePeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Each band is a simple moving average maintained as a running sum over a
       * shared trailing window (all three share optInTimePeriod, so one trailing
       * index walks all three windows in lockstep):
       *    middle = SMA( close )
       *    upper  = SMA( high * (1 + 4*(high-low)/(high+low)) )
       *    lower  = SMA( low  * (1 - 4*(high-low)/(high+low)) )
       * When high+low is zero the upper/lower map degenerates to high/low.
       * Fusing the three moving averages into one loop is bit-identical to the
       * former "two scratch buffers + three sma() calls": each accumulator's
       * add/record/subtract order is unchanged, and the High/Low map is a pure
       * function recomputed from the raw trailing bar.
       */
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      /* Warm up the running sums with the initial period,
       * except for the last value.
       */
      i = trailingIdx;
      while( i < startIdx ) {
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and output to be the
       * same buffer: every trailing bar is read before any output is written.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         /* Add the incoming bar to each running sum. */
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
         /* Record the current window sums. */
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         /* Remove the trailing bar from each running sum. */
         tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= inHigh[trailingIdx];
            periodTotalLower -= inLow[trailingIdx];
         }
         periodTotalMiddle -= inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         /* Write the three bands. */
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode accbandsUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outRealUpperBand[],
                                     double outRealMiddleBand[],
                                     double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            periodTotalUpper += inHigh[i] * (1 + tempReal);
            periodTotalLower += inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += inHigh[i];
            periodTotalLower += inLow[i];
         }
         periodTotalMiddle += inClose[i];
         i = i + 1;
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         tempReal = inHigh[trailingIdx] + inLow[trailingIdx];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[trailingIdx] - inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= inHigh[trailingIdx];
            periodTotalLower -= inLow[trailingIdx];
         }
         periodTotalMiddle -= inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode accbands( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            float inClose[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outRealUpperBand[],
                            double outRealMiddleBand[],
                            double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         tempReal = (double)inHigh[trailingIdx] + (double)inLow[trailingIdx];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[trailingIdx] - (double)inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= (double)inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= (double)inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= (double)inHigh[trailingIdx];
            periodTotalLower -= (double)inLow[trailingIdx];
         }
         periodTotalMiddle -= (double)inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode accbandsUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outRealUpperBand[],
                                     double outRealMiddleBand[],
                                     double outRealLowerBand[] )
   {
      double periodTotalUpper = 0;
      double periodTotalMiddle = 0;
      double periodTotalLower = 0;
      double tempUpper = 0;
      double tempMiddle = 0;
      double tempLower = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotalUpper = 0.0;
      periodTotalMiddle = 0.0;
      periodTotalLower = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      while( i < startIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            periodTotalUpper += (double)inHigh[i] * (1 + tempReal);
            periodTotalLower += (double)inLow[i] * (1 - tempReal);
         } else {
            periodTotalUpper += (double)inHigh[i];
            periodTotalLower += (double)inLow[i];
         }
         periodTotalMiddle += (double)inClose[i];
         i = i + 1;
         tempUpper = periodTotalUpper;
         tempMiddle = periodTotalMiddle;
         tempLower = periodTotalLower;
         tempReal = (double)inHigh[trailingIdx] + (double)inLow[trailingIdx];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[trailingIdx] - (double)inLow[trailingIdx]) / tempReal;
            periodTotalUpper -= (double)inHigh[trailingIdx] * (1 + tempReal);
            periodTotalLower -= (double)inLow[trailingIdx] * (1 - tempReal);
         } else {
            periodTotalUpper -= (double)inHigh[trailingIdx];
            periodTotalLower -= (double)inLow[trailingIdx];
         }
         periodTotalMiddle -= (double)inClose[trailingIdx];
         trailingIdx = trailingIdx + 1;
         outRealUpperBand[outIdx] = tempUpper / (double)optInTimePeriod;
         outRealMiddleBand[outIdx] = tempMiddle / (double)optInTimePeriod;
         outRealLowerBand[outIdx] = tempLower / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
