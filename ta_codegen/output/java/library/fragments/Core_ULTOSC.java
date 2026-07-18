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
/**** Streaming API *****/

   /**
    * A live ULTOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ultOsc} over the same series.
    * Open with {@link Core#ultOscOpen}; there is no close — the handle is
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
   public static final class UltOscStream {
      final Core core;
      int optInTimePeriod1;
      int optInTimePeriod2;
      int optInTimePeriod3;
      double a1Total;
      double a2Total;
      double a3Total;
      double b1Total;
      double b2Total;
      double b3Total;
      double output;
      double lag1_inClose;
      int ringPos_trailingIdx1;
      int ringCap_trailingIdx1;
      int ringLag_trailingIdx1;
      double[] ring_trailingIdx1_inHigh;
      double[] ring_trailingIdx1_inLow;
      double[] ring_trailingIdx1_inClose;
      int ringPos_trailingIdx2;
      int ringCap_trailingIdx2;
      int ringLag_trailingIdx2;
      double[] ring_trailingIdx2_inHigh;
      double[] ring_trailingIdx2_inLow;
      double[] ring_trailingIdx2_inClose;
      int ringPos_trailingIdx3;
      int ringCap_trailingIdx3;
      int ringLag_trailingIdx3;
      double[] ring_trailingIdx3_inHigh;
      double[] ring_trailingIdx3_inLow;
      double[] ring_trailingIdx3_inClose;
      double cur_outReal;

      UltOscStream( Core core ) { this.core = core; }

      UltOscStream( UltOscStream other ) {
         this.core = other.core;
         this.optInTimePeriod1 = other.optInTimePeriod1;
         this.optInTimePeriod2 = other.optInTimePeriod2;
         this.optInTimePeriod3 = other.optInTimePeriod3;
         this.a1Total = other.a1Total;
         this.a2Total = other.a2Total;
         this.a3Total = other.a3Total;
         this.b1Total = other.b1Total;
         this.b2Total = other.b2Total;
         this.b3Total = other.b3Total;
         this.output = other.output;
         this.lag1_inClose = other.lag1_inClose;
         this.ringPos_trailingIdx1 = other.ringPos_trailingIdx1;
         this.ringCap_trailingIdx1 = other.ringCap_trailingIdx1;
         this.ringLag_trailingIdx1 = other.ringLag_trailingIdx1;
         this.ring_trailingIdx1_inHigh = other.ring_trailingIdx1_inHigh.clone();
         this.ring_trailingIdx1_inLow = other.ring_trailingIdx1_inLow.clone();
         this.ring_trailingIdx1_inClose = other.ring_trailingIdx1_inClose.clone();
         this.ringPos_trailingIdx2 = other.ringPos_trailingIdx2;
         this.ringCap_trailingIdx2 = other.ringCap_trailingIdx2;
         this.ringLag_trailingIdx2 = other.ringLag_trailingIdx2;
         this.ring_trailingIdx2_inHigh = other.ring_trailingIdx2_inHigh.clone();
         this.ring_trailingIdx2_inLow = other.ring_trailingIdx2_inLow.clone();
         this.ring_trailingIdx2_inClose = other.ring_trailingIdx2_inClose.clone();
         this.ringPos_trailingIdx3 = other.ringPos_trailingIdx3;
         this.ringCap_trailingIdx3 = other.ringCap_trailingIdx3;
         this.ringLag_trailingIdx3 = other.ringLag_trailingIdx3;
         this.ring_trailingIdx3_inHigh = other.ring_trailingIdx3_inHigh.clone();
         this.ring_trailingIdx3_inLow = other.ring_trailingIdx3_inLow.clone();
         this.ring_trailingIdx3_inClose = other.ring_trailingIdx3_inClose.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.ultOscStreamStep(this, inHigh, inLow, inClose);
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
         UltOscStream scratch = new UltOscStream(this);
         core.ultOscStreamStep(scratch, inHigh, inLow, inClose);
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
      public UltOscStream copy() {
         return new UltOscStream(this);
      }
   }
   void ultOscStreamStep( UltOscStream sp, double inHigh, double inLow, double inClose )
   {
      double trueLow = 0.0;
      double trueRange = 0.0;
      double closeMinusTrueLow = 0.0;
      double tempDouble = 0.0;
      double tempHT = 0.0;
      double tempLT = 0.0;
      double tempCY = 0.0;
      sp.ring_trailingIdx1_inHigh[sp.ringPos_trailingIdx1] = inHigh;
      sp.ring_trailingIdx1_inLow[sp.ringPos_trailingIdx1] = inLow;
      sp.ring_trailingIdx1_inClose[sp.ringPos_trailingIdx1] = inClose;
      sp.ring_trailingIdx2_inHigh[sp.ringPos_trailingIdx2] = inHigh;
      sp.ring_trailingIdx2_inLow[sp.ringPos_trailingIdx2] = inLow;
      sp.ring_trailingIdx2_inClose[sp.ringPos_trailingIdx2] = inClose;
      sp.ring_trailingIdx3_inHigh[sp.ringPos_trailingIdx3] = inHigh;
      sp.ring_trailingIdx3_inLow[sp.ringPos_trailingIdx3] = inLow;
      sp.ring_trailingIdx3_inClose[sp.ringPos_trailingIdx3] = inClose;
      /* Add on today's terms */
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      trueLow = Math.min(tempLT, tempCY);
      closeMinusTrueLow = inClose - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.a1Total += closeMinusTrueLow;
      sp.a2Total += closeMinusTrueLow;
      sp.a3Total += closeMinusTrueLow;
      sp.b1Total += trueRange;
      sp.b2Total += trueRange;
      sp.b3Total += trueRange;
      /* Calculate the oscillator value for today */
      sp.output = 0.0;
      if( !((-0.00000000000001 < sp.b1Total) && (sp.b1Total < 0.00000000000001)) ) {
         sp.output += 4.0 * (sp.a1Total / sp.b1Total);
      }
      if( !((-0.00000000000001 < sp.b2Total) && (sp.b2Total < 0.00000000000001)) ) {
         sp.output += 2.0 * (sp.a2Total / sp.b2Total);
      }
      if( !((-0.00000000000001 < sp.b3Total) && (sp.b3Total < 0.00000000000001)) ) {
         sp.output += sp.a3Total / sp.b3Total;
      }
      /* Remove the trailing terms to prepare for next day */
      tempLT = sp.ring_trailingIdx1_inLow[(sp.ringPos_trailingIdx1 + sp.ringCap_trailingIdx1 - sp.ringLag_trailingIdx1) % sp.ringCap_trailingIdx1];
      tempHT = sp.ring_trailingIdx1_inHigh[(sp.ringPos_trailingIdx1 + sp.ringCap_trailingIdx1 - sp.ringLag_trailingIdx1) % sp.ringCap_trailingIdx1];
      tempCY = sp.ring_trailingIdx1_inClose[(sp.ringPos_trailingIdx1 + sp.ringCap_trailingIdx1 - sp.ringLag_trailingIdx1 - 1) % sp.ringCap_trailingIdx1];
      trueLow = Math.min(tempLT, tempCY);
      closeMinusTrueLow = sp.ring_trailingIdx1_inClose[(sp.ringPos_trailingIdx1 + sp.ringCap_trailingIdx1 - sp.ringLag_trailingIdx1) % sp.ringCap_trailingIdx1] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.a1Total -= closeMinusTrueLow;
      sp.b1Total -= trueRange;
      tempLT = sp.ring_trailingIdx2_inLow[(sp.ringPos_trailingIdx2 + sp.ringCap_trailingIdx2 - sp.ringLag_trailingIdx2) % sp.ringCap_trailingIdx2];
      tempHT = sp.ring_trailingIdx2_inHigh[(sp.ringPos_trailingIdx2 + sp.ringCap_trailingIdx2 - sp.ringLag_trailingIdx2) % sp.ringCap_trailingIdx2];
      tempCY = sp.ring_trailingIdx2_inClose[(sp.ringPos_trailingIdx2 + sp.ringCap_trailingIdx2 - sp.ringLag_trailingIdx2 - 1) % sp.ringCap_trailingIdx2];
      trueLow = Math.min(tempLT, tempCY);
      closeMinusTrueLow = sp.ring_trailingIdx2_inClose[(sp.ringPos_trailingIdx2 + sp.ringCap_trailingIdx2 - sp.ringLag_trailingIdx2) % sp.ringCap_trailingIdx2] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.a2Total -= closeMinusTrueLow;
      sp.b2Total -= trueRange;
      tempLT = sp.ring_trailingIdx3_inLow[(sp.ringPos_trailingIdx3 + sp.ringCap_trailingIdx3 - sp.ringLag_trailingIdx3) % sp.ringCap_trailingIdx3];
      tempHT = sp.ring_trailingIdx3_inHigh[(sp.ringPos_trailingIdx3 + sp.ringCap_trailingIdx3 - sp.ringLag_trailingIdx3) % sp.ringCap_trailingIdx3];
      tempCY = sp.ring_trailingIdx3_inClose[(sp.ringPos_trailingIdx3 + sp.ringCap_trailingIdx3 - sp.ringLag_trailingIdx3 - 1) % sp.ringCap_trailingIdx3];
      trueLow = Math.min(tempLT, tempCY);
      closeMinusTrueLow = sp.ring_trailingIdx3_inClose[(sp.ringPos_trailingIdx3 + sp.ringCap_trailingIdx3 - sp.ringLag_trailingIdx3) % sp.ringCap_trailingIdx3] - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.a3Total -= closeMinusTrueLow;
      sp.b3Total -= trueRange;
      /* Last operation is to write the output. Must
       * be done after the trailing index have all been
       * taken care of because the caller is allowed
       * to have the input array to be also the output
       * array.
       */
      sp.cur_outReal = 100.0 * (sp.output / 7.0);
      /* Increment indexes */
      sp.lag1_inClose = inClose;
      sp.ring_trailingIdx1_inHigh[sp.ringPos_trailingIdx1] = inHigh;
      sp.ring_trailingIdx1_inLow[sp.ringPos_trailingIdx1] = inLow;
      sp.ring_trailingIdx1_inClose[sp.ringPos_trailingIdx1] = inClose;
      sp.ringPos_trailingIdx1 = sp.ringPos_trailingIdx1 + 1;
      if( sp.ringPos_trailingIdx1 >= sp.ringCap_trailingIdx1 ) {
         sp.ringPos_trailingIdx1 = 0;
      }
      sp.ring_trailingIdx2_inHigh[sp.ringPos_trailingIdx2] = inHigh;
      sp.ring_trailingIdx2_inLow[sp.ringPos_trailingIdx2] = inLow;
      sp.ring_trailingIdx2_inClose[sp.ringPos_trailingIdx2] = inClose;
      sp.ringPos_trailingIdx2 = sp.ringPos_trailingIdx2 + 1;
      if( sp.ringPos_trailingIdx2 >= sp.ringCap_trailingIdx2 ) {
         sp.ringPos_trailingIdx2 = 0;
      }
      sp.ring_trailingIdx3_inHigh[sp.ringPos_trailingIdx3] = inHigh;
      sp.ring_trailingIdx3_inLow[sp.ringPos_trailingIdx3] = inLow;
      sp.ring_trailingIdx3_inClose[sp.ringPos_trailingIdx3] = inClose;
      sp.ringPos_trailingIdx3 = sp.ringPos_trailingIdx3 + 1;
      if( sp.ringPos_trailingIdx3 >= sp.ringCap_trailingIdx3 ) {
         sp.ringPos_trailingIdx3 = 0;
      }
   }
   private RetCode ultOscOpenBody( UltOscStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
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
         lastValue_outReal = 100.0 * (output / 7.0);
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
      /* Capture the live batch state into the handle. */
      int capLag_trailingIdx1 = today - trailingIdx1;
      int cap_trailingIdx1 = capLag_trailingIdx1 + 2;
      if( capLag_trailingIdx1 < 0 || cap_trailingIdx1 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx1 = (cap_trailingIdx1 > 0)? cap_trailingIdx1 : 1;
      double[] capRing_trailingIdx1_inHigh = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inHigh[fillJ % cap_trailingIdx1] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx1_inLow = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inLow[fillJ % cap_trailingIdx1] = inLow[fillJ];
      }
      double[] capRing_trailingIdx1_inClose = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inClose[fillJ % cap_trailingIdx1] = inClose[fillJ];
      }
      int capLag_trailingIdx2 = today - trailingIdx2;
      int cap_trailingIdx2 = capLag_trailingIdx2 + 2;
      if( capLag_trailingIdx2 < 0 || cap_trailingIdx2 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx2 = (cap_trailingIdx2 > 0)? cap_trailingIdx2 : 1;
      double[] capRing_trailingIdx2_inHigh = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inHigh[fillJ % cap_trailingIdx2] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx2_inLow = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inLow[fillJ % cap_trailingIdx2] = inLow[fillJ];
      }
      double[] capRing_trailingIdx2_inClose = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inClose[fillJ % cap_trailingIdx2] = inClose[fillJ];
      }
      int capLag_trailingIdx3 = today - trailingIdx3;
      int cap_trailingIdx3 = capLag_trailingIdx3 + 2;
      if( capLag_trailingIdx3 < 0 || cap_trailingIdx3 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx3 = (cap_trailingIdx3 > 0)? cap_trailingIdx3 : 1;
      double[] capRing_trailingIdx3_inHigh = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inHigh[fillJ % cap_trailingIdx3] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx3_inLow = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inLow[fillJ % cap_trailingIdx3] = inLow[fillJ];
      }
      double[] capRing_trailingIdx3_inClose = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inClose[fillJ % cap_trailingIdx3] = inClose[fillJ];
      }
      sp.optInTimePeriod1 = optInTimePeriod1;
      sp.optInTimePeriod2 = optInTimePeriod2;
      sp.optInTimePeriod3 = optInTimePeriod3;
      sp.a1Total = a1Total;
      sp.a2Total = a2Total;
      sp.a3Total = a3Total;
      sp.b1Total = b1Total;
      sp.b2Total = b2Total;
      sp.b3Total = b3Total;
      sp.output = output;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_trailingIdx1 = historyLen % cap_trailingIdx1;
      sp.ringCap_trailingIdx1 = cap_trailingIdx1;
      sp.ringLag_trailingIdx1 = capLag_trailingIdx1;
      sp.ring_trailingIdx1_inHigh = capRing_trailingIdx1_inHigh;
      sp.ring_trailingIdx1_inLow = capRing_trailingIdx1_inLow;
      sp.ring_trailingIdx1_inClose = capRing_trailingIdx1_inClose;
      sp.ringPos_trailingIdx2 = historyLen % cap_trailingIdx2;
      sp.ringCap_trailingIdx2 = cap_trailingIdx2;
      sp.ringLag_trailingIdx2 = capLag_trailingIdx2;
      sp.ring_trailingIdx2_inHigh = capRing_trailingIdx2_inHigh;
      sp.ring_trailingIdx2_inLow = capRing_trailingIdx2_inLow;
      sp.ring_trailingIdx2_inClose = capRing_trailingIdx2_inClose;
      sp.ringPos_trailingIdx3 = historyLen % cap_trailingIdx3;
      sp.ringCap_trailingIdx3 = cap_trailingIdx3;
      sp.ringLag_trailingIdx3 = capLag_trailingIdx3;
      sp.ring_trailingIdx3_inHigh = capRing_trailingIdx3_inHigh;
      sp.ring_trailingIdx3_inLow = capRing_trailingIdx3_inLow;
      sp.ring_trailingIdx3_inClose = capRing_trailingIdx3_inClose;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode ultOscOpenAndFillBody( UltOscStream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
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
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      int capLag_trailingIdx1 = today - trailingIdx1;
      int cap_trailingIdx1 = capLag_trailingIdx1 + 2;
      if( capLag_trailingIdx1 < 0 || cap_trailingIdx1 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx1 = (cap_trailingIdx1 > 0)? cap_trailingIdx1 : 1;
      double[] capRing_trailingIdx1_inHigh = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inHigh[fillJ % cap_trailingIdx1] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx1_inLow = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inLow[fillJ % cap_trailingIdx1] = inLow[fillJ];
      }
      double[] capRing_trailingIdx1_inClose = new double[allocN_trailingIdx1];
      for( int fillJ = historyLen - cap_trailingIdx1; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx1_inClose[fillJ % cap_trailingIdx1] = inClose[fillJ];
      }
      int capLag_trailingIdx2 = today - trailingIdx2;
      int cap_trailingIdx2 = capLag_trailingIdx2 + 2;
      if( capLag_trailingIdx2 < 0 || cap_trailingIdx2 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx2 = (cap_trailingIdx2 > 0)? cap_trailingIdx2 : 1;
      double[] capRing_trailingIdx2_inHigh = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inHigh[fillJ % cap_trailingIdx2] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx2_inLow = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inLow[fillJ % cap_trailingIdx2] = inLow[fillJ];
      }
      double[] capRing_trailingIdx2_inClose = new double[allocN_trailingIdx2];
      for( int fillJ = historyLen - cap_trailingIdx2; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx2_inClose[fillJ % cap_trailingIdx2] = inClose[fillJ];
      }
      int capLag_trailingIdx3 = today - trailingIdx3;
      int cap_trailingIdx3 = capLag_trailingIdx3 + 2;
      if( capLag_trailingIdx3 < 0 || cap_trailingIdx3 > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx3 = (cap_trailingIdx3 > 0)? cap_trailingIdx3 : 1;
      double[] capRing_trailingIdx3_inHigh = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inHigh[fillJ % cap_trailingIdx3] = inHigh[fillJ];
      }
      double[] capRing_trailingIdx3_inLow = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inLow[fillJ % cap_trailingIdx3] = inLow[fillJ];
      }
      double[] capRing_trailingIdx3_inClose = new double[allocN_trailingIdx3];
      for( int fillJ = historyLen - cap_trailingIdx3; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx3_inClose[fillJ % cap_trailingIdx3] = inClose[fillJ];
      }
      sp.optInTimePeriod1 = optInTimePeriod1;
      sp.optInTimePeriod2 = optInTimePeriod2;
      sp.optInTimePeriod3 = optInTimePeriod3;
      sp.a1Total = a1Total;
      sp.a2Total = a2Total;
      sp.a3Total = a3Total;
      sp.b1Total = b1Total;
      sp.b2Total = b2Total;
      sp.b3Total = b3Total;
      sp.output = output;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.ringPos_trailingIdx1 = historyLen % cap_trailingIdx1;
      sp.ringCap_trailingIdx1 = cap_trailingIdx1;
      sp.ringLag_trailingIdx1 = capLag_trailingIdx1;
      sp.ring_trailingIdx1_inHigh = capRing_trailingIdx1_inHigh;
      sp.ring_trailingIdx1_inLow = capRing_trailingIdx1_inLow;
      sp.ring_trailingIdx1_inClose = capRing_trailingIdx1_inClose;
      sp.ringPos_trailingIdx2 = historyLen % cap_trailingIdx2;
      sp.ringCap_trailingIdx2 = cap_trailingIdx2;
      sp.ringLag_trailingIdx2 = capLag_trailingIdx2;
      sp.ring_trailingIdx2_inHigh = capRing_trailingIdx2_inHigh;
      sp.ring_trailingIdx2_inLow = capRing_trailingIdx2_inLow;
      sp.ring_trailingIdx2_inClose = capRing_trailingIdx2_inClose;
      sp.ringPos_trailingIdx3 = historyLen % cap_trailingIdx3;
      sp.ringCap_trailingIdx3 = cap_trailingIdx3;
      sp.ringLag_trailingIdx3 = capLag_trailingIdx3;
      sp.ring_trailingIdx3_inHigh = capRing_trailingIdx3_inHigh;
      sp.ring_trailingIdx3_inLow = capRing_trailingIdx3_inLow;
      sp.ring_trailingIdx3_inClose = capRing_trailingIdx3_inClose;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind ultOscOpen (composition seam). */
   UltOscStream ultOscOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      UltOscStream sp = new UltOscStream(this);
      RetCode retCode = ultOscOpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ULTOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ULTOSC open: internal error");
      }
      throw new IllegalArgumentException("TA_ULTOSC open: " + retCode);
   }
   /**
    * Open a live ULTOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ultOsc} at that bar.
    * <p>The history must hold at least {@code ultOscLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public UltOscStream ultOscOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      return ultOscOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
   }
   /**
    * {@link Core#ultOscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ultOsc} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public UltOscStream ultOscOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      UltOscStream sp = new UltOscStream(this);
      RetCode retCode = ultOscOpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ULTOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ULTOSC openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ULTOSC openAndFill: " + retCode);
   }
