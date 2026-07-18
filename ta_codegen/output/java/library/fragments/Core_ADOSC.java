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
/**** Streaming API *****/

   /**
    * A live ADOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#adOsc} over the same series.
    * Open with {@link Core#adOscOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class AdOscStream {
      final Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      double slowEMA;
      double slowk;
      double one_minus_slowk;
      double fastEMA;
      double fastk;
      double one_minus_fastk;
      double ad;
      double cur_outReal;

      AdOscStream( Core core ) { this.core = core; }

      AdOscStream( AdOscStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.slowEMA = other.slowEMA;
         this.slowk = other.slowk;
         this.one_minus_slowk = other.one_minus_slowk;
         this.fastEMA = other.fastEMA;
         this.fastk = other.fastk;
         this.one_minus_fastk = other.one_minus_fastk;
         this.ad = other.ad;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         core.adOscStreamStep(this, inHigh, inLow, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         AdOscStream scratch = new AdOscStream(this);
         core.adOscStreamStep(scratch, inHigh, inLow, inClose, inVolume);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public AdOscStream copy() {
         return new AdOscStream(this);
      }
   }
   void adOscStreamStep( AdOscStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double high = 0.0;
      double low = 0.0;
      double close = 0.0;
      double tmp = 0.0;
      high = inHigh;
      low = inLow;
      tmp = high - low;
      close = inClose;
      if( tmp > 0.0 ) {
         sp.ad += (close - low - (high - close)) / tmp * (double)inVolume;
      }
      sp.fastEMA = Math.fma(sp.one_minus_fastk, sp.fastEMA, sp.fastk * sp.ad);
      sp.slowEMA = Math.fma(sp.one_minus_slowk, sp.slowEMA, sp.slowk * sp.ad);
      sp.cur_outReal = sp.fastEMA - sp.slowEMA;
   }
   private RetCode adOscOpenBody( AdOscStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
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
         lastValue_outReal = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.slowEMA = slowEMA;
      sp.slowk = slowk;
      sp.one_minus_slowk = one_minus_slowk;
      sp.fastEMA = fastEMA;
      sp.fastk = fastk;
      sp.one_minus_fastk = one_minus_fastk;
      sp.ad = ad;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode adOscOpenAndFillBody( AdOscStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
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
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.slowEMA = slowEMA;
      sp.slowk = slowk;
      sp.one_minus_slowk = one_minus_slowk;
      sp.fastEMA = fastEMA;
      sp.fastk = fastk;
      sp.one_minus_fastk = one_minus_fastk;
      sp.ad = ad;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind adOscOpen (composition seam). */
   AdOscStream adOscOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod )
   {
      AdOscStream sp = new AdOscStream(this);
      RetCode retCode = adOscOpenBody(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADOSC open: internal error");
      }
      throw new IllegalArgumentException("TA_ADOSC open: " + retCode);
   }
   /**
    * Open a live ADOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#adOsc} at that bar.
    * <p>The history must hold at least {@code adOscLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AdOscStream adOscOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod )
   {
      return adOscOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInFastPeriod, optInSlowPeriod);
   }
   /**
    * {@link Core#adOscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#adOsc} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AdOscStream adOscOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AdOscStream sp = new AdOscStream(this);
      RetCode retCode = adOscOpenAndFillBody(sp, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADOSC openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ADOSC openAndFill: " + retCode);
   }
