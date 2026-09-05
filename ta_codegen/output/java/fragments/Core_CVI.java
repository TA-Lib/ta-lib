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
 *  090426 MF,CC  First version (issue #358).
 */

   /**
    * Number of leading input bars {@link Core#CVI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the exponential average of the
    *        high-low spread (default 10; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInROCPeriod How many bars back the percent change reaches
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CVI_Lookback( int optInTimePeriod, int optInROCPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInROCPeriod == Integer.MIN_VALUE ) {
         optInROCPeriod = 10;
      } else if( optInROCPeriod < 1 || optInROCPeriod > 100000 ) {
         return -1;
      }
      return EMA_Lookback(optInTimePeriod) + ROCP_Lookback(optInROCPeriod) ;

   }
   RetCode CVI_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     int optInTimePeriod,
                     int optInROCPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double prevEMA = 0;
      double laggedEMA = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] emaRing;
      int emaRing_Idx = 0;
      int maxIdx_emaRing = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROCPeriod == Integer.MIN_VALUE ) {
         optInROCPeriod = 10;
      } else if( optInROCPeriod < 1 || optInROCPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* CVI[t] = 100 * (E[t] - E[t-optInROCPeriod]) / E[t-optInROCPeriod], with E
       * an EMA of the high-low spread. The spread is never materialised and the
       * EMA is anchored optInROCPeriod bars behind startIdx.
       *
       * The arithmetic below is TA_EMA's and TA_ROCP's verbatim -- seed sum
       * accumulated from 0.0 in ascending bar order, ((x-prev)*k)+prev, and
       * 100*((a-b)/b) under an exact zero test. That is what makes this fused pass
       * bit-identical to composing TA_SUB, TA_EMA and TA_ROCP, which test_cvi.c
       * holds it to memcmp-exact; reshaping any of it breaks that silently. The
       * guard stays an exact `!= 0.0` and never TA_IS_ZERO -- an epsilon band
       * carries the quote unit and would zero the indicator for anything priced
       * under it (issue #253).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = EMA_Lookback(optInTimePeriod) + ROCP_Lookback(optInROCPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInROCPeriod < 1 ) return RetCode.InternalError;
      emaRing = new double[optInROCPeriod];
      maxIdx_emaRing = (optInROCPeriod)-1;
      emaRing_Idx = 0;
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inHigh[today] - inLow[today];
         today += 1;
      }
      prevEMA = tempReal / optInTimePeriod;
      /* The ring keeps only the newest optInROCPeriod values, so pushing every EMA
       * value from the seed bar on leaves exactly the lagged terms the output
       * loop reads.
       */
      emaRing[emaRing_Idx] = prevEMA;
      emaRing_Idx++;
      if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      while( today < startIdx ) {
         tempReal = inHigh[today] - inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      }
      /* Read the expiring slot before overwriting it: that is what makes the lag
       * exactly optInROCPeriod rather than one less.
       */
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = inHigh[today] - inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         laggedEMA = emaRing[emaRing_Idx];
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
         if( laggedEMA != 0.0 ) {
            outReal[outIdx++] = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode CVI_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     int optInTimePeriod,
                     int optInROCPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double prevEMA = 0;
      double laggedEMA = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] emaRing;
      int emaRing_Idx = 0;
      int maxIdx_emaRing = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROCPeriod == Integer.MIN_VALUE ) {
         optInROCPeriod = 10;
      } else if( optInROCPeriod < 1 || optInROCPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = EMA_Lookback(optInTimePeriod) + ROCP_Lookback(optInROCPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInROCPeriod < 1 ) return RetCode.InternalError;
      emaRing = new double[optInROCPeriod];
      maxIdx_emaRing = (optInROCPeriod)-1;
      emaRing_Idx = 0;
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inHigh[today] - (double)inLow[today];
         today += 1;
      }
      prevEMA = tempReal / optInTimePeriod;
      emaRing[emaRing_Idx] = prevEMA;
      emaRing_Idx++;
      if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      while( today < startIdx ) {
         tempReal = (double)inHigh[today] - (double)inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      }
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = (double)inHigh[today] - (double)inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         laggedEMA = emaRing[emaRing_Idx];
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
         if( laggedEMA != 0.0 ) {
            outReal[outIdx++] = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Chaikin's Volatility: Marc Chaikin's reading of how fast a market's daily
    * trading range is widening or narrowing. The high-low spread is smoothed by
    * an exponential moving average, and the indicator reports the percent that
    * average has changed over a lookback of its own. Read it as a rate of
    * expansion. Positive means the smoothed range is wider than it was;
    * negative means it has contracted. Chaikin's own interpretation is
    * contrarian on the fast side: a range that widens sharply over a short span
    * is typical of the panic near a market bottom, while a range that narrows
    * steadily over a long span is typical of a market topping out. It measures
    * range, not direction, so it says nothing about which way price is heading.
    * <p><b>Formula</b>
    * <pre>{@code
    * HL = high - low
    * E = EMA( HL, optInTimePeriod )
    * CVI = 100 * ( E - E[optInROCPeriod bars ago] ) / E[optInROCPeriod bars ago]
    * The inner average is the standard TA-Lib EMA: smoothing factor 2 / (optInTimePeriod + 1), seeded with the simple average of the first optInTimePeriod spreads.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The averaging length and the rate-of-change length are independent, as in Achelis's relay of the author ("an exponential moving average of the difference between the daily high and low prices ... then the percent that this moving average has changed over a specified time period") and in the MathWorks {@code chaikvolat} signature. Implementations that expose a single length are the special case where both are set to the same value.</li>
    * <li>Some vendors default the rate-of-change length to 12 rather than to Achelis's recommendation, which is the same for both lengths.</li>
    * <li>A window whose bars are all exactly flat, high equal to low, leaves the lagged average at zero. CVI reports 0 there. Tulip Indicators and pandas-ta-classic leave the division unguarded and emit NaN; trading-signals returns 0, as here.</li>
    * <li>Implementations disagree on how the inner EMA is seeded. TA-Lib uses its own EMA convention, the simple average of the first {@code optInTimePeriod} spreads, where Tulip Indicators and trading-signals seed from a single raw spread and converge to these values only after many bars.</li>
    * <li>CVI inherits EMA's unstable period rather than owning one: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} moves CVI's first output too.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price.
    * @param inLow Low price.
    * @param optInTimePeriod Number of bars in the exponential average of the
    *        high-low spread (default 10; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInROCPeriod How many bars back the percent change reaches
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Percent change of the smoothed high-low spread. Must hold
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
    * @see Core#ATR
    * @see Core#NATR
    * @see Core#TRANGE
    * @see Core#EMA
    * @see Core#ROCP
    */
   public OutRange CVI( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        int optInTimePeriod,
                        int optInROCPeriod,
                        double outReal[] )
   {
      requireIndexRange("CVI", startIdx, endIdx);
      int guardStart = clampedStart("CVI", startIdx, CVI_Lookback(optInTimePeriod, optInROCPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CVI", "inHigh", inHigh, guardInLen);
      requireLength("CVI", "inLow", inLow, guardInLen);
      requireLength("CVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CVI_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chaikin's Volatility: Marc Chaikin's reading of how fast a market's daily
    * trading range is widening or narrowing. The high-low spread is smoothed by
    * an exponential moving average, and the indicator reports the percent that
    * average has changed over a lookback of its own. Read it as a rate of
    * expansion. Positive means the smoothed range is wider than it was;
    * negative means it has contracted. Chaikin's own interpretation is
    * contrarian on the fast side: a range that widens sharply over a short span
    * is typical of the panic near a market bottom, while a range that narrows
    * steadily over a long span is typical of a market topping out. It measures
    * range, not direction, so it says nothing about which way price is heading.
    * <p><b>Formula</b>
    * <pre>{@code
    * HL = high - low
    * E = EMA( HL, optInTimePeriod )
    * CVI = 100 * ( E - E[optInROCPeriod bars ago] ) / E[optInROCPeriod bars ago]
    * The inner average is the standard TA-Lib EMA: smoothing factor 2 / (optInTimePeriod + 1), seeded with the simple average of the first optInTimePeriod spreads.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The averaging length and the rate-of-change length are independent, as in Achelis's relay of the author ("an exponential moving average of the difference between the daily high and low prices ... then the percent that this moving average has changed over a specified time period") and in the MathWorks {@code chaikvolat} signature. Implementations that expose a single length are the special case where both are set to the same value.</li>
    * <li>Some vendors default the rate-of-change length to 12 rather than to Achelis's recommendation, which is the same for both lengths.</li>
    * <li>A window whose bars are all exactly flat, high equal to low, leaves the lagged average at zero. CVI reports 0 there. Tulip Indicators and pandas-ta-classic leave the division unguarded and emit NaN; trading-signals returns 0, as here.</li>
    * <li>Implementations disagree on how the inner EMA is seeded. TA-Lib uses its own EMA convention, the simple average of the first {@code optInTimePeriod} spreads, where Tulip Indicators and trading-signals seed from a single raw spread and converge to these values only after many bars.</li>
    * <li>CVI inherits EMA's unstable period rather than owning one: {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} moves CVI's first output too.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price.
    * @param inLow Low price.
    * @param optInTimePeriod Number of bars in the exponential average of the
    *        high-low spread (default 10; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInROCPeriod How many bars back the percent change reaches
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Percent change of the smoothed high-low spread. Must hold
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
    * @see Core#ATR
    * @see Core#NATR
    * @see Core#TRANGE
    * @see Core#EMA
    * @see Core#ROCP
    */
   public OutRange CVI( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        int optInTimePeriod,
                        int optInROCPeriod,
                        double outReal[] )
   {
      requireIndexRange("CVI", startIdx, endIdx);
      int guardStart = clampedStart("CVI", startIdx, CVI_Lookback(optInTimePeriod, optInROCPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CVI", "inHigh", inHigh, guardInLen);
      requireLength("CVI", "inLow", inLow, guardInLen);
      requireLength("CVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CVI_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CVI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CVI} over the same series.
    * Open with {@link Core#cviOpen}; there is no close — the handle is
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
   public static final class CviStream {
      Core core;
      int optInTimePeriod;
      int optInROCPeriod;
      double prevEMA;
      double optInK_1;
      int emaRing_Idx;
      int maxIdx_emaRing;
      int cbSize_emaRing;
      double[] cb_emaRing;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CviStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CVI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CviStream( CviStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInROCPeriod = other.optInROCPeriod;
         this.prevEMA = other.prevEMA;
         this.optInK_1 = other.optInK_1;
         this.emaRing_Idx = other.emaRing_Idx;
         this.maxIdx_emaRing = other.maxIdx_emaRing;
         this.cbSize_emaRing = other.cbSize_emaRing;
         this.cb_emaRing = other.cb_emaRing.clone();
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
            throw new TaLibArgumentException("CVI update: BadParam", RetCode.BadParam);
         }
         core.cviStepImpl(this, inHigh, inLow);
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
         requireArgument("CVI updateAndFill", "inHigh", inHigh);
         requireArgument("CVI updateAndFill", "inLow", inLow);
         requireArgument("CVI updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("CVI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CVI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cviStepImpl(this, inHigh[i], inLow[i]);
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
            throw new TaLibArgumentException("CVI peek: BadParam", RetCode.BadParam);
         CviStream sp = this;
         double laggedEMA = 0.0;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         int emaRing_Idx = sp.emaRing_Idx;
         double prevEMA = sp.prevEMA;
         tempReal = inHigh - inLow;
         prevEMA = Math.fma(tempReal - prevEMA, sp.optInK_1, prevEMA);
         laggedEMA = sp.cb_emaRing[emaRing_Idx];
         emaRing_Idx = emaRing_Idx + 1;
         if( emaRing_Idx > sp.maxIdx_emaRing ) {
            emaRing_Idx = 0;
         }
         if( laggedEMA != 0.0 ) {
            cur_outReal = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
         } else {
            cur_outReal = 0.0;
         }
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
      public CviStream clone() {
         return new CviStream(this);
      }
   }
   void cviStepImpl( CviStream sp, double inHigh, double inLow )
   {
      double laggedEMA = 0.0;
      double tempReal = 0.0;
      tempReal = inHigh - inLow;
      sp.prevEMA = Math.fma(tempReal - sp.prevEMA, sp.optInK_1, sp.prevEMA);
      laggedEMA = sp.cb_emaRing[sp.emaRing_Idx];
      sp.cb_emaRing[sp.emaRing_Idx] = sp.prevEMA;
      sp.emaRing_Idx = sp.emaRing_Idx + 1;
      if( sp.emaRing_Idx > sp.maxIdx_emaRing ) {
         sp.emaRing_Idx = 0;
      }
      if( laggedEMA != 0.0 ) {
         sp.cur_outReal = 100.0 * ((sp.prevEMA - laggedEMA) / laggedEMA);
      } else {
         sp.cur_outReal = 0.0;
      }
   }
   private RetCode cviOpenImpl( CviStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInROCPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double prevEMA = 0;
      double laggedEMA = 0;
      double tempReal = 0;
      double optInK_1 = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] emaRing;
      int emaRing_Idx = 0;
      int maxIdx_emaRing = (32)-1;
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
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInROCPeriod == Integer.MIN_VALUE ) {
         optInROCPeriod = 10;
      } else if( optInROCPeriod < 1 || optInROCPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* CVI[t] = 100 * (E[t] - E[t-optInROCPeriod]) / E[t-optInROCPeriod], with E
       * an EMA of the high-low spread. The spread is never materialised and the
       * EMA is anchored optInROCPeriod bars behind startIdx.
       *
       * The arithmetic below is TA_EMA's and TA_ROCP's verbatim -- seed sum
       * accumulated from 0.0 in ascending bar order, ((x-prev)*k)+prev, and
       * 100*((a-b)/b) under an exact zero test. That is what makes this fused pass
       * bit-identical to composing TA_SUB, TA_EMA and TA_ROCP, which test_cvi.c
       * holds it to memcmp-exact; reshaping any of it breaks that silently. The
       * guard stays an exact `!= 0.0` and never TA_IS_ZERO -- an epsilon band
       * carries the quote unit and would zero the indicator for anything priced
       * under it (issue #253).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = EMA_Lookback(optInTimePeriod) + ROCP_Lookback(optInROCPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      if( optInROCPeriod < 1 ) return RetCode.InternalError;
      emaRing = new double[optInROCPeriod];
      maxIdx_emaRing = (optInROCPeriod)-1;
      emaRing_Idx = 0;
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inHigh[today] - inLow[today];
         today += 1;
      }
      prevEMA = tempReal / optInTimePeriod;
      /* The ring keeps only the newest optInROCPeriod values, so pushing every EMA
       * value from the seed bar on leaves exactly the lagged terms the output
       * loop reads.
       */
      emaRing[emaRing_Idx] = prevEMA;
      emaRing_Idx++;
      if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      while( today < startIdx ) {
         tempReal = inHigh[today] - inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
      }
      /* Read the expiring slot before overwriting it: that is what makes the lag
       * exactly optInROCPeriod rather than one less.
       */
      outIdx = 0;
      while( today <= endIdx ) {
         tempReal = inHigh[today] - inLow[today];
         prevEMA = Math.fma(tempReal - prevEMA, optInK_1, prevEMA);
         today += 1;
         laggedEMA = emaRing[emaRing_Idx];
         emaRing[emaRing_Idx] = prevEMA;
         emaRing_Idx++;
         if( emaRing_Idx > maxIdx_emaRing ) { emaRing_Idx = 0; }
         if( laggedEMA != 0.0 ) {
            outReal[outIdx++ * outStride] = 100.0 * ((prevEMA - laggedEMA) / laggedEMA);
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_emaRing = maxIdx_emaRing + 1;
      if( capCb_emaRing > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInROCPeriod = optInROCPeriod;
      sp.prevEMA = prevEMA;
      sp.optInK_1 = optInK_1;
      sp.emaRing_Idx = emaRing_Idx;
      sp.maxIdx_emaRing = maxIdx_emaRing;
      sp.cbSize_emaRing = capCb_emaRing;
      sp.cb_emaRing = emaRing;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cviOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CviStream cviOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInROCPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CviStream sp = new CviStream(this);
      RetCode retCode = cviOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CVI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CVI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cviOpen (composition seam). */
   CviStream cviOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInROCPeriod )
   {
      CviStream sp = new CviStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = cviOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CVI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CVI open: internal error", retCode);
      }
      throw new TaLibArgumentException("CVI open: " + retCode, retCode);
   }
   /**
    * Open a live CVI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CVI} at that bar.
    * <p>The history must hold at least {@code CVI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CviStream cviOpen( double inHigh[], double inLow[], int optInTimePeriod, int optInROCPeriod )
   {
      requireArgument("CVI open", "inHigh", inHigh);
      requireHistory("CVI open", inHigh.length);
      requireArgument("CVI open", "inLow", inLow);
      requireHistoryLength("CVI open", "inLow", inLow.length, inHigh.length);
      return cviOpenInternal(inHigh, inLow, 0, optInTimePeriod, optInROCPeriod);
   }
   /**
    * {@link Core#cviOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CVI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CviStream#outRange()}.
    */
   public CviStream cviOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, int optInROCPeriod, double outReal[] )
   {
      requireArgument("CVI openAndFill", "inHigh", inHigh);
      requireHistory("CVI openAndFill", inHigh.length);
      requireArgument("CVI openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("CVI openAndFill", inHigh.length, CVI_Lookback(optInTimePeriod, optInROCPeriod));
      requireHistoryLength("CVI openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("CVI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("CVI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cviOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, optInROCPeriod, outBegIdx, outNBElement, outReal);
   }
