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
 */

   public int movingAverageLookback( int optInTimePeriod, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      if( optInTimePeriod <= 1 ) {
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
      default:
         retValue = 0;
         break;
      }
      return retValue ;

   }
   public RetCode movingAverage( int startIdx,
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
      if( optInTimePeriod == 1 ) {
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
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         /* The optInTimePeriod is ignored. FAMA is a nullable output
          * (issue #125): pass NULL to compute only the MAMA line into outReal.
          */
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverageUnguarded( int startIdx,
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
      if( optInTimePeriod == 1 ) {
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
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverage( int startIdx,
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
      if( optInTimePeriod == 1 ) {
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
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
   }
   public RetCode movingAverageUnguarded( int startIdx,
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
      if( optInTimePeriod == 1 ) {
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
         retCode = smaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Ema:
         retCode = emaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Wma:
         retCode = wmaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Dema:
         retCode = demaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Tema:
         retCode = temaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Trima:
         retCode = trimaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Kama:
         retCode = kamaUnguarded(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         break;
      case Mama:
         retCode = mamaUnguarded(startIdx, endIdx, inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[(int)(endIdx - startIdx + 1)]);
         break;
      case T3:
         retCode = t3Unguarded(startIdx, endIdx, inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
         break;
      default:
         retCode = RetCode.BadParam;
         break;
      }
      return retCode ;
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

      MovingAverageStream( Core core ) { this.core = core; }

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
            default:
               throw new IllegalStateException("unreachable: open rejects arms without a sub-stream");
            }
         }
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
      if( sp.optInTimePeriod == 1 ) {
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
      if( optInTimePeriod == 1 ) {
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
      if( optInTimePeriod == 1 ) {
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
         SmaStream sub = smaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Ema: {
         EmaStream sub = emaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Wma: {
         WmaStream sub = wmaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Dema: {
         DemaStream sub = demaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Tema: {
         TemaStream sub = temaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Trima: {
         TrimaStream sub = trimaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Kama: {
         KamaStream sub = kamaOpenAndFill(inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outReal;
         break;
      }
      case Mama: {
         MamaStream sub = mamaOpenAndFill(inReal, 0.5, 0.05, outBegIdx, outNBElement, outReal, new double[historyLen]);
         sp.sub = sub;
         sp.cur_outReal = sub.cur_outMAMA;
         break;
      }
      case T3: {
         T3Stream sub = t3OpenAndFill(inReal, optInTimePeriod, 0.7, outBegIdx, outNBElement, outReal);
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
    */
   public MovingAverageStream movingAverageOpenAndFill( double inReal[], int optInTimePeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MovingAverageStream sp = new MovingAverageStream(this);
      RetCode retCode = movingAverageOpenAndFillBody(sp, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, outReal);
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
