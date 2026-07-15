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
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   public int adOscLookback( int optInFastPeriod, int optInSlowPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      int slowestPeriod;
      /* Use the slowest EMA period to evaluate the total lookback. */
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      /* Adjust startIdx to account for the lookback period. */
      return emaLookback(slowestPeriod) ;

   }
   public RetCode adOsc( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         double inVolume[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Implementation Note:
       *     The fastEMA varaible is not neceseraly the
       *     fastest EMA.
       *     In the same way, slowEMA is not neceseraly the
       *     slowest EMA.
       *
       *     The ADOSC is always the (fastEMA - slowEMA) regardless
       *     of the period specified. In other word:
       *
       *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
       *
       *        while
       *
       *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
       *
       *     In the first case the EMA(3) is truly a faster EMA,
       *     while in the second case, the EMA(10) is still call
       *     fastEMA in the algorithm, even if it is in fact slower.
       *
       *     This gives more flexibility to the user if they want to
       *     experiment with unusual parameter settings.
       */
      /* Identify the slowest period.
       * This infomration is used soleley to bootstrap
       * the algorithm (skip the lookback period).
       */
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = emaLookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      /* The following variables are used to
       * calculate the "ad".
       */
      ad = 0.0;
      /* Constants for EMA */
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      /* Initialize the two EMA
       *
       * Use the same range of initialization inputs for
       * both EMA and simply seed with the first A/D value.
       *
       * Note: Metastock do the same.
       */
      high = inHigh[today];
      low = inLow[today];
      tmp = high - low;
      close = inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      /* Initialize the EMA and skip the unstable period. */
      while( today < startIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      /* Perform the calculation for the requested range */
      outIdx = 0;
      while( today <= endIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adOscUnguarded( int startIdx,
                                  int endIdx,
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  double inVolume[],
                                  int optInFastPeriod,
                                  int optInSlowPeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      lookbackTotal = emaLookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      ad = 0.0;
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      high = inHigh[today];
      low = inLow[today];
      tmp = high - low;
      close = inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      while( today < startIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      outIdx = 0;
      while( today <= endIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adOsc( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         float inVolume[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      lookbackTotal = emaLookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      ad = 0.0;
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      high = (double)inHigh[today];
      low = (double)inLow[today];
      tmp = high - low;
      close = (double)inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      while( today < startIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      outIdx = 0;
      while( today <= endIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adOscUnguarded( int startIdx,
                                  int endIdx,
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  float inVolume[],
                                  int optInFastPeriod,
                                  int optInSlowPeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      lookbackTotal = emaLookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      ad = 0.0;
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      high = (double)inHigh[today];
      low = (double)inLow[today];
      tmp = high - low;
      close = (double)inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      while( today < startIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      outIdx = 0;
      while( today <= endIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
