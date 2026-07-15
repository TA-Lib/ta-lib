/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   public int willRLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode willR( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
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
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Initialize 'diff', just to avoid warning. */
      diff = 0.0;
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      diff = highest;
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode willRUnguarded( int startIdx,
                                  int endIdx,
                                  double inHigh[],
                                  double inLow[],
                                  double inClose[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      diff = 0.0;
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      diff = highest;
      while( today <= endIdx ) {
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode willR( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
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
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      diff = 0.0;
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      diff = highest;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - (double)inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode willRUnguarded( int startIdx,
                                  int endIdx,
                                  float inHigh[],
                                  float inLow[],
                                  float inClose[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      diff = 0.0;
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      diff = highest;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - (double)inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
