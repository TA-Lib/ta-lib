/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  082303 MF    Fix #792298. Remove rounding. Bug reported by AM.
 *  062704 MF    Fix #965557. Div by zero bug reported by MIF.
 *  122204 MF,CF Fix #1090231. Issues when period is 1.
 */

   public int plusDILookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod > 1 ) {
         return optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDI.ordinal()] ;
      } else {
         return 1 ;
      }

   }
   public RetCode plusDI( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
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
      double prevClose = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
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
       *  |  | +DM1=0
       * B|  | -DM1=0
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
       * for a -DM14, sum the -DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous +DM14
       *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
       *                                         14
       *
       * Calculation of a +DI14 is as follow:
       *
       *               +DM14
       *     +DI14 =  --------
       *                TR14
       *
       * Calculation of the TR14 is:
       *
       *                                   Previous TR14
       *    Today's TR14 = Previous TR14 - -------------- + Today's TR1
       *                                         14
       *
       *    The first TR14 is the summation of the first 14 TR1. See the
       *    TA_TRANGE function on how to calculate the true range.
       *
       * Reference:
       *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
       */
      /* Original implementation from Wilder's book was doing some integer
       * rounding in its calculations.
       *
       * This was understandable in the context that at the time the book
       * was written, most user were doing the calculation by hand.
       *
       * For a computer, rounding is unnecessary (and even problematic when inputs
       * are close to 1).
       *
       * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
       * you can comment out the following #undef/#define and rebuild the library.
       */
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDI.ordinal()];
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
         /* No smoothing needed. Just do the following:
          * for each price bar.
          *          +DM1
          *   +DI1 = ----
          *           TR1
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         prevClose = inClose[today];
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
               double _true_range_0;
               double range_0 = prevHigh - prevLow;
               double tmp_0 = Math.abs(prevHigh - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               tmp_0 = Math.abs(prevLow - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               _true_range_0 = range_0;
               tempReal = _true_range_0;
               if( ((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                  outReal[outIdx++] = (double)0.0;
               } else {
                  outReal[outIdx++] = diffP / tempReal;
               }
            } else {
               outReal[outIdx++] = (double)0.0;
            }
            prevClose = inClose[today];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Process the initial DM and TR */
      today = startIdx;
      outBegIdx.value = today;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      prevClose = inClose[today];
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
         double _true_range_1;
         double range_1 = prevHigh - prevLow;
         double tmp_1 = Math.abs(prevHigh - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         tmp_1 = Math.abs(prevLow - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         _true_range_1 = range_1;
         tempReal = _true_range_1;
         prevTR += tempReal;
         prevClose = inClose[today];
      }
      /* Process subsequent DI */
      /* Skip the unstable period. Note that this loop must be executed
       * at least ONCE to calculate the first DI.
       */
      i = this.unstablePeriod[FuncUnstId.PlusDI.ordinal()] + 1;
      while( i-- != 0 ) {
         /* Calculate the prevPlusDM */
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
         /* Calculate the prevTR */
         double _true_range_2;
         double range_2 = prevHigh - prevLow;
         double tmp_2 = Math.abs(prevHigh - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         tmp_2 = Math.abs(prevLow - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         _true_range_2 = range_2;
         tempReal = _true_range_2;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
      }
      /* Now start to write the output in
       * the caller provided outReal.
       */
      if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
         outReal[0] = (100.0 * (prevPlusDM / prevTR));
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today < endIdx ) {
         /* Calculate the prevPlusDM */
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
         /* Calculate the prevTR */
         double _true_range_3;
         double range_3 = prevHigh - prevLow;
         double tmp_3 = Math.abs(prevHigh - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         tmp_3 = Math.abs(prevLow - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         _true_range_3 = range_3;
         tempReal = _true_range_3;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         /* Calculate the DI. The value is rounded (see Wilder book). */
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            outReal[outIdx++] = (100.0 * (prevPlusDM / prevTR));
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDIUnguarded( int startIdx,
                                   int endIdx,
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
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
      double prevClose = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDI.ordinal()];
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
         prevClose = inClose[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               double _true_range_0;
               double range_0 = prevHigh - prevLow;
               double tmp_0 = Math.abs(prevHigh - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               tmp_0 = Math.abs(prevLow - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               _true_range_0 = range_0;
               tempReal = _true_range_0;
               if( ((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                  outReal[outIdx++] = (double)0.0;
               } else {
                  outReal[outIdx++] = diffP / tempReal;
               }
            } else {
               outReal[outIdx++] = (double)0.0;
            }
            prevClose = inClose[today];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      today = startIdx;
      outBegIdx.value = today;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      prevClose = inClose[today];
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
         double _true_range_1;
         double range_1 = prevHigh - prevLow;
         double tmp_1 = Math.abs(prevHigh - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         tmp_1 = Math.abs(prevLow - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         _true_range_1 = range_1;
         tempReal = _true_range_1;
         prevTR += tempReal;
         prevClose = inClose[today];
      }
      i = this.unstablePeriod[FuncUnstId.PlusDI.ordinal()] + 1;
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
         double _true_range_2;
         double range_2 = prevHigh - prevLow;
         double tmp_2 = Math.abs(prevHigh - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         tmp_2 = Math.abs(prevLow - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         _true_range_2 = range_2;
         tempReal = _true_range_2;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
      }
      if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
         outReal[0] = (100.0 * (prevPlusDM / prevTR));
      } else {
         outReal[0] = 0.0;
      }
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
         double _true_range_3;
         double range_3 = prevHigh - prevLow;
         double tmp_3 = Math.abs(prevHigh - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         tmp_3 = Math.abs(prevLow - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         _true_range_3 = range_3;
         tempReal = _true_range_3;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            outReal[outIdx++] = (100.0 * (prevPlusDM / prevTR));
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDI( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
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
      double prevClose = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
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
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDI.ordinal()];
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
         prevClose = (double)inClose[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = (double)inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = (double)inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               double _true_range_0;
               double range_0 = prevHigh - prevLow;
               double tmp_0 = Math.abs(prevHigh - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               tmp_0 = Math.abs(prevLow - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               _true_range_0 = range_0;
               tempReal = _true_range_0;
               if( ((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                  outReal[outIdx++] = (double)0.0;
               } else {
                  outReal[outIdx++] = diffP / tempReal;
               }
            } else {
               outReal[outIdx++] = (double)0.0;
            }
            prevClose = (double)inClose[today];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      today = startIdx;
      outBegIdx.value = today;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = (double)inHigh[today];
      prevLow = (double)inLow[today];
      prevClose = (double)inClose[today];
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
         double _true_range_1;
         double range_1 = prevHigh - prevLow;
         double tmp_1 = Math.abs(prevHigh - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         tmp_1 = Math.abs(prevLow - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         _true_range_1 = range_1;
         tempReal = _true_range_1;
         prevTR += tempReal;
         prevClose = (double)inClose[today];
      }
      i = this.unstablePeriod[FuncUnstId.PlusDI.ordinal()] + 1;
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
         double _true_range_2;
         double range_2 = prevHigh - prevLow;
         double tmp_2 = Math.abs(prevHigh - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         tmp_2 = Math.abs(prevLow - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         _true_range_2 = range_2;
         tempReal = _true_range_2;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
      }
      if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
         outReal[0] = (100.0 * (prevPlusDM / prevTR));
      } else {
         outReal[0] = 0.0;
      }
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
         double _true_range_3;
         double range_3 = prevHigh - prevLow;
         double tmp_3 = Math.abs(prevHigh - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         tmp_3 = Math.abs(prevLow - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         _true_range_3 = range_3;
         tempReal = _true_range_3;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            outReal[outIdx++] = (100.0 * (prevPlusDM / prevTR));
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode plusDIUnguarded( int startIdx,
                                   int endIdx,
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
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
      double prevClose = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PlusDI.ordinal()];
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
         prevClose = (double)inClose[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = (double)inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = (double)inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               double _true_range_0;
               double range_0 = prevHigh - prevLow;
               double tmp_0 = Math.abs(prevHigh - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               tmp_0 = Math.abs(prevLow - prevClose);
               if( tmp_0 > range_0 ) {
                  range_0 = tmp_0;
               }
               _true_range_0 = range_0;
               tempReal = _true_range_0;
               if( ((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                  outReal[outIdx++] = (double)0.0;
               } else {
                  outReal[outIdx++] = diffP / tempReal;
               }
            } else {
               outReal[outIdx++] = (double)0.0;
            }
            prevClose = (double)inClose[today];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      today = startIdx;
      outBegIdx.value = today;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = (double)inHigh[today];
      prevLow = (double)inLow[today];
      prevClose = (double)inClose[today];
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
         double _true_range_1;
         double range_1 = prevHigh - prevLow;
         double tmp_1 = Math.abs(prevHigh - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         tmp_1 = Math.abs(prevLow - prevClose);
         if( tmp_1 > range_1 ) {
            range_1 = tmp_1;
         }
         _true_range_1 = range_1;
         tempReal = _true_range_1;
         prevTR += tempReal;
         prevClose = (double)inClose[today];
      }
      i = this.unstablePeriod[FuncUnstId.PlusDI.ordinal()] + 1;
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
         double _true_range_2;
         double range_2 = prevHigh - prevLow;
         double tmp_2 = Math.abs(prevHigh - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         tmp_2 = Math.abs(prevLow - prevClose);
         if( tmp_2 > range_2 ) {
            range_2 = tmp_2;
         }
         _true_range_2 = range_2;
         tempReal = _true_range_2;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
      }
      if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
         outReal[0] = (100.0 * (prevPlusDM / prevTR));
      } else {
         outReal[0] = 0.0;
      }
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
         double _true_range_3;
         double range_3 = prevHigh - prevLow;
         double tmp_3 = Math.abs(prevHigh - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         tmp_3 = Math.abs(prevLow - prevClose);
         if( tmp_3 > range_3 ) {
            range_3 = tmp_3;
         }
         _true_range_3 = range_3;
         tempReal = _true_range_3;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            outReal[outIdx++] = (100.0 * (prevPlusDM / prevTR));
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
