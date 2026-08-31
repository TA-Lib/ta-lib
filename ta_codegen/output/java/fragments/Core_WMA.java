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
 *  082426 MF,CC Fix #255. Re-anchor the running sums: every 32*period bars,
 *               and on the bar a large value leaves the window.
 */

   /**
    * Number of leading input bars {@link Core#WMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int WMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode WMA_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int j = 0;
      int rw = 0;
      int lookbackWin = 0;
      int barsSinceReseed = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
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
      /* To make the rest more efficient, handle exception
       * case where the user is asking for a period of '1'.
       * In that case outputs equals inputs for the requested
       * range.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         /* Element loop, not a block copy: the C single-precision variant reads a
          * float array, so a double-sized byte copy would reinterpret and
          * over-read it (#137). Forward order keeps the in-place case correct (#94).
          */
         inIdx = startIdx;
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = inReal[inIdx++];
         }
         return RetCode.Success ;
      }
      /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
       * int product n*(n+1) overflows int32 at n>=46341 (#142).
       */
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      /* The algo used here use a very basic property of
       * multiplication/addition: (x*2) = x+x
       *
       * As an example, a 3 period weighted can be
       * interpreted in two way:
       *  (x1*1)+(x2*2)+(x3*3)
       *      OR
       *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
       *
       * When you move forward in the time serie
       * you can quickly adjust the periodSum for the
       * period by substracting:
       *   x1+x2+x3 (This is the periodSub)
       * Making the new periodSum equals to:
       *   x2+x3+x3
       *
       * You can then add the new price bar
       * which is x4+x4+x4 giving:
       *   x2+x3+x3+x4+x4+x4
       *
       * At this point one iteration is completed and you can
       * see that we are back to the step 1 of this example.
       *
       * Why making it so un-intuitive? The number of memory
       * access and floating point operations are kept to a
       * minimum with this algo.
       */
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      /* Evaluate the initial periodSum/periodSub and trailingValue. */
      lookbackWin = optInTimePeriod - 1;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx];
         inIdx += 1;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      barsSinceReseed = 8 * optInTimePeriod;
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal[inIdx];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         /* Re-anchor: rebuild both totals from the window itself.
          *
          * periodSum and periodSub were running totals that were never
          * recomputed, so each bar's rounding joined a residue no later bar
          * could subtract, and its size was set by the largest value the totals
          * had ever held rather than by the current window. That is the defect
          * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
          * below is the same weight-shifting identity as that family's
          * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
          * it and TA_SMA, whose output lives at its own sum's scale, does not.
          * Measured before the fix: worst range disagreement 1.41e-08 at 200000
          * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
          * closes or ~1000 with one large print. After: 1.79e-12, flat in call
          * length.
          *
          * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
          * LINEARREG family also carries an OUTLIER trigger (rebuild when the
          * departing value outweighs the window) because for a slope the
          * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
          * that position: its weights are bounded by `period` and its divider is
          * period*(period+1)/2, which dilutes the residue enough that the
          * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
          * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
          * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
          * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
          * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
          * shorter interval buys most of the accuracy for ~1.1x instead.
          *
          * The rebuild walks the window OLDEST FIRST with the weight counting UP
          * from 1 -- the priming scan's own order and weighting -- so a
          * re-anchored bar is bit-identical to the same bar computed by a call
          * that started there. That identity is what the range-stability
          * contract measures, and what test_wma.c W2/W3 assert.
          *
          * The loop start is written INLINE rather than through a `windowStart`
          * local: only that form is recognised as a rescan window, which is what
          * keeps this on the stream classifier's primary path. See
          * docs/ta_codegen_input_code.md.
          *
          * Reading the window is safe when outReal aliases inReal: the outputs
          * written so far occupy [0, outIdx-1], and the window starts at
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInTimePeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            for( j = inIdx - lookbackWin; j <= inIdx; j += 1 ) {
               tempReal = inReal[j];
               periodSub += tempReal;
               periodSum += tempReal * rw;
               rw += 1;
            }
         }
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         /* Calculate the WMA for this price bar. */
         outReal[outIdx++] = periodSum / divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
         inIdx += 1;
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode WMA_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int j = 0;
      int rw = 0;
      int lookbackWin = 0;
      int barsSinceReseed = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         inIdx = startIdx;
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = (double)inReal[inIdx++];
         }
         return RetCode.Success ;
      }
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      lookbackWin = optInTimePeriod - 1;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = (double)inReal[inIdx];
         inIdx += 1;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      barsSinceReseed = 8 * optInTimePeriod;
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         tempReal = (double)inReal[inIdx];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInTimePeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            for( j = inIdx - lookbackWin; j <= inIdx; j += 1 ) {
               tempReal = (double)inReal[j];
               periodSub += tempReal;
               periodSum += tempReal * rw;
               rw += 1;
            }
         }
         trailingValue = (double)inReal[trailingIdx];
         trailingIdx += 1;
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
         inIdx += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Linearly weighted moving average: each of the last N prices is weighted by
    * its position, oldest getting weight 1 and newest weight N. Smooths price
    * while emphasizing recent bars.
    * <p><b>Formula</b>
    * <pre>{@code
    * WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/data series.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Weighted moving average series. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#MA
    * @see Core#DEMA
    * @see Core#TEMA
    */
   public OutRange WMA( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("WMA", startIdx, endIdx);
      int guardStart = clampedStart("WMA", startIdx, WMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WMA", "inReal", inReal, guardInLen);
      requireLength("WMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Linearly weighted moving average: each of the last N prices is weighted by
    * its position, oldest getting weight 1 and newest weight N. Smooths price
    * while emphasizing recent bars.
    * <p><b>Formula</b>
    * <pre>{@code
    * WMA = ( sum_{k=1..N} k * P_k ) / (N(N+1)/2), where P_N is the most recent bar
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/data series.
    * @param optInTimePeriod Number of bars in the weighting window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Weighted moving average series. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#MA
    * @see Core#DEMA
    * @see Core#TEMA
    */
   public OutRange WMA( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("WMA", startIdx, endIdx);
      int guardStart = clampedStart("WMA", startIdx, WMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WMA", "inReal", inReal, guardInLen);
      requireLength("WMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live WMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#WMA} over the same series.
    * Open with {@link Core#wmaOpen}; there is no close — the handle is
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
   public static final class WmaStream {
      Core core;
      int optInTimePeriod;
      int lookbackWin;
      int barsSinceReseed;
      double periodSum;
      double periodSub;
      double trailingValue;
      double divider;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      int winPos_j;
      int winCap_j;
      double[] win_j_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      WmaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#WMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      WmaStream( WmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lookbackWin = other.lookbackWin;
         this.barsSinceReseed = other.barsSinceReseed;
         this.periodSum = other.periodSum;
         this.periodSub = other.periodSub;
         this.trailingValue = other.trailingValue;
         this.divider = other.divider;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.winPos_j = other.winPos_j;
         this.winCap_j = other.winCap_j;
         this.win_j_inReal = other.win_j_inReal.clone();
         this.cur_outReal = other.cur_outReal;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("WMA update: BadParam", RetCode.BadParam);
         }
         core.wmaStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("WMA updateAndFill", "inReal", inReal);
         requireArgument("WMA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("WMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("WMA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.wmaStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("WMA peek: BadParam", RetCode.BadParam);
         WmaStream sp = this;
         int j = 0;
         int rw = 0;
         double tempReal = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = sp.cur_outReal;
         double periodSub = sp.periodSub;
         double periodSum = sp.periodSum;
         int ringPos_trailingIdx = sp.ringPos_trailingIdx;
         double trailingValue = sp.trailingValue;
         int winPos_j = sp.winPos_j;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         pkSlot1 = winPos_j;
         pkVal1 = inReal;
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal;
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * sp.optInTimePeriod;
         /* Re-anchor: rebuild both totals from the window itself.
          *
          * periodSum and periodSub were running totals that were never
          * recomputed, so each bar's rounding joined a residue no later bar
          * could subtract, and its size was set by the largest value the totals
          * had ever held rather than by the current window. That is the defect
          * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
          * below is the same weight-shifting identity as that family's
          * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
          * it and TA_SMA, whose output lives at its own sum's scale, does not.
          * Measured before the fix: worst range disagreement 1.41e-08 at 200000
          * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
          * closes or ~1000 with one large print. After: 1.79e-12, flat in call
          * length.
          *
          * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
          * LINEARREG family also carries an OUTLIER trigger (rebuild when the
          * departing value outweighs the window) because for a slope the
          * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
          * that position: its weights are bounded by `period` and its divider is
          * period*(period+1)/2, which dilutes the residue enough that the
          * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
          * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
          * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
          * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
          * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
          * shorter interval buys most of the accuracy for ~1.1x instead.
          *
          * The rebuild walks the window OLDEST FIRST with the weight counting UP
          * from 1 -- the priming scan's own order and weighting -- so a
          * re-anchored bar is bit-identical to the same bar computed by a call
          * that started there. That identity is what the range-stability
          * contract measures, and what test_wma.c W2/W3 assert.
          *
          * The loop start is written INLINE rather than through a `windowStart`
          * local: only that form is recognised as a rescan window, which is what
          * keeps this on the stream classifier's primary path. See
          * docs/ta_codegen_input_code.md.
          *
          * Reading the window is safe when outReal aliases inReal: the outputs
          * written so far occupy [0, outIdx-1], and the window starts at
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * sp.optInTimePeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            for( j = sp.lookbackWin; j >= 0; j -= 1 ) {
               tempReal = (((winPos_j + sp.winCap_j - j >= sp.winCap_j) ? winPos_j + sp.winCap_j - j - sp.winCap_j : winPos_j + sp.winCap_j - j) != pkSlot1) ? sp.win_j_inReal[(winPos_j + sp.winCap_j - j >= sp.winCap_j) ? winPos_j + sp.winCap_j - j - sp.winCap_j : winPos_j + sp.winCap_j - j] : pkVal1;
               periodSub += tempReal;
               periodSum += tempReal * rw;
               rw += 1;
            }
         }
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = (ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[ringPos_trailingIdx] : pkVal0;
         /* Calculate the WMA for this price bar. */
         cur_outReal = periodSum / sp.divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
         ringPos_trailingIdx = ringPos_trailingIdx + 1;
         if( ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
            ringPos_trailingIdx = 0;
         }
         winPos_j = winPos_j + 1;
         if( winPos_j >= sp.winCap_j ) {
            winPos_j = 0;
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
      public WmaStream clone() {
         return new WmaStream(this);
      }
   }
   void wmaStepImpl( WmaStream sp, double inReal )
   {
      int j = 0;
      int rw = 0;
      double tempReal = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.win_j_inReal[sp.winPos_j] = inReal;
      /* Add the current price bar to the sum
       * who are carried through the iterations.
       */
      tempReal = inReal;
      sp.periodSub += tempReal;
      sp.periodSub -= sp.trailingValue;
      sp.periodSum += tempReal * sp.optInTimePeriod;
      /* Re-anchor: rebuild both totals from the window itself.
       *
       * periodSum and periodSub were running totals that were never
       * recomputed, so each bar's rounding joined a residue no later bar
       * could subtract, and its size was set by the largest value the totals
       * had ever held rather than by the current window. That is the defect
       * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
       * below is the same weight-shifting identity as that family's
       * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
       * it and TA_SMA, whose output lives at its own sum's scale, does not.
       * Measured before the fix: worst range disagreement 1.41e-08 at 200000
       * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
       * closes or ~1000 with one large print. After: 1.79e-12, flat in call
       * length.
       *
       * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
       * LINEARREG family also carries an OUTLIER trigger (rebuild when the
       * departing value outweighs the window) because for a slope the
       * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
       * that position: its weights are bounded by `period` and its divider is
       * period*(period+1)/2, which dilutes the residue enough that the
       * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
       * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
       * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
       * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
       * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
       * shorter interval buys most of the accuracy for ~1.1x instead.
       *
       * The rebuild walks the window OLDEST FIRST with the weight counting UP
       * from 1 -- the priming scan's own order and weighting -- so a
       * re-anchored bar is bit-identical to the same bar computed by a call
       * that started there. That identity is what the range-stability
       * contract measures, and what test_wma.c W2/W3 assert.
       *
       * The loop start is written INLINE rather than through a `windowStart`
       * local: only that form is recognised as a rescan window, which is what
       * keeps this on the stream classifier's primary path. See
       * docs/ta_codegen_input_code.md.
       *
       * Reading the window is safe when outReal aliases inReal: the outputs
       * written so far occupy [0, outIdx-1], and the window starts at
       * startIdx-lookbackTotal+outIdx, which is >= outIdx.
       */
      sp.barsSinceReseed -= 1;
      if( sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 8 * sp.optInTimePeriod;
         sp.periodSub = (double)0.0;
         sp.periodSum = (double)0.0;
         rw = 1;
         for( j = sp.lookbackWin; j >= 0; j -= 1 ) {
            tempReal = sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
            sp.periodSub += tempReal;
            sp.periodSum += tempReal * rw;
            rw += 1;
         }
      }
      /* Save the trailing value for being substract at
       * the next iteration.
       * (must be saved here just in case outReal and
       *  inReal are the same buffer).
       */
      sp.trailingValue = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      /* Calculate the WMA for this price bar. */
      sp.cur_outReal = sp.periodSum / sp.divider;
      /* Prepare the periodSum for the next iteration. */
      sp.periodSum -= sp.periodSub;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
      sp.winPos_j = sp.winPos_j + 1;
      if( sp.winPos_j >= sp.winCap_j ) {
         sp.winPos_j = 0;
      }
   }
   private RetCode wmaOpenImpl( WmaStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int j = 0;
      int rw = 0;
      int lookbackWin = 0;
      int barsSinceReseed = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      double divider = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = WMA_Lookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.lookbackWin = 0;
         sp.barsSinceReseed = 0;
         sp.periodSum = 0.0;
         sp.periodSub = 0.0;
         sp.trailingValue = 0.0;
         sp.divider = 0.0;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = 0;
         sp.ring_trailingIdx_inReal = new double[1];
         sp.winPos_j = 0;
         sp.winCap_j = 1;
         sp.win_j_inReal = new double[1];
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      lookbackTotal = optInTimePeriod - 1;
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
      /* Weighted denominator 1+2+...+n = n(n+1)/2. Computed in double: the
       * int product n*(n+1) overflows int32 at n>=46341 (#142).
       */
      divider = (double)optInTimePeriod * (optInTimePeriod + 1) / 2.0;
      /* The algo used here use a very basic property of
       * multiplication/addition: (x*2) = x+x
       *
       * As an example, a 3 period weighted can be
       * interpreted in two way:
       *  (x1*1)+(x2*2)+(x3*3)
       *      OR
       *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
       *
       * When you move forward in the time serie
       * you can quickly adjust the periodSum for the
       * period by substracting:
       *   x1+x2+x3 (This is the periodSub)
       * Making the new periodSum equals to:
       *   x2+x3+x3
       *
       * You can then add the new price bar
       * which is x4+x4+x4 giving:
       *   x2+x3+x3+x4+x4+x4
       *
       * At this point one iteration is completed and you can
       * see that we are back to the step 1 of this example.
       *
       * Why making it so un-intuitive? The number of memory
       * access and floating point operations are kept to a
       * minimum with this algo.
       */
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      /* Evaluate the initial periodSum/periodSub and trailingValue. */
      lookbackWin = optInTimePeriod - 1;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx];
         inIdx += 1;
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      barsSinceReseed = 8 * optInTimePeriod;
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal[inIdx];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         /* Re-anchor: rebuild both totals from the window itself.
          *
          * periodSum and periodSub were running totals that were never
          * recomputed, so each bar's rounding joined a residue no later bar
          * could subtract, and its size was set by the largest value the totals
          * had ever held rather than by the current window. That is the defect
          * #254 fixed in the LINEARREG family, and `periodSum -= periodSub`
          * below is the same weight-shifting identity as that family's
          * `SumXY = SumXY + SumY - period*trailingValue` -- which is why WMA has
          * it and TA_SMA, whose output lives at its own sum's scale, does not.
          * Measured before the fix: worst range disagreement 1.41e-08 at 200000
          * bars against a 1e-10 tier, over the tier from ~10000 bars on ordinary
          * closes or ~1000 with one large print. After: 1.79e-12, flat in call
          * length.
          *
          * ONE TRIGGER, NOT TWO, AND THE INTERVAL IS 8*period NOT 32. The
          * LINEARREG family also carries an OUTLIER trigger (rebuild when the
          * departing value outweighs the window) because for a slope the
          * interval alone FAILS the tier outright, at 2.38e-10. WMA is not in
          * that position: its weights are bounded by `period` and its divider is
          * period*(period+1)/2, which dilutes the residue enough that the
          * interval alone holds. Swept over periods 2, 3, 4, 14, 50, 200, 1000,
          * 5000 and 20000 on 60000 bars, clean and with a 1000x print, the worst
          * is 2.2e-11 -- 4.6x inside the band, and the margin does not thin at
          * either end of the period range. Measured, the trigger bought 1.4e-11 -> 7e-12 and cost 1.17x
          * here and 1.65x in TA_HMA, whose three fused stages each pay it. The
          * shorter interval buys most of the accuracy for ~1.1x instead.
          *
          * The rebuild walks the window OLDEST FIRST with the weight counting UP
          * from 1 -- the priming scan's own order and weighting -- so a
          * re-anchored bar is bit-identical to the same bar computed by a call
          * that started there. That identity is what the range-stability
          * contract measures, and what test_wma.c W2/W3 assert.
          *
          * The loop start is written INLINE rather than through a `windowStart`
          * local: only that form is recognised as a rescan window, which is what
          * keeps this on the stream classifier's primary path. See
          * docs/ta_codegen_input_code.md.
          *
          * Reading the window is safe when outReal aliases inReal: the outputs
          * written so far occupy [0, outIdx-1], and the window starts at
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 ) {
            barsSinceReseed = 8 * optInTimePeriod;
            periodSub = (double)0.0;
            periodSum = (double)0.0;
            rw = 1;
            for( j = inIdx - lookbackWin; j <= inIdx; j += 1 ) {
               tempReal = inReal[j];
               periodSub += tempReal;
               periodSum += tempReal * rw;
               rw += 1;
            }
         }
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         /* Calculate the WMA for this price bar. */
         outReal[outIdx++ * outStride] = periodSum / divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
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
      int cap_j = (int)(lookbackWin + 1);
      if( cap_j < 1 || cap_j > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_j_inReal = new double[cap_j];
      System.arraycopy(inReal, historyLen - cap_j, capWin_j_inReal, 0, cap_j);
      sp.optInTimePeriod = optInTimePeriod;
      sp.lookbackWin = lookbackWin;
      sp.barsSinceReseed = barsSinceReseed;
      sp.periodSum = periodSum;
      sp.periodSub = periodSub;
      sp.trailingValue = trailingValue;
      sp.divider = divider;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.winPos_j = 0;
      sp.winCap_j = cap_j;
      sp.win_j_inReal = capWin_j_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* wmaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WmaStream wmaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WmaStream sp = new WmaStream(this);
      RetCode retCode = wmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind wmaOpen (composition seam). */
   WmaStream wmaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      WmaStream sp = new WmaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = wmaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("WMA open: " + retCode, retCode);
   }
   /**
    * Open a live WMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#WMA} at that bar.
    * <p>The history must hold at least {@code WMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public WmaStream wmaOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("WMA open", "inReal", inReal);
      requireHistory("WMA open", inReal.length);
      return wmaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#wmaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#WMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link WmaStream#outRange()}.
    */
   public WmaStream wmaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("WMA openAndFill", "inReal", inReal);
      requireHistory("WMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("WMA openAndFill", inReal.length, WMA_Lookback(optInTimePeriod));
      requireLength("WMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("WMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return wmaOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
