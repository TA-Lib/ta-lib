/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  281206 DM   Initial Implementation
 *  010606 MF   Abstract local arrays. Detect divide by zero.
 */

   public int ultOscLookback( int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return -1;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return -1;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return -1;
      }
      int maxPeriod;
      /* Lookback for the Ultimate Oscillator is the lookback of the SMA with the longest
       * time period, plus 1 for the True Range.
       */
      maxPeriod = Math.max(Math.max(optInTimePeriod1, optInTimePeriod2), optInTimePeriod3);
      return smaLookback(maxPeriod) + 1 ;

   }
   public RetCode ultOsc( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInTimePeriod1,
                          int optInTimePeriod2,
                          int optInTimePeriod3,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingIdx1 = 0;
      int trailingIdx2 = 0;
      int trailingIdx3 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Ensure that the time periods are ordered from shortest to longest.
       * Sort.
       */
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      /* Adjust startIdx for lookback period. */
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Prime running totals used in moving averages */
      a1Total = 0;
      b1Total = 0;
      for( i = startIdx - optInTimePeriod1 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         b1Total += trueRange;
      }
      a2Total = 0;
      b2Total = 0;
      for( i = startIdx - optInTimePeriod2 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total += closeMinusTrueLow;
         b2Total += trueRange;
      }
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      /* Calculate oscillator */
      today = startIdx;
      outIdx = 0;
      trailingIdx1 = today - optInTimePeriod1 + 1;
      trailingIdx2 = today - optInTimePeriod2 + 1;
      trailingIdx3 = today - optInTimePeriod3 + 1;
      while( today <= endIdx ) {
         /* Add on today's terms */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         /* Calculate the oscillator value for today */
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         /* Remove the trailing terms to prepare for next day */
         tempLT = inLow[trailingIdx1];
         tempHT = inHigh[trailingIdx1];
         tempCY = inClose[trailingIdx1 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx1] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total -= closeMinusTrueLow;
         b1Total -= trueRange;
         tempLT = inLow[trailingIdx2];
         tempHT = inHigh[trailingIdx2];
         tempCY = inClose[trailingIdx2 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx2] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total -= closeMinusTrueLow;
         b2Total -= trueRange;
         tempLT = inLow[trailingIdx3];
         tempHT = inHigh[trailingIdx3];
         tempCY = inClose[trailingIdx3 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx3] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total -= closeMinusTrueLow;
         b3Total -= trueRange;
         /* Last operation is to write the output. Must
          * be done after the trailing index have all been
          * taken care of because the caller is allowed
          * to have the input array to be also the output
          * array.
          */
         outReal[outIdx] = 100.0 * (output / 7.0);
         /* Increment indexes */
         outIdx += 1;
         today += 1;
         trailingIdx1 += 1;
         trailingIdx2 += 1;
         trailingIdx3 += 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode ultOscUnguarded( int startIdx,
                                   int endIdx,
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   int optInTimePeriod1,
                                   int optInTimePeriod2,
                                   int optInTimePeriod3,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingIdx1 = 0;
      int trailingIdx2 = 0;
      int trailingIdx3 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      outBegIdx.value = 0;
      outNBElement.value = 0;
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      a1Total = 0;
      b1Total = 0;
      for( i = startIdx - optInTimePeriod1 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         b1Total += trueRange;
      }
      a2Total = 0;
      b2Total = 0;
      for( i = startIdx - optInTimePeriod2 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total += closeMinusTrueLow;
         b2Total += trueRange;
      }
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      today = startIdx;
      outIdx = 0;
      trailingIdx1 = today - optInTimePeriod1 + 1;
      trailingIdx2 = today - optInTimePeriod2 + 1;
      trailingIdx3 = today - optInTimePeriod3 + 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         tempLT = inLow[trailingIdx1];
         tempHT = inHigh[trailingIdx1];
         tempCY = inClose[trailingIdx1 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx1] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total -= closeMinusTrueLow;
         b1Total -= trueRange;
         tempLT = inLow[trailingIdx2];
         tempHT = inHigh[trailingIdx2];
         tempCY = inClose[trailingIdx2 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx2] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total -= closeMinusTrueLow;
         b2Total -= trueRange;
         tempLT = inLow[trailingIdx3];
         tempHT = inHigh[trailingIdx3];
         tempCY = inClose[trailingIdx3 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[trailingIdx3] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total -= closeMinusTrueLow;
         b3Total -= trueRange;
         outReal[outIdx] = 100.0 * (output / 7.0);
         outIdx += 1;
         today += 1;
         trailingIdx1 += 1;
         trailingIdx2 += 1;
         trailingIdx3 += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode ultOsc( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInTimePeriod1,
                          int optInTimePeriod2,
                          int optInTimePeriod3,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingIdx1 = 0;
      int trailingIdx2 = 0;
      int trailingIdx3 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      a1Total = 0;
      b1Total = 0;
      for( i = startIdx - optInTimePeriod1 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         b1Total += trueRange;
      }
      a2Total = 0;
      b2Total = 0;
      for( i = startIdx - optInTimePeriod2 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total += closeMinusTrueLow;
         b2Total += trueRange;
      }
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      today = startIdx;
      outIdx = 0;
      trailingIdx1 = today - optInTimePeriod1 + 1;
      trailingIdx2 = today - optInTimePeriod2 + 1;
      trailingIdx3 = today - optInTimePeriod3 + 1;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         tempLT = (double)inLow[trailingIdx1];
         tempHT = (double)inHigh[trailingIdx1];
         tempCY = (double)inClose[trailingIdx1 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx1] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total -= closeMinusTrueLow;
         b1Total -= trueRange;
         tempLT = (double)inLow[trailingIdx2];
         tempHT = (double)inHigh[trailingIdx2];
         tempCY = (double)inClose[trailingIdx2 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx2] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total -= closeMinusTrueLow;
         b2Total -= trueRange;
         tempLT = (double)inLow[trailingIdx3];
         tempHT = (double)inHigh[trailingIdx3];
         tempCY = (double)inClose[trailingIdx3 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx3] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total -= closeMinusTrueLow;
         b3Total -= trueRange;
         outReal[outIdx] = 100.0 * (output / 7.0);
         outIdx += 1;
         today += 1;
         trailingIdx1 += 1;
         trailingIdx2 += 1;
         trailingIdx3 += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode ultOscUnguarded( int startIdx,
                                   int endIdx,
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   int optInTimePeriod1,
                                   int optInTimePeriod2,
                                   int optInTimePeriod3,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingIdx1 = 0;
      int trailingIdx2 = 0;
      int trailingIdx3 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      outBegIdx.value = 0;
      outNBElement.value = 0;
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      a1Total = 0;
      b1Total = 0;
      for( i = startIdx - optInTimePeriod1 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         b1Total += trueRange;
      }
      a2Total = 0;
      b2Total = 0;
      for( i = startIdx - optInTimePeriod2 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total += closeMinusTrueLow;
         b2Total += trueRange;
      }
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      today = startIdx;
      outIdx = 0;
      trailingIdx1 = today - optInTimePeriod1 + 1;
      trailingIdx2 = today - optInTimePeriod2 + 1;
      trailingIdx3 = today - optInTimePeriod3 + 1;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         tempLT = (double)inLow[trailingIdx1];
         tempHT = (double)inHigh[trailingIdx1];
         tempCY = (double)inClose[trailingIdx1 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx1] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a1Total -= closeMinusTrueLow;
         b1Total -= trueRange;
         tempLT = (double)inLow[trailingIdx2];
         tempHT = (double)inHigh[trailingIdx2];
         tempCY = (double)inClose[trailingIdx2 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx2] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a2Total -= closeMinusTrueLow;
         b2Total -= trueRange;
         tempLT = (double)inLow[trailingIdx3];
         tempHT = (double)inHigh[trailingIdx3];
         tempCY = (double)inClose[trailingIdx3 - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[trailingIdx3] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         a3Total -= closeMinusTrueLow;
         b3Total -= trueRange;
         outReal[outIdx] = 100.0 * (output / 7.0);
         outIdx += 1;
         today += 1;
         trailingIdx1 += 1;
         trailingIdx2 += 1;
         trailingIdx3 += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
