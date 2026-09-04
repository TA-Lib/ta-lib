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
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#ROCR100} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback distance (bars back) for the reference
    *        price (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ROCR100_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode ROCR100_Impl( int startIdx,
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
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      /* Calculate Rate of change Ratio: (price / prevPrice) */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = inReal[inIdx] / tempReal * 100.0;
         } else {
            outReal[outIdx++] = 0.0;
         }
         inIdx += 1;
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode ROCR100_Impl( int startIdx,
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
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
         tempReal = (double)inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (double)inReal[inIdx] / tempReal * 100.0;
         } else {
            outReal[outIdx++] = 0.0;
         }
         inIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Rate-of-change ratio scaled by 100: current price as a percentage of the
    * price optInTimePeriod bars ago. Momentum measure centered at 100 and
    * always positive. Above 100 = price rose vs n bars ago; below 100 = price
    * fell.
    * <p><b>Formula</b>
    * <pre>{@code
    * $ROCR100_t = \dfrac{price_t}{price_{t-n}} \times 100$, where $n$ = optInTimePeriod
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ROCR100_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input price/data series.
    * @param optInTimePeriod Lookback distance (bars back) for the reference
    *        price (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Rate-of-change ratio times 100. Must hold at least
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
    * @see Core#ROCR
    * @see Core#ROC
    * @see Core#ROCP
    * @see Core#MOM
    */
   public OutRange ROCR100( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            double outReal[] )
   {
      requireIndexRange("ROCR100", startIdx, endIdx);
      int guardStart = clampedStart("ROCR100", startIdx, ROCR100_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ROCR100", "inReal", inReal, guardInLen);
      requireLength("ROCR100", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ROCR100_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ROCR100", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rate-of-change ratio scaled by 100: current price as a percentage of the
    * price optInTimePeriod bars ago. Momentum measure centered at 100 and
    * always positive. Above 100 = price rose vs n bars ago; below 100 = price
    * fell.
    * <p><b>Formula</b>
    * <pre>{@code
    * $ROCR100_t = \dfrac{price_t}{price_{t-n}} \times 100$, where $n$ = optInTimePeriod
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ROCR100_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input price/data series.
    * @param optInTimePeriod Lookback distance (bars back) for the reference
    *        price (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Rate-of-change ratio times 100. Must hold at least
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
    * @see Core#ROCR
    * @see Core#ROC
    * @see Core#ROCP
    * @see Core#MOM
    */
   public OutRange ROCR100( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            double outReal[] )
   {
      requireIndexRange("ROCR100", startIdx, endIdx);
      int guardStart = clampedStart("ROCR100", startIdx, ROCR100_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ROCR100", "inReal", inReal, guardInLen);
      requireLength("ROCR100", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ROCR100_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ROCR100", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ROCR100 stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ROCR100} over the same series.
    * Open with {@link Core#rocr100Open}; there is no close — the handle is
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
   public static final class Rocr100Stream {
      Core core;
      int optInTimePeriod;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      Rocr100Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ROCR100} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      Rocr100Stream( Rocr100Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
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
            throw new TaLibArgumentException("ROCR100 update: BadParam", RetCode.BadParam);
         }
         core.rocr100StepImpl(this, inReal);
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
         requireArgument("ROCR100 updateAndFill", "inReal", inReal);
         requireArgument("ROCR100 updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("ROCR100 updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("ROCR100 updateAndFill: BadParam", RetCode.BadParam);
            }
            core.rocr100StepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("ROCR100 peek: BadParam", RetCode.BadParam);
         Rocr100Stream sp = this;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         tempReal = (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0;
         if( tempReal != 0.0 ) {
            cur_outReal = inReal / tempReal * 100.0;
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
      public Rocr100Stream clone() {
         return new Rocr100Stream(this);
      }
   }
   void rocr100StepImpl( Rocr100Stream sp, double inReal )
   {
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      if( tempReal != 0.0 ) {
         sp.cur_outReal = inReal / tempReal * 100.0;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode rocr100OpenImpl( Rocr100Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      double tempReal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
         return RetCode.InsufficientHistory ;
      }
      /* Calculate Rate of change Ratio: (price / prevPrice) */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++ * outStride] = inReal[inIdx] / tempReal * 100.0;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         inIdx += 1;
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
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* rocr100OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   Rocr100Stream rocr100OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      Rocr100Stream sp = new Rocr100Stream(this);
      RetCode retCode = rocr100OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ROCR100 openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ROCR100 openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ROCR100 openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind rocr100Open (composition seam). */
   Rocr100Stream rocr100OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      Rocr100Stream sp = new Rocr100Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rocr100OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ROCR100 open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ROCR100 open: internal error", retCode);
      }
      throw new TaLibArgumentException("ROCR100 open: " + retCode, retCode);
   }
   /**
    * Open a live ROCR100 stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ROCR100} at that bar.
    * <p>The history must hold at least {@code ROCR100_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public Rocr100Stream rocr100Open( double inReal[], int optInTimePeriod )
   {
      requireArgument("ROCR100 open", "inReal", inReal);
      requireHistory("ROCR100 open", inReal.length);
      return rocr100OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#rocr100Open} that also fills the output array(s) bit-identically
    * to {@link Core#ROCR100} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link Rocr100Stream#outRange()}.
    */
   public Rocr100Stream rocr100OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("ROCR100 openAndFill", "inReal", inReal);
      requireHistory("ROCR100 openAndFill", inReal.length);
      int guardOutLen = openFillCount("ROCR100 openAndFill", inReal.length, ROCR100_Lookback(optInTimePeriod));
      requireLength("ROCR100 openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("ROCR100 openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rocr100OpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
