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
 *  071726 MF,CC Implement Negative Volume Index (#126).
 */

   /**
    * Number of leading input bars {@link Core#NVI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int NVI_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode NVI_Impl( int startIdx,
                     int endIdx,
                     double inClose[],
                     double inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempNVI = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* The index is a running cumulative value seeded at 1000, updated only on
       * bars whose volume decreased versus the prior bar (Negative Volume).
       */
      prevNVI = 1000.0;
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
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
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
            tempNVI = prevNVI;
            tempNVI += (tempClose - prevClose) / prevClose * tempNVI;
            if( (Double.isFinite(tempNVI)) ) {
               prevNVI = tempNVI;
            }
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode NVI_Impl( int startIdx,
                     int endIdx,
                     float inClose[],
                     float inVolume[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempNVI = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevNVI = 1000.0;
      prevClose = (double)inClose[startIdx];
      prevVolume = (double)inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         tempVolume = (double)inVolume[i];
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
            tempNVI = prevNVI;
            tempNVI += (tempClose - prevClose) / prevClose * tempNVI;
            if( (Double.isFinite(tempNVI)) ) {
               prevNVI = tempNVI;
            }
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Negative Volume Index: a running cumulative index that changes only on
    * days when volume falls versus the prior day, compounding that day's
    * percentage price change. The premise is that quiet, low-volume days
    * reflect the actions of well-informed "smart money", so NVI is read as a
    * proxy for that cohort's positioning.
    * <p><b>Formula</b>
    * <pre>{@code
    * NVI[startIdx] = 1000
    * For each subsequent bar i:
    * NVI[i] = NVI[i-1] + ( inVolume[i] < inVolume[i-1]
    * ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * NVI[i-1]
    * : 0 )
    * The index carries forward unchanged on bars whose volume did not fall (and on the
    * degenerate case of a zero previous close, which would otherwise divide by zero).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The index compounds, so it has no upper bound. If a run of large rises ever pushes it past the largest representable number, the last representable value is carried forward instead of returning infinity. Real price series stay far away from that.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#NVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative negative volume index (seeded at 1000) Must hold
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
   public OutRange NVI( int startIdx,
                        int endIdx,
                        double inClose[],
                        double inVolume[],
                        double outReal[] )
   {
      requireIndexRange("NVI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, NVI_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("NVI", "inClose", inClose, guardInLen);
      requireLength("NVI", "inVolume", inVolume, guardInLen);
      requireLength("NVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NVI_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("NVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Negative Volume Index: a running cumulative index that changes only on
    * days when volume falls versus the prior day, compounding that day's
    * percentage price change. The premise is that quiet, low-volume days
    * reflect the actions of well-informed "smart money", so NVI is read as a
    * proxy for that cohort's positioning.
    * <p><b>Formula</b>
    * <pre>{@code
    * NVI[startIdx] = 1000
    * For each subsequent bar i:
    * NVI[i] = NVI[i-1] + ( inVolume[i] < inVolume[i-1]
    * ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * NVI[i-1]
    * : 0 )
    * The index carries forward unchanged on bars whose volume did not fall (and on the
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
    * valid range shorter than {@link Core#NVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative negative volume index (seeded at 1000) Must hold
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
   public OutRange NVI( int startIdx,
                        int endIdx,
                        float inClose[],
                        float inVolume[],
                        double outReal[] )
   {
      requireIndexRange("NVI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, NVI_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("NVI", "inClose", inClose, guardInLen);
      requireLength("NVI", "inVolume", inVolume, guardInLen);
      requireLength("NVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NVI_Impl(startIdx, endIdx, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("NVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live NVI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#NVI} over the same series.
    * Open with {@link Core#NVI_Open}; there is no close — the handle is
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
   public static final class NVI_Stream {
      Core core;
      double prevNVI;
      double prevClose;
      double prevVolume;
      double tempNVI;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      NVI_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#NVI_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      NVI_Stream( NVI_Stream other ) {
         this.core = other.core;
         this.prevNVI = other.prevNVI;
         this.prevClose = other.prevClose;
         this.prevVolume = other.prevVolume;
         this.tempNVI = other.tempNVI;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( NVI_Stream other ) {
         this.core = other.core;
         this.prevNVI = other.prevNVI;
         this.prevClose = other.prevClose;
         this.prevVolume = other.prevVolume;
         this.tempNVI = other.tempNVI;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
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
            throw new TaLibArgumentException("NVI update: BadParam", RetCode.BadParam);
         core.NVI_StreamStep(this, inClose, inVolume);
         return this.cur_outReal;
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
            throw new TaLibArgumentException("NVI peek: BadParam", RetCode.BadParam);
         NVI_Stream scratch = new NVI_Stream(this);
         core.NVI_StreamStep(scratch, inClose, inVolume);
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
      public NVI_Stream copy() {
         return new NVI_Stream(this);
      }
   }
   void NVI_StreamStep( NVI_Stream sp, double inClose, double inVolume )
   {
      double tempClose = 0.0;
      double tempVolume = 0.0;
      tempClose = inClose;
      tempVolume = inVolume;
      /* prevClose != 0 guards the percentage-change division: a zero previous
       * close is a degenerate input that would otherwise emit NaN/Inf; carry
       * the index forward unchanged instead. Never triggers on real prices.
       */
      if( tempVolume < sp.prevVolume && sp.prevClose != 0.0 ) {
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
         sp.tempNVI = sp.prevNVI;
         sp.tempNVI += (tempClose - sp.prevClose) / sp.prevClose * sp.tempNVI;
         if( (Double.isFinite(sp.tempNVI)) ) {
            sp.prevNVI = sp.tempNVI;
         }
      }
      sp.cur_outReal = sp.prevNVI;
      sp.prevClose = tempClose;
      sp.prevVolume = tempVolume;
   }
   private RetCode NVI_OpenPass( NVI_Stream sp, double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      double tempNVI = 0;
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      /* The index is a running cumulative value seeded at 1000, updated only on
       * bars whose volume decreased versus the prior bar (Negative Volume).
       */
      prevNVI = 1000.0;
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
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
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
            tempNVI = prevNVI;
            tempNVI += (tempClose - prevClose) / prevClose * tempNVI;
            if( (Double.isFinite(tempNVI)) ) {
               prevNVI = tempNVI;
            }
         }
         outReal[outIdx++ * outStride] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevNVI = prevNVI;
      sp.prevClose = prevClose;
      sp.prevVolume = prevVolume;
      sp.tempNVI = tempNVI;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode NVI_OpenImpl( NVI_Stream sp, double inClose[], double inVolume[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return NVI_OpenPass( sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode NVI_OpenAndFillImpl( NVI_Stream sp, double inClose[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      return NVI_OpenPass( sp, inClose, inVolume, 0, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode NVI_OpenAndFillInternalImpl( NVI_Stream sp, double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return NVI_OpenPass(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
   }
   /* NVI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   NVI_Stream NVI_OpenAndFillInternal( double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      NVI_Stream sp = new NVI_Stream(this);
      RetCode retCode = NVI_OpenAndFillInternalImpl(sp, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NVI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("NVI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind NVI_Open (composition seam). */
   NVI_Stream NVI_OpenInternal( double inClose[], double inVolume[], int startIdx )
   {
      NVI_Stream sp = new NVI_Stream(this);
      RetCode retCode = NVI_OpenImpl(sp, inClose, inVolume, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NVI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NVI open: internal error", retCode);
      }
      throw new TaLibArgumentException("NVI open: " + retCode, retCode);
   }
   /**
    * Open a live NVI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#NVI} at that bar.
    * <p>The history must hold at least {@code NVI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public NVI_Stream NVI_Open( double inClose[], double inVolume[] )
   {
      return NVI_OpenInternal(inClose, inVolume, 0);
   }
   /**
    * {@link Core#NVI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#NVI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link NVI_Stream#fillRange()}.
    */
   public NVI_Stream NVI_OpenAndFill( double inClose[], double inVolume[], double outReal[] )
   {
      NVI_Stream sp = new NVI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NVI_OpenAndFillImpl(sp, inClose, inVolume, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NVI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("NVI openAndFill: " + retCode, retCode);
   }
