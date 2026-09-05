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
 *  090626 KL     First version (issue #361).
 */

   /**
    * Number of leading input bars {@link Core#ERI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the EMA of close (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ERI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* Exactly the EMA of close underneath: its lookback, unstable period
       * included, is this function's lookback.
       */
      return EMA_Lookback(optInTimePeriod) ;

   }
   RetCode ERI_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outBullPower[],
                     double outBearPower[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int i = 0;
      double prevMA = 0;
      double tempReal = 0;
      double k = 0;
      double tempHT = 0;
      double tempLT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outBullPower == outBearPower ) {
         return RetCode.BadParam ;
      }
      /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
       * the bar's extremes sit from one shared EMA of close.
       *
       *   Bull Power = High - EMA(Close, n)
       *   Bear Power = Low  - EMA(Close, n)
       *
       * One fused loop, not ema() + a combine map: a composed form cannot
       * stream (raw bar inputs are outside check_map_step's provenance), which
       * is the same reason ACCBANDS is fused. The EMA is ema.c's DEFAULT arm
       * op for op -- sequential seed sum from 0.0 then one divide, the
       * unstable-period warm-up consumed bar by bar -- so the differential
       * against shipped TA_EMA holds bitwise. No compatibility branch: the
       * Metastock arm is unreachable from three of the four backends, and a
       * new function honouring it would make C diverge from them (EFI/SMI
       * precedent).
       *
       * No division in the per-bar map: no 0/0, no NaN path (#112 by
       * construction). Bull >= Bear on every bar since high >= low.
       */
      lookbackTotal = ERI_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
       * exists there. At n == 1 the recursion below is fl(fl(x-prev)+prev),
       * which returns x only while consecutive closes stay within a factor of
       * two (Sterbenz), so without this arm `High - TA_EMA(Close, 1)` is not
       * what this function returns. The unstable period still delays the first
       * output, through the shared lookback above.
       */
      if( optInTimePeriod == 1 ) {
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            tempHT = inHigh[today];
            tempLT = inLow[today];
            tempReal = inClose[today];
            outBullPower[outIdx] = tempHT - tempReal;
            outBearPower[outIdx] = tempLT - tempReal;
            outIdx += 1;
            today += 1;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      k = 2.0 / ((double)optInTimePeriod + 1.0);
      /* Seed: ema.c's DEFAULT arm, op for op. */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inClose[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      /* The warm-up also consumes the EMA unstable period. */
      while( today <= startIdx ) {
         prevMA = Math.fma(inClose[today++] - prevMA, k, prevMA);
      }
      /* prevMA is the EMA at bar startIdx; today == startIdx + 1. Load the
       * extremes into temps BEFORE writing either output: with two outputs
       * over three inputs the caller may alias any pair, and the second write
       * must not read a clobbered bar.
       */
      tempHT = inHigh[startIdx];
      tempLT = inLow[startIdx];
      outBullPower[0] = tempHT - prevMA;
      outBearPower[0] = tempLT - prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(inClose[today] - prevMA, k, prevMA);
         tempHT = inHigh[today];
         tempLT = inLow[today];
         outBullPower[outIdx] = tempHT - prevMA;
         outBearPower[outIdx] = tempLT - prevMA;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ERI_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outBullPower[],
                     double outBearPower[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int i = 0;
      double prevMA = 0;
      double tempReal = 0;
      double k = 0;
      double tempHT = 0;
      double tempLT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outBullPower == outBearPower ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = ERI_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            tempHT = (double)inHigh[today];
            tempLT = (double)inLow[today];
            tempReal = (double)inClose[today];
            outBullPower[outIdx] = tempHT - tempReal;
            outBearPower[outIdx] = tempLT - tempReal;
            outIdx += 1;
            today += 1;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      k = 2.0 / ((double)optInTimePeriod + 1.0);
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inClose[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma((double)inClose[today++] - prevMA, k, prevMA);
      }
      tempHT = (double)inHigh[startIdx];
      tempLT = (double)inLow[startIdx];
      outBullPower[0] = tempHT - prevMA;
      outBearPower[0] = tempLT - prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma((double)inClose[today] - prevMA, k, prevMA);
         tempHT = (double)inHigh[today];
         tempLT = (double)inLow[today];
         outBullPower[outIdx] = tempHT - prevMA;
         outBearPower[outIdx] = tempLT - prevMA;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from
    * *Trading for a Living* (1993) — how far the bar's high and low sit from an
    * EMA of the close. Bulls strong enough to push the high above the average
    * read as positive Bull Power; bears dragging the low below it read as
    * negative Bear Power.
    * <p><b>Formula</b>
    * <pre>{@code
    * `Bull Power = High − EMA(Close, n)` and `Bear Power = Low − EMA(Close, n)`, both lines against the **same** EMA. Bull ≥ Bear on every bar since high ≥ low. TradingView's built-in *Bull Bear Power* — which its own support page calls "otherwise known as the Elder-Ray Index" — plots only the sum of the two, not the pair; StockCharts, TC2000 and pandas-ta all ship the two lines.
    * Because the underlying average is an [`EMA`](/functions/ema), ERI inherits its unstable period: the warm-up consumes `TA_GetUnstablePeriod(TA_FUNC_UNST_EMA)` extra bars, exactly as `EMA` itself does.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>ERI is a cancelling difference: near the zero crossings that carry its signal, tiny EMA discrepancies are amplified without bound in relative terms. Compare against external values with an absolute tolerance.</li>
    * <li>No MAType parameter: every canonical source fixes the EMA, and a selectable average would invent a variant nobody ships.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ERI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Number of bars in the EMA of close (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outBullPower High minus the EMA of close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outBearPower Low minus the EMA of close. Must hold at least
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
    * @see Core#EMA
    * @see Core#EFI
    * @see Core#MACD
    */
   public OutRange ERI( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        double outBullPower[],
                        double outBearPower[] )
   {
      requireIndexRange("ERI", startIdx, endIdx);
      int guardStart = clampedStart("ERI", startIdx, ERI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ERI", "inHigh", inHigh, guardInLen);
      requireLength("ERI", "inLow", inLow, guardInLen);
      requireLength("ERI", "inClose", inClose, guardInLen);
      requireLength("ERI", "outBullPower", outBullPower, guardOutLen);
      requireLength("ERI", "outBearPower", outBearPower, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ERI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower);
      if( retCode != RetCode.Success ) {
         throw failure("ERI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from
    * *Trading for a Living* (1993) — how far the bar's high and low sit from an
    * EMA of the close. Bulls strong enough to push the high above the average
    * read as positive Bull Power; bears dragging the low below it read as
    * negative Bear Power.
    * <p><b>Formula</b>
    * <pre>{@code
    * `Bull Power = High − EMA(Close, n)` and `Bear Power = Low − EMA(Close, n)`, both lines against the **same** EMA. Bull ≥ Bear on every bar since high ≥ low. TradingView's built-in *Bull Bear Power* — which its own support page calls "otherwise known as the Elder-Ray Index" — plots only the sum of the two, not the pair; StockCharts, TC2000 and pandas-ta all ship the two lines.
    * Because the underlying average is an [`EMA`](/functions/ema), ERI inherits its unstable period: the warm-up consumes `TA_GetUnstablePeriod(TA_FUNC_UNST_EMA)` extra bars, exactly as `EMA` itself does.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>ERI is a cancelling difference: near the zero crossings that carry its signal, tiny EMA discrepancies are amplified without bound in relative terms. Compare against external values with an absolute tolerance.</li>
    * <li>No MAType parameter: every canonical source fixes the EMA, and a selectable average would invent a variant nobody ships.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ERI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Number of bars in the EMA of close (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outBullPower High minus the EMA of close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outBearPower Low minus the EMA of close. Must hold at least
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
    * @see Core#EMA
    * @see Core#EFI
    * @see Core#MACD
    */
   public OutRange ERI( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        double outBullPower[],
                        double outBearPower[] )
   {
      requireIndexRange("ERI", startIdx, endIdx);
      int guardStart = clampedStart("ERI", startIdx, ERI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ERI", "inHigh", inHigh, guardInLen);
      requireLength("ERI", "inLow", inLow, guardInLen);
      requireLength("ERI", "inClose", inClose, guardInLen);
      requireLength("ERI", "outBullPower", outBullPower, guardOutLen);
      requireLength("ERI", "outBearPower", outBearPower, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ERI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower);
      if( retCode != RetCode.Success ) {
         throw failure("ERI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ERI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ERI} over the same series.
    * Open with {@link Core#eriOpen}; there is no close — the handle is
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
   public static final class EriStream {
      Core core;
      int optInTimePeriod;
      double prevMA;
      double k;
      double cur_outBullPower;
      double cur_outBearPower;
      int outRangeBegIdx;
      int outRangeCount;

      EriStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ERI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      EriStream( EriStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevMA = other.prevMA;
         this.k = other.k;
         this.cur_outBullPower = other.cur_outBullPower;
         this.cur_outBearPower = other.cur_outBearPower;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(EriOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, EriOut out ) {
         requireArgument("ERI update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("ERI update: BadParam", RetCode.BadParam);
         }
         core.eriStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.bullPower = this.cur_outBullPower;
         out.bearPower = this.cur_outBearPower;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outBullPower[], double outBearPower[] ) {
         requireArgument("ERI updateAndFill", "inHigh", inHigh);
         requireArgument("ERI updateAndFill", "inLow", inLow);
         requireArgument("ERI updateAndFill", "inClose", inClose);
         requireArgument("ERI updateAndFill", "outBullPower", outBullPower);
         requireArgument("ERI updateAndFill", "outBearPower", outBearPower);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outBullPower.length < barCount || outBearPower.length < barCount || (Object)outBullPower == (Object)inHigh || (Object)outBullPower == (Object)inLow || (Object)outBullPower == (Object)inClose || (Object)outBearPower == (Object)inHigh || (Object)outBearPower == (Object)inLow || (Object)outBearPower == (Object)inClose || (Object)outBullPower == (Object)outBearPower )
            throw new TaLibArgumentException("ERI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("ERI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.eriStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outBullPower[i] = this.cur_outBullPower;
            outBearPower[i] = this.cur_outBearPower;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inHigh, double inLow, double inClose, EriOut out ) {
         requireArgument("ERI peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ERI peek: BadParam", RetCode.BadParam);
         EriStream sp = this;
         double cur_outBullPower = 0.0;
         double cur_outBearPower = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            double tempReal = 0.0;
            double tempHT = 0.0;
            double tempLT = 0.0;
            tempHT = inHigh;
            tempLT = inLow;
            tempReal = inClose;
            cur_outBullPower = tempHT - tempReal;
            cur_outBearPower = tempLT - tempReal;
         } else {
            double tempHT = 0.0;
            double tempLT = 0.0;
            double prevMA = sp.prevMA;
            prevMA = Math.fma(inClose - prevMA, sp.k, prevMA);
            tempHT = inHigh;
            tempLT = inLow;
            cur_outBullPower = tempHT - prevMA;
            cur_outBearPower = tempLT - prevMA;
         }
         out.bullPower = cur_outBullPower;
         out.bearPower = cur_outBearPower;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( EriOut out ) {
         requireArgument("ERI value", "out", out);
         out.bullPower = this.cur_outBullPower;
         out.bearPower = this.cur_outBearPower;
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
      public EriStream clone() {
         return new EriStream(this);
      }
   }

   /**
    * The outputs of one ERI bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class EriOut {
      /** High minus the EMA of close. */
      public double bullPower;
      /** Low minus the EMA of close. */
      public double bearPower;
   }
   void eriStepImpl( EriStream sp, double inHigh, double inLow, double inClose )
   {
      if( sp.optInTimePeriod == 1 ) {
         double tempReal = 0.0;
         double tempHT = 0.0;
         double tempLT = 0.0;
         tempHT = inHigh;
         tempLT = inLow;
         tempReal = inClose;
         sp.cur_outBullPower = tempHT - tempReal;
         sp.cur_outBearPower = tempLT - tempReal;
      } else {
         double tempHT = 0.0;
         double tempLT = 0.0;
         sp.prevMA = Math.fma(inClose - sp.prevMA, sp.k, sp.prevMA);
         tempHT = inHigh;
         tempLT = inLow;
         sp.cur_outBullPower = tempHT - sp.prevMA;
         sp.cur_outBearPower = tempLT - sp.prevMA;
      }
   }
   private RetCode eriOpenImpl( EriStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outBullPower[], double outBearPower[], int outStride )
   {
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         int outIdx = 0;
         int today = 0;
         int lookbackTotal = 0;
         int i = 0;
         double prevMA = 0;
         double tempReal = 0;
         double k = 0;
         double tempHT = 0;
         double tempLT = 0;
         /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
          * the bar's extremes sit from one shared EMA of close.
          *
          *   Bull Power = High - EMA(Close, n)
          *   Bear Power = Low  - EMA(Close, n)
          *
          * One fused loop, not ema() + a combine map: a composed form cannot
          * stream (raw bar inputs are outside check_map_step's provenance), which
          * is the same reason ACCBANDS is fused. The EMA is ema.c's DEFAULT arm
          * op for op -- sequential seed sum from 0.0 then one divide, the
          * unstable-period warm-up consumed bar by bar -- so the differential
          * against shipped TA_EMA holds bitwise. No compatibility branch: the
          * Metastock arm is unreachable from three of the four backends, and a
          * new function honouring it would make C diverge from them (EFI/SMI
          * precedent).
          *
          * No division in the per-bar map: no 0/0, no NaN path (#112 by
          * construction). Bull >= Bear on every bar since high >= low.
          */
         lookbackTotal = ERI_Lookback(optInTimePeriod);
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
          * exists there. At n == 1 the recursion below is fl(fl(x-prev)+prev),
          * which returns x only while consecutive closes stay within a factor of
          * two (Sterbenz), so without this arm `High - TA_EMA(Close, 1)` is not
          * what this function returns. The unstable period still delays the first
          * output, through the shared lookback above.
          */
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            tempHT = inHigh[today];
            tempLT = inLow[today];
            tempReal = inClose[today];
            outBullPower[outIdx * outStride] = tempHT - tempReal;
            outBearPower[outIdx * outStride] = tempLT - tempReal;
            outIdx += 1;
            today += 1;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevMA = prevMA;
         sp.k = k;
         sp.cur_outBullPower = outBullPower[(outNBElement.value - 1) * outStride];
         sp.cur_outBearPower = outBearPower[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      } else {
         int outIdx = 0;
         int today = 0;
         int lookbackTotal = 0;
         int i = 0;
         double prevMA = 0;
         double tempReal = 0;
         double k = 0;
         double tempHT = 0;
         double tempLT = 0;
         /* Elder Ray Index (Alexander Elder, Trading for a Living, 1993): how far
          * the bar's extremes sit from one shared EMA of close.
          *
          *   Bull Power = High - EMA(Close, n)
          *   Bear Power = Low  - EMA(Close, n)
          *
          * One fused loop, not ema() + a combine map: a composed form cannot
          * stream (raw bar inputs are outside check_map_step's provenance), which
          * is the same reason ACCBANDS is fused. The EMA is ema.c's DEFAULT arm
          * op for op -- sequential seed sum from 0.0 then one divide, the
          * unstable-period warm-up consumed bar by bar -- so the differential
          * against shipped TA_EMA holds bitwise. No compatibility branch: the
          * Metastock arm is unreachable from three of the four backends, and a
          * new function honouring it would make C diverge from them (EFI/SMI
          * precedent).
          *
          * No division in the per-bar map: no 0/0, no NaN path (#112 by
          * construction). Bull >= Bear on every bar since high >= low.
          */
         lookbackTotal = ERI_Lookback(optInTimePeriod);
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* Period 1: ema.c's explicit copy arm, kept here for the same reason it
          * exists there. At n == 1 the recursion below is fl(fl(x-prev)+prev),
          * which returns x only while consecutive closes stay within a factor of
          * two (Sterbenz), so without this arm `High - TA_EMA(Close, 1)` is not
          * what this function returns. The unstable period still delays the first
          * output, through the shared lookback above.
          */
         k = 2.0 / ((double)optInTimePeriod + 1.0);
         /* Seed: ema.c's DEFAULT arm, op for op. */
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += inClose[today++];
         }
         prevMA = tempReal / optInTimePeriod;
         /* The warm-up also consumes the EMA unstable period. */
         while( today <= startIdx ) {
            prevMA = Math.fma(inClose[today++] - prevMA, k, prevMA);
         }
         /* prevMA is the EMA at bar startIdx; today == startIdx + 1. Load the
          * extremes into temps BEFORE writing either output: with two outputs
          * over three inputs the caller may alias any pair, and the second write
          * must not read a clobbered bar.
          */
         tempHT = inHigh[startIdx];
         tempLT = inLow[startIdx];
         outBullPower[0 * outStride] = tempHT - prevMA;
         outBearPower[0 * outStride] = tempLT - prevMA;
         outIdx = 1;
         while( today <= endIdx ) {
            prevMA = Math.fma(inClose[today] - prevMA, k, prevMA);
            tempHT = inHigh[today];
            tempLT = inLow[today];
            outBullPower[outIdx * outStride] = tempHT - prevMA;
            outBearPower[outIdx * outStride] = tempLT - prevMA;
            outIdx += 1;
            today += 1;
         }
         outBegIdx.value = startIdx;
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevMA = prevMA;
         sp.k = k;
         sp.cur_outBullPower = outBullPower[(outNBElement.value - 1) * outStride];
         sp.cur_outBearPower = outBearPower[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
   }
   /* eriOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   EriStream eriOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outBullPower[], double outBearPower[] )
   {
      EriStream sp = new EriStream(this);
      RetCode retCode = eriOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ERI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ERI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ERI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind eriOpen (composition seam). */
   EriStream eriOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      EriStream sp = new EriStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outBullPower = new double[1];
      double[] sink_outBearPower = new double[1];
      RetCode retCode = eriOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outBullPower, sink_outBearPower, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ERI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ERI open: internal error", retCode);
      }
      throw new TaLibArgumentException("ERI open: " + retCode, retCode);
   }
   /**
    * Open a live ERI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ERI} at that bar.
    * <p>The history must hold at least {@code ERI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public EriStream eriOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("ERI open", "inHigh", inHigh);
      requireHistory("ERI open", inHigh.length);
      requireArgument("ERI open", "inLow", inLow);
      requireArgument("ERI open", "inClose", inClose);
      requireHistoryLength("ERI open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ERI open", "inClose", inClose.length, inHigh.length);
      return eriOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#eriOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ERI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link EriStream#outRange()}.
    */
   public EriStream eriOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outBullPower[], double outBearPower[] )
   {
      requireArgument("ERI openAndFill", "inHigh", inHigh);
      requireHistory("ERI openAndFill", inHigh.length);
      requireArgument("ERI openAndFill", "inLow", inLow);
      requireArgument("ERI openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ERI openAndFill", inHigh.length, ERI_Lookback(optInTimePeriod));
      requireHistoryLength("ERI openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ERI openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("ERI openAndFill", "outBullPower", outBullPower, guardOutLen);
      requireLength("ERI openAndFill", "outBearPower", outBearPower, guardOutLen);
      if( (Object)outBullPower == (Object)inHigh || (Object)outBullPower == (Object)inLow || (Object)outBullPower == (Object)inClose || (Object)outBearPower == (Object)inHigh || (Object)outBearPower == (Object)inLow || (Object)outBearPower == (Object)inClose || (Object)outBullPower == (Object)outBearPower ) {
         throw new TaLibArgumentException("ERI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return eriOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outBullPower, outBearPower);
   }
