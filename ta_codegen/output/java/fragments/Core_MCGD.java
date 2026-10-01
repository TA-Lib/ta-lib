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
 *  092926 MF,CC  First version (issue #471).
 */

   /**
    * Number of leading input bars {@link Core#mcgd} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod The N of the step's denominator (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int mcgdLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 + this.unstablePeriod[FuncUnstId.MCGD.ordinal()] ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#mcgd}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod The N of the step's denominator (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int mcgdDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( mcgdLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode mcgdImpl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbMCGD = 0;
      double prevMD = 0;
      double tempMD = 0;
      double tempReal = 0;
      double ratio = 0;
      double period = 0;
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
      lookbackTotal = mcgdLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      period = (double)optInTimePeriod;
      /* Keep the denominator free of pow(): a transcendental would break the
       * four languages' bit-identity.
       *
       * A step whose value is not finite (a zero price, a zero line meeting a
       * zero price, or a price so small against the line that the step
       * overflows) holds the previous value, since every later bar reads it.
       */
      today = startIdx - lookbackTotal;
      prevMD = inReal[today];
      today += 1;
      i = lookbackTotal;
      while( i != 0 ) {
         tempReal = inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         today += 1;
         i -= 1;
      }
      outIdx = 1;
      outReal[0] = prevMD;
      nbMCGD = endIdx - startIdx + 1;
      while( --nbMCGD != 0 ) {
         tempReal = inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         outReal[outIdx++] = prevMD;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode mcgdImpl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbMCGD = 0;
      double prevMD = 0;
      double tempMD = 0;
      double tempReal = 0;
      double ratio = 0;
      double period = 0;
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
      lookbackTotal = mcgdLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      period = (double)optInTimePeriod;
      today = startIdx - lookbackTotal;
      prevMD = (double)inReal[today];
      today += 1;
      i = lookbackTotal;
      while( i != 0 ) {
         tempReal = (double)inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         today += 1;
         i -= 1;
      }
      outIdx = 1;
      outReal[0] = prevMD;
      nbMCGD = endIdx - startIdx + 1;
      while( --nbMCGD != 0 ) {
         tempReal = (double)inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         outReal[outIdx++] = prevMD;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed
    * adjusts to the market. Each bar closes a fraction
    * {@code 1 / (N * (x/MD)^4)} of the gap to the price: less than
    * {@code RMA}'s {@code 1/N} while the price is above the line, more while it
    * is below, so the line tracks falling prices faster than rising ones.
    * McGinley suggests a period of about 60% of the simple moving average being
    * emulated: a Dynamic of 12 to follow a 20-bar average.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/mcgd">ta-lib.org/functions/mcgd</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Where the price is 0, or so small against the line that the step overflows, the step is undefined and the line keeps its previous value. A line at 0 stays at 0.</li>
    * <li>Being recursive, an output depends on how much history precedes it. Close to the price the seed's influence decays by a factor of {@code 1 - 1/N} per bar, as in {@code RMA}; the unstable period is how much of the warm-up to discard.</li>
    * <li>The line is scale-equivariant but meant for positive prices: a series crossing zero sends it off to meaningless values, and a single bar far enough below the line can take the line to 0 or below it.</li>
    * <li>Some implementations seed with the simple average of the first N bars. They agree with this one only once the seed's influence has decayed.</li>
    * <li>Some implementations write the step's denominator as {@code 0.6 * P * (x / MD)^4}. That is this function at a period of {@code 0.6 * P}, when that is an integer.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#mcgdLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod The N of the step's denominator (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal McGinley Dynamic line. Must hold at least
    *        {@code endIdx - max(startIdx, mcgdLookback(...)) + 1} values, the count
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
    * @see Core#rma
    * @see Core#ema
    * @see Core#kama
    * @see Core#sma
    */
   public OutRange mcgd( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("MCGD", startIdx, endIdx);
      int guardStart = clampedStart("MCGD", startIdx, mcgdLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MCGD", "inReal", inReal, guardInLen);
      requireLength("MCGD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = mcgdImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MCGD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed
    * adjusts to the market. Each bar closes a fraction
    * {@code 1 / (N * (x/MD)^4)} of the gap to the price: less than
    * {@code RMA}'s {@code 1/N} while the price is above the line, more while it
    * is below, so the line tracks falling prices faster than rising ones.
    * McGinley suggests a period of about 60% of the simple moving average being
    * emulated: a Dynamic of 12 to follow a 20-bar average.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/mcgd">ta-lib.org/functions/mcgd</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Where the price is 0, or so small against the line that the step overflows, the step is undefined and the line keeps its previous value. A line at 0 stays at 0.</li>
    * <li>Being recursive, an output depends on how much history precedes it. Close to the price the seed's influence decays by a factor of {@code 1 - 1/N} per bar, as in {@code RMA}; the unstable period is how much of the warm-up to discard.</li>
    * <li>The line is scale-equivariant but meant for positive prices: a series crossing zero sends it off to meaningless values, and a single bar far enough below the line can take the line to 0 or below it.</li>
    * <li>Some implementations seed with the simple average of the first N bars. They agree with this one only once the seed's influence has decayed.</li>
    * <li>Some implementations write the step's denominator as {@code 0.6 * P * (x / MD)^4}. That is this function at a period of {@code 0.6 * P}, when that is an integer.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#mcgdLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod The N of the step's denominator (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal McGinley Dynamic line. Must hold at least
    *        {@code endIdx - max(startIdx, mcgdLookback(...)) + 1} values, the count
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
    * @see Core#rma
    * @see Core#ema
    * @see Core#kama
    * @see Core#sma
    */
   public OutRange mcgd( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("MCGD", startIdx, endIdx);
      int guardStart = clampedStart("MCGD", startIdx, mcgdLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MCGD", "inReal", inReal, guardInLen);
      requireLength("MCGD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = mcgdImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MCGD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MCGD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#mcgd} over the same series.
    * Open with {@link Core#mcgdOpen}; there is no close — the handle is
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
   public static final class McgdStream {
      private Core core;
      private int optInTimePeriod;
      private double prevMD;
      private double period;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private McgdStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#mcgd} reports over the same bars: the
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
            throw failure("MCGD advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private McgdStream( McgdStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevMD = other.prevMD;
         this.period = other.period;
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
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("MCGD update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MCGD update", "inReal");
         core.mcgdStepImpl(this, inReal);
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MCGD peek", "inReal");
         McgdStream sp = this;
         double tempMD = 0.0;
         double tempReal = 0.0;
         double ratio = 0.0;
         double cur_outReal = 0.0;
         double prevMD = sp.prevMD;
         tempReal = inReal;
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (sp.period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         cur_outReal = prevMD;
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
      public McgdStream clone() {
         return new McgdStream(this);
      }
   }
   private void mcgdStepImpl( McgdStream sp, double inReal )
   {
      double tempMD = 0.0;
      double tempReal = 0.0;
      double ratio = 0.0;
      tempReal = inReal;
      ratio = tempReal / sp.prevMD;
      tempMD = sp.prevMD + (tempReal - sp.prevMD) / (sp.period * (ratio * ratio * (ratio * ratio)));
      if( (Double.isFinite(tempMD)) ) {
         sp.prevMD = tempMD;
      }
      sp.cur_outReal = sp.prevMD;
   }
   private RetCode mcgdOpenImpl( McgdStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbMCGD = 0;
      double prevMD = 0;
      double tempMD = 0;
      double tempReal = 0;
      double ratio = 0;
      double period = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
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
      lookbackTotal = mcgdLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      period = (double)optInTimePeriod;
      /* Keep the denominator free of pow(): a transcendental would break the
       * four languages' bit-identity.
       *
       * A step whose value is not finite (a zero price, a zero line meeting a
       * zero price, or a price so small against the line that the step
       * overflows) holds the previous value, since every later bar reads it.
       */
      today = startIdx - lookbackTotal;
      prevMD = inReal[today];
      today += 1;
      i = lookbackTotal;
      while( i != 0 ) {
         tempReal = inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         today += 1;
         i -= 1;
      }
      outIdx = 1;
      outReal[0 * outStride] = prevMD;
      nbMCGD = endIdx - startIdx + 1;
      while( --nbMCGD != 0 ) {
         tempReal = inReal[today];
         ratio = tempReal / prevMD;
         tempMD = prevMD + (tempReal - prevMD) / (period * (ratio * ratio * (ratio * ratio)));
         if( (Double.isFinite(tempMD)) ) {
            prevMD = tempMD;
         }
         outReal[outIdx++ * outStride] = prevMD;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevMD = prevMD;
      sp.period = period;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* mcgdOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   McgdStream mcgdOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      McgdStream sp = new McgdStream(this);
      RetCode retCode = mcgdOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MCGD openAndFill", inReal.length, startIdx, mcgdLookback(optInTimePeriod));
      }
      throw streamFailure("MCGD openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind mcgdOpen (composition seam). */
   McgdStream mcgdOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      McgdStream sp = new McgdStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = mcgdOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MCGD open", inReal.length, startIdx, mcgdLookback(optInTimePeriod));
      }
      throw streamFailure("MCGD open", retCode);
   }
   /**
    * Open a live MCGD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#mcgd} at that bar.
    * <p>The history must hold at least {@code mcgdLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public McgdStream mcgdOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MCGD open", "inReal", inReal);
      requireHistory("MCGD open", inReal.length);
      return mcgdOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#mcgdOpen} that also fills the output array(s) bit-identically
    * to {@link Core#mcgd} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link McgdStream#outRange()}.
    */
   public McgdStream mcgdOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("MCGD openAndFill", "inReal", inReal);
      requireHistory("MCGD openAndFill", inReal.length);
      int guardOutLen = openFillCount("MCGD openAndFill", inReal.length, mcgdLookback(optInTimePeriod));
      requireLength("MCGD openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("MCGD openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return mcgdOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
