/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   public int varianceLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode variance( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            double optInNbDev,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double tempReal = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double meanValue2 = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Validate the calculation method type and
       * identify the minimum number of price bar needed
       * to calculate at least one output.
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
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial periods, except for the last value. */
      periodTotal1 = 0;
      periodTotal2 = 0;
      trailingIdx = startIdx - nbInitialElementNeeded;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i++];
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         tempReal = inReal[i++];
         /* Square and add all the deviation over
          * the same periods.
          */
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         /* Square and add all the deviation over
          * the same period.
          */
         meanValue1 = periodTotal1 / optInTimePeriod;
         meanValue2 = periodTotal2 / optInTimePeriod;
         tempReal = inReal[trailingIdx++];
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         outReal[outIdx++] = meanValue2 - meanValue1 * meanValue1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode varianceUnguarded( int startIdx,
                                     int endIdx,
                                     double inReal[],
                                     int optInTimePeriod,
                                     double optInNbDev,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double tempReal = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double meanValue2 = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal1 = 0;
      periodTotal2 = 0;
      trailingIdx = startIdx - nbInitialElementNeeded;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i++];
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
      }
      outIdx = 0;
      do {
         tempReal = inReal[i++];
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 / optInTimePeriod;
         meanValue2 = periodTotal2 / optInTimePeriod;
         tempReal = inReal[trailingIdx++];
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         outReal[outIdx++] = meanValue2 - meanValue1 * meanValue1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode variance( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            double optInNbDev,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double tempReal = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double meanValue2 = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
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
      periodTotal1 = 0;
      periodTotal2 = 0;
      trailingIdx = startIdx - nbInitialElementNeeded;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = (double)inReal[i++];
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
      }
      outIdx = 0;
      do {
         tempReal = (double)inReal[i++];
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 / optInTimePeriod;
         meanValue2 = periodTotal2 / optInTimePeriod;
         tempReal = (double)inReal[trailingIdx++];
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         outReal[outIdx++] = meanValue2 - meanValue1 * meanValue1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode varianceUnguarded( int startIdx,
                                     int endIdx,
                                     float inReal[],
                                     int optInTimePeriod,
                                     double optInNbDev,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double tempReal = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double meanValue2 = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal1 = 0;
      periodTotal2 = 0;
      trailingIdx = startIdx - nbInitialElementNeeded;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = (double)inReal[i++];
            periodTotal1 += tempReal;
            tempReal *= tempReal;
            periodTotal2 += tempReal;
         }
      }
      outIdx = 0;
      do {
         tempReal = (double)inReal[i++];
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 / optInTimePeriod;
         meanValue2 = periodTotal2 / optInTimePeriod;
         tempReal = (double)inReal[trailingIdx++];
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         outReal[outIdx++] = meanValue2 - meanValue1 * meanValue1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
