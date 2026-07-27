/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  022203 MF   Add MAMA
 *  040503 MF   Add T3
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  111603 MF   Allow period of 1. Just copy input into output.
 *  060907 MF   Use TA_SMA/TA_EMA instead of internal implementation.
 *  072226 MF,CC Add HMA (issue #139).
 *  072426 MF,CC TA_MAType_DISABLED: period-independent identity copy (issue #93).
 */

   /**
    * Number of leading input bars {@link Core#movingAverage} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int movingAverageLookback( int optInTimePeriod, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      if( optInTimePeriod <= 1 || optInMAType == MAType.Disabled ) {
         return 0 ;
      }
      switch( optInMAType )
      {
      case Sma:
         retValue = smaLookback(optInTimePeriod);
         break;
      case Ema:
         retValue = emaLookback(optInTimePeriod);
         break;
      case Wma:
         retValue = wmaLookback(optInTimePeriod);
         break;
      case Dema:
         retValue = demaLookback(optInTimePeriod);
         break;
      case Tema:
         retValue = temaLookback(optInTimePeriod);
         break;
      case Trima:
         retValue = trimaLookback(optInTimePeriod);
         break;
      case Kama:
         retValue = kamaLookback(optInTimePeriod);
         break;
      case Mama:
         retValue = mamaLookback(0.5, 0.05);
         break;
      case T3:
         retValue = t3Lookback(optInTimePeriod, 0.7);
         break;
      case Hma:
         retValue = hmaLookback(optInTimePeriod);
         break;
      default:
         retValue = 0;
         break;
      }
      return retValue ;

   }
   RetCode movingAverageInternal( int startIdx,
                                  int endIdx,
                                  double inReal[],
                                  int optInTimePeriod,
                                  MAType optInMAType,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* No-smoothing identity: period 1 (every MA type) or the explicit
       * TA_MAType_DISABLED (any period, issue #93). One copy path, lookback 0.
       */
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      /* Simply forward the job to the corresponding TA function. */
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         /* The optInTimePeriod is ignored. FAMA is a nullable output
          * (issue #125): pass NULL to compute only the MAMA line into outReal.
          */
         retCode = mamaUnguardedInternal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3UnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case Hma:
         retCode = hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   RetCode movingAverageUnguardedInternal( int startIdx,
                                           int endIdx,
                                           double inReal[],
                                           int optInTimePeriod,
                                           MAType optInMAType,
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           double outReal[] )
   {
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguardedInternal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3UnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case Hma:
         retCode = hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   RetCode movingAverageInternal( int startIdx,
                                  int endIdx,
                                  float inReal[],
                                  int optInTimePeriod,
                                  MAType optInMAType,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = (double)inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguardedInternal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3UnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case Hma:
         retCode = hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   RetCode movingAverageUnguardedInternal( int startIdx,
                                           int endIdx,
                                           float inReal[],
                                           int optInTimePeriod,
                                           MAType optInMAType,
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           double outReal[] )
   {
      RetCode retCode;
      int nbElement = 0;
      int outIdx = 0;
      int todayIdx = 0;
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         nbElement = endIdx - startIdx + 1;
         outNBElement.value = nbElement;
         for( todayIdx = startIdx, outIdx = 0; outIdx < nbElement; outIdx += 1, todayIdx += 1 ) {
            outReal[outIdx] = (double)inReal[todayIdx];
         }
         outBegIdx.value = startIdx;
         return RetCode.Success ;
      }
      switch( optInMAType )
      {
      case Sma:
         retCode = smaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguardedInternal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3UnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case Hma:
         retCode = hmaUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   /**
    * Generic moving-average dispatcher that forwards the job to a concrete MA
    * implementation selected by optInMAType. Single uniform interface over all
    * TA-Lib moving averages.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal = MA_of_type(optInMAType)(inReal, optInTimePeriod); default type = SMA
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing for every MAType: the output is a copy of the input.</li>
    * <li>{@code TA_MAType_DISABLED} bypasses smoothing explicitly, for any period: the output is a copy of the input with a lookback of 0. Every function that takes an MAType parameter accepts it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#movingAverageLookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to average.
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED).
    * @param outReal Selected moving average of the input. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#sma
    * @see Core#ema
    * @see Core#wma
    * @see Core#dema
    * @see Core#tema
    * @see Core#trima
    * @see Core#kama
    * @see Core#mama
    * @see Core#t3
    * @see Core#hma
    */
   public OutRange movingAverage( int startIdx,
                                  int endIdx,
                                  double inReal[],
                                  int optInTimePeriod,
                                  MAType optInMAType,
                                  double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageInternal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Generic moving-average dispatcher that forwards the job to a concrete MA
    * implementation selected by optInMAType. Single uniform interface over all
    * TA-Lib moving averages. — <b>unchecked</b> variant of
    * {@link Core#movingAverage}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange movingAverageUnguarded( int startIdx,
                                           int endIdx,
                                           double inReal[],
                                           int optInTimePeriod,
                                           MAType optInMAType,
                                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      movingAverageUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Generic moving-average dispatcher that forwards the job to a concrete MA
    * implementation selected by optInMAType. Single uniform interface over all
    * TA-Lib moving averages.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal = MA_of_type(optInMAType)(inReal, optInTimePeriod); default type = SMA
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing for every MAType: the output is a copy of the input.</li>
    * <li>{@code TA_MAType_DISABLED} bypasses smoothing explicitly, for any period: the output is a copy of the input with a lookback of 0. Every function that takes an MAType parameter accepts it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#movingAverageLookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to average.
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED).
    * @param outReal Selected moving average of the input. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#sma
    * @see Core#ema
    * @see Core#wma
    * @see Core#dema
    * @see Core#tema
    * @see Core#trima
    * @see Core#kama
    * @see Core#mama
    * @see Core#t3
    * @see Core#hma
    */
   public OutRange movingAverage( int startIdx,
                                  int endIdx,
                                  float inReal[],
                                  int optInTimePeriod,
                                  MAType optInMAType,
                                  double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageInternal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Generic moving-average dispatcher that forwards the job to a concrete MA
    * implementation selected by optInMAType. Single uniform interface over all
    * TA-Lib moving averages. — <b>unchecked</b> variant of
    * {@link Core#movingAverage}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    * <p>This is the {@code float[]} overload; see the guarded method.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange movingAverageUnguarded( int startIdx,
                                           int endIdx,
                                           float inReal[],
                                           int optInTimePeriod,
                                           MAType optInMAType,
                                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      movingAverageUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#movingAverage} over the same series.
    * Open with {@link Core#movingAverageOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MovingAverageStream {
      final Core core;
      int optInTimePeriod;
      MAType optInMAType;
      double cur_outReal;
      // Sub-stream, tagged by optInMAType; null on the identity path.
      Object sub;
      OutRange fillRange;

      MovingAverageStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#movingAverageOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      MovingAverageStream( MovingAverageStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         if( other.sub == null ) {
            this.sub = null;
         } else {
            switch( this.optInMAType )
            {
            case Sma:
               this.sub = new SmaStream((SmaStream) other.sub);
               break;
            case Ema:
               this.sub = new EmaStream((EmaStream) other.sub);
               break;
            case Wma:
               this.sub = new WmaStream((WmaStream) other.sub);
               break;
            case Dema:
               this.sub = new DemaStream((DemaStream) other.sub);
               break;
            case Tema:
               this.sub = new TemaStream((TemaStream) other.sub);
               break;
            case Trima:
               this.sub = new TrimaStream((TrimaStream) other.sub);
               break;
            case Kama:
               this.sub = new KamaStream((KamaStream) other.sub);
               break;
            case Mama:
               this.sub = new MamaStream((MamaStream) other.sub);
               break;
            case T3:
               this.sub = new T3Stream((T3Stream) other.sub);
               break;
            case Hma:
               this.sub = new HmaStream((HmaStream) other.sub);
               break;
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.movingAverageStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         MovingAverageStream scratch = new MovingAverageStream(this);
         core.movingAverageStreamStep(scratch, inReal);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MovingAverageStream copy() {
         return new MovingAverageStream(this);
      }
   }
   void movingAverageStreamStep( MovingAverageStream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 || sp.optInMAType == MAType.Disabled ) {
         sp.cur_outReal = inReal;
         return;
      }
      switch( sp.optInMAType )
      {
      case Sma: {
         sp.cur_outReal = ((SmaStream) sp.sub).update(inReal);
         break;
      }
      case Ema: {
         sp.cur_outReal = ((EmaStream) sp.sub).update(inReal);
         break;
      }
      case Wma: {
         sp.cur_outReal = ((WmaStream) sp.sub).update(inReal);
         break;
      }
      case Dema: {
         sp.cur_outReal = ((DemaStream) sp.sub).update(inReal);
         break;
      }
      case Tema: {
         sp.cur_outReal = ((TemaStream) sp.sub).update(inReal);
         break;
      }
      case Trima: {
         sp.cur_outReal = ((TrimaStream) sp.sub).update(inReal);
         break;
      }
      case Kama: {
         sp.cur_outReal = ((KamaStream) sp.sub).update(inReal);
         break;
      }
      case Mama: {
         MamaStream.Value subValue = ((MamaStream) sp.sub).update(inReal);
         sp.cur_outReal = subValue.mama;
         break;
      }
      case T3: {
         sp.cur_outReal = ((T3Stream) sp.sub).update(inReal);
         break;
      }
      case Hma: {
         sp.cur_outReal = ((HmaStream) sp.sub).update(inReal);
         break;
      }
      default:
         break; /* unreachable: open rejects arms without a sub-stream */
      }
   }
   private RetCode movingAverageOpenBody( MovingAverageStream sp, double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( historyLen < movingAverageLookback(optInTimePeriod, optInMAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         if( historyLen < movingAverageLookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         sp.cur_outReal = inReal[historyLen - 1];
         return RetCode.Success;
      }
      switch( optInMAType )
      {
      case Sma: {
         SmaStream sub = smaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Ema: {
         EmaStream sub = emaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Wma: {
         WmaStream sub = wmaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Dema: {
         DemaStream sub = demaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Tema: {
         TemaStream sub = temaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Trima: {
         TrimaStream sub = trimaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Kama: {
         KamaStream sub = kamaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Mama: {
         MamaStream sub = mamaOpenInternal(inReal, startIdx, 0.5, 0.05);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenInternal(inReal, startIdx, optInTimePeriod, 0.7);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Hma: {
         HmaStream sub = hmaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      default:
         return RetCode.BadParam;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMAType = optInMAType;
      return RetCode.Success;
   }
   private RetCode movingAverageOpenAndFillBody( MovingAverageStream sp, double inReal[], int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( historyLen < movingAverageLookback(optInTimePeriod, optInMAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.Disabled ) {
         if( historyLen < movingAverageLookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         int fillLb = movingAverageLookback(optInTimePeriod, optInMAType);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
      switch( optInMAType )
      {
      case Sma: {
         SmaStream sub = smaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Ema: {
         EmaStream sub = emaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Wma: {
         WmaStream sub = wmaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Dema: {
         DemaStream sub = demaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Tema: {
         TemaStream sub = temaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Trima: {
         TrimaStream sub = trimaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Kama: {
         KamaStream sub = kamaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Mama: {
         MamaStream sub = mamaOpenAndFill(inReal, 0.5, 0.05, outReal, new double[historyLen]);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenAndFill(inReal, optInTimePeriod, 0.7, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Hma: {
         HmaStream sub = hmaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      default:
         return RetCode.BadParam;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMAType = optInMAType;
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind movingAverageOpen (composition seam). */
   MovingAverageStream movingAverageOpenInternal( double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      MovingAverageStream sp = new MovingAverageStream(this);
      RetCode retCode = movingAverageOpenBody(sp, inReal, startIdx, optInTimePeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MA open: internal error");
      }
      throw new IllegalArgumentException("TA_MA open: " + retCode);
   }
   /**
    * Open a live MA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#movingAverage} at that bar.
    * <p>The history must hold at least {@code movingAverageLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MovingAverageStream movingAverageOpen( double inReal[], int optInTimePeriod, MAType optInMAType )
   {
      return movingAverageOpenInternal(inReal, 0, optInTimePeriod, optInMAType);
   }
   /**
    * {@link Core#movingAverageOpen} that also fills the output array(s) bit-identically
    * to {@link Core#movingAverage} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MovingAverageStream#fillRange()}.
    */
   public MovingAverageStream movingAverageOpenAndFill( double inReal[], int optInTimePeriod, MAType optInMAType, double outReal[] )
   {
      MovingAverageStream sp = new MovingAverageStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageOpenAndFillBody(sp, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MA openAndFill: " + retCode);
   }
