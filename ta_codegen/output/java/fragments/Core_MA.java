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
    * Number of leading input bars {@link Core#MA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MA_Lookback( int optInTimePeriod, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      int retValue;
      if( optInTimePeriod <= 1 || optInMAType == MAType.DISABLED ) {
         return 0 ;
      }
      switch( optInMAType )
      {
      case SMA:
         retValue = SMA_Lookback(optInTimePeriod);
         break;
      case EMA:
         retValue = EMA_Lookback(optInTimePeriod);
         break;
      case WMA:
         retValue = WMA_Lookback(optInTimePeriod);
         break;
      case DEMA:
         retValue = DEMA_Lookback(optInTimePeriod);
         break;
      case TEMA:
         retValue = TEMA_Lookback(optInTimePeriod);
         break;
      case TRIMA:
         retValue = TRIMA_Lookback(optInTimePeriod);
         break;
      case KAMA:
         retValue = KAMA_Lookback(optInTimePeriod);
         break;
      case MAMA:
         retValue = MAMA_Lookback(0.5, 0.05);
         break;
      case T3:
         retValue = T3_Lookback(optInTimePeriod, 0.7);
         break;
      case HMA:
         retValue = HMA_Lookback(optInTimePeriod);
         break;
      default:
         retValue = 0;
         break;
      }
      return retValue ;

   }
   RetCode MA_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      /* Nothing to produce: the range is shorter than the lookback. Answer here
       * rather than forwarding.
       *
       * The VALUE is the same either way: ma_lookback returns exactly the lookback
       * the arm's callee computes for itself, from the same arguments the arm
       * passes it, and every callee clamps startIdx to that lookback and yields
       * 0,0 without reading. What changes is whose frame answers, and that is
       * visible to one caller only - the zero-length no-I/O probe. It hands the
       * body empty arrays on this range to check that nothing is read; forwarding
       * makes the callee's PUBLIC input bound (endIdx + 1 elements, no
       * sub-lookback escape) answer TA_BAD_PARAM before any array is reached, so
       * the probe cannot tell "read nothing" from "never ran". ma was the last
       * core withheld from that sweep for exactly this reason; apo, bbands, ppo,
       * pvo and stddev already carried this guard. No legitimate caller is
       * affected: ma's own public tier already requires endIdx + 1 input elements,
       * and on this range the callee's OUTPUT bound is 0, so a caller sizing by
       * the published formula was never rejected by it.
       *
       * It cannot mask a TA_BAD_PARAM. An optInMAType outside the enum is refused
       * by the generated entry point above this guard. An in-range member with no
       * arm here would reach ma_lookback's own default and get 0, and 0 > endIdx
       * is false for every endIdx the entry point admits, so control still reaches
       * the switch and still answers TA_BAD_PARAM. The identity path below is out
       * of the guard's reach for the same reason - its lookback is 0.
       */
      if( MA_Lookback(optInTimePeriod, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* No-smoothing identity: period 1 (every MA type) or the explicit
       * TA_MAType_DISABLED (any period, issue #93). One copy path, lookback 0.
       */
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
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
      case SMA:
         OutRange _xr0 = SMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr0.begIdx();
         outNBElement.value = _xr0.count();
         retCode = RetCode.Success;
         break;
      case EMA:
         OutRange _xr1 = EMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr1.begIdx();
         outNBElement.value = _xr1.count();
         retCode = RetCode.Success;
         break;
      case WMA:
         OutRange _xr2 = WMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr2.begIdx();
         outNBElement.value = _xr2.count();
         retCode = RetCode.Success;
         break;
      case DEMA:
         OutRange _xr3 = DEMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr3.begIdx();
         outNBElement.value = _xr3.count();
         retCode = RetCode.Success;
         break;
      case TEMA:
         OutRange _xr4 = TEMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr4.begIdx();
         outNBElement.value = _xr4.count();
         retCode = RetCode.Success;
         break;
      case TRIMA:
         OutRange _xr5 = TRIMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr5.begIdx();
         outNBElement.value = _xr5.count();
         retCode = RetCode.Success;
         break;
      case KAMA:
         OutRange _xr6 = KAMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr6.begIdx();
         outNBElement.value = _xr6.count();
         retCode = RetCode.Success;
         break;
      case MAMA:
         /* The optInTimePeriod is ignored. FAMA is a nullable output
          * (issue #125): pass NULL to compute only the MAMA line into outReal.
          */
         OutRange _xr7 = MAMA(startIdx, endIdx, inReal, 0.5, 0.05, outReal, null);
         outBegIdx.value = _xr7.begIdx();
         outNBElement.value = _xr7.count();
         retCode = RetCode.Success;
         break;
      case T3:
         OutRange _xr8 = T3(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outReal);
         outBegIdx.value = _xr8.begIdx();
         outNBElement.value = _xr8.count();
         retCode = RetCode.Success;
         break;
      case HMA:
         OutRange _xr9 = HMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr9.begIdx();
         outNBElement.value = _xr9.count();
         retCode = RetCode.Success;
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   RetCode MA_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( MA_Lookback(optInTimePeriod, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
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
      case SMA:
         OutRange _xr0 = SMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr0.begIdx();
         outNBElement.value = _xr0.count();
         retCode = RetCode.Success;
         break;
      case EMA:
         OutRange _xr1 = EMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr1.begIdx();
         outNBElement.value = _xr1.count();
         retCode = RetCode.Success;
         break;
      case WMA:
         OutRange _xr2 = WMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr2.begIdx();
         outNBElement.value = _xr2.count();
         retCode = RetCode.Success;
         break;
      case DEMA:
         OutRange _xr3 = DEMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr3.begIdx();
         outNBElement.value = _xr3.count();
         retCode = RetCode.Success;
         break;
      case TEMA:
         OutRange _xr4 = TEMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr4.begIdx();
         outNBElement.value = _xr4.count();
         retCode = RetCode.Success;
         break;
      case TRIMA:
         OutRange _xr5 = TRIMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr5.begIdx();
         outNBElement.value = _xr5.count();
         retCode = RetCode.Success;
         break;
      case KAMA:
         OutRange _xr6 = KAMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr6.begIdx();
         outNBElement.value = _xr6.count();
         retCode = RetCode.Success;
         break;
      case MAMA:
         OutRange _xr7 = MAMA(startIdx, endIdx, inReal, 0.5, 0.05, outReal, null);
         outBegIdx.value = _xr7.begIdx();
         outNBElement.value = _xr7.count();
         retCode = RetCode.Success;
         break;
      case T3:
         OutRange _xr8 = T3(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outReal);
         outBegIdx.value = _xr8.begIdx();
         outNBElement.value = _xr8.count();
         retCode = RetCode.Success;
         break;
      case HMA:
         OutRange _xr9 = HMA(startIdx, endIdx, inReal, optInTimePeriod, outReal);
         outBegIdx.value = _xr9.begIdx();
         outNBElement.value = _xr9.count();
         retCode = RetCode.Success;
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   /**
    * Generic moving-average dispatcher that forwards the job to the MA
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
    * <li>{@code TA_MAType_DEFAULT} selects the documented default of the parameter it is passed to — SMA here, EMA for APO, PPO and PVO. Every function that takes an MAType parameter accepts it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to average.
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT}
    *        selects the default).
    * @param outReal Selected moving average of the input. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#WMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#TRIMA
    * @see Core#KAMA
    * @see Core#MAMA
    * @see Core#T3
    * @see Core#HMA
    */
   public OutRange MA( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       MAType optInMAType,
                       double outReal[] )
   {
      requireIndexRange("MA", startIdx, endIdx);
      requireArgument("MA", "optInMAType", optInMAType);
      int guardStart = clampedStart("MA", startIdx, MA_Lookback(optInTimePeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MA", "inReal", inReal, guardInLen);
      requireLength("MA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MA_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Generic moving-average dispatcher that forwards the job to the MA
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
    * <li>{@code TA_MAType_DEFAULT} selects the documented default of the parameter it is passed to — SMA here, EMA for APO, PPO and PVO. Every function that takes an MAType parameter accepts it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to average.
    * @param optInTimePeriod Averaging window length (default 30; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Which moving-average algorithm to dispatch to (default
    *        0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA,
    *        7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT}
    *        selects the default).
    * @param outReal Selected moving average of the input. Must hold at least
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
    * @see Core#SMA
    * @see Core#EMA
    * @see Core#WMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#TRIMA
    * @see Core#KAMA
    * @see Core#MAMA
    * @see Core#T3
    * @see Core#HMA
    */
   public OutRange MA( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       MAType optInMAType,
                       double outReal[] )
   {
      requireIndexRange("MA", startIdx, endIdx);
      requireArgument("MA", "optInMAType", optInMAType);
      int guardStart = clampedStart("MA", startIdx, MA_Lookback(optInTimePeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MA", "inReal", inReal, guardInLen);
      requireLength("MA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MA_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MA} over the same series.
    * Open with {@link Core#maOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MaStream {
      Core core;
      int optInTimePeriod;
      MAType optInMAType;
      double cur_outReal;
      // Sub-stream, tagged by optInMAType; null on the identity path.
      Object sub;
      int outRangeBegIdx;
      int outRangeCount;

      MaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MaStream( MaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         if( other.sub == null ) {
            this.sub = null;
         } else {
            switch( this.optInMAType )
            {
            case SMA:
               this.sub = new SmaStream((SmaStream) other.sub);
               break;
            case EMA:
               this.sub = new EmaStream((EmaStream) other.sub);
               break;
            case WMA:
               this.sub = new WmaStream((WmaStream) other.sub);
               break;
            case DEMA:
               this.sub = new DemaStream((DemaStream) other.sub);
               break;
            case TEMA:
               this.sub = new TemaStream((TemaStream) other.sub);
               break;
            case TRIMA:
               this.sub = new TrimaStream((TrimaStream) other.sub);
               break;
            case KAMA:
               this.sub = new KamaStream((KamaStream) other.sub);
               break;
            case MAMA:
               this.sub = new MamaStream((MamaStream) other.sub);
               break;
            case T3:
               this.sub = new T3Stream((T3Stream) other.sub);
               break;
            case HMA:
               this.sub = new HmaStream((HmaStream) other.sub);
               break;
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( MaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         if( other.sub == null ) {
            this.sub = null;
         } else {
            switch( this.optInMAType )
            {
            case SMA:
               if( this.sub instanceof SmaStream ) {
                  ((SmaStream) this.sub).copyFrom((SmaStream) other.sub);
               } else {
                  this.sub = new SmaStream((SmaStream) other.sub);
               }
               break;
            case EMA:
               if( this.sub instanceof EmaStream ) {
                  ((EmaStream) this.sub).copyFrom((EmaStream) other.sub);
               } else {
                  this.sub = new EmaStream((EmaStream) other.sub);
               }
               break;
            case WMA:
               if( this.sub instanceof WmaStream ) {
                  ((WmaStream) this.sub).copyFrom((WmaStream) other.sub);
               } else {
                  this.sub = new WmaStream((WmaStream) other.sub);
               }
               break;
            case DEMA:
               if( this.sub instanceof DemaStream ) {
                  ((DemaStream) this.sub).copyFrom((DemaStream) other.sub);
               } else {
                  this.sub = new DemaStream((DemaStream) other.sub);
               }
               break;
            case TEMA:
               if( this.sub instanceof TemaStream ) {
                  ((TemaStream) this.sub).copyFrom((TemaStream) other.sub);
               } else {
                  this.sub = new TemaStream((TemaStream) other.sub);
               }
               break;
            case TRIMA:
               if( this.sub instanceof TrimaStream ) {
                  ((TrimaStream) this.sub).copyFrom((TrimaStream) other.sub);
               } else {
                  this.sub = new TrimaStream((TrimaStream) other.sub);
               }
               break;
            case KAMA:
               if( this.sub instanceof KamaStream ) {
                  ((KamaStream) this.sub).copyFrom((KamaStream) other.sub);
               } else {
                  this.sub = new KamaStream((KamaStream) other.sub);
               }
               break;
            case MAMA:
               if( this.sub instanceof MamaStream ) {
                  ((MamaStream) this.sub).copyFrom((MamaStream) other.sub);
               } else {
                  this.sub = new MamaStream((MamaStream) other.sub);
               }
               break;
            case T3:
               if( this.sub instanceof T3Stream ) {
                  ((T3Stream) this.sub).copyFrom((T3Stream) other.sub);
               } else {
                  this.sub = new T3Stream((T3Stream) other.sub);
               }
               break;
            case HMA:
               if( this.sub instanceof HmaStream ) {
                  ((HmaStream) this.sub).copyFrom((HmaStream) other.sub);
               } else {
                  this.sub = new HmaStream((HmaStream) other.sub);
               }
               break;
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<MaStream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MA update: BadParam", RetCode.BadParam);
         core.maStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("MA updateAndFill", "inReal", inReal);
         requireArgument("MA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("MA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) )
               throw new TaLibArgumentException("MA updateAndFill: BadParam", RetCode.BadParam);
            core.maStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MA peek: BadParam", RetCode.BadParam);
         MaStream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new MaStream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.maStepImpl(scratch, inReal);
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
      public MaStream copy() {
         return new MaStream(this);
      }
   }
   void maStepImpl( MaStream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 || sp.optInMAType == MAType.DISABLED ) {
         sp.cur_outReal = inReal;
         return;
      }
      switch( sp.optInMAType )
      {
      case SMA: {
         sp.cur_outReal = ((SmaStream) sp.sub).update(inReal);
         break;
      }
      case EMA: {
         sp.cur_outReal = ((EmaStream) sp.sub).update(inReal);
         break;
      }
      case WMA: {
         sp.cur_outReal = ((WmaStream) sp.sub).update(inReal);
         break;
      }
      case DEMA: {
         sp.cur_outReal = ((DemaStream) sp.sub).update(inReal);
         break;
      }
      case TEMA: {
         sp.cur_outReal = ((TemaStream) sp.sub).update(inReal);
         break;
      }
      case TRIMA: {
         sp.cur_outReal = ((TrimaStream) sp.sub).update(inReal);
         break;
      }
      case KAMA: {
         sp.cur_outReal = ((KamaStream) sp.sub).update(inReal);
         break;
      }
      case MAMA: {
         MamaStream.Value subValue = ((MamaStream) sp.sub).update(inReal);
         sp.cur_outReal = subValue.mama();
         break;
      }
      case T3: {
         sp.cur_outReal = ((T3Stream) sp.sub).update(inReal);
         break;
      }
      case HMA: {
         sp.cur_outReal = ((HmaStream) sp.sub).update(inReal);
         break;
      }
      default:
         break; /* unreachable: open rejects arms without a sub-stream */
      }
   }
   private RetCode maOpenImpl( MaStream sp, double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         sp.cur_outReal = inReal[historyLen - 1];
         int fillLb = MA_Lookback(optInTimePeriod, optInMAType);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.outRangeBegIdx = fillLb;
         sp.outRangeCount = historyLen - fillLb;
         return RetCode.Success;
      }
      switch( optInMAType )
      {
      case SMA: {
         SmaStream sub = smaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EmaStream sub = emaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WmaStream sub = wmaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DemaStream sub = demaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TemaStream sub = temaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TrimaStream sub = trimaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KamaStream sub = kamaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MamaStream sub = mamaOpenInternal(inReal, startIdx, 0.5, 0.05);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenInternal(inReal, startIdx, optInTimePeriod, 0.7);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HmaStream sub = hmaOpenInternal(inReal, startIdx, optInTimePeriod);
         sp.outRangeBegIdx = sub.outRangeBegIdx;
         sp.outRangeCount = sub.outRangeCount;
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
   private RetCode maOpenAndFillImpl( MaStream sp, double inReal[], int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         int fillLb = MA_Lookback(optInTimePeriod, optInMAType);
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
      case SMA: {
         SmaStream sub = smaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EmaStream sub = emaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WmaStream sub = wmaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DemaStream sub = demaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TemaStream sub = temaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TrimaStream sub = trimaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KamaStream sub = kamaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MamaStream sub = mamaOpenAndFill(inReal, 0.5, 0.05, outReal, null);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenAndFill(inReal, optInTimePeriod, 0.7, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HmaStream sub = hmaOpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.outRangeBegIdx;
         outNBElement.value = sub.outRangeCount;
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
   private RetCode maOpenAndFillInternalImpl( MaStream sp, double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         int fillLb = MA_Lookback(optInTimePeriod, optInMAType);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
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
      case SMA: {
         SmaStream sub = smaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EmaStream sub = emaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WmaStream sub = wmaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DemaStream sub = demaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TemaStream sub = temaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TrimaStream sub = trimaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KamaStream sub = kamaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MamaStream sub = mamaOpenAndFillInternal(inReal, startIdx, 0.5, 0.05, outBegIdx, outNBElement, outReal, null);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenAndFillInternal(inReal, startIdx, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HmaStream sub = hmaOpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
   /* Internal startIdx-anchored open behind maOpen (composition seam). */
   MaStream maOpenInternal( double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      MaStream sp = new MaStream(this);
      RetCode retCode = maOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MA open: internal error", retCode);
      }
      throw new TaLibArgumentException("MA open: " + retCode, retCode);
   }
   /**
    * Open a live MA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MA} at that bar.
    * <p>The history must hold at least {@code MA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MaStream maOpen( double inReal[], int optInTimePeriod, MAType optInMAType )
   {
      requireArgument("MA open", "inReal", inReal);
      requireHistory("MA open", inReal.length);
      requireArgument("MA open", "optInMAType", optInMAType);
      return maOpenInternal(inReal, 0, optInTimePeriod, optInMAType);
   }
   /**
    * {@link Core#maOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MaStream#outRange()}.
    */
   public MaStream maOpenAndFill( double inReal[], int optInTimePeriod, MAType optInMAType, double outReal[] )
   {
      requireArgument("MA openAndFill", "inReal", inReal);
      requireHistory("MA openAndFill", inReal.length);
      requireArgument("MA openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("MA openAndFill", inReal.length, MA_Lookback(optInTimePeriod, optInMAType));
      requireLength("MA openAndFill", "outReal", outReal, guardOutLen);
      MaStream sp = new MaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = maOpenAndFillImpl(sp, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MA openAndFill: " + retCode, retCode);
   }
   /* maOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MaStream maOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MaStream sp = new MaStream(this);
      RetCode retCode = maOpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MA openAndFill: " + retCode, retCode);
   }
