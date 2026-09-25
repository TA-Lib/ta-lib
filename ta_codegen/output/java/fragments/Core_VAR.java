/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  100502 JV     Speed optimization of the algorithm
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071726 MF,CC  #118 cancellation-free variance (shifted sums + reseed); fixes bug 90.
 *  082326 MF,CC  #243 reseed floor is scale-relative, not `variance < 0`.
 *  092226 MF,CC  #434 rebuild against the peak sum of squares; re-anchor a flat window.
 */

   /**
    * Number of leading input bars {@link Core#var} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int varLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode varImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInTimePeriod,
                    double optInNbDev,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double peakTotal2 = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      /* Identify the minimum number of price bar needed to calculate
       * at least one output.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not enough initial data. */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      /* Measure deviations against a shift near the window: the running sums
       * periodTotal1 = sum(inReal-shift) and periodTotal2 = sum((inReal-shift)^2)
       * stay at variance scale, so variance = periodTotal2/period - mean^2 no longer
       * subtracts two ~mean^2 quantities. Anchor the shift to the first window value
       * (also gives an exact 0 for period 1, with no division by period-1).
       */
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      /* inReal and outReal may be the same buffer: each trailing value is consumed
       * before its slot is overwritten by the output.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         /* Add the incoming value, measured against the shift. */
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
          * of the LARGEST mean squared deviation held since the last rebuild, or at
          * least every 32 windows. Measure against that peak, not the current sum:
          * the rounding the running sums carry scales with the peak, so once a
          * series settles back near the shift, or an outlier leaves the window,
          * the current sum holds nothing but that rounding. The collapse is seen
          * on the first bar whose sums carry it, and the rebuild recomputes that
          * bar.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            /* A window flat to within the rounding of its own mean leaves the
             * variance at that rounding, which would fire the trigger again on
             * every bar. Anchored on one of its own values it cannot: the variance
             * is then at least 1/(2n) of the mean square it is extracted from.
             */
            if( variance < 0.000001 * (periodTotal2 * invPeriod) ) {
               shift = inReal[i];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  tempReal = inReal[j] - shift;
                  periodTotal1 += tempReal;
                  tempReal *= tempReal;
                  periodTotal2 += tempReal;
               }
               meanValue1 = periodTotal1 * invPeriod;
               variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            }
            /* Before the re-remove below: the peak must hold the whole window. */
            peakTotal2 = periodTotal2;
            /* After the re-anchor a window with any spread sits orders above this
             * floor, so it catches only a variance that rounding left at or below
             * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
             * square-root it unconditionally (#243). A negative variance always
             * gets here: the peak is never negative, so the trigger fires on it.
             */
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            /* Re-remove the trailing value under the new shift so the carried state
             * matches the non-reseed path.
             */
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode varImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInTimePeriod,
                    double optInNbDev,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double peakTotal2 = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = (double)inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = (double)inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         tempReal = (double)inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000001 * (periodTotal2 * invPeriod) ) {
               shift = (double)inReal[i];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  tempReal = (double)inReal[j] - shift;
                  periodTotal1 += tempReal;
                  tempReal *= tempReal;
                  periodTotal2 += tempReal;
               }
               meanValue1 = periodTotal1 * invPeriod;
               variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            }
            peakTotal2 = periodTotal2;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++] = variance;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Rolling population variance of a real series over a given period. Measures
    * dispersion of values around their mean. Higher values indicate greater
    * dispersion; 0 means constant input.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/var">ta-lib.org/functions/var</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.</li>
    * <li>The deviation-count parameter is accepted but has no effect on the result.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#varLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param outReal Rolling population variance. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#stddev
    */
   public OutRange var( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        double outReal[] )
   {
      requireIndexRange("VAR", startIdx, endIdx);
      int guardStart = clampedStart("VAR", startIdx, varLookback(optInTimePeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VAR", "inReal", inReal, guardInLen);
      requireLength("VAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = varImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("VAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling population variance of a real series over a given period. Measures
    * dispersion of values around their mean. Higher values indicate greater
    * dispersion; 0 means constant input.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/var">ta-lib.org/functions/var</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.</li>
    * <li>The deviation-count parameter is accepted but has no effect on the result.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#varLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param outReal Rolling population variance. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#stddev
    */
   public OutRange var( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        double outReal[] )
   {
      requireIndexRange("VAR", startIdx, endIdx);
      int guardStart = clampedStart("VAR", startIdx, varLookback(optInTimePeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VAR", "inReal", inReal, guardInLen);
      requireLength("VAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = varImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("VAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#var} over the same series.
    * Open with {@link Core#varOpen}; there is no close — the handle is
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
   public static final class VarStream {
      private Core core;
      private int optInTimePeriod;
      private double optInNbDev;
      private double shift;
      private double periodTotal1;
      private double periodTotal2;
      private double invPeriod;
      private double peakTotal2;
      private int trailingIdx;
      private int nbInitialElementNeeded;
      private int barsSinceReseed;
      private int j;
      private int windowStart;
      private int i;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private VarStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#var} reports over the same bars: the
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
            throw failure("VAR advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private VarStream( VarStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.shift = other.shift;
         this.periodTotal1 = other.periodTotal1;
         this.periodTotal2 = other.periodTotal2;
         this.invPeriod = other.invPeriod;
         this.peakTotal2 = other.peakTotal2;
         this.trailingIdx = other.trailingIdx;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceReseed = other.barsSinceReseed;
         this.j = other.j;
         this.windowStart = other.windowStart;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
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
            throw failure("VAR update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("VAR update: BAD_PARAM", RetCode.BAD_PARAM);
         core.varStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("VAR peek: BAD_PARAM", RetCode.BAD_PARAM);
         VarStream sp = this;
         double tempReal = 0.0;
         double meanValue1 = 0.0;
         double variance = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double peakTotal2 = sp.peakTotal2;
         double periodTotal1 = sp.periodTotal1;
         double periodTotal2 = sp.periodTotal2;
         double shift = sp.shift;
         int trailingIdx = sp.trailingIdx;
         int windowStart = sp.windowStart;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.i & sp.xMask;
         pkVal0 = inReal;
         /* Add the incoming value, measured against the shift. */
         tempReal = (((sp.i & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0) - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanValue1 = periodTotal1 * sp.invPeriod;
         variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = (((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
          * of the LARGEST mean squared deviation held since the last rebuild, or at
          * least every 32 windows. Measure against that peak, not the current sum:
          * the rounding the running sums carry scales with the peak, so once a
          * series settles back near the shift, or an outlier leaves the window,
          * the current sum holds nothing but that rounding. The collapse is seen
          * on the first bar whose sums carry it, and the rebuild recomputes that
          * bar.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (peakTotal2 * sp.invPeriod) || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = sp.i - sp.nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
            }
            shift = tempReal * sp.invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               tempReal = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * sp.invPeriod;
            variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
            /* A window flat to within the rounding of its own mean leaves the
             * variance at that rounding, which would fire the trigger again on
             * every bar. Anchored on one of its own values it cannot: the variance
             * is then at least 1/(2n) of the mean square it is extracted from.
             */
            if( variance < 0.000001 * (periodTotal2 * sp.invPeriod) ) {
               shift = ((sp.i & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0;
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= sp.i; j += 1 ) {
                  tempReal = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
                  periodTotal1 += tempReal;
                  tempReal *= tempReal;
                  periodTotal2 += tempReal;
               }
               meanValue1 = periodTotal1 * sp.invPeriod;
               variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
            }
            /* Before the re-remove below: the peak must hold the whole window. */
            peakTotal2 = periodTotal2;
            /* After the re-anchor a window with any spread sits orders above this
             * floor, so it catches only a variance that rounding left at or below
             * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
             * square-root it unconditionally (#243). A negative variance always
             * gets here: the peak is never negative, so the trigger fires on it.
             */
            if( variance < 0.000000000001 * (periodTotal2 * sp.invPeriod) ) {
               variance = 0.0;
            }
            /* Re-remove the trailing value under the new shift so the carried state
             * matches the non-reseed path.
             */
            tempReal = (((windowStart & sp.xMask) != pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0) - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         cur_outReal = variance;
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
      public VarStream clone() {
         return new VarStream(this);
      }
   }
   private void varStepImpl( VarStream sp, double inReal )
   {
      double tempReal = 0.0;
      double meanValue1 = 0.0;
      double variance = 0.0;
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      /* Add the incoming value, measured against the shift. */
      tempReal = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
      sp.peakTotal2 = (sp.periodTotal2 > sp.peakTotal2) ? sp.periodTotal2 : sp.peakTotal2;
      meanValue1 = sp.periodTotal1 * sp.invPeriod;
      variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      /* Remove the trailing value (prepares the next window). */
      tempReal = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      sp.periodTotal1 -= tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 -= tempReal;
      sp.trailingIdx += 1;
      /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
       * of the LARGEST mean squared deviation held since the last rebuild, or at
       * least every 32 windows. Measure against that peak, not the current sum:
       * the rounding the running sums carry scales with the peak, so once a
       * series settles back near the shift, or an outlier leaves the window,
       * the current sum holds nothing but that rounding. The collapse is seen
       * on the first bar whose sums carry it, and the rebuild recomputes that
       * bar.
       */
      sp.barsSinceReseed -= 1;
      if( variance < 0.000001 * (sp.peakTotal2 * sp.invPeriod) || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         sp.windowStart = sp.i - sp.nbInitialElementNeeded;
         tempReal = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            tempReal += sp.x_inReal[sp.j & sp.xMask];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.periodTotal1 = 0.0;
         sp.periodTotal2 = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
            sp.periodTotal1 += tempReal;
            tempReal *= tempReal;
            sp.periodTotal2 += tempReal;
         }
         meanValue1 = sp.periodTotal1 * sp.invPeriod;
         variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
         /* A window flat to within the rounding of its own mean leaves the
          * variance at that rounding, which would fire the trigger again on
          * every bar. Anchored on one of its own values it cannot: the variance
          * is then at least 1/(2n) of the mean square it is extracted from.
          */
         if( variance < 0.000001 * (sp.periodTotal2 * sp.invPeriod) ) {
            sp.shift = sp.x_inReal[sp.i & sp.xMask];
            sp.periodTotal1 = 0.0;
            sp.periodTotal2 = 0.0;
            for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
               tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
               sp.periodTotal1 += tempReal;
               tempReal *= tempReal;
               sp.periodTotal2 += tempReal;
            }
            meanValue1 = sp.periodTotal1 * sp.invPeriod;
            variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
         }
         /* Before the re-remove below: the peak must hold the whole window. */
         sp.peakTotal2 = sp.periodTotal2;
         /* After the re-anchor a window with any spread sits orders above this
          * floor, so it catches only a variance that rounding left at or below
          * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
          * square-root it unconditionally (#243). A negative variance always
          * gets here: the peak is never negative, so the trigger fires on it.
          */
         if( variance < 0.000000000001 * (sp.periodTotal2 * sp.invPeriod) ) {
            variance = 0.0;
         }
         /* Re-remove the trailing value under the new shift so the carried state
          * matches the non-reseed path.
          */
         tempReal = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
         sp.periodTotal1 -= tempReal;
         tempReal *= tempReal;
         sp.periodTotal2 -= tempReal;
      }
      sp.cur_outReal = variance;
      sp.i += 1;
   }
   private RetCode varOpenImpl( VarStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double peakTotal2 = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Identify the minimum number of price bar needed to calculate
       * at least one output.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not enough initial data. */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      /* Measure deviations against a shift near the window: the running sums
       * periodTotal1 = sum(inReal-shift) and periodTotal2 = sum((inReal-shift)^2)
       * stay at variance scale, so variance = periodTotal2/period - mean^2 no longer
       * subtracts two ~mean^2 quantities. Anchor the shift to the first window value
       * (also gives an exact 0 for period 1, with no division by period-1).
       */
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      /* inReal and outReal may be the same buffer: each trailing value is consumed
       * before its slot is overwritten by the output.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         /* Add the incoming value, measured against the shift. */
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Rebuild with a fresh two-pass when the variance has shrunk below 1e-6
          * of the LARGEST mean squared deviation held since the last rebuild, or at
          * least every 32 windows. Measure against that peak, not the current sum:
          * the rounding the running sums carry scales with the peak, so once a
          * series settles back near the shift, or an outlier leaves the window,
          * the current sum holds nothing but that rounding. The collapse is seen
          * on the first bar whose sums carry it, and the rebuild recomputes that
          * bar.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (peakTotal2 * invPeriod) || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            /* A window flat to within the rounding of its own mean leaves the
             * variance at that rounding, which would fire the trigger again on
             * every bar. Anchored on one of its own values it cannot: the variance
             * is then at least 1/(2n) of the mean square it is extracted from.
             */
            if( variance < 0.000001 * (periodTotal2 * invPeriod) ) {
               shift = inReal[i];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  tempReal = inReal[j] - shift;
                  periodTotal1 += tempReal;
                  tempReal *= tempReal;
                  periodTotal2 += tempReal;
               }
               meanValue1 = periodTotal1 * invPeriod;
               variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            }
            /* Before the re-remove below: the peak must hold the whole window. */
            peakTotal2 = periodTotal2;
            /* After the re-anchor a window with any spread sits orders above this
             * floor, so it catches only a variance that rounding left at or below
             * 0. That keeps the output non-negative, which lets STDDEV and BBANDS
             * square-root it unconditionally (#243). A negative variance always
             * gets here: the peak is never negative, so the trigger fires on it.
             */
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            /* Re-remove the trailing value under the new shift so the carried state
             * matches the non-reseed path.
             */
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         outReal[outIdx++ * outStride] = variance;
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDev = optInNbDev;
      sp.shift = shift;
      sp.periodTotal1 = periodTotal1;
      sp.periodTotal2 = periodTotal2;
      sp.invPeriod = invPeriod;
      sp.peakTotal2 = peakTotal2;
      sp.trailingIdx = trailingIdx;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.barsSinceReseed = barsSinceReseed;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* varOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VarStream varOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VarStream sp = new VarStream(this);
      RetCode retCode = varOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("VAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("VAR openAndFill: internal error", retCode);
      }
      throw new TALibArgumentException("VAR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind varOpen (composition seam). */
   VarStream varOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      VarStream sp = new VarStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = varOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("VAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("VAR open: internal error", retCode);
      }
      throw new TALibArgumentException("VAR open: " + retCode, retCode);
   }
   /**
    * Open a live VAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#var} at that bar.
    * <p>The history must hold at least {@code varLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public VarStream varOpen( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      requireArgument("VAR open", "inReal", inReal);
      requireHistory("VAR open", inReal.length);
      return varOpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#varOpen} that also fills the output array(s) bit-identically
    * to {@link Core#var} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link VarStream#outRange()}.
    */
   public VarStream varOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, double outReal[] )
   {
      requireArgument("VAR openAndFill", "inReal", inReal);
      requireHistory("VAR openAndFill", inReal.length);
      int guardOutLen = openFillCount("VAR openAndFill", inReal.length, varLookback(optInTimePeriod, optInNbDev));
      requireLength("VAR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TALibArgumentException("VAR openAndFill: " + RetCode.BAD_PARAM, RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return varOpenAndFillInternal(inReal, 0, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
   }
