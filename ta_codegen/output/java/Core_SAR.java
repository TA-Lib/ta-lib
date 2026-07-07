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
 *  122104 MF,CF  Fix#1089506 for out-of-bound access to ep_temp.
 */

   public int sarLookback( double optInAcceleration, double optInMaximum )
   {
      if( optInAcceleration == -4e37 ) {
         optInAcceleration = 2e-2;
      } else if( optInAcceleration < 0e0 || optInAcceleration > 1.7976931348623157e308 ) {
         return -1;
      }
      if( optInMaximum == -4e37 ) {
         optInMaximum = 2e-1;
      } else if( optInMaximum < 0e0 || optInMaximum > 1.7976931348623157e308 ) {
         return -1;
      }
      /* SAR always sacrify one price bar to establish the
       * initial extreme price.
       */
      return 1 ;

   }
   public RetCode sar( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double optInAcceleration,
                       double optInMaximum,
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
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInAcceleration == -4e37 ) {
         optInAcceleration = 2e-2;
      } else if( optInAcceleration < 0e0 || optInAcceleration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      if( optInMaximum == -4e37 ) {
         optInMaximum = 2e-1;
      } else if( optInMaximum < 0e0 || optInMaximum > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      /* > 0 indicates long. == 0 indicates short */
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
       * In TA-Lib, the following logic is used:
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
      /* Make sure the acceleration and maximum are coherent.
       * If not, correct the acceleration.
       */
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      /* Identify if the initial direction is long or short.
       * (ep is just used as a temp buffer here, the name
       *  of the parameter is not significant).
       */
      retCode = minusDMUnguarded(startIdx, startIdx, inHigh, inLow, 1, tempInt, tempInt, ep_temp);
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      /* Write the first SAR. */
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = inLow[todayIdx];
         sar = newHigh;
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
               outReal[outIdx++] = sar;
               /* Adjust af and ep */
               af = optInAcceleration;
               ep = newLow;
               /* Calculate the new SAR */
               sar = sar + af * (ep - sar);
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
               /* Adjust af and ep. */
               if( newHigh > ep ) {
                  ep = newHigh;
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               /* Calculate the new SAR */
               sar = sar + af * (ep - sar);
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
            outReal[outIdx++] = sar;
            /* Adjust af and ep */
            af = optInAcceleration;
            ep = newHigh;
            /* Calculate the new SAR */
            sar = sar + af * (ep - sar);
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
            outReal[outIdx++] = sar;
            /* Adjust af and ep. */
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            /* Calculate the new SAR */
            sar = sar + af * (ep - sar);
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
   public RetCode sarUnguarded( int startIdx,
                                int endIdx,
                                double inHigh[],
                                double inLow[],
                                double optInAcceleration,
                                double optInMaximum,
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
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      retCode = minusDMUnguarded(startIdx, startIdx, inHigh, inLow, 1, tempInt, tempInt, ep_temp);
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = inLow[todayIdx];
         sar = newHigh;
      }
      newLow = inLow[todayIdx];
      newHigh = inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = inLow[todayIdx];
         newHigh = inHigh[todayIdx];
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
               outReal[outIdx++] = sar;
               af = optInAcceleration;
               ep = newLow;
               sar = sar + af * (ep - sar);
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
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               sar = sar + af * (ep - sar);
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
            outReal[outIdx++] = sar;
            af = optInAcceleration;
            ep = newHigh;
            sar = sar + af * (ep - sar);
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            outReal[outIdx++] = sar;
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            sar = sar + af * (ep - sar);
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
   public RetCode sar( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       double optInAcceleration,
                       double optInMaximum,
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
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInAcceleration == -4e37 ) {
         optInAcceleration = 2e-2;
      } else if( optInAcceleration < 0e0 || optInAcceleration > 1.7976931348623157e308 ) {
         return RetCode.BadParam;
      }
      if( optInMaximum == -4e37 ) {
         optInMaximum = 2e-1;
      } else if( optInMaximum < 0e0 || optInMaximum > 1.7976931348623157e308 ) {
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
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      retCode = minusDMUnguarded(startIdx, startIdx, inHigh, inLow, 1, tempInt, tempInt, ep_temp);
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      todayIdx = startIdx;
      newHigh = (double)inHigh[todayIdx - 1];
      newLow = (double)inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = (double)inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = (double)inLow[todayIdx];
         sar = newHigh;
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
               outReal[outIdx++] = sar;
               af = optInAcceleration;
               ep = newLow;
               sar = sar + af * (ep - sar);
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
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               sar = sar + af * (ep - sar);
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
            outReal[outIdx++] = sar;
            af = optInAcceleration;
            ep = newHigh;
            sar = sar + af * (ep - sar);
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            outReal[outIdx++] = sar;
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            sar = sar + af * (ep - sar);
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
   public RetCode sarUnguarded( int startIdx,
                                int endIdx,
                                float inHigh[],
                                float inLow[],
                                double optInAcceleration,
                                double optInMaximum,
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
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      retCode = minusDMUnguarded(startIdx, startIdx, inHigh, inLow, 1, tempInt, tempInt, ep_temp);
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      todayIdx = startIdx;
      newHigh = (double)inHigh[todayIdx - 1];
      newLow = (double)inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = (double)inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = (double)inLow[todayIdx];
         sar = newHigh;
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
               outReal[outIdx++] = sar;
               af = optInAcceleration;
               ep = newLow;
               sar = sar + af * (ep - sar);
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
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               sar = sar + af * (ep - sar);
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
            outReal[outIdx++] = sar;
            af = optInAcceleration;
            ep = newHigh;
            sar = sar + af * (ep - sar);
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            outReal[outIdx++] = sar;
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            sar = sar + af * (ep - sar);
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
