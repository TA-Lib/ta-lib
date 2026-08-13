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
   RetCode MA_Internal( int startIdx,
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
         retCode = SMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case EMA:
         retCode = EMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case WMA:
         retCode = WMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case DEMA:
         retCode = DEMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case TEMA:
         retCode = TEMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case TRIMA:
         retCode = TRIMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case KAMA:
         retCode = KAMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case MAMA:
         /* The optInTimePeriod is ignored. FAMA is a nullable output
          * (issue #125): pass NULL to compute only the MAMA line into outReal.
          */
         retCode = MAMA_Internal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = T3_Internal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case HMA:
         retCode = HMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   RetCode MA_Internal( int startIdx,
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
         retCode = SMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case EMA:
         retCode = EMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case WMA:
         retCode = WMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case DEMA:
         retCode = DEMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case TEMA:
         retCode = TEMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case TRIMA:
         retCode = TRIMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case KAMA:
         retCode = KAMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case MAMA:
         retCode = MAMA_Internal(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = T3_Internal(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      case HMA:
         retCode = HMA_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MA_Internal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MA_Internal(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MA} over the same series.
    * Open with {@link Core#MA_Open}; there is no close — the handle is
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
   public static final class MA_Stream {
      Core core;
      int optInTimePeriod;
      MAType optInMAType;
      double cur_outReal;
      // Sub-stream, tagged by optInMAType; null on the identity path.
      Object sub;
      OutRange fillRange = OutRange.EMPTY;

      MA_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MA_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MA_Stream( MA_Stream other ) {
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
               this.sub = new SMA_Stream((SMA_Stream) other.sub);
               break;
            case EMA:
               this.sub = new EMA_Stream((EMA_Stream) other.sub);
               break;
            case WMA:
               this.sub = new WMA_Stream((WMA_Stream) other.sub);
               break;
            case DEMA:
               this.sub = new DEMA_Stream((DEMA_Stream) other.sub);
               break;
            case TEMA:
               this.sub = new TEMA_Stream((TEMA_Stream) other.sub);
               break;
            case TRIMA:
               this.sub = new TRIMA_Stream((TRIMA_Stream) other.sub);
               break;
            case KAMA:
               this.sub = new KAMA_Stream((KAMA_Stream) other.sub);
               break;
            case MAMA:
               this.sub = new MAMA_Stream((MAMA_Stream) other.sub);
               break;
            case T3:
               this.sub = new T3_Stream((T3_Stream) other.sub);
               break;
            case HMA:
               this.sub = new HMA_Stream((HMA_Stream) other.sub);
               break;
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
         this.fillRange = other.fillRange;
      }

      void copyFrom( MA_Stream other ) {
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
               if( this.sub instanceof SMA_Stream ) {
                  ((SMA_Stream) this.sub).copyFrom((SMA_Stream) other.sub);
               } else {
                  this.sub = new SMA_Stream((SMA_Stream) other.sub);
               }
               break;
            case EMA:
               if( this.sub instanceof EMA_Stream ) {
                  ((EMA_Stream) this.sub).copyFrom((EMA_Stream) other.sub);
               } else {
                  this.sub = new EMA_Stream((EMA_Stream) other.sub);
               }
               break;
            case WMA:
               if( this.sub instanceof WMA_Stream ) {
                  ((WMA_Stream) this.sub).copyFrom((WMA_Stream) other.sub);
               } else {
                  this.sub = new WMA_Stream((WMA_Stream) other.sub);
               }
               break;
            case DEMA:
               if( this.sub instanceof DEMA_Stream ) {
                  ((DEMA_Stream) this.sub).copyFrom((DEMA_Stream) other.sub);
               } else {
                  this.sub = new DEMA_Stream((DEMA_Stream) other.sub);
               }
               break;
            case TEMA:
               if( this.sub instanceof TEMA_Stream ) {
                  ((TEMA_Stream) this.sub).copyFrom((TEMA_Stream) other.sub);
               } else {
                  this.sub = new TEMA_Stream((TEMA_Stream) other.sub);
               }
               break;
            case TRIMA:
               if( this.sub instanceof TRIMA_Stream ) {
                  ((TRIMA_Stream) this.sub).copyFrom((TRIMA_Stream) other.sub);
               } else {
                  this.sub = new TRIMA_Stream((TRIMA_Stream) other.sub);
               }
               break;
            case KAMA:
               if( this.sub instanceof KAMA_Stream ) {
                  ((KAMA_Stream) this.sub).copyFrom((KAMA_Stream) other.sub);
               } else {
                  this.sub = new KAMA_Stream((KAMA_Stream) other.sub);
               }
               break;
            case MAMA:
               if( this.sub instanceof MAMA_Stream ) {
                  ((MAMA_Stream) this.sub).copyFrom((MAMA_Stream) other.sub);
               } else {
                  this.sub = new MAMA_Stream((MAMA_Stream) other.sub);
               }
               break;
            case T3:
               if( this.sub instanceof T3_Stream ) {
                  ((T3_Stream) this.sub).copyFrom((T3_Stream) other.sub);
               } else {
                  this.sub = new T3_Stream((T3_Stream) other.sub);
               }
               break;
            case HMA:
               if( this.sub instanceof HMA_Stream ) {
                  ((HMA_Stream) this.sub).copyFrom((HMA_Stream) other.sub);
               } else {
                  this.sub = new HMA_Stream((HMA_Stream) other.sub);
               }
               break;
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
         this.fillRange = other.fillRange;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<MA_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.MA_StreamStep(this, inReal);
         return this.cur_outReal;
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
         MA_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new MA_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.MA_StreamStep(scratch, inReal);
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
      public MA_Stream copy() {
         return new MA_Stream(this);
      }
   }
   void MA_StreamStep( MA_Stream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 || sp.optInMAType == MAType.DISABLED ) {
         sp.cur_outReal = inReal;
         return;
      }
      switch( sp.optInMAType )
      {
      case SMA: {
         sp.cur_outReal = ((SMA_Stream) sp.sub).update(inReal);
         break;
      }
      case EMA: {
         sp.cur_outReal = ((EMA_Stream) sp.sub).update(inReal);
         break;
      }
      case WMA: {
         sp.cur_outReal = ((WMA_Stream) sp.sub).update(inReal);
         break;
      }
      case DEMA: {
         sp.cur_outReal = ((DEMA_Stream) sp.sub).update(inReal);
         break;
      }
      case TEMA: {
         sp.cur_outReal = ((TEMA_Stream) sp.sub).update(inReal);
         break;
      }
      case TRIMA: {
         sp.cur_outReal = ((TRIMA_Stream) sp.sub).update(inReal);
         break;
      }
      case KAMA: {
         sp.cur_outReal = ((KAMA_Stream) sp.sub).update(inReal);
         break;
      }
      case MAMA: {
         MAMA_Stream.Value subValue = ((MAMA_Stream) sp.sub).update(inReal);
         sp.cur_outReal = subValue.mama();
         break;
      }
      case T3: {
         sp.cur_outReal = ((T3_Stream) sp.sub).update(inReal);
         break;
      }
      case HMA: {
         sp.cur_outReal = ((HMA_Stream) sp.sub).update(inReal);
         break;
      }
      default:
         break; /* unreachable: open rejects arms without a sub-stream */
      }
   }
   private RetCode MA_OpenBody( MA_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
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
      case SMA: {
         SMA_Stream sub = SMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EMA_Stream sub = EMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WMA_Stream sub = WMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DEMA_Stream sub = DEMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TEMA_Stream sub = TEMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TRIMA_Stream sub = TRIMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KAMA_Stream sub = KAMA_OpenInternal(inReal, startIdx, optInTimePeriod);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MAMA_Stream sub = MAMA_OpenInternal(inReal, startIdx, 0.5, 0.05);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3_Stream sub = T3_OpenInternal(inReal, startIdx, optInTimePeriod, 0.7);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HMA_Stream sub = HMA_OpenInternal(inReal, startIdx, optInTimePeriod);
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
   private RetCode MA_OpenAndFillBody( MA_Stream sp, double inReal[], int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
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
         SMA_Stream sub = SMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EMA_Stream sub = EMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WMA_Stream sub = WMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DEMA_Stream sub = DEMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TEMA_Stream sub = TEMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TRIMA_Stream sub = TRIMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KAMA_Stream sub = KAMA_OpenAndFill(inReal, optInTimePeriod, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MAMA_Stream sub = MAMA_OpenAndFill(inReal, 0.5, 0.05, outReal, new double[historyLen]);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3_Stream sub = T3_OpenAndFill(inReal, optInTimePeriod, 0.7, outReal);
         outBegIdx.value = sub.fillRange().begIdx();
         outNBElement.value = sub.fillRange().count();
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HMA_Stream sub = HMA_OpenAndFill(inReal, optInTimePeriod, outReal);
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
   private RetCode MA_OpenAndFillInternalBody( MA_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == 1 || optInMAType == MAType.DISABLED ) {
         if( historyLen < MA_Lookback(optInTimePeriod, optInMAType) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInMAType = optInMAType;
         sp.sub = null;
         int fillLb = MA_Lookback(optInTimePeriod, optInMAType);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.OutOfRangeEndIndex;
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
         SMA_Stream sub = SMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case EMA: {
         EMA_Stream sub = EMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case WMA: {
         WMA_Stream sub = WMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case DEMA: {
         DEMA_Stream sub = DEMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TEMA: {
         TEMA_Stream sub = TEMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case TRIMA: {
         TRIMA_Stream sub = TRIMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case KAMA: {
         KAMA_Stream sub = KAMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case MAMA: {
         MAMA_Stream sub = MAMA_OpenAndFillInternal(inReal, startIdx, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[historyLen]);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3_Stream sub = T3_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case HMA: {
         HMA_Stream sub = HMA_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
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
   /* Internal startIdx-anchored open behind MA_Open (composition seam). */
   MA_Stream MA_OpenInternal( double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType )
   {
      MA_Stream sp = new MA_Stream(this);
      RetCode retCode = MA_OpenBody(sp, inReal, startIdx, optInTimePeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MA open: internal error");
      }
      throw new IllegalArgumentException("MA open: " + retCode);
   }
   /**
    * Open a live MA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MA} at that bar.
    * <p>The history must hold at least {@code MA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MA_Stream MA_Open( double inReal[], int optInTimePeriod, MAType optInMAType )
   {
      return MA_OpenInternal(inReal, 0, optInTimePeriod, optInMAType);
   }
   /**
    * {@link Core#MA_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MA_Stream#fillRange()}.
    */
   public MA_Stream MA_OpenAndFill( double inReal[], int optInTimePeriod, MAType optInMAType, double outReal[] )
   {
      MA_Stream sp = new MA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MA_OpenAndFillBody(sp, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MA openAndFill: internal error");
      }
      throw new IllegalArgumentException("MA openAndFill: " + retCode);
   }
   /* MA_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MA_Stream MA_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MA_Stream sp = new MA_Stream(this);
      RetCode retCode = MA_OpenAndFillInternalBody(sp, inReal, startIdx, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MA openAndFill: internal error");
      }
      throw new IllegalArgumentException("MA openAndFill: " + retCode);
   }
