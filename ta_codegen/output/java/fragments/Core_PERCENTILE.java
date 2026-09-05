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
 *  090426 MF,CC  First version (issue #368).
 */

   /**
    * Number of leading input bars {@link Core#PERCENTILE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentile Percentage position within the sorted window
    *        (default 50; range 0..100; {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PERCENTILE_Lookback( int optInTimePeriod, double optInPercentile )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInPercentile == REAL_DEFAULT ) {
         optInPercentile = 5e1;
      } else if( !(optInPercentile >= 0e0 && optInPercentile <= 1e2) ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode PERCENTILE_Impl( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            double optInPercentile,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int rank = 0;
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
      if( optInPercentile == REAL_DEFAULT ) {
         optInPercentile = 5e1;
      } else if( !(optInPercentile >= 0e0 && optInPercentile <= 1e2) ) {
         return RetCode.BadParam;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. */
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
      /* Keep the multiply left of the divide. (P*n)/100 reproduces exact integer
       * arithmetic; P/100 is inexact in binary64 and lands the product just above
       * an integer, one order statistic too high, at exactly the round
       * percentages a caller types.
       */
      rank = (int)Math.ceil(optInPercentile * (double)optInTimePeriod / 100.0);
      if( rank < 1 ) {
         rank = 1;
      }
      if( rank > optInTimePeriod ) {
         rank = optInTimePeriod;
      }
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
       * Every buffer store sits BELOW the output store on purpose: deriving the
       * whole answer read-only above it is what lets the streaming peek frame drop
       * the state update rather than shadow a shift loop, which it cannot do.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( rank - 1 < pos ) {
            result = sorted[rank - 1];
         } else if( rank - 1 == pos ) {
            result = newValue;
         } else {
            result = sorted[rank - 2];
         }
         outReal[outIdx] = result;
         outIdx += 1;
         /* Shifting only the strictly greater entries leaves equal values in
          * insertion order, which is age order -- that is what lets the delete
          * below evict the oldest of a run by value alone, with no slot array.
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
   RetCode PERCENTILE_Impl( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            double optInPercentile,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int rank = 0;
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
      if( optInPercentile == REAL_DEFAULT ) {
         optInPercentile = 5e1;
      } else if( !(optInPercentile >= 0e0 && optInPercentile <= 1e2) ) {
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
      rank = (int)Math.ceil(optInPercentile * (double)optInTimePeriod / 100.0);
      if( rank < 1 ) {
         rank = 1;
      }
      if( rank > optInTimePeriod ) {
         rank = optInTimePeriod;
      }
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
         if( rank - 1 < pos ) {
            result = sorted[rank - 1];
         } else if( rank - 1 == pos ) {
            result = newValue;
         } else {
            result = sorted[rank - 2];
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
    * Rolling percentile by the nearest-rank method: sort the trailing window
    * ascending and report the value whose 1-based ordinal rank is the P-th
    * percentile of the window size. The result is always a value that actually
    * occurred in the window, never an interpolation, so it stays on the price
    * scale and never invents a level the series never traded at. At P = 50 with
    * an odd window it is the rolling median; at the extremes it degenerates to
    * the rolling minimum and maximum.
    * <p><b>Formula</b>
    * <pre>{@code
    * $W_t = \operatorname{sort}(x_{t-N+1}, \dots, x_t)$; $k = \left\lceil \frac{P \cdot N}{100} \right\rceil$ clamped to $[1, N]$; $PERCENTILE_t = W_t[k]$ (N = optInTimePeriod, P = optInPercentile, $W_t[1]$ the smallest)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The nearest-rank method is one of several incompatible percentile conventions. The linear-interpolation family (Hyndman &amp; Fan type 7, the default of most statistical packages, and TradingView's {@code ta.percentile_linear_interpolation}) reports a weighted blend of two neighbouring order statistics and can emit a value that never occurred. That is a different indicator, not a mode of this one: PERCENTILE's parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it later without changing the function's arity.</li>
    * <li>Every input value in the window must be finite. A NaN makes every comparison against it false, which breaks the ordering the rank index is read from.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PERCENTILE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to take the percentile of.
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentile Percentage position within the sorted window
    *        (default 50; range 0..100; {@code -4e37} selects the default).
    * @param outReal The value at the requested rank within the trailing window.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MEDPRICE
    * @see Core#STDDEV
    */
   public OutRange PERCENTILE( int startIdx,
                               int endIdx,
                               double inReal[],
                               int optInTimePeriod,
                               double optInPercentile,
                               double outReal[] )
   {
      requireIndexRange("PERCENTILE", startIdx, endIdx);
      int guardStart = clampedStart("PERCENTILE", startIdx, PERCENTILE_Lookback(optInTimePeriod, optInPercentile));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTILE", "inReal", inReal, guardInLen);
      requireLength("PERCENTILE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PERCENTILE_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PERCENTILE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling percentile by the nearest-rank method: sort the trailing window
    * ascending and report the value whose 1-based ordinal rank is the P-th
    * percentile of the window size. The result is always a value that actually
    * occurred in the window, never an interpolation, so it stays on the price
    * scale and never invents a level the series never traded at. At P = 50 with
    * an odd window it is the rolling median; at the extremes it degenerates to
    * the rolling minimum and maximum.
    * <p><b>Formula</b>
    * <pre>{@code
    * $W_t = \operatorname{sort}(x_{t-N+1}, \dots, x_t)$; $k = \left\lceil \frac{P \cdot N}{100} \right\rceil$ clamped to $[1, N]$; $PERCENTILE_t = W_t[k]$ (N = optInTimePeriod, P = optInPercentile, $W_t[1]$ the smallest)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The nearest-rank method is one of several incompatible percentile conventions. The linear-interpolation family (Hyndman &amp; Fan type 7, the default of most statistical packages, and TradingView's {@code ta.percentile_linear_interpolation}) reports a weighted blend of two neighbouring order statistics and can emit a value that never occurred. That is a different indicator, not a mode of this one: PERCENTILE's parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it later without changing the function's arity.</li>
    * <li>Every input value in the window must be finite. A NaN makes every comparison against it false, which breaks the ordering the rank index is read from.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PERCENTILE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to take the percentile of.
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentile Percentage position within the sorted window
    *        (default 50; range 0..100; {@code -4e37} selects the default).
    * @param outReal The value at the requested rank within the trailing window.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MEDPRICE
    * @see Core#STDDEV
    */
   public OutRange PERCENTILE( int startIdx,
                               int endIdx,
                               float inReal[],
                               int optInTimePeriod,
                               double optInPercentile,
                               double outReal[] )
   {
      requireIndexRange("PERCENTILE", startIdx, endIdx);
      int guardStart = clampedStart("PERCENTILE", startIdx, PERCENTILE_Lookback(optInTimePeriod, optInPercentile));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTILE", "inReal", inReal, guardInLen);
      requireLength("PERCENTILE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PERCENTILE_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PERCENTILE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PERCENTILE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PERCENTILE} over the same series.
    * Open with {@link Core#percentileOpen}; there is no close — the handle is
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
   public static final class PercentileStream {
      Core core;
      int optInTimePeriod;
      double optInPercentile;
      int lookbackTotal;
      int rank;
      int ring_Idx;
      int maxIdx_ring;
      int sorted_Idx;
      int maxIdx_sorted;
      int cbSize_ring;
      double[] cb_ring;
      int cbSize_sorted;
      double[] cb_sorted;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      PercentileStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PERCENTILE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PercentileStream( PercentileStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInPercentile = other.optInPercentile;
         this.lookbackTotal = other.lookbackTotal;
         this.rank = other.rank;
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
            throw new TaLibArgumentException("PERCENTILE update: BadParam", RetCode.BadParam);
         }
         core.percentileStepImpl(this, inReal);
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
         requireArgument("PERCENTILE updateAndFill", "inReal", inReal);
         requireArgument("PERCENTILE updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("PERCENTILE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("PERCENTILE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.percentileStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("PERCENTILE peek: BadParam", RetCode.BadParam);
         PercentileStream sp = this;
         double newValue = 0.0;
         double result = 0.0;
         int pos = 0;
         double cur_outReal = 0.0;
         newValue = inReal;
         pos = 0;
         while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( sp.rank - 1 < pos ) {
            result = sp.cb_sorted[sp.rank - 1];
         } else if( sp.rank - 1 == pos ) {
            result = newValue;
         } else {
            result = sp.cb_sorted[sp.rank - 2];
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
      public PercentileStream clone() {
         return new PercentileStream(this);
      }
   }
   void percentileStepImpl( PercentileStream sp, double inReal )
   {
      double newValue = 0.0;
      double oldValue = 0.0;
      double result = 0.0;
      int j = 0;
      int pos = 0;
      newValue = inReal;
      pos = 0;
      while( pos < sp.lookbackTotal && sp.cb_sorted[pos] <= newValue ) {
         pos += 1;
      }
      if( sp.rank - 1 < pos ) {
         result = sp.cb_sorted[sp.rank - 1];
      } else if( sp.rank - 1 == pos ) {
         result = newValue;
      } else {
         result = sp.cb_sorted[sp.rank - 2];
      }
      sp.cur_outReal = result;
      /* Shifting only the strictly greater entries leaves equal values in
       * insertion order, which is age order -- that is what lets the delete
       * below evict the oldest of a run by value alone, with no slot array.
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
   private RetCode percentileOpenImpl( PercentileStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInPercentile, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double newValue = 0;
      double oldValue = 0;
      double result = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int pos = 0;
      int nbSorted = 0;
      int rank = 0;
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
      if( optInPercentile == REAL_DEFAULT ) {
         optInPercentile = 5e1;
      } else if( !(optInPercentile >= 0e0 && optInPercentile <= 1e2) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* The window is carried twice: "ring" by age, "sorted" by value. */
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
      /* Keep the multiply left of the divide. (P*n)/100 reproduces exact integer
       * arithmetic; P/100 is inexact in binary64 and lands the product just above
       * an integer, one order statistic too high, at exactly the round
       * percentages a caller types.
       */
      rank = (int)Math.ceil(optInPercentile * (double)optInTimePeriod / 100.0);
      if( rank < 1 ) {
         rank = 1;
      }
      if( rank > optInTimePeriod ) {
         rank = optInTimePeriod;
      }
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
       * Every buffer store sits BELOW the output store on purpose: deriving the
       * whole answer read-only above it is what lets the streaming peek frame drop
       * the state update rather than shadow a shift loop, which it cannot do.
       */
      outIdx = 0;
      do {
         newValue = inReal[i];
         pos = 0;
         while( pos < lookbackTotal && sorted[pos] <= newValue ) {
            pos += 1;
         }
         if( rank - 1 < pos ) {
            result = sorted[rank - 1];
         } else if( rank - 1 == pos ) {
            result = newValue;
         } else {
            result = sorted[rank - 2];
         }
         outReal[outIdx * outStride] = result;
         outIdx += 1;
         /* Shifting only the strictly greater entries leaves equal values in
          * insertion order, which is age order -- that is what lets the delete
          * below evict the oldest of a run by value alone, with no slot array.
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
      sp.optInPercentile = optInPercentile;
      sp.lookbackTotal = lookbackTotal;
      sp.rank = rank;
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
   /* percentileOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PercentileStream percentileOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInPercentile, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PercentileStream sp = new PercentileStream(this);
      RetCode retCode = percentileOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PERCENTILE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PERCENTILE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PERCENTILE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind percentileOpen (composition seam). */
   PercentileStream percentileOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInPercentile )
   {
      PercentileStream sp = new PercentileStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = percentileOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PERCENTILE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PERCENTILE open: internal error", retCode);
      }
      throw new TaLibArgumentException("PERCENTILE open: " + retCode, retCode);
   }
   /**
    * Open a live PERCENTILE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PERCENTILE} at that bar.
    * <p>The history must hold at least {@code PERCENTILE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PercentileStream percentileOpen( double inReal[], int optInTimePeriod, double optInPercentile )
   {
      requireArgument("PERCENTILE open", "inReal", inReal);
      requireHistory("PERCENTILE open", inReal.length);
      return percentileOpenInternal(inReal, 0, optInTimePeriod, optInPercentile);
   }
   /**
    * {@link Core#percentileOpen} that also fills the output array(s) bit-identically
    * to {@link Core#PERCENTILE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PercentileStream#outRange()}.
    */
   public PercentileStream percentileOpenAndFill( double inReal[], int optInTimePeriod, double optInPercentile, double outReal[] )
   {
      requireArgument("PERCENTILE openAndFill", "inReal", inReal);
      requireHistory("PERCENTILE openAndFill", inReal.length);
      int guardOutLen = openFillCount("PERCENTILE openAndFill", inReal.length, PERCENTILE_Lookback(optInTimePeriod, optInPercentile));
      requireLength("PERCENTILE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("PERCENTILE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return percentileOpenAndFillInternal(inReal, 0, optInTimePeriod, optInPercentile, outBegIdx, outNBElement, outReal);
   }
