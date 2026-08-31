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
 *  082126 MF,CC  First version (issue #237).
 */

   /**
    * Number of leading input bars {@link Core#VWAP} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int VWAP_Lookback( )
   {
      /* Cumulative from the first bar of the requested range, so the very
       * first bar already has a complete answer and nothing is consumed
       * before it. Same shape as ta_AD.c and ta_OBV.c.
       */
      return 0 ;

   }
   RetCode VWAP_Impl( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      double inClose[],
                      double inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double typPrice = 0;
      double volume = 0;
      double tempReal = 0;
      double vwap = 0;
      int outIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Volume Weighted Average Price: the average typical price paid per
       * unit of volume, accumulated from the first bar of the range.
       *
       *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
       *
       * Every charting package anchors this to a trading session and resets
       * the two sums at each session boundary. TA-Lib takes no timestamp on
       * any function, so the anchor is the caller's choice of range: pass one
       * session's slice of bars to get that session's VWAP. This is how AD
       * and OBV, the other two cumulative volume functions, are already used
       * across session boundaries (issue #237).
       */
      sumPV = 0.0;
      sumV = 0.0;
      vwap = 0.0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* The typical price is written exactly as in ta_TYPPRICE.c so that the
          * two agree bit for bit and this stays a true composite of it.
          */
         typPrice = (inHigh[i] + inLow[i] + inClose[i]) / 3.0;
         volume = inVolume[i];
         /* A bar is weighted only if both of its terms are real numbers. That is
          * the whole condition: a NaN or an infinity in the price or the volume
          * is the only way a bar cannot be weighted, and every other bar --
          * including one that traded nothing -- is weighted normally.
          *
          * The test gates BOTH adds. Letting the volume in without its matching
          * price term would leave a weight in the divisor that nothing paid for,
          * biasing every later value: a NaN close with a good volume would drag
          * the next value 25% low.
          *
          * Skipping the bar is what makes this recoverable. These are CUMULATIVE
          * sums with no trailing term to subtract anything back out, so a single
          * non-finite bar allowed in would leave both sums non-finite for the
          * REST of the call -- the line would repeat one stale value on every
          * later bar however clean it was, silently, and looking like a plausible
          * price the whole way. Skipping keeps the state usable, so the average
          * resumes on the very next bar that can be weighted.
          *
          * Testing the two INPUTS, not the product and not the candidate sums, is
          * a measured choice:
          *
          *   - The candidate sums would have to be committed conditionally, which
          *     puts four cmovs in the loop-carried dependency chain and costs
          *     +60% on this loop. Both forms below leave the adds unconditional
          *     inside a predicted branch and measure free.
          *   - The product alone would also detect every unusable bar, one test
          *     instead of two, and measures the same. But it would additionally
          *     drop a WELL-FORMED bar whose price and volume are both finite and
          *     whose product merely overflows -- silently, and taking that bar's
          *     volume out of the divisor with it. Testing the inputs leaves that
          *     case exactly as it was before this guard existed: the overflow
          *     reaches the sum and the call reports Inf, which is the documented
          *     `double` overflow class rather than an indicator defect, and is
          *     louder than a freeze.
          *
          * So this changes behaviour for one thing only: a bar whose price or
          * volume is not a finite number. On finite data the test is always true
          * and no value the function has ever produced moves. Only the batch path
          * needs it -- the streaming Update/Peek entry points reject a non-finite
          * bar with TA_BAD_PARAM before it reaches any accumulator.
          */
         /* The product is kept in its own statement so no compiler may contract it
          * into an FMA. Contracting here would make the C output disagree with the
          * Rust, Java and C# backends under the cross-language bitwise gate. Same
          * reason as in ta_codegen/input/vwma/vwma.c.
          *
          * Computed before the guard rather than inside it, and unconditionally,
          * so it stays a per-bar temporary. Assigned only on the taken arm it
          * would instead be live across bars, and the streaming tier would carry
          * it as a fourth state field in every handle -- 8 bytes to hold a value
          * no later bar reads. The multiply on a skipped bar is discarded.
          */
         tempReal = typPrice * volume;
         if( (Double.isFinite(typPrice)) && (Double.isFinite(volume)) ) {
            sumPV += tempReal;
            sumV += volume;
         }
         /* Bars that traded nothing carry no weight, so a zero-volume bar in
          * the middle of a series leaves both sums untouched and repeats the
          * previous value on its own -- no arm needed for that. A bar skipped
          * by the guard above repeats it for the same reason.
          *
          * The arm below is for the one case the ratio cannot express: a
          * leading run of bars before any volume has traded, where there are
          * no weights at all and the weighted mean is undefined. The last
          * value computed is carried forward instead, which is 0.0 until the
          * first bar with volume. Volume is non-negative, so once the divisor
          * leaves zero it never returns and this arm cannot fire again.
          *
          * A successful call therefore never emits NaN or Inf (issue #112),
          * which is the divergence from pandas-ta-classic and from
          * trading-signals: the first emits NaN there, the second no bar at
          * all. Testing sumV rather than the bar's own volume also keeps a
          * negative divisor -- which no non-negative volume series can
          * produce -- out of a price-scale output, as ta_CMF.c does.
          */
         if( sumV > 0.0 ) {
            vwap = sumPV / sumV;
         }
         outReal[outIdx++] = vwap;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode VWAP_Impl( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      float inClose[],
                      float inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double typPrice = 0;
      double volume = 0;
      double tempReal = 0;
      double vwap = 0;
      int outIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      sumPV = 0.0;
      sumV = 0.0;
      vwap = 0.0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         typPrice = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3.0;
         volume = (double)inVolume[i];
         tempReal = typPrice * volume;
         if( (Double.isFinite(typPrice)) && (Double.isFinite(volume)) ) {
            sumPV += tempReal;
            sumV += volume;
         }
         if( sumV > 0.0 ) {
            vwap = sumPV / sumV;
         }
         outReal[outIdx++] = vwap;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Volume Weighted Average Price: the average price paid per unit of volume
    * traded, accumulated from the first bar of the range onward. Because every
    * bar is weighted by the volume that traded on it, VWAP tracks where the
    * bulk of the money actually changed hands rather than where the last trade
    * printed. Price above VWAP is read as buyers paying up relative to the
    * session's average cost, price below it as the reverse, which is why
    * execution desks quote fills against it. VWAP is a running mean, not a
    * moving average: it has no window and no decay, so each new bar carries a
    * smaller share of the total and the line grows steadily more sluggish the
    * further it runs from its anchor. It stays within the range of the typical
    * prices it averages, but over a long trending range it can sit far from the
    * current price.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP_t = ( High_t + Low_t + Close_t ) / 3; VWAP_t = ( Σ TP · Volume ) / ( Σ Volume ), both sums running from the first bar of the range
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The sums run from the first bar of the range and are never reset. Charting packages anchor VWAP to a trading session and restart it at each session boundary; no TA-Lib function takes a timestamp or a session boundary, so the anchor is the range the caller asks for — pass one session's bars to get that session's VWAP. This is how AD and OBV, the other cumulative volume functions, are already used across sessions.</li>
    * <li>Volume is expected to be non-negative. A zero-volume bar carries no weight, so one occurring after volume has traded leaves the average exactly where it was. Before *any* volume has traded there are no weights at all and the weighted mean is undefined; those bars carry the previous value forward, which is 0 until the first bar with volume. A successful call never emits NaN or ±Inf. Other implementations differ here: pandas-ta-classic divides through and emits NaN, and trading-signals emits no value for the bar at all.</li>
    * <li>A bar whose price or volume is not a finite number cannot be weighted, so it is left out of the average entirely and repeats the previous value. It is skipped, not absorbed: the running average stays usable and resumes on the next bar that can be weighted, rather than being held at one stale value for the remainder of the range.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VWAP_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Volume weighted average price, cumulative from the first
    *        bar of the range. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#OBV
    * @see Core#TYPPRICE
    * @see Core#VWMA
    */
   public OutRange VWAP( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         double inVolume[],
                         double outReal[] )
   {
      requireIndexRange("VWAP", startIdx, endIdx);
      int guardStart = clampedStart("VWAP", startIdx, VWAP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VWAP", "inHigh", inHigh, guardInLen);
      requireLength("VWAP", "inLow", inLow, guardInLen);
      requireLength("VWAP", "inClose", inClose, guardInLen);
      requireLength("VWAP", "inVolume", inVolume, guardInLen);
      requireLength("VWAP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWAP_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VWAP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Volume Weighted Average Price: the average price paid per unit of volume
    * traded, accumulated from the first bar of the range onward. Because every
    * bar is weighted by the volume that traded on it, VWAP tracks where the
    * bulk of the money actually changed hands rather than where the last trade
    * printed. Price above VWAP is read as buyers paying up relative to the
    * session's average cost, price below it as the reverse, which is why
    * execution desks quote fills against it. VWAP is a running mean, not a
    * moving average: it has no window and no decay, so each new bar carries a
    * smaller share of the total and the line grows steadily more sluggish the
    * further it runs from its anchor. It stays within the range of the typical
    * prices it averages, but over a long trending range it can sit far from the
    * current price.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP_t = ( High_t + Low_t + Close_t ) / 3; VWAP_t = ( Σ TP · Volume ) / ( Σ Volume ), both sums running from the first bar of the range
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The sums run from the first bar of the range and are never reset. Charting packages anchor VWAP to a trading session and restart it at each session boundary; no TA-Lib function takes a timestamp or a session boundary, so the anchor is the range the caller asks for — pass one session's bars to get that session's VWAP. This is how AD and OBV, the other cumulative volume functions, are already used across sessions.</li>
    * <li>Volume is expected to be non-negative. A zero-volume bar carries no weight, so one occurring after volume has traded leaves the average exactly where it was. Before *any* volume has traded there are no weights at all and the weighted mean is undefined; those bars carry the previous value forward, which is 0 until the first bar with volume. A successful call never emits NaN or ±Inf. Other implementations differ here: pandas-ta-classic divides through and emits NaN, and trading-signals emits no value for the bar at all.</li>
    * <li>A bar whose price or volume is not a finite number cannot be weighted, so it is left out of the average entirely and repeats the previous value. It is skipped, not absorbed: the running average stays usable and resumes on the next bar that can be weighted, rather than being held at one stale value for the remainder of the range.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#VWAP_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param outReal Volume weighted average price, cumulative from the first
    *        bar of the range. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#OBV
    * @see Core#TYPPRICE
    * @see Core#VWMA
    */
   public OutRange VWAP( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         float inVolume[],
                         double outReal[] )
   {
      requireIndexRange("VWAP", startIdx, endIdx);
      int guardStart = clampedStart("VWAP", startIdx, VWAP_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("VWAP", "inHigh", inHigh, guardInLen);
      requireLength("VWAP", "inLow", inLow, guardInLen);
      requireLength("VWAP", "inClose", inClose, guardInLen);
      requireLength("VWAP", "inVolume", inVolume, guardInLen);
      requireLength("VWAP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = VWAP_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("VWAP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live VWAP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#VWAP} over the same series.
    * Open with {@link Core#vwapOpen}; there is no close — the handle is
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
   public static final class VwapStream {
      Core core;
      double sumPV;
      double sumV;
      double vwap;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      VwapStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#VWAP} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      VwapStream( VwapStream other ) {
         this.core = other.core;
         this.sumPV = other.sumPV;
         this.sumV = other.sumV;
         this.vwap = other.vwap;
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
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("VWAP update: BadParam", RetCode.BadParam);
         }
         core.vwapStepImpl(this, inHigh, inLow, inClose, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] ) {
         requireArgument("VWAP updateAndFill", "inHigh", inHigh);
         requireArgument("VWAP updateAndFill", "inLow", inLow);
         requireArgument("VWAP updateAndFill", "inClose", inClose);
         requireArgument("VWAP updateAndFill", "inVolume", inVolume);
         requireArgument("VWAP updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("VWAP updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("VWAP updateAndFill: BadParam", RetCode.BadParam);
            }
            core.vwapStepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
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
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("VWAP peek: BadParam", RetCode.BadParam);
         VwapStream sp = this;
         double typPrice = 0.0;
         double volume = 0.0;
         double tempReal = 0.0;
         double cur_outReal = sp.cur_outReal;
         double sumPV = sp.sumPV;
         double sumV = sp.sumV;
         double vwap = sp.vwap;
         /* The typical price is written exactly as in ta_TYPPRICE.c so that the
          * two agree bit for bit and this stays a true composite of it.
          */
         typPrice = (inHigh + inLow + inClose) / 3.0;
         volume = inVolume;
         /* A bar is weighted only if both of its terms are real numbers. That is
          * the whole condition: a NaN or an infinity in the price or the volume
          * is the only way a bar cannot be weighted, and every other bar --
          * including one that traded nothing -- is weighted normally.
          *
          * The test gates BOTH adds. Letting the volume in without its matching
          * price term would leave a weight in the divisor that nothing paid for,
          * biasing every later value: a NaN close with a good volume would drag
          * the next value 25% low.
          *
          * Skipping the bar is what makes this recoverable. These are CUMULATIVE
          * sums with no trailing term to subtract anything back out, so a single
          * non-finite bar allowed in would leave both sums non-finite for the
          * REST of the call -- the line would repeat one stale value on every
          * later bar however clean it was, silently, and looking like a plausible
          * price the whole way. Skipping keeps the state usable, so the average
          * resumes on the very next bar that can be weighted.
          *
          * Testing the two INPUTS, not the product and not the candidate sums, is
          * a measured choice:
          *
          *   - The candidate sums would have to be committed conditionally, which
          *     puts four cmovs in the loop-carried dependency chain and costs
          *     +60% on this loop. Both forms below leave the adds unconditional
          *     inside a predicted branch and measure free.
          *   - The product alone would also detect every unusable bar, one test
          *     instead of two, and measures the same. But it would additionally
          *     drop a WELL-FORMED bar whose price and volume are both finite and
          *     whose product merely overflows -- silently, and taking that bar's
          *     volume out of the divisor with it. Testing the inputs leaves that
          *     case exactly as it was before this guard existed: the overflow
          *     reaches the sum and the call reports Inf, which is the documented
          *     `double` overflow class rather than an indicator defect, and is
          *     louder than a freeze.
          *
          * So this changes behaviour for one thing only: a bar whose price or
          * volume is not a finite number. On finite data the test is always true
          * and no value the function has ever produced moves. Only the batch path
          * needs it -- the streaming Update/Peek entry points reject a non-finite
          * bar with TA_BAD_PARAM before it reaches any accumulator.
          */
         /* The product is kept in its own statement so no compiler may contract it
          * into an FMA. Contracting here would make the C output disagree with the
          * Rust, Java and C# backends under the cross-language bitwise gate. Same
          * reason as in ta_codegen/input/vwma/vwma.c.
          *
          * Computed before the guard rather than inside it, and unconditionally,
          * so it stays a per-bar temporary. Assigned only on the taken arm it
          * would instead be live across bars, and the streaming tier would carry
          * it as a fourth state field in every handle -- 8 bytes to hold a value
          * no later bar reads. The multiply on a skipped bar is discarded.
          */
         tempReal = typPrice * volume;
         if( (Double.isFinite(typPrice)) && (Double.isFinite(volume)) ) {
            sumPV += tempReal;
            sumV += volume;
         }
         /* Bars that traded nothing carry no weight, so a zero-volume bar in
          * the middle of a series leaves both sums untouched and repeats the
          * previous value on its own -- no arm needed for that. A bar skipped
          * by the guard above repeats it for the same reason.
          *
          * The arm below is for the one case the ratio cannot express: a
          * leading run of bars before any volume has traded, where there are
          * no weights at all and the weighted mean is undefined. The last
          * value computed is carried forward instead, which is 0.0 until the
          * first bar with volume. Volume is non-negative, so once the divisor
          * leaves zero it never returns and this arm cannot fire again.
          *
          * A successful call therefore never emits NaN or Inf (issue #112),
          * which is the divergence from pandas-ta-classic and from
          * trading-signals: the first emits NaN there, the second no bar at
          * all. Testing sumV rather than the bar's own volume also keeps a
          * negative divisor -- which no non-negative volume series can
          * produce -- out of a price-scale output, as ta_CMF.c does.
          */
         if( sumV > 0.0 ) {
            vwap = sumPV / sumV;
         }
         cur_outReal = vwap;
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
      public VwapStream clone() {
         return new VwapStream(this);
      }
   }
   void vwapStepImpl( VwapStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double typPrice = 0.0;
      double volume = 0.0;
      double tempReal = 0.0;
      /* The typical price is written exactly as in ta_TYPPRICE.c so that the
       * two agree bit for bit and this stays a true composite of it.
       */
      typPrice = (inHigh + inLow + inClose) / 3.0;
      volume = inVolume;
      /* A bar is weighted only if both of its terms are real numbers. That is
       * the whole condition: a NaN or an infinity in the price or the volume
       * is the only way a bar cannot be weighted, and every other bar --
       * including one that traded nothing -- is weighted normally.
       *
       * The test gates BOTH adds. Letting the volume in without its matching
       * price term would leave a weight in the divisor that nothing paid for,
       * biasing every later value: a NaN close with a good volume would drag
       * the next value 25% low.
       *
       * Skipping the bar is what makes this recoverable. These are CUMULATIVE
       * sums with no trailing term to subtract anything back out, so a single
       * non-finite bar allowed in would leave both sums non-finite for the
       * REST of the call -- the line would repeat one stale value on every
       * later bar however clean it was, silently, and looking like a plausible
       * price the whole way. Skipping keeps the state usable, so the average
       * resumes on the very next bar that can be weighted.
       *
       * Testing the two INPUTS, not the product and not the candidate sums, is
       * a measured choice:
       *
       *   - The candidate sums would have to be committed conditionally, which
       *     puts four cmovs in the loop-carried dependency chain and costs
       *     +60% on this loop. Both forms below leave the adds unconditional
       *     inside a predicted branch and measure free.
       *   - The product alone would also detect every unusable bar, one test
       *     instead of two, and measures the same. But it would additionally
       *     drop a WELL-FORMED bar whose price and volume are both finite and
       *     whose product merely overflows -- silently, and taking that bar's
       *     volume out of the divisor with it. Testing the inputs leaves that
       *     case exactly as it was before this guard existed: the overflow
       *     reaches the sum and the call reports Inf, which is the documented
       *     `double` overflow class rather than an indicator defect, and is
       *     louder than a freeze.
       *
       * So this changes behaviour for one thing only: a bar whose price or
       * volume is not a finite number. On finite data the test is always true
       * and no value the function has ever produced moves. Only the batch path
       * needs it -- the streaming Update/Peek entry points reject a non-finite
       * bar with TA_BAD_PARAM before it reaches any accumulator.
       */
      /* The product is kept in its own statement so no compiler may contract it
       * into an FMA. Contracting here would make the C output disagree with the
       * Rust, Java and C# backends under the cross-language bitwise gate. Same
       * reason as in ta_codegen/input/vwma/vwma.c.
       *
       * Computed before the guard rather than inside it, and unconditionally,
       * so it stays a per-bar temporary. Assigned only on the taken arm it
       * would instead be live across bars, and the streaming tier would carry
       * it as a fourth state field in every handle -- 8 bytes to hold a value
       * no later bar reads. The multiply on a skipped bar is discarded.
       */
      tempReal = typPrice * volume;
      if( (Double.isFinite(typPrice)) && (Double.isFinite(volume)) ) {
         sp.sumPV += tempReal;
         sp.sumV += volume;
      }
      /* Bars that traded nothing carry no weight, so a zero-volume bar in
       * the middle of a series leaves both sums untouched and repeats the
       * previous value on its own -- no arm needed for that. A bar skipped
       * by the guard above repeats it for the same reason.
       *
       * The arm below is for the one case the ratio cannot express: a
       * leading run of bars before any volume has traded, where there are
       * no weights at all and the weighted mean is undefined. The last
       * value computed is carried forward instead, which is 0.0 until the
       * first bar with volume. Volume is non-negative, so once the divisor
       * leaves zero it never returns and this arm cannot fire again.
       *
       * A successful call therefore never emits NaN or Inf (issue #112),
       * which is the divergence from pandas-ta-classic and from
       * trading-signals: the first emits NaN there, the second no bar at
       * all. Testing sumV rather than the bar's own volume also keeps a
       * negative divisor -- which no non-negative volume series can
       * produce -- out of a price-scale output, as ta_CMF.c does.
       */
      if( sp.sumV > 0.0 ) {
         sp.vwap = sp.sumPV / sp.sumV;
      }
      sp.cur_outReal = sp.vwap;
   }
   private RetCode vwapOpenImpl( VwapStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumPV = 0;
      double sumV = 0;
      double typPrice = 0;
      double volume = 0;
      double tempReal = 0;
      double vwap = 0;
      int outIdx = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Volume Weighted Average Price: the average typical price paid per
       * unit of volume, accumulated from the first bar of the range.
       *
       *    VWAP = sum( TYPPRICE * Volume ) / sum( Volume )
       *
       * Every charting package anchors this to a trading session and resets
       * the two sums at each session boundary. TA-Lib takes no timestamp on
       * any function, so the anchor is the caller's choice of range: pass one
       * session's slice of bars to get that session's VWAP. This is how AD
       * and OBV, the other two cumulative volume functions, are already used
       * across session boundaries (issue #237).
       */
      sumPV = 0.0;
      sumV = 0.0;
      vwap = 0.0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         /* The typical price is written exactly as in ta_TYPPRICE.c so that the
          * two agree bit for bit and this stays a true composite of it.
          */
         typPrice = (inHigh[i] + inLow[i] + inClose[i]) / 3.0;
         volume = inVolume[i];
         /* A bar is weighted only if both of its terms are real numbers. That is
          * the whole condition: a NaN or an infinity in the price or the volume
          * is the only way a bar cannot be weighted, and every other bar --
          * including one that traded nothing -- is weighted normally.
          *
          * The test gates BOTH adds. Letting the volume in without its matching
          * price term would leave a weight in the divisor that nothing paid for,
          * biasing every later value: a NaN close with a good volume would drag
          * the next value 25% low.
          *
          * Skipping the bar is what makes this recoverable. These are CUMULATIVE
          * sums with no trailing term to subtract anything back out, so a single
          * non-finite bar allowed in would leave both sums non-finite for the
          * REST of the call -- the line would repeat one stale value on every
          * later bar however clean it was, silently, and looking like a plausible
          * price the whole way. Skipping keeps the state usable, so the average
          * resumes on the very next bar that can be weighted.
          *
          * Testing the two INPUTS, not the product and not the candidate sums, is
          * a measured choice:
          *
          *   - The candidate sums would have to be committed conditionally, which
          *     puts four cmovs in the loop-carried dependency chain and costs
          *     +60% on this loop. Both forms below leave the adds unconditional
          *     inside a predicted branch and measure free.
          *   - The product alone would also detect every unusable bar, one test
          *     instead of two, and measures the same. But it would additionally
          *     drop a WELL-FORMED bar whose price and volume are both finite and
          *     whose product merely overflows -- silently, and taking that bar's
          *     volume out of the divisor with it. Testing the inputs leaves that
          *     case exactly as it was before this guard existed: the overflow
          *     reaches the sum and the call reports Inf, which is the documented
          *     `double` overflow class rather than an indicator defect, and is
          *     louder than a freeze.
          *
          * So this changes behaviour for one thing only: a bar whose price or
          * volume is not a finite number. On finite data the test is always true
          * and no value the function has ever produced moves. Only the batch path
          * needs it -- the streaming Update/Peek entry points reject a non-finite
          * bar with TA_BAD_PARAM before it reaches any accumulator.
          */
         /* The product is kept in its own statement so no compiler may contract it
          * into an FMA. Contracting here would make the C output disagree with the
          * Rust, Java and C# backends under the cross-language bitwise gate. Same
          * reason as in ta_codegen/input/vwma/vwma.c.
          *
          * Computed before the guard rather than inside it, and unconditionally,
          * so it stays a per-bar temporary. Assigned only on the taken arm it
          * would instead be live across bars, and the streaming tier would carry
          * it as a fourth state field in every handle -- 8 bytes to hold a value
          * no later bar reads. The multiply on a skipped bar is discarded.
          */
         tempReal = typPrice * volume;
         if( (Double.isFinite(typPrice)) && (Double.isFinite(volume)) ) {
            sumPV += tempReal;
            sumV += volume;
         }
         /* Bars that traded nothing carry no weight, so a zero-volume bar in
          * the middle of a series leaves both sums untouched and repeats the
          * previous value on its own -- no arm needed for that. A bar skipped
          * by the guard above repeats it for the same reason.
          *
          * The arm below is for the one case the ratio cannot express: a
          * leading run of bars before any volume has traded, where there are
          * no weights at all and the weighted mean is undefined. The last
          * value computed is carried forward instead, which is 0.0 until the
          * first bar with volume. Volume is non-negative, so once the divisor
          * leaves zero it never returns and this arm cannot fire again.
          *
          * A successful call therefore never emits NaN or Inf (issue #112),
          * which is the divergence from pandas-ta-classic and from
          * trading-signals: the first emits NaN there, the second no bar at
          * all. Testing sumV rather than the bar's own volume also keeps a
          * negative divisor -- which no non-negative volume series can
          * produce -- out of a price-scale output, as ta_CMF.c does.
          */
         if( sumV > 0.0 ) {
            vwap = sumPV / sumV;
         }
         outReal[outIdx++ * outStride] = vwap;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.sumPV = sumPV;
      sp.sumV = sumV;
      sp.vwap = vwap;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* vwapOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   VwapStream vwapOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VwapStream sp = new VwapStream(this);
      RetCode retCode = vwapOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VWAP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VWAP openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("VWAP openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind vwapOpen (composition seam). */
   VwapStream vwapOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx )
   {
      VwapStream sp = new VwapStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = vwapOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("VWAP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("VWAP open: internal error", retCode);
      }
      throw new TaLibArgumentException("VWAP open: " + retCode, retCode);
   }
   /**
    * Open a live VWAP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#VWAP} at that bar.
    * <p>The history must hold at least {@code VWAP_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public VwapStream vwapOpen( double inHigh[], double inLow[], double inClose[], double inVolume[] )
   {
      requireArgument("VWAP open", "inHigh", inHigh);
      requireHistory("VWAP open", inHigh.length);
      requireArgument("VWAP open", "inLow", inLow);
      requireArgument("VWAP open", "inClose", inClose);
      requireArgument("VWAP open", "inVolume", inVolume);
      requireHistoryLength("VWAP open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("VWAP open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("VWAP open", "inVolume", inVolume.length, inHigh.length);
      return vwapOpenInternal(inHigh, inLow, inClose, inVolume, 0);
   }
   /**
    * {@link Core#vwapOpen} that also fills the output array(s) bit-identically
    * to {@link Core#VWAP} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link VwapStream#outRange()}.
    */
   public VwapStream vwapOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] )
   {
      requireArgument("VWAP openAndFill", "inHigh", inHigh);
      requireHistory("VWAP openAndFill", inHigh.length);
      requireArgument("VWAP openAndFill", "inLow", inLow);
      requireArgument("VWAP openAndFill", "inClose", inClose);
      requireArgument("VWAP openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("VWAP openAndFill", inHigh.length, VWAP_Lookback());
      requireHistoryLength("VWAP openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("VWAP openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("VWAP openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("VWAP openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("VWAP openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return vwapOpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, outBegIdx, outNBElement, outReal);
   }
