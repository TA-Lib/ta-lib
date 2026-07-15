/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Speed optimization: for periods above 20, cache the
 *                highest/lowest index instead of rescanning the window
 *                on every bar (same approach as MIN/MAX/WILLR). Smaller
 *                periods keep the simple scan, which auto-vectorizes
 *                and is faster there. Both paths produce identical
 *                output.
 */

   public int midPriceLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode midPrice( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
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
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
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
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * Two equivalent algorithms, picked by period. Their outputs are
       * bit-identical; only the scan strategy differs:
       *
       * - Small periods (<= 20): rescan the whole window on every bar.
       *   The two independent comparison chains auto-vectorize on modern
       *   compilers, which beats any per-bar bookkeeping while the window
       *   is short. The threshold sits near the measured crossover
       *   (~period 19-20 with gcc/clang -O3 on x86-64).
       *
       * - Larger periods: cache the highest high/lowest low with its
       *   index; a rescan of the window is needed only when the cached
       *   extremum drops out of the window (amortized O(1) per bar
       *   instead of O(period)).
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = inLow[trailingIdx];
            highest = inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = inHigh[today];
            tmpLow = inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPriceUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
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
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = inLow[trailingIdx];
            highest = inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = inHigh[today];
            tmpLow = inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPrice( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
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
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = (double)inLow[trailingIdx];
            highest = (double)inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = (double)inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = (double)inHigh[today];
            tmpLow = (double)inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = (double)inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = (double)inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = (double)inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = (double)inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPriceUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
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
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = (double)inLow[trailingIdx];
            highest = (double)inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = (double)inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = (double)inHigh[today];
            tmpLow = (double)inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = (double)inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = (double)inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = (double)inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = (double)inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
