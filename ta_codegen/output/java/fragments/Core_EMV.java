/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  092826 KL,CC  First version (#465).
 */

   /**
    * Number of leading input bars {@link Core#emv} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of one-bar values in the simple moving
    *        average (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInVolumeDivisor Volume is divided by this before it forms the
    *        box ratio (default 10000; minimum 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int emvLookback( int optInTimePeriod, double optInVolumeDivisor )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInVolumeDivisor == REAL_DEFAULT ) {
         optInVolumeDivisor = 1e4;
      } else if( !(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= REAL_MAX) ) {
         return -1;
      }
      return 1 + smaLookback(optInTimePeriod) ;

   }
   RetCode emvImpl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    double inVolume[],
                    int optInTimePeriod,
                    double optInVolumeDivisor,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double periodTotal = 0;
      double prevMid = 0;
      double mid = 0;
      double range = 0;
      double boxRatio = 0;
      double raw = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      double[] rawBuffer;
      int rawBuffer_Idx = 0;
      int maxIdx_rawBuffer = (50)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInVolumeDivisor == REAL_DEFAULT ) {
         optInVolumeDivisor = 1e4;
      } else if( !(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      /* The one-bar values are kept in a ring rather than recomputed at the
       * trailing index: outReal may alias an input, and those bars may already
       * hold outputs.
       */
      lookbackTotal = emvLookback(optInTimePeriod, optInVolumeDivisor);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      rawBuffer = new double[optInTimePeriod];
      maxIdx_rawBuffer = (optInTimePeriod)-1;
      rawBuffer_Idx = 0;
      /* Keep the operand order and sma.c's add / snapshot / subtract order: the
       * composite gate rebuilds this from MEDPRICE, MOM, SUB, DIV and SMA and
       * compares bitwise.
       *
       * The zero guards are exact != 0.0 tests, never TA_IS_ZERO: the output
       * scale follows the volume and the divisor, so no absolute band fits. A
       * nonzero range with a box ratio that underflows to 0 takes the guard too.
       */
      i = startIdx - lookbackTotal;
      prevMid = (inHigh[i] + inLow[i]) / 2.0;
      i = i + 1;
      periodTotal = 0.0;
      while( i < startIdx ) {
         mid = (inHigh[i] + inLow[i]) / 2.0;
         range = inHigh[i] - inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         mid = (inHigh[i] + inLow[i]) / 2.0;
         range = inHigh[i] - inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         tempReal = periodTotal;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
         periodTotal -= rawBuffer[rawBuffer_Idx];
         outReal[outIdx] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode emvImpl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    float inVolume[],
                    int optInTimePeriod,
                    double optInVolumeDivisor,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double periodTotal = 0;
      double prevMid = 0;
      double mid = 0;
      double range = 0;
      double boxRatio = 0;
      double raw = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      double[] rawBuffer;
      int rawBuffer_Idx = 0;
      int maxIdx_rawBuffer = (50)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInVolumeDivisor == REAL_DEFAULT ) {
         optInVolumeDivisor = 1e4;
      } else if( !(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = emvLookback(optInTimePeriod, optInVolumeDivisor);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      rawBuffer = new double[optInTimePeriod];
      maxIdx_rawBuffer = (optInTimePeriod)-1;
      rawBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      prevMid = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      i = i + 1;
      periodTotal = 0.0;
      while( i < startIdx ) {
         mid = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         range = (double)inHigh[i] - (double)inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = (double)inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         mid = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
         range = (double)inHigh[i] - (double)inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = (double)inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         tempReal = periodTotal;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
         periodTotal -= rawBuffer[rawBuffer_Idx];
         outReal[outIdx] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Ease of Movement: the bar-to-bar move of the high-low midpoint divided by
    * a box ratio of volume to range, averaged over a trailing window. It is the
    * numeric form of Richard W. Arms, Jr.'s Equivolume box. The box ratio is
    * positive whenever the bar traded and has a range, so the sign follows the
    * midpoint move. A large positive value means price rose easily, on light
    * volume relative to its range; a large negative value means it fell easily.
    * Values near zero mean volume was heavy for the distance travelled, or
    * price hardly moved. The output scales with the instrument's volume and
    * with the volume divisor, so its level is comparable only within one
    * instrument at one divisor. Traders mostly watch its sign and its zero
    * crossings.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/emv">ta-lib.org/functions/emv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The range is in price points. Achelis's text gives it in eighths of a point, the pre-decimal US quote unit; that reading is reached by multiplying the divisor by 8.</li>
    * <li>The divisor is a pure output scale: doubling it doubles every value. Pick one that suits the instrument's volume.</li>
    * <li>A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple moving average; for an exponential one, apply {@code EMA} to this function's output at a period of 1.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#emvLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of one-bar values in the simple moving
    *        average (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInVolumeDivisor Volume is divided by this before it forms the
    *        box ratio (default 10000; minimum 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param outReal Ease of Movement, averaged over the window. Must hold at
    *        least {@code endIdx - max(startIdx, emvLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    * @see Core#medprice
    * @see Core#marketfi
    * @see Core#efi
    * @see Core#sma
    * @see Core#ema
    */
   public OutRange emv( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inVolume[],
                        int optInTimePeriod,
                        double optInVolumeDivisor,
                        double outReal[] )
   {
      requireIndexRange("EMV", startIdx, endIdx);
      int guardStart = clampedStart("EMV", startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EMV", "inHigh", inHigh, guardInLen);
      requireLength("EMV", "inLow", inLow, guardInLen);
      requireLength("EMV", "inVolume", inVolume, guardInLen);
      requireLength("EMV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = emvImpl(startIdx, endIdx, inHigh, inLow, inVolume, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("EMV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Ease of Movement: the bar-to-bar move of the high-low midpoint divided by
    * a box ratio of volume to range, averaged over a trailing window. It is the
    * numeric form of Richard W. Arms, Jr.'s Equivolume box. The box ratio is
    * positive whenever the bar traded and has a range, so the sign follows the
    * midpoint move. A large positive value means price rose easily, on light
    * volume relative to its range; a large negative value means it fell easily.
    * Values near zero mean volume was heavy for the distance travelled, or
    * price hardly moved. The output scales with the instrument's volume and
    * with the volume divisor, so its level is comparable only within one
    * instrument at one divisor. Traders mostly watch its sign and its zero
    * crossings.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/emv">ta-lib.org/functions/emv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The range is in price points. Achelis's text gives it in eighths of a point, the pre-decimal US quote unit; that reading is reached by multiplying the divisor by 8.</li>
    * <li>The divisor is a pure output scale: doubling it doubles every value. Pick one that suits the instrument's volume.</li>
    * <li>A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple moving average; for an exponential one, apply {@code EMA} to this function's output at a period of 1.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#emvLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod Number of one-bar values in the simple moving
    *        average (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInVolumeDivisor Volume is divided by this before it forms the
    *        box ratio (default 10000; minimum 1; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param outReal Ease of Movement, averaged over the window. Must hold at
    *        least {@code endIdx - max(startIdx, emvLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    * @see Core#medprice
    * @see Core#marketfi
    * @see Core#efi
    * @see Core#sma
    * @see Core#ema
    */
   public OutRange emv( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inVolume[],
                        int optInTimePeriod,
                        double optInVolumeDivisor,
                        double outReal[] )
   {
      requireIndexRange("EMV", startIdx, endIdx);
      int guardStart = clampedStart("EMV", startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EMV", "inHigh", inHigh, guardInLen);
      requireLength("EMV", "inLow", inLow, guardInLen);
      requireLength("EMV", "inVolume", inVolume, guardInLen);
      requireLength("EMV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = emvImpl(startIdx, endIdx, inHigh, inLow, inVolume, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("EMV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live EMV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#emv} over the same series.
    * Open with {@link Core#emvOpen}; there is no close — the handle is
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
   public static final class EmvStream {
      private Core core;
      private int optInTimePeriod;
      private double optInVolumeDivisor;
      private double periodTotal;
      private double prevMid;
      private int rawBuffer_Idx;
      private int maxIdx_rawBuffer;
      private int cbSize_rawBuffer;
      private double[] cb_rawBuffer;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private EmvStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#emv} reports over the same bars: the
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
            throw failure("EMV advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private EmvStream( EmvStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInVolumeDivisor = other.optInVolumeDivisor;
         this.periodTotal = other.periodTotal;
         this.prevMid = other.prevMid;
         this.rawBuffer_Idx = other.rawBuffer_Idx;
         this.maxIdx_rawBuffer = other.maxIdx_rawBuffer;
         this.cbSize_rawBuffer = other.cbSize_rawBuffer;
         this.cb_rawBuffer = other.cb_rawBuffer.clone();
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
      public double update( double inHigh, double inLow, double inVolume ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("EMV update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inVolume) )
            throw nonFiniteBar("EMV update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inVolume");
         core.emvStepImpl(this, inHigh, inLow, inVolume);
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
      public double peek( double inHigh, double inLow, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inVolume) )
            throw nonFiniteBar("EMV peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inVolume");
         EmvStream sp = this;
         double mid = 0.0;
         double range = 0.0;
         double boxRatio = 0.0;
         double raw = 0.0;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         double periodTotal = sp.periodTotal;
         double prevMid = sp.prevMid;
         int rawBuffer_Idx = sp.rawBuffer_Idx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         mid = (inHigh + inLow) / 2.0;
         range = inHigh - inLow;
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = inVolume / sp.optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         pkSlot0 = rawBuffer_Idx;
         pkVal0 = raw;
         periodTotal += raw;
         tempReal = periodTotal;
         rawBuffer_Idx = rawBuffer_Idx + 1;
         if( rawBuffer_Idx > sp.maxIdx_rawBuffer ) {
            rawBuffer_Idx = 0;
         }
         periodTotal -= (rawBuffer_Idx != pkSlot0) ? sp.cb_rawBuffer[rawBuffer_Idx] : pkVal0;
         cur_outReal = tempReal / (double)sp.optInTimePeriod;
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
      public EmvStream clone() {
         return new EmvStream(this);
      }
   }
   private void emvStepImpl( EmvStream sp, double inHigh, double inLow, double inVolume )
   {
      double mid = 0.0;
      double range = 0.0;
      double boxRatio = 0.0;
      double raw = 0.0;
      double tempReal = 0.0;
      mid = (inHigh + inLow) / 2.0;
      range = inHigh - inLow;
      raw = 0.0;
      if( range != 0.0 ) {
         boxRatio = inVolume / sp.optInVolumeDivisor / range;
         if( boxRatio != 0.0 ) {
            raw = (mid - sp.prevMid) / boxRatio;
         }
      }
      sp.prevMid = mid;
      sp.cb_rawBuffer[sp.rawBuffer_Idx] = raw;
      sp.periodTotal += raw;
      tempReal = sp.periodTotal;
      sp.rawBuffer_Idx = sp.rawBuffer_Idx + 1;
      if( sp.rawBuffer_Idx > sp.maxIdx_rawBuffer ) {
         sp.rawBuffer_Idx = 0;
      }
      sp.periodTotal -= sp.cb_rawBuffer[sp.rawBuffer_Idx];
      sp.cur_outReal = tempReal / (double)sp.optInTimePeriod;
   }
   private RetCode emvOpenImpl( EmvStream sp, double inHigh[], double inLow[], double inVolume[], int startIdx, int optInTimePeriod, double optInVolumeDivisor, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double periodTotal = 0;
      double prevMid = 0;
      double mid = 0;
      double range = 0;
      double boxRatio = 0;
      double raw = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      double[] rawBuffer;
      int rawBuffer_Idx = 0;
      int maxIdx_rawBuffer = (50)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInVolumeDivisor == REAL_DEFAULT ) {
         optInVolumeDivisor = 1e4;
      } else if( !(optInVolumeDivisor >= 1e0 && optInVolumeDivisor <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* The one-bar values are kept in a ring rather than recomputed at the
       * trailing index: outReal may alias an input, and those bars may already
       * hold outputs.
       */
      lookbackTotal = emvLookback(optInTimePeriod, optInVolumeDivisor);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      rawBuffer = new double[optInTimePeriod];
      maxIdx_rawBuffer = (optInTimePeriod)-1;
      rawBuffer_Idx = 0;
      /* Keep the operand order and sma.c's add / snapshot / subtract order: the
       * composite gate rebuilds this from MEDPRICE, MOM, SUB, DIV and SMA and
       * compares bitwise.
       *
       * The zero guards are exact != 0.0 tests, never TA_IS_ZERO: the output
       * scale follows the volume and the divisor, so no absolute band fits. A
       * nonzero range with a box ratio that underflows to 0 takes the guard too.
       */
      i = startIdx - lookbackTotal;
      prevMid = (inHigh[i] + inLow[i]) / 2.0;
      i = i + 1;
      periodTotal = 0.0;
      while( i < startIdx ) {
         mid = (inHigh[i] + inLow[i]) / 2.0;
         range = inHigh[i] - inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         mid = (inHigh[i] + inLow[i]) / 2.0;
         range = inHigh[i] - inLow[i];
         raw = 0.0;
         if( range != 0.0 ) {
            boxRatio = inVolume[i] / optInVolumeDivisor / range;
            if( boxRatio != 0.0 ) {
               raw = (mid - prevMid) / boxRatio;
            }
         }
         prevMid = mid;
         i = i + 1;
         rawBuffer[rawBuffer_Idx] = raw;
         periodTotal += raw;
         tempReal = periodTotal;
         rawBuffer_Idx++;
         if( rawBuffer_Idx > maxIdx_rawBuffer ) { rawBuffer_Idx = 0; }
         periodTotal -= rawBuffer[rawBuffer_Idx];
         outReal[outIdx * outStride] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_rawBuffer = maxIdx_rawBuffer + 1;
      if( capCb_rawBuffer > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInVolumeDivisor = optInVolumeDivisor;
      sp.periodTotal = periodTotal;
      sp.prevMid = prevMid;
      sp.rawBuffer_Idx = rawBuffer_Idx;
      sp.maxIdx_rawBuffer = maxIdx_rawBuffer;
      sp.cbSize_rawBuffer = capCb_rawBuffer;
      sp.cb_rawBuffer = rawBuffer;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* emvOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   EmvStream emvOpenAndFillInternal( double inHigh[], double inLow[], double inVolume[], int startIdx, int optInTimePeriod, double optInVolumeDivisor, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      EmvStream sp = new EmvStream(this);
      RetCode retCode = emvOpenImpl(sp, inHigh, inLow, inVolume, startIdx, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("EMV openAndFill", inHigh.length, startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor));
      }
      throw streamFailure("EMV openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind emvOpen (composition seam). */
   EmvStream emvOpenInternal( double inHigh[], double inLow[], double inVolume[], int startIdx, int optInTimePeriod, double optInVolumeDivisor )
   {
      EmvStream sp = new EmvStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = emvOpenImpl(sp, inHigh, inLow, inVolume, startIdx, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("EMV open", inHigh.length, startIdx, emvLookback(optInTimePeriod, optInVolumeDivisor));
      }
      throw streamFailure("EMV open", retCode);
   }
   /**
    * Open a live EMV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#emv} at that bar.
    * <p>The history must hold at least {@code emvLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public EmvStream emvOpen( double inHigh[], double inLow[], double inVolume[], int optInTimePeriod, double optInVolumeDivisor )
   {
      requireArgument("EMV open", "inHigh", inHigh);
      requireHistory("EMV open", inHigh.length);
      requireArgument("EMV open", "inLow", inLow);
      requireArgument("EMV open", "inVolume", inVolume);
      requireHistoryLength("EMV open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("EMV open", "inVolume", inVolume.length, inHigh.length);
      return emvOpenInternal(inHigh, inLow, inVolume, 0, optInTimePeriod, optInVolumeDivisor);
   }
   /**
    * {@link Core#emvOpen} that also fills the output array(s) bit-identically
    * to {@link Core#emv} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link EmvStream#outRange()}.
    */
   public EmvStream emvOpenAndFill( double inHigh[], double inLow[], double inVolume[], int optInTimePeriod, double optInVolumeDivisor, double outReal[] )
   {
      requireArgument("EMV openAndFill", "inHigh", inHigh);
      requireHistory("EMV openAndFill", inHigh.length);
      requireArgument("EMV openAndFill", "inLow", inLow);
      requireArgument("EMV openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("EMV openAndFill", inHigh.length, emvLookback(optInTimePeriod, optInVolumeDivisor));
      requireHistoryLength("EMV openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("EMV openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("EMV openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inVolume ) {
         throw streamFailure("EMV openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return emvOpenAndFillInternal(inHigh, inLow, inVolume, 0, optInTimePeriod, optInVolumeDivisor, outBegIdx, outNBElement, outReal);
   }
