/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  110206 AC   Change volume and open interest to double
 */

   /**
    * Number of leading input bars {@link Core#OBV} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int OBV_Lookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   RetCode OBV_Internal( int startIdx,
                         int endIdx,
                         double inReal[],
                         double inVolume[],
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevOBV = inVolume[startIdx];
      prevReal = inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode OBV_Internal( int startIdx,
                         int endIdx,
                         float inReal[],
                         float inVolume[],
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevOBV = (double)inVolume[startIdx];
      prevReal = (double)inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = (double)inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += (double)inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= (double)inVolume[i];
         }
         outReal[outIdx++] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * On Balance Volume: a running cumulative total of volume, added on up-price
    * bars and subtracted on down-price bars. Relates volume flow to price
    * direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * OBV[i] = OBV[i-1] + (inReal[i] > inReal[i-1] ? V[i] : inReal[i] < inReal[i-1] ? -V[i] : 0); seed OBV[startIdx] = V[startIdx]
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#OBV_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series, typically close.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative on-balance volume. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    */
   public OutRange OBV( int startIdx,
                        int endIdx,
                        double inReal[],
                        double inVolume[],
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = OBV_Internal(startIdx, endIdx, inReal, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("OBV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * On Balance Volume: a running cumulative total of volume, added on up-price
    * bars and subtracted on down-price bars. Relates volume flow to price
    * direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * OBV[i] = OBV[i-1] + (inReal[i] > inReal[i-1] ? V[i] : inReal[i] < inReal[i-1] ? -V[i] : 0); seed OBV[startIdx] = V[startIdx]
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#OBV_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series, typically close.
    * @param inVolume Volume of each bar.
    * @param outReal Cumulative on-balance volume. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    */
   public OutRange OBV( int startIdx,
                        int endIdx,
                        float inReal[],
                        float inVolume[],
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = OBV_Internal(startIdx, endIdx, inReal, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("OBV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live OBV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#OBV} over the same series.
    * Open with {@link Core#OBV_Open}; there is no close — the handle is
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
   public static final class OBV_Stream {
      Core core;
      double prevReal;
      double prevOBV;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      OBV_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#OBV_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      OBV_Stream( OBV_Stream other ) {
         this.core = other.core;
         this.prevReal = other.prevReal;
         this.prevOBV = other.prevOBV;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( OBV_Stream other ) {
         this.core = other.core;
         this.prevReal = other.prevReal;
         this.prevOBV = other.prevOBV;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal, double inVolume ) {
         core.OBV_StreamStep(this, inReal, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal, double inVolume ) {
         OBV_Stream scratch = new OBV_Stream(this);
         core.OBV_StreamStep(scratch, inReal, inVolume);
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
      public OBV_Stream copy() {
         return new OBV_Stream(this);
      }
   }
   void OBV_StreamStep( OBV_Stream sp, double inReal, double inVolume )
   {
      double tempReal = 0.0;
      tempReal = inReal;
      if( tempReal > sp.prevReal ) {
         sp.prevOBV += inVolume;
      } else if( tempReal < sp.prevReal ) {
         sp.prevOBV -= inVolume;
      }
      sp.cur_outReal = sp.prevOBV;
      sp.prevReal = tempReal;
   }
   private RetCode OBV_OpenCore( OBV_Stream sp, double inReal[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      prevOBV = inVolume[startIdx];
      prevReal = inReal[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempReal = inReal[i];
         if( tempReal > prevReal ) {
            prevOBV += inVolume[i];
         } else if( tempReal < prevReal ) {
            prevOBV -= inVolume[i];
         }
         outReal[outIdx++ * outStride] = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevReal = prevReal;
      sp.prevOBV = prevOBV;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode OBV_OpenBody( OBV_Stream sp, double inReal[], double inVolume[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return OBV_OpenCore( sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode OBV_OpenAndFillBody( OBV_Stream sp, double inReal[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      return OBV_OpenCore( sp, inReal, inVolume, 0, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode OBV_OpenAndFillInternalBody( OBV_Stream sp, double inReal[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return OBV_OpenCore(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
   }
   /* OBV_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   OBV_Stream OBV_OpenAndFillInternal( double inReal[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      OBV_Stream sp = new OBV_Stream(this);
      RetCode retCode = OBV_OpenAndFillInternalBody(sp, inReal, inVolume, startIdx, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("OBV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("OBV openAndFill: internal error");
      }
      throw new IllegalArgumentException("OBV openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind OBV_Open (composition seam). */
   OBV_Stream OBV_OpenInternal( double inReal[], double inVolume[], int startIdx )
   {
      OBV_Stream sp = new OBV_Stream(this);
      RetCode retCode = OBV_OpenBody(sp, inReal, inVolume, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("OBV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("OBV open: internal error");
      }
      throw new IllegalArgumentException("OBV open: " + retCode);
   }
   /**
    * Open a live OBV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#OBV} at that bar.
    * <p>The history must hold at least {@code OBV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public OBV_Stream OBV_Open( double inReal[], double inVolume[] )
   {
      return OBV_OpenInternal(inReal, inVolume, 0);
   }
   /**
    * {@link Core#OBV_Open} that also fills the output array(s) bit-identically
    * to {@link Core#OBV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link OBV_Stream#fillRange()}.
    */
   public OBV_Stream OBV_OpenAndFill( double inReal[], double inVolume[], double outReal[] )
   {
      OBV_Stream sp = new OBV_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = OBV_OpenAndFillBody(sp, inReal, inVolume, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("OBV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("OBV openAndFill: internal error");
      }
      throw new IllegalArgumentException("OBV openAndFill: " + retCode);
   }
