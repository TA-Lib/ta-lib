/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120305 AC   Creation
 *  071226 MF,CC Streaming-friendly rewrite: carry the confirmation state
 *               (countdown + cached 2nd-candle high/low) instead of the absolute
 *               bar index, so the per-bar logic reads no cursor. Bit-identical
 *               batch results (verified vs v0.6.4).
 */

   public int cdlHikkakeLookback( )
   {
      return 5 ;

   }
   public RetCode cdlHikkake( int startIdx,
                              int endIdx,
                              double inOpen[],
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
       * state carried without an absolute bar index.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlHikkakeLookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
       * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
       * outInteger[hikkakebar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 2nd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeUnguarded( int startIdx,
                                       int endIdx,
                                       double inOpen[],
                                       double inHigh[],
                                       double inLow[],
                                       double inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 && (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 && (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkake( int startIdx,
                              int endIdx,
                              float inOpen[],
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = (double)inHigh[i - 1];
            savedLow = (double)inLow[i - 1];
            cd = 4;
         } else if( cd > 0 && (patternResult > 0 && (double)inClose[i] > savedHigh || patternResult < 0 && (double)inClose[i] < savedLow) ) {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = (double)inHigh[i - 1];
            savedLow = (double)inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 && (patternResult > 0 && (double)inClose[i] > savedHigh || patternResult < 0 && (double)inClose[i] < savedLow) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeUnguarded( int startIdx,
                                       int endIdx,
                                       float inOpen[],
                                       float inHigh[],
                                       float inLow[],
                                       float inClose[],
                                       MInteger outBegIdx,
                                       MInteger outNBElement,
                                       int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = (double)inHigh[i - 1];
            savedLow = (double)inLow[i - 1];
            cd = 4;
         } else if( cd > 0 && (patternResult > 0 && (double)inClose[i] > savedHigh || patternResult < 0 && (double)inClose[i] < savedLow) ) {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = (double)inHigh[i - 1];
            savedLow = (double)inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 && (patternResult > 0 && (double)inClose[i] > savedHigh || patternResult < 0 && (double)inClose[i] < savedLow) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CDLHIKKAKE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cdlHikkake} over the same series.
    * Open with {@link Core#cdlHikkakeOpen}; there is no close — the handle is
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
   public static final class CdlHikkakeStream {
      final Core core;
      int patternResult;
      int cd;
      double savedHigh;
      double savedLow;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      int cur_outInteger;

      CdlHikkakeStream( Core core ) { this.core = core; }

      CdlHikkakeStream( CdlHikkakeStream other ) {
         this.core = other.core;
         this.patternResult = other.patternResult;
         this.cd = other.cd;
         this.savedHigh = other.savedHigh;
         this.savedLow = other.savedLow;
         this.lag1_inHigh = other.lag1_inHigh;
         this.lag2_inHigh = other.lag2_inHigh;
         this.lag1_inLow = other.lag1_inLow;
         this.lag2_inLow = other.lag2_inLow;
         this.cur_outInteger = other.cur_outInteger;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.cdlHikkakeStreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CdlHikkakeStream scratch = new CdlHikkakeStream(this);
         core.cdlHikkakeStreamStep(scratch, inOpen, inHigh, inLow, inClose);
         return scratch.cur_outInteger;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public CdlHikkakeStream copy() {
         return new CdlHikkakeStream(this);
      }
   }
   void cdlHikkakeStreamStep( CdlHikkakeStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
          (inHigh < sp.lag1_inHigh && inLow < sp.lag1_inLow || inHigh > sp.lag1_inHigh && inLow > sp.lag1_inLow) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
      {
         sp.patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
         sp.savedHigh = sp.lag1_inHigh;
         sp.savedLow = sp.lag1_inLow;
         sp.cd = 4;
         sp.cur_outInteger = sp.patternResult;
      } else if( sp.cd > 0 &&
          (sp.patternResult > 0 && inClose > sp.savedHigh || sp.patternResult < 0 && inClose < sp.savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
      {
         sp.cur_outInteger = sp.patternResult + 100 * ((sp.patternResult > 0) ? 1 : 0 - 1);
         sp.cd = 0;
      } else {
         sp.cur_outInteger = 0;
      }
      if( sp.cd > 0 ) {
         sp.cd -= 1;
      }
      sp.lag2_inHigh = sp.lag1_inHigh;
      sp.lag1_inHigh = inHigh;
      sp.lag2_inLow = sp.lag1_inLow;
      sp.lag1_inLow = inLow;
   }
   private RetCode cdlHikkakeOpenBody( CdlHikkakeStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
       * state carried without an absolute bar index.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlHikkakeLookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
       * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
       * outInteger[hikkakebar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 2nd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            lastValue_outInteger = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            lastValue_outInteger = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            lastValue_outInteger = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.patternResult = patternResult;
      sp.cd = cd;
      sp.savedHigh = savedHigh;
      sp.savedLow = savedLow;
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode cdlHikkakeOpenAndFillBody( CdlHikkakeStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int patternResult = 0;
      int cd = 0;
      double savedHigh = 0;
      double savedLow = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
       * state carried without an absolute bar index.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlHikkakeLookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      cd = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            cd = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
       * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
       * outInteger[hikkakebar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 2nd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 && inClose[i] > savedHigh || patternResult < 0 && inClose[i] < savedLow) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         if( cd > 0 ) {
            cd -= 1;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.patternResult = patternResult;
      sp.cd = cd;
      sp.savedHigh = savedHigh;
      sp.savedLow = savedLow;
      sp.lag1_inHigh = inHigh[historyLen - 1];
      sp.lag2_inHigh = inHigh[historyLen - 2];
      sp.lag1_inLow = inLow[historyLen - 1];
      sp.lag2_inLow = inLow[historyLen - 2];
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cdlHikkakeOpen (composition seam). */
   CdlHikkakeStream cdlHikkakeOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlHikkakeStream sp = new CdlHikkakeStream(this);
      RetCode retCode = cdlHikkakeOpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLHIKKAKE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLHIKKAKE open: internal error");
      }
      throw new IllegalArgumentException("TA_CDLHIKKAKE open: " + retCode);
   }
   /**
    * Open a live CDLHIKKAKE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cdlHikkake} at that bar.
    * <p>The history must hold at least {@code cdlHikkakeLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CdlHikkakeStream cdlHikkakeOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return cdlHikkakeOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlHikkakeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cdlHikkake} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CdlHikkakeStream cdlHikkakeOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlHikkakeStream sp = new CdlHikkakeStream(this);
      RetCode retCode = cdlHikkakeOpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CDLHIKKAKE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CDLHIKKAKE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CDLHIKKAKE openAndFill: " + retCode);
   }
