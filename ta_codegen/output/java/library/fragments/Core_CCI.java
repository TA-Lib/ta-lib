/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AF       Alexander Trufanov (github @trufanov-nok)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  031202 MF     Template creation.
 *  052603 MF     Port to managed C++. Change to use CIRCBUF macros.
 *  061704 MF     Lower limit for period to 2, and correct algorithm
 *                to avoid cummulative error when value are close to
 *                the floating point epsilon.
 *  070626 AF,CC  Guard the final division with TA_IS_ZERO instead of an exact
 *                "!= 0.0" check: identical prices over the period leave
 *                sub-epsilon residue that the exact check divided into a
 *                spurious value (issue #7 / SF bug #107). Now returns 0.0.
 */

   public int cciLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode cci( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
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
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
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
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      return RetCode.Success ;
   }
   public RetCode cciUnguarded( int startIdx,
                                int endIdx,
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cci( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
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
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cciUnguarded( int startIdx,
                                int endIdx,
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
