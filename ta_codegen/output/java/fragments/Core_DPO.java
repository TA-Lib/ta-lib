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
 *  090426 MF,CC  Initial version (#363).
 */

   /**
    * Number of leading input bars {@link Core#DPO} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars spanned by the moving average being
    *        removed; the displacement is derived from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int DPO_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* The max is load-bearing, not cosmetic. The displaced read reaches back
       * optInTimePeriod/2+1 bars, which exceeds the moving average's own
       * optInTimePeriod-1 at a period of 2, and a bare optInTimePeriod-1 would
       * then read inReal[-1].
       */
      return Math.max(optInTimePeriod - 1, optInTimePeriod / 2 + 1) ;

   }
   RetCode DPO_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double periodTotal = 0;
      double tempReal = 0;
      double dispVal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int dispIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The running sum is accumulated, snapshot and divided in exactly the order
       * ta_codegen/input/sma/sma.c uses, so this fused loop is bit-for-bit equal
       * to a TA_SMA anchored at the same startIdx. That equality is what makes
       * the composite differential in test_dpo.c a memcmp instead of a tolerance
       * argument, and it is lost by dividing once into a reciprocal or by
       * reordering the add/snapshot/subtract.
       */
      lookbackTotal = DPO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - (optInTimePeriod - 1);
      dispIdx = startIdx - (optInTimePeriod / 2 + 1);
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += inReal[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += inReal[i];
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= inReal[trailingIdx];
         trailingIdx = trailingIdx + 1;
         /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
          * displaced one whenever startIdx equals the displacement, the trailing
          * one whenever startIdx sits at the lookback -- so a store hoisted above
          * them would read back what it had just overwritten when the caller
          * aliases outReal over inReal.
          */
         dispVal = inReal[dispIdx];
         dispIdx = dispIdx + 1;
         outReal[outIdx] = dispVal - tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode DPO_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double periodTotal = 0;
      double tempReal = 0;
      double dispVal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int dispIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = DPO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - (optInTimePeriod - 1);
      dispIdx = startIdx - (optInTimePeriod / 2 + 1);
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += (double)inReal[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += (double)inReal[i];
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= (double)inReal[trailingIdx];
         trailingIdx = trailingIdx + 1;
         dispVal = (double)inReal[dispIdx];
         dispIdx = dispIdx + 1;
         outReal[outIdx] = dispVal - tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Detrended Price Oscillator: the price a half-cycle back, less the moving
    * average that spans the cycle. Removing the average removes the trend,
    * leaving the shorter oscillation that the trend was hiding. It crosses zero
    * as price crosses its own average, so peaks and troughs mark the cycle
    * rather than the direction of the market. The distance between successive
    * peaks estimates the cycle length, and the amplitude is in price units, so
    * it is comparable across time only for one instrument.
    * <p><b>Formula</b>
    * <pre>{@code
    * Let `t = optInTimePeriod / 2 + 1`, an integer division, so a period and its odd successor share the same displacement.
    * DPO[i] = P[i - t] - SMA(P, optInTimePeriod)[i]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The value is emitted at the bar whose moving average produced it. Charting packages usually draw it {@code t} bars to the left instead, which is a plotting convention rather than a different series; a caller wanting that view shifts {@code outReal} itself.</li>
    * <li>A causal variant, {@code P[i] - SMA(P, optInTimePeriod)[i - t]}, displaces the average instead of the price. It is a genuinely different series, not a re-indexing of this one, and is not implemented here.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Number of bars spanned by the moving average being
    *        removed; the displacement is derived from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Detrended Price Oscillator value, in the units of the
    *        input. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SMA
    * @see Core#MOM
    * @see Core#APO
    */
   public OutRange DPO( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("DPO", startIdx, endIdx);
      int guardStart = clampedStart("DPO", startIdx, DPO_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DPO", "inReal", inReal, guardInLen);
      requireLength("DPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DPO_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("DPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Detrended Price Oscillator: the price a half-cycle back, less the moving
    * average that spans the cycle. Removing the average removes the trend,
    * leaving the shorter oscillation that the trend was hiding. It crosses zero
    * as price crosses its own average, so peaks and troughs mark the cycle
    * rather than the direction of the market. The distance between successive
    * peaks estimates the cycle length, and the amplitude is in price units, so
    * it is comparable across time only for one instrument.
    * <p><b>Formula</b>
    * <pre>{@code
    * Let `t = optInTimePeriod / 2 + 1`, an integer division, so a period and its odd successor share the same displacement.
    * DPO[i] = P[i - t] - SMA(P, optInTimePeriod)[i]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The value is emitted at the bar whose moving average produced it. Charting packages usually draw it {@code t} bars to the left instead, which is a plotting convention rather than a different series; a caller wanting that view shifts {@code outReal} itself.</li>
    * <li>A causal variant, {@code P[i] - SMA(P, optInTimePeriod)[i - t]}, displaces the average instead of the price. It is a genuinely different series, not a re-indexing of this one, and is not implemented here.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Number of bars spanned by the moving average being
    *        removed; the displacement is derived from it (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Detrended Price Oscillator value, in the units of the
    *        input. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SMA
    * @see Core#MOM
    * @see Core#APO
    */
   public OutRange DPO( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("DPO", startIdx, endIdx);
      int guardStart = clampedStart("DPO", startIdx, DPO_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DPO", "inReal", inReal, guardInLen);
      requireLength("DPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DPO_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("DPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live DPO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#DPO} over the same series.
    * Open with {@link Core#dpoOpen}; there is no close — the handle is
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
   public static final class DpoStream {
      Core core;
      int optInTimePeriod;
      double periodTotal;
      int ringPos_dispIdx;
      int ringCap_dispIdx;
      double[] ring_dispIdx_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      DpoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#DPO} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      DpoStream( DpoStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotal = other.periodTotal;
         this.ringPos_dispIdx = other.ringPos_dispIdx;
         this.ringCap_dispIdx = other.ringCap_dispIdx;
         this.ring_dispIdx_inReal = other.ring_dispIdx_inReal.clone();
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
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
            throw new TaLibArgumentException("DPO update: BadParam", RetCode.BadParam);
         }
         core.dpoStepImpl(this, inReal);
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
         requireArgument("DPO updateAndFill", "inReal", inReal);
         requireArgument("DPO updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("DPO updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("DPO updateAndFill: BadParam", RetCode.BadParam);
            }
            core.dpoStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("DPO peek: BadParam", RetCode.BadParam);
         DpoStream sp = this;
         double tempReal = 0.0;
         double dispVal = 0.0;
         double cur_outReal = 0.0;
         double periodTotal = sp.periodTotal;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( sp.ringCap_dispIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot1 = 0;
            pkVal1 = inReal;
         }
         periodTotal += inReal;
         tempReal = periodTotal;
         periodTotal -= (sp.ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal1;
         /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
          * displaced one whenever startIdx equals the displacement, the trailing
          * one whenever startIdx sits at the lookback -- so a store hoisted above
          * them would read back what it had just overwritten when the caller
          * aliases outReal over inReal.
          */
         dispVal = (sp.ringPos_dispIdx != pkSlot0) ? sp.ring_dispIdx_inReal[sp.ringPos_dispIdx] : pkVal0;
         cur_outReal = dispVal - tempReal / (double)sp.optInTimePeriod;
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
      public DpoStream clone() {
         return new DpoStream(this);
      }
   }
   void dpoStepImpl( DpoStream sp, double inReal )
   {
      double tempReal = 0.0;
      double dispVal = 0.0;
      if( sp.ringCap_dispIdx == 0 ) {
         sp.ring_dispIdx_inReal[0] = inReal;
      }
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      sp.periodTotal += inReal;
      tempReal = sp.periodTotal;
      sp.periodTotal -= sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
       * displaced one whenever startIdx equals the displacement, the trailing
       * one whenever startIdx sits at the lookback -- so a store hoisted above
       * them would read back what it had just overwritten when the caller
       * aliases outReal over inReal.
       */
      dispVal = sp.ring_dispIdx_inReal[sp.ringPos_dispIdx];
      sp.cur_outReal = dispVal - tempReal / (double)sp.optInTimePeriod;
      sp.ring_dispIdx_inReal[sp.ringPos_dispIdx] = inReal;
      sp.ringPos_dispIdx = sp.ringPos_dispIdx + 1;
      if( sp.ringPos_dispIdx >= sp.ringCap_dispIdx ) {
         sp.ringPos_dispIdx = 0;
      }
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode dpoOpenImpl( DpoStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double periodTotal = 0;
      double tempReal = 0;
      double dispVal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int dispIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* The running sum is accumulated, snapshot and divided in exactly the order
       * ta_codegen/input/sma/sma.c uses, so this fused loop is bit-for-bit equal
       * to a TA_SMA anchored at the same startIdx. That equality is what makes
       * the composite differential in test_dpo.c a memcmp instead of a tolerance
       * argument, and it is lost by dividing once into a reciprocal or by
       * reordering the add/snapshot/subtract.
       */
      lookbackTotal = DPO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - (optInTimePeriod - 1);
      dispIdx = startIdx - (optInTimePeriod / 2 + 1);
      i = trailingIdx;
      while( i < startIdx ) {
         periodTotal += inReal[i];
         i = i + 1;
      }
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += inReal[i];
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= inReal[trailingIdx];
         trailingIdx = trailingIdx + 1;
         /* Both reads precede the store. Either cursor can EQUAL outIdx -- the
          * displaced one whenever startIdx equals the displacement, the trailing
          * one whenever startIdx sits at the lookback -- so a store hoisted above
          * them would read back what it had just overwritten when the caller
          * aliases outReal over inReal.
          */
         dispVal = inReal[dispIdx];
         dispIdx = dispIdx + 1;
         outReal[outIdx * outStride] = dispVal - tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_dispIdx = i - dispIdx;
      if( cap_dispIdx < 0 || cap_dispIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_dispIdx = (cap_dispIdx > 0)? cap_dispIdx : 1;
      double[] capRing_dispIdx_inReal = new double[allocN_dispIdx];
      System.arraycopy(inReal, historyLen - cap_dispIdx, capRing_dispIdx_inReal, 0, cap_dispIdx);
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.periodTotal = periodTotal;
      sp.ringPos_dispIdx = 0;
      sp.ringCap_dispIdx = cap_dispIdx;
      sp.ring_dispIdx_inReal = capRing_dispIdx_inReal;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* dpoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   DpoStream dpoOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      DpoStream sp = new DpoStream(this);
      RetCode retCode = dpoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("DPO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("DPO openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("DPO openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind dpoOpen (composition seam). */
   DpoStream dpoOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      DpoStream sp = new DpoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = dpoOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("DPO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("DPO open: internal error", retCode);
      }
      throw new TaLibArgumentException("DPO open: " + retCode, retCode);
   }
   /**
    * Open a live DPO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#DPO} at that bar.
    * <p>The history must hold at least {@code DPO_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public DpoStream dpoOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("DPO open", "inReal", inReal);
      requireHistory("DPO open", inReal.length);
      return dpoOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#dpoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#DPO} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link DpoStream#outRange()}.
    */
   public DpoStream dpoOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("DPO openAndFill", "inReal", inReal);
      requireHistory("DPO openAndFill", inReal.length);
      int guardOutLen = openFillCount("DPO openAndFill", inReal.length, DPO_Lookback(optInTimePeriod));
      requireLength("DPO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("DPO openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return dpoOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
