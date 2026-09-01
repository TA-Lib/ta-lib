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
 *  083126 MF,CC  First version (issue #273).
 */

   /**
    * Number of leading input bars {@link Core#KC} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Smoothing period of the typical price moving
    *        average (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInATRPeriod Smoothing period of the Average True Range (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the Average True Range (default 2;
    *        {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int KC_Lookback( int optInTimePeriod, int optInATRPeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInATRPeriod == Integer.MIN_VALUE ) {
         optInATRPeriod = 10;
      } else if( optInATRPeriod < 1 || optInATRPeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return -1;
      }
      int emaLookback;
      int atrLookback;
      /* A band value needs BOTH the centre line and the ATR at the same bar, so the
       * first valid output is the later of the two lookbacks. Each term is exactly
       * the lookback of the function it comes from and is never restated here, which
       * is what makes KC inherit TA_FUNC_UNST_EMA and TA_FUNC_UNST_ATR from its two
       * callees. Reporting the honest max keeps outBegIdx == lookback (issue #99),
       * which streaming's Open depends on.
       */
      emaLookback = EMA_Lookback(optInTimePeriod);
      atrLookback = ATR_Lookback(optInATRPeriod);
      return (emaLookback > atrLookback) ? emaLookback : atrLookback ;

   }
   RetCode KC_Impl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    double inClose[],
                    int optInTimePeriod,
                    int optInATRPeriod,
                    double optInNbDev,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outRealUpperBand[],
                    double outRealMiddleBand[],
                    double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      int emaLookback = 0;
      int tpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double tempReal = 0;
      double middle = 0;
      double[] tempTP;
      double[] tempATR;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInATRPeriod == Integer.MIN_VALUE ) {
         optInATRPeriod = 10;
      } else if( optInATRPeriod < 1 || optInATRPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      emaLookback = EMA_Lookback(optInTimePeriod);
      lookbackTotal = KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev);
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything, so that a caller-supplied input which stops short of
       * endIdx is never read past its end.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Each leg is entered at its OWN lookback, so each is the shipped function
       * over this range and nothing here over-warms the shorter one: a caller who
       * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
       * calling TA_ATR directly. The typical price is a derived input, so it is
       * materialized only over the window the moving average reads.
       */
      tpStartIdx = startIdx - emaLookback;
      tempTP = new double[(int)((endIdx - tpStartIdx + 1) * 1)];
      tempATR = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = TYPPRICE(tpStartIdx, endIdx, inHigh, inLow, inClose, tempTP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.Success;
      /* The ATR consumes the price inputs before the moving average below writes
       * the middle band, which may be aliased onto one of them.
       */
      OutRange _xr1 = ATR(startIdx, endIdx, inHigh, inLow, inClose, optInATRPeriod, tempATR);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.Success;
      /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
       * own lookback puts its first output on startIdx, where the ATR's already is.
       */
      OutRange _xr2 = EMA(emaLookback, endIdx - tpStartIdx, tempTP, optInTimePeriod, outRealMiddleBand);
      outBegIdx.value = _xr2.begIdx();
      outNBElement.value = _xr2.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = outRealMiddleBand[i];
         tempReal = tempATR[i] * optInNbDev;
         outRealUpperBand[i] = middle + tempReal;
         outRealLowerBand[i] = middle - tempReal;
      }
      return RetCode.Success ;
   }
   RetCode KC_Impl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    float inClose[],
                    int optInTimePeriod,
                    int optInATRPeriod,
                    double optInNbDev,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outRealUpperBand[],
                    double outRealMiddleBand[],
                    double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      int emaLookback = 0;
      int tpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double tempReal = 0;
      double middle = 0;
      double[] tempTP;
      double[] tempATR;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInATRPeriod == Integer.MIN_VALUE ) {
         optInATRPeriod = 10;
      } else if( optInATRPeriod < 1 || optInATRPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      emaLookback = EMA_Lookback(optInTimePeriod);
      lookbackTotal = KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      tpStartIdx = startIdx - emaLookback;
      tempTP = new double[(int)((endIdx - tpStartIdx + 1) * 1)];
      tempATR = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = TYPPRICE(tpStartIdx, endIdx, inHigh, inLow, inClose, tempTP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.Success;
      OutRange _xr1 = ATR(startIdx, endIdx, inHigh, inLow, inClose, optInATRPeriod, tempATR);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.Success;
      OutRange _xr2 = EMA(emaLookback, endIdx - tpStartIdx, tempTP, optInTimePeriod, outRealMiddleBand);
      outBegIdx.value = _xr2.begIdx();
      outNBElement.value = _xr2.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = outRealMiddleBand[i];
         tempReal = tempATR[i] * optInNbDev;
         outRealUpperBand[i] = middle + tempReal;
         outRealLowerBand[i] = middle - tempReal;
      }
      return RetCode.Success ;
   }
   /**
    * Keltner Channels: three overlap lines around price. The centre line is an
    * exponential moving average of the typical price; the outer bands sit a
    * multiple of the Average True Range above and below it. The band width
    * tracks volatility, so the channel widens in fast markets and narrows in
    * quiet ones.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP = (High + Low + Close) / 3
    * Middle = EMA(TP, N)
    * Band = ATR(M)
    * Upper = Middle + Deviations * Band
    * Lower = Middle - Deviations * Band
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Several incompatible indicators are published under the name "Keltner Channel", disagreeing by percent rather than by rounding. This is the typical-price centre line with a Wilder-smoothed Average True Range band, the form implemented by TTR and ta4j.</li>
    * <li>Chester Keltner's 1960 original smooths the typical price with a simple moving average and takes the band from the plain daily range; the widely charted modern variant centres on the close instead. Expect a visible difference against a package plotting either.</li>
    * <li>TTR ties the Average True Range period to the centre line's period. Here the two are independent, so the band width can be tuned separately.</li>
    * <li>The centre line and the band are separate recursions, each with its own warm-up. They are entered at their own lookbacks, so a caller who wants either one converged sets that function's unstable period — {@code TA_FUNC_UNST_EMA} for the centre line, {@code TA_FUNC_UNST_ATR} for the band.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KC_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period of the typical price moving
    *        average (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInATRPeriod Smoothing period of the Average True Range (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the Average True Range (default 2;
    *        {@code -4e37} selects the default).
    * @param outRealUpperBand Centre line plus the scaled Average True Range.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand Exponential moving average of the typical price.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand Centre line minus the scaled Average True Range.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#ATR
    * @see Core#TYPPRICE
    * @see Core#BBANDS
    * @see Core#ACCBANDS
    */
   public OutRange KC( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInTimePeriod,
                       int optInATRPeriod,
                       double optInNbDev,
                       double outRealUpperBand[],
                       double outRealMiddleBand[],
                       double outRealLowerBand[] )
   {
      requireIndexRange("KC", startIdx, endIdx);
      int guardStart = clampedStart("KC", startIdx, KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KC", "inHigh", inHigh, guardInLen);
      requireLength("KC", "inLow", inLow, guardInLen);
      requireLength("KC", "inClose", inClose, guardInLen);
      requireLength("KC", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("KC", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("KC", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KC_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("KC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Keltner Channels: three overlap lines around price. The centre line is an
    * exponential moving average of the typical price; the outer bands sit a
    * multiple of the Average True Range above and below it. The band width
    * tracks volatility, so the channel widens in fast markets and narrows in
    * quiet ones.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP = (High + Low + Close) / 3
    * Middle = EMA(TP, N)
    * Band = ATR(M)
    * Upper = Middle + Deviations * Band
    * Lower = Middle - Deviations * Band
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Several incompatible indicators are published under the name "Keltner Channel", disagreeing by percent rather than by rounding. This is the typical-price centre line with a Wilder-smoothed Average True Range band, the form implemented by TTR and ta4j.</li>
    * <li>Chester Keltner's 1960 original smooths the typical price with a simple moving average and takes the band from the plain daily range; the widely charted modern variant centres on the close instead. Expect a visible difference against a package plotting either.</li>
    * <li>TTR ties the Average True Range period to the centre line's period. Here the two are independent, so the band width can be tuned separately.</li>
    * <li>The centre line and the band are separate recursions, each with its own warm-up. They are entered at their own lookbacks, so a caller who wants either one converged sets that function's unstable period — {@code TA_FUNC_UNST_EMA} for the centre line, {@code TA_FUNC_UNST_ATR} for the band.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KC_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period of the typical price moving
    *        average (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInATRPeriod Smoothing period of the Average True Range (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the Average True Range (default 2;
    *        {@code -4e37} selects the default).
    * @param outRealUpperBand Centre line plus the scaled Average True Range.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand Exponential moving average of the typical price.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand Centre line minus the scaled Average True Range.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#EMA
    * @see Core#ATR
    * @see Core#TYPPRICE
    * @see Core#BBANDS
    * @see Core#ACCBANDS
    */
   public OutRange KC( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInTimePeriod,
                       int optInATRPeriod,
                       double optInNbDev,
                       double outRealUpperBand[],
                       double outRealMiddleBand[],
                       double outRealLowerBand[] )
   {
      requireIndexRange("KC", startIdx, endIdx);
      int guardStart = clampedStart("KC", startIdx, KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KC", "inHigh", inHigh, guardInLen);
      requireLength("KC", "inLow", inLow, guardInLen);
      requireLength("KC", "inClose", inClose, guardInLen);
      requireLength("KC", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("KC", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("KC", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KC_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("KC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#KC} over the same series.
    * Open with {@link Core#kcOpen}; there is no close — the handle is
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
   public static final class KcStream {
      Core core;
      int optInTimePeriod;
      int optInATRPeriod;
      double optInNbDev;
      double cur_outRealUpperBand;
      double cur_outRealMiddleBand;
      double cur_outRealLowerBand;
      TyppriceStream sub0;
      AtrStream sub1;
      EmaStream sub2;
      int outRangeBegIdx;
      int outRangeCount;

      KcStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#KC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      KcStream( KcStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInATRPeriod = other.optInATRPeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.sub0 = new TyppriceStream(other.sub0);
         this.sub1 = new AtrStream(other.sub1);
         this.sub2 = new EmaStream(other.sub2);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(KcOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, KcOut out ) {
         requireArgument("KC update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("KC update: BadParam", RetCode.BadParam);
         }
         core.kcStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] ) {
         requireArgument("KC updateAndFill", "inHigh", inHigh);
         requireArgument("KC updateAndFill", "inLow", inLow);
         requireArgument("KC updateAndFill", "inClose", inClose);
         requireArgument("KC updateAndFill", "outRealUpperBand", outRealUpperBand);
         requireArgument("KC updateAndFill", "outRealMiddleBand", outRealMiddleBand);
         requireArgument("KC updateAndFill", "outRealLowerBand", outRealLowerBand);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outRealUpperBand.length < barCount || outRealMiddleBand.length < barCount || outRealLowerBand.length < barCount || (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealUpperBand == (Object)inClose || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inClose || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealLowerBand == (Object)inClose || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand )
            throw new TaLibArgumentException("KC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("KC updateAndFill: BadParam", RetCode.BadParam);
            }
            core.kcStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outRealUpperBand[i] = this.cur_outRealUpperBand;
            outRealMiddleBand[i] = this.cur_outRealMiddleBand;
            outRealLowerBand[i] = this.cur_outRealLowerBand;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inHigh, double inLow, double inClose, KcOut out ) {
         requireArgument("KC peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("KC peek: BadParam", RetCode.BadParam);
         KcStream sp = this;
         double middle = 0.0;
         double tempReal = 0.0;
         double cur_tempTP = 0.0;
         double cur_tempATR = 0.0;
         double cur_outRealMiddleBand = 0.0;
         double cur_outRealUpperBand = 0.0;
         double cur_outRealLowerBand = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempTP = sp.sub0.peek(inHigh, inLow, inClose);
         cur_tempATR = sp.sub1.peek(inHigh, inLow, inClose);
         cur_outRealMiddleBand = sp.sub2.peek(cur_tempTP);
         /* Combine map (batch tail, per bar). */
         middle = cur_outRealMiddleBand;
         tempReal = cur_tempATR * sp.optInNbDev;
         cur_outRealUpperBand = middle + tempReal;
         cur_outRealLowerBand = middle - tempReal;
         out.realUpperBand = cur_outRealUpperBand;
         out.realMiddleBand = cur_outRealMiddleBand;
         out.realLowerBand = cur_outRealLowerBand;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( KcOut out ) {
         requireArgument("KC value", "out", out);
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public KcStream clone() {
         return new KcStream(this);
      }
   }

   /**
    * The outputs of one KC bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class KcOut {
      /** Centre line plus the scaled Average True Range. */
      public double realUpperBand;
      /** Exponential moving average of the typical price. */
      public double realMiddleBand;
      /** Centre line minus the scaled Average True Range. */
      public double realLowerBand;
   }
   void kcStepImpl( KcStream sp, double inHigh, double inLow, double inClose )
   {
      double middle = 0.0;
      double tempReal = 0.0;
      double cur_tempTP = 0.0;
      double cur_tempATR = 0.0;
      double cur_outRealMiddleBand = 0.0;
      double cur_outRealUpperBand = 0.0;
      double cur_outRealLowerBand = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempTP = sp.sub0.update(inHigh, inLow, inClose);
      cur_tempATR = sp.sub1.update(inHigh, inLow, inClose);
      cur_outRealMiddleBand = sp.sub2.update(cur_tempTP);
      /* Combine map (batch tail, per bar). */
      middle = cur_outRealMiddleBand;
      tempReal = cur_tempATR * sp.optInNbDev;
      cur_outRealUpperBand = middle + tempReal;
      cur_outRealLowerBand = middle - tempReal;
      sp.cur_outRealUpperBand = cur_outRealUpperBand;
      sp.cur_outRealMiddleBand = cur_outRealMiddleBand;
      sp.cur_outRealLowerBand = cur_outRealLowerBand;
   }
   private RetCode kcOpenImpl( KcStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInATRPeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      int emaLookback = 0;
      int tpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double tempReal = 0;
      double middle = 0;
      double[] tempTP;
      double[] tempATR;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInATRPeriod == Integer.MIN_VALUE ) {
         optInATRPeriod = 10;
      } else if( optInATRPeriod < 1 || optInATRPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outRealUpperBand = outStride == 1 ? outRealUpperBand : new double[historyLen];
      double[] sc_outRealMiddleBand = outStride == 1 ? outRealMiddleBand : new double[historyLen];
      double[] sc_outRealLowerBand = outStride == 1 ? outRealLowerBand : new double[historyLen];
      emaLookback = EMA_Lookback(optInTimePeriod);
      lookbackTotal = KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev);
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything, so that a caller-supplied input which stops short of
       * endIdx is never read past its end.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Each leg is entered at its OWN lookback, so each is the shipped function
       * over this range and nothing here over-warms the shorter one: a caller who
       * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
       * calling TA_ATR directly. The typical price is a derived input, so it is
       * materialized only over the window the moving average reads.
       */
      tpStartIdx = startIdx - emaLookback;
      tempTP = new double[(int)((endIdx - tpStartIdx + 1) * 1)];
      tempATR = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Sub-stream 0: typprice over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      TyppriceStream sub0 = typpriceOpenAndFillInternal(inHigh, inLow, inClose, tpStartIdx, tempBegIdx, tempNbElement, tempTP);
      retCode = RetCode.Success;
      /* The ATR consumes the price inputs before the moving average below writes
       * the middle band, which may be aliased onto one of them.
       */
      /* Sub-stream 1: atr over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      AtrStream sub1 = atrOpenAndFillInternal(inHigh, inLow, inClose, startIdx, optInATRPeriod, tempBegIdx, tempNbElement, tempATR);
      retCode = RetCode.Success;
      /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
       * own lookback puts its first output on startIdx, where the ATR's already is.
       */
      /* Sub-stream 2: ema over `tempTP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      EmaStream sub2 = emaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempTP, 0, (endIdx - tpStartIdx) + 1), emaLookback, optInTimePeriod, outBegIdx, outNBElement, sc_outRealMiddleBand);
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = sc_outRealMiddleBand[i];
         tempReal = tempATR[i] * optInNbDev;
         sc_outRealUpperBand[i] = middle + tempReal;
         sc_outRealLowerBand[i] = middle - tempReal;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInATRPeriod = optInATRPeriod;
      sp.optInNbDev = optInNbDev;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
      sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
      sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* kcOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KcStream kcOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInATRPeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      KcStream sp = new KcStream(this);
      RetCode retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("KC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind kcOpen (composition seam). */
   KcStream kcOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInATRPeriod, double optInNbDev )
   {
      KcStream sp = new KcStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealUpperBand = new double[1];
      double[] sink_outRealMiddleBand = new double[1];
      double[] sink_outRealLowerBand = new double[1];
      RetCode retCode = kcOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KC open: internal error", retCode);
      }
      throw new TaLibArgumentException("KC open: " + retCode, retCode);
   }
   /**
    * Open a live KC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#KC} at that bar.
    * <p>The history must hold at least {@code KC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KcStream kcOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, int optInATRPeriod, double optInNbDev )
   {
      requireArgument("KC open", "inHigh", inHigh);
      requireHistory("KC open", inHigh.length);
      requireArgument("KC open", "inLow", inLow);
      requireArgument("KC open", "inClose", inClose);
      requireHistoryLength("KC open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KC open", "inClose", inClose.length, inHigh.length);
      return kcOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInATRPeriod, optInNbDev);
   }
   /**
    * {@link Core#kcOpen} that also fills the output array(s) bit-identically
    * to {@link Core#KC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KcStream#outRange()}.
    */
   public KcStream kcOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, int optInATRPeriod, double optInNbDev, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      requireArgument("KC openAndFill", "inHigh", inHigh);
      requireHistory("KC openAndFill", inHigh.length);
      requireArgument("KC openAndFill", "inLow", inLow);
      requireArgument("KC openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("KC openAndFill", inHigh.length, KC_Lookback(optInTimePeriod, optInATRPeriod, optInNbDev));
      requireHistoryLength("KC openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KC openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("KC openAndFill", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("KC openAndFill", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("KC openAndFill", "outRealLowerBand", outRealLowerBand, guardOutLen);
      if( (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealUpperBand == (Object)inClose || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inClose || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealLowerBand == (Object)inClose || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         throw new TaLibArgumentException("KC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kcOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInATRPeriod, optInNbDev, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
   }
