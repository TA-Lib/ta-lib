/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  021807 MF     Initial Version
 */

   public int movingAverageVariablePeriodLookback( int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return -1;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return -1;
      }
      return movingAverageLookback(optInMaxPeriod, optInMAType) ;

   }
   public RetCode movingAverageVariablePeriod( int startIdx,
                                               int endIdx,
                                               double inReal[],
                                               double inPeriods[],
                                               int optInMinPeriod,
                                               int optInMaxPeriod,
                                               MAType optInMAType,
                                               MInteger outBegIdx,
                                               MInteger outNBElement,
                                               double outReal[] )
   {
      int i = 0;
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* An inverted period window (min above max) is an invalid parameter
       * combination: the per-bar clamp below would push a period above
       * optInMaxPeriod, exceeding the lookback and reading uninitialized
       * results. Reject it cleanly instead of returning garbage.
       */
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Calculate exact output size */
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         /* No output */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      /* Allocate intermediate local buffer. */
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      /* Copy caller array of period into local buffer.
       * At the same time, truncate to min/max.
       */
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      /* Process each element of the input.
       * For each possible period value, the MA is calculated
       * only once.
       * The outReal is then fill up for all element with
       * the same period.
       * A local flag (value 0) is set in localPeriodArray
       * to avoid doing a second time the same calculation.
       */
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            /* TODO: This portion of the function can be slightly speed
             *       optimized by making the function without unstable period
             *       start their calculation at 'startIdx+i' instead of startIdx.
             */
            /* Calculation of the MA required. */
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  /* Flag to avoid recalculation */
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      /* Done. Inform the caller of the success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriodUnguarded( int startIdx,
                                                        int endIdx,
                                                        double inReal[],
                                                        double inPeriods[],
                                                        int optInMinPeriod,
                                                        int optInMaxPeriod,
                                                        MAType optInMAType,
                                                        MInteger outBegIdx,
                                                        MInteger outNBElement,
                                                        double outReal[] )
   {
      int i = 0;
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriod( int startIdx,
                                               int endIdx,
                                               float inReal[],
                                               float inPeriods[],
                                               int optInMinPeriod,
                                               int optInMaxPeriod,
                                               MAType optInMAType,
                                               MInteger outBegIdx,
                                               MInteger outNBElement,
                                               double outReal[] )
   {
      int i = 0;
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriodUnguarded( int startIdx,
                                                        int endIdx,
                                                        float inReal[],
                                                        float inPeriods[],
                                                        int optInMinPeriod,
                                                        int optInMaxPeriod,
                                                        MAType optInMAType,
                                                        MInteger outBegIdx,
                                                        MInteger outNBElement,
                                                        double outReal[] )
   {
      int i = 0;
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
