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
 */

   /**
    * Number of leading input bars {@link Core#VAR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int VAR_Lookback( int optInTimePeriod, double optInNbDev )
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
   RetCode VAR_Impl( int startIdx,
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
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
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
         return RetCode.Success ;
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
      do {
         /* Add the incoming value, measured against the shift. */
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Re-anchor the shift and rebuild the running sums with a fresh two-pass
          * when the shift is stale enough that the subtraction loses digits - i.e.
          * the variance has shrunk below 1e-6 of the mean squared deviation it is
          * extracted from (that ratio bounds the cancellation error to ~eps/1e-6 ~
          * 2e-10, so partial cancellation, not just total collapse, is caught); OR
          * when the value just removed sat so far from the shift that its squared term
          * (tempReal) dwarfs the surviving sum (a large outlier passing through the
          * window buries the small terms below its ulp, and the residual left when it
          * leaves is cancellation garbage); OR at least every 32 windows so a slow
          * drift stays bounded regardless of the series length. The strict `<` also
          * leaves an exactly-constant window (variance 0, scale 0) alone instead of
          * reseeding it every bar. Guarantees a non-negative output.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
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
            /* A variance is non-negative by definition, but this one is extracted
             * as a difference of two nearly equal quantities, so its SIGN is not
             * guaranteed: on a window sitting entirely inside a flat stretch every
             * deviation is the same rounding residual and the subtraction is pure
             * cancellation, landing either side of zero. Enforce the invariant.
             *
             * Clamping HERE and not at the output write is what keeps this off the
             * per-bar path, and it is sufficient because a negative variance always
             * reseeds on the same bar - the guard above covers all three cases:
             * periodTotal2 > 0 makes its first disjunct `negative < positive`;
             * periodTotal2 < 0 makes the second disjunct's right side negative,
             * which the squared tempReal always exceeds; periodTotal2 == 0 reduces
             * the first to `variance < 0`. CHANGING THAT GUARD MEANS RE-CHECKING
             * THIS - the alternative is an unconditional clamp at the output write,
             * which needs no such argument but does cost ~3%.
             */
            if( variance < 0.0 ) {
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
      return RetCode.Success ;
   }
   RetCode VAR_Impl( int startIdx,
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
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
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
      do {
         tempReal = (double)inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
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
            if( variance < 0.0 ) {
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
      return RetCode.Success ;
   }
   /**
    * Rolling population variance of a real series over a given period. Measures
    * dispersion of values around their mean. Higher values indicate greater
    * dispersion; 0 means constant input.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\mathrm{VAR} = \frac{1}{n}\sum x_i^2 - \left(\frac{1}{n}\sum x_i\right)^2$, over the last $n$ = optInTimePeriod values (population, divides by $n$).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.</li>
    * <li>The deviation-count parameter is accepted but has no effect on the result.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VAR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@code -4e37} selects the default).
    * @param outReal Rolling population variance. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#STDDEV
    */
   public OutRange VAR( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        double outReal[] )
   {
      requireIndexRange("VAR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, VAR_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VAR", "inReal", inReal, guardInLen);
      requireLength("VAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VAR_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling population variance of a real series over a given period. Measures
    * dispersion of values around their mean. Higher values indicate greater
    * dispersion; 0 means constant input.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\mathrm{VAR} = \frac{1}{n}\sum x_i^2 - \left(\frac{1}{n}\sum x_i\right)^2$, over the last $n$ = optInTimePeriod values (population, divides by $n$).
    * }</pre>
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
    * valid range shorter than {@link Core#VAR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInTimePeriod Window length for the variance (default 5; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Deviation count accepted by the API but never used in
    *        the computation (default 1; {@code -4e37} selects the default).
    * @param outReal Rolling population variance. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#STDDEV
    */
   public OutRange VAR( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        double outReal[] )
   {
      requireIndexRange("VAR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, VAR_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VAR", "inReal", inReal, guardInLen);
      requireLength("VAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VAR_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#VAR} over the same series.
    * Open with {@link Core#VAR_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class VAR_Stream {
      Core core;
      int optInTimePeriod;
      double optInNbDev;
      double shift;
      double periodTotal1;
      double periodTotal2;
      double meanValue1;
      double variance;
      double invPeriod;
      int j;
      int trailingIdx;
      int windowStart;
      int nbInitialElementNeeded;
      int barsSinceReseed;
      int i;
      int xMask;
      double[] x_inReal;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      VAR_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#VAR_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      VAR_Stream( VAR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.shift = other.shift;
         this.periodTotal1 = other.periodTotal1;
         this.periodTotal2 = other.periodTotal2;
         this.meanValue1 = other.meanValue1;
         this.variance = other.variance;
         this.invPeriod = other.invPeriod;
         this.j = other.j;
         this.trailingIdx = other.trailingIdx;
         this.windowStart = other.windowStart;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceReseed = other.barsSinceReseed;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( VAR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.shift = other.shift;
         this.periodTotal1 = other.periodTotal1;
         this.periodTotal2 = other.periodTotal2;
         this.meanValue1 = other.meanValue1;
         this.variance = other.variance;
         this.invPeriod = other.invPeriod;
         this.j = other.j;
         this.trailingIdx = other.trailingIdx;
         this.windowStart = other.windowStart;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceReseed = other.barsSinceReseed;
         this.i = other.i;
         this.xMask = other.xMask;
         if( this.x_inReal != null && this.x_inReal.length == other.x_inReal.length ) {
            System.arraycopy( other.x_inReal, 0, this.x_inReal, 0, other.x_inReal.length );
         } else {
            this.x_inReal = other.x_inReal.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("VAR update: BadParam", RetCode.BadParam);
         core.VAR_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("VAR peek: BadParam", RetCode.BadParam);
         VAR_Stream scratch = new VAR_Stream(this);
         core.VAR_StreamStep(scratch, inReal);
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
      public VAR_Stream copy() {
         return new VAR_Stream(this);
      }
   }
   void VAR_StreamStep( VAR_Stream sp, double inReal )
   {
      double tempReal = 0.0;
      if( sp.i >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.i -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
         sp.windowStart -= rebaseShift;
      }
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      /* Add the incoming value, measured against the shift. */
      tempReal = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
      sp.meanValue1 = sp.periodTotal1 * sp.invPeriod;
      sp.variance = sp.periodTotal2 * sp.invPeriod - sp.meanValue1 * sp.meanValue1;
      /* Remove the trailing value (prepares the next window). */
      tempReal = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      sp.periodTotal1 -= tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 -= tempReal;
      sp.trailingIdx += 1;
      /* Re-anchor the shift and rebuild the running sums with a fresh two-pass
       * when the shift is stale enough that the subtraction loses digits - i.e.
       * the variance has shrunk below 1e-6 of the mean squared deviation it is
       * extracted from (that ratio bounds the cancellation error to ~eps/1e-6 ~
       * 2e-10, so partial cancellation, not just total collapse, is caught); OR
       * when the value just removed sat so far from the shift that its squared term
       * (tempReal) dwarfs the surviving sum (a large outlier passing through the
       * window buries the small terms below its ulp, and the residual left when it
       * leaves is cancellation garbage); OR at least every 32 windows so a slow
       * drift stays bounded regardless of the series length. The strict `<` also
       * leaves an exactly-constant window (variance 0, scale 0) alone instead of
       * reseeding it every bar. Guarantees a non-negative output.
       */
      sp.barsSinceReseed -= 1;
      if( sp.variance < 0.000001 * (sp.periodTotal2 * sp.invPeriod) || tempReal > 1000000.0 * sp.periodTotal2 || sp.barsSinceReseed <= 0 ) {
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
         sp.meanValue1 = sp.periodTotal1 * sp.invPeriod;
         sp.variance = sp.periodTotal2 * sp.invPeriod - sp.meanValue1 * sp.meanValue1;
         /* A variance is non-negative by definition, but this one is extracted
          * as a difference of two nearly equal quantities, so its SIGN is not
          * guaranteed: on a window sitting entirely inside a flat stretch every
          * deviation is the same rounding residual and the subtraction is pure
          * cancellation, landing either side of zero. Enforce the invariant.
          *
          * Clamping HERE and not at the output write is what keeps this off the
          * per-bar path, and it is sufficient because a negative variance always
          * reseeds on the same bar - the guard above covers all three cases:
          * periodTotal2 > 0 makes its first disjunct `negative < positive`;
          * periodTotal2 < 0 makes the second disjunct's right side negative,
          * which the squared tempReal always exceeds; periodTotal2 == 0 reduces
          * the first to `variance < 0`. CHANGING THAT GUARD MEANS RE-CHECKING
          * THIS - the alternative is an unconditional clamp at the output write,
          * which needs no such argument but does cost ~3%.
          */
         if( sp.variance < 0.0 ) {
            sp.variance = 0.0;
         }
         /* Re-remove the trailing value under the new shift so the carried state
          * matches the non-reseed path.
          */
         tempReal = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
         sp.periodTotal1 -= tempReal;
         tempReal *= tempReal;
         sp.periodTotal2 -= tempReal;
      }
      sp.cur_outReal = sp.variance;
      sp.i += 1;
   }
   private RetCode VAR_OpenPass( VAR_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
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
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
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
         return RetCode.InsufficientHistory ;
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
      do {
         /* Add the incoming value, measured against the shift. */
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         /* Remove the trailing value (prepares the next window). */
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         /* Re-anchor the shift and rebuild the running sums with a fresh two-pass
          * when the shift is stale enough that the subtraction loses digits - i.e.
          * the variance has shrunk below 1e-6 of the mean squared deviation it is
          * extracted from (that ratio bounds the cancellation error to ~eps/1e-6 ~
          * 2e-10, so partial cancellation, not just total collapse, is caught); OR
          * when the value just removed sat so far from the shift that its squared term
          * (tempReal) dwarfs the surviving sum (a large outlier passing through the
          * window buries the small terms below its ulp, and the residual left when it
          * leaves is cancellation garbage); OR at least every 32 windows so a slow
          * drift stays bounded regardless of the series length. The strict `<` also
          * leaves an exactly-constant window (variance 0, scale 0) alone instead of
          * reseeding it every bar. Guarantees a non-negative output.
          */
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
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
            /* A variance is non-negative by definition, but this one is extracted
             * as a difference of two nearly equal quantities, so its SIGN is not
             * guaranteed: on a window sitting entirely inside a flat stretch every
             * deviation is the same rounding residual and the subtraction is pure
             * cancellation, landing either side of zero. Enforce the invariant.
             *
             * Clamping HERE and not at the output write is what keeps this off the
             * per-bar path, and it is sufficient because a negative variance always
             * reseeds on the same bar - the guard above covers all three cases:
             * periodTotal2 > 0 makes its first disjunct `negative < positive`;
             * periodTotal2 < 0 makes the second disjunct's right side negative,
             * which the squared tempReal always exceeds; periodTotal2 == 0 reduces
             * the first to `variance < 0`. CHANGING THAT GUARD MEANS RE-CHECKING
             * THIS - the alternative is an unconditional clamp at the output write,
             * which needs no such argument but does cost ~3%.
             */
            if( variance < 0.0 ) {
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
         return RetCode.InternalError;
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
      sp.meanValue1 = meanValue1;
      sp.variance = variance;
      sp.invPeriod = invPeriod;
      sp.j = j;
      sp.trailingIdx = trailingIdx;
      sp.windowStart = windowStart;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.barsSinceReseed = barsSinceReseed;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode VAR_OpenImpl( VAR_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return VAR_OpenPass( sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode VAR_OpenAndFillImpl( VAR_Stream sp, double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return VAR_OpenPass( sp, inReal, 0, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode VAR_OpenAndFillInternalImpl( VAR_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return VAR_OpenPass(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
   }
   /* VAR_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VAR_Stream VAR_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VAR_Stream sp = new VAR_Stream(this);
      RetCode retCode = VAR_OpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VAR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VAR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind VAR_Open (composition seam). */
   VAR_Stream VAR_OpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      VAR_Stream sp = new VAR_Stream(this);
      RetCode retCode = VAR_OpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VAR open: internal error", retCode);
      }
      throw new TaLibArgumentException("VAR open: " + retCode, retCode);
   }
   /**
    * Open a live VAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#VAR} at that bar.
    * <p>The history must hold at least {@code VAR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public VAR_Stream VAR_Open( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      return VAR_OpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#VAR_Open} that also fills the output array(s) bit-identically
    * to {@link Core#VAR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link VAR_Stream#fillRange()}.
    */
   public VAR_Stream VAR_OpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, double outReal[] )
   {
      VAR_Stream sp = new VAR_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VAR_OpenAndFillImpl(sp, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VAR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VAR openAndFill: " + retCode, retCode);
   }
