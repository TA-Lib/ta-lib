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
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#mom} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback distance in bars (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int momLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode momInternal( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The interpretation of the rate of change varies widely depending
       * which software and/or books you are refering to.
       *
       * The following is the table of Rate-Of-Change implemented in TA-LIB:
       *       MOM     = (price - prevPrice)         [Momentum]
       *       ROC     = ((price/prevPrice)-1)*100   [Rate of change]
       *       ROCP    = (price-prevPrice)/prevPrice [Rate of change Percentage]
       *       ROCR    = (price/prevPrice)           [Rate of change ratio]
       *       ROCR100 = (price/prevPrice)*100       [Rate of change ratio 100 Scale]
       *
       * Here are the equivalent function in other software:
       *       TA-Lib  |   Tradestation   |    Metastock
       *       =================================================
       *       MOM     |   Momentum       |    ROC (Point)
       *       ROC     |   ROC            |    ROC (Percent)
       *       ROCP    |   PercentChange  |    -
       *       ROCR    |   -              |    -
       *       ROCR100 |   -              |    MO
       *
       * The MOM function is the only one who is not normalized, and thus
       * should be avoided for comparing different time serie of prices.
       *
       * ROC and ROCP are centered at zero and can have positive and negative
       * value. Here are some equivalence:
       *    ROC = ROCP/100
       *        = ((price-prevPrice)/prevPrice)/100
       *        = ((price/prevPrice)-1)*100
       *
       * ROCR and ROCR100 are ratio respectively centered at 1 and 100 and are
       * always positive values.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Calculate Momentum:
       *    Just substract the value from 'period' ago from
       *    current value.
       */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         outReal[outIdx++] = inReal[inIdx++] - inReal[trailingIdx++];
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode momUnguardedInternal( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         outReal[outIdx++] = inReal[inIdx++] - inReal[trailingIdx++];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode momInternal( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         outReal[outIdx++] = (double)inReal[inIdx++] - (double)inReal[trailingIdx++];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode momUnguardedInternal( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         outReal[outIdx++] = (double)inReal[inIdx++] - (double)inReal[trailingIdx++];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Momentum: current price minus the price optInTimePeriod bars ago. The
    * absolute (unnormalized) rate of change. Positive = price rose over the
    * period, negative = fell; centered at zero.
    * <p><b>Formula</b>
    * <pre>{@code
    * MOM[i] = inReal[i] - inReal[i - optInTimePeriod]
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#momLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input price series.
    * @param optInTimePeriod Lookback distance in bars (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Momentum (current minus value optInTimePeriod bars ago)
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#roc
    * @see Core#rocP
    * @see Core#rocR
    * @see Core#rocR100
    */
   public OutRange mom( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = momInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MOM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Momentum: current price minus the price optInTimePeriod bars ago. The
    * absolute (unnormalized) rate of change. Positive = price rose over the
    * period, negative = fell; centered at zero. — <b>unchecked</b> variant of
    * {@link Core#mom}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange momUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      momUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Momentum: current price minus the price optInTimePeriod bars ago. The
    * absolute (unnormalized) rate of change. Positive = price rose over the
    * period, negative = fell; centered at zero.
    * <p><b>Formula</b>
    * <pre>{@code
    * MOM[i] = inReal[i] - inReal[i - optInTimePeriod]
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#momLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input price series.
    * @param optInTimePeriod Lookback distance in bars (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Momentum (current minus value optInTimePeriod bars ago)
    *        Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#roc
    * @see Core#rocP
    * @see Core#rocR
    * @see Core#rocR100
    */
   public OutRange mom( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = momInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MOM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Momentum: current price minus the price optInTimePeriod bars ago. The
    * absolute (unnormalized) rate of change. Positive = price rose over the
    * period, negative = fell; centered at zero. — <b>unchecked</b> variant of
    * {@link Core#mom}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    * <p>This is the {@code float[]} overload; see the guarded method.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange momUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      momUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MOM stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#mom} over the same series.
    * Open with {@link Core#momOpen}; there is no close — the handle is
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
   public static final class MomStream {
      final Core core;
      int optInTimePeriod;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      OutRange fillRange;

      MomStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#momOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      MomStream( MomStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.momStreamStep(this, inReal);
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
         MomStream scratch = new MomStream(this);
         core.momStreamStep(scratch, inReal);
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
      public MomStream copy() {
         return new MomStream(this);
      }
   }
   void momStreamStep( MomStream sp, double inReal )
   {
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.cur_outReal = inReal - sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode momOpenBody( MomStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The interpretation of the rate of change varies widely depending
       * which software and/or books you are refering to.
       *
       * The following is the table of Rate-Of-Change implemented in TA-LIB:
       *       MOM     = (price - prevPrice)         [Momentum]
       *       ROC     = ((price/prevPrice)-1)*100   [Rate of change]
       *       ROCP    = (price-prevPrice)/prevPrice [Rate of change Percentage]
       *       ROCR    = (price/prevPrice)           [Rate of change ratio]
       *       ROCR100 = (price/prevPrice)*100       [Rate of change ratio 100 Scale]
       *
       * Here are the equivalent function in other software:
       *       TA-Lib  |   Tradestation   |    Metastock
       *       =================================================
       *       MOM     |   Momentum       |    ROC (Point)
       *       ROC     |   ROC            |    ROC (Percent)
       *       ROCP    |   PercentChange  |    -
       *       ROCR    |   -              |    -
       *       ROCR100 |   -              |    MO
       *
       * The MOM function is the only one who is not normalized, and thus
       * should be avoided for comparing different time serie of prices.
       *
       * ROC and ROCP are centered at zero and can have positive and negative
       * value. Here are some equivalence:
       *    ROC = ROCP/100
       *        = ((price-prevPrice)/prevPrice)/100
       *        = ((price/prevPrice)-1)*100
       *
       * ROCR and ROCR100 are ratio respectively centered at 1 and 100 and are
       * always positive values.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Calculate Momentum:
       *    Just substract the value from 'period' ago from
       *    current value.
       */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         lastValue_outReal = inReal[inIdx++] - inReal[trailingIdx++];
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = inIdx - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode momOpenAndFillBody( MomStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      /* The interpretation of the rate of change varies widely depending
       * which software and/or books you are refering to.
       *
       * The following is the table of Rate-Of-Change implemented in TA-LIB:
       *       MOM     = (price - prevPrice)         [Momentum]
       *       ROC     = ((price/prevPrice)-1)*100   [Rate of change]
       *       ROCP    = (price-prevPrice)/prevPrice [Rate of change Percentage]
       *       ROCR    = (price/prevPrice)           [Rate of change ratio]
       *       ROCR100 = (price/prevPrice)*100       [Rate of change ratio 100 Scale]
       *
       * Here are the equivalent function in other software:
       *       TA-Lib  |   Tradestation   |    Metastock
       *       =================================================
       *       MOM     |   Momentum       |    ROC (Point)
       *       ROC     |   ROC            |    ROC (Percent)
       *       ROCP    |   PercentChange  |    -
       *       ROCR    |   -              |    -
       *       ROCR100 |   -              |    MO
       *
       * The MOM function is the only one who is not normalized, and thus
       * should be avoided for comparing different time serie of prices.
       *
       * ROC and ROCP are centered at zero and can have positive and negative
       * value. Here are some equivalence:
       *    ROC = ROCP/100
       *        = ((price-prevPrice)/prevPrice)/100
       *        = ((price/prevPrice)-1)*100
       *
       * ROCR and ROCR100 are ratio respectively centered at 1 and 100 and are
       * always positive values.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Calculate Momentum:
       *    Just substract the value from 'period' ago from
       *    current value.
       */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         outReal[outIdx++] = inReal[inIdx++] - inReal[trailingIdx++];
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = inIdx - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind momOpen (composition seam). */
   MomStream momOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MomStream sp = new MomStream(this);
      RetCode retCode = momOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MOM open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MOM open: internal error");
      }
      throw new IllegalArgumentException("TA_MOM open: " + retCode);
   }
   /**
    * Open a live MOM stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#mom} at that bar.
    * <p>The history must hold at least {@code momLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MomStream momOpen( double inReal[], int optInTimePeriod )
   {
      return momOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#momOpen} that also fills the output array(s) bit-identically
    * to {@link Core#mom} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MomStream#fillRange()}.
    */
   public MomStream momOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      MomStream sp = new MomStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = momOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MOM openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MOM openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MOM openAndFill: " + retCode);
   }
