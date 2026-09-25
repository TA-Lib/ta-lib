/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #73).
 *  092226 MF,CC  Binary search above 128 values, one shift per bar (issue #432).
 */

   /**
    * Number of leading input bars {@link Core#median} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..10000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int medianLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode medianImpl( int startIdx,
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
      int lo = 0;
      int hi = 0;
      int mid = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      /* Never read: set so two handles opened over the same bars hold the same
       * state.
       */
      sorted[lookbackTotal] = 0.0;
      /* The two central ordinals, zero-based; the same slot at odd n. */
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = inReal[i];
         if( lookbackTotal < 128 ) {
            j = nbSorted;
            while( j > 0 && sorted[j - 1] > newValue ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
         } else {
            lo = 0;
            hi = nbSorted;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = nbSorted;
            while( j > lo ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
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
       * Every buffer store sits BELOW the output store: deriving the whole answer
       * read-only above it is what lets the streaming peek frame drop the state
       * update.
       *
       * Below 128 values a linear scan beats a binary search: one mispredicted
       * loop exit costs less than log2(n) unpredictable halvings.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         /* pos counts the retained values <= newValue, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..].
          */
         if( lookbackTotal < 128 ) {
            pos = 0;
            while( pos < lookbackTotal && sorted[pos] <= newValue ) {
               pos += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            pos = lo;
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
         /* At odd n both reads are one value; averaging it would overflow above
          * DBL_MAX/2.
          */
         if( lowerIdx == upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         outReal[outIdx] = result;
         outIdx += 1;
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         /* j is the first retained value >= oldValue. Keep every run of equal
          * values in age order (newValue goes after its equals, as above): the
          * oldest of a run is then the departing value bit for bit, which is what
          * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
          * no value but flips the sign of some zero outputs.
          */
         if( lookbackTotal < 128 ) {
            j = 0;
            while( j < lookbackTotal && sorted[j] < oldValue ) {
               j += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] < oldValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = lo;
         }
         /* Evict oldValue and place newValue with one shift of the slots between
          * them.
          */
         if( j < pos ) {
            while( j < pos - 1 ) {
               sorted[j] = sorted[j + 1];
               j += 1;
            }
            sorted[pos - 1] = newValue;
         } else {
            while( j > pos ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
            sorted[pos] = newValue;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode medianImpl( int startIdx,
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
      int lo = 0;
      int hi = 0;
      int mid = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      sorted[lookbackTotal] = 0.0;
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = (double)inReal[i];
         if( lookbackTotal < 128 ) {
            j = nbSorted;
            while( j > 0 && sorted[j - 1] > newValue ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
         } else {
            lo = 0;
            hi = nbSorted;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = nbSorted;
            while( j > lo ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
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
         if( lookbackTotal < 128 ) {
            pos = 0;
            while( pos < lookbackTotal && sorted[pos] <= newValue ) {
               pos += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            pos = lo;
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
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         if( lookbackTotal < 128 ) {
            j = 0;
            while( j < lookbackTotal && sorted[j] < oldValue ) {
               j += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] < oldValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = lo;
         }
         if( j < pos ) {
            while( j < pos - 1 ) {
               sorted[j] = sorted[j + 1];
               j += 1;
            }
            sorted[pos - 1] = newValue;
         } else {
            while( j > pos ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
            sorted[pos] = newValue;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * The middle order statistic of the trailing window: the central value when
    * {@code optInTimePeriod} is odd, the mean of the two central values when it
    * is even. A robust measure of central tendency — unlike <a
    * href="https://ta-lib.org/functions/sma">{@code SMA}</a>, a single spike
    * moves it by at most one rank however large the spike is, which is what
    * makes it useful as a filter rather than as a level. Not to be confused
    * with <a href="https://ta-lib.org/functions/medprice">{@code MEDPRICE}</a>,
    * which is {@code (High + Low) / 2} of one bar and is not an order
    * statistic.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/median">ta-lib.org/functions/median</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is not <a href="https://ta-lib.org/functions/percentile">{@code PERCENTILE}</a> at 50. {@code PERCENTILE} reports the nearest rank, which at even {@code n} selects the <b>lower</b> of the two central values: on a 4-bar window of {@code 1, 2, 3, 4} this function returns {@code 2.5} and {@code PERCENTILE} returns {@code 2}. At odd {@code n} the two agree bit for bit.</li>
    * <li>At even {@code n} the output can therefore be a value the series never traded at, which is the deliberate opposite of {@code PERCENTILE}'s design property.</li>
    * <li>{@code optInTimePeriod} is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.</li>
    * <li>Every input value must be finite. A NaN makes every comparison against it false, which breaks the order the window is kept in, and the output can stay wrong long after the NaN has left the window.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#medianLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to take the median of.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..10000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Median of the trailing window. Must hold at least
    *        {@code endIdx - max(startIdx, medianLookback(...)) + 1} values, the count
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
    * @see Core#percentile
    * @see Core#sma
    * @see Core#medprice
    */
   public OutRange median( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("MEDIAN", startIdx, endIdx);
      int guardStart = clampedStart("MEDIAN", startIdx, medianLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MEDIAN", "inReal", inReal, guardInLen);
      requireLength("MEDIAN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = medianImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MEDIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The middle order statistic of the trailing window: the central value when
    * {@code optInTimePeriod} is odd, the mean of the two central values when it
    * is even. A robust measure of central tendency — unlike <a
    * href="https://ta-lib.org/functions/sma">{@code SMA}</a>, a single spike
    * moves it by at most one rank however large the spike is, which is what
    * makes it useful as a filter rather than as a level. Not to be confused
    * with <a href="https://ta-lib.org/functions/medprice">{@code MEDPRICE}</a>,
    * which is {@code (High + Low) / 2} of one bar and is not an order
    * statistic.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/median">ta-lib.org/functions/median</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is not <a href="https://ta-lib.org/functions/percentile">{@code PERCENTILE}</a> at 50. {@code PERCENTILE} reports the nearest rank, which at even {@code n} selects the <b>lower</b> of the two central values: on a 4-bar window of {@code 1, 2, 3, 4} this function returns {@code 2.5} and {@code PERCENTILE} returns {@code 2}. At odd {@code n} the two agree bit for bit.</li>
    * <li>At even {@code n} the output can therefore be a value the series never traded at, which is the deliberate opposite of {@code PERCENTILE}'s design property.</li>
    * <li>{@code optInTimePeriod} is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.</li>
    * <li>Every input value must be finite. A NaN makes every comparison against it false, which breaks the order the window is kept in, and the output can stay wrong long after the NaN has left the window.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#medianLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to take the median of.
    * @param optInTimePeriod Number of trailing values in the window (default
    *        30; range 2..10000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Median of the trailing window. Must hold at least
    *        {@code endIdx - max(startIdx, medianLookback(...)) + 1} values, the count
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
    * @see Core#percentile
    * @see Core#sma
    * @see Core#medprice
    */
   public OutRange median( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("MEDIAN", startIdx, endIdx);
      int guardStart = clampedStart("MEDIAN", startIdx, medianLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MEDIAN", "inReal", inReal, guardInLen);
      requireLength("MEDIAN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = medianImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MEDIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MEDIAN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#median} over the same series.
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
       * <p>It is what {@link Core#median} reports over the same bars: the
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
            throw failure("MEDIAN advance", RetCode.OUT_OF_RANGE_END_INDEX);
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
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("MEDIAN update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MEDIAN update", "inReal");
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
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MEDIAN peek", "inReal");
         MedianStream sp = this;
         double newValue = 0.0;
         double result = 0.0;
         double lower = 0.0;
         double upper = 0.0;
         int pos = 0;
         int lo = 0;
         int hi = 0;
         int mid = 0;
         double cur_outReal = 0.0;
         newValue = inReal;
         /* pos counts the retained values <= newValue, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..].
          */
         if( sp.lookbackTotal < 128 ) {
            pos = 0;
            while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
               pos += 1;
            }
         } else {
            lo = 0;
            hi = sp.lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sp.cb_sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            pos = lo;
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
         /* At odd n both reads are one value; averaging it would overflow above
          * DBL_MAX/2.
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
      int lo = 0;
      int hi = 0;
      int mid = 0;
      newValue = inReal;
      /* pos counts the retained values <= newValue, so the full window is
       * sorted[0..pos-1], newValue, sorted[pos..].
       */
      if( sp.lookbackTotal < 128 ) {
         pos = 0;
         while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
            pos += 1;
         }
      } else {
         lo = 0;
         hi = sp.lookbackTotal;
         while( lo < hi ) {
            mid = (lo + hi) / 2;
            if( sp.cb_sorted[mid] <= newValue ) {
               lo = mid + 1;
            } else {
               hi = mid;
            }
         }
         pos = lo;
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
      /* At odd n both reads are one value; averaging it would overflow above
       * DBL_MAX/2.
       */
      if( sp.lowerIdx == sp.upperIdx ) {
         result = lower;
      } else {
         result = (lower + upper) / 2.0;
      }
      sp.cur_outReal = result;
      sp.cb_ring[sp.ring_Idx] = newValue;
      sp.ring_Idx = sp.ring_Idx + 1;
      if( sp.ring_Idx > sp.maxIdx_ring ) {
         sp.ring_Idx = 0;
      }
      oldValue = sp.cb_ring[sp.ring_Idx];
      /* j is the first retained value >= oldValue. Keep every run of equal
       * values in age order (newValue goes after its equals, as above): the
       * oldest of a run is then the departing value bit for bit, which is what
       * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
       * no value but flips the sign of some zero outputs.
       */
      if( sp.lookbackTotal < 128 ) {
         j = 0;
         while( j < sp.lookbackTotal && sp.cb_sorted[j] < oldValue ) {
            j += 1;
         }
      } else {
         lo = 0;
         hi = sp.lookbackTotal;
         while( lo < hi ) {
            mid = (lo + hi) / 2;
            if( sp.cb_sorted[mid] < oldValue ) {
               lo = mid + 1;
            } else {
               hi = mid;
            }
         }
         j = lo;
      }
      /* Evict oldValue and place newValue with one shift of the slots between
       * them.
       */
      if( j < pos ) {
         while( j < pos - 1 ) {
            sp.cb_sorted[j] = sp.cb_sorted[j + 1];
            j += 1;
         }
         sp.cb_sorted[pos - 1] = newValue;
      } else {
         while( j > pos ) {
            sp.cb_sorted[j] = sp.cb_sorted[j - 1];
            j -= 1;
         }
         sp.cb_sorted[pos] = newValue;
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
      int lo = 0;
      int hi = 0;
      int mid = 0;
      double[] ring;
      int ring_Idx = 0;
      int maxIdx_ring = (30)-1;
      double[] sorted;
      int sorted_Idx = 0;
      int maxIdx_sorted = (30)-1;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      ring = new double[optInTimePeriod];
      maxIdx_ring = (optInTimePeriod)-1;
      ring_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sorted = new double[optInTimePeriod];
      maxIdx_sorted = (optInTimePeriod)-1;
      sorted_Idx = 0;
      /* Never read: set so two handles opened over the same bars hold the same
       * state.
       */
      sorted[lookbackTotal] = 0.0;
      /* The two central ordinals, zero-based; the same slot at odd n. */
      lowerIdx = (optInTimePeriod - 1) / 2;
      upperIdx = optInTimePeriod / 2;
      nbSorted = 0;
      i = startIdx - lookbackTotal;
      while( i < startIdx ) {
         newValue = inReal[i];
         if( lookbackTotal < 128 ) {
            j = nbSorted;
            while( j > 0 && sorted[j - 1] > newValue ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
         } else {
            lo = 0;
            hi = nbSorted;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = nbSorted;
            while( j > lo ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
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
       * Every buffer store sits BELOW the output store: deriving the whole answer
       * read-only above it is what lets the streaming peek frame drop the state
       * update.
       *
       * Below 128 values a linear scan beats a binary search: one mispredicted
       * loop exit costs less than log2(n) unpredictable halvings.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         /* pos counts the retained values <= newValue, so the full window is
          * sorted[0..pos-1], newValue, sorted[pos..].
          */
         if( lookbackTotal < 128 ) {
            pos = 0;
            while( pos < lookbackTotal && sorted[pos] <= newValue ) {
               pos += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] <= newValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            pos = lo;
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
         /* At odd n both reads are one value; averaging it would overflow above
          * DBL_MAX/2.
          */
         if( lowerIdx == upperIdx ) {
            result = lower;
         } else {
            result = (lower + upper) / 2.0;
         }
         outReal[outIdx * outStride] = result;
         outIdx += 1;
         ring[ring_Idx] = newValue;
         ring_Idx++;
         if( ring_Idx > maxIdx_ring ) { ring_Idx = 0; }
         oldValue = ring[ring_Idx];
         /* j is the first retained value >= oldValue. Keep every run of equal
          * values in age order (newValue goes after its equals, as above): the
          * oldest of a run is then the departing value bit for bit, which is what
          * keeps -0.0 and 0.0 apart. Inserting before the equals instead changes
          * no value but flips the sign of some zero outputs.
          */
         if( lookbackTotal < 128 ) {
            j = 0;
            while( j < lookbackTotal && sorted[j] < oldValue ) {
               j += 1;
            }
         } else {
            lo = 0;
            hi = lookbackTotal;
            while( lo < hi ) {
               mid = (lo + hi) / 2;
               if( sorted[mid] < oldValue ) {
                  lo = mid + 1;
               } else {
                  hi = mid;
               }
            }
            j = lo;
         }
         /* Evict oldValue and place newValue with one shift of the slots between
          * them.
          */
         if( j < pos ) {
            while( j < pos - 1 ) {
               sorted[j] = sorted[j + 1];
               j += 1;
            }
            sorted[pos - 1] = newValue;
         } else {
            while( j > pos ) {
               sorted[j] = sorted[j - 1];
               j -= 1;
            }
            sorted[pos] = newValue;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_ring = maxIdx_ring + 1;
      if( capCb_ring > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_sorted = maxIdx_sorted + 1;
      if( capCb_sorted > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
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
      return RetCode.SUCCESS;
   }
   /* medianOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MedianStream medianOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MedianStream sp = new MedianStream(this);
      RetCode retCode = medianOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MEDIAN openAndFill", inReal.length, startIdx, medianLookback(optInTimePeriod));
      }
      throw streamFailure("MEDIAN openAndFill", retCode);
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
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MEDIAN open", inReal.length, startIdx, medianLookback(optInTimePeriod));
      }
      throw streamFailure("MEDIAN open", retCode);
   }
   /**
    * Open a live MEDIAN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#median} at that bar.
    * <p>The history must hold at least {@code medianLookback(...) + 1} bars
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
    * to {@link Core#median} over the whole history in the same single pass
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
      int guardOutLen = openFillCount("MEDIAN openAndFill", inReal.length, medianLookback(optInTimePeriod));
      requireLength("MEDIAN openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("MEDIAN openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return medianOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
