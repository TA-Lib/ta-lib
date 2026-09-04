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
 *  090426 MF,CC  Initial version (#346).
 */

   /**
    * Number of leading input bars {@link Core#VHF} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing closes spanned by the range
    *        (default 28; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int VHF_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 28;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode VHF_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumChange = 0;
      double prev = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 28;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = VHF_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* The two windows are NOT co-terminal: the extrema span the
          * optInTimePeriod newest closes, while the optInTimePeriod changes reach
          * one bar further back. That is what makes the lookback optInTimePeriod
          * rather than optInTimePeriod-1.
          */
         highest = inReal[today];
         lowest = highest;
         prev = highest;
         sumChange = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            tempReal = inReal[today - i];
            if( i < optInTimePeriod ) {
               sumChange += Math.abs(tempReal - prev);
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            prev = tempReal;
         }
         /* A fresh sum of non-negative magnitudes is exactly zero only on an
          * exactly flat window, which forces highest-lowest to zero too. Guard on
          * exact zero, never an epsilon band: a band carries the quote unit and
          * zeroes the filter for anything priced under it (issue #253).
          */
         if( sumChange > 0.0 ) {
            outReal[outIdx] = (highest - lowest) / sumChange;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode VHF_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumChange = 0;
      double prev = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 28;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = VHF_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         highest = (double)inReal[today];
         lowest = highest;
         prev = highest;
         sumChange = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            tempReal = (double)inReal[today - i];
            if( i < optInTimePeriod ) {
               sumChange += Math.abs(tempReal - prev);
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            prev = tempReal;
         }
         if( sumChange > 0.0 ) {
            outReal[outIdx] = (highest - lowest) / sumChange;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Vertical Horizontal Filter: Adam White's trend-versus-range filter, the
    * range a window covered divided by the path it actually travelled. Bounded
    * in [0,1]. Values near 1 mean the market covered most of its path in one
    * direction (trending); values near 0 mean it retraced repeatedly and went
    * nowhere (choppy). Like ADX it measures trend *strength*, not direction,
    * but it uses no smoothing and carries no recursion. A common use is regime
    * selection: run trend-following logic while VHF is high, oscillator logic
    * while it is low.
    * <p><b>Formula</b>
    * <pre>{@code
    * num = MAX(C[t-optInTimePeriod+1..t]) - MIN(C[t-optInTimePeriod+1..t]), the range spanned by the `optInTimePeriod` most recent closes. den = SUM( |C[j] - C[j-1]| ) for j = t-optInTimePeriod+1 .. t, the total absolute movement over the same number of changes, which therefore reaches one close further back. VHF = num / den.
    * The two windows are deliberately not co-terminal: the extrema span `optInTimePeriod` closes, the changes consume one more. Because `num` is the distance between two points the changes connect, `num <= den` always, so the result never leaves [0,1].
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose closes are all identical has no vertical movement and no horizontal movement. VHF reports 0 there. Other libraries differ: Tulip Indicators leaves the division unguarded and emits NaN, pandas-ta-classic perturbs the numerator and emits +Inf.</li>
    * <li>Adam White later described an 18-bar VHF smoothed by a 6-bar moving average. That variant is not implemented here; apply a moving average to {@code outReal} to obtain it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VHF_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Number of trailing closes spanned by the range
    *        (default 28; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Vertical Horizontal Filter value. Must hold at least
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
    * @see Core#ADX
    * @see Core#CMO
    * @see Core#CMOU
    */
   public OutRange VHF( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("VHF", startIdx, endIdx);
      int guardStart = clampedStart("VHF", startIdx, VHF_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VHF", "inReal", inReal, guardInLen);
      requireLength("VHF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VHF_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VHF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Vertical Horizontal Filter: Adam White's trend-versus-range filter, the
    * range a window covered divided by the path it actually travelled. Bounded
    * in [0,1]. Values near 1 mean the market covered most of its path in one
    * direction (trending); values near 0 mean it retraced repeatedly and went
    * nowhere (choppy). Like ADX it measures trend *strength*, not direction,
    * but it uses no smoothing and carries no recursion. A common use is regime
    * selection: run trend-following logic while VHF is high, oscillator logic
    * while it is low.
    * <p><b>Formula</b>
    * <pre>{@code
    * num = MAX(C[t-optInTimePeriod+1..t]) - MIN(C[t-optInTimePeriod+1..t]), the range spanned by the `optInTimePeriod` most recent closes. den = SUM( |C[j] - C[j-1]| ) for j = t-optInTimePeriod+1 .. t, the total absolute movement over the same number of changes, which therefore reaches one close further back. VHF = num / den.
    * The two windows are deliberately not co-terminal: the extrema span `optInTimePeriod` closes, the changes consume one more. Because `num` is the distance between two points the changes connect, `num <= den` always, so the result never leaves [0,1].
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose closes are all identical has no vertical movement and no horizontal movement. VHF reports 0 there. Other libraries differ: Tulip Indicators leaves the division unguarded and emits NaN, pandas-ta-classic perturbs the numerator and emits +Inf.</li>
    * <li>Adam White later described an 18-bar VHF smoothed by a 6-bar moving average. That variant is not implemented here; apply a moving average to {@code outReal} to obtain it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VHF_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Number of trailing closes spanned by the range
    *        (default 28; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Vertical Horizontal Filter value. Must hold at least
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
    * @see Core#ADX
    * @see Core#CMO
    * @see Core#CMOU
    */
   public OutRange VHF( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("VHF", startIdx, endIdx);
      int guardStart = clampedStart("VHF", startIdx, VHF_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VHF", "inReal", inReal, guardInLen);
      requireLength("VHF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VHF_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VHF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VHF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#VHF} over the same series.
    * Open with {@link Core#vhfOpen}; there is no close — the handle is
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
   public static final class VhfStream {
      Core core;
      int optInTimePeriod;
      int winPos_i;
      int winCap_i;
      double[] win_i_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      VhfStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#VHF} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      VhfStream( VhfStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inReal = other.win_i_inReal.clone();
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
            throw new TaLibArgumentException("VHF update: BadParam", RetCode.BadParam);
         }
         core.vhfStepImpl(this, inReal);
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
         requireArgument("VHF updateAndFill", "inReal", inReal);
         requireArgument("VHF updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("VHF updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("VHF updateAndFill: BadParam", RetCode.BadParam);
            }
            core.vhfStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("VHF peek: BadParam", RetCode.BadParam);
         VhfStream sp = this;
         int i = 0;
         double highest = 0.0;
         double lowest = 0.0;
         double sumChange = 0.0;
         double prev = 0.0;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inReal;
         /* The two windows are NOT co-terminal: the extrema span the
          * optInTimePeriod newest closes, while the optInTimePeriod changes reach
          * one bar further back. That is what makes the lookback optInTimePeriod
          * rather than optInTimePeriod-1.
          */
         highest = inReal;
         lowest = highest;
         prev = highest;
         sumChange = 0.0;
         for( i = sp.optInTimePeriod; i >= 0; i -= 1 ) {
            tempReal = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
            if( i < sp.optInTimePeriod ) {
               sumChange += Math.abs(tempReal - prev);
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            prev = tempReal;
         }
         /* A fresh sum of non-negative magnitudes is exactly zero only on an
          * exactly flat window, which forces highest-lowest to zero too. Guard on
          * exact zero, never an epsilon band: a band carries the quote unit and
          * zeroes the filter for anything priced under it (issue #253).
          */
         if( sumChange > 0.0 ) {
            cur_outReal = (highest - lowest) / sumChange;
         } else {
            cur_outReal = 0.0;
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
      public VhfStream clone() {
         return new VhfStream(this);
      }
   }
   void vhfStepImpl( VhfStream sp, double inReal )
   {
      int i = 0;
      double highest = 0.0;
      double lowest = 0.0;
      double sumChange = 0.0;
      double prev = 0.0;
      double tempReal = 0.0;
      sp.win_i_inReal[sp.winPos_i] = inReal;
      /* The two windows are NOT co-terminal: the extrema span the
       * optInTimePeriod newest closes, while the optInTimePeriod changes reach
       * one bar further back. That is what makes the lookback optInTimePeriod
       * rather than optInTimePeriod-1.
       */
      highest = inReal;
      lowest = highest;
      prev = highest;
      sumChange = 0.0;
      for( i = sp.optInTimePeriod; i >= 0; i -= 1 ) {
         tempReal = sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         if( i < sp.optInTimePeriod ) {
            sumChange += Math.abs(tempReal - prev);
            if( tempReal > highest ) {
               highest = tempReal;
            }
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         prev = tempReal;
      }
      /* A fresh sum of non-negative magnitudes is exactly zero only on an
       * exactly flat window, which forces highest-lowest to zero too. Guard on
       * exact zero, never an epsilon band: a band carries the quote unit and
       * zeroes the filter for anything priced under it (issue #253).
       */
      if( sumChange > 0.0 ) {
         sp.cur_outReal = (highest - lowest) / sumChange;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode vhfOpenImpl( VhfStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double highest = 0;
      double lowest = 0;
      double sumChange = 0;
      double prev = 0;
      double tempReal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 28;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = VHF_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* The two windows are NOT co-terminal: the extrema span the
          * optInTimePeriod newest closes, while the optInTimePeriod changes reach
          * one bar further back. That is what makes the lookback optInTimePeriod
          * rather than optInTimePeriod-1.
          */
         highest = inReal[today];
         lowest = highest;
         prev = highest;
         sumChange = 0.0;
         for( i = optInTimePeriod; i >= 0; i -= 1 ) {
            tempReal = inReal[today - i];
            if( i < optInTimePeriod ) {
               sumChange += Math.abs(tempReal - prev);
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            prev = tempReal;
         }
         /* A fresh sum of non-negative magnitudes is exactly zero only on an
          * exactly flat window, which forces highest-lowest to zero too. Guard on
          * exact zero, never an epsilon band: a band carries the quote unit and
          * zeroes the filter for anything priced under it (issue #253).
          */
         if( sumChange > 0.0 ) {
            outReal[outIdx * outStride] = (highest - lowest) / sumChange;
         } else {
            outReal[outIdx * outStride] = 0.0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inReal = new double[cap_i];
      System.arraycopy(inReal, historyLen - cap_i, capWin_i_inReal, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inReal = capWin_i_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* vhfOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VhfStream vhfOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VhfStream sp = new VhfStream(this);
      RetCode retCode = vhfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VHF openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VHF openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VHF openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind vhfOpen (composition seam). */
   VhfStream vhfOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      VhfStream sp = new VhfStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = vhfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VHF open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VHF open: internal error", retCode);
      }
      throw new TaLibArgumentException("VHF open: " + retCode, retCode);
   }
   /**
    * Open a live VHF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#VHF} at that bar.
    * <p>The history must hold at least {@code VHF_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public VhfStream vhfOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("VHF open", "inReal", inReal);
      requireHistory("VHF open", inReal.length);
      return vhfOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#vhfOpen} that also fills the output array(s) bit-identically
    * to {@link Core#VHF} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link VhfStream#outRange()}.
    */
   public VhfStream vhfOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("VHF openAndFill", "inReal", inReal);
      requireHistory("VHF openAndFill", inReal.length);
      int guardOutLen = openFillCount("VHF openAndFill", inReal.length, VHF_Lookback(optInTimePeriod));
      requireLength("VHF openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("VHF openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return vhfOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
