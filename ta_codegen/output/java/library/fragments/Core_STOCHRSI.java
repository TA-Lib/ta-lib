/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101103 PP   Initial creation of code.
 *  112603 MF   Add independent control to the RSI period.
 *  020605 AA   Fix #1117656. NULL pointer assignement.
 */

   public int stochRsiLookback( int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return -1;
      }
      int retValue;
      retValue = rsiLookback(optInTimePeriod) + stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   public RetCode stochRsi( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            int optInFastK_Period,
                            int optInFastD_Period,
                            MAType optInFastD_MAType,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outFastK[],
                            double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsiUnguarded( int startIdx,
                                     int endIdx,
                                     double inReal[],
                                     int optInTimePeriod,
                                     int optInFastK_Period,
                                     int optInFastD_Period,
                                     MAType optInFastD_MAType,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outFastK[],
                                     double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsi( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            int optInFastK_Period,
                            int optInFastD_Period,
                            MAType optInFastD_MAType,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outFastK[],
                            double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsiUnguarded( int startIdx,
                                     int endIdx,
                                     float inReal[],
                                     int optInTimePeriod,
                                     int optInFastK_Period,
                                     int optInFastD_Period,
                                     MAType optInFastD_MAType,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outFastK[],
                                     double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live STOCHRSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#stochRsi} over the same series.
    * Open with {@link Core#stochRsiOpen}; there is no close — the handle is
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
   public static final class StochRsiStream {
      final Core core;
      int optInTimePeriod;
      int optInFastK_Period;
      int optInFastD_Period;
      MAType optInFastD_MAType;
      double cur_outFastK;
      double cur_outFastD;
      Value cachedValue;
      RsiStream sub0;
      StochFStream sub1;

      StochRsiStream( Core core ) { this.core = core; }

      StochRsiStream( StochRsiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInFastD_Period = other.optInFastD_Period;
         this.optInFastD_MAType = other.optInFastD_MAType;
         this.cur_outFastK = other.cur_outFastK;
         this.cur_outFastD = other.cur_outFastD;
         this.cachedValue = other.cachedValue;
         this.sub0 = new RsiStream(other.sub0);
         this.sub1 = new StochFStream(other.sub1);
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double fastK;
         public final double fastD;
         Value( double fastK, double fastD ) {
            this.fastK = fastK;
            this.fastD = fastD;
         }
         @Override public String toString() {
            return "Value[" + "fastK=" + fastK + ", " + "fastD=" + fastD + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.fastK) == Double.doubleToLongBits(v.fastK) && Double.doubleToLongBits(this.fastD) == Double.doubleToLongBits(v.fastD);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(fastK);
            h = 31 * h + Double.hashCode(fastD);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.stochRsiStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outFastK, this.cur_outFastD);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         StochRsiStream scratch = new StochRsiStream(this);
         core.stochRsiStreamStep(scratch, inReal);
         return new Value(scratch.cur_outFastK, scratch.cur_outFastD);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public StochRsiStream copy() {
         return new StochRsiStream(this);
      }
   }
   void stochRsiStreamStep( StochRsiStream sp, double inReal )
   {
      double cur_tempRSIBuffer = 0.0;
      double cur_outFastK = 0.0;
      double cur_outFastD = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempRSIBuffer = sp.sub0.update(inReal);
      {
         StochFStream.Value subOut1 = sp.sub1.update(cur_tempRSIBuffer, cur_tempRSIBuffer, cur_tempRSIBuffer);
         cur_outFastK = subOut1.fastK;
         cur_outFastD = subOut1.fastD;
      }
      sp.cur_outFastK = cur_outFastK;
      sp.cur_outFastD = cur_outFastD;
   }
   private RetCode stochRsiOpenBody( StochRsiStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      /* Sub-stream 0: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RsiStream sub0 = rsiOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx - lookbackSTOCHF, optInTimePeriod);
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Sub-stream 1: stochf over `tempRSIBuffer, tempRSIBuffer, tempRSIBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StochFStream sub1 = stochFOpenInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new StochRsiStream.Value(sp.cur_outFastK, sp.cur_outFastD);
      return RetCode.Success;
   }
   private RetCode stochRsiOpenAndFillBody( StochRsiStream sp, double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outFastK == (Object)inReal || (Object)outFastD == (Object)inReal || (Object)outFastK == (Object)outFastD ) {
         return RetCode.BadParam;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      /* Sub-stream 0: rsi over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RsiStream sub0 = rsiOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx - lookbackSTOCHF, optInTimePeriod);
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Sub-stream 1: stochf over `tempRSIBuffer, tempRSIBuffer, tempRSIBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StochFStream sub1 = stochFOpenInternal(java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), java.util.Arrays.copyOfRange(tempRSIBuffer, 0, (tempArraySize - 1) + 1), 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, sc_outFastK, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new StochRsiStream.Value(sp.cur_outFastK, sp.cur_outFastD);
      System.arraycopy(sc_outFastK, 0, outFastK, 0, outNBElement.value);
      System.arraycopy(sc_outFastD, 0, outFastD, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind stochRsiOpen (composition seam). */
   StochRsiStream stochRsiOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      StochRsiStream sp = new StochRsiStream(this);
      RetCode retCode = stochRsiOpenBody(sp, inReal, startIdx, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCHRSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCHRSI open: internal error");
      }
      throw new IllegalArgumentException("TA_STOCHRSI open: " + retCode);
   }
   /**
    * Open a live STOCHRSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#stochRsi} at that bar.
    * <p>The history must hold at least {@code stochRsiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public StochRsiStream stochRsiOpen( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      return stochRsiOpenInternal(inReal, 0, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
   }
   /**
    * {@link Core#stochRsiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#stochRsi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public StochRsiStream stochRsiOpenAndFill( double inReal[], int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
   {
      StochRsiStream sp = new StochRsiStream(this);
      RetCode retCode = stochRsiOpenAndFillBody(sp, inReal, optInTimePeriod, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCHRSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCHRSI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_STOCHRSI openAndFill: " + retCode);
   }
