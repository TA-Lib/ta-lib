/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  091503 PP    Reworked TA_SAR to allow customisation of more SAR params.
 *  092103 MF    Some changes related on first round of tests
 *  092303 PP    Minor bug fixes.
 *  122104 MF,CF Fix#1089506 for out-of-bound access to ep_temp.
 *  082726 MF,CC Answer a rejected minus_dm before reading ep_temp, not after:
 *               the read was of an uninitialised local.
 */

   /**
    * Number of leading input bars {@link Core#SAREXT} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInStartValue Initial SAR/direction: 0 auto, &gt;0 start long at
    *        value, &lt;0 start short at |value| (default 0; {@code -4e37} selects the
    *        default).
    * @param optInOffsetOnReverse Fractional offset applied to the stop on each
    *        reversal (default 0; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitLong Initial acceleration factor when long
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationLong AF increment per new long extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxLong Cap on the long acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitShort Initial acceleration factor when short
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationShort AF increment per new short extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxShort Cap on the short acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SAREXT_Lookback( double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort )
   {
      if( optInStartValue == REAL_DEFAULT ) {
         optInStartValue = 0e0;
      } else if( !(optInStartValue >= REAL_MIN && optInStartValue <= REAL_MAX) ) {
         return -1;
      }
      if( optInOffsetOnReverse == REAL_DEFAULT ) {
         optInOffsetOnReverse = 0e0;
      } else if( !(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationInitLong == REAL_DEFAULT ) {
         optInAccelerationInitLong = 2e-2;
      } else if( !(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationLong == REAL_DEFAULT ) {
         optInAccelerationLong = 2e-2;
      } else if( !(optInAccelerationLong >= 0e0 && optInAccelerationLong <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationMaxLong == REAL_DEFAULT ) {
         optInAccelerationMaxLong = 2e-1;
      } else if( !(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationInitShort == REAL_DEFAULT ) {
         optInAccelerationInitShort = 2e-2;
      } else if( !(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationShort == REAL_DEFAULT ) {
         optInAccelerationShort = 2e-2;
      } else if( !(optInAccelerationShort >= 0e0 && optInAccelerationShort <= REAL_MAX) ) {
         return -1;
      }
      if( optInAccelerationMaxShort == REAL_DEFAULT ) {
         optInAccelerationMaxShort = 2e-1;
      } else if( !(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= REAL_MAX) ) {
         return -1;
      }
      /* SAR always sacrifices one price bar to establish the
       * initial extreme price.
       */
      return 1 ;

   }
   RetCode SAREXT_Impl( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double optInStartValue,
                        double optInOffsetOnReverse,
                        double optInAccelerationInitLong,
                        double optInAccelerationLong,
                        double optInAccelerationMaxLong,
                        double optInAccelerationInitShort,
                        double optInAccelerationShort,
                        double optInAccelerationMaxShort,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double afLong = 0;
      double afShort = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInStartValue == REAL_DEFAULT ) {
         optInStartValue = 0e0;
      } else if( !(optInStartValue >= REAL_MIN && optInStartValue <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInOffsetOnReverse == REAL_DEFAULT ) {
         optInOffsetOnReverse = 0e0;
      } else if( !(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitLong == REAL_DEFAULT ) {
         optInAccelerationInitLong = 2e-2;
      } else if( !(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationLong == REAL_DEFAULT ) {
         optInAccelerationLong = 2e-2;
      } else if( !(optInAccelerationLong >= 0e0 && optInAccelerationLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxLong == REAL_DEFAULT ) {
         optInAccelerationMaxLong = 2e-1;
      } else if( !(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitShort == REAL_DEFAULT ) {
         optInAccelerationInitShort = 2e-2;
      } else if( !(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationShort == REAL_DEFAULT ) {
         optInAccelerationShort = 2e-2;
      } else if( !(optInAccelerationShort >= 0e0 && optInAccelerationShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxShort == REAL_DEFAULT ) {
         optInAccelerationMaxShort = 2e-1;
      } else if( !(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      /* > 0 indicates long. == 0 indicates short */
      /* This function is the same as TA_SAR, except that the caller has
       * greater control on the SAR dynamic and initial state.
       *
       * In additon, the TA_SAREXT returns negative values when the position
       * is short. This allow to distinguish when the SAR do actually reverse.
       */
      /* Implementation of the SAR has been a little bit open to interpretation
       * since Wilder (the original author) did not define a precise algorithm
       * on how to bootstrap the algorithm. Take any existing software application
       * and you will see slight variation on how the algorithm was adapted.
       *
       * What is the initial trade direction? Long or short?
       * ===================================================
       * The interpretation of what should be the initial SAR values is
       * open to interpretation, particularly since the caller to the function
       * does not specify the initial direction of the trade.
       *
       * In TA-Lib, the following default logic is used:
       *  - Calculate +DM and -DM between the first and
       *    second bar. The highest directional indication will
       *    indicate the assumed direction of the trade for the second
       *    price bar.
       *  - In the case of a tie between +DM and -DM,
       *    the direction is LONG by default.
       *
       * What is the initial "extreme point" and thus SAR?
       * =================================================
       * The following shows how different people took different approach:
       *  - Metastock use the first price bar high/low depending of
       *    the direction. No SAR is calculated for the first price
       *    bar.
       *  - Tradestation use the closing price of the second bar. No
       *    SAR are calculated for the first price bar.
       *  - Wilder (the original author) use the SIP from the
       *    previous trade (cannot be implement here since the
       *    direction and length of the previous trade is unknonw).
       *  - The Magazine TASC seems to follow Wilder approach which
       *    is not practical here.
       *
       * TA-Lib "consume" the first price bar and use its high/low as the
       * initial SAR of the second price bar. I found that approach to be
       * the closest to Wilders idea of having the first entry day use
       * the previous extreme point, except that here the extreme point is
       * derived solely from the first price bar. I found the same approach
       * to be used by Metastock.
       *
       *
       * Can I force the initial SAR?
       * ============================
       * Yes. Using the optInStartValue_0 parameter:
       *  optInStartValue_0 >  0 : SAR is long at optInStartValue_0.
       *  optInStartValue_0 <  0 : SAR is short at fabs(optInStartValue_0).
       *
       * And when optInStartValue_0 == 0, the logic is the same as for TA_SAR
       * (See previous two sections).
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       *
       * Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Check if the acceleration factors are being defined by the user.
       * Make sure the acceleration and maximum are coherent.
       * If not, correct the acceleration.
       * Default afLong = 0.02
       * Default afShort = 0.02
       */
      afLong = optInAccelerationInitLong;
      afShort = optInAccelerationInitShort;
      if( afLong > optInAccelerationMaxLong ) {
         optInAccelerationInitLong = optInAccelerationMaxLong;
         afLong = optInAccelerationInitLong;
      }
      if( optInAccelerationLong > optInAccelerationMaxLong ) {
         optInAccelerationLong = optInAccelerationMaxLong;
      }
      if( afShort > optInAccelerationMaxShort ) {
         optInAccelerationInitShort = optInAccelerationMaxShort;
         afShort = optInAccelerationInitShort;
      }
      if( optInAccelerationShort > optInAccelerationMaxShort ) {
         optInAccelerationShort = optInAccelerationMaxShort;
      }
      /* Initialise SAR calculations */
      if( optInStartValue == 0 ) {
         /* Default action */
         /* Identify if the initial direction is long or short.
          * (ep is just used as a temp buffer here, the name
          *  of the parameter is not significant).
          */
         OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
         tempInt.value = _xr0.begIdx();
         tempInt.value = _xr0.count();
         retCode = RetCode.Success;
         if( ep_temp[0] > 0 ) {
            isLong = 0;
         } else {
            isLong = 1;
         }
      } else if( optInStartValue > 0 ) {
         /* Start Long */
         isLong = 1;
      } else {
         /* optInStartValue_0 < 0 => Start Short */
         isLong = 0;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      /* Write the first SAR. */
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( optInStartValue == 0 ) {
         /* Default action */
         if( isLong == 1 ) {
            ep = inHigh[todayIdx];
            sar = newLow;
         } else {
            ep = inLow[todayIdx];
            sar = newHigh;
         }
      } else if( optInStartValue > 0 ) {
         /* Start Long at specified value. */
         ep = inHigh[todayIdx];
         sar = optInStartValue;
      } else {
         /* if optInStartValue < 0 => Start Short at specified value. */
         ep = inLow[todayIdx];
         sar = Math.abs(optInStartValue);
      }
      /* Cheat on the newLow and newHigh for the
       * first iteration.
       */
      newLow = inLow[todayIdx];
      newHigh = inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = inLow[todayIdx];
         newHigh = inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            /* Switch to short if the low penetrates the SAR value. */
            if( newLow <= sar ) {
               /* Switch and Overide the SAR with the ep */
               isLong = 0;
               sar = ep;
               /* Make sure the overide SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               /* Output the overide SAR */
               if( optInOffsetOnReverse != 0.0 ) {
                  sar += sar * optInOffsetOnReverse;
               }
               outReal[outIdx++] = 0 - sar;
               /* Adjust afShort and ep */
               afShort = optInAccelerationInitShort;
               ep = newLow;
               /* Calculate the new SAR */
               sar = Math.fma(afShort, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               /* No switch */
               /* Output the SAR (was calculated in the previous iteration) */
               outReal[outIdx++] = sar;
               /* Adjust afLong and ep. */
               if( newHigh > ep ) {
                  ep = newHigh;
                  afLong += optInAccelerationLong;
                  if( afLong > optInAccelerationMaxLong ) {
                     afLong = optInAccelerationMaxLong;
                  }
               }
               /* Calculate the new SAR */
               sar = Math.fma(afLong, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         /* Switch to long if the high penetrates the SAR value. */
         } else if( newHigh >= sar ) {
            /* Switch and Overide the SAR with the ep */
            isLong = 1;
            sar = ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            /* Output the overide SAR */
            if( optInOffsetOnReverse != 0.0 ) {
               sar -= sar * optInOffsetOnReverse;
            }
            outReal[outIdx++] = sar;
            /* Adjust afLong and ep */
            afLong = optInAccelerationInitLong;
            ep = newHigh;
            /* Calculate the new SAR */
            sar = Math.fma(afLong, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            outReal[outIdx++] = 0 - sar;
            /* Adjust afShort and ep. */
            if( newLow < ep ) {
               ep = newLow;
               afShort += optInAccelerationShort;
               if( afShort > optInAccelerationMaxShort ) {
                  afShort = optInAccelerationMaxShort;
               }
            }
            /* Calculate the new SAR */
            sar = Math.fma(afShort, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode SAREXT_Impl( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        double optInStartValue,
                        double optInOffsetOnReverse,
                        double optInAccelerationInitLong,
                        double optInAccelerationLong,
                        double optInAccelerationMaxLong,
                        double optInAccelerationInitShort,
                        double optInAccelerationShort,
                        double optInAccelerationMaxShort,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double afLong = 0;
      double afShort = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInStartValue == REAL_DEFAULT ) {
         optInStartValue = 0e0;
      } else if( !(optInStartValue >= REAL_MIN && optInStartValue <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInOffsetOnReverse == REAL_DEFAULT ) {
         optInOffsetOnReverse = 0e0;
      } else if( !(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitLong == REAL_DEFAULT ) {
         optInAccelerationInitLong = 2e-2;
      } else if( !(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationLong == REAL_DEFAULT ) {
         optInAccelerationLong = 2e-2;
      } else if( !(optInAccelerationLong >= 0e0 && optInAccelerationLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxLong == REAL_DEFAULT ) {
         optInAccelerationMaxLong = 2e-1;
      } else if( !(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitShort == REAL_DEFAULT ) {
         optInAccelerationInitShort = 2e-2;
      } else if( !(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationShort == REAL_DEFAULT ) {
         optInAccelerationShort = 2e-2;
      } else if( !(optInAccelerationShort >= 0e0 && optInAccelerationShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxShort == REAL_DEFAULT ) {
         optInAccelerationMaxShort = 2e-1;
      } else if( !(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      afLong = optInAccelerationInitLong;
      afShort = optInAccelerationInitShort;
      if( afLong > optInAccelerationMaxLong ) {
         optInAccelerationInitLong = optInAccelerationMaxLong;
         afLong = optInAccelerationInitLong;
      }
      if( optInAccelerationLong > optInAccelerationMaxLong ) {
         optInAccelerationLong = optInAccelerationMaxLong;
      }
      if( afShort > optInAccelerationMaxShort ) {
         optInAccelerationInitShort = optInAccelerationMaxShort;
         afShort = optInAccelerationInitShort;
      }
      if( optInAccelerationShort > optInAccelerationMaxShort ) {
         optInAccelerationShort = optInAccelerationMaxShort;
      }
      if( optInStartValue == 0 ) {
         OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
         tempInt.value = _xr0.begIdx();
         tempInt.value = _xr0.count();
         retCode = RetCode.Success;
         if( ep_temp[0] > 0 ) {
            isLong = 0;
         } else {
            isLong = 1;
         }
      } else if( optInStartValue > 0 ) {
         isLong = 1;
      } else {
         isLong = 0;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      todayIdx = startIdx;
      newHigh = (double)inHigh[todayIdx - 1];
      newLow = (double)inLow[todayIdx - 1];
      if( optInStartValue == 0 ) {
         if( isLong == 1 ) {
            ep = (double)inHigh[todayIdx];
            sar = newLow;
         } else {
            ep = (double)inLow[todayIdx];
            sar = newHigh;
         }
      } else if( optInStartValue > 0 ) {
         ep = (double)inHigh[todayIdx];
         sar = optInStartValue;
      } else {
         ep = (double)inLow[todayIdx];
         sar = Math.abs(optInStartValue);
      }
      newLow = (double)inLow[todayIdx];
      newHigh = (double)inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = (double)inLow[todayIdx];
         newHigh = (double)inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            if( newLow <= sar ) {
               isLong = 0;
               sar = ep;
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               if( optInOffsetOnReverse != 0.0 ) {
                  sar += sar * optInOffsetOnReverse;
               }
               outReal[outIdx++] = 0 - sar;
               afShort = optInAccelerationInitShort;
               ep = newLow;
               sar = Math.fma(afShort, ep - sar, sar);
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               outReal[outIdx++] = sar;
               if( newHigh > ep ) {
                  ep = newHigh;
                  afLong += optInAccelerationLong;
                  if( afLong > optInAccelerationMaxLong ) {
                     afLong = optInAccelerationMaxLong;
                  }
               }
               sar = Math.fma(afLong, ep - sar, sar);
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         } else if( newHigh >= sar ) {
            isLong = 1;
            sar = ep;
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            if( optInOffsetOnReverse != 0.0 ) {
               sar -= sar * optInOffsetOnReverse;
            }
            outReal[outIdx++] = sar;
            afLong = optInAccelerationInitLong;
            ep = newHigh;
            sar = Math.fma(afLong, ep - sar, sar);
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            outReal[outIdx++] = 0 - sar;
            if( newLow < ep ) {
               ep = newLow;
               afShort += optInAccelerationShort;
               if( afShort > optInAccelerationMaxShort ) {
                  afShort = optInAccelerationMaxShort;
               }
            }
            sar = Math.fma(afShort, ep - sar, sar);
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Extended Parabolic SAR (stop and reverse) giving the caller full control
    * over the initial state and separate acceleration factors for long and
    * short positions. Unlike SAR, it returns negative values while short so
    * reversals are distinguishable. Sign flip of the output marks a trend
    * reversal (positive=long stop, negative=short stop).
    * <p><b>Formula</b>
    * <pre>{@code
    * SAR_next = SAR + AF*(EP - SAR), then clamped within the prior and current bar's range. On penetration, reverse: set SAR=EP (clamped), reset AF to its Init value, EP=extreme of the new direction. Output is +SAR when long, -SAR when short. On reversal an optional offset is applied: long->short SAR*(1+offset), short->long SAR*(1-offset).
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SAREXT_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInStartValue Initial SAR/direction: 0 auto, &gt;0 start long at
    *        value, &lt;0 start short at |value| (default 0; {@code -4e37} selects the
    *        default).
    * @param optInOffsetOnReverse Fractional offset applied to the stop on each
    *        reversal (default 0; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitLong Initial acceleration factor when long
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationLong AF increment per new long extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxLong Cap on the long acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitShort Initial acceleration factor when short
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationShort AF increment per new short extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxShort Cap on the short acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @param outReal SAR stop level; positive while long, negative while short.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SAR
    * @see Core#MINUS_DM
    */
   public OutRange SAREXT( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double optInStartValue,
                           double optInOffsetOnReverse,
                           double optInAccelerationInitLong,
                           double optInAccelerationLong,
                           double optInAccelerationMaxLong,
                           double optInAccelerationInitShort,
                           double optInAccelerationShort,
                           double optInAccelerationMaxShort,
                           double outReal[] )
   {
      requireIndexRange("SAREXT", startIdx, endIdx);
      int guardStart = clampedStart("SAREXT", startIdx, SAREXT_Lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SAREXT", "inHigh", inHigh, guardInLen);
      requireLength("SAREXT", "inLow", inLow, guardInLen);
      requireLength("SAREXT", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SAREXT_Impl(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SAREXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Extended Parabolic SAR (stop and reverse) giving the caller full control
    * over the initial state and separate acceleration factors for long and
    * short positions. Unlike SAR, it returns negative values while short so
    * reversals are distinguishable. Sign flip of the output marks a trend
    * reversal (positive=long stop, negative=short stop).
    * <p><b>Formula</b>
    * <pre>{@code
    * SAR_next = SAR + AF*(EP - SAR), then clamped within the prior and current bar's range. On penetration, reverse: set SAR=EP (clamped), reset AF to its Init value, EP=extreme of the new direction. Output is +SAR when long, -SAR when short. On reversal an optional offset is applied: long->short SAR*(1+offset), short->long SAR*(1-offset).
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SAREXT_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInStartValue Initial SAR/direction: 0 auto, &gt;0 start long at
    *        value, &lt;0 start short at |value| (default 0; {@code -4e37} selects the
    *        default).
    * @param optInOffsetOnReverse Fractional offset applied to the stop on each
    *        reversal (default 0; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitLong Initial acceleration factor when long
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationLong AF increment per new long extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxLong Cap on the long acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationInitShort Initial acceleration factor when short
    *        (default 0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationShort AF increment per new short extreme (default
    *        0.02; minimum 0; {@code -4e37} selects the default).
    * @param optInAccelerationMaxShort Cap on the short acceleration factor
    *        (default 0.2; minimum 0; {@code -4e37} selects the default).
    * @param outReal SAR stop level; positive while long, negative while short.
    *        Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SAR
    * @see Core#MINUS_DM
    */
   public OutRange SAREXT( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           double optInStartValue,
                           double optInOffsetOnReverse,
                           double optInAccelerationInitLong,
                           double optInAccelerationLong,
                           double optInAccelerationMaxLong,
                           double optInAccelerationInitShort,
                           double optInAccelerationShort,
                           double optInAccelerationMaxShort,
                           double outReal[] )
   {
      requireIndexRange("SAREXT", startIdx, endIdx);
      int guardStart = clampedStart("SAREXT", startIdx, SAREXT_Lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SAREXT", "inHigh", inHigh, guardInLen);
      requireLength("SAREXT", "inLow", inLow, guardInLen);
      requireLength("SAREXT", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SAREXT_Impl(startIdx, endIdx, inHigh, inLow, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SAREXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SAREXT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SAREXT} over the same series.
    * Open with {@link Core#SAREXT_Open}; there is no close — the handle is
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
   public static final class SAREXT_Stream {
      Core core;
      double optInStartValue;
      double optInOffsetOnReverse;
      double optInAccelerationInitLong;
      double optInAccelerationLong;
      double optInAccelerationMaxLong;
      double optInAccelerationInitShort;
      double optInAccelerationShort;
      double optInAccelerationMaxShort;
      int isLong;
      double newHigh;
      double newLow;
      double afLong;
      double afShort;
      double ep;
      double sar;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      SAREXT_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#SAREXT} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      SAREXT_Stream( SAREXT_Stream other ) {
         this.core = other.core;
         this.optInStartValue = other.optInStartValue;
         this.optInOffsetOnReverse = other.optInOffsetOnReverse;
         this.optInAccelerationInitLong = other.optInAccelerationInitLong;
         this.optInAccelerationLong = other.optInAccelerationLong;
         this.optInAccelerationMaxLong = other.optInAccelerationMaxLong;
         this.optInAccelerationInitShort = other.optInAccelerationInitShort;
         this.optInAccelerationShort = other.optInAccelerationShort;
         this.optInAccelerationMaxShort = other.optInAccelerationMaxShort;
         this.isLong = other.isLong;
         this.newHigh = other.newHigh;
         this.newLow = other.newLow;
         this.afLong = other.afLong;
         this.afShort = other.afShort;
         this.ep = other.ep;
         this.sar = other.sar;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( SAREXT_Stream other ) {
         this.core = other.core;
         this.optInStartValue = other.optInStartValue;
         this.optInOffsetOnReverse = other.optInOffsetOnReverse;
         this.optInAccelerationInitLong = other.optInAccelerationInitLong;
         this.optInAccelerationLong = other.optInAccelerationLong;
         this.optInAccelerationMaxLong = other.optInAccelerationMaxLong;
         this.optInAccelerationInitShort = other.optInAccelerationInitShort;
         this.optInAccelerationShort = other.optInAccelerationShort;
         this.optInAccelerationMaxShort = other.optInAccelerationMaxShort;
         this.isLong = other.isLong;
         this.newHigh = other.newHigh;
         this.newLow = other.newLow;
         this.afLong = other.afLong;
         this.afShort = other.afShort;
         this.ep = other.ep;
         this.sar = other.sar;
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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("SAREXT update: BadParam", RetCode.BadParam);
         core.SAREXT_StepImpl(this, inHigh, inLow);
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
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("SAREXT updateAndFill", "inHigh", inHigh);
         requireArgument("SAREXT updateAndFill", "inLow", inLow);
         requireArgument("SAREXT updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("SAREXT updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("SAREXT updateAndFill: BadParam", RetCode.BadParam);
            core.SAREXT_StepImpl(this, inHigh[i], inLow[i]);
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("SAREXT peek: BadParam", RetCode.BadParam);
         SAREXT_Stream scratch = new SAREXT_Stream(this);
         core.SAREXT_StepImpl(scratch, inHigh, inLow);
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
      public SAREXT_Stream copy() {
         return new SAREXT_Stream(this);
      }
   }
   void SAREXT_StepImpl( SAREXT_Stream sp, double inHigh, double inLow )
   {
      double prevHigh = 0.0;
      double prevLow = 0.0;
      prevLow = sp.newLow;
      prevHigh = sp.newHigh;
      sp.newLow = inLow;
      sp.newHigh = inHigh;
      if( sp.isLong == 1 ) {
         /* Switch to short if the low penetrates the SAR value. */
         if( sp.newLow <= sp.sar ) {
            /* Switch and Overide the SAR with the ep */
            sp.isLong = 0;
            sp.sar = sp.ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar < prevHigh ) {
               sp.sar = prevHigh;
            }
            if( sp.sar < sp.newHigh ) {
               sp.sar = sp.newHigh;
            }
            /* Output the overide SAR */
            if( sp.optInOffsetOnReverse != 0.0 ) {
               sp.sar += sp.sar * sp.optInOffsetOnReverse;
            }
            sp.cur_outReal = 0 - sp.sar;
            /* Adjust afShort and ep */
            sp.afShort = sp.optInAccelerationInitShort;
            sp.ep = sp.newLow;
            /* Calculate the new SAR */
            sp.sar = Math.fma(sp.afShort, sp.ep - sp.sar, sp.sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar < prevHigh ) {
               sp.sar = prevHigh;
            }
            if( sp.sar < sp.newHigh ) {
               sp.sar = sp.newHigh;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            sp.cur_outReal = sp.sar;
            /* Adjust afLong and ep. */
            if( sp.newHigh > sp.ep ) {
               sp.ep = sp.newHigh;
               sp.afLong += sp.optInAccelerationLong;
               if( sp.afLong > sp.optInAccelerationMaxLong ) {
                  sp.afLong = sp.optInAccelerationMaxLong;
               }
            }
            /* Calculate the new SAR */
            sp.sar = Math.fma(sp.afLong, sp.ep - sp.sar, sp.sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar > prevLow ) {
               sp.sar = prevLow;
            }
            if( sp.sar > sp.newLow ) {
               sp.sar = sp.newLow;
            }
         }
      /* Switch to long if the high penetrates the SAR value. */
      } else if( sp.newHigh >= sp.sar ) {
         /* Switch and Overide the SAR with the ep */
         sp.isLong = 1;
         sp.sar = sp.ep;
         /* Make sure the overide SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar > prevLow ) {
            sp.sar = prevLow;
         }
         if( sp.sar > sp.newLow ) {
            sp.sar = sp.newLow;
         }
         /* Output the overide SAR */
         if( sp.optInOffsetOnReverse != 0.0 ) {
            sp.sar -= sp.sar * sp.optInOffsetOnReverse;
         }
         sp.cur_outReal = sp.sar;
         /* Adjust afLong and ep */
         sp.afLong = sp.optInAccelerationInitLong;
         sp.ep = sp.newHigh;
         /* Calculate the new SAR */
         sp.sar = Math.fma(sp.afLong, sp.ep - sp.sar, sp.sar);
         /* Make sure the new SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar > prevLow ) {
            sp.sar = prevLow;
         }
         if( sp.sar > sp.newLow ) {
            sp.sar = sp.newLow;
         }
      } else {
         /* No switch */
         /* Output the SAR (was calculated in the previous iteration) */
         sp.cur_outReal = 0 - sp.sar;
         /* Adjust afShort and ep. */
         if( sp.newLow < sp.ep ) {
            sp.ep = sp.newLow;
            sp.afShort += sp.optInAccelerationShort;
            if( sp.afShort > sp.optInAccelerationMaxShort ) {
               sp.afShort = sp.optInAccelerationMaxShort;
            }
         }
         /* Calculate the new SAR */
         sp.sar = Math.fma(sp.afShort, sp.ep - sp.sar, sp.sar);
         /* Make sure the new SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar < prevHigh ) {
            sp.sar = prevHigh;
         }
         if( sp.sar < sp.newHigh ) {
            sp.sar = sp.newHigh;
         }
      }
   }
   private RetCode SAREXT_OpenImpl( SAREXT_Stream sp, double inHigh[], double inLow[], int startIdx, double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double afLong = 0;
      double afShort = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInStartValue == REAL_DEFAULT ) {
         optInStartValue = 0e0;
      } else if( !(optInStartValue >= REAL_MIN && optInStartValue <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInOffsetOnReverse == REAL_DEFAULT ) {
         optInOffsetOnReverse = 0e0;
      } else if( !(optInOffsetOnReverse >= 0e0 && optInOffsetOnReverse <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitLong == REAL_DEFAULT ) {
         optInAccelerationInitLong = 2e-2;
      } else if( !(optInAccelerationInitLong >= 0e0 && optInAccelerationInitLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationLong == REAL_DEFAULT ) {
         optInAccelerationLong = 2e-2;
      } else if( !(optInAccelerationLong >= 0e0 && optInAccelerationLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxLong == REAL_DEFAULT ) {
         optInAccelerationMaxLong = 2e-1;
      } else if( !(optInAccelerationMaxLong >= 0e0 && optInAccelerationMaxLong <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationInitShort == REAL_DEFAULT ) {
         optInAccelerationInitShort = 2e-2;
      } else if( !(optInAccelerationInitShort >= 0e0 && optInAccelerationInitShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationShort == REAL_DEFAULT ) {
         optInAccelerationShort = 2e-2;
      } else if( !(optInAccelerationShort >= 0e0 && optInAccelerationShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInAccelerationMaxShort == REAL_DEFAULT ) {
         optInAccelerationMaxShort = 2e-1;
      } else if( !(optInAccelerationMaxShort >= 0e0 && optInAccelerationMaxShort <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* > 0 indicates long. == 0 indicates short */
      /* This function is the same as TA_SAR, except that the caller has
       * greater control on the SAR dynamic and initial state.
       *
       * In additon, the TA_SAREXT returns negative values when the position
       * is short. This allow to distinguish when the SAR do actually reverse.
       */
      /* Implementation of the SAR has been a little bit open to interpretation
       * since Wilder (the original author) did not define a precise algorithm
       * on how to bootstrap the algorithm. Take any existing software application
       * and you will see slight variation on how the algorithm was adapted.
       *
       * What is the initial trade direction? Long or short?
       * ===================================================
       * The interpretation of what should be the initial SAR values is
       * open to interpretation, particularly since the caller to the function
       * does not specify the initial direction of the trade.
       *
       * In TA-Lib, the following default logic is used:
       *  - Calculate +DM and -DM between the first and
       *    second bar. The highest directional indication will
       *    indicate the assumed direction of the trade for the second
       *    price bar.
       *  - In the case of a tie between +DM and -DM,
       *    the direction is LONG by default.
       *
       * What is the initial "extreme point" and thus SAR?
       * =================================================
       * The following shows how different people took different approach:
       *  - Metastock use the first price bar high/low depending of
       *    the direction. No SAR is calculated for the first price
       *    bar.
       *  - Tradestation use the closing price of the second bar. No
       *    SAR are calculated for the first price bar.
       *  - Wilder (the original author) use the SIP from the
       *    previous trade (cannot be implement here since the
       *    direction and length of the previous trade is unknonw).
       *  - The Magazine TASC seems to follow Wilder approach which
       *    is not practical here.
       *
       * TA-Lib "consume" the first price bar and use its high/low as the
       * initial SAR of the second price bar. I found that approach to be
       * the closest to Wilders idea of having the first entry day use
       * the previous extreme point, except that here the extreme point is
       * derived solely from the first price bar. I found the same approach
       * to be used by Metastock.
       *
       *
       * Can I force the initial SAR?
       * ============================
       * Yes. Using the optInStartValue_0 parameter:
       *  optInStartValue_0 >  0 : SAR is long at optInStartValue_0.
       *  optInStartValue_0 <  0 : SAR is short at fabs(optInStartValue_0).
       *
       * And when optInStartValue_0 == 0, the logic is the same as for TA_SAR
       * (See previous two sections).
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       *
       * Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Check if the acceleration factors are being defined by the user.
       * Make sure the acceleration and maximum are coherent.
       * If not, correct the acceleration.
       * Default afLong = 0.02
       * Default afShort = 0.02
       */
      afLong = optInAccelerationInitLong;
      afShort = optInAccelerationInitShort;
      if( afLong > optInAccelerationMaxLong ) {
         optInAccelerationInitLong = optInAccelerationMaxLong;
         afLong = optInAccelerationInitLong;
      }
      if( optInAccelerationLong > optInAccelerationMaxLong ) {
         optInAccelerationLong = optInAccelerationMaxLong;
      }
      if( afShort > optInAccelerationMaxShort ) {
         optInAccelerationInitShort = optInAccelerationMaxShort;
         afShort = optInAccelerationInitShort;
      }
      if( optInAccelerationShort > optInAccelerationMaxShort ) {
         optInAccelerationShort = optInAccelerationMaxShort;
      }
      /* Initialise SAR calculations */
      if( optInStartValue == 0 ) {
         /* Default action */
         /* Identify if the initial direction is long or short.
          * (ep is just used as a temp buffer here, the name
          *  of the parameter is not significant).
          */
         OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
         tempInt.value = _xr0.begIdx();
         tempInt.value = _xr0.count();
         retCode = RetCode.Success;
         if( ep_temp[0] > 0 ) {
            isLong = 0;
         } else {
            isLong = 1;
         }
      } else if( optInStartValue > 0 ) {
         /* Start Long */
         isLong = 1;
      } else {
         /* optInStartValue_0 < 0 => Start Short */
         isLong = 0;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      /* Write the first SAR. */
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( optInStartValue == 0 ) {
         /* Default action */
         if( isLong == 1 ) {
            ep = inHigh[todayIdx];
            sar = newLow;
         } else {
            ep = inLow[todayIdx];
            sar = newHigh;
         }
      } else if( optInStartValue > 0 ) {
         /* Start Long at specified value. */
         ep = inHigh[todayIdx];
         sar = optInStartValue;
      } else {
         /* if optInStartValue < 0 => Start Short at specified value. */
         ep = inLow[todayIdx];
         sar = Math.abs(optInStartValue);
      }
      /* Cheat on the newLow and newHigh for the
       * first iteration.
       */
      newLow = inLow[todayIdx];
      newHigh = inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = inLow[todayIdx];
         newHigh = inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            /* Switch to short if the low penetrates the SAR value. */
            if( newLow <= sar ) {
               /* Switch and Overide the SAR with the ep */
               isLong = 0;
               sar = ep;
               /* Make sure the overide SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               /* Output the overide SAR */
               if( optInOffsetOnReverse != 0.0 ) {
                  sar += sar * optInOffsetOnReverse;
               }
               outReal[outIdx++ * outStride] = 0 - sar;
               /* Adjust afShort and ep */
               afShort = optInAccelerationInitShort;
               ep = newLow;
               /* Calculate the new SAR */
               sar = Math.fma(afShort, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               /* No switch */
               /* Output the SAR (was calculated in the previous iteration) */
               outReal[outIdx++ * outStride] = sar;
               /* Adjust afLong and ep. */
               if( newHigh > ep ) {
                  ep = newHigh;
                  afLong += optInAccelerationLong;
                  if( afLong > optInAccelerationMaxLong ) {
                     afLong = optInAccelerationMaxLong;
                  }
               }
               /* Calculate the new SAR */
               sar = Math.fma(afLong, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         /* Switch to long if the high penetrates the SAR value. */
         } else if( newHigh >= sar ) {
            /* Switch and Overide the SAR with the ep */
            isLong = 1;
            sar = ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            /* Output the overide SAR */
            if( optInOffsetOnReverse != 0.0 ) {
               sar -= sar * optInOffsetOnReverse;
            }
            outReal[outIdx++ * outStride] = sar;
            /* Adjust afLong and ep */
            afLong = optInAccelerationInitLong;
            ep = newHigh;
            /* Calculate the new SAR */
            sar = Math.fma(afLong, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            outReal[outIdx++ * outStride] = 0 - sar;
            /* Adjust afShort and ep. */
            if( newLow < ep ) {
               ep = newLow;
               afShort += optInAccelerationShort;
               if( afShort > optInAccelerationMaxShort ) {
                  afShort = optInAccelerationMaxShort;
               }
            }
            /* Calculate the new SAR */
            sar = Math.fma(afShort, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInStartValue = optInStartValue;
      sp.optInOffsetOnReverse = optInOffsetOnReverse;
      sp.optInAccelerationInitLong = optInAccelerationInitLong;
      sp.optInAccelerationLong = optInAccelerationLong;
      sp.optInAccelerationMaxLong = optInAccelerationMaxLong;
      sp.optInAccelerationInitShort = optInAccelerationInitShort;
      sp.optInAccelerationShort = optInAccelerationShort;
      sp.optInAccelerationMaxShort = optInAccelerationMaxShort;
      sp.isLong = isLong;
      sp.newHigh = newHigh;
      sp.newLow = newLow;
      sp.afLong = afLong;
      sp.afShort = afShort;
      sp.ep = ep;
      sp.sar = sar;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* SAREXT_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SAREXT_Stream SAREXT_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SAREXT_Stream sp = new SAREXT_Stream(this);
      RetCode retCode = SAREXT_OpenImpl(sp, inHigh, inLow, startIdx, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SAREXT openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SAREXT openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SAREXT openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind SAREXT_Open (composition seam). */
   SAREXT_Stream SAREXT_OpenInternal( double inHigh[], double inLow[], int startIdx, double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort )
   {
      SAREXT_Stream sp = new SAREXT_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = SAREXT_OpenImpl(sp, inHigh, inLow, startIdx, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SAREXT open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SAREXT open: internal error", retCode);
      }
      throw new TaLibArgumentException("SAREXT open: " + retCode, retCode);
   }
   /**
    * Open a live SAREXT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SAREXT} at that bar.
    * <p>The history must hold at least {@code SAREXT_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SAREXT_Stream SAREXT_Open( double inHigh[], double inLow[], double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort )
   {
      requireArgument("SAREXT open", "inHigh", inHigh);
      requireHistory("SAREXT open", inHigh.length);
      requireArgument("SAREXT open", "inLow", inLow);
      requireHistoryLength("SAREXT open", "inLow", inLow.length, inHigh.length);
      return SAREXT_OpenInternal(inHigh, inLow, 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort);
   }
   /**
    * {@link Core#SAREXT_Open} that also fills the output array(s) bit-identically
    * to {@link Core#SAREXT} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SAREXT_Stream#outRange()}.
    */
   public SAREXT_Stream SAREXT_OpenAndFill( double inHigh[], double inLow[], double optInStartValue, double optInOffsetOnReverse, double optInAccelerationInitLong, double optInAccelerationLong, double optInAccelerationMaxLong, double optInAccelerationInitShort, double optInAccelerationShort, double optInAccelerationMaxShort, double outReal[] )
   {
      requireArgument("SAREXT openAndFill", "inHigh", inHigh);
      requireHistory("SAREXT openAndFill", inHigh.length);
      requireArgument("SAREXT openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("SAREXT openAndFill", inHigh.length, SAREXT_Lookback(optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort));
      requireHistoryLength("SAREXT openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("SAREXT openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("SAREXT openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return SAREXT_OpenAndFillInternal(inHigh, inLow, 0, optInStartValue, optInOffsetOnReverse, optInAccelerationInitLong, optInAccelerationLong, optInAccelerationMaxLong, optInAccelerationInitShort, optInAccelerationShort, optInAccelerationMaxShort, outBegIdx, outNBElement, outReal);
   }
