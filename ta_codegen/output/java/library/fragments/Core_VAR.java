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

   public int varianceLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode variance( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
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
   public RetCode varianceUnguarded( int startIdx,
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
      shift = inReal[trailingIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tempReal = inReal[i] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
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
            tempReal = inReal[windowStart] - shift;
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
   public RetCode variance( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
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
   public RetCode varianceUnguarded( int startIdx,
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
/**** Streaming API *****/

   /**
    * A live VAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#variance} over the same series.
    * Open with {@link Core#varianceOpen}; there is no close — the handle is
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
   public static final class VarianceStream {
      final Core core;
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
      int xCap;
      double[] x_inReal;
      double cur_outReal;

      VarianceStream( Core core ) { this.core = core; }

      VarianceStream( VarianceStream other ) {
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
         this.xCap = other.xCap;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.varianceStreamStep(this, inReal);
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
         VarianceStream scratch = new VarianceStream(this);
         core.varianceStreamStep(scratch, inReal);
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
      public VarianceStream copy() {
         return new VarianceStream(this);
      }
   }
   void varianceStreamStep( VarianceStream sp, double inReal )
   {
      double tempReal = 0.0;
      if( sp.i >= 1073741824 ) {
         int rebaseShift = (sp.trailingIdx / sp.xCap) * sp.xCap;
         sp.i -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
         sp.windowStart -= rebaseShift;
      }
      sp.x_inReal[sp.i % sp.xCap] = inReal;
      /* Add the incoming value, measured against the shift. */
      tempReal = sp.x_inReal[sp.i % sp.xCap] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
      sp.meanValue1 = sp.periodTotal1 * sp.invPeriod;
      sp.variance = sp.periodTotal2 * sp.invPeriod - sp.meanValue1 * sp.meanValue1;
      /* Remove the trailing value (prepares the next window). */
      tempReal = sp.x_inReal[sp.trailingIdx % sp.xCap] - sp.shift;
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
            tempReal += sp.x_inReal[sp.j % sp.xCap];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.periodTotal1 = 0.0;
         sp.periodTotal2 = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            tempReal = sp.x_inReal[sp.j % sp.xCap] - sp.shift;
            sp.periodTotal1 += tempReal;
            tempReal *= tempReal;
            sp.periodTotal2 += tempReal;
         }
         sp.meanValue1 = sp.periodTotal1 * sp.invPeriod;
         sp.variance = sp.periodTotal2 * sp.invPeriod - sp.meanValue1 * sp.meanValue1;
         /* Re-remove the trailing value under the new shift so the carried state
          * matches the non-reseed path.
          */
         tempReal = sp.x_inReal[sp.windowStart % sp.xCap] - sp.shift;
         sp.periodTotal1 -= tempReal;
         tempReal *= tempReal;
         sp.periodTotal2 -= tempReal;
      }
      sp.cur_outReal = sp.variance;
      sp.i += 1;
   }
   private RetCode varianceOpenBody( VarianceStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
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
         return RetCode.OutOfRangeEndIndex ;
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
            /* Re-remove the trailing value under the new shift so the carried state
             * matches the non-reseed path.
             */
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         lastValue_outReal = variance;
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
      double[] capX_inReal = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ % capX] = inReal[fillJ];
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
      sp.xCap = capX;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode varianceOpenAndFillBody( VarianceStream sp, double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      if( (Object)outReal == (Object)inReal ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capX_inReal = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ % capX] = inReal[fillJ];
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
      sp.xCap = capX;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind varianceOpen (composition seam). */
   VarianceStream varianceOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      VarianceStream sp = new VarianceStream(this);
      RetCode retCode = varianceOpenBody(sp, inReal, startIdx, optInTimePeriod, optInNbDev);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_VAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_VAR open: internal error");
      }
      throw new IllegalArgumentException("TA_VAR open: " + retCode);
   }
   /**
    * Open a live VAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#variance} at that bar.
    * <p>The history must hold at least {@code varianceLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public VarianceStream varianceOpen( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      return varianceOpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#varianceOpen} that also fills the output array(s) bit-identically
    * to {@link Core#variance} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public VarianceStream varianceOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VarianceStream sp = new VarianceStream(this);
      RetCode retCode = varianceOpenAndFillBody(sp, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_VAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_VAR openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_VAR openAndFill: " + retCode);
   }
