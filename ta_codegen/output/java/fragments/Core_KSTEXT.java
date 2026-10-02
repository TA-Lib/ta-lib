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
 *  100126 MF,CC  First version (#491).
 */

   /**
    * Number of leading input bars {@link Core#kstext} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInROC1Period Rate-of-change period of leg 1 (weight 1) (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Rate-of-change period of leg 2 (weight 2) (default
    *        15; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC3Period Rate-of-change period of leg 3 (weight 3) (default
    *        20; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC4Period Rate-of-change period of leg 4 (weight 4) (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA1Period Period of the MA smoothing leg 1 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA2Period Period of the MA smoothing leg 2 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA3Period Period of the MA smoothing leg 3 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA4Period Period of the MA smoothing leg 4 (default 15; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROCMAType MA type smoothing the four legs (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int kstextLookback( int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType )
   {
      if( optInROCMAType == null ) {
         return -1;
      }
      if( optInSignalMAType == null ) {
         return -1;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 10;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return -1;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 15;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return -1;
      }
      if( optInROC3Period == Integer.MIN_VALUE ) {
         optInROC3Period = 20;
      } else if( optInROC3Period < 1 || optInROC3Period > 100000 ) {
         return -1;
      }
      if( optInROC4Period == Integer.MIN_VALUE ) {
         optInROC4Period = 30;
      } else if( optInROC4Period < 1 || optInROC4Period > 100000 ) {
         return -1;
      }
      if( optInMA1Period == Integer.MIN_VALUE ) {
         optInMA1Period = 10;
      } else if( optInMA1Period < 1 || optInMA1Period > 100000 ) {
         return -1;
      }
      if( optInMA2Period == Integer.MIN_VALUE ) {
         optInMA2Period = 10;
      } else if( optInMA2Period < 1 || optInMA2Period > 100000 ) {
         return -1;
      }
      if( optInMA3Period == Integer.MIN_VALUE ) {
         optInMA3Period = 10;
      } else if( optInMA3Period < 1 || optInMA3Period > 100000 ) {
         return -1;
      }
      if( optInMA4Period == Integer.MIN_VALUE ) {
         optInMA4Period = 15;
      } else if( optInMA4Period < 1 || optInMA4Period > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      if( optInROCMAType == MAType.DEFAULT ) {
         optInROCMAType = MAType.SMA;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      int legMax;
      int leg;
      legMax = optInROC1Period + maLookback(optInMA1Period, optInROCMAType);
      leg = optInROC2Period + maLookback(optInMA2Period, optInROCMAType);
      if( leg > legMax ) {
         legMax = leg;
      }
      leg = optInROC3Period + maLookback(optInMA3Period, optInROCMAType);
      if( leg > legMax ) {
         legMax = leg;
      }
      leg = optInROC4Period + maLookback(optInMA4Period, optInROCMAType);
      if( leg > legMax ) {
         legMax = leg;
      }
      return legMax + maLookback(optInSignalPeriod, optInSignalMAType) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#kstext}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInROC1Period Rate-of-change period of leg 1 (weight 1) (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Rate-of-change period of leg 2 (weight 2) (default
    *        15; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC3Period Rate-of-change period of leg 3 (weight 3) (default
    *        20; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC4Period Rate-of-change period of leg 4 (weight 4) (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA1Period Period of the MA smoothing leg 1 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA2Period Period of the MA smoothing leg 2 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA3Period Period of the MA smoothing leg 3 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA4Period Period of the MA smoothing leg 4 (default 15; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROCMAType MA type smoothing the four legs (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int kstextDisplayShift( int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType, int outputIdx )
   {
      if( kstextLookback( optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode kstextImpl( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInROC1Period,
                       int optInROC2Period,
                       int optInROC3Period,
                       int optInROC4Period,
                       int optInMA1Period,
                       int optInMA2Period,
                       int optInMA3Period,
                       int optInMA4Period,
                       int optInSignalPeriod,
                       MAType optInROCMAType,
                       MAType optInSignalMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outKST[],
                       double outKSTSignal[] )
   {
      double[] kstBuffer;
      double[] tempBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackMA = 0;
      int sigStart = 0;
      int tempInteger = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rocNb = new MInteger();
      MInteger kstNb = new MInteger();
      MInteger legNb = new MInteger();
      MInteger sigNb = new MInteger();
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 10;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 15;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC3Period == Integer.MIN_VALUE ) {
         optInROC3Period = 20;
      } else if( optInROC3Period < 1 || optInROC3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC4Period == Integer.MIN_VALUE ) {
         optInROC4Period = 30;
      } else if( optInROC4Period < 1 || optInROC4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA1Period == Integer.MIN_VALUE ) {
         optInMA1Period = 10;
      } else if( optInMA1Period < 1 || optInMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA2Period == Integer.MIN_VALUE ) {
         optInMA2Period = 10;
      } else if( optInMA2Period < 1 || optInMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA3Period == Integer.MIN_VALUE ) {
         optInMA3Period = 10;
      } else if( optInMA3Period < 1 || optInMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA4Period == Integer.MIN_VALUE ) {
         optInMA4Period = 15;
      } else if( optInMA4Period < 1 || optInMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROCMAType == MAType.DEFAULT ) {
         optInROCMAType = MAType.SMA;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( outKST == outKSTSignal ) {
         return RetCode.BAD_PARAM ;
      }
      /* With every type SMA this is bit-exact with kst(): each leg's average
       * starts on the first bar the signal consumes, and the line accumulates
       * its legs left to right. Changing either breaks the equality.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSignal = maLookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      sigStart = startIdx - lookbackSignal;
      /* Both buffers are sized by the longest leg average: a rate of change
       * starts its average's lookback before sigStart.
       */
      lookbackMA = maLookback(optInMA1Period, optInROCMAType);
      tempInteger = maLookback(optInMA2Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA3Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA4Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = endIdx - sigStart + 1 + lookbackMA;
      kstBuffer = new double[(int)(tempInteger * 1)];
      tempBuffer = new double[(int)(tempInteger * 1)];
      OutRange _xr0 = roc(sigStart - maLookback(optInMA1Period, optInROCMAType), endIdx, inReal, optInROC1Period, kstBuffer);
      tempBegIdx.value = _xr0.begIdx();
      rocNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = ma(0, rocNb.value - 1, kstBuffer, optInMA1Period, optInROCMAType, kstBuffer);
      tempBegIdx.value = _xr1.begIdx();
      kstNb.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = roc(sigStart - maLookback(optInMA2Period, optInROCMAType), endIdx, inReal, optInROC2Period, tempBuffer);
      tempBegIdx.value = _xr2.begIdx();
      rocNb.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = ma(0, rocNb.value - 1, tempBuffer, optInMA2Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr3.begIdx();
      legNb.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(2.0, tempBuffer[i], kstBuffer[i]);
      }
      OutRange _xr4 = roc(sigStart - maLookback(optInMA3Period, optInROCMAType), endIdx, inReal, optInROC3Period, tempBuffer);
      tempBegIdx.value = _xr4.begIdx();
      rocNb.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr5 = ma(0, rocNb.value - 1, tempBuffer, optInMA3Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr5.begIdx();
      legNb.value = _xr5.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(3.0, tempBuffer[i], kstBuffer[i]);
      }
      OutRange _xr6 = roc(sigStart - maLookback(optInMA4Period, optInROCMAType), endIdx, inReal, optInROC4Period, tempBuffer);
      tempBegIdx.value = _xr6.begIdx();
      rocNb.value = _xr6.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr7 = ma(0, rocNb.value - 1, tempBuffer, optInMA4Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr7.begIdx();
      legNb.value = _xr7.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(4.0, tempBuffer[i], kstBuffer[i]);
      }
      /* Every read of inReal is done: an output may alias it. */
      System.arraycopy(kstBuffer, lookbackSignal, outKST, 0, (endIdx - startIdx + 1) * 1);
      OutRange _xr8 = ma(0, kstNb.value - 1, kstBuffer, optInSignalPeriod, optInSignalMAType, outKSTSignal);
      tempBegIdx.value = _xr8.begIdx();
      sigNb.value = _xr8.count();
      retCode = RetCode.SUCCESS;
      outBegIdx.value = startIdx;
      outNBElement.value = sigNb.value;
      return RetCode.SUCCESS ;
   }
   RetCode kstextImpl( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInROC1Period,
                       int optInROC2Period,
                       int optInROC3Period,
                       int optInROC4Period,
                       int optInMA1Period,
                       int optInMA2Period,
                       int optInMA3Period,
                       int optInMA4Period,
                       int optInSignalPeriod,
                       MAType optInROCMAType,
                       MAType optInSignalMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outKST[],
                       double outKSTSignal[] )
   {
      double[] kstBuffer;
      double[] tempBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackMA = 0;
      int sigStart = 0;
      int tempInteger = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rocNb = new MInteger();
      MInteger kstNb = new MInteger();
      MInteger legNb = new MInteger();
      MInteger sigNb = new MInteger();
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 10;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 15;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC3Period == Integer.MIN_VALUE ) {
         optInROC3Period = 20;
      } else if( optInROC3Period < 1 || optInROC3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC4Period == Integer.MIN_VALUE ) {
         optInROC4Period = 30;
      } else if( optInROC4Period < 1 || optInROC4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA1Period == Integer.MIN_VALUE ) {
         optInMA1Period = 10;
      } else if( optInMA1Period < 1 || optInMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA2Period == Integer.MIN_VALUE ) {
         optInMA2Period = 10;
      } else if( optInMA2Period < 1 || optInMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA3Period == Integer.MIN_VALUE ) {
         optInMA3Period = 10;
      } else if( optInMA3Period < 1 || optInMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA4Period == Integer.MIN_VALUE ) {
         optInMA4Period = 15;
      } else if( optInMA4Period < 1 || optInMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROCMAType == MAType.DEFAULT ) {
         optInROCMAType = MAType.SMA;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( outKST == outKSTSignal ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSignal = maLookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      sigStart = startIdx - lookbackSignal;
      lookbackMA = maLookback(optInMA1Period, optInROCMAType);
      tempInteger = maLookback(optInMA2Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA3Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA4Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = endIdx - sigStart + 1 + lookbackMA;
      kstBuffer = new double[(int)(tempInteger * 1)];
      tempBuffer = new double[(int)(tempInteger * 1)];
      OutRange _xr0 = roc(sigStart - maLookback(optInMA1Period, optInROCMAType), endIdx, inReal, optInROC1Period, kstBuffer);
      tempBegIdx.value = _xr0.begIdx();
      rocNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = ma(0, rocNb.value - 1, kstBuffer, optInMA1Period, optInROCMAType, kstBuffer);
      tempBegIdx.value = _xr1.begIdx();
      kstNb.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = roc(sigStart - maLookback(optInMA2Period, optInROCMAType), endIdx, inReal, optInROC2Period, tempBuffer);
      tempBegIdx.value = _xr2.begIdx();
      rocNb.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = ma(0, rocNb.value - 1, tempBuffer, optInMA2Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr3.begIdx();
      legNb.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(2.0, tempBuffer[i], kstBuffer[i]);
      }
      OutRange _xr4 = roc(sigStart - maLookback(optInMA3Period, optInROCMAType), endIdx, inReal, optInROC3Period, tempBuffer);
      tempBegIdx.value = _xr4.begIdx();
      rocNb.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr5 = ma(0, rocNb.value - 1, tempBuffer, optInMA3Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr5.begIdx();
      legNb.value = _xr5.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(3.0, tempBuffer[i], kstBuffer[i]);
      }
      OutRange _xr6 = roc(sigStart - maLookback(optInMA4Period, optInROCMAType), endIdx, inReal, optInROC4Period, tempBuffer);
      tempBegIdx.value = _xr6.begIdx();
      rocNb.value = _xr6.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr7 = ma(0, rocNb.value - 1, tempBuffer, optInMA4Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr7.begIdx();
      legNb.value = _xr7.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(4.0, tempBuffer[i], kstBuffer[i]);
      }
      System.arraycopy(kstBuffer, lookbackSignal, outKST, 0, (endIdx - startIdx + 1) * 1);
      OutRange _xr8 = ma(0, kstNb.value - 1, kstBuffer, optInSignalPeriod, optInSignalMAType, outKSTSignal);
      tempBegIdx.value = _xr8.begIdx();
      sigNb.value = _xr8.count();
      retCode = RetCode.SUCCESS;
      outBegIdx.value = startIdx;
      outNBElement.value = sigNb.value;
      return RetCode.SUCCESS ;
   }
   /**
    * Know Sure Thing with selectable moving averages: Pring's weighted sum of
    * four smoothed rates of change, with a signal line. One MA type smooths the
    * four legs and another the signal line. {@code KST} is Pring's definition,
    * with simple averages throughout. Formulas published since then smooth the
    * legs exponentially, and Pring allows a simple or an exponential signal
    * line. The reading is the same as {@code KST}'s: above zero the combined
    * momentum is positive, and the usual signals are the line crossing its
    * signal line and the line changing direction.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kstext">ta-lib.org/functions/kstext</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>With both MA types set to {@code TA_MAType_SMA} the outputs are those of {@code KST}.</li>
    * <li>Both outputs start at the first bar where the signal line exists. A signal period of 1 disables signal-line smoothing for every signal MAType: the signal is then a copy of the line.</li>
    * <li>The unstable period of a selected MA type lengthens the lookback, for the legs and for the signal line separately.</li>
    * <li>{@code TA_MAType_MAMA} ignores its period argument, so where it is selected every period above 1 gives the same average.</li>
    * <li>Each rate of change follows {@code ROC}: a zero price in the denominator makes that term 0.</li>
    * <li>The weights go with leg position, not with the length of the rate of change.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#kstextLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series (canonically the close)
    * @param optInROC1Period Rate-of-change period of leg 1 (weight 1) (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Rate-of-change period of leg 2 (weight 2) (default
    *        15; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC3Period Rate-of-change period of leg 3 (weight 3) (default
    *        20; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC4Period Rate-of-change period of leg 4 (weight 4) (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA1Period Period of the MA smoothing leg 1 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA2Period Period of the MA smoothing leg 2 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA3Period Period of the MA smoothing leg 3 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA4Period Period of the MA smoothing leg 4 (default 15; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROCMAType MA type smoothing the four legs (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outKST Know Sure Thing line. Must hold at least
    *        {@code endIdx - max(startIdx, kstextLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outKSTSignal Signal line: MA of the line. Must hold at least
    *        {@code endIdx - max(startIdx, kstextLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#kst
    * @see Core#roc
    * @see Core#ma
    * @see Core#macdext
    * @see Core#coppock
    */
   public OutRange kstext( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInROC1Period,
                           int optInROC2Period,
                           int optInROC3Period,
                           int optInROC4Period,
                           int optInMA1Period,
                           int optInMA2Period,
                           int optInMA3Period,
                           int optInMA4Period,
                           int optInSignalPeriod,
                           MAType optInROCMAType,
                           MAType optInSignalMAType,
                           double outKST[],
                           double outKSTSignal[] )
   {
      requireIndexRange("KSTEXT", startIdx, endIdx);
      requireArgument("KSTEXT", "optInROCMAType", optInROCMAType);
      requireArgument("KSTEXT", "optInSignalMAType", optInSignalMAType);
      int guardStart = clampedStart("KSTEXT", startIdx, kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KSTEXT", "inReal", inReal, guardInLen);
      requireLength("KSTEXT", "outKST", outKST, guardOutLen);
      requireLength("KSTEXT", "outKSTSignal", outKSTSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kstextImpl(startIdx, endIdx, inReal, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KSTEXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Know Sure Thing with selectable moving averages: Pring's weighted sum of
    * four smoothed rates of change, with a signal line. One MA type smooths the
    * four legs and another the signal line. {@code KST} is Pring's definition,
    * with simple averages throughout. Formulas published since then smooth the
    * legs exponentially, and Pring allows a simple or an exponential signal
    * line. The reading is the same as {@code KST}'s: above zero the combined
    * momentum is positive, and the usual signals are the line crossing its
    * signal line and the line changing direction.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kstext">ta-lib.org/functions/kstext</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>With both MA types set to {@code TA_MAType_SMA} the outputs are those of {@code KST}.</li>
    * <li>Both outputs start at the first bar where the signal line exists. A signal period of 1 disables signal-line smoothing for every signal MAType: the signal is then a copy of the line.</li>
    * <li>The unstable period of a selected MA type lengthens the lookback, for the legs and for the signal line separately.</li>
    * <li>{@code TA_MAType_MAMA} ignores its period argument, so where it is selected every period above 1 gives the same average.</li>
    * <li>Each rate of change follows {@code ROC}: a zero price in the denominator makes that term 0.</li>
    * <li>The weights go with leg position, not with the length of the rate of change.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#kstextLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series (canonically the close)
    * @param optInROC1Period Rate-of-change period of leg 1 (weight 1) (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC2Period Rate-of-change period of leg 2 (weight 2) (default
    *        15; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC3Period Rate-of-change period of leg 3 (weight 3) (default
    *        20; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROC4Period Rate-of-change period of leg 4 (weight 4) (default
    *        30; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA1Period Period of the MA smoothing leg 1 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA2Period Period of the MA smoothing leg 2 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA3Period Period of the MA smoothing leg 3 (default 10; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMA4Period Period of the MA smoothing leg 4 (default 15; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInROCMAType MA type smoothing the four legs (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outKST Know Sure Thing line. Must hold at least
    *        {@code endIdx - max(startIdx, kstextLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outKSTSignal Signal line: MA of the line. Must hold at least
    *        {@code endIdx - max(startIdx, kstextLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#kst
    * @see Core#roc
    * @see Core#ma
    * @see Core#macdext
    * @see Core#coppock
    */
   public OutRange kstext( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInROC1Period,
                           int optInROC2Period,
                           int optInROC3Period,
                           int optInROC4Period,
                           int optInMA1Period,
                           int optInMA2Period,
                           int optInMA3Period,
                           int optInMA4Period,
                           int optInSignalPeriod,
                           MAType optInROCMAType,
                           MAType optInSignalMAType,
                           double outKST[],
                           double outKSTSignal[] )
   {
      requireIndexRange("KSTEXT", startIdx, endIdx);
      requireArgument("KSTEXT", "optInROCMAType", optInROCMAType);
      requireArgument("KSTEXT", "optInSignalMAType", optInSignalMAType);
      int guardStart = clampedStart("KSTEXT", startIdx, kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KSTEXT", "inReal", inReal, guardInLen);
      requireLength("KSTEXT", "outKST", outKST, guardOutLen);
      requireLength("KSTEXT", "outKSTSignal", outKSTSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kstextImpl(startIdx, endIdx, inReal, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KSTEXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KSTEXT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#kstext} over the same series.
    * Open with {@link Core#kstextOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class KstextStream {
      private Core core;
      private int optInROC1Period;
      private int optInROC2Period;
      private int optInROC3Period;
      private int optInROC4Period;
      private int optInMA1Period;
      private int optInMA2Period;
      private int optInMA3Period;
      private int optInMA4Period;
      private int optInSignalPeriod;
      private MAType optInROCMAType;
      private MAType optInSignalMAType;
      private double cur_outKST;
      private double cur_outKSTSignal;
      private RocStream sub0;
      private MaStream sub1;
      private RocStream sub2;
      private MaStream sub3;
      private RocStream sub4;
      private MaStream sub5;
      private RocStream sub6;
      private MaStream sub7;
      private MaStream sub8;
      private int outRangeBegIdx;
      private int outRangeCount;

      private KstextStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#kstext} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value(KstextOut)} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("KSTEXT advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private KstextStream( KstextStream other ) {
         this.core = other.core;
         this.optInROC1Period = other.optInROC1Period;
         this.optInROC2Period = other.optInROC2Period;
         this.optInROC3Period = other.optInROC3Period;
         this.optInROC4Period = other.optInROC4Period;
         this.optInMA1Period = other.optInMA1Period;
         this.optInMA2Period = other.optInMA2Period;
         this.optInMA3Period = other.optInMA3Period;
         this.optInMA4Period = other.optInMA4Period;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.optInROCMAType = other.optInROCMAType;
         this.optInSignalMAType = other.optInSignalMAType;
         this.cur_outKST = other.cur_outKST;
         this.cur_outKSTSignal = other.cur_outKSTSignal;
         this.sub0 = new RocStream(other.sub0);
         this.sub1 = new MaStream(other.sub1);
         this.sub2 = new RocStream(other.sub2);
         this.sub3 = new MaStream(other.sub3);
         this.sub4 = new RocStream(other.sub4);
         this.sub5 = new MaStream(other.sub5);
         this.sub6 = new RocStream(other.sub6);
         this.sub7 = new MaStream(other.sub7);
         this.sub8 = new MaStream(other.sub8);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(KstextOut)} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public void update( double inReal, KstextOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("KSTEXT update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("KSTEXT update", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("KSTEXT update", "inReal");
         core.kstextStepImpl(this, inReal);
         this.outRangeCount++;
         out.kst = this.cur_outKST;
         out.kstSignal = this.cur_outKSTSignal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inReal, KstextOut out ) {
         requireArgument("KSTEXT peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("KSTEXT peek", "inReal");
         KstextStream sp = this;
         double cur_kstBuffer = 0.0;
         double cur_tempBuffer = 0.0;
         double cur_outKSTSignal = 0.0;
         double cur_outKST = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_kstBuffer = sp.sub0.peek(inReal);
         cur_kstBuffer = sp.sub1.peek(cur_kstBuffer);
         cur_tempBuffer = sp.sub2.peek(inReal);
         cur_tempBuffer = sp.sub3.peek(cur_tempBuffer);
         /* Combine map (batch tail, per bar). */
         cur_kstBuffer = Math.fma(2.0, cur_tempBuffer, cur_kstBuffer);
         cur_tempBuffer = sp.sub4.peek(inReal);
         cur_tempBuffer = sp.sub5.peek(cur_tempBuffer);
         /* Combine map (batch tail, per bar). */
         cur_kstBuffer = Math.fma(3.0, cur_tempBuffer, cur_kstBuffer);
         cur_tempBuffer = sp.sub6.peek(inReal);
         cur_tempBuffer = sp.sub7.peek(cur_tempBuffer);
         /* Combine map (batch tail, per bar). */
         cur_kstBuffer = Math.fma(4.0, cur_tempBuffer, cur_kstBuffer);
         cur_outKSTSignal = sp.sub8.peek(cur_kstBuffer);
         cur_outKST = cur_kstBuffer;
         out.kst = cur_outKST;
         out.kstSignal = cur_outKSTSignal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( KstextOut out ) {
         requireArgument("KSTEXT value", "out", out);
         out.kst = this.cur_outKST;
         out.kstSignal = this.cur_outKSTSignal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public KstextStream clone() {
         return new KstextStream(this);
      }
   }

   /**
    * The outputs of one KSTEXT bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class KstextOut {
      /** Know Sure Thing line. */
      public double kst;
      /** Signal line: MA of the line. */
      public double kstSignal;
   }
   private void kstextStepImpl( KstextStream sp, double inReal )
   {
      double cur_kstBuffer = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outKSTSignal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_kstBuffer = sp.sub0.update(inReal);
      cur_kstBuffer = sp.sub1.update(cur_kstBuffer);
      cur_tempBuffer = sp.sub2.update(inReal);
      cur_tempBuffer = sp.sub3.update(cur_tempBuffer);
      /* Combine map (batch tail, per bar). */
      cur_kstBuffer = Math.fma(2.0, cur_tempBuffer, cur_kstBuffer);
      cur_tempBuffer = sp.sub4.update(inReal);
      cur_tempBuffer = sp.sub5.update(cur_tempBuffer);
      /* Combine map (batch tail, per bar). */
      cur_kstBuffer = Math.fma(3.0, cur_tempBuffer, cur_kstBuffer);
      cur_tempBuffer = sp.sub6.update(inReal);
      cur_tempBuffer = sp.sub7.update(cur_tempBuffer);
      /* Combine map (batch tail, per bar). */
      cur_kstBuffer = Math.fma(4.0, cur_tempBuffer, cur_kstBuffer);
      cur_outKSTSignal = sp.sub8.update(cur_kstBuffer);
      sp.cur_outKST = cur_kstBuffer;
      sp.cur_outKSTSignal = cur_outKSTSignal;
   }
   private RetCode kstextOpenImpl( KstextStream sp, double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outKST[], double outKSTSignal[], int outStride )
   {
      double[] kstBuffer;
      double[] tempBuffer;
      RetCode retCode;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackMA = 0;
      int sigStart = 0;
      int tempInteger = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger rocNb = new MInteger();
      MInteger kstNb = new MInteger();
      MInteger legNb = new MInteger();
      MInteger sigNb = new MInteger();
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInROC1Period == Integer.MIN_VALUE ) {
         optInROC1Period = 10;
      } else if( optInROC1Period < 1 || optInROC1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC2Period == Integer.MIN_VALUE ) {
         optInROC2Period = 15;
      } else if( optInROC2Period < 1 || optInROC2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC3Period == Integer.MIN_VALUE ) {
         optInROC3Period = 20;
      } else if( optInROC3Period < 1 || optInROC3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROC4Period == Integer.MIN_VALUE ) {
         optInROC4Period = 30;
      } else if( optInROC4Period < 1 || optInROC4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA1Period == Integer.MIN_VALUE ) {
         optInMA1Period = 10;
      } else if( optInMA1Period < 1 || optInMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA2Period == Integer.MIN_VALUE ) {
         optInMA2Period = 10;
      } else if( optInMA2Period < 1 || optInMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA3Period == Integer.MIN_VALUE ) {
         optInMA3Period = 10;
      } else if( optInMA3Period < 1 || optInMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMA4Period == Integer.MIN_VALUE ) {
         optInMA4Period = 15;
      } else if( optInMA4Period < 1 || optInMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInROCMAType == MAType.DEFAULT ) {
         optInROCMAType = MAType.SMA;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outKST = outStride == 1 ? outKST : new double[historyLen];
      double[] sc_outKSTSignal = outStride == 1 ? outKSTSignal : new double[historyLen];
      /* With every type SMA this is bit-exact with kst(): each leg's average
       * starts on the first bar the signal consumes, and the line accumulates
       * its legs left to right. Changing either breaks the equality.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackSignal = maLookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      sigStart = startIdx - lookbackSignal;
      /* Both buffers are sized by the longest leg average: a rate of change
       * starts its average's lookback before sigStart.
       */
      lookbackMA = maLookback(optInMA1Period, optInROCMAType);
      tempInteger = maLookback(optInMA2Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA3Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = maLookback(optInMA4Period, optInROCMAType);
      if( tempInteger > lookbackMA ) {
         lookbackMA = tempInteger;
      }
      tempInteger = endIdx - sigStart + 1 + lookbackMA;
      kstBuffer = new double[(int)(tempInteger * 1)];
      tempBuffer = new double[(int)(tempInteger * 1)];
      /* Sub-stream 0: roc over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RocStream sub0 = rocOpenAndFillInternal(inReal, sigStart - maLookback(optInMA1Period, optInROCMAType), optInROC1Period, tempBegIdx, rocNb, kstBuffer);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 1: ma over `kstBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub1 = maOpenInternal(java.util.Arrays.copyOfRange(kstBuffer, 0, (rocNb.value - 1) + 1), 0, optInMA1Period, optInROCMAType);
      OutRange _xr0 = ma(0, rocNb.value - 1, kstBuffer, optInMA1Period, optInROCMAType, kstBuffer);
      tempBegIdx.value = _xr0.begIdx();
      kstNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      /* Sub-stream 2: roc over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RocStream sub2 = rocOpenAndFillInternal(inReal, sigStart - maLookback(optInMA2Period, optInROCMAType), optInROC2Period, tempBegIdx, rocNb, tempBuffer);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 3: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub3 = maOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (rocNb.value - 1) + 1), 0, optInMA2Period, optInROCMAType);
      OutRange _xr1 = ma(0, rocNb.value - 1, tempBuffer, optInMA2Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr1.begIdx();
      legNb.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(2.0, tempBuffer[i], kstBuffer[i]);
      }
      /* Sub-stream 4: roc over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RocStream sub4 = rocOpenAndFillInternal(inReal, sigStart - maLookback(optInMA3Period, optInROCMAType), optInROC3Period, tempBegIdx, rocNb, tempBuffer);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 5: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub5 = maOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (rocNb.value - 1) + 1), 0, optInMA3Period, optInROCMAType);
      OutRange _xr2 = ma(0, rocNb.value - 1, tempBuffer, optInMA3Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr2.begIdx();
      legNb.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(3.0, tempBuffer[i], kstBuffer[i]);
      }
      /* Sub-stream 6: roc over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RocStream sub6 = rocOpenAndFillInternal(inReal, sigStart - maLookback(optInMA4Period, optInROCMAType), optInROC4Period, tempBegIdx, rocNb, tempBuffer);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 7: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub7 = maOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (rocNb.value - 1) + 1), 0, optInMA4Period, optInROCMAType);
      OutRange _xr3 = ma(0, rocNb.value - 1, tempBuffer, optInMA4Period, optInROCMAType, tempBuffer);
      tempBegIdx.value = _xr3.begIdx();
      legNb.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < kstNb.value; i += 1 ) {
         kstBuffer[i] = Math.fma(4.0, tempBuffer[i], kstBuffer[i]);
      }
      /* Every read of inReal is done: an output may alias it. */
      System.arraycopy(kstBuffer, lookbackSignal, sc_outKST, 0, (endIdx - startIdx + 1) * 1);
      /* Sub-stream 8: ma over `kstBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub8 = maOpenAndFillInternal(java.util.Arrays.copyOfRange(kstBuffer, 0, (kstNb.value - 1) + 1), 0, optInSignalPeriod, optInSignalMAType, tempBegIdx, sigNb, sc_outKSTSignal);
      retCode = RetCode.SUCCESS;
      outBegIdx.value = startIdx;
      outNBElement.value = sigNb.value;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInROC1Period = optInROC1Period;
      sp.optInROC2Period = optInROC2Period;
      sp.optInROC3Period = optInROC3Period;
      sp.optInROC4Period = optInROC4Period;
      sp.optInMA1Period = optInMA1Period;
      sp.optInMA2Period = optInMA2Period;
      sp.optInMA3Period = optInMA3Period;
      sp.optInMA4Period = optInMA4Period;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.optInROCMAType = optInROCMAType;
      sp.optInSignalMAType = optInSignalMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.sub3 = sub3;
      sp.sub4 = sub4;
      sp.sub5 = sub5;
      sp.sub6 = sub6;
      sp.sub7 = sub7;
      sp.sub8 = sub8;
      sp.cur_outKST = sc_outKST[outNBElement.value - 1];
      sp.cur_outKSTSignal = sc_outKSTSignal[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* kstextOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KstextStream kstextOpenAndFillInternal( double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outKST[], double outKSTSignal[] )
   {
      KstextStream sp = new KstextStream(this);
      RetCode retCode = kstextOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KSTEXT openAndFill", inReal.length, startIdx, kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType));
      }
      throw streamFailure("KSTEXT openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind kstextOpen (composition seam). */
   KstextStream kstextOpenInternal( double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType )
   {
      KstextStream sp = new KstextStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outKST = new double[1];
      double[] sink_outKSTSignal = new double[1];
      RetCode retCode = kstextOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, sink_outKST, sink_outKSTSignal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KSTEXT open", inReal.length, startIdx, kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType));
      }
      throw streamFailure("KSTEXT open", retCode);
   }
   /**
    * Open a live KSTEXT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#kstext} at that bar.
    * <p>The history must hold at least {@code kstextLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link MAType#DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KstextStream kstextOpen( double inReal[], int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType )
   {
      requireArgument("KSTEXT open", "inReal", inReal);
      requireHistory("KSTEXT open", inReal.length);
      requireArgument("KSTEXT open", "optInROCMAType", optInROCMAType);
      requireArgument("KSTEXT open", "optInSignalMAType", optInSignalMAType);
      return kstextOpenInternal(inReal, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType);
   }
   /**
    * {@link Core#kstextOpen} that also fills the output array(s) bit-identically
    * to {@link Core#kstext} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KstextStream#outRange()}.
    */
   public KstextStream kstextOpenAndFill( double inReal[], int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInMA1Period, int optInMA2Period, int optInMA3Period, int optInMA4Period, int optInSignalPeriod, MAType optInROCMAType, MAType optInSignalMAType, double outKST[], double outKSTSignal[] )
   {
      requireArgument("KSTEXT openAndFill", "inReal", inReal);
      requireHistory("KSTEXT openAndFill", inReal.length);
      requireArgument("KSTEXT openAndFill", "optInROCMAType", optInROCMAType);
      requireArgument("KSTEXT openAndFill", "optInSignalMAType", optInSignalMAType);
      int guardOutLen = openFillCount("KSTEXT openAndFill", inReal.length, kstextLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType));
      requireLength("KSTEXT openAndFill", "outKST", outKST, guardOutLen);
      requireLength("KSTEXT openAndFill", "outKSTSignal", outKSTSignal, guardOutLen);
      if( (Object)outKST == (Object)inReal || (Object)outKSTSignal == (Object)inReal || (Object)outKST == (Object)outKSTSignal ) {
         throw streamFailure("KSTEXT openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kstextOpenAndFillInternal(inReal, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInMA1Period, optInMA2Period, optInMA3Period, optInMA4Period, optInSignalPeriod, optInROCMAType, optInSignalMAType, outBegIdx, outNBElement, outKST, outKSTSignal);
   }
