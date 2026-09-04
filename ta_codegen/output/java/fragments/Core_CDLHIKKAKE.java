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

   /**
    * Number of leading input bars {@link Core#CDLHIKKAKE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLHIKKAKE_Lookback( )
   {
      return 5 ;

   }
   RetCode CDLHIKKAKE_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      savedHigh = 0.0;
      savedLow = 0.0;
      /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
       * state carried without an absolute bar index.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIKKAKE_Lookback();
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1]) )     /* (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 &&
             (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
               inClose[i] > savedHigh || /* close higher than the high of 2nd */
              patternResult < 0 &&
               inClose[i] < savedLow) )  /* close lower than the low of 2nd */
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1]) )     /* (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            outInteger[outIdx++] = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
               inClose[i] > savedHigh || /* close higher than the high of 2nd */
              patternResult < 0 &&
               inClose[i] < savedLow) )  /* close lower than the low of 2nd */
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
   RetCode CDLHIKKAKE_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      savedHigh = 0.0;
      savedLow = 0.0;
      lookbackTotal = CDLHIKKAKE_Lookback();
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
   /**
    * A 3-bar pattern: an inside bar followed by a false breakout, optionally
    * later confirmed by a follow-through bar. Signals a bullish or bearish
    * reversal/continuation depending on the breakout direction. A
    * false-breakout setup: positive = bullish, negative = bearish; magnitude
    * 200 flags the confirming bar.
    * <p><b>Notes</b>
    * <ul>
    * <li>The name comes from the Japanese word for a deceptive move or "trap" — fitting, since the pattern exists to catch traders acting on a false breakout. Bulkowski's testing of the confirmed pattern found the trap itself barely beats a coin flip: the bullish variant continues as expected only 52% of the time and the bearish variant exactly 50% ("random"), both ranking in the bottom fifth (83rd-84th of 105) for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIKKAKE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/-100 at the hikkake (breakout) bar for bull/bear;
    *        +200/-200 at a later confirmation bar; 0 otherwise. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CDLHIKKAKEMOD
    * @see Core#CDLHARAMI
    */
   public OutRange CDLHIKKAKE( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               int outInteger[] )
   {
      requireIndexRange("CDLHIKKAKE", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIKKAKE", startIdx, CDLHIKKAKE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIKKAKE", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIKKAKE", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIKKAKE", "inLow", inLow, guardInLen);
      requireLength("CDLHIKKAKE", "inClose", inClose, guardInLen);
      requireLength("CDLHIKKAKE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIKKAKE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A 3-bar pattern: an inside bar followed by a false breakout, optionally
    * later confirmed by a follow-through bar. Signals a bullish or bearish
    * reversal/continuation depending on the breakout direction. A
    * false-breakout setup: positive = bullish, negative = bearish; magnitude
    * 200 flags the confirming bar.
    * <p><b>Notes</b>
    * <ul>
    * <li>The name comes from the Japanese word for a deceptive move or "trap" — fitting, since the pattern exists to catch traders acting on a false breakout. Bulkowski's testing of the confirmed pattern found the trap itself barely beats a coin flip: the bullish variant continues as expected only 52% of the time and the bearish variant exactly 50% ("random"), both ranking in the bottom fifth (83rd-84th of 105) for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLHIKKAKE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100/-100 at the hikkake (breakout) bar for bull/bear;
    *        +200/-200 at a later confirmation bar; 0 otherwise. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#CDLHIKKAKEMOD
    * @see Core#CDLHARAMI
    */
   public OutRange CDLHIKKAKE( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               int outInteger[] )
   {
      requireIndexRange("CDLHIKKAKE", startIdx, endIdx);
      int guardStart = clampedStart("CDLHIKKAKE", startIdx, CDLHIKKAKE_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CDLHIKKAKE", "inOpen", inOpen, guardInLen);
      requireLength("CDLHIKKAKE", "inHigh", inHigh, guardInLen);
      requireLength("CDLHIKKAKE", "inLow", inLow, guardInLen);
      requireLength("CDLHIKKAKE", "inClose", inClose, guardInLen);
      requireLength("CDLHIKKAKE", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLHIKKAKE_Impl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLHIKKAKE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLHIKKAKE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLHIKKAKE} over the same series.
    * Open with {@link Core#cdlhikkakeOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CdlhikkakeStream {
      Core core;
      int patternResult;
      int cd;
      double savedHigh;
      double savedLow;
      double lag1_inHigh;
      double lag2_inHigh;
      double lag1_inLow;
      double lag2_inLow;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      CdlhikkakeStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CDLHIKKAKE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CdlhikkakeStream( CdlhikkakeStream other ) {
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
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("CDLHIKKAKE update: BadParam", RetCode.BadParam);
         }
         core.cdlhikkakeStepImpl(this, inOpen, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] ) {
         requireArgument("CDLHIKKAKE updateAndFill", "inOpen", inOpen);
         requireArgument("CDLHIKKAKE updateAndFill", "inHigh", inHigh);
         requireArgument("CDLHIKKAKE updateAndFill", "inLow", inLow);
         requireArgument("CDLHIKKAKE updateAndFill", "inClose", inClose);
         requireArgument("CDLHIKKAKE updateAndFill", "outInteger", outInteger);
         final int barCount = inOpen.length;
         if( inHigh.length != barCount || inLow.length != barCount || inClose.length != barCount || outInteger.length < barCount || (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose )
            throw new TaLibArgumentException("CDLHIKKAKE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CDLHIKKAKE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cdlhikkakeStepImpl(this, inOpen[i], inHigh[i], inLow[i], inClose[i]);
            outInteger[i] = this.cur_outInteger;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CDLHIKKAKE peek: BadParam", RetCode.BadParam);
         CdlhikkakeStream sp = this;
         int cd = sp.cd;
         int cur_outInteger = 0;
         int patternResult = sp.patternResult;
         double savedHigh = sp.savedHigh;
         double savedLow = sp.savedLow;
         if( sp.lag1_inHigh < sp.lag2_inHigh &&
             sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
             (inHigh < sp.lag1_inHigh &&
               inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
              inHigh > sp.lag1_inHigh &&
               inLow > sp.lag1_inLow) )         /* (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
            savedHigh = sp.lag1_inHigh;
            savedLow = sp.lag1_inLow;
            cd = 4;
            cur_outInteger = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
               inClose > savedHigh || /* close higher than the high of 2nd */
              patternResult < 0 &&
               inClose < savedLow) )  /* close lower than the low of 2nd */
         {
            cur_outInteger = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            cur_outInteger = 0;
         }
         return cur_outInteger;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public CdlhikkakeStream clone() {
         return new CdlhikkakeStream(this);
      }
   }
   void cdlhikkakeStepImpl( CdlhikkakeStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      if( sp.lag1_inHigh < sp.lag2_inHigh &&
          sp.lag1_inLow > sp.lag2_inLow &&   /* 1st + 2nd: lower high and higher low */
          (inHigh < sp.lag1_inHigh &&
            inLow < sp.lag1_inLow ||         /* (bull) 3rd: lower high and lower low */
           inHigh > sp.lag1_inHigh &&
            inLow > sp.lag1_inLow) )         /* (bear) 3rd: higher high and higher low */
      {
         sp.patternResult = 100 * ((inHigh < sp.lag1_inHigh) ? 1 : 0 - 1);
         sp.savedHigh = sp.lag1_inHigh;
         sp.savedLow = sp.lag1_inLow;
         sp.cd = 4;
         sp.cur_outInteger = sp.patternResult;
      } else if( sp.cd > 0 &&
          (sp.patternResult > 0 &&    /* search for confirmation if hikkake was no more than 3 bars ago */
            inClose > sp.savedHigh || /* close higher than the high of 2nd */
           sp.patternResult < 0 &&
            inClose < sp.savedLow) )  /* close lower than the low of 2nd */
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
   private RetCode cdlhikkakeOpenImpl( CdlhikkakeStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      savedHigh = 0.0;
      savedLow = 0.0;
      /* Confirmation-window countdown + cached 2nd-candle high/low: the pattern
       * state carried without an absolute bar index.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLHIKKAKE_Lookback();
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
         return RetCode.InsufficientHistory ;
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1]) )     /* (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
         } else if( cd > 0 &&
             (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
               inClose[i] > savedHigh || /* close higher than the high of 2nd */
              patternResult < 0 &&
               inClose[i] < savedLow) )  /* close lower than the low of 2nd */
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
             (inHigh[i] < inHigh[i - 1] &&
               inLow[i] < inLow[i - 1] ||     /* (bull) 3rd: lower high and lower low */
              inHigh[i] > inHigh[i - 1] &&
               inLow[i] > inLow[i - 1]) )     /* (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            savedHigh = inHigh[i - 1];
            savedLow = inLow[i - 1];
            cd = 4;
            outInteger[outIdx++ * outStride] = patternResult;
         } else if( cd > 0 &&
             (patternResult > 0 &&       /* search for confirmation if hikkake was no more than 3 bars ago */
               inClose[i] > savedHigh || /* close higher than the high of 2nd */
              patternResult < 0 &&
               inClose[i] < savedLow) )  /* close lower than the low of 2nd */
         {
            outInteger[outIdx++ * outStride] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            cd = 0;
         } else {
            outInteger[outIdx++ * outStride] = 0;
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
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cdlhikkakeOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CdlhikkakeStream cdlhikkakeOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      CdlhikkakeStream sp = new CdlhikkakeStream(this);
      RetCode retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIKKAKE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIKKAKE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIKKAKE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cdlhikkakeOpen (composition seam). */
   CdlhikkakeStream cdlhikkakeOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CdlhikkakeStream sp = new CdlhikkakeStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = cdlhikkakeOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CDLHIKKAKE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CDLHIKKAKE open: internal error", retCode);
      }
      throw new TaLibArgumentException("CDLHIKKAKE open: " + retCode, retCode);
   }
   /**
    * Open a live CDLHIKKAKE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLHIKKAKE} at that bar.
    * <p>The history must hold at least {@code CDLHIKKAKE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CdlhikkakeStream cdlhikkakeOpen( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("CDLHIKKAKE open", "inOpen", inOpen);
      requireHistory("CDLHIKKAKE open", inOpen.length);
      requireArgument("CDLHIKKAKE open", "inHigh", inHigh);
      requireArgument("CDLHIKKAKE open", "inLow", inLow);
      requireArgument("CDLHIKKAKE open", "inClose", inClose);
      requireHistoryLength("CDLHIKKAKE open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKE open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKE open", "inClose", inClose.length, inOpen.length);
      return cdlhikkakeOpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#cdlhikkakeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CDLHIKKAKE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CdlhikkakeStream#outRange()}.
    */
   public CdlhikkakeStream cdlhikkakeOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      requireArgument("CDLHIKKAKE openAndFill", "inOpen", inOpen);
      requireHistory("CDLHIKKAKE openAndFill", inOpen.length);
      requireArgument("CDLHIKKAKE openAndFill", "inHigh", inHigh);
      requireArgument("CDLHIKKAKE openAndFill", "inLow", inLow);
      requireArgument("CDLHIKKAKE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CDLHIKKAKE openAndFill", inOpen.length, CDLHIKKAKE_Lookback());
      requireHistoryLength("CDLHIKKAKE openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKE openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("CDLHIKKAKE openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("CDLHIKKAKE openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         throw new TaLibArgumentException("CDLHIKKAKE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cdlhikkakeOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outInteger);
   }
