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
 *  090426 MF,CC  First version (#360).
 */

   /**
    * Number of leading input bars {@link Core#TSI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFirstPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 13; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int TSI_Lookback( int optInFirstPeriod, int optInSecondPeriod )
   {
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 25;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return -1;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 13;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return -1;
      }
      /* One bar forms the first close-to-close change, then the two EMA warm-ups
       * the pipeline stacks on it. Each term is exactly the lookback of the
       * function it comes from, which is also what makes TSI inherit
       * TA_FUNC_UNST_EMA from its callee.
       */
      return 1 + EMA_Lookback(optInFirstPeriod) + EMA_Lookback(optInSecondPeriod) ;

   }
   RetCode TSI_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInFirstPeriod,
                     int optInSecondPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double kFirst = 0;
      double kSecond = 0;
      double emaFirstNum = 0;
      double emaFirstDen = 0;
      double emaSecondNum = 0;
      double emaSecondDen = 0;
      double sumFirstNum = 0;
      double sumFirstDen = 0;
      double sumSecondNum = 0;
      double sumSecondDen = 0;
      double prevClose = 0;
      double mom = 0;
      double absMom = 0;
      double tsiValue = 0;
      int lookbackTotal = 0;
      int lookbackFirst = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int nSecond = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 25;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 13;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = TSI_Lookback(optInFirstPeriod, optInSecondPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      /* Blau's double smoothing in one pass: the signed momentum and its
       * magnitude are carried through the same two EMA stages, then divided.
       *
       * Each stage seeds the way ema.c does -- a simple average of that stage's
       * first 'period' inputs -- so the result is bit-identical to TA_MOM(1)
       * followed by two TA_EMA calls on each chain. The stage boundary below is
       * the callee LOOKBACK, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second stage then
       * seeds from the values the first would have published. The seed sums
       * accumulate from 0.0 in production order and the recurrence is
       * ((x-prev)*k)+prev rather than the algebraically equal k*x+(1-k)*prev; do
       * not reorder or fuse them (0.0+x is not x for x=-0.0). That order IS the
       * bit-exactness contract against the composed reference.
       *
       * TA_GetCompatibility() is deliberately NOT consulted, for the reason
       * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
       * preserved for the functions that already shipped with it and dropped from
       * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
       *
       * prevClose is carried in a scalar rather than re-read from inReal[t-1]
       * because outReal may alias inReal: the slot holding close[t-1] may already
       * hold an output written a bar earlier.
       */
      kFirst = 2.0 / (double)(optInFirstPeriod + 1);
      kSecond = 2.0 / (double)(optInSecondPeriod + 1);
      lookbackFirst = EMA_Lookback(optInFirstPeriod);
      emaFirstNum = 0.0;
      emaFirstDen = 0.0;
      emaSecondNum = 0.0;
      emaSecondDen = 0.0;
      sumFirstNum = 0.0;
      sumFirstDen = 0.0;
      sumSecondNum = 0.0;
      sumSecondDen = 0.0;
      /* The first bar carrying a close-to-close change. */
      today = startIdx - lookbackTotal + 1;
      prevClose = inReal[today - 1];
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here completes
       * the second stage's seed, so it produces the first output.
       */
      while( today <= startIdx ) {
         mom = inReal[today] - prevClose;
         prevClose = inReal[today];
         absMom = Math.abs(mom);
         /* Stage 1: the first EMA, over the raw momentum and its magnitude. */
         if( nBar < optInFirstPeriod ) {
            sumFirstNum = sumFirstNum + mom;
            sumFirstDen = sumFirstDen + absMom;
            if( nBar == optInFirstPeriod - 1 ) {
               emaFirstNum = sumFirstNum / optInFirstPeriod;
               emaFirstDen = sumFirstDen / optInFirstPeriod;
            }
         } else {
            emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
            emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         }
         /* Stage 2: the second EMA, over what stage 1 publishes.
          *
          * The stage counter is compared BEFORE it is subtracted, never after.
          * Writing this as `nSecond = nBar - lookbackFirst; if( nSecond >= 0 )`
          * is correct in C, where the counters are signed, and broken everywhere
          * else: the Rust backend renders them as usize, so the subtraction
          * underflows for the first lookbackFirst bars -- a panic in a debug
          * build and a wrap in release. It would also be invisible to the
          * cross-language gate, which runs release servers at unstable period 0,
          * where the branch the wrap wrongly takes happens to be a no-op because
          * both accumulators are still 0.0. smi.c states the same rule.
          */
         if( nBar >= lookbackFirst ) {
            nSecond = nBar - lookbackFirst;
            if( nSecond < optInSecondPeriod ) {
               sumSecondNum = sumSecondNum + emaFirstNum;
               sumSecondDen = sumSecondDen + emaFirstDen;
               if( nSecond == optInSecondPeriod - 1 ) {
                  emaSecondNum = sumSecondNum / optInSecondPeriod;
                  emaSecondDen = sumSecondDen / optInSecondPeriod;
               }
            } else {
               emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
               emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      /* The denominator is an EMA of an EMA of |momentum|: every term is
       * non-negative and every weight positive, so it is zero only when every
       * change that reached it was exactly zero -- 0/0, since the numerator is
       * zero with it, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
       * convention. Tested exactly rather than against a fixed band: a price
       * change carries the quote unit, and TA_IS_ZERO zeroes the oscillator for
       * any instrument quoted below it (issue #253).
       */
      if( emaSecondDen > 0.0 ) {
         tsiValue = 100.0 * emaSecondNum / emaSecondDen;
      } else {
         tsiValue = 0.0;
      }
      outReal[0] = tsiValue;
      outIdx = 1;
      /* Stable zone. Both stages are a pure recursion from here on. */
      while( today <= endIdx ) {
         mom = inReal[today] - prevClose;
         prevClose = inReal[today];
         absMom = Math.abs(mom);
         emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
         emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
         emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
         if( emaSecondDen > 0.0 ) {
            tsiValue = 100.0 * emaSecondNum / emaSecondDen;
         } else {
            tsiValue = 0.0;
         }
         outReal[outIdx] = tsiValue;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode TSI_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInFirstPeriod,
                     int optInSecondPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double kFirst = 0;
      double kSecond = 0;
      double emaFirstNum = 0;
      double emaFirstDen = 0;
      double emaSecondNum = 0;
      double emaSecondDen = 0;
      double sumFirstNum = 0;
      double sumFirstDen = 0;
      double sumSecondNum = 0;
      double sumSecondDen = 0;
      double prevClose = 0;
      double mom = 0;
      double absMom = 0;
      double tsiValue = 0;
      int lookbackTotal = 0;
      int lookbackFirst = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int nSecond = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 25;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 13;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = TSI_Lookback(optInFirstPeriod, optInSecondPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      kFirst = 2.0 / (double)(optInFirstPeriod + 1);
      kSecond = 2.0 / (double)(optInSecondPeriod + 1);
      lookbackFirst = EMA_Lookback(optInFirstPeriod);
      emaFirstNum = 0.0;
      emaFirstDen = 0.0;
      emaSecondNum = 0.0;
      emaSecondDen = 0.0;
      sumFirstNum = 0.0;
      sumFirstDen = 0.0;
      sumSecondNum = 0.0;
      sumSecondDen = 0.0;
      today = startIdx - lookbackTotal + 1;
      prevClose = (double)inReal[today - 1];
      nBar = 0;
      while( today <= startIdx ) {
         mom = (double)inReal[today] - prevClose;
         prevClose = (double)inReal[today];
         absMom = Math.abs(mom);
         if( nBar < optInFirstPeriod ) {
            sumFirstNum = sumFirstNum + mom;
            sumFirstDen = sumFirstDen + absMom;
            if( nBar == optInFirstPeriod - 1 ) {
               emaFirstNum = sumFirstNum / optInFirstPeriod;
               emaFirstDen = sumFirstDen / optInFirstPeriod;
            }
         } else {
            emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
            emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         }
         if( nBar >= lookbackFirst ) {
            nSecond = nBar - lookbackFirst;
            if( nSecond < optInSecondPeriod ) {
               sumSecondNum = sumSecondNum + emaFirstNum;
               sumSecondDen = sumSecondDen + emaFirstDen;
               if( nSecond == optInSecondPeriod - 1 ) {
                  emaSecondNum = sumSecondNum / optInSecondPeriod;
                  emaSecondDen = sumSecondDen / optInSecondPeriod;
               }
            } else {
               emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
               emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      if( emaSecondDen > 0.0 ) {
         tsiValue = 100.0 * emaSecondNum / emaSecondDen;
      } else {
         tsiValue = 0.0;
      }
      outReal[0] = tsiValue;
      outIdx = 1;
      while( today <= endIdx ) {
         mom = (double)inReal[today] - prevClose;
         prevClose = (double)inReal[today];
         absMom = Math.abs(mom);
         emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
         emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
         emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
         if( emaSecondDen > 0.0 ) {
            tsiValue = 100.0 * emaSecondNum / emaSecondDen;
         } else {
            tsiValue = 0.0;
         }
         outReal[outIdx] = tsiValue;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * True Strength Index: William Blau's double-smoothed momentum oscillator.
    * The one-bar price change is smoothed twice with exponential averages, and
    * the same pair of averages is applied to the magnitude of that change; the
    * ratio of the two is scaled by 100. Dividing the smoothed signed momentum
    * by the smoothed absolute momentum normalises the reading, so the result is
    * bounded by -100 and +100 and comparable across instruments. The double
    * smoothing is what separates it from a raw momentum plot: the curve is
    * smooth enough to read while keeping far less lag than a single average of
    * the same total length. Zero is the reference line — positive means the
    * smoothed momentum is net upward, negative net downward — and its crossings
    * are the usual trade trigger. Extreme readings mark overbought and oversold
    * conditions, and divergence against price is the classic Blau reading. A
    * signal line is not part of the output; apply {@code EMA} to
    * {@code outReal} to obtain one, since no source agrees on its period.
    * <p><b>Formula</b>
    * <pre>{@code
    * m = close - previous close
    * TSI = 100 * EMA(EMA(m, firstPeriod), secondPeriod) / EMA(EMA(|m|, firstPeriod), secondPeriod)
    * The first period is applied first, to the raw change; the second smooths its result. The order matters: the two averages do not commute, because each is seeded from a simple average of its own inputs.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>An input whose every change is exactly zero leaves both the numerator and the denominator at zero. Rather than divide, TSI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up. Implementations seeding from a single first sample — trading-signals among them — differ over the transient and agree once it decays.</li>
    * <li>The parameters are named by the order they are applied in, not fast and slow. Blau's published pair applies the longer average first, the inverse of the differenced fast/slow pairs elsewhere in the library, so swapping them silently returns a different indicator with the same lookback.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TSI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInFirstPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 13; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal True Strength Index, -100 to +100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#SMI
    * @see Core#MACD
    * @see Core#CMO
    * @see Core#RSI
    */
   public OutRange TSI( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFirstPeriod,
                        int optInSecondPeriod,
                        double outReal[] )
   {
      requireIndexRange("TSI", startIdx, endIdx);
      int guardStart = clampedStart("TSI", startIdx, TSI_Lookback(optInFirstPeriod, optInSecondPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TSI", "inReal", inReal, guardInLen);
      requireLength("TSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TSI_Impl(startIdx, endIdx, inReal, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * True Strength Index: William Blau's double-smoothed momentum oscillator.
    * The one-bar price change is smoothed twice with exponential averages, and
    * the same pair of averages is applied to the magnitude of that change; the
    * ratio of the two is scaled by 100. Dividing the smoothed signed momentum
    * by the smoothed absolute momentum normalises the reading, so the result is
    * bounded by -100 and +100 and comparable across instruments. The double
    * smoothing is what separates it from a raw momentum plot: the curve is
    * smooth enough to read while keeping far less lag than a single average of
    * the same total length. Zero is the reference line — positive means the
    * smoothed momentum is net upward, negative net downward — and its crossings
    * are the usual trade trigger. Extreme readings mark overbought and oversold
    * conditions, and divergence against price is the classic Blau reading. A
    * signal line is not part of the output; apply {@code EMA} to
    * {@code outReal} to obtain one, since no source agrees on its period.
    * <p><b>Formula</b>
    * <pre>{@code
    * m = close - previous close
    * TSI = 100 * EMA(EMA(m, firstPeriod), secondPeriod) / EMA(EMA(|m|, firstPeriod), secondPeriod)
    * The first period is applied first, to the raw change; the second smooths its result. The order matters: the two averages do not commute, because each is seeded from a simple average of its own inputs.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>An input whose every change is exactly zero leaves both the numerator and the denominator at zero. Rather than divide, TSI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up. Implementations seeding from a single first sample — trading-signals among them — differ over the transient and agree once it decays.</li>
    * <li>The parameters are named by the order they are applied in, not fast and slow. Blau's published pair applies the longer average first, the inverse of the differenced fast/slow pairs elsewhere in the library, so swapping them silently returns a different indicator with the same lookback.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TSI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInFirstPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 13; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal True Strength Index, -100 to +100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#SMI
    * @see Core#MACD
    * @see Core#CMO
    * @see Core#RSI
    */
   public OutRange TSI( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFirstPeriod,
                        int optInSecondPeriod,
                        double outReal[] )
   {
      requireIndexRange("TSI", startIdx, endIdx);
      int guardStart = clampedStart("TSI", startIdx, TSI_Lookback(optInFirstPeriod, optInSecondPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TSI", "inReal", inReal, guardInLen);
      requireLength("TSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TSI_Impl(startIdx, endIdx, inReal, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live TSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#TSI} over the same series.
    * Open with {@link Core#tsiOpen}; there is no close — the handle is
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
   public static final class TsiStream {
      Core core;
      int optInFirstPeriod;
      int optInSecondPeriod;
      double kFirst;
      double kSecond;
      double emaFirstNum;
      double emaFirstDen;
      double emaSecondNum;
      double emaSecondDen;
      double prevClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      TsiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#TSI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      TsiStream( TsiStream other ) {
         this.core = other.core;
         this.optInFirstPeriod = other.optInFirstPeriod;
         this.optInSecondPeriod = other.optInSecondPeriod;
         this.kFirst = other.kFirst;
         this.kSecond = other.kSecond;
         this.emaFirstNum = other.emaFirstNum;
         this.emaFirstDen = other.emaFirstDen;
         this.emaSecondNum = other.emaSecondNum;
         this.emaSecondDen = other.emaSecondDen;
         this.prevClose = other.prevClose;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("TSI update: BadParam", RetCode.BadParam);
         }
         core.tsiStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("TSI updateAndFill", "inReal", inReal);
         requireArgument("TSI updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("TSI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("TSI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.tsiStepImpl(this, inReal[i]);
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("TSI peek: BadParam", RetCode.BadParam);
         TsiStream sp = this;
         double mom = 0.0;
         double absMom = 0.0;
         double tsiValue = 0.0;
         double cur_outReal = 0.0;
         double emaFirstDen = sp.emaFirstDen;
         double emaFirstNum = sp.emaFirstNum;
         double emaSecondDen = sp.emaSecondDen;
         double emaSecondNum = sp.emaSecondNum;
         double prevClose = sp.prevClose;
         mom = inReal - prevClose;
         prevClose = inReal;
         absMom = Math.abs(mom);
         emaFirstNum = Math.fma(mom - emaFirstNum, sp.kFirst, emaFirstNum);
         emaFirstDen = Math.fma(absMom - emaFirstDen, sp.kFirst, emaFirstDen);
         emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, sp.kSecond, emaSecondNum);
         emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, sp.kSecond, emaSecondDen);
         if( emaSecondDen > 0.0 ) {
            tsiValue = 100.0 * emaSecondNum / emaSecondDen;
         } else {
            tsiValue = 0.0;
         }
         cur_outReal = tsiValue;
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
      public TsiStream clone() {
         return new TsiStream(this);
      }
   }
   void tsiStepImpl( TsiStream sp, double inReal )
   {
      double mom = 0.0;
      double absMom = 0.0;
      double tsiValue = 0.0;
      mom = inReal - sp.prevClose;
      sp.prevClose = inReal;
      absMom = Math.abs(mom);
      sp.emaFirstNum = Math.fma(mom - sp.emaFirstNum, sp.kFirst, sp.emaFirstNum);
      sp.emaFirstDen = Math.fma(absMom - sp.emaFirstDen, sp.kFirst, sp.emaFirstDen);
      sp.emaSecondNum = Math.fma(sp.emaFirstNum - sp.emaSecondNum, sp.kSecond, sp.emaSecondNum);
      sp.emaSecondDen = Math.fma(sp.emaFirstDen - sp.emaSecondDen, sp.kSecond, sp.emaSecondDen);
      if( sp.emaSecondDen > 0.0 ) {
         tsiValue = 100.0 * sp.emaSecondNum / sp.emaSecondDen;
      } else {
         tsiValue = 0.0;
      }
      sp.cur_outReal = tsiValue;
   }
   private RetCode tsiOpenImpl( TsiStream sp, double inReal[], int startIdx, int optInFirstPeriod, int optInSecondPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double kFirst = 0;
      double kSecond = 0;
      double emaFirstNum = 0;
      double emaFirstDen = 0;
      double emaSecondNum = 0;
      double emaSecondDen = 0;
      double sumFirstNum = 0;
      double sumFirstDen = 0;
      double sumSecondNum = 0;
      double sumSecondDen = 0;
      double prevClose = 0;
      double mom = 0;
      double absMom = 0;
      double tsiValue = 0;
      int lookbackTotal = 0;
      int lookbackFirst = 0;
      int today = 0;
      int outIdx = 0;
      int nBar = 0;
      int nSecond = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 25;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 13;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      lookbackTotal = TSI_Lookback(optInFirstPeriod, optInSecondPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      /* Blau's double smoothing in one pass: the signed momentum and its
       * magnitude are carried through the same two EMA stages, then divided.
       *
       * Each stage seeds the way ema.c does -- a simple average of that stage's
       * first 'period' inputs -- so the result is bit-identical to TA_MOM(1)
       * followed by two TA_EMA calls on each chain. The stage boundary below is
       * the callee LOOKBACK, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second stage then
       * seeds from the values the first would have published. The seed sums
       * accumulate from 0.0 in production order and the recurrence is
       * ((x-prev)*k)+prev rather than the algebraically equal k*x+(1-k)*prev; do
       * not reorder or fuse them (0.0+x is not x for x=-0.0). That order IS the
       * bit-exactness contract against the composed reference.
       *
       * TA_GetCompatibility() is deliberately NOT consulted, for the reason
       * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
       * preserved for the functions that already shipped with it and dropped from
       * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
       *
       * prevClose is carried in a scalar rather than re-read from inReal[t-1]
       * because outReal may alias inReal: the slot holding close[t-1] may already
       * hold an output written a bar earlier.
       */
      kFirst = 2.0 / (double)(optInFirstPeriod + 1);
      kSecond = 2.0 / (double)(optInSecondPeriod + 1);
      lookbackFirst = EMA_Lookback(optInFirstPeriod);
      emaFirstNum = 0.0;
      emaFirstDen = 0.0;
      emaSecondNum = 0.0;
      emaSecondDen = 0.0;
      sumFirstNum = 0.0;
      sumFirstDen = 0.0;
      sumSecondNum = 0.0;
      sumSecondDen = 0.0;
      /* The first bar carrying a close-to-close change. */
      today = startIdx - lookbackTotal + 1;
      prevClose = inReal[today - 1];
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here completes
       * the second stage's seed, so it produces the first output.
       */
      while( today <= startIdx ) {
         mom = inReal[today] - prevClose;
         prevClose = inReal[today];
         absMom = Math.abs(mom);
         /* Stage 1: the first EMA, over the raw momentum and its magnitude. */
         if( nBar < optInFirstPeriod ) {
            sumFirstNum = sumFirstNum + mom;
            sumFirstDen = sumFirstDen + absMom;
            if( nBar == optInFirstPeriod - 1 ) {
               emaFirstNum = sumFirstNum / optInFirstPeriod;
               emaFirstDen = sumFirstDen / optInFirstPeriod;
            }
         } else {
            emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
            emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         }
         /* Stage 2: the second EMA, over what stage 1 publishes.
          *
          * The stage counter is compared BEFORE it is subtracted, never after.
          * Writing this as `nSecond = nBar - lookbackFirst; if( nSecond >= 0 )`
          * is correct in C, where the counters are signed, and broken everywhere
          * else: the Rust backend renders them as usize, so the subtraction
          * underflows for the first lookbackFirst bars -- a panic in a debug
          * build and a wrap in release. It would also be invisible to the
          * cross-language gate, which runs release servers at unstable period 0,
          * where the branch the wrap wrongly takes happens to be a no-op because
          * both accumulators are still 0.0. smi.c states the same rule.
          */
         if( nBar >= lookbackFirst ) {
            nSecond = nBar - lookbackFirst;
            if( nSecond < optInSecondPeriod ) {
               sumSecondNum = sumSecondNum + emaFirstNum;
               sumSecondDen = sumSecondDen + emaFirstDen;
               if( nSecond == optInSecondPeriod - 1 ) {
                  emaSecondNum = sumSecondNum / optInSecondPeriod;
                  emaSecondDen = sumSecondDen / optInSecondPeriod;
               }
            } else {
               emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
               emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
            }
         }
         nBar = nBar + 1;
         today = today + 1;
      }
      /* The denominator is an EMA of an EMA of |momentum|: every term is
       * non-negative and every weight positive, so it is zero only when every
       * change that reached it was exactly zero -- 0/0, since the numerator is
       * zero with it, reported as the neutral 0.0 by the CCI (#7) and IMI (#112)
       * convention. Tested exactly rather than against a fixed band: a price
       * change carries the quote unit, and TA_IS_ZERO zeroes the oscillator for
       * any instrument quoted below it (issue #253).
       */
      if( emaSecondDen > 0.0 ) {
         tsiValue = 100.0 * emaSecondNum / emaSecondDen;
      } else {
         tsiValue = 0.0;
      }
      outReal[0 * outStride] = tsiValue;
      outIdx = 1;
      /* Stable zone. Both stages are a pure recursion from here on. */
      while( today <= endIdx ) {
         mom = inReal[today] - prevClose;
         prevClose = inReal[today];
         absMom = Math.abs(mom);
         emaFirstNum = Math.fma(mom - emaFirstNum, kFirst, emaFirstNum);
         emaFirstDen = Math.fma(absMom - emaFirstDen, kFirst, emaFirstDen);
         emaSecondNum = Math.fma(emaFirstNum - emaSecondNum, kSecond, emaSecondNum);
         emaSecondDen = Math.fma(emaFirstDen - emaSecondDen, kSecond, emaSecondDen);
         if( emaSecondDen > 0.0 ) {
            tsiValue = 100.0 * emaSecondNum / emaSecondDen;
         } else {
            tsiValue = 0.0;
         }
         outReal[outIdx * outStride] = tsiValue;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFirstPeriod = optInFirstPeriod;
      sp.optInSecondPeriod = optInSecondPeriod;
      sp.kFirst = kFirst;
      sp.kSecond = kSecond;
      sp.emaFirstNum = emaFirstNum;
      sp.emaFirstDen = emaFirstDen;
      sp.emaSecondNum = emaSecondNum;
      sp.emaSecondDen = emaSecondDen;
      sp.prevClose = prevClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* tsiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   TsiStream tsiOpenAndFillInternal( double inReal[], int startIdx, int optInFirstPeriod, int optInSecondPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TsiStream sp = new TsiStream(this);
      RetCode retCode = tsiOpenImpl(sp, inReal, startIdx, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TSI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("TSI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind tsiOpen (composition seam). */
   TsiStream tsiOpenInternal( double inReal[], int startIdx, int optInFirstPeriod, int optInSecondPeriod )
   {
      TsiStream sp = new TsiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = tsiOpenImpl(sp, inReal, startIdx, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TSI open: internal error", retCode);
      }
      throw new TaLibArgumentException("TSI open: " + retCode, retCode);
   }
   /**
    * Open a live TSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#TSI} at that bar.
    * <p>The history must hold at least {@code TSI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public TsiStream tsiOpen( double inReal[], int optInFirstPeriod, int optInSecondPeriod )
   {
      requireArgument("TSI open", "inReal", inReal);
      requireHistory("TSI open", inReal.length);
      return tsiOpenInternal(inReal, 0, optInFirstPeriod, optInSecondPeriod);
   }
   /**
    * {@link Core#tsiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#TSI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link TsiStream#outRange()}.
    */
   public TsiStream tsiOpenAndFill( double inReal[], int optInFirstPeriod, int optInSecondPeriod, double outReal[] )
   {
      requireArgument("TSI openAndFill", "inReal", inReal);
      requireHistory("TSI openAndFill", inReal.length);
      int guardOutLen = openFillCount("TSI openAndFill", inReal.length, TSI_Lookback(optInFirstPeriod, optInSecondPeriod));
      requireLength("TSI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("TSI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return tsiOpenAndFillInternal(inReal, 0, optInFirstPeriod, optInSecondPeriod, outBegIdx, outNBElement, outReal);
   }
