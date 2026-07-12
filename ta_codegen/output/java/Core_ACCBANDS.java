/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  RM       Robert Meier
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120307 RM     Initial Version
 *  120907 MF     Handling of a few limit cases
 */

   public int accbandsLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return smaLookback(optInTimePeriod) ;

   }
   public RetCode accbands( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            double inClose[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outRealUpperBand[],
                            double outRealMiddleBand[],
                            double outRealLowerBand[] )
   {
      RetCode retCode;
      double[] tempBuffer1;
      double[] tempBuffer2;
      MInteger outBegIdxDummy = new MInteger();
      MInteger outNbElementDummy = new MInteger();
      int i = 0;
      int j = 0;
      int outputSize = 0;
      int bufferSize = 0;
      int lookbackTotal = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = smaLookback(optInTimePeriod);
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
      /* Buffer will contains also the lookback required for SMA
       * to satisfy the caller requested startIdx/endIdx.
       */
      outputSize = endIdx - startIdx + 1;
      bufferSize = outputSize + lookbackTotal;
      tempBuffer1 = new double[(int)(bufferSize * 1)];
      tempBuffer2 = new double[(int)(bufferSize * 1)];
      /* Calculate the upper/lower band at the same time (no SMA yet).
       * Must start calculation back enough to cover the lookback
       * required later for the SMA.
       */
      for( j = 0, i = startIdx - lookbackTotal; i <= endIdx; i += 1, j += 1 ) {
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            tempBuffer1[j] = inHigh[i] * (1 + tempReal);
            tempBuffer2[j] = inLow[i] * (1 - tempReal);
         } else {
            tempBuffer1[j] = inHigh[i];
            tempBuffer2[j] = inLow[i];
         }
      }
      /* Calculate the middle band, which is a moving average of the close. */
      retCode = smaUnguarded(startIdx, endIdx, inClose, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealMiddleBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Now let's take the SMA for the upper band. */
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer1, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealUpperBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Now let's take the SMA for the lower band. */
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer2, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealLowerBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode accbandsUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     double inClose[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outRealUpperBand[],
                                     double outRealMiddleBand[],
                                     double outRealLowerBand[] )
   {
      RetCode retCode;
      double[] tempBuffer1;
      double[] tempBuffer2;
      MInteger outBegIdxDummy = new MInteger();
      MInteger outNbElementDummy = new MInteger();
      int i = 0;
      int j = 0;
      int outputSize = 0;
      int bufferSize = 0;
      int lookbackTotal = 0;
      double tempReal = 0;
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - startIdx + 1;
      bufferSize = outputSize + lookbackTotal;
      tempBuffer1 = new double[(int)(bufferSize * 1)];
      tempBuffer2 = new double[(int)(bufferSize * 1)];
      for( j = 0, i = startIdx - lookbackTotal; i <= endIdx; i += 1, j += 1 ) {
         tempReal = inHigh[i] + inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * (inHigh[i] - inLow[i]) / tempReal;
            tempBuffer1[j] = inHigh[i] * (1 + tempReal);
            tempBuffer2[j] = inLow[i] * (1 - tempReal);
         } else {
            tempBuffer1[j] = inHigh[i];
            tempBuffer2[j] = inLow[i];
         }
      }
      retCode = smaUnguarded(startIdx, endIdx, inClose, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealMiddleBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer1, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealUpperBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer2, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealLowerBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode accbands( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            float inClose[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outRealUpperBand[],
                            double outRealMiddleBand[],
                            double outRealLowerBand[] )
   {
      RetCode retCode;
      double[] tempBuffer1;
      double[] tempBuffer2;
      MInteger outBegIdxDummy = new MInteger();
      MInteger outNbElementDummy = new MInteger();
      int i = 0;
      int j = 0;
      int outputSize = 0;
      int bufferSize = 0;
      int lookbackTotal = 0;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - startIdx + 1;
      bufferSize = outputSize + lookbackTotal;
      tempBuffer1 = new double[(int)(bufferSize * 1)];
      tempBuffer2 = new double[(int)(bufferSize * 1)];
      for( j = 0, i = startIdx - lookbackTotal; i <= endIdx; i += 1, j += 1 ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            tempBuffer1[j] = (double)inHigh[i] * (1 + tempReal);
            tempBuffer2[j] = (double)inLow[i] * (1 - tempReal);
         } else {
            tempBuffer1[j] = (double)inHigh[i];
            tempBuffer2[j] = (double)inLow[i];
         }
      }
      retCode = smaUnguarded(startIdx, endIdx, inClose, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealMiddleBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer1, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealUpperBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer2, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealLowerBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode accbandsUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     float inClose[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outRealUpperBand[],
                                     double outRealMiddleBand[],
                                     double outRealLowerBand[] )
   {
      RetCode retCode;
      double[] tempBuffer1;
      double[] tempBuffer2;
      MInteger outBegIdxDummy = new MInteger();
      MInteger outNbElementDummy = new MInteger();
      int i = 0;
      int j = 0;
      int outputSize = 0;
      int bufferSize = 0;
      int lookbackTotal = 0;
      double tempReal = 0;
      lookbackTotal = smaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - startIdx + 1;
      bufferSize = outputSize + lookbackTotal;
      tempBuffer1 = new double[(int)(bufferSize * 1)];
      tempBuffer2 = new double[(int)(bufferSize * 1)];
      for( j = 0, i = startIdx - lookbackTotal; i <= endIdx; i += 1, j += 1 ) {
         tempReal = (double)inHigh[i] + (double)inLow[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            tempReal = 4 * ((double)inHigh[i] - (double)inLow[i]) / tempReal;
            tempBuffer1[j] = (double)inHigh[i] * (1 + tempReal);
            tempBuffer2[j] = (double)inLow[i] * (1 - tempReal);
         } else {
            tempBuffer1[j] = (double)inHigh[i];
            tempBuffer2[j] = (double)inLow[i];
         }
      }
      retCode = smaUnguarded(startIdx, endIdx, inClose, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealMiddleBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer1, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealUpperBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = smaUnguarded(0, bufferSize - 1, tempBuffer2, optInTimePeriod, outBegIdxDummy, outNbElementDummy, outRealLowerBand);
      if( retCode != RetCode.Success || (int)outNbElementDummy.value != outputSize ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
