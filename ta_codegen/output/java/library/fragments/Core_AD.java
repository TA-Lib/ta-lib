/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JD       jdoyle
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  111705 MF,JD  Fix#1359452 for handling properly start/end range.
 */

   public int adLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode ad( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      double inClose[],
                      double inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Note: Results from this function might vary slightly
       *       from Metastock outputs. The reason being that
       *       Metastock use float instead of double and this
       *       cause a different floating-point precision to
       *       be used.
       *
       *       For most function, this is not an apparent difference
       *       but for function using large cummulative values (like
       *       this AD function), minor imprecision adds up and becomes
       *       significative.
       *
       *       For better precision, TA-Lib use double in all its
       *       its calculations.
       */
      /* Default return values */
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode adUnguarded( int startIdx,
                               int endIdx,
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               double inVolume[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode ad( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      float inClose[],
                      float inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = (double)inHigh[currentBar];
         low = (double)inLow[currentBar];
         tmp = high - low;
         close = (double)inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode adUnguarded( int startIdx,
                               int endIdx,
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               float inVolume[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = (double)inHigh[currentBar];
         low = (double)inLow[currentBar];
         tmp = high - low;
         close = (double)inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live AD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ad} over the same series.
    * Open with {@link Core#adOpen}; there is no close — the handle is
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
   public static final class AdStream {
      final Core core;
      double ad;
      double cur_outReal;

      AdStream( Core core ) { this.core = core; }

      AdStream( AdStream other ) {
         this.core = other.core;
         this.ad = other.ad;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         core.adStreamStep(this, inHigh, inLow, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         AdStream scratch = new AdStream(this);
         core.adStreamStep(scratch, inHigh, inLow, inClose, inVolume);
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
      public AdStream copy() {
         return new AdStream(this);
      }
   }
   void adStreamStep( AdStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double high = 0.0;
      double low = 0.0;
      double close = 0.0;
      double tmp = 0.0;
      high = inHigh;
      low = inLow;
      tmp = high - low;
      close = inClose;
      if( tmp > 0.0 ) {
         sp.ad += (close - low - (high - close)) / tmp * (double)inVolume;
      }
      sp.cur_outReal = sp.ad;
   }
   private RetCode adOpenBody( AdStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      /* Note: Results from this function might vary slightly
       *       from Metastock outputs. The reason being that
       *       Metastock use float instead of double and this
       *       cause a different floating-point precision to
       *       be used.
       *
       *       For most function, this is not an apparent difference
       *       but for function using large cummulative values (like
       *       this AD function), minor imprecision adds up and becomes
       *       significative.
       *
       *       For better precision, TA-Lib use double in all its
       *       its calculations.
       */
      /* Default return values */
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         lastValue_outReal = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      /* Capture the live batch state into the handle. */
      sp.ad = ad;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode adOpenAndFillBody( AdStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      /* Note: Results from this function might vary slightly
       *       from Metastock outputs. The reason being that
       *       Metastock use float instead of double and this
       *       cause a different floating-point precision to
       *       be used.
       *
       *       For most function, this is not an apparent difference
       *       but for function using large cummulative values (like
       *       this AD function), minor imprecision adds up and becomes
       *       significative.
       *
       *       For better precision, TA-Lib use double in all its
       *       its calculations.
       */
      /* Default return values */
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      /* Capture the live batch state into the handle. */
      sp.ad = ad;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind adOpen (composition seam). */
   AdStream adOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx )
   {
      AdStream sp = new AdStream(this);
      RetCode retCode = adOpenBody(sp, inHigh, inLow, inClose, inVolume, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AD open: internal error");
      }
      throw new IllegalArgumentException("TA_AD open: " + retCode);
   }
   /**
    * Open a live AD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ad} at that bar.
    * <p>The history must hold at least {@code adLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AdStream adOpen( double inHigh[], double inLow[], double inClose[], double inVolume[] )
   {
      return adOpenInternal(inHigh, inLow, inClose, inVolume, 0);
   }
   /**
    * {@link Core#adOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ad} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AdStream adOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AdStream sp = new AdStream(this);
      RetCode retCode = adOpenAndFillBody(sp, inHigh, inLow, inClose, inVolume, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AD openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_AD openAndFill: " + retCode);
   }
