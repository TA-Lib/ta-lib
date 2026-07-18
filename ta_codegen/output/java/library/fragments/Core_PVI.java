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

   public int pviLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode pvi( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode pviUnguarded( int startIdx,
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
      prevPVI = 1000.0;
      prevClose = inClose[startIdx];
      prevVolume = inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         tempVolume = inVolume[i];
         if( tempVolume > prevVolume && prevClose != 0.0 ) {
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode pvi( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode pviUnguarded( int startIdx,
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
      prevPVI = 1000.0;
      prevClose = (double)inClose[startIdx];
      prevVolume = (double)inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         tempVolume = (double)inVolume[i];
         if( tempVolume > prevVolume && prevClose != 0.0 ) {
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live PVI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#pvi} over the same series.
    * Open with {@link Core#pviOpen}; there is no close — the handle is
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
   public static final class PviStream {
      final Core core;
      double prevPVI;
      double prevClose;
      double prevVolume;
      double cur_outReal;

      PviStream( Core core ) { this.core = core; }

      PviStream( PviStream other ) {
         this.core = other.core;
         this.prevPVI = other.prevPVI;
         this.prevClose = other.prevClose;
         this.prevVolume = other.prevVolume;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inClose, double inVolume ) {
         core.pviStreamStep(this, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inClose, double inVolume ) {
         PviStream scratch = new PviStream(this);
         core.pviStreamStep(scratch, inClose, inVolume);
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
      public PviStream copy() {
         return new PviStream(this);
      }
   }
   void pviStreamStep( PviStream sp, double inClose, double inVolume )
   {
      double tempClose = 0.0;
      double tempVolume = 0.0;
      tempClose = inClose;
      tempVolume = inVolume;
      /* prevClose != 0 guards the percentage-change division: a zero previous
       * close is a degenerate input that would otherwise emit NaN/Inf; carry
       * the index forward unchanged instead. Never triggers on real prices.
       */
      if( tempVolume > sp.prevVolume && sp.prevClose != 0.0 ) {
         sp.prevPVI += (tempClose - sp.prevClose) / sp.prevClose * sp.prevPVI;
      }
      sp.cur_outReal = sp.prevPVI;
      sp.prevClose = tempClose;
      sp.prevVolume = tempVolume;
   }
   private RetCode pviOpenBody( PviStream sp, double inClose[], double inVolume[], int startIdx )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inClose.length ) {
         return RetCode.BadParam;
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
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         lastValue_outReal = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevPVI = prevPVI;
      sp.prevClose = prevClose;
      sp.prevVolume = prevVolume;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode pviOpenAndFillBody( PviStream sp, double inClose[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevPVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
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
            prevPVI += (tempClose - prevClose) / prevClose * prevPVI;
         }
         outReal[outIdx++] = prevPVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.prevPVI = prevPVI;
      sp.prevClose = prevClose;
      sp.prevVolume = prevVolume;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind pviOpen (composition seam). */
   PviStream pviOpenInternal( double inClose[], double inVolume[], int startIdx )
   {
      PviStream sp = new PviStream(this);
      RetCode retCode = pviOpenBody(sp, inClose, inVolume, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_PVI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_PVI open: internal error");
      }
      throw new IllegalArgumentException("TA_PVI open: " + retCode);
   }
   /**
    * Open a live PVI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#pvi} at that bar.
    * <p>The history must hold at least {@code pviLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public PviStream pviOpen( double inClose[], double inVolume[] )
   {
      return pviOpenInternal(inClose, inVolume, 0);
   }
   /**
    * {@link Core#pviOpen} that also fills the output array(s) bit-identically
    * to {@link Core#pvi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public PviStream pviOpenAndFill( double inClose[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PviStream sp = new PviStream(this);
      RetCode retCode = pviOpenAndFillBody(sp, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_PVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_PVI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_PVI openAndFill: " + retCode);
   }
