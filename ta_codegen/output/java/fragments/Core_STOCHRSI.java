/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101103 PP   Initial creation of code.
 *  112603 MF   Add independent control to the RSI period.
 *  020605 AA   Fix #1117656. NULL pointer assignement.
 */

   /**
    * Number of leading input bars {@link Core#STOCHRSI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod RSI period (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastK_Period Lookback window for the RSI min/max stochastic
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInFastD_Period Smoothing period for %D (default 3; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType MA type used to smooth %D (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int STOCHRSI_Lookback( int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      int retValue;
      retValue = RSI_Lookback(optInTimePeriod) + STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   RetCode STOCHRSI_Impl( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          int optInFastK_Period,
                          int optInFastD_Period,
                          MAType optInFastD_MAType,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outFastK[],
                          double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = RSI_Lookback(optInTimePeriod) + lookbackSTOCHF;
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
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      OutRange _xr0 = RSI(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, tempRSIBuffer);
      outBegIdx1.value = _xr0.begIdx();
      outNbElement1.value = _xr0.count();
      retCode = RetCode.Success;
      if( outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr1 = STOCHF(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD);
      outBegIdx2.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   RetCode STOCHRSI_Impl( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          int optInFastK_Period,
                          int optInFastD_Period,
                          MAType optInFastD_MAType,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outFastK[],
                          double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = RSI_Lookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      OutRange _xr0 = RSI(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, tempRSIBuffer);
      outBegIdx1.value = _xr0.begIdx();
      outNbElement1.value = _xr0.count();
      retCode = RetCode.Success;
      if( outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr1 = STOCHF(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outFastK, outFastD);
      outBegIdx2.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   /**
    * Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead
    * of price, measuring where RSI sits within its recent min/max range.
    * Oscillates 0-100; high = RSI near its recent top, low = near its recent
    * bottom.
    * <p><b>Formula</b>
    * <pre>{@code
    * rsi = RSI(inReal, optInTimePeriod)
    * FastK = 100 * (rsi_t - min(rsi, FastK_Period)) / (max(rsi, FastK_Period) - min(rsi, FastK_Period))
    * FastD = MA(FastK, FastD_Period, FastD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>To reproduce the original article's unsmoothed Stochastic RSI, set the RSI period equal to the %K period and read the raw %K output.</li>
    * <li>When the RSI's recent range is zero, %K is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCHRSI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series fed into the RSI calculation.
    * @param optInTimePeriod RSI period (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastK_Period Lookback window for the RSI min/max stochastic
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInFastD_Period Smoothing period for %D (default 3; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType MA type used to smooth %D (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outFastK Unsmoothed stochastic of the RSI (raw %K) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outFastD %K smoothed over FastD_Period (signal line) Must hold at
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
    * @see Core#RSI
    * @see Core#STOCHF
    * @see Core#STOCH
    * @see Core#MA
    */
   public OutRange STOCHRSI( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             int optInFastK_Period,
                             int optInFastD_Period,
                             MAType optInFastD_MAType,
                             double outFastK[],
                             double outFastD[] )
   {
      requireIndexRange("STOCHRSI", startIdx, endIdx);
      requireArgument("STOCHRSI", "optInFastD_MAType", optInFastD_MAType);
      int guardStart = clampedStart("STOCHRSI", startIdx, STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCHRSI", "inReal", inReal, guardInLen);
      requireLength("STOCHRSI", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHRSI", "outFastD", outFastD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHRSI_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCHRSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead
    * of price, measuring where RSI sits within its recent min/max range.
    * Oscillates 0-100; high = RSI near its recent top, low = near its recent
    * bottom.
    * <p><b>Formula</b>
    * <pre>{@code
    * rsi = RSI(inReal, optInTimePeriod)
    * FastK = 100 * (rsi_t - min(rsi, FastK_Period)) / (max(rsi, FastK_Period) - min(rsi, FastK_Period))
    * FastD = MA(FastK, FastD_Period, FastD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>To reproduce the original article's unsmoothed Stochastic RSI, set the RSI period equal to the %K period and read the raw %K output.</li>
    * <li>When the RSI's recent range is zero, %K is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCHRSI_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series fed into the RSI calculation.
    * @param optInTimePeriod RSI period (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastK_Period Lookback window for the RSI min/max stochastic
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInFastD_Period Smoothing period for %D (default 3; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType MA type used to smooth %D (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outFastK Unsmoothed stochastic of the RSI (raw %K) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outFastD %K smoothed over FastD_Period (signal line) Must hold at
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
    * @see Core#RSI
    * @see Core#STOCHF
    * @see Core#STOCH
    * @see Core#MA
    */
   public OutRange STOCHRSI( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             int optInFastK_Period,
                             int optInFastD_Period,
                             MAType optInFastD_MAType,
                             double outFastK[],
                             double outFastD[] )
   {
      requireIndexRange("STOCHRSI", startIdx, endIdx);
      requireArgument("STOCHRSI", "optInFastD_MAType", optInFastD_MAType);
      int guardStart = clampedStart("STOCHRSI", startIdx, STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCHRSI", "inReal", inReal, guardInLen);
      requireLength("STOCHRSI", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHRSI", "outFastD", outFastD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHRSI_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCHRSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STOCHRSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STOCHRSI} over the same series.
    * Open with {@link Core#stochrsiOpen}; there is no close — the handle is
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
   public static final class StochrsiStream {
      Core core;
      int optInTimePeriod;
      int optInFastK_Period;
      int optInFastD_Period;
      MAType optInFastD_MAType;
      double cur_outFastK;
      double cur_outFastD;
      RsiStream sub0;
      StochfStream sub1;
      int outRangeBegIdx;
      int outRangeCount;

      StochrsiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#STOCHRSI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      StochrsiStream( StochrsiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInFastD_Period = other.optInFastD_Period;
         this.optInFastD_MAType = other.optInFastD_MAType;
         this.cur_outFastK = other.cur_outFastK;
         this.cur_outFastD = other.cur_outFastD;
         this.sub0 = new RsiStream(other.sub0);
         this.sub1 = new StochfStream(other.sub1);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(StochrsiOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inReal, StochrsiOut out ) {
         requireArgument("STOCHRSI update", "out", out);
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("STOCHRSI update: BadParam", RetCode.BadParam);
         }
         core.stochrsiStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.fastK = this.cur_outFastK;
         out.fastD = this.cur_outFastD;
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
      public void updateAndFill( double inReal[], double outFastK[], double outFastD[] ) {
         requireArgument("STOCHRSI updateAndFill", "inReal", inReal);
         requireArgument("STOCHRSI updateAndFill", "outFastK", outFastK);
         requireArgument("STOCHRSI updateAndFill", "outFastD", outFastD);
         final int barCount = inReal.length;
         if( outFastK.length < barCount || outFastD.length < barCount || (Object)outFastK == (Object)inReal || (Object)outFastD == (Object)inReal || (Object)outFastK == (Object)outFastD )
            throw new TaLibArgumentException("STOCHRSI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("STOCHRSI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.stochrsiStepImpl(this, inReal[i]);
            outFastK[i] = this.cur_outFastK;
            outFastD[i] = this.cur_outFastD;
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
      public void peek( double inReal, StochrsiOut out ) {
         requireArgument("STOCHRSI peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("STOCHRSI peek: BadParam", RetCode.BadParam);
         StochrsiStream sp = this;
         double cur_tempRSIBuffer = 0.0;
         double cur_outFastK = 0.0;
         double cur_outFastD = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempRSIBuffer = sp.sub0.peek(inReal);
         {
            StochfOut subOut1 = new StochfOut();
            sp.sub1.peek(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer, subOut1);
            cur_outFastK = subOut1.fastK;
            cur_outFastD = subOut1.fastD;
         }
         out.fastK = cur_outFastK;
         out.fastD = cur_outFastD;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( StochrsiOut out ) {
         requireArgument("STOCHRSI value", "out", out);
         out.fastK = this.cur_outFastK;
         out.fastD = this.cur_outFastD;
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
      public StochrsiStream clone() {
         return new StochrsiStream(this);
      }
   }

   /**
    * The outputs of one STOCHRSI bar, written by the stream into an object the
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
   public static final class StochrsiOut {
      /** Unsmoothed stochastic of the RSI (raw %K) */
      public double fastK;
      /** %K smoothed over FastD_Period (signal line) */
      public double fastD;
   }
   void stochrsiStepImpl( StochrsiStream sp, double inReal )
   {
      double cur_tempRSIBuffer = 0.0;
      double cur_outFastK = 0.0;
      double cur_outFastD = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempRSIBuffer = sp.sub0.update(inReal);
      {
         StochfOut subOut1 = new StochfOut();
         sp.sub1.update(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer, subOut1);
         cur_outFastK = subOut1.fastK;
         cur_outFastD = subOut1.fastD;
      }
      sp.cur_outFastK = cur_outFastK;
      sp.cur_outFastD = cur_outFastD;
   }
   private RetCode stochrsiOpenImpl( StochrsiStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[], int outStride )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outFastK = outStride == 1 ? outFastK : new double[historyLen];
      double[] sc_outFastD = outStride == 1 ? outFastD : new double[historyLen];
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = RSI_Lookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      /* Sub-stream 0: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RsiStream sub0 = rsiOpenAndFillInternal(inReal, startIdx - lookbackSTOCHF, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      retCode = RetCode.Success;
      if( outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Sub-stream 1: stochf over `tempRSIBuffer, tempRSIBuffer, tempRSIBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StochfStream sub1 = stochfOpenAndFillInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* stochrsiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   StochrsiStream stochrsiOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
   {
      StochrsiStream sp = new StochrsiStream(this);
      RetCode retCode = stochrsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCHRSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCHRSI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCHRSI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind stochrsiOpen (composition seam). */
   StochrsiStream stochrsiOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      StochrsiStream sp = new StochrsiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outFastK = new double[1];
      double[] sink_outFastD = new double[1];
      RetCode retCode = stochrsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sink_outFastK, sink_outFastD, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCHRSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCHRSI open: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCHRSI open: " + retCode, retCode);
   }
   /**
    * Open a live STOCHRSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STOCHRSI} at that bar.
    * <p>The history must hold at least {@code STOCHRSI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public StochrsiStream stochrsiOpen( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      requireArgument("STOCHRSI open", "inReal", inReal);
      requireHistory("STOCHRSI open", inReal.length);
      requireArgument("STOCHRSI open", "optInFastD_MAType", optInFastD_MAType);
      return stochrsiOpenInternal(inReal, 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
   }
   /**
    * {@link Core#stochrsiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#STOCHRSI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link StochrsiStream#outRange()}.
    */
   public StochrsiStream stochrsiOpenAndFill( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, double outFastK[], double outFastD[] )
   {
      requireArgument("STOCHRSI openAndFill", "inReal", inReal);
      requireHistory("STOCHRSI openAndFill", inReal.length);
      requireArgument("STOCHRSI openAndFill", "optInFastD_MAType", optInFastD_MAType);
      int guardOutLen = openFillCount("STOCHRSI openAndFill", inReal.length, STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      requireLength("STOCHRSI openAndFill", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHRSI openAndFill", "outFastD", outFastD, guardOutLen);
      if( (Object)outFastK == (Object)inReal || (Object)outFastD == (Object)inReal || (Object)outFastK == (Object)outFastD ) {
         throw new TaLibArgumentException("STOCHRSI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return stochrsiOpenAndFillInternal(inReal, 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
   }
