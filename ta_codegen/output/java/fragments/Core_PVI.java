/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  071726 MF,CC Implement Positive Volume Index (#126).
 */

   /**
    * Number of leading input bars {@link Core#PVI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PVI_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode PVI_Impl( int startIdx,
                     int endIdx,
                     double inClose[],
                     double inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempPVI = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* The index is a running cumulative value seeded at 1000, updated only on
       * bars whose volume increased versus the prior bar (Positive Volume).
       */
      prevPVI = 1000.0;
      prevClose = inClose[startIdx];
      prevVolume = inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         tempVolume = inVolume[i];
         /* prevClose != 0 guards the percentage-change division: a zero previous
          * close is a degenerate input that would otherwise emit NaN/Inf; carry
          * the index forward unchanged instead. Never triggers on real prices.
          */
         if( tempVolume > prevVolume && prevClose != 0.0 ) {
            /* The index is a running product, so it has no upper bound: enough
             * compounding gains push it past the largest double. Keep the last
             * representable value instead of writing +/-Inf, which no caller can
             * chart and which poisons every arithmetic downstream of it. Real
             * price series never come close.
             *
             * Written as a compound assignment on the copy, exactly as the update
             * was before the guard: spelling it `a + r*a` would match the FMA
             * fusion detector and silently re-round every bar, not just the
             * overflowing one.
             */
            tempPVI = prevPVI;
            tempPVI += (tempClose - prevClose) / prevClose * tempPVI;
            if( (Double.isFinite(tempPVI)) ) {
               prevPVI = tempPVI;
            }
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode PVI_Impl( int startIdx,
                     int endIdx,
                     float inClose[],
                     float inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempPVI = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevPVI = 1000.0;
      prevClose = (double)inClose[startIdx];
      prevVolume = (double)inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         tempVolume = (double)inVolume[i];
         if( tempVolume > prevVolume && prevClose != 0.0 ) {
            tempPVI = prevPVI;
            tempPVI += (tempClose - prevClose) / prevClose * tempPVI;
            if( (Double.isFinite(tempPVI)) ) {
               prevPVI = tempPVI;
            }
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Positive Volume Index: a running cumulative index that changes only on
    * days when volume rises versus the prior day, compounding that day's
    * percentage price change. The premise is that active, high-volume days
    * reflect the actions of the less-informed "crowd", so PVI is read as a
    * proxy for that cohort's positioning.
    * <p><b>Formula</b>
    * <pre>{@code
    * PVI[startIdx] = 1000
    * For each subsequent bar i:
    * PVI[i] = PVI[i-1] + ( inVolume[i] > inVolume[i-1]
    * ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * PVI[i-1]
    * : 0 )
    * The index carries forward unchanged on bars whose volume did not rise (and on the
    * degenerate case of a zero previous close, which would otherwise divide by zero).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The index compounds, so it has no upper bound. If a run of large rises ever pushes it past the largest representable number, the last representable value is carried forward instead of returning infinity. Real price series stay far away from that.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative positive volume index (seeded at 1000) Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    */
   public OutRange PVI( int startIdx,
                        int endIdx,
                        double inClose[],
                        double inVolume[],
                        double outReal[] )
   {
      requireIndexRange("PVI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, PVI_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PVI", "inClose", inClose, guardInLen);
      requireLength("PVI", "inVolume", inVolume, guardInLen);
      requireLength("PVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PVI_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Positive Volume Index: a running cumulative index that changes only on
    * days when volume rises versus the prior day, compounding that day's
    * percentage price change. The premise is that active, high-volume days
    * reflect the actions of the less-informed "crowd", so PVI is read as a
    * proxy for that cohort's positioning.
    * <p><b>Formula</b>
    * <pre>{@code
    * PVI[startIdx] = 1000
    * For each subsequent bar i:
    * PVI[i] = PVI[i-1] + ( inVolume[i] > inVolume[i-1]
    * ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * PVI[i-1]
    * : 0 )
    * The index carries forward unchanged on bars whose volume did not rise (and on the
    * degenerate case of a zero previous close, which would otherwise divide by zero).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The index compounds, so it has no upper bound. If a run of large rises ever pushes it past the largest representable number, the last representable value is carried forward instead of returning infinity. Real price series stay far away from that.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative positive volume index (seeded at 1000) Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    */
   public OutRange PVI( int startIdx,
                        int endIdx,
                        float inClose[],
                        float inVolume[],
                        double outReal[] )
   {
      requireIndexRange("PVI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, PVI_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PVI", "inClose", inClose, guardInLen);
      requireLength("PVI", "inVolume", inVolume, guardInLen);
      requireLength("PVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PVI_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PVI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PVI} over the same series.
    * Open with {@link Core#PVI_Open}; there is no close — the handle is
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
   public static final class PVI_Stream {
      Core core;
      double prevPVI;
      double prevClose;
      double prevVolume;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      PVI_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PVI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PVI_Stream( PVI_Stream other ) {
         this.core = other.core;
         this.prevPVI = other.prevPVI;
         this.prevClose = other.prevClose;
         this.prevVolume = other.prevVolume;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( PVI_Stream other ) {
         this.core = other.core;
         this.prevPVI = other.prevPVI;
         this.prevClose = other.prevClose;
         this.prevVolume = other.prevVolume;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("PVI update: BadParam", RetCode.BadParam);
         core.PVI_StepImpl(this, inClose, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inClose.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inClose[], double inVolume[], double outReal[] ) {
         final int barCount = inClose.length;
         if( inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("PVI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("PVI updateAndFill: BadParam", RetCode.BadParam);
            core.PVI_StepImpl(this, inClose[i], inVolume[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("PVI peek: BadParam", RetCode.BadParam);
         PVI_Stream scratch = new PVI_Stream(this);
         core.PVI_StepImpl(scratch, inClose, inVolume);
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
      public PVI_Stream copy() {
         return new PVI_Stream(this);
      }
   }
   void PVI_StepImpl( PVI_Stream sp, double inClose, double inVolume )
   {
      double tempClose = 0.0;
      double tempVolume = 0.0;
      double tempPVI = 0.0;
      tempClose = inClose;
      tempVolume = inVolume;
      /* prevClose != 0 guards the percentage-change division: a zero previous
       * close is a degenerate input that would otherwise emit NaN/Inf; carry
       * the index forward unchanged instead. Never triggers on real prices.
       */
      if( tempVolume > sp.prevVolume && sp.prevClose != 0.0 ) {
         /* The index is a running product, so it has no upper bound: enough
          * compounding gains push it past the largest double. Keep the last
          * representable value instead of writing +/-Inf, which no caller can
          * chart and which poisons every arithmetic downstream of it. Real
          * price series never come close.
          *
          * Written as a compound assignment on the copy, exactly as the update
          * was before the guard: spelling it `a + r*a` would match the FMA
          * fusion detector and silently re-round every bar, not just the
          * overflowing one.
          */
         tempPVI = sp.prevPVI;
         tempPVI += (tempClose - sp.prevClose) / sp.prevClose * tempPVI;
         if( (Double.isFinite(tempPVI)) ) {
            sp.prevPVI = tempPVI;
         }
      }
      sp.cur_outReal = sp.prevPVI;
      sp.prevClose = tempClose;
      sp.prevVolume = tempVolume;
   }
   private RetCode PVI_OpenImpl( PVI_Stream sp, double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempPVI = 0;
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* The index is a running cumulative value seeded at 1000, updated only on
       * bars whose volume increased versus the prior bar (Positive Volume).
       */
      prevPVI = 1000.0;
      prevClose = inClose[startIdx];
      prevVolume = inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         tempVolume = inVolume[i];
         /* prevClose != 0 guards the percentage-change division: a zero previous
          * close is a degenerate input that would otherwise emit NaN/Inf; carry
          * the index forward unchanged instead. Never triggers on real prices.
          */
         if( tempVolume > prevVolume && prevClose != 0.0 ) {
            /* The index is a running product, so it has no upper bound: enough
             * compounding gains push it past the largest double. Keep the last
             * representable value instead of writing +/-Inf, which no caller can
             * chart and which poisons every arithmetic downstream of it. Real
             * price series never come close.
             *
             * Written as a compound assignment on the copy, exactly as the update
             * was before the guard: spelling it `a + r*a` would match the FMA
             * fusion detector and silently re-round every bar, not just the
             * overflowing one.
             */
            tempPVI = prevPVI;
            tempPVI += (tempClose - prevClose) / prevClose * tempPVI;
            if( (Double.isFinite(tempPVI)) ) {
               prevPVI = tempPVI;
            }
         }
         outReal[outIdx++ * outStride] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevPVI = prevPVI;
      sp.prevClose = prevClose;
      sp.prevVolume = prevVolume;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* PVI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PVI_Stream PVI_OpenAndFillInternal( double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PVI_Stream sp = new PVI_Stream(this);
      RetCode retCode = PVI_OpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PVI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PVI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind PVI_Open (composition seam). */
   PVI_Stream PVI_OpenInternal( double inClose[], double inVolume[], int startIdx )
   {
      PVI_Stream sp = new PVI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = PVI_OpenImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PVI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PVI open: internal error", retCode);
      }
      throw new TaLibArgumentException("PVI open: " + retCode, retCode);
   }
   /**
    * Open a live PVI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PVI} at that bar.
    * <p>The history must hold at least {@code PVI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public PVI_Stream PVI_Open( double inClose[], double inVolume[] )
   {
      return PVI_OpenInternal(inClose, inVolume, 0);
   }
   /**
    * {@link Core#PVI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#PVI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link PVI_Stream#outRange()}.
    */
   public PVI_Stream PVI_OpenAndFill( double inClose[], double inVolume[], double outReal[] )
   {
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("PVI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return PVI_OpenAndFillInternal(inClose, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
