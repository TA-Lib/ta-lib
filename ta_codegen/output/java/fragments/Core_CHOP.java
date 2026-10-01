/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092926 MF,CC  Initial version (#469).
 */

   /**
    * Number of leading input bars {@link Core#chop} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int chopLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#chop}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of bars in the window (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int chopDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( chopLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode chopImpl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumTR = 0;
      double logPeriod = 0;
      double tempHT = 0;
      double tempLT = 0;
      double prevClose = 0;
      double trueHigh = 0;
      double trueLow = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = chopLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      logPeriod = Math.log10((double)optInTimePeriod);
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Re-summed oldest to newest every bar, never a running total: a
          * running total leaves a residue on a window of zero true ranges, which
          * the exact guard below would then read as a trend.
          */
         highest = inHigh[today];
         lowest = inLow[today];
         sumTR = 0.0;
         prevClose = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            if( i < optInTimePeriod ) {
               tempHT = inHigh[today - i];
               tempLT = inLow[today - i];
               trueHigh = tempHT;
               if( prevClose > trueHigh ) {
                  trueHigh = prevClose;
               }
               trueLow = tempLT;
               if( prevClose < trueLow ) {
                  trueLow = prevClose;
               }
               sumTR += trueHigh - trueLow;
               if( tempHT > highest ) {
                  highest = tempHT;
               }
               if( tempLT < lowest ) {
                  lowest = tempLT;
               }
            }
            prevClose = inClose[today - i];
         }
         /* Exact tests, never an epsilon band (issue #253). Keep the "<=" form:
          * a NaN box, or a NaN sum over a non-empty box, must reach the log
          * rather than read as 100.
          * Keep the parenthesised quotient: it returns exactly 100 whenever the
          * computed ratio is exactly the period.
          */
         if( highest - lowest <= 0.0 || sumTR <= 0.0 ) {
            outReal[outIdx] = 100.0;
         } else {
            outReal[outIdx] = 100.0 * (Math.log10(sumTR / (highest - lowest)) / logPeriod);
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode chopImpl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumTR = 0;
      double logPeriod = 0;
      double tempHT = 0;
      double tempLT = 0;
      double prevClose = 0;
      double trueHigh = 0;
      double trueLow = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = chopLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      logPeriod = Math.log10((double)optInTimePeriod);
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         highest = (double)inHigh[today];
         lowest = (double)inLow[today];
         sumTR = 0.0;
         prevClose = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            if( i < optInTimePeriod ) {
               tempHT = (double)inHigh[today - i];
               tempLT = (double)inLow[today - i];
               trueHigh = tempHT;
               if( prevClose > trueHigh ) {
                  trueHigh = prevClose;
               }
               trueLow = tempLT;
               if( prevClose < trueLow ) {
                  trueLow = prevClose;
               }
               sumTR += trueHigh - trueLow;
               if( tempHT > highest ) {
                  highest = tempHT;
               }
               if( tempLT < lowest ) {
                  lowest = tempLT;
               }
            }
            prevClose = (double)inClose[today - i];
         }
         if( highest - lowest <= 0.0 || sumTR <= 0.0 ) {
            outReal[outIdx] = 100.0;
         } else {
            outReal[outIdx] = 100.0 * (Math.log10(sumTR / (highest - lowest)) / logPeriod);
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Choppiness Index: a trend-versus-consolidation gauge, the true range
    * travelled over a window against the height of the box its bars span.
    * Devised by Bill Dreiss. Log-scaled so that a straight run reads 0 and bars
    * that each fill the whole box read 100. Like ADX and VHF it measures
    * whether the market is trending, not in which direction: low values mean a
    * directional run, high values sideways chop.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/chop">ta-lib.org/functions/chop</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The box is the window's highest high minus its lowest low. The 1993 publication instead spans the highest true high and the lowest true low, which also reach the close just before each bar; that form is CHOPTR. The two differ only on bars where the close before the window lies outside the window's high-low range: a gap into the window.</li>
    * <li>Because a gap into the window adds to the true range but not to the box, CHOP has no upper bound and can exceed 100.</li>
    * <li>A window with no box height, or no true range, reports 100.</li>
    * <li>Dreiss's 3-bar smoothing of the index is not built in; apply a moving average to {@code outReal} to obtain it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#chopLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the window (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Choppiness Index value. Must hold at least
    *        {@code endIdx - max(startIdx, chopLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#choptr
    * @see Core#vhf
    * @see Core#adx
    * @see Core#trange
    */
   public OutRange chop( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("CHOP", startIdx, endIdx);
      int guardStart = clampedStart("CHOP", startIdx, chopLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CHOP", "inHigh", inHigh, guardInLen);
      requireLength("CHOP", "inLow", inLow, guardInLen);
      requireLength("CHOP", "inClose", inClose, guardInLen);
      requireLength("CHOP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = chopImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CHOP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Choppiness Index: a trend-versus-consolidation gauge, the true range
    * travelled over a window against the height of the box its bars span.
    * Devised by Bill Dreiss. Log-scaled so that a straight run reads 0 and bars
    * that each fill the whole box read 100. Like ADX and VHF it measures
    * whether the market is trending, not in which direction: low values mean a
    * directional run, high values sideways chop.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/chop">ta-lib.org/functions/chop</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The box is the window's highest high minus its lowest low. The 1993 publication instead spans the highest true high and the lowest true low, which also reach the close just before each bar; that form is CHOPTR. The two differ only on bars where the close before the window lies outside the window's high-low range: a gap into the window.</li>
    * <li>Because a gap into the window adds to the true range but not to the box, CHOP has no upper bound and can exceed 100.</li>
    * <li>A window with no box height, or no true range, reports 100.</li>
    * <li>Dreiss's 3-bar smoothing of the index is not built in; apply a moving average to {@code outReal} to obtain it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#chopLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the window (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Choppiness Index value. Must hold at least
    *        {@code endIdx - max(startIdx, chopLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#choptr
    * @see Core#vhf
    * @see Core#adx
    * @see Core#trange
    */
   public OutRange chop( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("CHOP", startIdx, endIdx);
      int guardStart = clampedStart("CHOP", startIdx, chopLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CHOP", "inHigh", inHigh, guardInLen);
      requireLength("CHOP", "inLow", inLow, guardInLen);
      requireLength("CHOP", "inClose", inClose, guardInLen);
      requireLength("CHOP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = chopImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CHOP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CHOP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#chop} over the same series.
    * Open with {@link Core#chopOpen}; there is no close — the handle is
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
   public static final class ChopStream {
      private Core core;
      private int optInTimePeriod;
      private double logPeriod;
      private int winPos_i;
      private int winCap_i;
      private double[] win_i_inHigh;
      private double[] win_i_inLow;
      private double[] win_i_inClose;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private ChopStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#chop} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CHOP advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private ChopStream( ChopStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.logPeriod = other.logPeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inHigh = other.win_i_inHigh.clone();
         this.win_i_inLow = other.win_i_inLow.clone();
         this.win_i_inClose = other.win_i_inClose.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CHOP update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CHOP update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.chopStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CHOP peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         ChopStream sp = this;
         int i = 0;
         double highest = 0.0;
         double lowest = 0.0;
         double sumTR = 0.0;
         double tempHT = 0.0;
         double tempLT = 0.0;
         double prevClose = 0.0;
         double trueHigh = 0.0;
         double trueLow = 0.0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inHigh;
         pkSlot1 = sp.winPos_i;
         pkVal1 = inLow;
         pkSlot2 = sp.winPos_i;
         pkVal2 = inClose;
         /* Re-summed oldest to newest every bar, never a running total: a
          * running total leaves a residue on a window of zero true ranges, which
          * the exact guard below would then read as a trend.
          */
         highest = inHigh;
         lowest = inLow;
         sumTR = 0.0;
         prevClose = 0.0;
         for( i = sp.optInTimePeriod; i >= 0; i -= 1 ) {
            if( i < sp.optInTimePeriod ) {
               tempHT = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
               tempLT = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot1) ? sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal1;
               trueHigh = tempHT;
               if( prevClose > trueHigh ) {
                  trueHigh = prevClose;
               }
               trueLow = tempLT;
               if( prevClose < trueLow ) {
                  trueLow = prevClose;
               }
               sumTR += trueHigh - trueLow;
               if( tempHT > highest ) {
                  highest = tempHT;
               }
               if( tempLT < lowest ) {
                  lowest = tempLT;
               }
            }
            prevClose = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot2) ? sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal2;
         }
         /* Exact tests, never an epsilon band (issue #253). Keep the "<=" form:
          * a NaN box, or a NaN sum over a non-empty box, must reach the log
          * rather than read as 100.
          * Keep the parenthesised quotient: it returns exactly 100 whenever the
          * computed ratio is exactly the period.
          */
         if( highest - lowest <= 0.0 || sumTR <= 0.0 ) {
            cur_outReal = 100.0;
         } else {
            cur_outReal = 100.0 * (Math.log10(sumTR / (highest - lowest)) / sp.logPeriod);
         }
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
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
      public ChopStream clone() {
         return new ChopStream(this);
      }
   }
   private void chopStepImpl( ChopStream sp, double inHigh, double inLow, double inClose )
   {
      int i = 0;
      double highest = 0.0;
      double lowest = 0.0;
      double sumTR = 0.0;
      double tempHT = 0.0;
      double tempLT = 0.0;
      double prevClose = 0.0;
      double trueHigh = 0.0;
      double trueLow = 0.0;
      sp.win_i_inHigh[sp.winPos_i] = inHigh;
      sp.win_i_inLow[sp.winPos_i] = inLow;
      sp.win_i_inClose[sp.winPos_i] = inClose;
      /* Re-summed oldest to newest every bar, never a running total: a
       * running total leaves a residue on a window of zero true ranges, which
       * the exact guard below would then read as a trend.
       */
      highest = inHigh;
      lowest = inLow;
      sumTR = 0.0;
      prevClose = 0.0;
      for( i = sp.optInTimePeriod; i >= 0; i -= 1 ) {
         if( i < sp.optInTimePeriod ) {
            tempHT = sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
            tempLT = sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
            trueHigh = tempHT;
            if( prevClose > trueHigh ) {
               trueHigh = prevClose;
            }
            trueLow = tempLT;
            if( prevClose < trueLow ) {
               trueLow = prevClose;
            }
            sumTR += trueHigh - trueLow;
            if( tempHT > highest ) {
               highest = tempHT;
            }
            if( tempLT < lowest ) {
               lowest = tempLT;
            }
         }
         prevClose = sp.win_i_inClose[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
      }
      /* Exact tests, never an epsilon band (issue #253). Keep the "<=" form:
       * a NaN box, or a NaN sum over a non-empty box, must reach the log
       * rather than read as 100.
       * Keep the parenthesised quotient: it returns exactly 100 whenever the
       * computed ratio is exactly the period.
       */
      if( highest - lowest <= 0.0 || sumTR <= 0.0 ) {
         sp.cur_outReal = 100.0;
      } else {
         sp.cur_outReal = 100.0 * (Math.log10(sumTR / (highest - lowest)) / sp.logPeriod);
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode chopOpenImpl( ChopStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumTR = 0;
      double logPeriod = 0;
      double tempHT = 0;
      double tempLT = 0;
      double prevClose = 0;
      double trueHigh = 0;
      double trueLow = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = chopLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      logPeriod = Math.log10((double)optInTimePeriod);
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Re-summed oldest to newest every bar, never a running total: a
          * running total leaves a residue on a window of zero true ranges, which
          * the exact guard below would then read as a trend.
          */
         highest = inHigh[today];
         lowest = inLow[today];
         sumTR = 0.0;
         prevClose = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            if( i < optInTimePeriod ) {
               tempHT = inHigh[today - i];
               tempLT = inLow[today - i];
               trueHigh = tempHT;
               if( prevClose > trueHigh ) {
                  trueHigh = prevClose;
               }
               trueLow = tempLT;
               if( prevClose < trueLow ) {
                  trueLow = prevClose;
               }
               sumTR += trueHigh - trueLow;
               if( tempHT > highest ) {
                  highest = tempHT;
               }
               if( tempLT < lowest ) {
                  lowest = tempLT;
               }
            }
            prevClose = inClose[today - i];
         }
         /* Exact tests, never an epsilon band (issue #253). Keep the "<=" form:
          * a NaN box, or a NaN sum over a non-empty box, must reach the log
          * rather than read as 100.
          * Keep the parenthesised quotient: it returns exactly 100 whenever the
          * computed ratio is exactly the period.
          */
         if( highest - lowest <= 0.0 || sumTR <= 0.0 ) {
            outReal[outIdx * outStride] = 100.0;
         } else {
            outReal[outIdx * outStride] = 100.0 * (Math.log10(sumTR / (highest - lowest)) / logPeriod);
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      double[] capWin_i_inHigh = new double[cap_i];
      System.arraycopy(inHigh, historyLen - cap_i, capWin_i_inHigh, 0, cap_i);
      double[] capWin_i_inLow = new double[cap_i];
      System.arraycopy(inLow, historyLen - cap_i, capWin_i_inLow, 0, cap_i);
      double[] capWin_i_inClose = new double[cap_i];
      System.arraycopy(inClose, historyLen - cap_i, capWin_i_inClose, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.logPeriod = logPeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inHigh = capWin_i_inHigh;
      sp.win_i_inLow = capWin_i_inLow;
      sp.win_i_inClose = capWin_i_inClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* chopOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ChopStream chopOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ChopStream sp = new ChopStream(this);
      RetCode retCode = chopOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CHOP openAndFill", inHigh.length, startIdx, chopLookback(optInTimePeriod));
      }
      throw streamFailure("CHOP openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind chopOpen (composition seam). */
   ChopStream chopOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ChopStream sp = new ChopStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = chopOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CHOP open", inHigh.length, startIdx, chopLookback(optInTimePeriod));
      }
      throw streamFailure("CHOP open", retCode);
   }
   /**
    * Open a live CHOP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#chop} at that bar.
    * <p>The history must hold at least {@code chopLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ChopStream chopOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("CHOP open", "inHigh", inHigh);
      requireHistory("CHOP open", inHigh.length);
      requireArgument("CHOP open", "inLow", inLow);
      requireArgument("CHOP open", "inClose", inClose);
      requireHistoryLength("CHOP open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CHOP open", "inClose", inClose.length, inHigh.length);
      return chopOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#chopOpen} that also fills the output array(s) bit-identically
    * to {@link Core#chop} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ChopStream#outRange()}.
    */
   public ChopStream chopOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CHOP openAndFill", "inHigh", inHigh);
      requireHistory("CHOP openAndFill", inHigh.length);
      requireArgument("CHOP openAndFill", "inLow", inLow);
      requireArgument("CHOP openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CHOP openAndFill", inHigh.length, chopLookback(optInTimePeriod));
      requireHistoryLength("CHOP openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CHOP openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("CHOP openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw streamFailure("CHOP openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return chopOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
