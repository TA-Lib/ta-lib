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
 *  112605 MF   Fix outBegIdx when startIdx != 0
 */

   public int medPriceLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode medPrice( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPriceUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPrice( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPriceUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live MEDPRICE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#medPrice} over the same series.
    * Open with {@link Core#medPriceOpen}; there is no close — the handle is
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
   public static final class MedPriceStream {
      final Core core;
      double cur_outReal;

      MedPriceStream( Core core ) { this.core = core; }

      MedPriceStream( MedPriceStream other ) {
         this.core = other.core;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow ) {
         core.medPriceStreamStep(this, inHigh, inLow);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow ) {
         MedPriceStream scratch = new MedPriceStream(this);
         core.medPriceStreamStep(scratch, inHigh, inLow);
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
      public MedPriceStream copy() {
         return new MedPriceStream(this);
      }
   }
   void medPriceStreamStep( MedPriceStream sp, double inHigh, double inLow )
   {
      sp.cur_outReal = (inHigh + inLow) / 2.0;
   }
   private RetCode medPriceOpenBody( MedPriceStream sp, double inHigh[], double inLow[], int startIdx )
   {
      int outIdx = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         lastValue_outReal = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode medPriceOpenAndFillBody( MedPriceStream sp, double inHigh[], double inLow[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         return RetCode.BadParam;
      }
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind medPriceOpen (composition seam). */
   MedPriceStream medPriceOpenInternal( double inHigh[], double inLow[], int startIdx )
   {
      MedPriceStream sp = new MedPriceStream(this);
      RetCode retCode = medPriceOpenBody(sp, inHigh, inLow, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MEDPRICE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MEDPRICE open: internal error");
      }
      throw new IllegalArgumentException("TA_MEDPRICE open: " + retCode);
   }
   /**
    * Open a live MEDPRICE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#medPrice} at that bar.
    * <p>The history must hold at least {@code medPriceLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MedPriceStream medPriceOpen( double inHigh[], double inLow[] )
   {
      return medPriceOpenInternal(inHigh, inLow, 0);
   }
   /**
    * {@link Core#medPriceOpen} that also fills the output array(s) bit-identically
    * to {@link Core#medPrice} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MedPriceStream medPriceOpenAndFill( double inHigh[], double inLow[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MedPriceStream sp = new MedPriceStream(this);
      RetCode retCode = medPriceOpenAndFillBody(sp, inHigh, inLow, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MEDPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MEDPRICE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MEDPRICE openAndFill: " + retCode);
   }
