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
 *  110126 MF,CC  First version (issue #569).
 */

   /**
    * Number of leading input bars {@link Core#skew} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 3..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int skewLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 3 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#skew}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 3..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int skewDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( skewLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode skewImpl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment3 = 0;
      double dPeriod = 0;
      double coef = 0;
      double invPeriod = 0;
      double skewness = 0;
      double peak2 = 0;
      double denominator = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 3 || optInTimePeriod > 100000 ) {
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
      /* G1 = n*sqrt(n-1)/(n-2) * M3 / M2^(3/2), over the central sums M2 and
       * M3. The n-2 is why the range starts at 3, and the n-1 lookback hands the
       * estimator a full window at the first emitted bar.
       */
      dPeriod = (double)optInTimePeriod;
      coef = dPeriod * Math.sqrt(dPeriod - 1.0) / (dPeriod - 2.0);
      invPeriod = 1.0 / dPeriod;
      /* Deviations are measured against a shift near the window, and the
       * central sums are recovered from them below. The periodic rebuild bounds
       * the rounding the slide accumulates, and is the only way out of a NaN or
       * Inf input, on which every trigger comparison is false.
       */
      reseedPeriod = 32 * optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      peak2 = total2;
      do {
         dev = inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         if( total2 > peak2 ) {
            peak2 = total2;
         }
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
         /* Each bar rounds the cubic sum by at most eps times the peak second
          * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
          * 1000x before the rebuild at 1% of the peak.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peak2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            /* The left-to-right sum can miss the mean by more than the window's
             * spread, and the trigger would then fire again on every bar. Adding
             * the measured miss back lands within about an ulp of the mean, where
             * a flat window's deviations are exactly 0. Keep this threshold the
             * trigger's.
             */
            if( moment2 < 0.01 * total2 ) {
               shift += residue;
               total1 = 0.0;
               total2 = 0.0;
               total3 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  dev = inReal[j] - shift;
                  dev2 = dev * dev;
                  total1 += dev;
                  total2 += dev2;
                  total3 += dev2 * dev;
               }
               residue = total1 * invPeriod;
               residueSq = residue * residue;
               moment2 = total2 - dPeriod * residueSq;
               moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            }
            peak2 = total2;
         }
         /* A window of equal values is symmetric, and reaches here with every
          * deviation exactly 0. The bound is the smallest normal double, not a
          * scale: a denominator under it comes with cubed deviations that have
          * lost their precision in the subnormals.
          */
         denominator = moment2 * Math.sqrt(moment2);
         if( denominator < 2.2250738585072014e-308 ) {
            skewness = 0.0;
         } else {
            skewness = coef * moment3 / denominator;
         }
         /* inReal and outReal may be the same buffer: read the trailing value
          * before the output write.
          */
         dev = inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         trailingIdx += 1;
         outReal[outIdx++] = skewness;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode skewImpl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment3 = 0;
      double dPeriod = 0;
      double coef = 0;
      double invPeriod = 0;
      double skewness = 0;
      double peak2 = 0;
      double denominator = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 3 || optInTimePeriod > 100000 ) {
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
      dPeriod = (double)optInTimePeriod;
      coef = dPeriod * Math.sqrt(dPeriod - 1.0) / (dPeriod - 2.0);
      invPeriod = 1.0 / dPeriod;
      reseedPeriod = 32 * optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = (double)inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = (double)inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      peak2 = total2;
      do {
         dev = (double)inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         if( total2 > peak2 ) {
            peak2 = total2;
         }
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peak2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = (double)inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            if( moment2 < 0.01 * total2 ) {
               shift += residue;
               total1 = 0.0;
               total2 = 0.0;
               total3 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  dev = (double)inReal[j] - shift;
                  dev2 = dev * dev;
                  total1 += dev;
                  total2 += dev2;
                  total3 += dev2 * dev;
               }
               residue = total1 * invPeriod;
               residueSq = residue * residue;
               moment2 = total2 - dPeriod * residueSq;
               moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            }
            peak2 = total2;
         }
         denominator = moment2 * Math.sqrt(moment2);
         if( denominator < 2.2250738585072014e-308 ) {
            skewness = 0.0;
         } else {
            skewness = coef * moment3 / denominator;
         }
         dev = (double)inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         trailingIdx += 1;
         outReal[outIdx++] = skewness;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * An estimate, from the trailing window, of the skewness of the distribution
    * its values come from: the third standardised moment. An asymmetry measure:
    * above 0 the distribution has a longer tail above its mean, below 0 a
    * longer tail below it, and a symmetric window reads 0. The companion to <a
    * href="https://ta-lib.org/functions/kurtosis">{@code KURTOSIS}</a>, <a
    * href="https://ta-lib.org/functions/var">{@code VAR}</a> and <a
    * href="https://ta-lib.org/functions/stddev">{@code STDDEV}</a> in the same
    * group.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/skew">ta-lib.org/functions/skew</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is {@code G1}, the form Excel {@code SKEW}, {@code scipy.stats.skew(bias=False)} and R {@code e1071::skewness(type = 2)} compute. Two other estimators are in circulation, each a constant multiple of {@code g1} rather than a rounding convention: {@code g1} itself ({@code scipy.stats.skew(bias=True)}) and {@code b1 = g1 · ((n−1)/n)^(3/2)} ({@code type = 3}).</li>
    * <li>One reading apart from {@code n−1} equal ones gives exactly {@code √n}, above the rest when the odd one is above and {@code −√n} when it is below. That is the largest magnitude a window can reach, so a window dominated by one outlier is legitimately far from 0.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#skewLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 3..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Skewness of the trailing window, or 0 where the window has
    *        no spread. Must hold at least
    *        {@code endIdx - max(startIdx, skewLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#kurtosis
    * @see Core#var
    * @see Core#stddev
    */
   public OutRange skew( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("SKEW", startIdx, endIdx);
      int guardStart = clampedStart("SKEW", startIdx, skewLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SKEW", "inReal", inReal, guardInLen);
      requireLength("SKEW", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = skewImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SKEW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * An estimate, from the trailing window, of the skewness of the distribution
    * its values come from: the third standardised moment. An asymmetry measure:
    * above 0 the distribution has a longer tail above its mean, below 0 a
    * longer tail below it, and a symmetric window reads 0. The companion to <a
    * href="https://ta-lib.org/functions/kurtosis">{@code KURTOSIS}</a>, <a
    * href="https://ta-lib.org/functions/var">{@code VAR}</a> and <a
    * href="https://ta-lib.org/functions/stddev">{@code STDDEV}</a> in the same
    * group.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/skew">ta-lib.org/functions/skew</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is {@code G1}, the form Excel {@code SKEW}, {@code scipy.stats.skew(bias=False)} and R {@code e1071::skewness(type = 2)} compute. Two other estimators are in circulation, each a constant multiple of {@code g1} rather than a rounding convention: {@code g1} itself ({@code scipy.stats.skew(bias=True)}) and {@code b1 = g1 · ((n−1)/n)^(3/2)} ({@code type = 3}).</li>
    * <li>One reading apart from {@code n−1} equal ones gives exactly {@code √n}, above the rest when the odd one is above and {@code −√n} when it is below. That is the largest magnitude a window can reach, so a window dominated by one outlier is legitimately far from 0.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#skewLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 3..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Skewness of the trailing window, or 0 where the window has
    *        no spread. Must hold at least
    *        {@code endIdx - max(startIdx, skewLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#kurtosis
    * @see Core#var
    * @see Core#stddev
    */
   public OutRange skew( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("SKEW", startIdx, endIdx);
      int guardStart = clampedStart("SKEW", startIdx, skewLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SKEW", "inReal", inReal, guardInLen);
      requireLength("SKEW", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = skewImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SKEW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SKEW stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#skew} over the same series.
    * Open with {@link Core#skewOpen}; there is no close — the handle is
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
   public static final class SkewStream {
      private Core core;
      private int optInTimePeriod;
      private double shift;
      private double total1;
      private double total2;
      private double total3;
      private double dPeriod;
      private double coef;
      private double invPeriod;
      private double peak2;
      private int trailingIdx;
      private int nbInitialElementNeeded;
      private int barsSinceReseed;
      private int reseedPeriod;
      private int j;
      private int i;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SkewStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#skew} reports over the same bars: the
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
            throw failure("SKEW advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SkewStream( SkewStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.shift = other.shift;
         this.total1 = other.total1;
         this.total2 = other.total2;
         this.total3 = other.total3;
         this.dPeriod = other.dPeriod;
         this.coef = other.coef;
         this.invPeriod = other.invPeriod;
         this.peak2 = other.peak2;
         this.trailingIdx = other.trailingIdx;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceReseed = other.barsSinceReseed;
         this.reseedPeriod = other.reseedPeriod;
         this.j = other.j;
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
            throw failure("SKEW update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SKEW update", "inReal");
         core.skewStepImpl(this, inReal);
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
            throw nonFiniteBar("SKEW peek", "inReal");
         SkewStream sp = this;
         double tempReal = 0.0;
         double dev = 0.0;
         double dev2 = 0.0;
         double residue = 0.0;
         double residueSq = 0.0;
         double moment2 = 0.0;
         double moment3 = 0.0;
         double skewness = 0.0;
         double denominator = 0.0;
         int windowStart = 0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double peak2 = sp.peak2;
         double shift = sp.shift;
         double total1 = sp.total1;
         double total2 = sp.total2;
         double total3 = sp.total3;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.i & sp.xMask;
         pkVal0 = inReal;
         dev = (((sp.i & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0) - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         if( total2 > peak2 ) {
            peak2 = total2;
         }
         residue = total1 * sp.invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - sp.dPeriod * residueSq;
         moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
         /* Each bar rounds the cubic sum by at most eps times the peak second
          * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
          * 1000x before the rebuild at 1% of the peak.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peak2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = sp.reseedPeriod;
            windowStart = sp.i - sp.nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
            }
            shift = tempReal * sp.invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               dev = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
            }
            residue = total1 * sp.invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - sp.dPeriod * residueSq;
            moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            /* The left-to-right sum can miss the mean by more than the window's
             * spread, and the trigger would then fire again on every bar. Adding
             * the measured miss back lands within about an ulp of the mean, where
             * a flat window's deviations are exactly 0. Keep this threshold the
             * trigger's.
             */
            if( moment2 < 0.01 * total2 ) {
               shift += residue;
               total1 = 0.0;
               total2 = 0.0;
               total3 = 0.0;
               for( j = windowStart; j <= sp.i; j += 1 ) {
                  dev = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
                  dev2 = dev * dev;
                  total1 += dev;
                  total2 += dev2;
                  total3 += dev2 * dev;
               }
               residue = total1 * sp.invPeriod;
               residueSq = residue * residue;
               moment2 = total2 - sp.dPeriod * residueSq;
               moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            }
            peak2 = total2;
         }
         /* A window of equal values is symmetric, and reaches here with every
          * deviation exactly 0. The bound is the smallest normal double, not a
          * scale: a denominator under it comes with cubed deviations that have
          * lost their precision in the subnormals.
          */
         denominator = moment2 * Math.sqrt(moment2);
         if( denominator < 2.2250738585072014e-308 ) {
            skewness = 0.0;
         } else {
            skewness = sp.coef * moment3 / denominator;
         }
         /* inReal and outReal may be the same buffer: read the trailing value
          * before the output write.
          */
         dev = (((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         trailingIdx += 1;
         cur_outReal = skewness;
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
      public SkewStream clone() {
         return new SkewStream(this);
      }
   }
   private void skewStepImpl( SkewStream sp, double inReal )
   {
      double tempReal = 0.0;
      double dev = 0.0;
      double dev2 = 0.0;
      double residue = 0.0;
      double residueSq = 0.0;
      double moment2 = 0.0;
      double moment3 = 0.0;
      double skewness = 0.0;
      double denominator = 0.0;
      int windowStart = 0;
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      dev = sp.x_inReal[sp.i & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 += dev;
      sp.total2 += dev2;
      sp.total3 += dev2 * dev;
      if( sp.total2 > sp.peak2 ) {
         sp.peak2 = sp.total2;
      }
      residue = sp.total1 * sp.invPeriod;
      residueSq = residue * residue;
      moment2 = sp.total2 - sp.dPeriod * residueSq;
      moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, sp.total3 - 3.0 * residue * sp.total2);
      /* Each bar rounds the cubic sum by at most eps times the peak second
       * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
       * 1000x before the rebuild at 1% of the peak.
       */
      sp.barsSinceReseed -= 1;
      if( moment2 < 0.01 * sp.peak2 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = sp.reseedPeriod;
         windowStart = sp.i - sp.nbInitialElementNeeded;
         tempReal = 0.0;
         for( sp.j = windowStart; sp.j <= sp.i; sp.j += 1 ) {
            tempReal += sp.x_inReal[sp.j & sp.xMask];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.total1 = 0.0;
         sp.total2 = 0.0;
         sp.total3 = 0.0;
         for( sp.j = windowStart; sp.j <= sp.i; sp.j += 1 ) {
            dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
            dev2 = dev * dev;
            sp.total1 += dev;
            sp.total2 += dev2;
            sp.total3 += dev2 * dev;
         }
         residue = sp.total1 * sp.invPeriod;
         residueSq = residue * residue;
         moment2 = sp.total2 - sp.dPeriod * residueSq;
         moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, sp.total3 - 3.0 * residue * sp.total2);
         /* The left-to-right sum can miss the mean by more than the window's
          * spread, and the trigger would then fire again on every bar. Adding
          * the measured miss back lands within about an ulp of the mean, where
          * a flat window's deviations are exactly 0. Keep this threshold the
          * trigger's.
          */
         if( moment2 < 0.01 * sp.total2 ) {
            sp.shift += residue;
            sp.total1 = 0.0;
            sp.total2 = 0.0;
            sp.total3 = 0.0;
            for( sp.j = windowStart; sp.j <= sp.i; sp.j += 1 ) {
               dev = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
               dev2 = dev * dev;
               sp.total1 += dev;
               sp.total2 += dev2;
               sp.total3 += dev2 * dev;
            }
            residue = sp.total1 * sp.invPeriod;
            residueSq = residue * residue;
            moment2 = sp.total2 - sp.dPeriod * residueSq;
            moment3 = Math.fma(2.0 * sp.dPeriod * residueSq, residue, sp.total3 - 3.0 * residue * sp.total2);
         }
         sp.peak2 = sp.total2;
      }
      /* A window of equal values is symmetric, and reaches here with every
       * deviation exactly 0. The bound is the smallest normal double, not a
       * scale: a denominator under it comes with cubed deviations that have
       * lost their precision in the subnormals.
       */
      denominator = moment2 * Math.sqrt(moment2);
      if( denominator < 2.2250738585072014e-308 ) {
         skewness = 0.0;
      } else {
         skewness = sp.coef * moment3 / denominator;
      }
      /* inReal and outReal may be the same buffer: read the trailing value
       * before the output write.
       */
      dev = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      dev2 = dev * dev;
      sp.total1 -= dev;
      sp.total2 -= dev2;
      sp.total3 -= dev2 * dev;
      sp.trailingIdx += 1;
      sp.cur_outReal = skewness;
      sp.i += 1;
   }
   private RetCode skewOpenImpl( SkewStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double shift = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double dev = 0;
      double dev2 = 0;
      double residue = 0;
      double residueSq = 0;
      double moment2 = 0;
      double moment3 = 0;
      double dPeriod = 0;
      double coef = 0;
      double invPeriod = 0;
      double skewness = 0;
      double peak2 = 0;
      double denominator = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceReseed = 0;
      int reseedPeriod = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 3 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* G1 = n*sqrt(n-1)/(n-2) * M3 / M2^(3/2), over the central sums M2 and
       * M3. The n-2 is why the range starts at 3, and the n-1 lookback hands the
       * estimator a full window at the first emitted bar.
       */
      dPeriod = (double)optInTimePeriod;
      coef = dPeriod * Math.sqrt(dPeriod - 1.0) / (dPeriod - 2.0);
      invPeriod = 1.0 / dPeriod;
      /* Deviations are measured against a shift near the window, and the
       * central sums are recovered from them below. The periodic rebuild bounds
       * the rounding the slide accumulates, and is the only way out of a NaN or
       * Inf input, on which every trigger comparison is false.
       */
      reseedPeriod = 32 * optInTimePeriod;
      trailingIdx = startIdx - nbInitialElementNeeded;
      shift = inReal[trailingIdx];
      total1 = 0.0;
      total2 = 0.0;
      total3 = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         dev = inReal[j] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = reseedPeriod;
      peak2 = total2;
      do {
         dev = inReal[i] - shift;
         dev2 = dev * dev;
         total1 += dev;
         total2 += dev2;
         total3 += dev2 * dev;
         if( total2 > peak2 ) {
            peak2 = total2;
         }
         residue = total1 * invPeriod;
         residueSq = residue * residue;
         moment2 = total2 - dPeriod * residueSq;
         moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
         /* Each bar rounds the cubic sum by at most eps times the peak second
          * sum to the power 3/2, so G1's error grows as (peak/M2)^(3/2), at most
          * 1000x before the rebuild at 1% of the peak.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peak2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = reseedPeriod;
            windowStart = i - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            total1 = 0.0;
            total2 = 0.0;
            total3 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               dev = inReal[j] - shift;
               dev2 = dev * dev;
               total1 += dev;
               total2 += dev2;
               total3 += dev2 * dev;
            }
            residue = total1 * invPeriod;
            residueSq = residue * residue;
            moment2 = total2 - dPeriod * residueSq;
            moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            /* The left-to-right sum can miss the mean by more than the window's
             * spread, and the trigger would then fire again on every bar. Adding
             * the measured miss back lands within about an ulp of the mean, where
             * a flat window's deviations are exactly 0. Keep this threshold the
             * trigger's.
             */
            if( moment2 < 0.01 * total2 ) {
               shift += residue;
               total1 = 0.0;
               total2 = 0.0;
               total3 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  dev = inReal[j] - shift;
                  dev2 = dev * dev;
                  total1 += dev;
                  total2 += dev2;
                  total3 += dev2 * dev;
               }
               residue = total1 * invPeriod;
               residueSq = residue * residue;
               moment2 = total2 - dPeriod * residueSq;
               moment3 = Math.fma(2.0 * dPeriod * residueSq, residue, total3 - 3.0 * residue * total2);
            }
            peak2 = total2;
         }
         /* A window of equal values is symmetric, and reaches here with every
          * deviation exactly 0. The bound is the smallest normal double, not a
          * scale: a denominator under it comes with cubed deviations that have
          * lost their precision in the subnormals.
          */
         denominator = moment2 * Math.sqrt(moment2);
         if( denominator < 2.2250738585072014e-308 ) {
            skewness = 0.0;
         } else {
            skewness = coef * moment3 / denominator;
         }
         /* inReal and outReal may be the same buffer: read the trailing value
          * before the output write.
          */
         dev = inReal[trailingIdx] - shift;
         dev2 = dev * dev;
         total1 -= dev;
         total2 -= dev2;
         total3 -= dev2 * dev;
         trailingIdx += 1;
         outReal[outIdx++ * outStride] = skewness;
         i += 1;
      } while( i <= endIdx );
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
      sp.shift = shift;
      sp.total1 = total1;
      sp.total2 = total2;
      sp.total3 = total3;
      sp.dPeriod = dPeriod;
      sp.coef = coef;
      sp.invPeriod = invPeriod;
      sp.peak2 = peak2;
      sp.trailingIdx = trailingIdx;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.barsSinceReseed = barsSinceReseed;
      sp.reseedPeriod = reseedPeriod;
      sp.j = j;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* skewOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SkewStream skewOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SkewStream sp = new SkewStream(this);
      RetCode retCode = skewOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SKEW openAndFill", inReal.length, startIdx, skewLookback(optInTimePeriod));
      }
      throw streamFailure("SKEW openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind skewOpen (composition seam). */
   SkewStream skewOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      SkewStream sp = new SkewStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = skewOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SKEW open", inReal.length, startIdx, skewLookback(optInTimePeriod));
      }
      throw streamFailure("SKEW open", retCode);
   }
   /**
    * Open a live SKEW stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#skew} at that bar.
    * <p>The history must hold at least {@code skewLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link InsufficientHistoryException} too, and a null argument
    * {@link IllegalArgumentException}, both ahead of everything above.
    */
   public SkewStream skewOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("SKEW open", "inReal", inReal);
      requireHistory("SKEW open", inReal.length);
      return skewOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#skewOpen} that also fills the output array(s) bit-identically
    * to {@link Core#skew} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SkewStream#outRange()}.
    */
   public SkewStream skewOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("SKEW openAndFill", "inReal", inReal);
      requireHistory("SKEW openAndFill", inReal.length);
      int guardOutLen = openFillCount("SKEW openAndFill", inReal.length, skewLookback(optInTimePeriod));
      requireLength("SKEW openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SKEW openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return skewOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
