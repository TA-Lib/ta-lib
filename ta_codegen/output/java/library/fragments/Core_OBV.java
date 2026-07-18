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

   public int obvLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode obv( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
   public RetCode obvUnguarded( int startIdx,
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
   public RetCode obv( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
   public RetCode obvUnguarded( int startIdx,
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
/**** Streaming API *****/

   /**
    * A live OBV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#obv} over the same series.
    * Open with {@link Core#obvOpen}; there is no close — the handle is
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
   public static final class ObvStream {
      final Core core;
      double prevReal;
      double prevOBV;
      double cur_outReal;

      ObvStream( Core core ) { this.core = core; }

      ObvStream( ObvStream other ) {
         this.core = other.core;
         this.prevReal = other.prevReal;
         this.prevOBV = other.prevOBV;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal, double inVolume ) {
         core.obvStreamStep(this, inReal, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal, double inVolume ) {
         ObvStream scratch = new ObvStream(this);
         core.obvStreamStep(scratch, inReal, inVolume);
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
      public ObvStream copy() {
         return new ObvStream(this);
      }
   }
   void obvStreamStep( ObvStream sp, double inReal, double inVolume )
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
   private RetCode obvOpenBody( ObvStream sp, double inReal[], double inVolume[], int startIdx )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
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
         lastValue_outReal = prevOBV;
         prevReal = tempReal;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevReal = prevReal;
      sp.prevOBV = prevOBV;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode obvOpenAndFillBody( ObvStream sp, double inReal[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevReal = 0;
      double tempReal = 0;
      double prevOBV = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
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
      /* Capture the live batch state into the handle. */
      sp.prevReal = prevReal;
      sp.prevOBV = prevOBV;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind obvOpen (composition seam). */
   ObvStream obvOpenInternal( double inReal[], double inVolume[], int startIdx )
   {
      ObvStream sp = new ObvStream(this);
      RetCode retCode = obvOpenBody(sp, inReal, inVolume, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_OBV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_OBV open: internal error");
      }
      throw new IllegalArgumentException("TA_OBV open: " + retCode);
   }
   /**
    * Open a live OBV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#obv} at that bar.
    * <p>The history must hold at least {@code obvLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public ObvStream obvOpen( double inReal[], double inVolume[] )
   {
      return obvOpenInternal(inReal, inVolume, 0);
   }
   /**
    * {@link Core#obvOpen} that also fills the output array(s) bit-identically
    * to {@link Core#obv} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public ObvStream obvOpenAndFill( double inReal[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ObvStream sp = new ObvStream(this);
      RetCode retCode = obvOpenAndFillBody(sp, inReal, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_OBV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_OBV openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_OBV openAndFill: " + retCode);
   }
