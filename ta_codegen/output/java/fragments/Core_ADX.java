/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  MIF      Mirek Fontan (mira@fontan.cz)
 *  GC       guycom@users.sourceforge.net
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  082303 MF    Fix #792298. Remove rounding. Bug reported by AM.
 *  062704 MF    Fix #965557. Div by zero bug reported by MIF.
 *  082206 MF    Fix #1544555. Div by zero bug reported by GC.
 *  082326 MF,CC Fix #253. Test the true-range sum exactly instead of against
 *               the fixed TA_IS_ZERO band, which zeroed the index for any
 *               instrument quoted small enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#ADX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Smoothing/averaging period for DM, TR, and ADX
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ADX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.ADX.ordinal()] - 1 ;

   }
   RetCode ADX_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.ADX.ordinal()] - 1;
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
         /* Calculate the DX. The value is rounded (see Wilder book).
          *
          * prevTR is a running sum of true ranges: non-negative by construction
          * and built only by adding, so it carries no cancellation residue and
          * reaches zero only for a window whose every range is exactly zero. Test
          * it exactly. A true range carries the quote unit, so the fixed
          * TA_IS_ZERO band it used to be compared against was a constant in some
          * arbitrary unit, and zeroed the index for any instrument quoted below
          * it (issue #253). The DI legs it feeds are ratios -- dimensionless --
          * so the fixed band on THEIR sum is scale-invariant and stays.
          */
         if( prevTR > 0.0 ) {
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
      i = this.unstablePeriod[FuncUnstId.ADX.ordinal()];
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
         if( prevTR > 0.0 ) {
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
         if( prevTR > 0.0 ) {
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
   RetCode ADX_Impl( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.ADX.ordinal()] - 1;
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
         if( prevTR > 0.0 ) {
            minusDI = (100.0 * (prevMinusDM / prevTR));
            plusDI = (100.0 * (prevPlusDM / prevTR));
            tempReal = minusDI + plusDI;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sumDX += (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            }
         }
      }
      prevADX = (sumDX / optInTimePeriod);
      i = this.unstablePeriod[FuncUnstId.ADX.ordinal()];
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
         if( prevTR > 0.0 ) {
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
         if( prevTR > 0.0 ) {
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
   /**
    * Wilder's Average Directional Movement Index, a smoothed measure of trend
    * strength derived from the directional indicators (+DI/-DI). Quantifies how
    * strongly a market is trending, regardless of direction. Higher values
    * indicate a stronger trend (a common convention treats &gt;25 as trending);
    * says nothing about direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * +DI = 100*(+DM_p/TR_p), -DI = 100*(-DM_p/TR_p); DX = 100*|(-DI)-(+DI)| / ((-DI)+(+DI)); first ADX = mean of the first `period` DX; then ADX = (prevADX*(period-1) + DX)/period. +DM_p/-DM_p/TR_p use Wilder smoothing: X = X - X/period + today's one-bar value.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's original integer rounding is not applied.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADX_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing/averaging period for DM, TR, and ADX
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Smoothed directional trend-strength index (0-100) Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#ADXR
    * @see Core#DX
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#PLUS_DM
    * @see Core#MINUS_DM
    * @see Core#TRANGE
    */
   public OutRange ADX( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("ADX", startIdx, endIdx);
      int guardStart = clampedStart("ADX", startIdx, ADX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADX", "inHigh", inHigh, guardInLen);
      requireLength("ADX", "inLow", inLow, guardInLen);
      requireLength("ADX", "inClose", inClose, guardInLen);
      requireLength("ADX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADX_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder's Average Directional Movement Index, a smoothed measure of trend
    * strength derived from the directional indicators (+DI/-DI). Quantifies how
    * strongly a market is trending, regardless of direction. Higher values
    * indicate a stronger trend (a common convention treats &gt;25 as trending);
    * says nothing about direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * +DI = 100*(+DM_p/TR_p), -DI = 100*(-DM_p/TR_p); DX = 100*|(-DI)-(+DI)| / ((-DI)+(+DI)); first ADX = mean of the first `period` DX; then ADX = (prevADX*(period-1) + DX)/period. +DM_p/-DM_p/TR_p use Wilder smoothing: X = X - X/period + today's one-bar value.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder's original integer rounding is not applied.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADX_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing/averaging period for DM, TR, and ADX
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Smoothed directional trend-strength index (0-100) Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#ADXR
    * @see Core#DX
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#PLUS_DM
    * @see Core#MINUS_DM
    * @see Core#TRANGE
    */
   public OutRange ADX( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("ADX", startIdx, endIdx);
      int guardStart = clampedStart("ADX", startIdx, ADX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADX", "inHigh", inHigh, guardInLen);
      requireLength("ADX", "inLow", inLow, guardInLen);
      requireLength("ADX", "inClose", inClose, guardInLen);
      requireLength("ADX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADX_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ADX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ADX} over the same series.
    * Open with {@link Core#ADX_Open}; there is no close — the handle is
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
   public static final class ADX_Stream {
      Core core;
      int optInTimePeriod;
      double prevHigh;
      double prevLow;
      double prevClose;
      double prevMinusDM;
      double prevPlusDM;
      double prevTR;
      double prevADX;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      ADX_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ADX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      ADX_Stream( ADX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevHigh = other.prevHigh;
         this.prevLow = other.prevLow;
         this.prevClose = other.prevClose;
         this.prevMinusDM = other.prevMinusDM;
         this.prevPlusDM = other.prevPlusDM;
         this.prevTR = other.prevTR;
         this.prevADX = other.prevADX;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( ADX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevHigh = other.prevHigh;
         this.prevLow = other.prevLow;
         this.prevClose = other.prevClose;
         this.prevMinusDM = other.prevMinusDM;
         this.prevPlusDM = other.prevPlusDM;
         this.prevTR = other.prevTR;
         this.prevADX = other.prevADX;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ADX update: BadParam", RetCode.BadParam);
         core.ADX_StepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("ADX updateAndFill", "inHigh", inHigh);
         requireArgument("ADX updateAndFill", "inLow", inLow);
         requireArgument("ADX updateAndFill", "inClose", inClose);
         requireArgument("ADX updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("ADX updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("ADX updateAndFill: BadParam", RetCode.BadParam);
            core.ADX_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ADX peek: BadParam", RetCode.BadParam);
         ADX_Stream scratch = new ADX_Stream(this);
         core.ADX_StepImpl(scratch, inHigh, inLow, inClose);
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
      public ADX_Stream copy() {
         return new ADX_Stream(this);
      }
   }
   void ADX_StepImpl( ADX_Stream sp, double inHigh, double inLow, double inClose )
   {
      double tempReal = 0.0;
      double diffP = 0.0;
      double diffM = 0.0;
      double minusDI = 0.0;
      double plusDI = 0.0;
      /* Calculate the prevMinusDM and prevPlusDM */
      tempReal = inHigh;
      diffP = tempReal - sp.prevHigh;
      /* Plus Delta */
      sp.prevHigh = tempReal;
      tempReal = inLow;
      diffM = sp.prevLow - tempReal;
      /* Minus Delta */
      sp.prevLow = tempReal;
      sp.prevMinusDM -= sp.prevMinusDM / sp.optInTimePeriod;
      sp.prevPlusDM -= sp.prevPlusDM / sp.optInTimePeriod;
      if( diffM > 0 && diffP < diffM ) {
         /* Case 2 and 4: +DM=0,-DM=diffM */
         sp.prevMinusDM += diffM;
      } else if( diffP > 0 && diffP > diffM ) {
         /* Case 1 and 3: +DM=diffP,-DM=0 */
         sp.prevPlusDM += diffP;
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
      tempReal = _true_range_0;
      sp.prevTR = sp.prevTR - sp.prevTR / sp.optInTimePeriod + tempReal;
      sp.prevClose = inClose;
      if( sp.prevTR > 0.0 ) {
         /* Calculate the DX. The value is rounded (see Wilder book). */
         minusDI = (100.0 * (sp.prevMinusDM / sp.prevTR));
         plusDI = (100.0 * (sp.prevPlusDM / sp.prevTR));
         tempReal = minusDI + plusDI;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = (100.0 * (Math.abs(minusDI - plusDI) / tempReal));
            /* Calculate the ADX */
            sp.prevADX = ((sp.prevADX * (sp.optInTimePeriod - 1) + tempReal) / sp.optInTimePeriod);
         }
      }
      /* Output the ADX */
      sp.cur_outReal = sp.prevADX;
   }
   private RetCode ADX_OpenImpl( ADX_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      lookbackTotal = 2 * optInTimePeriod + this.unstablePeriod[FuncUnstId.ADX.ordinal()] - 1;
      /* Adjust startIdx to account for the lookback period. */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
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
         /* Calculate the DX. The value is rounded (see Wilder book).
          *
          * prevTR is a running sum of true ranges: non-negative by construction
          * and built only by adding, so it carries no cancellation residue and
          * reaches zero only for a window whose every range is exactly zero. Test
          * it exactly. A true range carries the quote unit, so the fixed
          * TA_IS_ZERO band it used to be compared against was a constant in some
          * arbitrary unit, and zeroed the index for any instrument quoted below
          * it (issue #253). The DI legs it feeds are ratios -- dimensionless --
          * so the fixed band on THEIR sum is scale-invariant and stays.
          */
         if( prevTR > 0.0 ) {
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
      i = this.unstablePeriod[FuncUnstId.ADX.ordinal()];
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
         if( prevTR > 0.0 ) {
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
      outReal[0 * outStride] = prevADX;
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
         if( prevTR > 0.0 ) {
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
         outReal[outIdx++ * outStride] = prevADX;
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
      sp.prevADX = prevADX;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* ADX_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ADX_Stream ADX_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ADX_Stream sp = new ADX_Stream(this);
      RetCode retCode = ADX_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ADX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ADX_Open (composition seam). */
   ADX_Stream ADX_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      ADX_Stream sp = new ADX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ADX_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADX open: internal error", retCode);
      }
      throw new TaLibArgumentException("ADX open: " + retCode, retCode);
   }
   /**
    * Open a live ADX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ADX} at that bar.
    * <p>The history must hold at least {@code ADX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ADX_Stream ADX_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("ADX open", "inHigh", inHigh);
      requireHistory("ADX open", inHigh.length);
      requireArgument("ADX open", "inLow", inLow);
      requireArgument("ADX open", "inClose", inClose);
      requireHistoryLength("ADX open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ADX open", "inClose", inClose.length, inHigh.length);
      return ADX_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#ADX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#ADX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ADX_Stream#outRange()}.
    */
   public ADX_Stream ADX_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("ADX openAndFill", "inHigh", inHigh);
      requireHistory("ADX openAndFill", inHigh.length);
      requireArgument("ADX openAndFill", "inLow", inLow);
      requireArgument("ADX openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ADX openAndFill", inHigh.length, ADX_Lookback(optInTimePeriod));
      requireHistoryLength("ADX openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ADX openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("ADX openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("ADX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ADX_OpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
