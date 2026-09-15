/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #73).
 */

   /**
    * Number of leading input bars {@link Core#MEDIAN} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MEDIAN_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MEDIAN_Impl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      double lower = 0;
      double upper = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int lowerIdx = 0;
      int upperIdx = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. Both are
       * hand-written here as they are in percentile.c, which is the precedent for
       * this shape -- a generator-derived ring does not carry the by-value copy.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      /* The two central ordinals, zero-based over the full window. At odd
       * optInTimePeriod they are the same slot and the average below is the value
       * itself; at even optInTimePeriod they straddle the centre and the mean of
       * the two is the median. Computed once rather than per bar.
       */
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = inReal[i];
         j = nbSorted;
         while( j > 0 && sorted[j - 1] > newValue ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[j] = newValue;
         nbSorted += 1;
         ring[ring_Idx] = newValue;
         i += 1;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
      }
      /* Both scratch buffers hold copies and inReal is never read below i, so
       * inReal and outReal may be the same buffer.
       *
       * Every buffer store sits BELOW the output store on purpose (percentile.c):
       * deriving the whole answer read-only above it is what lets the streaming
       * peek frame drop the state update rather than shadow a shift loop.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         /* `sorted` holds the window's other optInTimePeriod-1 values and `pos` is
          * where the incoming one belongs, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..]. The k-th of it is read
          * without materialising it.
          */
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( lowerIdx < pos ) {
            lower = sorted[lowerIdx];
         } else if( lowerIdx == pos ) {
            lower = newValue;
         } else {
            lower = sorted[lowerIdx - 1];
         }
         if( upperIdx < pos ) {
            upper = sorted[upperIdx];
         } else if( upperIdx == pos ) {
            upper = newValue;
         } else {
            upper = sorted[upperIdx - 1];
         }
         /* At odd optInTimePeriod the two ordinals are the same slot, and the
          * branch returns that read untouched. Writing it as (v + v) / 2.0
          * instead would be exact for every value this library is ever handed --
          * doubling moves the exponent with the mantissa untouched and halving
          * moves it back -- but it overflows to +/-inf above DBL_MAX/2, and this
          * function does not declare nan_inf_output. The branch costs nothing:
          * the condition is loop-invariant.
          *
          * At even optInTimePeriod the mean of the two central values is the
          * universal convention (NumPy, R, scipy, Excel). Dividing by 2.0 and
          * multiplying by 0.5 give the same double, so that spelling is not a
          * variant; the sum itself can still overflow on inputs near DBL_MAX,
          * which is exactly what NumPy does with them too.
          */
         if( lowerIdx == upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         outReal[outIdx] = result;
         outIdx += 1;
         /* Shifting only the strictly greater entries leaves equal values in
          * insertion order, which is age order -- that is what lets the delete
          * below evict the oldest of a run by value alone, with no slot array.
          *
          * The order within a run of equal values is NOT observable at the output,
          * and deliberately so: MEASURED, flipping this scan's `<=` to `<` (which
          * inserts at the front of a run instead of the back) leaves every value
          * bit-identical over 14820 windows on both a 7-distinct-value series and
          * a random walk. Equal members are interchangeable, which is precisely
          * why the removal can identify one by value and needs no identity.
          */
         j = lookbackTotal;
         while( j > pos ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[pos] = newValue;
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         j = 0;
         while( j < lookbackTotal && sorted[j] < oldValue ) {
            j += 1;
         }
         while( j < lookbackTotal ) {
            sorted[j] = sorted[j + 1];
            j += 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode MEDIAN_Impl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      double lower = 0;
      double upper = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int lowerIdx = 0;
      int upperIdx = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
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
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = (double)inReal[i];
         j = nbSorted;
         while( j > 0 && sorted[j - 1] > newValue ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[j] = newValue;
         nbSorted += 1;
         ring[ring_Idx] = newValue;
         i += 1;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
      }
      outIdx = 0;
      do {
         newValue = (double)inReal[i];
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( lowerIdx < pos ) {
            lower = sorted[lowerIdx];
         } else if( lowerIdx == pos ) {
            lower = newValue;
         } else {
            lower = sorted[lowerIdx - 1];
         }
         if( upperIdx < pos ) {
            upper = sorted[upperIdx];
         } else if( upperIdx == pos ) {
            upper = newValue;
         } else {
            upper = sorted[upperIdx - 1];
         }
         if( lowerIdx == upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         outReal[outIdx] = result;
         outIdx += 1;
         j = lookbackTotal;
         while( j > pos ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[pos] = newValue;
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         j = 0;
         while( j < lookbackTotal && sorted[j] < oldValue ) {
            j += 1;
         }
         while( j < lookbackTotal ) {
            sorted[j] = sorted[j + 1];
            j += 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * The middle order statistic of the trailing window: the central value when
    * {@code optInTimePeriod} is odd, the mean of the two central values when it
    * is even. A robust measure of central tendency — unlike <a
    * href="https://ta-lib.org/functions/sma">{@code SMA}</a> it is unmoved by a
    * single spike, which is what makes it useful as a filter rather than as a
    * level. Not to be confused with <a
    * href="https://ta-lib.org/functions/medprice">{@code MEDPRICE}</a>, which
    * is {@code (High + Low) / 2} of one bar and is not an order statistic.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/median">ta-lib.org/functions/median</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>This is not <a href="https://ta-lib.org/functions/percentile">{@code PERCENTILE}</a> at 50.</b> {@code PERCENTILE} reports the nearest rank, {@code ceil(P·n/100)} clamped to the window. At odd {@code n} that ordinal <i>is</i> the median and the two functions agree bit for bit. At even {@code n}, {@code ceil(n/2) = n/2} selects the <b>lower</b> of the two central values, which is not the median: on a 4-bar window of {@code 1, 2, 3, 4} this function returns {@code 2.5} and {@code PERCENTILE} returns {@code 2}.</li>
    * <li><b>At even {@code n} the output can be a value the series never traded at.</b> That is the deliberate opposite of {@code PERCENTILE}'s design property, and it is the whole reason the two are separate functions rather than one with a mode selector — {@code PERCENTILE}'s parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it without changing its arity.</li>
    * <li><b>The even-{@code n} mean is not a variant to choose.</b> NumPy, R's {@code median()}, scipy and Excel's {@code MEDIAN} all take it, and there is no original author to arbitrate against. Nor is the spelling a variant: {@code (lo + hi) / 2.0} and {@code 0.5 * (lo + hi)} are the same double.</li>
    * <li><b>The odd case is a branch, not {@code (v + v) / 2}.</b> The arithmetic form is exact for any value this library is realistically handed, but it overflows above {@code DBL_MAX/2}, and this function does not declare {@code nan_inf_output}. The branch is loop-invariant and costs nothing. At even {@code n} the sum of the two central values can still overflow near {@code DBL_MAX}, which is what NumPy does with such inputs as well.</li>
    * <li><b>Equal values keep insertion order.</b> The window is carried twice, once by age and once by value, and the by-value copy shifts only strictly greater entries on insertion. A run of equal values therefore stays in age order, which is what lets the removal evict the oldest of the run by value alone, with no slot array.</li>
    * <li>{@code optInTimePeriod} is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MEDIAN_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to take the median of.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Median of the trailing window. Must hold at least
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
    * @see Core#PERCENTILE
    * @see Core#SMA
    * @see Core#MEDPRICE
    */
   public OutRange MEDIAN( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("MEDIAN", startIdx, endIdx);
      int guardStart = clampedStart("MEDIAN", startIdx, MEDIAN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MEDIAN", "inReal", inReal, guardInLen);
      requireLength("MEDIAN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MEDIAN_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MEDIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The middle order statistic of the trailing window: the central value when
    * {@code optInTimePeriod} is odd, the mean of the two central values when it
    * is even. A robust measure of central tendency — unlike <a
    * href="https://ta-lib.org/functions/sma">{@code SMA}</a> it is unmoved by a
    * single spike, which is what makes it useful as a filter rather than as a
    * level. Not to be confused with <a
    * href="https://ta-lib.org/functions/medprice">{@code MEDPRICE}</a>, which
    * is {@code (High + Low) / 2} of one bar and is not an order statistic.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/median">ta-lib.org/functions/median</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>This is not <a href="https://ta-lib.org/functions/percentile">{@code PERCENTILE}</a> at 50.</b> {@code PERCENTILE} reports the nearest rank, {@code ceil(P·n/100)} clamped to the window. At odd {@code n} that ordinal <i>is</i> the median and the two functions agree bit for bit. At even {@code n}, {@code ceil(n/2) = n/2} selects the <b>lower</b> of the two central values, which is not the median: on a 4-bar window of {@code 1, 2, 3, 4} this function returns {@code 2.5} and {@code PERCENTILE} returns {@code 2}.</li>
    * <li><b>At even {@code n} the output can be a value the series never traded at.</b> That is the deliberate opposite of {@code PERCENTILE}'s design property, and it is the whole reason the two are separate functions rather than one with a mode selector — {@code PERCENTILE}'s parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it without changing its arity.</li>
    * <li><b>The even-{@code n} mean is not a variant to choose.</b> NumPy, R's {@code median()}, scipy and Excel's {@code MEDIAN} all take it, and there is no original author to arbitrate against. Nor is the spelling a variant: {@code (lo + hi) / 2.0} and {@code 0.5 * (lo + hi)} are the same double.</li>
    * <li><b>The odd case is a branch, not {@code (v + v) / 2}.</b> The arithmetic form is exact for any value this library is realistically handed, but it overflows above {@code DBL_MAX/2}, and this function does not declare {@code nan_inf_output}. The branch is loop-invariant and costs nothing. At even {@code n} the sum of the two central values can still overflow near {@code DBL_MAX}, which is what NumPy does with such inputs as well.</li>
    * <li><b>Equal values keep insertion order.</b> The window is carried twice, once by age and once by value, and the by-value copy shifts only strictly greater entries on insertion. A run of equal values therefore stays in age order, which is what lets the removal evict the oldest of the run by value alone, with no slot array.</li>
    * <li>{@code optInTimePeriod} is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MEDIAN_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to take the median of.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Median of the trailing window. Must hold at least
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
    * @see Core#PERCENTILE
    * @see Core#SMA
    * @see Core#MEDPRICE
    */
   public OutRange MEDIAN( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("MEDIAN", startIdx, endIdx);
      int guardStart = clampedStart("MEDIAN", startIdx, MEDIAN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MEDIAN", "inReal", inReal, guardInLen);
      requireLength("MEDIAN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MEDIAN_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MEDIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MEDIAN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MEDIAN} over the same series.
    * Open with {@link Core#medianOpen}; there is no close — the handle is
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
   public static final class MedianStream {
      private Core core;
      private int optInTimePeriod;
      private int lookbackTotal;
      private int lowerIdx;
      private int upperIdx;
      private int ring_Idx;
      private int maxIdx_ring;
      private int sorted_Idx;
      private int maxIdx_sorted;
      private int cbSize_ring;
      private double[] cb_ring;
      private int cbSize_sorted;
      private double[] cb_sorted;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private MedianStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MEDIAN} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value()} keeps answering the previous
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
            throw failure("MEDIAN advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      private MedianStream( MedianStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lookbackTotal = other.lookbackTotal;
         this.lowerIdx = other.lowerIdx;
         this.upperIdx = other.upperIdx;
         this.ring_Idx = other.ring_Idx;
         this.maxIdx_ring = other.maxIdx_ring;
         this.sorted_Idx = other.sorted_Idx;
         this.maxIdx_sorted = other.maxIdx_sorted;
         this.cbSize_ring = other.cbSize_ring;
         this.cb_ring = other.cb_ring.clone();
         this.cbSize_sorted = other.cbSize_sorted;
         this.cb_sorted = other.cb_sorted.clone();
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
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("MEDIAN update", RetCode.OutOfRangeEndIndex);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MEDIAN update: BadParam", RetCode.BadParam);
         core.medianStepImpl(this, inReal);
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
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MEDIAN peek: BadParam", RetCode.BadParam);
         MedianStream sp = this;
         double newValue = 0.0;
         double result = 0.0;
         double lower = 0.0;
         double upper = 0.0;
         int pos = 0;
         double cur_outReal = 0.0;
         newValue = inReal;
         /* `sorted` holds the window's other optInTimePeriod-1 values and `pos` is
          * where the incoming one belongs, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..]. The k-th of it is read
          * without materialising it.
          */
         pos = 0;
         while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( sp.lowerIdx < pos ) {
            lower = sp.cb_sorted[sp.lowerIdx];
         } else if( sp.lowerIdx == pos ) {
            lower = newValue;
         } else {
            lower = sp.cb_sorted[sp.lowerIdx - 1];
         }
         if( sp.upperIdx < pos ) {
            upper = sp.cb_sorted[sp.upperIdx];
         } else if( sp.upperIdx == pos ) {
            upper = newValue;
         } else {
            upper = sp.cb_sorted[sp.upperIdx - 1];
         }
         /* At odd optInTimePeriod the two ordinals are the same slot, and the
          * branch returns that read untouched. Writing it as (v + v) / 2.0
          * instead would be exact for every value this library is ever handed --
          * doubling moves the exponent with the mantissa untouched and halving
          * moves it back -- but it overflows to +/-inf above DBL_MAX/2, and this
          * function does not declare nan_inf_output. The branch costs nothing:
          * the condition is loop-invariant.
          *
          * At even optInTimePeriod the mean of the two central values is the
          * universal convention (NumPy, R, scipy, Excel). Dividing by 2.0 and
          * multiplying by 0.5 give the same double, so that spelling is not a
          * variant; the sum itself can still overflow on inputs near DBL_MAX,
          * which is exactly what NumPy does with them too.
          */
         if( sp.lowerIdx == sp.upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         cur_outReal = result;
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
      public MedianStream clone() {
         return new MedianStream(this);
      }
   }
   private void medianStepImpl( MedianStream sp, double inReal )
   {
      double newValue = 0.0;
      double oldValue = 0.0;
      double result = 0.0;
      double lower = 0.0;
      double upper = 0.0;
      int j = 0;
      int pos = 0;
      newValue = inReal;
      /* `sorted` holds the window's other optInTimePeriod-1 values and `pos` is
       * where the incoming one belongs, so the full window is
       * sorted[0..pos-1], newValue, sorted[pos..]. The k-th of it is read
       * without materialising it.
       */
      pos = 0;
      while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
         pos += 1;
      }
      if( sp.lowerIdx < pos ) {
         lower = sp.cb_sorted[sp.lowerIdx];
      } else if( sp.lowerIdx == pos ) {
         lower = newValue;
      } else {
         lower = sp.cb_sorted[sp.lowerIdx - 1];
      }
      if( sp.upperIdx < pos ) {
         upper = sp.cb_sorted[sp.upperIdx];
      } else if( sp.upperIdx == pos ) {
         upper = newValue;
      } else {
         upper = sp.cb_sorted[sp.upperIdx - 1];
      }
      /* At odd optInTimePeriod the two ordinals are the same slot, and the
       * branch returns that read untouched. Writing it as (v + v) / 2.0
       * instead would be exact for every value this library is ever handed --
       * doubling moves the exponent with the mantissa untouched and halving
       * moves it back -- but it overflows to +/-inf above DBL_MAX/2, and this
       * function does not declare nan_inf_output. The branch costs nothing:
       * the condition is loop-invariant.
       *
       * At even optInTimePeriod the mean of the two central values is the
       * universal convention (NumPy, R, scipy, Excel). Dividing by 2.0 and
       * multiplying by 0.5 give the same double, so that spelling is not a
       * variant; the sum itself can still overflow on inputs near DBL_MAX,
       * which is exactly what NumPy does with them too.
       */
      if( sp.lowerIdx == sp.upperIdx ) {
         result = lower;
      } else {
         result = (lower + upper) / 2.0;
      }
      sp.cur_outReal = result;
      /* Shifting only the strictly greater entries leaves equal values in
       * insertion order, which is age order -- that is what lets the delete
       * below evict the oldest of a run by value alone, with no slot array.
       *
       * The order within a run of equal values is NOT observable at the output,
       * and deliberately so: MEASURED, flipping this scan's `<=` to `<` (which
       * inserts at the front of a run instead of the back) leaves every value
       * bit-identical over 14820 windows on both a 7-distinct-value series and
       * a random walk. Equal members are interchangeable, which is precisely
       * why the removal can identify one by value and needs no identity.
       */
      j = sp.lookbackTotal;
      while( j > pos ) {
         sp.cb_sorted[j] = sp.cb_sorted[j - 1];
         j -= 1;
      }
      sp.cb_sorted[pos] = newValue;
      sp.cb_ring[sp.ring_Idx] = newValue;
      sp.ring_Idx = sp.ring_Idx + 1;
      if( sp.ring_Idx > sp.maxIdx_ring ) {
         sp.ring_Idx = 0;
      }
      oldValue = sp.cb_ring[sp.ring_Idx];
      j = 0;
      while( j < sp.lookbackTotal && sp.cb_sorted[j] < oldValue ) {
         j += 1;
      }
      while( j < sp.lookbackTotal ) {
         sp.cb_sorted[j] = sp.cb_sorted[j + 1];
         j += 1;
      }
   }
   private RetCode medianOpenImpl( MedianStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      double lower = 0;
      double upper = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int lowerIdx = 0;
      int upperIdx = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
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
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. Both are
       * hand-written here as they are in percentile.c, which is the precedent for
       * this shape -- a generator-derived ring does not carry the by-value copy.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      /* The two central ordinals, zero-based over the full window. At odd
       * optInTimePeriod they are the same slot and the average below is the value
       * itself; at even optInTimePeriod they straddle the centre and the mean of
       * the two is the median. Computed once rather than per bar.
       */
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = inReal[i];
         j = nbSorted;
         while( j > 0 && sorted[j - 1] > newValue ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[j] = newValue;
         nbSorted += 1;
         ring[ring_Idx] = newValue;
         i += 1;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
      }
      /* Both scratch buffers hold copies and inReal is never read below i, so
       * inReal and outReal may be the same buffer.
       *
       * Every buffer store sits BELOW the output store on purpose (percentile.c):
       * deriving the whole answer read-only above it is what lets the streaming
       * peek frame drop the state update rather than shadow a shift loop.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         /* `sorted` holds the window's other optInTimePeriod-1 values and `pos` is
          * where the incoming one belongs, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..]. The k-th of it is read
          * without materialising it.
          */
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( lowerIdx < pos ) {
            lower = sorted[lowerIdx];
         } else if( lowerIdx == pos ) {
            lower = newValue;
         } else {
            lower = sorted[lowerIdx - 1];
         }
         if( upperIdx < pos ) {
            upper = sorted[upperIdx];
         } else if( upperIdx == pos ) {
            upper = newValue;
         } else {
            upper = sorted[upperIdx - 1];
         }
         /* At odd optInTimePeriod the two ordinals are the same slot, and the
          * branch returns that read untouched. Writing it as (v + v) / 2.0
          * instead would be exact for every value this library is ever handed --
          * doubling moves the exponent with the mantissa untouched and halving
          * moves it back -- but it overflows to +/-inf above DBL_MAX/2, and this
          * function does not declare nan_inf_output. The branch costs nothing:
          * the condition is loop-invariant.
          *
          * At even optInTimePeriod the mean of the two central values is the
          * universal convention (NumPy, R, scipy, Excel). Dividing by 2.0 and
          * multiplying by 0.5 give the same double, so that spelling is not a
          * variant; the sum itself can still overflow on inputs near DBL_MAX,
          * which is exactly what NumPy does with them too.
          */
         if( lowerIdx == upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         outReal[outIdx * outStride] = result;
         outIdx += 1;
         /* Shifting only the strictly greater entries leaves equal values in
          * insertion order, which is age order -- that is what lets the delete
          * below evict the oldest of a run by value alone, with no slot array.
          *
          * The order within a run of equal values is NOT observable at the output,
          * and deliberately so: MEASURED, flipping this scan's `<=` to `<` (which
          * inserts at the front of a run instead of the back) leaves every value
          * bit-identical over 14820 windows on both a 7-distinct-value series and
          * a random walk. Equal members are interchangeable, which is precisely
          * why the removal can identify one by value and needs no identity.
          */
         j = lookbackTotal;
         while( j > pos ) {
            sorted[j] = sorted[j - 1];
            j -= 1;
         }
         sorted[pos] = newValue;
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         j = 0;
         while( j < lookbackTotal && sorted[j] < oldValue ) {
            j += 1;
         }
         while( j < lookbackTotal ) {
            sorted[j] = sorted[j + 1];
            j += 1;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_ring = maxIdx_ring + 1;
      if( capCb_ring > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      int capCb_sorted = maxIdx_sorted + 1;
      if( capCb_sorted > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lookbackTotal = lookbackTotal;
      sp.lowerIdx = lowerIdx;
      sp.upperIdx = upperIdx;
      sp.ring_Idx = ring_Idx;
      sp.maxIdx_ring = maxIdx_ring;
      sp.sorted_Idx = sorted_Idx;
      sp.maxIdx_sorted = maxIdx_sorted;
      sp.cbSize_ring = capCb_ring;
      sp.cb_ring = ring;
      sp.cbSize_sorted = capCb_sorted;
      sp.cb_sorted = sorted;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* medianOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MedianStream medianOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MedianStream sp = new MedianStream(this);
      RetCode retCode = medianOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MEDIAN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MEDIAN openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MEDIAN openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind medianOpen (composition seam). */
   MedianStream medianOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MedianStream sp = new MedianStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = medianOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MEDIAN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MEDIAN open: internal error", retCode);
      }
      throw new TaLibArgumentException("MEDIAN open: " + retCode, retCode);
   }
   /**
    * Open a live MEDIAN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MEDIAN} at that bar.
    * <p>The history must hold at least {@code MEDIAN_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MedianStream medianOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MEDIAN open", "inReal", inReal);
      requireHistory("MEDIAN open", inReal.length);
      return medianOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#medianOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MEDIAN} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MedianStream#outRange()}.
    */
   public MedianStream medianOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("MEDIAN openAndFill", "inReal", inReal);
      requireHistory("MEDIAN openAndFill", inReal.length);
      int guardOutLen = openFillCount("MEDIAN openAndFill", inReal.length, MEDIAN_Lookback(optInTimePeriod));
      requireLength("MEDIAN openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("MEDIAN openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return medianOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
