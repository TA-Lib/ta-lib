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
 *  112126 MF,CC  First version (issue #610).
 */

   /**
    * Number of leading input bars {@link Core#maenv} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Periods of the moving average (default 20; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentUp Percent of the moving average added to form the
    *        upper band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInPercentDn Percent of the moving average subtracted to form the
    *        lower band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA,
    *        16=SSF; {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int maenvLookback( int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType )
   {
      if( optInMAType == null ) {
         return -1;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInPercentUp == REAL_DEFAULT ) {
         optInPercentUp = 1e1;
      } else if( !(optInPercentUp >= REAL_MIN && optInPercentUp <= REAL_MAX) ) {
         return -1;
      }
      if( optInPercentDn == REAL_DEFAULT ) {
         optInPercentDn = 1e1;
      } else if( !(optInPercentDn >= REAL_MIN && optInPercentDn <= REAL_MAX) ) {
         return -1;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      return maLookback(optInTimePeriod, optInMAType) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#maenv}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Periods of the moving average (default 20; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentUp Percent of the moving average added to form the
    *        upper band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInPercentDn Percent of the moving average subtracted to form the
    *        lower band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA,
    *        16=SSF; {@code MAType.DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int maenvDisplayShift( int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType, int outputIdx )
   {
      if( maenvLookback( optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 3 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode maenvImpl( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInTimePeriod,
                      double optInPercentUp,
                      double optInPercentDn,
                      MAType optInMAType,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outRealUpperBand[],
                      double outRealMiddleBand[],
                      double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      double upFactor = 0;
      double dnFactor = 0;
      double middle = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentUp == REAL_DEFAULT ) {
         optInPercentUp = 1e1;
      } else if( !(optInPercentUp >= REAL_MIN && optInPercentUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentDn == REAL_DEFAULT ) {
         optInPercentDn = 1e1;
      } else if( !(optInPercentDn >= REAL_MIN && optInPercentDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BAD_PARAM ;
      }
      if( maLookback(optInTimePeriod, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outRealMiddleBand);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      upFactor = 1.0 + optInPercentUp / 100.0;
      dnFactor = 1.0 - optInPercentDn / 100.0;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = outRealMiddleBand[i];
         outRealUpperBand[i] = middle * upFactor;
         outRealLowerBand[i] = middle * dnFactor;
      }
      return RetCode.SUCCESS ;
   }
   RetCode maenvImpl( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInTimePeriod,
                      double optInPercentUp,
                      double optInPercentDn,
                      MAType optInMAType,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outRealUpperBand[],
                      double outRealMiddleBand[],
                      double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      double upFactor = 0;
      double dnFactor = 0;
      double middle = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentUp == REAL_DEFAULT ) {
         optInPercentUp = 1e1;
      } else if( !(optInPercentUp >= REAL_MIN && optInPercentUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentDn == REAL_DEFAULT ) {
         optInPercentDn = 1e1;
      } else if( !(optInPercentDn >= REAL_MIN && optInPercentDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BAD_PARAM ;
      }
      if( maLookback(optInTimePeriod, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outRealMiddleBand);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      upFactor = 1.0 + optInPercentUp / 100.0;
      dnFactor = 1.0 - optInPercentDn / 100.0;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = outRealMiddleBand[i];
         outRealUpperBand[i] = middle * upFactor;
         outRealLowerBand[i] = middle * dnFactor;
      }
      return RetCode.SUCCESS ;
   }
   /**
    * Moving Average Envelope: a moving average with one band a fixed percentage
    * above it and one a fixed percentage below. The bands keep a constant
    * proportional distance from the average, so price reaching a band reads as
    * stretched relative to its recent mean, and a move beyond one as unusual
    * strength or weakness.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/maenv">ta-lib.org/functions/maenv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The percentages are in percent: 10, not 0.1, places a band 10% from the moving average.</li>
    * <li>Each band has its own percentage; the usual single-percentage envelope is the case where both are equal. A negative percentage puts its band on the other side of the average.</li>
    * <li><b>The input is assumed positive.</b> Each band sits a percent of the moving average away from it. Where the average is 0 both bands equal it, and where it is negative the two bands trade places; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.</li>
    * <li>A displaced envelope is a shift of the plot by a number of bars. The outputs are not displaced.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#maenvLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInTimePeriod Periods of the moving average (default 20; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentUp Percent of the moving average added to form the
    *        upper band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInPercentDn Percent of the moving average subtracted to form the
    *        lower band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA,
    *        16=SSF; {@code MAType.DEFAULT} selects the default).
    * @param outRealUpperBand The moving average plus the upper percentage of
    *        it. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outRealMiddleBand The moving average. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outRealLowerBand The moving average minus the lower percentage of
    *        it. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    * @see Core#ma
    * @see Core#bbands
    * @see Core#kc
    * @see Core#accbands
    */
   public OutRange maenv( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          double optInPercentUp,
                          double optInPercentDn,
                          MAType optInMAType,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      requireIndexRange("MAENV", startIdx, endIdx);
      requireArgument("MAENV", "optInMAType", optInMAType);
      int guardStart = clampedStart("MAENV", startIdx, maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAENV", "inReal", inReal, guardInLen);
      requireLength("MAENV", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("MAENV", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("MAENV", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = maenvImpl(startIdx, endIdx, inReal, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MAENV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Moving Average Envelope: a moving average with one band a fixed percentage
    * above it and one a fixed percentage below. The bands keep a constant
    * proportional distance from the average, so price reaching a band reads as
    * stretched relative to its recent mean, and a move beyond one as unusual
    * strength or weakness.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/maenv">ta-lib.org/functions/maenv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The percentages are in percent: 10, not 0.1, places a band 10% from the moving average.</li>
    * <li>Each band has its own percentage; the usual single-percentage envelope is the case where both are equal. A negative percentage puts its band on the other side of the average.</li>
    * <li><b>The input is assumed positive.</b> Each band sits a percent of the moving average away from it. Where the average is 0 both bands equal it, and where it is negative the two bands trade places; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.</li>
    * <li>A displaced envelope is a shift of the plot by a number of bars. The outputs are not displaced.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#maenvLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInTimePeriod Periods of the moving average (default 20; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInPercentUp Percent of the moving average added to form the
    *        upper band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInPercentDn Percent of the moving average subtracted to form the
    *        lower band (default 10; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA,
    *        16=SSF; {@code MAType.DEFAULT} selects the default).
    * @param outRealUpperBand The moving average plus the upper percentage of
    *        it. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outRealMiddleBand The moving average. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outRealLowerBand The moving average minus the lower percentage of
    *        it. Must hold at least
    *        {@code endIdx - max(startIdx, maenvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    * @see Core#ma
    * @see Core#bbands
    * @see Core#kc
    * @see Core#accbands
    */
   public OutRange maenv( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          double optInPercentUp,
                          double optInPercentDn,
                          MAType optInMAType,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      requireIndexRange("MAENV", startIdx, endIdx);
      requireArgument("MAENV", "optInMAType", optInMAType);
      int guardStart = clampedStart("MAENV", startIdx, maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAENV", "inReal", inReal, guardInLen);
      requireLength("MAENV", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("MAENV", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("MAENV", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = maenvImpl(startIdx, endIdx, inReal, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MAENV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MAENV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#maenv} over the same series.
    * Open with {@link Core#maenvOpen}; there is no close — the handle is
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
   public static final class MaenvStream {
      private Core core;
      private int optInTimePeriod;
      private double optInPercentUp;
      private double optInPercentDn;
      private MAType optInMAType;
      private double cur_outRealUpperBand;
      private double cur_outRealMiddleBand;
      private double cur_outRealLowerBand;
      private double dnFactor;
      private double upFactor;
      private MaStream sub0;
      private int outRangeBegIdx;
      private int outRangeCount;

      private MaenvStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#maenv} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(MaenvOut)} keeps answering the previous
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
            throw failure("MAENV advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private MaenvStream( MaenvStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInPercentUp = other.optInPercentUp;
         this.optInPercentDn = other.optInPercentDn;
         this.optInMAType = other.optInMAType;
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.dnFactor = other.dnFactor;
         this.upFactor = other.upFactor;
         this.sub0 = new MaStream(other.sub0);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(MaenvOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inReal, MaenvOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("MAENV update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("MAENV update", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MAENV update", "inReal");
         core.maenvStepImpl(this, inReal);
         this.outRangeCount++;
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inReal, MaenvOut out ) {
         requireArgument("MAENV peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("MAENV peek", "inReal");
         MaenvStream sp = this;
         double middle = 0.0;
         double cur_outRealMiddleBand = 0.0;
         double cur_outRealUpperBand = 0.0;
         double cur_outRealLowerBand = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_outRealMiddleBand = sp.sub0.peek(inReal);
         double dnFactor = sp.dnFactor;
         double upFactor = sp.upFactor;
         /* Combine map (batch tail, per bar). */
         middle = cur_outRealMiddleBand;
         cur_outRealUpperBand = middle * upFactor;
         cur_outRealLowerBand = middle * dnFactor;
         out.realUpperBand = cur_outRealUpperBand;
         out.realMiddleBand = cur_outRealMiddleBand;
         out.realLowerBand = cur_outRealLowerBand;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( MaenvOut out ) {
         requireArgument("MAENV value", "out", out);
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public MaenvStream clone() {
         return new MaenvStream(this);
      }
   }

   /**
    * The outputs of one MAENV bar, written by the stream into an object the
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
   public static final class MaenvOut {
      /** The moving average plus the upper percentage of it. */
      public double realUpperBand;
      /** The moving average. */
      public double realMiddleBand;
      /** The moving average minus the lower percentage of it. */
      public double realLowerBand;
   }
   private void maenvStepImpl( MaenvStream sp, double inReal )
   {
      double middle = 0.0;
      double cur_outRealMiddleBand = 0.0;
      double cur_outRealUpperBand = 0.0;
      double cur_outRealLowerBand = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_outRealMiddleBand = sp.sub0.update(inReal);
      double dnFactor = sp.dnFactor;
      double upFactor = sp.upFactor;
      /* Combine map (batch tail, per bar). */
      middle = cur_outRealMiddleBand;
      cur_outRealUpperBand = middle * upFactor;
      cur_outRealLowerBand = middle * dnFactor;
      sp.cur_outRealUpperBand = cur_outRealUpperBand;
      sp.cur_outRealMiddleBand = cur_outRealMiddleBand;
      sp.cur_outRealLowerBand = cur_outRealLowerBand;
   }
   private RetCode maenvOpenImpl( MaenvStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      double upFactor = 0;
      double dnFactor = 0;
      double middle = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentUp == REAL_DEFAULT ) {
         optInPercentUp = 1e1;
      } else if( !(optInPercentUp >= REAL_MIN && optInPercentUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInPercentDn == REAL_DEFAULT ) {
         optInPercentDn = 1e1;
      } else if( !(optInPercentDn >= REAL_MIN && optInPercentDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( startIdx >= historyLen ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outRealUpperBand = outStride == 1 ? outRealUpperBand : new double[historyLen];
      double[] sc_outRealMiddleBand = outStride == 1 ? outRealMiddleBand : new double[historyLen];
      double[] sc_outRealLowerBand = outStride == 1 ? outRealLowerBand : new double[historyLen];
      if( maLookback(optInTimePeriod, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, startIdx, optInTimePeriod, optInMAType, outBegIdx, outNBElement, sc_outRealMiddleBand);
      retCode = RetCode.SUCCESS;
      upFactor = 1.0 + optInPercentUp / 100.0;
      dnFactor = 1.0 - optInPercentDn / 100.0;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         middle = sc_outRealMiddleBand[i];
         sc_outRealUpperBand[i] = middle * upFactor;
         sc_outRealLowerBand[i] = middle * dnFactor;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInPercentUp = optInPercentUp;
      sp.optInPercentDn = optInPercentDn;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.dnFactor = dnFactor;
      sp.upFactor = upFactor;
      sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
      sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
      sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* maenvOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MaenvStream maenvOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      MaenvStream sp = new MaenvStream(this);
      RetCode retCode = maenvOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MAENV openAndFill", inReal.length, startIdx, maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType));
      }
      throw streamFailure("MAENV openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind maenvOpen (composition seam). */
   MaenvStream maenvOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType )
   {
      MaenvStream sp = new MaenvStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealUpperBand = new double[1];
      double[] sink_outRealMiddleBand = new double[1];
      double[] sink_outRealLowerBand = new double[1];
      RetCode retCode = maenvOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MAENV open", inReal.length, startIdx, maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType));
      }
      throw streamFailure("MAENV open", retCode);
   }
   /**
    * Open a live MAENV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#maenv} at that bar.
    * <p>The history must hold at least {@code maenvLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE}, {@link Core#REAL_DEFAULT} and
    * {@link MAType#DEFAULT} select a parameter's documented default, as in
    * the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public MaenvStream maenvOpen( double inReal[], int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType )
   {
      requireArgument("MAENV open", "inReal", inReal);
      requireHistory("MAENV open", inReal.length);
      requireArgument("MAENV open", "optInMAType", optInMAType);
      return maenvOpenInternal(inReal, 0, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType);
   }
   /**
    * {@link Core#maenvOpen} that also fills the output array(s) bit-identically
    * to {@link Core#maenv} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MaenvStream#outRange()}.
    */
   public MaenvStream maenvOpenAndFill( double inReal[], int optInTimePeriod, double optInPercentUp, double optInPercentDn, MAType optInMAType, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      requireArgument("MAENV openAndFill", "inReal", inReal);
      requireHistory("MAENV openAndFill", inReal.length);
      requireArgument("MAENV openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("MAENV openAndFill", inReal.length, maenvLookback(optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType));
      requireLength("MAENV openAndFill", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("MAENV openAndFill", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("MAENV openAndFill", "outRealLowerBand", outRealLowerBand, guardOutLen);
      if( (Object)outRealUpperBand == (Object)inReal || (Object)outRealMiddleBand == (Object)inReal || (Object)outRealLowerBand == (Object)inReal || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         throw streamFailure("MAENV openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return maenvOpenAndFillInternal(inReal, 0, optInTimePeriod, optInPercentUp, optInPercentDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
   }
