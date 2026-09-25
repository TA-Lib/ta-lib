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
 *  092226 MF,CC  Initial version (#431).
 */

   /**
    * Number of leading input bars {@link Core#crsi} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Period of the RSI of the closes (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInStreakPeriod Period of the RSI of the up/down streak (default
    *        2; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInRankPeriod Number of earlier one-bar returns each return is
    *        ranked against (default 100; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int crsiLookback( int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 3;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInStreakPeriod == Integer.MIN_VALUE ) {
         optInStreakPeriod = 2;
      } else if( optInStreakPeriod < 2 || optInStreakPeriod > 100000 ) {
         return -1;
      }
      if( optInRankPeriod == Integer.MIN_VALUE ) {
         optInRankPeriod = 100;
      } else if( optInRankPeriod < 2 || optInRankPeriod > 10000 ) {
         return -1;
      }
      int retValue;
      retValue = rsiLookback(optInTimePeriod);
      retValue = Math.max(retValue, rsiLookback(optInStreakPeriod) + 1);
      retValue = Math.max(retValue, percentrankLookback(optInRankPeriod) + rocpLookback(1));
      return retValue ;

   }
   RetCode crsiImpl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     int optInStreakPeriod,
                     int optInRankPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] tempRSI;
      double[] tempStreak;
      double[] tempStreakRSI;
      RetCode retCode;
      int lookbackTotal = 0;
      int anchorIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int offsetRSI = 0;
      int offsetStreak = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rsiNb = new MInteger();
      MInteger streakNb = new MInteger();
      MInteger rocNb = new MInteger();
      double prevClose = 0;
      double close = 0;
      double streak = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 3;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStreakPeriod == Integer.MIN_VALUE ) {
         optInStreakPeriod = 2;
      } else if( optInStreakPeriod < 2 || optInStreakPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInRankPeriod == Integer.MIN_VALUE ) {
         optInRankPeriod = 100;
      } else if( optInRankPeriod < 2 || optInRankPeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod);
      if( lookbackTotal > endIdx ) {
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Both RSI legs start on the bar lookbackTotal before startIdx (the streak,
       * a difference, one bar later), so the leg with the shorter lookback warms
       * up over the spare bars instead of starting cold at its own lookback.
       */
      anchorIdx = startIdx - lookbackTotal;
      tempStreak = new double[(int)((endIdx - anchorIdx) * 1)];
      tempRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInTimePeriod) + 1) * 1)];
      tempStreakRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInStreakPeriod)) * 1)];
      streak = 0.0;
      prevClose = inReal[anchorIdx];
      outIdx = 0;
      today = anchorIdx + 1;
      while( today <= endIdx ) {
         close = inReal[today];
         if( close > prevClose ) {
            streak = (streak > 0.0) ? streak + 1.0 : 1.0;
         } else if( close < prevClose ) {
            streak = (streak < 0.0) ? streak - 1.0 : 0 - 1.0;
         } else {
            streak = 0.0;
         }
         tempStreak[outIdx++] = streak;
         prevClose = close;
         today += 1;
      }
      OutRange _xr0 = rsi(0, outIdx - 1, tempStreak, optInStreakPeriod, tempStreakRSI);
      tempBegIdx.value = _xr0.begIdx();
      streakNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = rsi(anchorIdx + rsiLookback(optInTimePeriod), endIdx, inReal, optInTimePeriod, tempRSI);
      tempBegIdx.value = _xr1.begIdx();
      rsiNb.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      /* The streak is consumed: its buffer holds the returns. */
      OutRange _xr2 = rocp(startIdx - optInRankPeriod, endIdx, inReal, 1, tempStreak);
      tempBegIdx.value = _xr2.begIdx();
      rocNb.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = percentrank(0, rocNb.value - 1, tempStreak, optInRankPeriod, outReal);
      outBegIdx.value = _xr3.begIdx();
      outNBElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      offsetRSI = rsiNb.value - outNBElement.value;
      offsetStreak = streakNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = (tempRSI[i + offsetRSI] + tempStreakRSI[i + offsetStreak] + outReal[i]) / 3.0;
      }
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode crsiImpl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     int optInStreakPeriod,
                     int optInRankPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] tempRSI;
      double[] tempStreak;
      double[] tempStreakRSI;
      RetCode retCode;
      int lookbackTotal = 0;
      int anchorIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int offsetRSI = 0;
      int offsetStreak = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rsiNb = new MInteger();
      MInteger streakNb = new MInteger();
      MInteger rocNb = new MInteger();
      double prevClose = 0;
      double close = 0;
      double streak = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 3;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStreakPeriod == Integer.MIN_VALUE ) {
         optInStreakPeriod = 2;
      } else if( optInStreakPeriod < 2 || optInStreakPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInRankPeriod == Integer.MIN_VALUE ) {
         optInRankPeriod = 100;
      } else if( optInRankPeriod < 2 || optInRankPeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod);
      if( lookbackTotal > endIdx ) {
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      anchorIdx = startIdx - lookbackTotal;
      tempStreak = new double[(int)((endIdx - anchorIdx) * 1)];
      tempRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInTimePeriod) + 1) * 1)];
      tempStreakRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInStreakPeriod)) * 1)];
      streak = 0.0;
      prevClose = (double)inReal[anchorIdx];
      outIdx = 0;
      today = anchorIdx + 1;
      while( today <= endIdx ) {
         close = (double)inReal[today];
         if( close > prevClose ) {
            streak = (streak > 0.0) ? streak + 1.0 : 1.0;
         } else if( close < prevClose ) {
            streak = (streak < 0.0) ? streak - 1.0 : 0 - 1.0;
         } else {
            streak = 0.0;
         }
         tempStreak[outIdx++] = streak;
         prevClose = close;
         today += 1;
      }
      OutRange _xr0 = rsi(0, outIdx - 1, tempStreak, optInStreakPeriod, tempStreakRSI);
      tempBegIdx.value = _xr0.begIdx();
      streakNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = rsi(anchorIdx + rsiLookback(optInTimePeriod), endIdx, inReal, optInTimePeriod, tempRSI);
      tempBegIdx.value = _xr1.begIdx();
      rsiNb.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = rocp(startIdx - optInRankPeriod, endIdx, inReal, 1, tempStreak);
      tempBegIdx.value = _xr2.begIdx();
      rocNb.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = percentrank(0, rocNb.value - 1, tempStreak, optInRankPeriod, outReal);
      outBegIdx.value = _xr3.begIdx();
      outNBElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      offsetRSI = rsiNb.value - outNBElement.value;
      offsetStreak = streakNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = (tempRSI[i + offsetRSI] + tempStreakRSI[i + offsetStreak] + outReal[i]) / 3.0;
      }
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Connors RSI: a 0 to 100 oscillator, the plain average of three short-term
    * momentum readings on that same scale. The first is Wilder's RSI of the
    * closes over {@code optInTimePeriod} bars. The second is an RSI over
    * {@code optInStreakPeriod} bars of the up/down streak, the signed length of
    * the current run of higher or lower closes (+3 after three higher closes in
    * a row, -2 after two lower ones); an unchanged close resets the streak to
    * 0. The third is the percent rank of the bar's one-bar return, (close -
    * previous close) / previous close, among the {@code optInRankPeriod}
    * returns before it: the percentage of them strictly below it. High readings
    * mark a short-term overbought market and low readings an oversold one.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/crsi">ta-lib.org/functions/crsi</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>CRSI has no unstable period of its own: both RSI legs take {@code TA_FUNC_UNST_RSI}. They start warming up on the first bar the call reads (the streak's RSI one bar later, the streak being a change), so a leg that needs fewer bars than the lookback warms up over the spare ones.</li>
    * <li>Finite input is a precondition. A NaN close does not propagate: the output stays finite and no error is reported, but the values that follow it are wrong.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#crsiLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Close price series.
    * @param optInTimePeriod Period of the RSI of the closes (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInStreakPeriod Period of the RSI of the up/down streak (default
    *        2; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInRankPeriod Number of earlier one-bar returns each return is
    *        ranked against (default 100; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal The averaged reading, 0 to 100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#rsi
    * @see Core#percentrank
    * @see Core#stochrsi
    */
   public OutRange crsi( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         int optInStreakPeriod,
                         int optInRankPeriod,
                         double outReal[] )
   {
      requireIndexRange("CRSI", startIdx, endIdx);
      int guardStart = clampedStart("CRSI", startIdx, crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CRSI", "inReal", inReal, guardInLen);
      requireLength("CRSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = crsiImpl(startIdx, endIdx, inReal, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CRSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Connors RSI: a 0 to 100 oscillator, the plain average of three short-term
    * momentum readings on that same scale. The first is Wilder's RSI of the
    * closes over {@code optInTimePeriod} bars. The second is an RSI over
    * {@code optInStreakPeriod} bars of the up/down streak, the signed length of
    * the current run of higher or lower closes (+3 after three higher closes in
    * a row, -2 after two lower ones); an unchanged close resets the streak to
    * 0. The third is the percent rank of the bar's one-bar return, (close -
    * previous close) / previous close, among the {@code optInRankPeriod}
    * returns before it: the percentage of them strictly below it. High readings
    * mark a short-term overbought market and low readings an oversold one.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/crsi">ta-lib.org/functions/crsi</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>CRSI has no unstable period of its own: both RSI legs take {@code TA_FUNC_UNST_RSI}. They start warming up on the first bar the call reads (the streak's RSI one bar later, the streak being a change), so a leg that needs fewer bars than the lookback warms up over the spare ones.</li>
    * <li>Finite input is a precondition. A NaN close does not propagate: the output stays finite and no error is reported, but the values that follow it are wrong.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#crsiLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Close price series.
    * @param optInTimePeriod Period of the RSI of the closes (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInStreakPeriod Period of the RSI of the up/down streak (default
    *        2; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInRankPeriod Number of earlier one-bar returns each return is
    *        ranked against (default 100; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal The averaged reading, 0 to 100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#rsi
    * @see Core#percentrank
    * @see Core#stochrsi
    */
   public OutRange crsi( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         int optInStreakPeriod,
                         int optInRankPeriod,
                         double outReal[] )
   {
      requireIndexRange("CRSI", startIdx, endIdx);
      int guardStart = clampedStart("CRSI", startIdx, crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CRSI", "inReal", inReal, guardInLen);
      requireLength("CRSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = crsiImpl(startIdx, endIdx, inReal, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CRSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CRSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#crsi} over the same series.
    * Open with {@link Core#crsiOpen}; there is no close — the handle is
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
   public static final class CrsiStream {
      private Core core;
      private int optInTimePeriod;
      private int optInStreakPeriod;
      private int optInRankPeriod;
      private double prevClose;
      private double streak;
      private double cur_outReal;
      private RsiStream sub0;
      private RsiStream sub1;
      private RocpStream sub2;
      private PercentrankStream sub3;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CrsiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#crsi} reports over the same bars: the
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
            throw failure("CRSI advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CrsiStream( CrsiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInStreakPeriod = other.optInStreakPeriod;
         this.optInRankPeriod = other.optInRankPeriod;
         this.prevClose = other.prevClose;
         this.streak = other.streak;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new RsiStream(other.sub0);
         this.sub1 = new RsiStream(other.sub1);
         this.sub2 = new RocpStream(other.sub2);
         this.sub3 = new PercentrankStream(other.sub3);
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
            throw failure("CRSI update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("CRSI update: BAD_PARAM", RetCode.BAD_PARAM);
         core.crsiStepImpl(this, inReal);
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
            throw new TALibArgumentException("CRSI peek: BAD_PARAM", RetCode.BAD_PARAM);
         CrsiStream sp = this;
         double cur_tempStreak = 0.0;
         double cur_tempStreakRSI = 0.0;
         double cur_tempRSI = 0.0;
         double cur_outReal = 0.0;
         double close = 0.0;
         double streak = sp.streak;
         close = inReal;
         if( close > sp.prevClose ) {
            streak = (streak > 0.0) ? streak + 1.0 : 1.0;
         } else if( close < sp.prevClose ) {
            streak = (streak < 0.0) ? streak - 1.0 : 0 - 1.0;
         } else {
            streak = 0.0;
         }
         cur_tempStreak = streak;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempStreakRSI = sp.sub0.peek(cur_tempStreak);
         cur_tempRSI = sp.sub1.peek(inReal);
         cur_tempStreak = sp.sub2.peek(inReal);
         cur_outReal = sp.sub3.peek(cur_tempStreak);
         /* Combine map (batch tail, per bar). */
         cur_outReal = (cur_tempRSI + cur_tempStreakRSI + cur_outReal) / 3.0;
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
      public CrsiStream clone() {
         return new CrsiStream(this);
      }
   }
   private void crsiStepImpl( CrsiStream sp, double inReal )
   {
      double close = 0.0;
      double cur_tempStreak = 0.0;
      double cur_tempStreakRSI = 0.0;
      double cur_tempRSI = 0.0;
      double cur_outReal = 0.0;
      close = inReal;
      if( close > sp.prevClose ) {
         sp.streak = (sp.streak > 0.0) ? sp.streak + 1.0 : 1.0;
      } else if( close < sp.prevClose ) {
         sp.streak = (sp.streak < 0.0) ? sp.streak - 1.0 : 0 - 1.0;
      } else {
         sp.streak = 0.0;
      }
      cur_tempStreak = sp.streak;
      sp.prevClose = close;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempStreakRSI = sp.sub0.update(cur_tempStreak);
      cur_tempRSI = sp.sub1.update(inReal);
      cur_tempStreak = sp.sub2.update(inReal);
      cur_outReal = sp.sub3.update(cur_tempStreak);
      /* Combine map (batch tail, per bar). */
      cur_outReal = (cur_tempRSI + cur_tempStreakRSI + cur_outReal) / 3.0;
      sp.cur_outReal = cur_outReal;
   }
   private RetCode crsiOpenImpl( CrsiStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] tempRSI;
      double[] tempStreak;
      double[] tempStreakRSI;
      RetCode retCode;
      int lookbackTotal = 0;
      int anchorIdx = 0;
      int today = 0;
      int outIdx = 0;
      int i = 0;
      int offsetRSI = 0;
      int offsetStreak = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rsiNb = new MInteger();
      MInteger streakNb = new MInteger();
      MInteger rocNb = new MInteger();
      double prevClose = 0;
      double close = 0;
      double streak = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 3;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStreakPeriod == Integer.MIN_VALUE ) {
         optInStreakPeriod = 2;
      } else if( optInStreakPeriod < 2 || optInStreakPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInRankPeriod == Integer.MIN_VALUE ) {
         optInRankPeriod = 100;
      } else if( optInRankPeriod < 2 || optInRankPeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod);
      if( lookbackTotal > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Both RSI legs start on the bar lookbackTotal before startIdx (the streak,
       * a difference, one bar later), so the leg with the shorter lookback warms
       * up over the spare bars instead of starting cold at its own lookback.
       */
      anchorIdx = startIdx - lookbackTotal;
      tempStreak = new double[(int)((endIdx - anchorIdx) * 1)];
      tempRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInTimePeriod) + 1) * 1)];
      tempStreakRSI = new double[(int)((endIdx - anchorIdx - rsiLookback(optInStreakPeriod)) * 1)];
      streak = 0.0;
      prevClose = inReal[anchorIdx];
      outIdx = 0;
      today = anchorIdx + 1;
      while( today <= endIdx ) {
         close = inReal[today];
         if( close > prevClose ) {
            streak = (streak > 0.0) ? streak + 1.0 : 1.0;
         } else if( close < prevClose ) {
            streak = (streak < 0.0) ? streak - 1.0 : 0 - 1.0;
         } else {
            streak = 0.0;
         }
         tempStreak[outIdx++] = streak;
         prevClose = close;
         today += 1;
      }
      /* Sub-stream 0: rsi over `tempStreak`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RsiStream sub0 = rsiOpenAndFillInternal(java.util.Arrays.copyOfRange(tempStreak, 0, (outIdx - 1) + 1), 0, optInStreakPeriod, tempBegIdx, streakNb, tempStreakRSI);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 1: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RsiStream sub1 = rsiOpenAndFillInternal(inReal, anchorIdx + rsiLookback(optInTimePeriod), optInTimePeriod, tempBegIdx, rsiNb, tempRSI);
      retCode = RetCode.SUCCESS;
      /* The streak is consumed: its buffer holds the returns. */
      /* Sub-stream 2: rocp over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RocpStream sub2 = rocpOpenAndFillInternal(inReal, startIdx - optInRankPeriod, 1, tempBegIdx, rocNb, tempStreak);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 3: percentrank over `tempStreak`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      PercentrankStream sub3 = percentrankOpenAndFillInternal(java.util.Arrays.copyOfRange(tempStreak, 0, (rocNb.value - 1) + 1), 0, optInRankPeriod, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.SUCCESS;
      offsetRSI = rsiNb.value - outNBElement.value;
      offsetStreak = streakNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         sc_outReal[i] = (tempRSI[i + offsetRSI] + tempStreakRSI[i + offsetStreak] + sc_outReal[i]) / 3.0;
      }
      outBegIdx.value = startIdx;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInStreakPeriod = optInStreakPeriod;
      sp.optInRankPeriod = optInRankPeriod;
      sp.prevClose = prevClose;
      sp.streak = streak;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.sub3 = sub3;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* crsiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CrsiStream crsiOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CrsiStream sp = new CrsiStream(this);
      RetCode retCode = crsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("CRSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("CRSI openAndFill: internal error", retCode);
      }
      throw new TALibArgumentException("CRSI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind crsiOpen (composition seam). */
   CrsiStream crsiOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod )
   {
      CrsiStream sp = new CrsiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = crsiOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("CRSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("CRSI open: internal error", retCode);
      }
      throw new TALibArgumentException("CRSI open: " + retCode, retCode);
   }
   /**
    * Open a live CRSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#crsi} at that bar.
    * <p>The history must hold at least {@code crsiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CrsiStream crsiOpen( double inReal[], int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod )
   {
      requireArgument("CRSI open", "inReal", inReal);
      requireHistory("CRSI open", inReal.length);
      return crsiOpenInternal(inReal, 0, optInTimePeriod, optInStreakPeriod, optInRankPeriod);
   }
   /**
    * {@link Core#crsiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#crsi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CrsiStream#outRange()}.
    */
   public CrsiStream crsiOpenAndFill( double inReal[], int optInTimePeriod, int optInStreakPeriod, int optInRankPeriod, double outReal[] )
   {
      requireArgument("CRSI openAndFill", "inReal", inReal);
      requireHistory("CRSI openAndFill", inReal.length);
      int guardOutLen = openFillCount("CRSI openAndFill", inReal.length, crsiLookback(optInTimePeriod, optInStreakPeriod, optInRankPeriod));
      requireLength("CRSI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TALibArgumentException("CRSI openAndFill: " + RetCode.BAD_PARAM, RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return crsiOpenAndFillInternal(inReal, 0, optInTimePeriod, optInStreakPeriod, optInRankPeriod, outBegIdx, outNBElement, outReal);
   }
