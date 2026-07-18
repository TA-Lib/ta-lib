/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  082303 MF     Fix #792298. Remove rounding. Bug reported by AM.
 *  071126 MF,CC  Rewrite the ADX combine as a single cursor: outReal[k] =
 *                (adx[k+(period-1)] + adx[k])/2 (current ADX + ADX lagged by
 *                period-1). Bit-identical to the two-cursor form, and the
 *                streamable-source form (a sub-output lag ring).
 */

   public int adxrLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod > 1 ) {
         return optInTimePeriod + adxLookback(optInTimePeriod) - 1 ;
      } else {
         return 3 ;
      }

   }
   public RetCode adxr( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      /* Compute ADX over a range that starts (period-1) bars earlier, so each
       * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
       */
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
       * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
       * is adx[k]; the ADX range holds (period-1) more elements than the output.
       */
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
   public RetCode adxrUnguarded( int startIdx,
                                 int endIdx,
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
   public RetCode adxr( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
   public RetCode adxrUnguarded( int startIdx,
                                 int endIdx,
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live ADXR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#adxr} over the same series.
    * Open with {@link Core#adxrOpen}; there is no close — the handle is
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
   public static final class AdxrStream {
      final Core core;
      int optInTimePeriod;
      double cur_outReal;
      int lagRingPos_adx;
      int lagRingCap_adx;
      double[] lagRing_adx;
      AdxStream sub0;

      AdxrStream( Core core ) { this.core = core; }

      AdxrStream( AdxrStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.cur_outReal = other.cur_outReal;
         this.lagRingPos_adx = other.lagRingPos_adx;
         this.lagRingCap_adx = other.lagRingCap_adx;
         this.lagRing_adx = other.lagRing_adx.clone();
         this.sub0 = new AdxStream(other.sub0);
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.adxrStreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         AdxrStream scratch = new AdxrStream(this);
         core.adxrStreamStep(scratch, inHigh, inLow, inClose);
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
      public AdxrStream copy() {
         return new AdxrStream(this);
      }
   }
   void adxrStreamStep( AdxrStream sp, double inHigh, double inLow, double inClose )
   {
      double cur_adx = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_adx = sp.sub0.update(inHigh, inLow, inClose);
      /* Combine map (batch tail, per bar). */
      cur_outReal = ((cur_adx + sp.lagRing_adx[sp.lagRingPos_adx]) / 2.0);
      sp.lagRing_adx[sp.lagRingPos_adx] = cur_adx;
      sp.lagRingPos_adx = (sp.lagRingPos_adx + 1) % sp.lagRingCap_adx;
      sp.cur_outReal = cur_outReal;
   }
   private RetCode adxrOpenBody( AdxrStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      /* Compute ADX over a range that starts (period-1) bars earlier, so each
       * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
       */
      /* Sub-stream 0: adx over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      AdxStream sub0 = adxOpenInternal(java.util.Arrays.copyOfRange(inHigh, 0, (endIdx) + 1), java.util.Arrays.copyOfRange(inLow, 0, (endIdx) + 1), java.util.Arrays.copyOfRange(inClose, 0, (endIdx) + 1), startIdx - (optInTimePeriod - 1), optInTimePeriod);
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
       * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
       * is adx[k]; the ADX range holds (period-1) more elements than the output.
       */
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         sc_outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int lagCap_adx = (int)(optInTimePeriod - 1);
      double[] lagRing_adx = new double[lagCap_adx];
      for( int lagI = 0; lagI < lagCap_adx; lagI++ ) {
         lagRing_adx[lagI] = adx[outNBElement.value + lagI];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sub0 = sub0;
      sp.lagRingPos_adx = 0;
      sp.lagRingCap_adx = lagCap_adx;
      sp.lagRing_adx = lagRing_adx;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   private RetCode adxrOpenAndFillBody( AdxrStream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double[] adx;
      int adxrLookback = 0;
      int outIdx = 0;
      int nbElement = 0;
      RetCode retCode;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      /* Move up the start index if there is not
       * enough initial data.
       * Always one price bar gets consumed.
       */
      adxrLookback = adxrLookback(optInTimePeriod);
      if( startIdx < adxrLookback ) {
         startIdx = adxrLookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      adx = new double[(int)((endIdx - startIdx + optInTimePeriod) * 1)];
      /* Compute ADX over a range that starts (period-1) bars earlier, so each
       * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
       */
      /* Sub-stream 0: adx over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      AdxStream sub0 = adxOpenInternal(java.util.Arrays.copyOfRange(inHigh, 0, (endIdx) + 1), java.util.Arrays.copyOfRange(inLow, 0, (endIdx) + 1), java.util.Arrays.copyOfRange(inClose, 0, (endIdx) + 1), startIdx - (optInTimePeriod - 1), optInTimePeriod);
      retCode = adxUnguarded(startIdx - (optInTimePeriod - 1), endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, adx);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
       * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
       * is adx[k]; the ADX range holds (period-1) more elements than the output.
       */
      nbElement = outNBElement.value - (optInTimePeriod - 1);
      for( outIdx = 0; outIdx < nbElement; outIdx += 1 ) {
         sc_outReal[outIdx] = ((adx[outIdx + (optInTimePeriod - 1)] + adx[outIdx]) / 2.0);
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int lagCap_adx = (int)(optInTimePeriod - 1);
      double[] lagRing_adx = new double[lagCap_adx];
      for( int lagI = 0; lagI < lagCap_adx; lagI++ ) {
         lagRing_adx[lagI] = adx[outNBElement.value + lagI];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sub0 = sub0;
      sp.lagRingPos_adx = 0;
      sp.lagRingCap_adx = lagCap_adx;
      sp.lagRing_adx = lagRing_adx;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      System.arraycopy(sc_outReal, 0, outReal, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind adxrOpen (composition seam). */
   AdxrStream adxrOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      AdxrStream sp = new AdxrStream(this);
      RetCode retCode = adxrOpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADXR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADXR open: internal error");
      }
      throw new IllegalArgumentException("TA_ADXR open: " + retCode);
   }
   /**
    * Open a live ADXR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#adxr} at that bar.
    * <p>The history must hold at least {@code adxrLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AdxrStream adxrOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return adxrOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#adxrOpen} that also fills the output array(s) bit-identically
    * to {@link Core#adxr} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AdxrStream adxrOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AdxrStream sp = new AdxrStream(this);
      RetCode retCode = adxrOpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADXR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADXR openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ADXR openAndFill: " + retCode);
   }
