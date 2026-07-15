/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  PP       Peter Pudaite
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101103 PP   Initial creation of code.
 *  112603 MF   Add independent control to the RSI period.
 *  020605 AA   Fix #1117656. NULL pointer assignement.
 */

   public int stochRsiLookback( int optInTimePeriod, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return -1;
      }
      int retValue;
      retValue = rsiLookback(optInTimePeriod) + stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   public RetCode stochRsi( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            int optInFastK_Period,
                            int optInFastD_Period,
                            MAType optInFastD_MAType,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outFastK[],
                            double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
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
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      /* Stochastic RSI
       *
       * Reference: "Stochastic RSI and Dynamic Momentum Index"
       *            by Tushar Chande and Stanley Kroll
       *            Stock&Commodities V.11:5 (189-199)
       *
       * The TA-Lib version offer flexibility beyond what is explain
       * in the Stock&Commodities article.
       *
       * To calculate the "Unsmoothed stochastic RSI" with symetry like
       * explain in the article, keep the optInTimePeriod and optInFastK_Period
       * equal. Example:
       *
       *    unsmoothed stoch RSI 14 : optInTimePeriod   = 14
       *                              optInFastK_Period = 14
       *                              optInFastD_Period = 'x'
       *
       * The outFastK is the unsmoothed RSI discuss in the article.
       *
       * You can set the optInFastD_Period to smooth the RSI. The smooth
       * version will be found in outFastD. The outFastK will still contain
       * the unsmoothed stoch RSI. If you do not care about the smoothing of
       * the StochRSI, just leave optInFastD_Period to 1 and ignore outFastD.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsiUnguarded( int startIdx,
                                     int endIdx,
                                     double inReal[],
                                     int optInTimePeriod,
                                     int optInFastK_Period,
                                     int optInFastD_Period,
                                     MAType optInFastD_MAType,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outFastK[],
                                     double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsi( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            int optInFastK_Period,
                            int optInFastD_Period,
                            MAType optInFastD_MAType,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outFastK[],
                            double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
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
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
   public RetCode stochRsiUnguarded( int startIdx,
                                     int endIdx,
                                     float inReal[],
                                     int optInTimePeriod,
                                     int optInFastK_Period,
                                     int optInFastD_Period,
                                     MAType optInFastD_MAType,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outFastK[],
                                     double outFastD[] )
   {
      double[] tempRSIBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSTOCHF = 0;
      int tempArraySize = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSTOCHF = stochFLookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      lookbackTotal = rsiLookback(optInTimePeriod) + lookbackSTOCHF;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      tempArraySize = endIdx - startIdx + 1 + lookbackSTOCHF;
      tempRSIBuffer = new double[(int)(tempArraySize * 1)];
      retCode = rsiUnguarded(startIdx - lookbackSTOCHF, endIdx, inReal, optInTimePeriod, outBegIdx1, outNbElement1, tempRSIBuffer);
      if( retCode != RetCode.Success || outNbElement1.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = stochFUnguarded(0, tempArraySize - 1, tempRSIBuffer, tempRSIBuffer, tempRSIBuffer, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx2, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      return RetCode.Success ;
   }
