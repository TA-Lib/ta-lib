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
