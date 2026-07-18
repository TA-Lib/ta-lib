/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  122104 MF,CF  Fix#1089506 for when optInTimePeriod is 1.
 */

   public int plusDMLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod > 1 ) {
         return optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1 ;
      } else {
         return 1 ;
      }

   }
   public RetCode plusDM( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /*
       * The DM1 (one period) is base on the largest part of
       * today's range that is outside of yesterdays range.
       *
       * The following 7 cases explain how the +DM and -DM are
       * calculated on one period:
       *
       * Case 1:                       Case 2:
       *    C|                        A|
       *     |                         | C|
       *     | +DM1 = (C-A)           B|  | +DM1 = 0
       *     | -DM1 = 0                   | -DM1 = (B-D)
       * A|  |                           D|
       *  | D|
       * B|
       *
       * Case 3:                       Case 4:
       *    C|                           C|
       *     |                        A|  |
       *     | +DM1 = (C-A)            |  | +DM1 = 0
       *     | -DM1 = 0               B|  | -DM1 = (B-D)
       * A|  |                            |
       *  |  |                           D|
       * B|  |
       *    D|
       *
       * Case 5:                      Case 6:
       * A|                           A| C|
       *  | C| +DM1 = 0                |  |  +DM1 = 0
       *  |  | -DM1 = 0                |  |  -DM1 = 0
       *  | D|                         |  |
       * B|                           B| D|
       *
       *
       * Case 7:
       *
       *    C|
       * A|  |
       *  |  | +DM=0
       * B|  | -DM=0
       *    D|
       *
       * In case 3 and 4, the rule is that the smallest delta between
       * (C-A) and (B-D) determine which of +DM or -DM is zero.
       *
       * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
       * zero.
       *
       * The rules remain the same when A=B and C=D (when the highs
       * equal the lows).
       *
       * When calculating the DM over a period > 1, the one-period DM
       * for the desired period are initialy sum. In other word,
       * for a +DM14, sum the +DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous +DM14
       *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
       *                                         14
       *
       * Reference:
       *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
       */
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      /* Adjust startIdx to account for the lookback period. */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Indicate where the next output should be put
       * in the outReal.
       */
      outIdx = 0;
      /* Trap the case where no smoothing is needed. */
      if( optInTimePeriod <= 1 ) {
         /* No smoothing needed. Just do a simple DM1
          * for each price bar.
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Process the initial DM */
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
      }
      /* Process subsequent DM */
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            /* Case 2,4,5 and 7 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      /* Now start to write the output in
       * the caller provided outReal.
       */
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            /* Case 2,4,5 and 7 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDMUnguarded( int startIdx,
                                   int endIdx,
                                   double inHigh[],
                                   double inLow[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod <= 1 ) {
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
      }
      i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDM( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod <= 1 ) {
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = (double)inHigh[today];
         prevLow = (double)inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = (double)inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = (double)inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = (double)inHigh[today];
      prevLow = (double)inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
      }
      i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDMUnguarded( int startIdx,
                                   int endIdx,
                                   float inHigh[],
                                   float inLow[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod <= 1 ) {
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = (double)inHigh[today];
         prevLow = (double)inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = (double)inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = (double)inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = (double)inHigh[today];
      prevLow = (double)inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
      }
      i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live PLUS_DM stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#plusDM} over the same series.
    * Open with {@link Core#plusDMOpen}; there is no close — the handle is
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
   public static final class PlusDMStream {
      final Core core;
      int optInTimePeriod;
      double prevHigh;
      double prevLow;
      double tempReal;
      double diffP;
      double diffM;
      double prevPlusDM;
      double cur_outReal;

      PlusDMStream( Core core ) { this.core = core; }

      PlusDMStream( PlusDMStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevHigh = other.prevHigh;
         this.prevLow = other.prevLow;
         this.tempReal = other.tempReal;
         this.diffP = other.diffP;
         this.diffM = other.diffM;
         this.prevPlusDM = other.prevPlusDM;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow ) {
         core.plusDMStreamStep(this, inHigh, inLow);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow ) {
         PlusDMStream scratch = new PlusDMStream(this);
         core.plusDMStreamStep(scratch, inHigh, inLow);
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
      public PlusDMStream copy() {
         return new PlusDMStream(this);
      }
   }
   void plusDMStreamStep( PlusDMStream sp, double inHigh, double inLow )
   {
      if( sp.optInTimePeriod <= 1 ) {
         sp.tempReal = inHigh;
         sp.diffP = sp.tempReal - sp.prevHigh;
         /* Plus Delta */
         sp.prevHigh = sp.tempReal;
         sp.tempReal = inLow;
         sp.diffM = sp.prevLow - sp.tempReal;
         /* Minus Delta */
         sp.prevLow = sp.tempReal;
         if( sp.diffP > 0 && sp.diffP > sp.diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            sp.cur_outReal = sp.diffP;
         } else {
            sp.cur_outReal = 0;
         }
      } else {
         sp.tempReal = inHigh;
         sp.diffP = sp.tempReal - sp.prevHigh;
         /* Plus Delta */
         sp.prevHigh = sp.tempReal;
         sp.tempReal = inLow;
         sp.diffM = sp.prevLow - sp.tempReal;
         /* Minus Delta */
         sp.prevLow = sp.tempReal;
         if( sp.diffP > 0 && sp.diffP > sp.diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            sp.prevPlusDM = sp.prevPlusDM - sp.prevPlusDM / sp.optInTimePeriod + sp.diffP;
         } else {
            /* Case 2,4,5 and 7 */
            sp.prevPlusDM = sp.prevPlusDM - sp.prevPlusDM / sp.optInTimePeriod;
         }
         sp.cur_outReal = sp.prevPlusDM;
      }
   }
   private RetCode plusDMOpenBody( PlusDMStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod <= 1 ) {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* No smoothing needed. Just do a simple DM1
          * for each price bar.
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               lastValue_outReal = diffP;
            } else {
               lastValue_outReal = 0;
            }
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.tempReal = tempReal;
         sp.diffP = diffP;
         sp.diffM = diffM;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      } else {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* Process the initial DM */
         outBegIdx.value = startIdx;
         prevPlusDM = 0.0;
         today = startIdx - lookbackTotal;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM += diffP;
            }
         }
         /* Process subsequent DM */
         /* Skip the unstable period. */
         i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
         while( i-- != 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
         }
         /* Now start to write the output in
          * the caller provided outReal.
          */
         lastValue_outReal = prevPlusDM;
         outIdx = 1;
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
            lastValue_outReal = prevPlusDM;
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.tempReal = tempReal;
         sp.diffP = diffP;
         sp.diffM = diffM;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      }
   }
   private RetCode plusDMOpenAndFillBody( PlusDMStream sp, double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod <= 1 ) {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* No smoothing needed. Just do a simple DM1
          * for each price bar.
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.tempReal = tempReal;
         sp.diffP = diffP;
         sp.diffM = diffM;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      } else {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* Process the initial DM */
         outBegIdx.value = startIdx;
         prevPlusDM = 0.0;
         today = startIdx - lookbackTotal;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM += diffP;
            }
         }
         /* Process subsequent DM */
         /* Skip the unstable period. */
         i = this.unstablePeriod[FuncUnstId.PlusDM.ordinal()];
         while( i-- != 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
         }
         /* Now start to write the output in
          * the caller provided outReal.
          */
         outReal[0] = prevPlusDM;
         outIdx = 1;
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
            outReal[outIdx++] = prevPlusDM;
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.tempReal = tempReal;
         sp.diffP = diffP;
         sp.diffM = diffM;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
   }
   /* Internal startIdx-anchored open behind plusDMOpen (composition seam). */
   PlusDMStream plusDMOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      PlusDMStream sp = new PlusDMStream(this);
      RetCode retCode = plusDMOpenBody(sp, inHigh, inLow, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_PLUS_DM open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_PLUS_DM open: internal error");
      }
      throw new IllegalArgumentException("TA_PLUS_DM open: " + retCode);
   }
   /**
    * Open a live PLUS_DM stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#plusDM} at that bar.
    * <p>The history must hold at least {@code plusDMLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public PlusDMStream plusDMOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      return plusDMOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#plusDMOpen} that also fills the output array(s) bit-identically
    * to {@link Core#plusDM} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public PlusDMStream plusDMOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PlusDMStream sp = new PlusDMStream(this);
      RetCode retCode = plusDMOpenAndFillBody(sp, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_PLUS_DM openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_PLUS_DM openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_PLUS_DM openAndFill: " + retCode);
   }
