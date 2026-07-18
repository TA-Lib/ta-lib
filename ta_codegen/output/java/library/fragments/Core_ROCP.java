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

   public int rocPLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   public RetCode rocP( int startIdx,
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
      /* Calculate Rate of change Ratio: (price / prevPrice) */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (inReal[inIdx] - tempReal) / tempReal;
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
   public RetCode rocPUnguarded( int startIdx,
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
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (inReal[inIdx] - tempReal) / tempReal;
         } else {
            outReal[outIdx++] = 0.0;
         }
         inIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode rocP( int startIdx,
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
         tempReal = (double)inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = ((double)inReal[inIdx] - tempReal) / tempReal;
         } else {
            outReal[outIdx++] = 0.0;
         }
         inIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode rocPUnguarded( int startIdx,
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
            outReal[outIdx++] = ((double)inReal[inIdx] - tempReal) / tempReal;
         } else {
            outReal[outIdx++] = 0.0;
         }
         inIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live ROCP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#rocP} over the same series.
    * Open with {@link Core#rocPOpen}; there is no close — the handle is
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
   public static final class RocPStream {
      final Core core;
      int optInTimePeriod;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      RocPStream( Core core ) { this.core = core; }

      RocPStream( RocPStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.rocPStreamStep(this, inReal);
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
         RocPStream scratch = new RocPStream(this);
         core.rocPStreamStep(scratch, inReal);
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
      public RocPStream copy() {
         return new RocPStream(this);
      }
   }
   void rocPStreamStep( RocPStream sp, double inReal )
   {
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      if( tempReal != 0.0 ) {
         sp.cur_outReal = (inReal - tempReal) / tempReal;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode rocPOpenBody( RocPStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      double tempReal = 0;
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
      /* Calculate Rate of change Ratio: (price / prevPrice) */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            lastValue_outReal = (inReal[inIdx] - tempReal) / tempReal;
         } else {
            lastValue_outReal = 0.0;
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode rocPOpenAndFillBody( RocPStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      double tempReal = 0;
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
      /* Calculate Rate of change Ratio: (price / prevPrice) */
      outIdx = 0;
      inIdx = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      while( inIdx <= endIdx ) {
         tempReal = inReal[trailingIdx++];
         if( tempReal != 0.0 ) {
            outReal[outIdx++] = (inReal[inIdx] - tempReal) / tempReal;
         } else {
            outReal[outIdx++] = 0.0;
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
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind rocPOpen (composition seam). */
   RocPStream rocPOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      RocPStream sp = new RocPStream(this);
      RetCode retCode = rocPOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ROCP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ROCP open: internal error");
      }
      throw new IllegalArgumentException("TA_ROCP open: " + retCode);
   }
   /**
    * Open a live ROCP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#rocP} at that bar.
    * <p>The history must hold at least {@code rocPLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public RocPStream rocPOpen( double inReal[], int optInTimePeriod )
   {
      return rocPOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#rocPOpen} that also fills the output array(s) bit-identically
    * to {@link Core#rocP} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public RocPStream rocPOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RocPStream sp = new RocPStream(this);
      RetCode retCode = rocPOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ROCP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ROCP openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ROCP openAndFill: " + retCode);
   }
