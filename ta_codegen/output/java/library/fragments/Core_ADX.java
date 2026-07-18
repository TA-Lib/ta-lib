/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  GC       guycom@users.sourceforge.net
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  082303 MF   Fix #792298. Remove rounding. Bug reported by AM.
 *  062704 MF   Fix #965557. Div by zero bug reported by MIF.
 *  082206 MF   Fix #1544555. Div by zero bug reported by GC.
 */

   public int adxLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1 ;

   }
   public RetCode adx( int startIdx,
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
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
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
       * for a -DM14, sum the -DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous -DM14
       *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
       *                                         14
       *
       * (Same thing for +DM14)
       *
       * Calculation of a -DI14 is as follow:
       *
       *               -DM14
       *     -DI14 =  --------
       *                TR14
       *
       * (Same thing for +DI14)
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
       * Calculation of the DX14 is:
       *
       *    diffDI = ABS( (-DI14) - (+DI14) )
       *    sumDI  = (-DI14) + (+DI14)
       *
       *    DX14 = 100 * (diffDI / sumDI)
       *
       * Calculation of the first ADX:
       *
       *    ADX14 = SUM of the first 14 DX
       *
       * Calculation of subsequent ADX:
       *
       *            ((Previous ADX14)*(14-1))+ Today's DX
       *    ADX14 = -------------------------------------
       *                             14
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
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
      /* Process the initial DM and TR */
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      prevClose = inClose[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
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
         prevTR += tempReal;
         prevClose = inClose[today];
      }
      /* Add up all the initial DX. */
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         /* Calculate the prevTR */
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
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         /* Calculate the DX. The value is rounded (see Wilder book). */
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            /* This loop is just to accumulate the initial DX */
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      /* Calculate the first ADX */
      prevADX = (sumDX / optInTimePeriod);
      /* Skip the unstable period */
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      /* Output the first ADX */
      outReal[0] = prevADX;
      outIdx = 1;
      /* Calculate and output subsequent ADX */
      while( today < endIdx ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         /* Output the ADX */
         outReal[outIdx++] = prevADX;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adxUnguarded( int startIdx,
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
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
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
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
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
         prevTR += tempReal;
         prevClose = inClose[today];
      }
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
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
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      prevADX = (sumDX / optInTimePeriod);
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      outReal[0] = prevADX;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         outReal[outIdx++] = prevADX;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adx( int startIdx,
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
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
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
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
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
         prevTR += tempReal;
         prevClose = (double)inClose[today];
      }
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
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
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      prevADX = (sumDX / optInTimePeriod);
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      outReal[0] = prevADX;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         outReal[outIdx++] = prevADX;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode adxUnguarded( int startIdx,
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
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
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
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
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
         prevTR += tempReal;
         prevClose = (double)inClose[today];
      }
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
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
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = (double)inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      prevADX = (sumDX / optInTimePeriod);
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      outReal[0] = prevADX;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
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
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         outReal[outIdx++] = prevADX;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live ADX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#adx} over the same series.
    * Open with {@link Core#adxOpen}; there is no close — the handle is
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
   public static final class AdxStream {
      final Core core;
      int optInTimePeriod;
      double prevHigh;
      double prevLow;
      double prevClose;
      double prevMinusDM;
      double prevPlusDM;
      double prevTR;
      double tempReal;
      double diffP;
      double diffM;
      double minusDI;
      double plusDI;
      double prevADX;
      double cur_outReal;

      AdxStream( Core core ) { this.core = core; }

      AdxStream( AdxStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevHigh = other.prevHigh;
         this.prevLow = other.prevLow;
         this.prevClose = other.prevClose;
         this.prevMinusDM = other.prevMinusDM;
         this.prevPlusDM = other.prevPlusDM;
         this.prevTR = other.prevTR;
         this.tempReal = other.tempReal;
         this.diffP = other.diffP;
         this.diffM = other.diffM;
         this.minusDI = other.minusDI;
         this.plusDI = other.plusDI;
         this.prevADX = other.prevADX;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.adxStreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         AdxStream scratch = new AdxStream(this);
         core.adxStreamStep(scratch, inHigh, inLow, inClose);
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
      public AdxStream copy() {
         return new AdxStream(this);
      }
   }
   void adxStreamStep( AdxStream sp, double inHigh, double inLow, double inClose )
   {
      /* Calculate the prevMinusDM and prevPlusDM */
      sp.tempReal = inHigh;
      sp.diffP = sp.tempReal - sp.prevHigh;
      /* Plus Delta */
      sp.prevHigh = sp.tempReal;
      sp.tempReal = inLow;
      sp.diffM = sp.prevLow - sp.tempReal;
      /* Minus Delta */
      sp.prevLow = sp.tempReal;
      sp.prevMinusDM -= sp.prevMinusDM / sp.optInTimePeriod;
      sp.prevPlusDM -= sp.prevPlusDM / sp.optInTimePeriod;
      if( sp.diffM > 0 && sp.diffP < sp.diffM ) {
         /* Case 2 and 4: +DM=0,-DM=diffM */
         sp.prevMinusDM += sp.diffM;
      } else if( sp.diffP > 0 && sp.diffP > sp.diffM ) {
         /* Case 1 and 3: +DM=diffP,-DM=0 */
         sp.prevPlusDM += sp.diffP;
      }
      /* Calculate the prevTR */
      double _true_range_0;
      double range_0 = sp.prevHigh - sp.prevLow;
      double tmp_0 = Math.abs(sp.prevHigh - sp.prevClose);
      if( tmp_0 > range_0 ) {
         range_0 = tmp_0;
      }
      tmp_0 = Math.abs(sp.prevLow - sp.prevClose);
      if( tmp_0 > range_0 ) {
         range_0 = tmp_0;
      }
      _true_range_0 = range_0;
      sp.tempReal = _true_range_0;
      sp.prevTR = sp.prevTR - sp.prevTR / sp.optInTimePeriod + sp.tempReal;
      sp.prevClose = inClose;
      if( !((-0.00000000000001 < sp.prevTR) && (sp.prevTR < 0.00000000000001)) ) {
         /* Calculate the DX. The value is rounded (see Wilder book). */
         sp.minusDI = (100.0 * (sp.prevMinusDM / sp.prevTR));
         sp.plusDI = (100.0 * (sp.prevPlusDM / sp.prevTR));
         sp.tempReal = sp.minusDI + sp.plusDI;
         if( !((-0.00000000000001 < sp.tempReal) && (sp.tempReal < 0.00000000000001)) ) {
            sp.tempReal = (100.0 * (Math.abs(sp.minusDI - sp.plusDI) / sp.tempReal));
            /* Calculate the ADX */
            sp.prevADX = ((sp.prevADX * (sp.optInTimePeriod - 1) + sp.tempReal) / sp.optInTimePeriod);
         }
      }
      /* Output the ADX */
      sp.cur_outReal = sp.prevADX;
   }
   private RetCode adxOpenBody( AdxStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double prevClose = 0;
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
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
       * for a -DM14, sum the -DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous -DM14
       *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
       *                                         14
       *
       * (Same thing for +DM14)
       *
       * Calculation of a -DI14 is as follow:
       *
       *               -DM14
       *     -DI14 =  --------
       *                TR14
       *
       * (Same thing for +DI14)
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
       * Calculation of the DX14 is:
       *
       *    diffDI = ABS( (-DI14) - (+DI14) )
       *    sumDI  = (-DI14) + (+DI14)
       *
       *    DX14 = 100 * (diffDI / sumDI)
       *
       * Calculation of the first ADX:
       *
       *    ADX14 = SUM of the first 14 DX
       *
       * Calculation of subsequent ADX:
       *
       *            ((Previous ADX14)*(14-1))+ Today's DX
       *    ADX14 = -------------------------------------
       *                             14
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
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
      /* Process the initial DM and TR */
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      prevClose = inClose[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
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
      /* Add up all the initial DX. */
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
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
         /* Calculate the DX. The value is rounded (see Wilder book). */
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            /* This loop is just to accumulate the initial DX */
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      /* Calculate the first ADX */
      prevADX = (sumDX / optInTimePeriod);
      /* Skip the unstable period */
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
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
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      /* Output the first ADX */
      lastValue_outReal = prevADX;
      outIdx = 1;
      /* Calculate and output subsequent ADX */
      while( today < endIdx ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         /* Calculate the prevTR */
         double _true_range_4;
         double range_4 = prevHigh - prevLow;
         double tmp_4 = Math.abs(prevHigh - prevClose);
         if( tmp_4 > range_4 ) {
            range_4 = tmp_4;
         }
         tmp_4 = Math.abs(prevLow - prevClose);
         if( tmp_4 > range_4 ) {
            range_4 = tmp_4;
         }
         _true_range_4 = range_4;
         tempReal = _true_range_4;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         /* Output the ADX */
         lastValue_outReal = prevADX;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevHigh = prevHigh;
      sp.prevLow = prevLow;
      sp.prevClose = prevClose;
      sp.prevMinusDM = prevMinusDM;
      sp.prevPlusDM = prevPlusDM;
      sp.prevTR = prevTR;
      sp.tempReal = tempReal;
      sp.diffP = diffP;
      sp.diffM = diffM;
      sp.minusDI = minusDI;
      sp.plusDI = plusDI;
      sp.prevADX = prevADX;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode adxOpenAndFillBody( AdxStream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double prevClose = 0;
      double prevMinusDM = 0;
      double prevPlusDM = 0;
      double prevTR = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double diffP = 0;
      double diffM = 0;
      double minusDI = 0;
      double plusDI = 0;
      double sumDX = 0;
      double prevADX = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
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
       * for a -DM14, sum the -DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous -DM14
       *  Today's -DM14 = Previous -DM14 -  -------------- + Today's -DM1
       *                                         14
       *
       * (Same thing for +DM14)
       *
       * Calculation of a -DI14 is as follow:
       *
       *               -DM14
       *     -DI14 =  --------
       *                TR14
       *
       * (Same thing for +DI14)
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
       * Calculation of the DX14 is:
       *
       *    diffDI = ABS( (-DI14) - (+DI14) )
       *    sumDI  = (-DI14) + (+DI14)
       *
       *    DX14 = 100 * (diffDI / sumDI)
       *
       * Calculation of the first ADX:
       *
       *    ADX14 = SUM of the first 14 DX
       *
       * Calculation of subsequent ADX:
       *
       *            ((Previous ADX14)*(14-1))+ Today's DX
       *    ADX14 = -------------------------------------
       *                             14
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.Adx.ordinal()] - 1;
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
      /* Process the initial DM and TR */
      today = startIdx;
      outBegIdx.value = today;
      prevMinusDM = 0.0;
      prevPlusDM = 0.0;
      prevTR = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      prevClose = inClose[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         double _true_range_5;
         double range_5 = prevHigh - prevLow;
         double tmp_5 = Math.abs(prevHigh - prevClose);
         if( tmp_5 > range_5 ) {
            range_5 = tmp_5;
         }
         tmp_5 = Math.abs(prevLow - prevClose);
         if( tmp_5 > range_5 ) {
            range_5 = tmp_5;
         }
         _true_range_5 = range_5;
         tempReal = _true_range_5;
         prevTR += tempReal;
         prevClose = inClose[today];
      }
      /* Add up all the initial DX. */
      sumDX = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         /* Calculate the prevTR */
         double _true_range_6;
         double range_6 = prevHigh - prevLow;
         double tmp_6 = Math.abs(prevHigh - prevClose);
         if( tmp_6 > range_6 ) {
            range_6 = tmp_6;
         }
         tmp_6 = Math.abs(prevLow - prevClose);
         if( tmp_6 > range_6 ) {
            range_6 = tmp_6;
         }
         _true_range_6 = range_6;
         tempReal = _true_range_6;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         /* Calculate the DX. The value is rounded (see Wilder book). */
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            /* This loop is just to accumulate the initial DX */
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      /* Calculate the first ADX */
      prevADX = (sumDX / optInTimePeriod);
      /* Skip the unstable period */
      i = this.unstablePeriod[FuncUnstId.Adx.ordinal()];
      while( i-- > 0 ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         /* Calculate the prevTR */
         double _true_range_7;
         double range_7 = prevHigh - prevLow;
         double tmp_7 = Math.abs(prevHigh - prevClose);
         if( tmp_7 > range_7 ) {
            range_7 = tmp_7;
         }
         tmp_7 = Math.abs(prevLow - prevClose);
         if( tmp_7 > range_7 ) {
            range_7 = tmp_7;
         }
         _true_range_7 = range_7;
         tempReal = _true_range_7;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
      }
      /* Output the first ADX */
      outReal[0] = prevADX;
      outIdx = 1;
      /* Calculate and output subsequent ADX */
      while( today < endIdx ) {
         /* Calculate the prevMinusDM and prevPlusDM */
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         prevMinusDM -= prevMinusDM / optInTimePeriod;
         prevPlusDM -= prevPlusDM / optInTimePeriod;
         if( diffM > 0 && diffP < diffM ) {
            /* Case 2 and 4: +DM=0,-DM=diffM */
            prevMinusDM += diffM;
         } else if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
         /* Calculate the prevTR */
         double _true_range_8;
         double range_8 = prevHigh - prevLow;
         double tmp_8 = Math.abs(prevHigh - prevClose);
         if( tmp_8 > range_8 ) {
            range_8 = tmp_8;
         }
         tmp_8 = Math.abs(prevLow - prevClose);
         if( tmp_8 > range_8 ) {
            range_8 = tmp_8;
         }
         _true_range_8 = range_8;
         tempReal = _true_range_8;
         prevTR = prevTR - prevTR / optInTimePeriod + tempReal;
         prevClose = inClose[today];
         if( !((-0.00000000000001 < prevTR) && (prevTR < 0.00000000000001)) ) {
            /* Calculate the DX. The value is rounded (see Wilder book). */
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
               /* Calculate the ADX */
               prevADX = ((prevADX * (optInTimePeriod - 1) + tempReal) / optInTimePeriod);
            }
         }
         /* Output the ADX */
         outReal[outIdx++] = prevADX;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevHigh = prevHigh;
      sp.prevLow = prevLow;
      sp.prevClose = prevClose;
      sp.prevMinusDM = prevMinusDM;
      sp.prevPlusDM = prevPlusDM;
      sp.prevTR = prevTR;
      sp.tempReal = tempReal;
      sp.diffP = diffP;
      sp.diffM = diffM;
      sp.minusDI = minusDI;
      sp.plusDI = plusDI;
      sp.prevADX = prevADX;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind adxOpen (composition seam). */
   AdxStream adxOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      AdxStream sp = new AdxStream(this);
      RetCode retCode = adxOpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADX open: internal error");
      }
      throw new IllegalArgumentException("TA_ADX open: " + retCode);
   }
   /**
    * Open a live ADX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#adx} at that bar.
    * <p>The history must hold at least {@code adxLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AdxStream adxOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return adxOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#adxOpen} that also fills the output array(s) bit-identically
    * to {@link Core#adx} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AdxStream adxOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AdxStream sp = new AdxStream(this);
      RetCode retCode = adxOpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ADX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ADX openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ADX openAndFill: " + retCode);
   }
