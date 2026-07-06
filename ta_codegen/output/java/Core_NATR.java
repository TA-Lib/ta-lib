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
 *  060306 MF     Initial Version
 *  070526 MF,CC  Fix #98: partial-range calls normalized with a close
 *                from the wrong bar (TR-buffer-relative index).
 */

   public int natrLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* The ATR lookback is the sum of:
       *    1 + (optInTimePeriod - 1)
       *
       * Where 1 is for the True Range, and
       * (optInTimePeriod-1) is for the simple
       * moving average.
       */
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.Natr.ordinal()] ;

   }
   public RetCode natr( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      RetCode retCode;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      double[] prevATR = new double[1];
      double tempValue = 0;
      double[] tempBuffer;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This function is very similar as ATR, except
       * it is being normalized as follow:
       *
       *    NATR = (ATR(period) / Close) * 100
       *
       *
       * Normalization make the ATR function more relevant
       * in the folllowing scenario:
       *    - Long term analysis where the price changes drastically.
       *    - Cross-market or cross-security ATR comparison.
       *
       * More Info:
       *      Technical Analysis of Stock & Commodities (TASC)
       *      May 2006 by John Forman
       */
      /* Average True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * These value are averaged for the specified period using
       * Wilder method. This method have an unstable period comparable
       * to an Exponential Moving Average (EMA).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = natrLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Trap the case where no smoothing is needed. */
      if( optInTimePeriod <= 1 ) {
         /* No smoothing needed. Just do a TRANGE. */
         return trueRangeUnguarded(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal) ;
      }
      /* Allocate an intermediate buffer for TRANGE. */
      tempBuffer = new double[(int)((lookbackTotal + (endIdx - startIdx) + 1) * 1)];
      /* Do TRANGE in the intermediate buffer. */
      retCode = trueRangeUnguarded(startIdx - lookbackTotal + 1, endIdx, inHigh, inLow, inClose, outBegIdx1, outNbElement1, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* First value of the ATR is a simple Average of
       * the TRANGE output for the specified period.
       */
      retCode = smaUnguarded(optInTimePeriod - 1, optInTimePeriod - 1, tempBuffer, optInTimePeriod, outBegIdx1, outNbElement1, prevATR);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Subsequent value are smoothed using the
       * previous ATR value (Wilder's approach).
       *  1) Multiply the previous ATR by 'period-1'.
       *  2) Add today TR value.
       *  3) Divide by 'period'.
       */
      today = optInTimePeriod;
      outIdx = this.unstablePeriod[FuncUnstId.Natr.ordinal()];
      /* Skip the unstable period. */
      while( outIdx != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         outIdx -= 1;
      }
      /* Now start to write the final ATR in the caller
       * provided outReal.
       */
      outIdx = 1;
      tempValue = inClose[startIdx - lookbackTotal + today];
      if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
         outReal[0] = prevATR[0] / tempValue * 100.0;
      } else {
         outReal[0] = 0.0;
      }
      /* Now do the number of requested ATR. */
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         tempValue = inClose[startIdx - lookbackTotal + today];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[outIdx] = prevATR[0] / tempValue * 100.0;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return retCode ;
   }
   public RetCode natrUnguarded( int startIdx,
                                 int endIdx,
                                 double inHigh[],
                                 double inLow[],
                                 double inClose[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      RetCode retCode;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      double[] prevATR = new double[1];
      double tempValue = 0;
      double[] tempBuffer;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = natrLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod <= 1 ) {
         return trueRangeUnguarded(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal) ;
      }
      tempBuffer = new double[(int)((lookbackTotal + (endIdx - startIdx) + 1) * 1)];
      retCode = trueRangeUnguarded(startIdx - lookbackTotal + 1, endIdx, inHigh, inLow, inClose, outBegIdx1, outNbElement1, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = smaUnguarded(optInTimePeriod - 1, optInTimePeriod - 1, tempBuffer, optInTimePeriod, outBegIdx1, outNbElement1, prevATR);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      today = optInTimePeriod;
      outIdx = this.unstablePeriod[FuncUnstId.Natr.ordinal()];
      while( outIdx != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         outIdx -= 1;
      }
      outIdx = 1;
      tempValue = inClose[startIdx - lookbackTotal + today];
      if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
         outReal[0] = prevATR[0] / tempValue * 100.0;
      } else {
         outReal[0] = 0.0;
      }
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         tempValue = inClose[startIdx - lookbackTotal + today];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[outIdx] = prevATR[0] / tempValue * 100.0;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return retCode ;
   }
   public RetCode natr( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      RetCode retCode;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      double[] prevATR = new double[1];
      double tempValue = 0;
      double[] tempBuffer;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = natrLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod <= 1 ) {
         return trueRangeUnguarded(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal) ;
      }
      tempBuffer = new double[(int)((lookbackTotal + (endIdx - startIdx) + 1) * 1)];
      retCode = trueRangeUnguarded(startIdx - lookbackTotal + 1, endIdx, inHigh, inLow, inClose, outBegIdx1, outNbElement1, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = smaUnguarded(optInTimePeriod - 1, optInTimePeriod - 1, tempBuffer, optInTimePeriod, outBegIdx1, outNbElement1, prevATR);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      today = optInTimePeriod;
      outIdx = this.unstablePeriod[FuncUnstId.Natr.ordinal()];
      while( outIdx != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         outIdx -= 1;
      }
      outIdx = 1;
      tempValue = inClose[startIdx - lookbackTotal + today];
      if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
         outReal[0] = prevATR[0] / tempValue * 100.0;
      } else {
         outReal[0] = 0.0;
      }
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         tempValue = inClose[startIdx - lookbackTotal + today];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[outIdx] = prevATR[0] / tempValue * 100.0;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return retCode ;
   }
   public RetCode natrUnguarded( int startIdx,
                                 int endIdx,
                                 float inHigh[],
                                 float inLow[],
                                 float inClose[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      RetCode retCode;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      double[] prevATR = new double[1];
      double tempValue = 0;
      double[] tempBuffer;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = natrLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod <= 1 ) {
         return trueRangeUnguarded(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal) ;
      }
      tempBuffer = new double[(int)((lookbackTotal + (endIdx - startIdx) + 1) * 1)];
      retCode = trueRangeUnguarded(startIdx - lookbackTotal + 1, endIdx, inHigh, inLow, inClose, outBegIdx1, outNbElement1, tempBuffer);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      retCode = smaUnguarded(optInTimePeriod - 1, optInTimePeriod - 1, tempBuffer, optInTimePeriod, outBegIdx1, outNbElement1, prevATR);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      today = optInTimePeriod;
      outIdx = this.unstablePeriod[FuncUnstId.Natr.ordinal()];
      while( outIdx != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         outIdx -= 1;
      }
      outIdx = 1;
      tempValue = inClose[startIdx - lookbackTotal + today];
      if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
         outReal[0] = prevATR[0] / tempValue * 100.0;
      } else {
         outReal[0] = 0.0;
      }
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         prevATR[0] *= optInTimePeriod - 1;
         prevATR[0] += tempBuffer[today++];
         prevATR[0] /= optInTimePeriod;
         tempValue = inClose[startIdx - lookbackTotal + today];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[outIdx] = prevATR[0] / tempValue * 100.0;
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return retCode ;
   }
