/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  020605 AA     Fix #1117666 Lookback bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe; the TA_IS_ZERO guard is unchanged.
 */

   /**
    * Number of leading input bars {@link Core#PPO} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PPO_Lookback( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      /* Lookback is driven by the slowest MA. */
      return MA_Lookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) ;

   }
   RetCode PPO_Internal( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         MAType optInMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      retCode = MA_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      retCode = MA_Internal(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            outReal[i] = 0.0;
         }
      }
      return RetCode.Success ;
   }
   RetCode PPO_Internal( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         MAType optInMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = MA_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = MA_Internal(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      offset = fastNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            outReal[i] = 0.0;
         }
      }
      return RetCode.Success ;
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p><b>Formula</b>
    * <pre>{@code
    * PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0
    * The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED).
    * @param outReal PPO value in percent. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MA
    */
   public OutRange PPO( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PPO_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p><b>Formula</b>
    * <pre>{@code
    * PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0
    * The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED).
    * @param outReal PPO value in percent. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MA
    */
   public OutRange PPO( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PPO_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PPO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PPO} over the same series.
    * Open with {@link Core#PPO_Open}; there is no close — the handle is
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
   public static final class PPO_Stream {
      final Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      MAType optInMAType;
      double cur_outReal;
      MA_Stream sub0;
      MA_Stream sub1;
      OutRange fillRange = OutRange.EMPTY;

      PPO_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#PPO_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      PPO_Stream( PPO_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new MA_Stream(other.sub0);
         this.sub1 = new MA_Stream(other.sub1);
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.PPO_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         PPO_Stream scratch = new PPO_Stream(this);
         core.PPO_StreamStep(scratch, inReal);
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
      public PPO_Stream copy() {
         return new PPO_Stream(this);
      }
   }
   void PPO_StreamStep( PPO_Stream sp, double inReal )
   {
      double tempReal = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      tempReal = cur_outReal;
      if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
         cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
      } else {
         cur_outReal = 0.0;
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode PPO_OpenBody( PPO_Stream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
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
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( historyLen < PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      double[] sc_outReal = new double[historyLen];
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MA_Stream sub0 = MA_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInFastPeriod, optInMAType);
      retCode = MA_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MA_Stream sub1 = MA_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInSlowPeriod, optInMAType);
      retCode = MA_Internal(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = sc_outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            sc_outReal[i] = 0.0;
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   private RetCode PPO_OpenAndFillBody( PPO_Stream sp, double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( historyLen < PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      double[] sc_outReal = new double[historyLen];
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MA_Stream sub0 = MA_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInFastPeriod, optInMAType);
      retCode = MA_Internal(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MA_Stream sub1 = MA_OpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInSlowPeriod, optInMAType);
      retCode = MA_Internal(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = sc_outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            sc_outReal[i] = 0.0;
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      System.arraycopy(sc_outReal, 0, outReal, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind PPO_Open (composition seam). */
   PPO_Stream PPO_OpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      PPO_Stream sp = new PPO_Stream(this);
      RetCode retCode = PPO_OpenBody(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("PPO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("PPO open: internal error");
      }
      throw new IllegalArgumentException("PPO open: " + retCode);
   }
   /**
    * Open a live PPO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PPO} at that bar.
    * <p>The history must hold at least {@code PPO_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public PPO_Stream PPO_Open( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      return PPO_OpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType);
   }
   /**
    * {@link Core#PPO_Open} that also fills the output array(s) bit-identically
    * to {@link Core#PPO} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link PPO_Stream#fillRange()}.
    */
   public PPO_Stream PPO_OpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, double outReal[] )
   {
      PPO_Stream sp = new PPO_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PPO_OpenAndFillBody(sp, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("PPO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("PPO openAndFill: internal error");
      }
      throw new IllegalArgumentException("PPO openAndFill: " + retCode);
   }
