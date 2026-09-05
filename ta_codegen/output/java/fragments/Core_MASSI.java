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
 *  090426 MF,CC  First version (issue #359).
 */

   /**
    * Number of leading input bars {@link Core#MASSI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Number of bars in each of the two exponential
    *        averages of the high-low range (default 9; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars the ratio is summed over (default
    *        25; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MASSI_Lookback( int optInFastPeriod, int optInSlowPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 9;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      /* Two stacked EMA warm-ups over the high-low range, then the summation
       * window. The EMA term is exactly the callee's own lookback, which is what
       * makes MASSI inherit TA_FUNC_UNST_EMA -- and it shifts by 2u, not u.
       */
      return EMA_Lookback(optInFastPeriod) * 2 + (optInSlowPeriod - 1) ;

   }
   RetCode MASSI_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double optInK_1 = 0;
      double hl = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double ratio = 0;
      double total = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int lookbackEma = 0;
      int lookbackEma2 = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      double[] ratioRing;
      int ratioRing_Idx = 0;
      int maxIdx_ratioRing = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 9;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackEma = EMA_Lookback(optInFastPeriod);
      lookbackEma2 = lookbackEma * 2;
      lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInSlowPeriod < 1 ) return RetCode.InternalError;
      ratioRing = new double[optInSlowPeriod];
      maxIdx_ratioRing = (optInSlowPeriod)-1;
      ratioRing_Idx = 0;
      outBegIdx.value = startIdx;
      /* Dorsey's pipeline in one pass: EMA of the high-low range, EMA of that,
       * their ratio summed over a trailing window.
       *
       * Each stage seeds the way ema.c does -- a simple average of that stage's
       * first optInFastPeriod inputs -- and its boundary below is the callee
       * LOOKBACK, never (optInFastPeriod - 1). The two coincide exactly at
       * unstable period 0, which is where every cross-language gate runs, so
       * confusing them is invisible until TA_SetUnstablePeriod(TA_FUNC_UNST_EMA)
       * is warmed. The seed sums accumulate from 0.0 in production order; do not
       * reorder or fuse them (0.0+x is not x for x=-0.0).
       *
       * Seed from the SMA arm only: ema.c's TA_COMPATIBILITY_METASTOCK arm is
       * unreachable from the Rust, Java and C# APIs, so consulting
       * TA_GetCompatibility() here would make C diverge from three backends for a
       * setting they cannot read.
       */
      optInK_1 = 2.0 / (double)(optInFastPeriod + 1);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      total = 0.0;
      tempReal = 0.0;
      today = startIdx - lookbackTotal;
      nBar = 0;
      /* Runs through startIdx inclusive: that last pass fills the summation
       * window and so produces the first output.
       */
      while( today <= startIdx ) {
         hl = inHigh[today] - inLow[today];
         if( nBar < optInFastPeriod ) {
            sum1 = sum1 + hl;
            if( nBar == optInFastPeriod - 1 ) {
               ema1 = sum1 / optInFastPeriod;
            }
         } else {
            ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         }
         /* The stage counter is compared BEFORE it is subtracted, never after.
          * `n2 = nBar - lookbackEma; if( n2 >= 0 )` is correct in C and broken
          * everywhere else: the Rust backend renders these as usize, so the
          * subtraction underflows for the first lookbackEma bars.
          */
         if( nBar >= lookbackEma ) {
            n2 = nBar - lookbackEma;
            if( n2 < optInFastPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInFastPeriod - 1 ) {
                  ema2 = sum2 / optInFastPeriod;
               }
            } else {
               ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
            }
         }
         if( nBar >= lookbackEma2 ) {
            /* A flat market is the ratio's continuous limit, 1.0, not the zero
             * that an oscillator centred on zero would report: MASSI's own
             * neutral is optInSlowPeriod. Test ema2 exactly and never through an
             * epsilon band -- a smoothed price range carries the quote unit, so a
             * fixed band would pin the whole index at optInSlowPeriod for any
             * instrument quoted under it (issue #253).
             */
            if( ema2 == 0.0 ) {
               ratio = 1.0;
            } else {
               ratio = ema1 / ema2;
            }
            /* TA_SUM's accumulation order -- add, publish, subtract -- reproduced
             * over a ring: the slot written here was emptied out of `total` at the
             * end of the previous bar, so nothing has to be read before the store.
             */
            ratioRing[ratioRing_Idx] = ratio;
            total = total + ratio;
            ratioRing_Idx++;
            if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
            if( nBar == lookbackTotal ) {
               tempReal = total;
               total = total - ratioRing[ratioRing_Idx];
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      /* In-place safe: this store lands lookbackTotal bars behind the input
       * cursor, so no bar is written under a read still owed to it.
       */
      outReal[0] = tempReal;
      outIdx = 1;
      while( today <= endIdx ) {
         hl = inHigh[today] - inLow[today];
         ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
         if( ema2 == 0.0 ) {
            ratio = 1.0;
         } else {
            ratio = ema1 / ema2;
         }
         ratioRing[ratioRing_Idx] = ratio;
         total = total + ratio;
         ratioRing_Idx++;
         if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
         tempReal = total;
         total = total - ratioRing[ratioRing_Idx];
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MASSI_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double optInK_1 = 0;
      double hl = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double ratio = 0;
      double total = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int lookbackEma = 0;
      int lookbackEma2 = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      double[] ratioRing;
      int ratioRing_Idx = 0;
      int maxIdx_ratioRing = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 9;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackEma = EMA_Lookback(optInFastPeriod);
      lookbackEma2 = lookbackEma * 2;
      lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInSlowPeriod < 1 ) return RetCode.InternalError;
      ratioRing = new double[optInSlowPeriod];
      maxIdx_ratioRing = (optInSlowPeriod)-1;
      ratioRing_Idx = 0;
      outBegIdx.value = startIdx;
      optInK_1 = 2.0 / (double)(optInFastPeriod + 1);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      total = 0.0;
      tempReal = 0.0;
      today = startIdx - lookbackTotal;
      nBar = 0;
      while( today <= startIdx ) {
         hl = (double)inHigh[today] - (double)inLow[today];
         if( nBar < optInFastPeriod ) {
            sum1 = sum1 + hl;
            if( nBar == optInFastPeriod - 1 ) {
               ema1 = sum1 / optInFastPeriod;
            }
         } else {
            ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         }
         if( nBar >= lookbackEma ) {
            n2 = nBar - lookbackEma;
            if( n2 < optInFastPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInFastPeriod - 1 ) {
                  ema2 = sum2 / optInFastPeriod;
               }
            } else {
               ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
            }
         }
         if( nBar >= lookbackEma2 ) {
            if( ema2 == 0.0 ) {
               ratio = 1.0;
            } else {
               ratio = ema1 / ema2;
            }
            ratioRing[ratioRing_Idx] = ratio;
            total = total + ratio;
            ratioRing_Idx++;
            if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
            if( nBar == lookbackTotal ) {
               tempReal = total;
               total = total - ratioRing[ratioRing_Idx];
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      outReal[0] = tempReal;
      outIdx = 1;
      while( today <= endIdx ) {
         hl = (double)inHigh[today] - (double)inLow[today];
         ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
         if( ema2 == 0.0 ) {
            ratio = 1.0;
         } else {
            ratio = ema1 / ema2;
         }
         ratioRing[ratioRing_Idx] = ratio;
         total = total + ratio;
         ratioRing_Idx++;
         if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
         tempReal = total;
         total = total - ratioRing[ratioRing_Idx];
         outReal[outIdx] = tempReal;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Mass Index: Donald Dorsey's non-directional measure of how the trading
    * range itself is expanding or contracting. The high-low range is smoothed
    * by an exponential moving average, that average is smoothed again by a
    * second one of the same length, and the ratio of the first to the second is
    * summed over a trailing window. Read it as a bulge detector, not a
    * direction. A ratio above one means the range is widening faster than its
    * own smoothing can absorb, so the sum rises; a narrowing range pulls it
    * back down. Dorsey's own rule is the "reversal bulge": the index rising
    * above 27, then falling back under 26.5, warns that the prevailing trend is
    * about to reverse. Which way it reverses has to come from a trend
    * indicator, because the Mass Index has no sign of its own.
    * <p><b>Formula</b>
    * <pre>{@code
    * HL = high - low
    * single = EMA( HL, optInFastPeriod )
    * double = EMA( single, optInFastPeriod )
    * MASSI = SUM( single / double, optInSlowPeriod )
    * Both averages are the standard TA-Lib EMA: smoothing factor 2 / (optInFastPeriod + 1), seeded with the simple average of the first optInFastPeriod inputs of that stage.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The two periods are not interchangeable and are never swapped: {@code optInFastPeriod} is the length of both exponential averages, {@code optInSlowPeriod} the length of the summation window. Some implementations reorder them when the summation window is the shorter of the two; this one does not.</li>
    * <li>A window in which every bar is exactly flat, high equal to low, leaves both averages at zero. The ratio is reported as 1 there, its continuous limit, so a flat market yields exactly {@code optInSlowPeriod} rather than a spurious zero.</li>
    * <li>Implementations disagree on how the exponential averages are seeded. TA-Lib uses its own EMA convention, the simple average of the first {@code optInFastPeriod} inputs, where Tulip Indicators, ta4j and trading-signals seed from a single raw value and converge to these values only after many bars. Published sample vectors, including the one in Achelis, are seeded that way and match only in the tail.</li>
    * <li>MASSI inherits EMA's unstable period rather than owning one, and inherits it twice: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, u)} moves the first output by 2u.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MASSI_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price.
    * @param inLow Low price.
    * @param optInFastPeriod Number of bars in each of the two exponential
    *        averages of the high-low range (default 9; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars the ratio is summed over (default
    *        25; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Summed ratio of the two smoothed high-low ranges. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CVI
    * @see Core#ATR
    * @see Core#NATR
    * @see Core#TRANGE
    * @see Core#EMA
    * @see Core#SUM
    */
   public OutRange MASSI( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          double outReal[] )
   {
      requireIndexRange("MASSI", startIdx, endIdx);
      int guardStart = clampedStart("MASSI", startIdx, MASSI_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MASSI", "inHigh", inHigh, guardInLen);
      requireLength("MASSI", "inLow", inLow, guardInLen);
      requireLength("MASSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MASSI_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MASSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Mass Index: Donald Dorsey's non-directional measure of how the trading
    * range itself is expanding or contracting. The high-low range is smoothed
    * by an exponential moving average, that average is smoothed again by a
    * second one of the same length, and the ratio of the first to the second is
    * summed over a trailing window. Read it as a bulge detector, not a
    * direction. A ratio above one means the range is widening faster than its
    * own smoothing can absorb, so the sum rises; a narrowing range pulls it
    * back down. Dorsey's own rule is the "reversal bulge": the index rising
    * above 27, then falling back under 26.5, warns that the prevailing trend is
    * about to reverse. Which way it reverses has to come from a trend
    * indicator, because the Mass Index has no sign of its own.
    * <p><b>Formula</b>
    * <pre>{@code
    * HL = high - low
    * single = EMA( HL, optInFastPeriod )
    * double = EMA( single, optInFastPeriod )
    * MASSI = SUM( single / double, optInSlowPeriod )
    * Both averages are the standard TA-Lib EMA: smoothing factor 2 / (optInFastPeriod + 1), seeded with the simple average of the first optInFastPeriod inputs of that stage.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The two periods are not interchangeable and are never swapped: {@code optInFastPeriod} is the length of both exponential averages, {@code optInSlowPeriod} the length of the summation window. Some implementations reorder them when the summation window is the shorter of the two; this one does not.</li>
    * <li>A window in which every bar is exactly flat, high equal to low, leaves both averages at zero. The ratio is reported as 1 there, its continuous limit, so a flat market yields exactly {@code optInSlowPeriod} rather than a spurious zero.</li>
    * <li>Implementations disagree on how the exponential averages are seeded. TA-Lib uses its own EMA convention, the simple average of the first {@code optInFastPeriod} inputs, where Tulip Indicators, ta4j and trading-signals seed from a single raw value and converge to these values only after many bars. Published sample vectors, including the one in Achelis, are seeded that way and match only in the tail.</li>
    * <li>MASSI inherits EMA's unstable period rather than owning one, and inherits it twice: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, u)} moves the first output by 2u.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MASSI_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price.
    * @param inLow Low price.
    * @param optInFastPeriod Number of bars in each of the two exponential
    *        averages of the high-low range (default 9; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Number of bars the ratio is summed over (default
    *        25; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Summed ratio of the two smoothed high-low ranges. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CVI
    * @see Core#ATR
    * @see Core#NATR
    * @see Core#TRANGE
    * @see Core#EMA
    * @see Core#SUM
    */
   public OutRange MASSI( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          double outReal[] )
   {
      requireIndexRange("MASSI", startIdx, endIdx);
      int guardStart = clampedStart("MASSI", startIdx, MASSI_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MASSI", "inHigh", inHigh, guardInLen);
      requireLength("MASSI", "inLow", inLow, guardInLen);
      requireLength("MASSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MASSI_Impl(startIdx, endIdx, inHigh, inLow, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MASSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MASSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MASSI} over the same series.
    * Open with {@link Core#massiOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MassiStream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      double optInK_1;
      double ema1;
      double ema2;
      double total;
      int ratioRing_Idx;
      int maxIdx_ratioRing;
      int cbSize_ratioRing;
      double[] cb_ratioRing;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      MassiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MASSI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MassiStream( MassiStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInK_1 = other.optInK_1;
         this.ema1 = other.ema1;
         this.ema2 = other.ema2;
         this.total = other.total;
         this.ratioRing_Idx = other.ratioRing_Idx;
         this.maxIdx_ratioRing = other.maxIdx_ratioRing;
         this.cbSize_ratioRing = other.cbSize_ratioRing;
         this.cb_ratioRing = other.cb_ratioRing.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MASSI update: BadParam", RetCode.BadParam);
         }
         core.massiStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("MASSI updateAndFill", "inHigh", inHigh);
         requireArgument("MASSI updateAndFill", "inLow", inLow);
         requireArgument("MASSI updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("MASSI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("MASSI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.massiStepImpl(this, inHigh[i], inLow[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("MASSI peek: BadParam", RetCode.BadParam);
         MassiStream sp = this;
         double hl = 0.0;
         double ratio = 0.0;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         double ema1 = sp.ema1;
         double ema2 = sp.ema2;
         int ratioRing_Idx = sp.ratioRing_Idx;
         double total = sp.total;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         hl = inHigh - inLow;
         ema1 = Math.fma(hl - ema1, sp.optInK_1, ema1);
         ema2 = Math.fma(ema1 - ema2, sp.optInK_1, ema2);
         if( ema2 == 0.0 ) {
            ratio = 1.0;
         } else {
            ratio = ema1 / ema2;
         }
         pkSlot0 = ratioRing_Idx;
         pkVal0 = ratio;
         total = total + ratio;
         ratioRing_Idx = ratioRing_Idx + 1;
         if( ratioRing_Idx > sp.maxIdx_ratioRing ) {
            ratioRing_Idx = 0;
         }
         tempReal = total;
         total = total - ((ratioRing_Idx != pkSlot0) ? sp.cb_ratioRing[ratioRing_Idx] : pkVal0);
         cur_outReal = tempReal;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public MassiStream clone() {
         return new MassiStream(this);
      }
   }
   void massiStepImpl( MassiStream sp, double inHigh, double inLow )
   {
      double hl = 0.0;
      double ratio = 0.0;
      double tempReal = 0.0;
      hl = inHigh - inLow;
      sp.ema1 = Math.fma(hl - sp.ema1, sp.optInK_1, sp.ema1);
      sp.ema2 = Math.fma(sp.ema1 - sp.ema2, sp.optInK_1, sp.ema2);
      if( sp.ema2 == 0.0 ) {
         ratio = 1.0;
      } else {
         ratio = sp.ema1 / sp.ema2;
      }
      sp.cb_ratioRing[sp.ratioRing_Idx] = ratio;
      sp.total = sp.total + ratio;
      sp.ratioRing_Idx = sp.ratioRing_Idx + 1;
      if( sp.ratioRing_Idx > sp.maxIdx_ratioRing ) {
         sp.ratioRing_Idx = 0;
      }
      tempReal = sp.total;
      sp.total = sp.total - sp.cb_ratioRing[sp.ratioRing_Idx];
      sp.cur_outReal = tempReal;
   }
   private RetCode massiOpenImpl( MassiStream sp, double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double optInK_1 = 0;
      double hl = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double ratio = 0;
      double total = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int lookbackEma = 0;
      int lookbackEma2 = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      double[] ratioRing;
      int ratioRing_Idx = 0;
      int maxIdx_ratioRing = (32)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 9;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      lookbackEma = EMA_Lookback(optInFastPeriod);
      lookbackEma2 = lookbackEma * 2;
      lookbackTotal = lookbackEma2 + (optInSlowPeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      if( optInSlowPeriod < 1 ) return RetCode.InternalError;
      ratioRing = new double[optInSlowPeriod];
      maxIdx_ratioRing = (optInSlowPeriod)-1;
      ratioRing_Idx = 0;
      outBegIdx.value = startIdx;
      /* Dorsey's pipeline in one pass: EMA of the high-low range, EMA of that,
       * their ratio summed over a trailing window.
       *
       * Each stage seeds the way ema.c does -- a simple average of that stage's
       * first optInFastPeriod inputs -- and its boundary below is the callee
       * LOOKBACK, never (optInFastPeriod - 1). The two coincide exactly at
       * unstable period 0, which is where every cross-language gate runs, so
       * confusing them is invisible until TA_SetUnstablePeriod(TA_FUNC_UNST_EMA)
       * is warmed. The seed sums accumulate from 0.0 in production order; do not
       * reorder or fuse them (0.0+x is not x for x=-0.0).
       *
       * Seed from the SMA arm only: ema.c's TA_COMPATIBILITY_METASTOCK arm is
       * unreachable from the Rust, Java and C# APIs, so consulting
       * TA_GetCompatibility() here would make C diverge from three backends for a
       * setting they cannot read.
       */
      optInK_1 = 2.0 / (double)(optInFastPeriod + 1);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      total = 0.0;
      tempReal = 0.0;
      today = startIdx - lookbackTotal;
      nBar = 0;
      /* Runs through startIdx inclusive: that last pass fills the summation
       * window and so produces the first output.
       */
      while( today <= startIdx ) {
         hl = inHigh[today] - inLow[today];
         if( nBar < optInFastPeriod ) {
            sum1 = sum1 + hl;
            if( nBar == optInFastPeriod - 1 ) {
               ema1 = sum1 / optInFastPeriod;
            }
         } else {
            ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         }
         /* The stage counter is compared BEFORE it is subtracted, never after.
          * `n2 = nBar - lookbackEma; if( n2 >= 0 )` is correct in C and broken
          * everywhere else: the Rust backend renders these as usize, so the
          * subtraction underflows for the first lookbackEma bars.
          */
         if( nBar >= lookbackEma ) {
            n2 = nBar - lookbackEma;
            if( n2 < optInFastPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInFastPeriod - 1 ) {
                  ema2 = sum2 / optInFastPeriod;
               }
            } else {
               ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
            }
         }
         if( nBar >= lookbackEma2 ) {
            /* A flat market is the ratio's continuous limit, 1.0, not the zero
             * that an oscillator centred on zero would report: MASSI's own
             * neutral is optInSlowPeriod. Test ema2 exactly and never through an
             * epsilon band -- a smoothed price range carries the quote unit, so a
             * fixed band would pin the whole index at optInSlowPeriod for any
             * instrument quoted under it (issue #253).
             */
            if( ema2 == 0.0 ) {
               ratio = 1.0;
            } else {
               ratio = ema1 / ema2;
            }
            /* TA_SUM's accumulation order -- add, publish, subtract -- reproduced
             * over a ring: the slot written here was emptied out of `total` at the
             * end of the previous bar, so nothing has to be read before the store.
             */
            ratioRing[ratioRing_Idx] = ratio;
            total = total + ratio;
            ratioRing_Idx++;
            if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
            if( nBar == lookbackTotal ) {
               tempReal = total;
               total = total - ratioRing[ratioRing_Idx];
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      /* In-place safe: this store lands lookbackTotal bars behind the input
       * cursor, so no bar is written under a read still owed to it.
       */
      outReal[0 * outStride] = tempReal;
      outIdx = 1;
      while( today <= endIdx ) {
         hl = inHigh[today] - inLow[today];
         ema1 = Math.fma(hl - ema1, optInK_1, ema1);
         ema2 = Math.fma(ema1 - ema2, optInK_1, ema2);
         if( ema2 == 0.0 ) {
            ratio = 1.0;
         } else {
            ratio = ema1 / ema2;
         }
         ratioRing[ratioRing_Idx] = ratio;
         total = total + ratio;
         ratioRing_Idx++;
         if( ratioRing_Idx > maxIdx_ratioRing ) { ratioRing_Idx = 0; }
         tempReal = total;
         total = total - ratioRing[ratioRing_Idx];
         outReal[outIdx * outStride] = tempReal;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_ratioRing = maxIdx_ratioRing + 1;
      if( capCb_ratioRing > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInK_1 = optInK_1;
      sp.ema1 = ema1;
      sp.ema2 = ema2;
      sp.total = total;
      sp.ratioRing_Idx = ratioRing_Idx;
      sp.maxIdx_ratioRing = maxIdx_ratioRing;
      sp.cbSize_ratioRing = capCb_ratioRing;
      sp.cb_ratioRing = ratioRing;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* massiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MassiStream massiOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MassiStream sp = new MassiStream(this);
      RetCode retCode = massiOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MASSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MASSI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MASSI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind massiOpen (composition seam). */
   MassiStream massiOpenInternal( double inHigh[], double inLow[], int startIdx, int optInFastPeriod, int optInSlowPeriod )
   {
      MassiStream sp = new MassiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = massiOpenImpl(sp, inHigh, inLow, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MASSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MASSI open: internal error", retCode);
      }
      throw new TaLibArgumentException("MASSI open: " + retCode, retCode);
   }
   /**
    * Open a live MASSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MASSI} at that bar.
    * <p>The history must hold at least {@code MASSI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MassiStream massiOpen( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod )
   {
      requireArgument("MASSI open", "inHigh", inHigh);
      requireHistory("MASSI open", inHigh.length);
      requireArgument("MASSI open", "inLow", inLow);
      requireHistoryLength("MASSI open", "inLow", inLow.length, inHigh.length);
      return massiOpenInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod);
   }
   /**
    * {@link Core#massiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MASSI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MassiStream#outRange()}.
    */
   public MassiStream massiOpenAndFill( double inHigh[], double inLow[], int optInFastPeriod, int optInSlowPeriod, double outReal[] )
   {
      requireArgument("MASSI openAndFill", "inHigh", inHigh);
      requireHistory("MASSI openAndFill", inHigh.length);
      requireArgument("MASSI openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("MASSI openAndFill", inHigh.length, MASSI_Lookback(optInFastPeriod, optInSlowPeriod));
      requireHistoryLength("MASSI openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("MASSI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("MASSI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return massiOpenAndFillInternal(inHigh, inLow, 0, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
   }
