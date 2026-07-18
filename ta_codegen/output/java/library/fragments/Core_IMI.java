/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *  MF       Mario Fortier
 *  WZ       wony (github @wony-zheng)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 *  070726 WZ,CC  (#14) IMI has no unstable period; drop the unstable-period
 *                term from the lookback so TA_SetUnstablePeriod is a no-op.
 *  071326 MF,CC  Fix #112: an all-flat window (every close==open) leaves
 *                upsum==downsum==0, so 100*(0/0) emitted NaN from a *successful*
 *                call. Guard the divide, returning IMI's neutral center 50.0.
 */

   public int imiLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode imi( int startIdx,
                       int endIdx,
                       double inOpen[],
                       double inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
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
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imiUnguarded( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imi( int startIdx,
                       int endIdx,
                       float inOpen[],
                       float inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
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
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = (double)inClose[i];
            double open = (double)inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imiUnguarded( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = (double)inClose[i];
            double open = (double)inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live IMI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#imi} over the same series.
    * Open with {@link Core#imiOpen}; there is no close — the handle is
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
   public static final class ImiStream {
      final Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inOpen;
      double[] win_i_inClose;
      double cur_outReal;

      ImiStream( Core core ) { this.core = core; }

      ImiStream( ImiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inOpen = other.win_i_inOpen.clone();
         this.win_i_inClose = other.win_i_inClose.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inOpen, double inClose ) {
         core.imiStreamStep(this, inOpen, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inOpen, double inClose ) {
         ImiStream scratch = new ImiStream(this);
         core.imiStreamStep(scratch, inOpen, inClose);
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
      public ImiStream copy() {
         return new ImiStream(this);
      }
   }
   void imiStreamStep( ImiStream sp, double inOpen, double inClose )
   {
      double upsum = 0.0;
      double downsum = 0.0;
      int i = 0;
      double close = 0.0;
      double open = 0.0;
      sp.win_i_inOpen[sp.winPos_i] = inOpen;
      sp.win_i_inClose[sp.winPos_i] = inClose;
      upsum = 0.0;
      downsum = 0.0;
      for( i = sp.optInTimePeriod - 1; i >= 0; i -= 1 ) {
         close = sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         open = sp.win_i_inOpen[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         if( close > open ) {
            upsum += close - open;
         } else {
            downsum += open - close;
         }
         /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
          * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
          * oscillator, so no up/down bias returns its neutral center, 50.0.
          */
         sp.cur_outReal = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode imiOpenBody( ImiStream sp, double inOpen[], double inClose[], int startIdx, int optInTimePeriod )
   {
      int lookback = 0;
      int outIdx = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            lastValue_outReal = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod - 1 + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inOpen = new double[cap_i];
      System.arraycopy(inOpen, historyLen - cap_i, capWin_i_inOpen, 0, cap_i);
      double[] capWin_i_inClose = new double[cap_i];
      System.arraycopy(inClose, historyLen - cap_i, capWin_i_inClose, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inOpen = capWin_i_inOpen;
      sp.win_i_inClose = capWin_i_inClose;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode imiOpenAndFillBody( ImiStream sp, double inOpen[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod - 1 + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inOpen = new double[cap_i];
      System.arraycopy(inOpen, historyLen - cap_i, capWin_i_inOpen, 0, cap_i);
      double[] capWin_i_inClose = new double[cap_i];
      System.arraycopy(inClose, historyLen - cap_i, capWin_i_inClose, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inOpen = capWin_i_inOpen;
      sp.win_i_inClose = capWin_i_inClose;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind imiOpen (composition seam). */
   ImiStream imiOpenInternal( double inOpen[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ImiStream sp = new ImiStream(this);
      RetCode retCode = imiOpenBody(sp, inOpen, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_IMI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_IMI open: internal error");
      }
      throw new IllegalArgumentException("TA_IMI open: " + retCode);
   }
   /**
    * Open a live IMI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#imi} at that bar.
    * <p>The history must hold at least {@code imiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public ImiStream imiOpen( double inOpen[], double inClose[], int optInTimePeriod )
   {
      return imiOpenInternal(inOpen, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#imiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#imi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public ImiStream imiOpenAndFill( double inOpen[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ImiStream sp = new ImiStream(this);
      RetCode retCode = imiOpenAndFillBody(sp, inOpen, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_IMI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_IMI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_IMI openAndFill: " + retCode);
   }
