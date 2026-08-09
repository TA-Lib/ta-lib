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
    *        8=T3, 9=HMA, 10=DISABLED).
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
      int retValue;
      retValue = RSI_Lookback(optInTimePeriod) + STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   RetCode STOCHRSI_Internal( int startIdx,
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
      retCode = RSI_Internal(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = STOCHF_Internal(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   RetCode STOCHRSI_Internal( int startIdx,
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
      retCode = RSI_Internal(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = STOCHF_Internal(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
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
    *        8=T3, 9=HMA, 10=DISABLED).
    * @param outFastK Unsmoothed stochastic of the RSI (raw %K) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outFastD %K smoothed over FastD_Period (signal line) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHRSI_Internal(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
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
    *        8=T3, 9=HMA, 10=DISABLED).
    * @param outFastK Unsmoothed stochastic of the RSI (raw %K) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @param outFastD %K smoothed over FastD_Period (signal line) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHRSI_Internal(startIdx, endIdx, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCHRSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STOCHRSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STOCHRSI} over the same series.
    * Open with {@link Core#STOCHRSI_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class STOCHRSI_Stream {
      final Core core;
      int optInTimePeriod;
      int optInFastK_Period;
      int optInFastD_Period;
      MAType optInFastD_MAType;
      double cur_outFastK;
      double cur_outFastD;
      Value cachedValue;
      RSI_Stream sub0;
      STOCHF_Stream sub1;
      OutRange fillRange = OutRange.EMPTY;

      STOCHRSI_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#STOCHRSI_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      STOCHRSI_Stream( STOCHRSI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInFastD_Period = other.optInFastD_Period;
         this.optInFastD_MAType = other.optInFastD_MAType;
         this.cur_outFastK = other.cur_outFastK;
         this.cur_outFastD = other.cur_outFastD;
         this.cachedValue = other.cachedValue;
         this.sub0 = new RSI_Stream(other.sub0);
         this.sub1 = new STOCHF_Stream(other.sub1);
         this.fillRange = other.fillRange;
      }

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param fastK Unsmoothed stochastic of the RSI (raw %K)
       * @param fastD %K smoothed over FastD_Period (signal line)
       */
      public record Value(double fastK, double fastD) { }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.STOCHRSI_StreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outFastK, this.cur_outFastD);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         STOCHRSI_Stream scratch = new STOCHRSI_Stream(this);
         core.STOCHRSI_StreamStep(scratch, inReal);
         return new Value(scratch.cur_outFastK, scratch.cur_outFastD);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public STOCHRSI_Stream copy() {
         return new STOCHRSI_Stream(this);
      }
   }
   void STOCHRSI_StreamStep( STOCHRSI_Stream sp, double inReal )
   {
      double cur_tempRSIBuffer = 0.0;
      double cur_outFastK = 0.0;
      double cur_outFastD = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempRSIBuffer = sp.sub0.update(inReal);
      {
         STOCHF_Stream.Value subOut1 = sp.sub1.update(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer);
         cur_outFastK = subOut1.fastK();
         cur_outFastD = subOut1.fastD();
      }
      sp.cur_outFastK = cur_outFastK;
      sp.cur_outFastD = cur_outFastD;
   }
   private RetCode STOCHRSI_OpenBody( STOCHRSI_Stream sp, double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( historyLen < STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      /* Sub-stream 0: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RSI_Stream sub0 = RSI_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx - lookbackSTOCHF, optInTimePeriod);
      retCode = RSI_Internal(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Sub-stream 1: stochf over `tempRSIBuffer, tempRSIBuffer, tempRSIBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      STOCHF_Stream sub1 = STOCHF_OpenInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      retCode = STOCHF_Internal(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new STOCHRSI_Stream.Value(sp.cur_outFastK, sp.cur_outFastD);
      return RetCode.Success;
   }
   private RetCode STOCHRSI_OpenAndFillBody( STOCHRSI_Stream sp, double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
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
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( (Object)outFastK == (Object)inReal || (Object)outFastD == (Object)inReal || (Object)outFastK == (Object)outFastD ) {
         return RetCode.BadParam;
      }
      if( historyLen < STOCHRSI_Lookback(optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
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
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      /* Sub-stream 0: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RSI_Stream sub0 = RSI_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx - lookbackSTOCHF, optInTimePeriod);
      retCode = RSI_Internal(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Sub-stream 1: stochf over `tempRSIBuffer, tempRSIBuffer, tempRSIBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      STOCHF_Stream sub1 = STOCHF_OpenInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      retCode = STOCHF_Internal(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new STOCHRSI_Stream.Value(sp.cur_outFastK, sp.cur_outFastD);
      System.arraycopy(sc_outFastK, 0, outFastK, 0, outNBElement.value);
      System.arraycopy(sc_outFastD, 0, outFastD, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind STOCHRSI_Open (composition seam). */
   STOCHRSI_Stream STOCHRSI_OpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      STOCHRSI_Stream sp = new STOCHRSI_Stream(this);
      RetCode retCode = STOCHRSI_OpenBody(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("STOCHRSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("STOCHRSI open: internal error");
      }
      throw new IllegalArgumentException("STOCHRSI open: " + retCode);
   }
   /**
    * Open a live STOCHRSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STOCHRSI} at that bar.
    * <p>The history must hold at least {@code STOCHRSI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public STOCHRSI_Stream STOCHRSI_Open( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      return STOCHRSI_OpenInternal(inReal, 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
   }
   /**
    * {@link Core#STOCHRSI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#STOCHRSI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link STOCHRSI_Stream#fillRange()}.
    */
   public STOCHRSI_Stream STOCHRSI_OpenAndFill( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, double outFastK[], double outFastD[] )
   {
      STOCHRSI_Stream sp = new STOCHRSI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHRSI_OpenAndFillBody(sp, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("STOCHRSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("STOCHRSI openAndFill: internal error");
      }
      throw new IllegalArgumentException("STOCHRSI openAndFill: " + retCode);
   }
