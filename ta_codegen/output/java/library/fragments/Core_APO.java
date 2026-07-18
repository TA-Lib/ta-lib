/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  062804 MF     Resolve div by zero bug on limit case.
 *  020605 AA     Fix #1117666 Lookback & out-of-bound bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe.
 */

   public int apoLookback( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      /* The slow MA is the key factor determining the lookback period. */
      return movingAverageLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) ;

   }
   public RetCode apo( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MAType optInMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate (fast MA)-(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = tempBuffer[i + offset] - outReal[i];
      }
      return RetCode.Success ;
   }
   public RetCode apoUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInFastPeriod,
                                int optInSlowPeriod,
                                MAType optInMAType,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      offset = fastNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = tempBuffer[i + offset] - outReal[i];
      }
      return RetCode.Success ;
   }
   public RetCode apo( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MAType optInMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      offset = fastNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = tempBuffer[i + offset] - outReal[i];
      }
      return RetCode.Success ;
   }
   public RetCode apoUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInFastPeriod,
                                int optInSlowPeriod,
                                MAType optInMAType,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      offset = fastNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         outReal[i] = tempBuffer[i + offset] - outReal[i];
      }
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live APO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#apo} over the same series.
    * Open with {@link Core#apoOpen}; there is no close — the handle is
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
   public static final class ApoStream {
      final Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      MAType optInMAType;
      double cur_outReal;
      MovingAverageStream sub0;
      MovingAverageStream sub1;

      ApoStream( Core core ) { this.core = core; }

      ApoStream( ApoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new MovingAverageStream(other.sub0);
         this.sub1 = new MovingAverageStream(other.sub1);
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.apoStreamStep(this, inReal);
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
         ApoStream scratch = new ApoStream(this);
         core.apoStreamStep(scratch, inReal);
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
      public ApoStream copy() {
         return new ApoStream(this);
      }
   }
   void apoStreamStep( ApoStream sp, double inReal )
   {
      double cur_tempBuffer = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      cur_outReal = cur_tempBuffer - cur_outReal;
      sp.cur_outReal = cur_outReal;
   }
   private RetCode apoOpenBody( ApoStream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInFastPeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInSlowPeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate (fast MA)-(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         sc_outReal[i] = tempBuffer[i + offset] - sc_outReal[i];
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   private RetCode apoOpenAndFillBody( ApoStream sp, double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInFastPeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInSlowPeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate (fast MA)-(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         sc_outReal[i] = tempBuffer[i + offset] - sc_outReal[i];
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      System.arraycopy(sc_outReal, 0, outReal, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind apoOpen (composition seam). */
   ApoStream apoOpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      ApoStream sp = new ApoStream(this);
      RetCode retCode = apoOpenBody(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_APO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_APO open: internal error");
      }
      throw new IllegalArgumentException("TA_APO open: " + retCode);
   }
   /**
    * Open a live APO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#apo} at that bar.
    * <p>The history must hold at least {@code apoLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public ApoStream apoOpen( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      return apoOpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType);
   }
   /**
    * {@link Core#apoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#apo} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public ApoStream apoOpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ApoStream sp = new ApoStream(this);
      RetCode retCode = apoOpenAndFillBody(sp, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_APO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_APO openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_APO openAndFill: " + retCode);
   }
