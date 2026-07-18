/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090812 AB     Initial Version
 */

   public int avgDevLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode avgDev( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
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
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      /* Make sure there is still something to evaluate. */
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Process the initial DM and TR */
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode avgDevUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode avgDev( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
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
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += (double)inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs((double)inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode avgDevUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += (double)inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs((double)inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live AVGDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#avgDev} over the same series.
    * Open with {@link Core#avgDevOpen}; there is no close — the handle is
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
   public static final class AvgDevStream {
      final Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inReal;
      double cur_outReal;

      AvgDevStream( Core core ) { this.core = core; }

      AvgDevStream( AvgDevStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inReal = other.win_i_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.avgDevStreamStep(this, inReal);
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
         AvgDevStream scratch = new AvgDevStream(this);
         core.avgDevStreamStep(scratch, inReal);
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
      public AvgDevStream copy() {
         return new AvgDevStream(this);
      }
   }
   void avgDevStreamStep( AvgDevStream sp, double inReal )
   {
      double todaySum = 0.0;
      double todayDev = 0.0;
      int i = 0;
      sp.win_i_inReal[sp.winPos_i] = inReal;
      todaySum = 0.0;
      for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
         todaySum += sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
      }
      todayDev = 0.0;
      for( i = 0; i < sp.optInTimePeriod; i += 1 ) {
         todayDev += Math.abs(sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] - todaySum / sp.optInTimePeriod);
      }
      sp.cur_outReal = todayDev / sp.optInTimePeriod;
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode avgDevOpenBody( AvgDevStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      /* Make sure there is still something to evaluate. */
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Process the initial DM and TR */
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         lastValue_outReal = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inReal = new double[cap_i];
      System.arraycopy(inReal, historyLen - cap_i, capWin_i_inReal, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inReal = capWin_i_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode avgDevOpenAndFillBody( AvgDevStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookback = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      lookback = optInTimePeriod - 1;
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      today = startIdx;
      /* Make sure there is still something to evaluate. */
      if( today > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Process the initial DM and TR */
      outBegIdx.value = today;
      outIdx = 0;
      while( today <= endIdx ) {
         double todaySum;
         double todayDev;
         int i;
         todaySum = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todaySum += inReal[today - i];
         }
         todayDev = 0.0;
         for( i = 0; i < optInTimePeriod; i += 1 ) {
            todayDev += Math.abs(inReal[today - i] - todaySum / optInTimePeriod);
         }
         outReal[outIdx] = todayDev / optInTimePeriod;
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inReal = new double[cap_i];
      System.arraycopy(inReal, historyLen - cap_i, capWin_i_inReal, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inReal = capWin_i_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind avgDevOpen (composition seam). */
   AvgDevStream avgDevOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      AvgDevStream sp = new AvgDevStream(this);
      RetCode retCode = avgDevOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AVGDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AVGDEV open: internal error");
      }
      throw new IllegalArgumentException("TA_AVGDEV open: " + retCode);
   }
   /**
    * Open a live AVGDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#avgDev} at that bar.
    * <p>The history must hold at least {@code avgDevLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AvgDevStream avgDevOpen( double inReal[], int optInTimePeriod )
   {
      return avgDevOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#avgDevOpen} that also fills the output array(s) bit-identically
    * to {@link Core#avgDev} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AvgDevStream avgDevOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AvgDevStream sp = new AvgDevStream(this);
      RetCode retCode = avgDevOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AVGDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AVGDEV openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_AVGDEV openAndFill: " + retCode);
   }
