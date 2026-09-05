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
 *  090426 MF,CC  First version (issue #365).
 */

   /**
    * Number of leading input bars {@link Core#KDJ} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastK_Period Lookback window for the raw stochastic high-low
    *        range (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning the raw stochastic into
    *        K (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into K (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the D signal line (default
    *        3; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowD_MAType MA type used for the D line (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int KDJ_Lookback( int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 9;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return -1;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.RMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return -1;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.RMA;
      }
      return STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType) ;

   }
   RetCode KDJ_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInFastK_Period,
                     int optInSlowK_Period,
                     MAType optInSlowK_MAType,
                     int optInSlowD_Period,
                     MAType optInSlowD_MAType,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outK[],
                     double outD[],
                     double outJ[] )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 9;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.RMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.RMA;
      }
      if( outK == outD || outK == outJ || outD == outJ ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
      /* Nothing to produce: the range is shorter than the lookback. Answering here
       * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
       * its own argument check would reject before any array is touched.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      OutRange _xr0 = STOCH(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outK, outD);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Keep this one Sub expression: spelling it as an Add of a negated term
       * would arm the multiply-add fusion and move the last bits of J.
       */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outJ[i] = 3.0 * outK[i] - 2.0 * outD[i];
      }
      return RetCode.Success ;
   }
   RetCode KDJ_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInFastK_Period,
                     int optInSlowK_Period,
                     MAType optInSlowK_MAType,
                     int optInSlowD_Period,
                     MAType optInSlowD_MAType,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outK[],
                     double outD[],
                     double outJ[] )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 9;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.RMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.RMA;
      }
      if( outK == outD || outK == outJ || outD == outJ ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      OutRange _xr0 = STOCH(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outK, outD);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outJ[i] = 3.0 * outK[i] - 2.0 * outD[i];
      }
      return RetCode.Success ;
   }
   /**
    * The stochastic oscillator as it is drawn on Chinese-market platforms: the
    * Slow-%K and Slow-%D lines plus a third line J that amplifies the gap
    * between them. K and D are read like any stochastic — above 80 overbought,
    * below 20 oversold, a K/D crossing signalling a momentum shift — while J is
    * a divergence gauge whose excursions outside the 0-100 band mark the
    * strongest moves. Both smoothing stages default to Wilder's moving average,
    * which is the smoother the original formula language specifies; selecting a
    * simple moving average for both reproduces the classic Slow Stochastic with
    * a J line attached.
    * <p><b>Formula</b>
    * <pre>{@code
    * RSV = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
    * K = MA(RSV, SlowK_Period, SlowK_MAType)
    * D = MA(K, SlowD_Period, SlowD_MAType)
    * J = 3*K - 2*D
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The default smoothing is Wilder's moving average. The originating 通达信 (Tongdaxin) formula language writes each stage as {@code SMA(X, N, 1)}, a recurrence with weight 1/N on the new value, which is Wilder's smoothing under another name — not a simple average.</li>
    * <li>How that recurrence is started is a TA-Lib house convention, not something the originating specification settles: like every other Wilder-smoothed function here, the first value is the simple average of the first N inputs, and callers who want the transient gone set the unstable period. Platforms that seed the recurrence at 50, or at the first raw value, differ for the first several dozen bars.</li>
    * <li>J is deliberately unbounded. It leaves the 0-100 band routinely and is never clamped.</li>
    * <li>When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KDJ_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the raw stochastic high-low
    *        range (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning the raw stochastic into
    *        K (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into K (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the D signal line (default
    *        3; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowD_MAType MA type used for the D line (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outK Raw stochastic smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outD Signal line: K smoothed by SlowD_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outJ Divergence line, three parts K less two parts D. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCH
    * @see Core#STOCHF
    * @see Core#RMA
    * @see Core#MA
    */
   public OutRange KDJ( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInFastK_Period,
                        int optInSlowK_Period,
                        MAType optInSlowK_MAType,
                        int optInSlowD_Period,
                        MAType optInSlowD_MAType,
                        double outK[],
                        double outD[],
                        double outJ[] )
   {
      requireIndexRange("KDJ", startIdx, endIdx);
      requireArgument("KDJ", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("KDJ", "optInSlowD_MAType", optInSlowD_MAType);
      int guardStart = clampedStart("KDJ", startIdx, KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KDJ", "inHigh", inHigh, guardInLen);
      requireLength("KDJ", "inLow", inLow, guardInLen);
      requireLength("KDJ", "inClose", inClose, guardInLen);
      requireLength("KDJ", "outK", outK, guardOutLen);
      requireLength("KDJ", "outD", outD, guardOutLen);
      requireLength("KDJ", "outJ", outJ, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KDJ_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ);
      if( retCode != RetCode.Success ) {
         throw failure("KDJ", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The stochastic oscillator as it is drawn on Chinese-market platforms: the
    * Slow-%K and Slow-%D lines plus a third line J that amplifies the gap
    * between them. K and D are read like any stochastic — above 80 overbought,
    * below 20 oversold, a K/D crossing signalling a momentum shift — while J is
    * a divergence gauge whose excursions outside the 0-100 band mark the
    * strongest moves. Both smoothing stages default to Wilder's moving average,
    * which is the smoother the original formula language specifies; selecting a
    * simple moving average for both reproduces the classic Slow Stochastic with
    * a J line attached.
    * <p><b>Formula</b>
    * <pre>{@code
    * RSV = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
    * K = MA(RSV, SlowK_Period, SlowK_MAType)
    * D = MA(K, SlowD_Period, SlowD_MAType)
    * J = 3*K - 2*D
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The default smoothing is Wilder's moving average. The originating 通达信 (Tongdaxin) formula language writes each stage as {@code SMA(X, N, 1)}, a recurrence with weight 1/N on the new value, which is Wilder's smoothing under another name — not a simple average.</li>
    * <li>How that recurrence is started is a TA-Lib house convention, not something the originating specification settles: like every other Wilder-smoothed function here, the first value is the simple average of the first N inputs, and callers who want the transient gone set the unstable period. Platforms that seed the recurrence at 50, or at the first raw value, differ for the first several dozen bars.</li>
    * <li>J is deliberately unbounded. It leaves the 0-100 band routinely and is never clamped.</li>
    * <li>When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#KDJ_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the raw stochastic high-low
    *        range (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning the raw stochastic into
    *        K (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into K (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the D signal line (default
    *        3; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowD_MAType MA type used for the D line (default 13 = RMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outK Raw stochastic smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outD Signal line: K smoothed by SlowD_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outJ Divergence line, three parts K less two parts D. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCH
    * @see Core#STOCHF
    * @see Core#RMA
    * @see Core#MA
    */
   public OutRange KDJ( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInFastK_Period,
                        int optInSlowK_Period,
                        MAType optInSlowK_MAType,
                        int optInSlowD_Period,
                        MAType optInSlowD_MAType,
                        double outK[],
                        double outD[],
                        double outJ[] )
   {
      requireIndexRange("KDJ", startIdx, endIdx);
      requireArgument("KDJ", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("KDJ", "optInSlowD_MAType", optInSlowD_MAType);
      int guardStart = clampedStart("KDJ", startIdx, KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KDJ", "inHigh", inHigh, guardInLen);
      requireLength("KDJ", "inLow", inLow, guardInLen);
      requireLength("KDJ", "inClose", inClose, guardInLen);
      requireLength("KDJ", "outK", outK, guardOutLen);
      requireLength("KDJ", "outD", outD, guardOutLen);
      requireLength("KDJ", "outJ", outJ, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = KDJ_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ);
      if( retCode != RetCode.Success ) {
         throw failure("KDJ", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KDJ stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#KDJ} over the same series.
    * Open with {@link Core#kdjOpen}; there is no close — the handle is
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
   public static final class KdjStream {
      Core core;
      int optInFastK_Period;
      int optInSlowK_Period;
      MAType optInSlowK_MAType;
      int optInSlowD_Period;
      MAType optInSlowD_MAType;
      double cur_outK;
      double cur_outD;
      double cur_outJ;
      StochStream sub0;
      int outRangeBegIdx;
      int outRangeCount;

      KdjStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#KDJ} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      KdjStream( KdjStream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInSlowK_Period = other.optInSlowK_Period;
         this.optInSlowK_MAType = other.optInSlowK_MAType;
         this.optInSlowD_Period = other.optInSlowD_Period;
         this.optInSlowD_MAType = other.optInSlowD_MAType;
         this.cur_outK = other.cur_outK;
         this.cur_outD = other.cur_outD;
         this.cur_outJ = other.cur_outJ;
         this.sub0 = new StochStream(other.sub0);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(KdjOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, KdjOut out ) {
         requireArgument("KDJ update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("KDJ update: BadParam", RetCode.BadParam);
         }
         core.kdjStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.k = this.cur_outK;
         out.d = this.cur_outD;
         out.j = this.cur_outJ;
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outK[], double outD[], double outJ[] ) {
         requireArgument("KDJ updateAndFill", "inHigh", inHigh);
         requireArgument("KDJ updateAndFill", "inLow", inLow);
         requireArgument("KDJ updateAndFill", "inClose", inClose);
         requireArgument("KDJ updateAndFill", "outK", outK);
         requireArgument("KDJ updateAndFill", "outD", outD);
         requireArgument("KDJ updateAndFill", "outJ", outJ);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outK.length < barCount || outD.length < barCount || outJ.length < barCount || (Object)outK == (Object)inHigh || (Object)outK == (Object)inLow || (Object)outK == (Object)inClose || (Object)outD == (Object)inHigh || (Object)outD == (Object)inLow || (Object)outD == (Object)inClose || (Object)outJ == (Object)inHigh || (Object)outJ == (Object)inLow || (Object)outJ == (Object)inClose || (Object)outK == (Object)outD || (Object)outK == (Object)outJ || (Object)outD == (Object)outJ )
            throw new TaLibArgumentException("KDJ updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("KDJ updateAndFill: BadParam", RetCode.BadParam);
            }
            core.kdjStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outK[i] = this.cur_outK;
            outD[i] = this.cur_outD;
            outJ[i] = this.cur_outJ;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies no buffer: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period. It does allocate a small bounded amount
       * per call — a size fixed by the indicator, never by the period.
       */
      public void peek( double inHigh, double inLow, double inClose, KdjOut out ) {
         requireArgument("KDJ peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("KDJ peek: BadParam", RetCode.BadParam);
         KdjStream sp = this;
         double cur_outK = 0.0;
         double cur_outD = 0.0;
         double cur_outJ = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         {
            StochOut subOut0 = new StochOut();
            sp.sub0.peek(inHigh, inLow, inClose, subOut0);
            cur_outK = subOut0.slowK;
            cur_outD = subOut0.slowD;
         }
         /* Combine map (batch tail, per bar). */
         cur_outJ = 3.0 * cur_outK - 2.0 * cur_outD;
         out.k = cur_outK;
         out.d = cur_outD;
         out.j = cur_outJ;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( KdjOut out ) {
         requireArgument("KDJ value", "out", out);
         out.k = this.cur_outK;
         out.d = this.cur_outD;
         out.j = this.cur_outJ;
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
      public KdjStream clone() {
         return new KdjStream(this);
      }
   }

   /**
    * The outputs of one KDJ bar, written by the stream into an object the
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
   public static final class KdjOut {
      /** Raw stochastic smoothed by SlowK_Period MA. */
      public double k;
      /** Signal line: K smoothed by SlowD_Period MA. */
      public double d;
      /** Divergence line, three parts K less two parts D. */
      public double j;
   }
   void kdjStepImpl( KdjStream sp, double inHigh, double inLow, double inClose )
   {
      double cur_outK = 0.0;
      double cur_outD = 0.0;
      double cur_outJ = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      {
         StochOut subOut0 = new StochOut();
         sp.sub0.update(inHigh, inLow, inClose, subOut0);
         cur_outK = subOut0.slowK;
         cur_outD = subOut0.slowD;
      }
      /* Combine map (batch tail, per bar). */
      cur_outJ = 3.0 * cur_outK - 2.0 * cur_outD;
      sp.cur_outK = cur_outK;
      sp.cur_outD = cur_outD;
      sp.cur_outJ = cur_outJ;
   }
   private RetCode kdjOpenImpl( KdjStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outK[], double outD[], double outJ[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
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
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 9;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.RMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.RMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outK = outStride == 1 ? outK : new double[historyLen];
      double[] sc_outD = outStride == 1 ? outD : new double[historyLen];
      double[] sc_outJ = outStride == 1 ? outJ : new double[historyLen];
      lookbackTotal = KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
      /* Nothing to produce: the range is shorter than the lookback. Answering here
       * keeps the sub-call out of the phantom-I/O sweep's zero-length range, where
       * its own argument check would reject before any array is touched.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Sub-stream 0: stoch over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StochStream sub0 = stochOpenAndFillInternal(inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sc_outK, sc_outD);
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Keep this one Sub expression: spelling it as an Add of a negated term
       * would arm the multiply-add fusion and move the last bits of J.
       */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         sc_outJ[i] = 3.0 * sc_outK[i] - 2.0 * sc_outD[i];
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInSlowK_Period = optInSlowK_Period;
      sp.optInSlowK_MAType = optInSlowK_MAType;
      sp.optInSlowD_Period = optInSlowD_Period;
      sp.optInSlowD_MAType = optInSlowD_MAType;
      sp.sub0 = sub0;
      sp.cur_outK = sc_outK[outNBElement.value - 1];
      sp.cur_outD = sc_outD[outNBElement.value - 1];
      sp.cur_outJ = sc_outJ[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* kdjOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KdjStream kdjOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outK[], double outD[], double outJ[] )
   {
      KdjStream sp = new KdjStream(this);
      RetCode retCode = kdjOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KDJ openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KDJ openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("KDJ openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind kdjOpen (composition seam). */
   KdjStream kdjOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      KdjStream sp = new KdjStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outK = new double[1];
      double[] sink_outD = new double[1];
      double[] sink_outJ = new double[1];
      RetCode retCode = kdjOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sink_outK, sink_outD, sink_outJ, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("KDJ open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("KDJ open: internal error", retCode);
      }
      throw new TaLibArgumentException("KDJ open: " + retCode, retCode);
   }
   /**
    * Open a live KDJ stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#KDJ} at that bar.
    * <p>The history must hold at least {@code KDJ_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KdjStream kdjOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      requireArgument("KDJ open", "inHigh", inHigh);
      requireHistory("KDJ open", inHigh.length);
      requireArgument("KDJ open", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("KDJ open", "optInSlowD_MAType", optInSlowD_MAType);
      requireArgument("KDJ open", "inLow", inLow);
      requireArgument("KDJ open", "inClose", inClose);
      requireHistoryLength("KDJ open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KDJ open", "inClose", inClose.length, inHigh.length);
      return kdjOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
   }
   /**
    * {@link Core#kdjOpen} that also fills the output array(s) bit-identically
    * to {@link Core#KDJ} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KdjStream#outRange()}.
    */
   public KdjStream kdjOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, double outK[], double outD[], double outJ[] )
   {
      requireArgument("KDJ openAndFill", "inHigh", inHigh);
      requireHistory("KDJ openAndFill", inHigh.length);
      requireArgument("KDJ openAndFill", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("KDJ openAndFill", "optInSlowD_MAType", optInSlowD_MAType);
      requireArgument("KDJ openAndFill", "inLow", inLow);
      requireArgument("KDJ openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("KDJ openAndFill", inHigh.length, KDJ_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      requireHistoryLength("KDJ openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KDJ openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("KDJ openAndFill", "outK", outK, guardOutLen);
      requireLength("KDJ openAndFill", "outD", outD, guardOutLen);
      requireLength("KDJ openAndFill", "outJ", outJ, guardOutLen);
      if( (Object)outK == (Object)inHigh || (Object)outK == (Object)inLow || (Object)outK == (Object)inClose || (Object)outD == (Object)inHigh || (Object)outD == (Object)inLow || (Object)outD == (Object)inClose || (Object)outJ == (Object)inHigh || (Object)outJ == (Object)inLow || (Object)outJ == (Object)inClose || (Object)outK == (Object)outD || (Object)outK == (Object)outJ || (Object)outD == (Object)outJ ) {
         throw new TaLibArgumentException("KDJ openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kdjOpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outK, outD, outJ);
   }
