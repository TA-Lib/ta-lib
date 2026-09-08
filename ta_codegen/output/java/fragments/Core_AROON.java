/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

   /**
    * Number of leading input bars {@link Core#AROON} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AROON_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode AROON_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outAroonDown[],
                       double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
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
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode AROON_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outAroonDown[],
                       double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
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
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Aroon reports how recently the highest high and lowest low occurred within
    * a rolling window of length optInTimePeriod, as two 0-100 oscillators.
    * Indicates trend strength and direction. Up near 100 = a very recent new
    * high (strong uptrend); Down near 100 = a very recent new low. Up/Down
    * crossovers signal trend shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROON_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outAroonDown Recency of the lowest low (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @param outAroonUp Recency of the highest high (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#AROONOSC
    * @see Core#MINMAXINDEX
    * @see Core#MIN
    * @see Core#MAX
    */
   public OutRange AROON( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          double outAroonDown[],
                          double outAroonUp[] )
   {
      requireIndexRange("AROON", startIdx, endIdx);
      int guardStart = clampedStart("AROON", startIdx, AROON_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROON", "inHigh", inHigh, guardInLen);
      requireLength("AROON", "inLow", inLow, guardInLen);
      requireLength("AROON", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON", "outAroonUp", outAroonUp, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROON_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
      if( retCode != RetCode.Success ) {
         throw failure("AROON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Aroon reports how recently the highest high and lowest low occurred within
    * a rolling window of length optInTimePeriod, as two 0-100 oscillators.
    * Indicates trend strength and direction. Up near 100 = a very recent new
    * high (strong uptrend); Down near 100 = a very recent new low. Up/Down
    * crossovers signal trend shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROON_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outAroonDown Recency of the lowest low (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @param outAroonUp Recency of the highest high (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#AROONOSC
    * @see Core#MINMAXINDEX
    * @see Core#MIN
    * @see Core#MAX
    */
   public OutRange AROON( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          double outAroonDown[],
                          double outAroonUp[] )
   {
      requireIndexRange("AROON", startIdx, endIdx);
      int guardStart = clampedStart("AROON", startIdx, AROON_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROON", "inHigh", inHigh, guardInLen);
      requireLength("AROON", "inLow", inLow, guardInLen);
      requireLength("AROON", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON", "outAroonUp", outAroonUp, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROON_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
      if( retCode != RetCode.Success ) {
         throw failure("AROON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AROON stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AROON} over the same series.
    * Open with {@link Core#aroonOpen}; there is no close — the handle is
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
   public static final class AroonStream {
      private Core core;
      private int optInTimePeriod;
      private double lowest;
      private double highest;
      private double factor;
      private int trailingIdx;
      private int lowestIdx;
      private int highestIdx;
      private int i;
      private int today;
      private int xMask;
      private double[] x_inHigh;
      private double[] x_inLow;
      private double cur_outAroonDown;
      private double cur_outAroonUp;
      private int outRangeBegIdx;
      private int outRangeCount;

      private AroonStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AROON} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#MAX_INDEX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value(AroonOut)} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#MAX_INDEX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("AROON advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      private AroonStream( AroonStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.factor = other.factor;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.cur_outAroonDown = other.cur_outAroonDown;
         this.cur_outAroonUp = other.cur_outAroonUp;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(AroonOut)} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public void update( double inHigh, double inLow, AroonOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("AROON update", RetCode.OutOfRangeEndIndex);
         requireArgument("AROON update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROON update: BadParam", RetCode.BadParam);
         core.aroonStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.aroonDown = this.cur_outAroonDown;
         out.aroonUp = this.cur_outAroonUp;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, AroonOut out ) {
         requireArgument("AROON peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROON peek: BadParam", RetCode.BadParam);
         AroonStream sp = this;
         double tmp = 0.0;
         double cur_outAroonDown = 0.0;
         double cur_outAroonUp = 0.0;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inHigh;
         pkSlot1 = sp.today & sp.xMask;
         pkVal1 = inLow;
         /* Keep track of the lowestIdx */
         tmp = ((sp.today & sp.xMask) != pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
         if( lowestIdx < sp.trailingIdx ) {
            lowestIdx = sp.trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            i = lowestIdx;
            while( ++i <= sp.today ) {
               tmp = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = sp.today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = ((sp.today & sp.xMask) != pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
         if( highestIdx < sp.trailingIdx ) {
            highestIdx = sp.trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= sp.today ) {
               tmp = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = sp.today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - highestIdx));
         cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - lowestIdx));
         out.aroonDown = cur_outAroonDown;
         out.aroonUp = cur_outAroonUp;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( AroonOut out ) {
         requireArgument("AROON value", "out", out);
         out.aroonDown = this.cur_outAroonDown;
         out.aroonUp = this.cur_outAroonUp;
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
      public AroonStream clone() {
         return new AroonStream(this);
      }
   }

   /**
    * The outputs of one AROON bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class AroonOut {
      /** Recency of the lowest low (100 = it is the current bar, decaying as it ages) */
      public double aroonDown;
      /** Recency of the highest high (100 = it is the current bar, decaying as it ages) */
      public double aroonUp;
   }
   private void aroonStepImpl( AroonStream sp, double inHigh, double inLow )
   {
      double tmp = 0.0;
      sp.x_inHigh[sp.today & sp.xMask] = inHigh;
      sp.x_inLow[sp.today & sp.xMask] = inLow;
      /* Keep track of the lowestIdx */
      tmp = sp.x_inLow[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i & sp.xMask];
            if( tmp <= sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      /* Keep track of the highestIdx */
      tmp = sp.x_inHigh[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i & sp.xMask];
            if( tmp >= sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      /* Note: Do not forget that input and output buffer can be the same,
       *       so writing to the output is the last thing being done here.
       */
      sp.cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - sp.highestIdx));
      sp.cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - sp.lowestIdx));
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode aroonOpenImpl( AroonStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[], int outStride )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
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
         return RetCode.InsufficientHistory ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx * outStride] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx * outStride] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inHigh = new double[physX];
      double[] capX_inLow = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.factor = factor;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outAroonDown = outAroonDown[(outNBElement.value - 1) * outStride];
      sp.cur_outAroonUp = outAroonUp[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* aroonOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AroonStream aroonOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[] )
   {
      AroonStream sp = new AroonStream(this);
      RetCode retCode = aroonOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROON openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROON openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AROON openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind aroonOpen (composition seam). */
   AroonStream aroonOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      AroonStream sp = new AroonStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outAroonDown = new double[1];
      double[] sink_outAroonUp = new double[1];
      RetCode retCode = aroonOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outAroonDown, sink_outAroonUp, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROON open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROON open: internal error", retCode);
      }
      throw new TaLibArgumentException("AROON open: " + retCode, retCode);
   }
   /**
    * Open a live AROON stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AROON} at that bar.
    * <p>The history must hold at least {@code AROON_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AroonStream aroonOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("AROON open", "inHigh", inHigh);
      requireHistory("AROON open", inHigh.length);
      requireArgument("AROON open", "inLow", inLow);
      requireHistoryLength("AROON open", "inLow", inLow.length, inHigh.length);
      return aroonOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#aroonOpen} that also fills the output array(s) bit-identically
    * to {@link Core#AROON} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AroonStream#outRange()}.
    */
   public AroonStream aroonOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outAroonDown[], double outAroonUp[] )
   {
      requireArgument("AROON openAndFill", "inHigh", inHigh);
      requireHistory("AROON openAndFill", inHigh.length);
      requireArgument("AROON openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("AROON openAndFill", inHigh.length, AROON_Lookback(optInTimePeriod));
      requireHistoryLength("AROON openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("AROON openAndFill", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON openAndFill", "outAroonUp", outAroonUp, guardOutLen);
      if( (Object)outAroonDown == (Object)inHigh || (Object)outAroonDown == (Object)inLow || (Object)outAroonUp == (Object)inHigh || (Object)outAroonUp == (Object)inLow || (Object)outAroonDown == (Object)outAroonUp ) {
         throw new TaLibArgumentException("AROON openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return aroonOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
   }
