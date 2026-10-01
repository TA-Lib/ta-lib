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
 *  092926 MF,CC  First version (#472).
 */

   /**
    * Number of leading input bars {@link Core#kst} consumes before it can
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
    * @param optInSMA1Period Simple-moving-average period smoothing leg 1
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA2Period Simple-moving-average period smoothing leg 2
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA3Period Simple-moving-average period smoothing leg 3
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA4Period Simple-moving-average period smoothing leg 4
    *        (default 15; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Simple-moving-average period of the signal line
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int kstLookback( int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod )
   {
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
      if( optInSMA1Period == Integer.MIN_VALUE ) {
         optInSMA1Period = 10;
      } else if( optInSMA1Period < 1 || optInSMA1Period > 100000 ) {
         return -1;
      }
      if( optInSMA2Period == Integer.MIN_VALUE ) {
         optInSMA2Period = 10;
      } else if( optInSMA2Period < 1 || optInSMA2Period > 100000 ) {
         return -1;
      }
      if( optInSMA3Period == Integer.MIN_VALUE ) {
         optInSMA3Period = 10;
      } else if( optInSMA3Period < 1 || optInSMA3Period > 100000 ) {
         return -1;
      }
      if( optInSMA4Period == Integer.MIN_VALUE ) {
         optInSMA4Period = 15;
      } else if( optInSMA4Period < 1 || optInSMA4Period > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      int legMax;
      int leg;
      legMax = optInROC1Period + optInSMA1Period - 1;
      leg = optInROC2Period + optInSMA2Period - 1;
      if( leg > legMax ) {
         legMax = leg;
      }
      leg = optInROC3Period + optInSMA3Period - 1;
      if( leg > legMax ) {
         legMax = leg;
      }
      leg = optInROC4Period + optInSMA4Period - 1;
      if( leg > legMax ) {
         legMax = leg;
      }
      return legMax + optInSignalPeriod - 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#kst}.
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
    * @param optInSMA1Period Simple-moving-average period smoothing leg 1
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA2Period Simple-moving-average period smoothing leg 2
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA3Period Simple-moving-average period smoothing leg 3
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA4Period Simple-moving-average period smoothing leg 4
    *        (default 15; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Simple-moving-average period of the signal line
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int kstDisplayShift( int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod, int outputIdx )
   {
      if( kstLookback( optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode kstImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInROC1Period,
                    int optInROC2Period,
                    int optInROC3Period,
                    int optInROC4Period,
                    int optInSMA1Period,
                    int optInSMA2Period,
                    int optInSMA3Period,
                    int optInSMA4Period,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outKST[],
                    double outKSTSignal[] )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int sigStart = 0;
      int den1 = 0;
      int den2 = 0;
      int den3 = 0;
      int den4 = 0;
      double prior = 0;
      double roc = 0;
      double kst = 0;
      double sig = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double sigTotal = 0;
      double rcma1 = 0;
      double rcma2 = 0;
      double rcma3 = 0;
      double rcma4 = 0;
      double[] ring1;
      int ring1_Idx = 0;
      int maxIdx_ring1 = (30)-1;
      double[] ring2;
      int ring2_Idx = 0;
      int maxIdx_ring2 = (30)-1;
      double[] ring3;
      int ring3_Idx = 0;
      int maxIdx_ring3 = (30)-1;
      double[] ring4;
      int ring4_Idx = 0;
      int maxIdx_ring4 = (30)-1;
      double[] sigRing;
      int sigRing_Idx = 0;
      int maxIdx_sigRing = (30)-1;
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
      if( optInSMA1Period == Integer.MIN_VALUE ) {
         optInSMA1Period = 10;
      } else if( optInSMA1Period < 1 || optInSMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA2Period == Integer.MIN_VALUE ) {
         optInSMA2Period = 10;
      } else if( optInSMA2Period < 1 || optInSMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA3Period == Integer.MIN_VALUE ) {
         optInSMA3Period = 10;
      } else if( optInSMA3Period < 1 || optInSMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA4Period == Integer.MIN_VALUE ) {
         optInSMA4Period = 15;
      } else if( optInSMA4Period < 1 || optInSMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outKST == outKSTSignal ) {
         return RetCode.BAD_PARAM ;
      }
      /* Bit-exact with TA_ROC per leg into TA_SMA, each SMA called at the start
       * its consumer needs: keep every running sum's add/subtract order and the
       * left-to-right weighted sum, or the composite differential stops being
       * exact. Each ring holds its stage's last SMA-period values, so the only
       * inputs read are the current bar and each leg's lagged denominator,
       * both at or after the slot the outputs write: outKST or outKSTSignal
       * may alias inReal.
       */
      lookbackTotal = kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInSMA1Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring1 = new double[optInSMA1Period];
      maxIdx_ring1 = (optInSMA1Period)-1;
      ring1_Idx = 0;
      if( optInSMA2Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring2 = new double[optInSMA2Period];
      maxIdx_ring2 = (optInSMA2Period)-1;
      ring2_Idx = 0;
      if( optInSMA3Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring3 = new double[optInSMA3Period];
      maxIdx_ring3 = (optInSMA3Period)-1;
      ring3_Idx = 0;
      if( optInSMA4Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring4 = new double[optInSMA4Period];
      maxIdx_ring4 = (optInSMA4Period)-1;
      ring4_Idx = 0;
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sigRing = new double[optInSignalPeriod];
      maxIdx_sigRing = (optInSignalPeriod)-1;
      sigRing_Idx = 0;
      sigStart = startIdx - (optInSignalPeriod - 1);
      sigTotal = 0.0;
      total4 = sigTotal;
      total3 = total4;
      total2 = total3;
      total1 = total2;
      inIdx = sigStart - (optInSMA1Period - 1);
      den1 = inIdx - optInROC1Period;
      while( inIdx < sigStart ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA2Period - 1);
      den2 = inIdx - optInROC2Period;
      while( inIdx < sigStart ) {
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA3Period - 1);
      den3 = inIdx - optInROC3Period;
      while( inIdx < sigStart ) {
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA4Period - 1);
      den4 = inIdx - optInROC4Period;
      while( inIdx < sigStart ) {
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart;
      while( inIdx < startIdx ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         inIdx += 1;
      }
      outIdx = 0;
      while( inIdx <= endIdx ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sig = sigTotal / (double)optInSignalPeriod;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         sigTotal -= sigRing[sigRing_Idx];
         outKST[outIdx] = kst;
         outKSTSignal[outIdx] = sig;
         outIdx += 1;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode kstImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInROC1Period,
                    int optInROC2Period,
                    int optInROC3Period,
                    int optInROC4Period,
                    int optInSMA1Period,
                    int optInSMA2Period,
                    int optInSMA3Period,
                    int optInSMA4Period,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outKST[],
                    double outKSTSignal[] )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int sigStart = 0;
      int den1 = 0;
      int den2 = 0;
      int den3 = 0;
      int den4 = 0;
      double prior = 0;
      double roc = 0;
      double kst = 0;
      double sig = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double sigTotal = 0;
      double rcma1 = 0;
      double rcma2 = 0;
      double rcma3 = 0;
      double rcma4 = 0;
      double[] ring1;
      int ring1_Idx = 0;
      int maxIdx_ring1 = (30)-1;
      double[] ring2;
      int ring2_Idx = 0;
      int maxIdx_ring2 = (30)-1;
      double[] ring3;
      int ring3_Idx = 0;
      int maxIdx_ring3 = (30)-1;
      double[] ring4;
      int ring4_Idx = 0;
      int maxIdx_ring4 = (30)-1;
      double[] sigRing;
      int sigRing_Idx = 0;
      int maxIdx_sigRing = (30)-1;
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
      if( optInSMA1Period == Integer.MIN_VALUE ) {
         optInSMA1Period = 10;
      } else if( optInSMA1Period < 1 || optInSMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA2Period == Integer.MIN_VALUE ) {
         optInSMA2Period = 10;
      } else if( optInSMA2Period < 1 || optInSMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA3Period == Integer.MIN_VALUE ) {
         optInSMA3Period = 10;
      } else if( optInSMA3Period < 1 || optInSMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA4Period == Integer.MIN_VALUE ) {
         optInSMA4Period = 15;
      } else if( optInSMA4Period < 1 || optInSMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outKST == outKSTSignal ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInSMA1Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring1 = new double[optInSMA1Period];
      maxIdx_ring1 = (optInSMA1Period)-1;
      ring1_Idx = 0;
      if( optInSMA2Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring2 = new double[optInSMA2Period];
      maxIdx_ring2 = (optInSMA2Period)-1;
      ring2_Idx = 0;
      if( optInSMA3Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring3 = new double[optInSMA3Period];
      maxIdx_ring3 = (optInSMA3Period)-1;
      ring3_Idx = 0;
      if( optInSMA4Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring4 = new double[optInSMA4Period];
      maxIdx_ring4 = (optInSMA4Period)-1;
      ring4_Idx = 0;
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sigRing = new double[optInSignalPeriod];
      maxIdx_sigRing = (optInSignalPeriod)-1;
      sigRing_Idx = 0;
      sigStart = startIdx - (optInSignalPeriod - 1);
      sigTotal = 0.0;
      total4 = sigTotal;
      total3 = total4;
      total2 = total3;
      total1 = total2;
      inIdx = sigStart - (optInSMA1Period - 1);
      den1 = inIdx - optInROC1Period;
      while( inIdx < sigStart ) {
         prior = (double)inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA2Period - 1);
      den2 = inIdx - optInROC2Period;
      while( inIdx < sigStart ) {
         prior = (double)inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA3Period - 1);
      den3 = inIdx - optInROC3Period;
      while( inIdx < sigStart ) {
         prior = (double)inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA4Period - 1);
      den4 = inIdx - optInROC4Period;
      while( inIdx < sigStart ) {
         prior = (double)inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart;
      while( inIdx < startIdx ) {
         prior = (double)inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = (double)inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = (double)inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = (double)inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         inIdx += 1;
      }
      outIdx = 0;
      while( inIdx <= endIdx ) {
         prior = (double)inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = (double)inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = (double)inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = (double)inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? ((double)inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sig = sigTotal / (double)optInSignalPeriod;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         sigTotal -= sigRing[sigRing_Idx];
         outKST[outIdx] = kst;
         outKSTSignal[outIdx] = sig;
         outIdx += 1;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of
    * four smoothed rates of change with a moving-average signal line. Each leg
    * smooths a rate of change over a different span, and the longer legs carry
    * the larger weights, so the line follows the dominant trend while the short
    * legs make it turn early. Readings above zero mean the combined momentum is
    * positive. The usual signals are the line crossing its signal line and the
    * line changing direction. The level is unbounded, and the usual comparison
    * is against the line's own history.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kst">ta-lib.org/functions/kst</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The weights are 1 to 4 and nothing divides by their total. Pring's daily table prints each leg's SMA period times its weight (a total of 120) only to show how much the longest leg dominates.</li>
    * <li>Both outputs start at the first bar where the signal line exists, so the first few bars where the KST line alone is defined are not emitted. Setting the signal period to 1 returns the line from its own first bar, and the signal output is then a copy of it.</li>
    * <li>Each rate of change follows {@code ROC}: a zero price in the denominator makes that term 0.</li>
    * <li>The legs can be in any order. The weights go with leg position, not with the length of the rate of change.</li>
    * <li>Pring's daily page uses a 10-day signal. The default signal period follows the charting platforms instead.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#kstLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    * @param optInSMA1Period Simple-moving-average period smoothing leg 1
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA2Period Simple-moving-average period smoothing leg 2
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA3Period Simple-moving-average period smoothing leg 3
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA4Period Simple-moving-average period smoothing leg 4
    *        (default 15; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Simple-moving-average period of the signal line
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outKST Know Sure Thing line. Must hold at least
    *        {@code endIdx - max(startIdx, kstLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @param outKSTSignal Simple moving average of the line. Must hold at least
    *        {@code endIdx - max(startIdx, kstLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
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
    * @see Core#kstext
    * @see Core#roc
    * @see Core#sma
    * @see Core#coppock
    * @see Core#macd
    */
   public OutRange kst( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInROC1Period,
                        int optInROC2Period,
                        int optInROC3Period,
                        int optInROC4Period,
                        int optInSMA1Period,
                        int optInSMA2Period,
                        int optInSMA3Period,
                        int optInSMA4Period,
                        int optInSignalPeriod,
                        double outKST[],
                        double outKSTSignal[] )
   {
      requireIndexRange("KST", startIdx, endIdx);
      int guardStart = clampedStart("KST", startIdx, kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KST", "inReal", inReal, guardInLen);
      requireLength("KST", "outKST", outKST, guardOutLen);
      requireLength("KST", "outKSTSignal", outKSTSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kstImpl(startIdx, endIdx, inReal, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KST", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of
    * four smoothed rates of change with a moving-average signal line. Each leg
    * smooths a rate of change over a different span, and the longer legs carry
    * the larger weights, so the line follows the dominant trend while the short
    * legs make it turn early. Readings above zero mean the combined momentum is
    * positive. The usual signals are the line crossing its signal line and the
    * line changing direction. The level is unbounded, and the usual comparison
    * is against the line's own history.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kst">ta-lib.org/functions/kst</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The weights are 1 to 4 and nothing divides by their total. Pring's daily table prints each leg's SMA period times its weight (a total of 120) only to show how much the longest leg dominates.</li>
    * <li>Both outputs start at the first bar where the signal line exists, so the first few bars where the KST line alone is defined are not emitted. Setting the signal period to 1 returns the line from its own first bar, and the signal output is then a copy of it.</li>
    * <li>Each rate of change follows {@code ROC}: a zero price in the denominator makes that term 0.</li>
    * <li>The legs can be in any order. The weights go with leg position, not with the length of the rate of change.</li>
    * <li>Pring's daily page uses a 10-day signal. The default signal period follows the charting platforms instead.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#kstLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    * @param optInSMA1Period Simple-moving-average period smoothing leg 1
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA2Period Simple-moving-average period smoothing leg 2
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA3Period Simple-moving-average period smoothing leg 3
    *        (default 10; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSMA4Period Simple-moving-average period smoothing leg 4
    *        (default 15; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Simple-moving-average period of the signal line
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outKST Know Sure Thing line. Must hold at least
    *        {@code endIdx - max(startIdx, kstLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
    * @param outKSTSignal Simple moving average of the line. Must hold at least
    *        {@code endIdx - max(startIdx, kstLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
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
    * @see Core#kstext
    * @see Core#roc
    * @see Core#sma
    * @see Core#coppock
    * @see Core#macd
    */
   public OutRange kst( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInROC1Period,
                        int optInROC2Period,
                        int optInROC3Period,
                        int optInROC4Period,
                        int optInSMA1Period,
                        int optInSMA2Period,
                        int optInSMA3Period,
                        int optInSMA4Period,
                        int optInSignalPeriod,
                        double outKST[],
                        double outKSTSignal[] )
   {
      requireIndexRange("KST", startIdx, endIdx);
      int guardStart = clampedStart("KST", startIdx, kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KST", "inReal", inReal, guardInLen);
      requireLength("KST", "outKST", outKST, guardOutLen);
      requireLength("KST", "outKSTSignal", outKSTSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kstImpl(startIdx, endIdx, inReal, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KST", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KST stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#kst} over the same series.
    * Open with {@link Core#kstOpen}; there is no close — the handle is
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
   public static final class KstStream {
      private Core core;
      private int optInROC1Period;
      private int optInROC2Period;
      private int optInROC3Period;
      private int optInROC4Period;
      private int optInSMA1Period;
      private int optInSMA2Period;
      private int optInSMA3Period;
      private int optInSMA4Period;
      private int optInSignalPeriod;
      private double total1;
      private double total2;
      private double total3;
      private double total4;
      private double sigTotal;
      private int ring1_Idx;
      private int ring2_Idx;
      private int ring3_Idx;
      private int ring4_Idx;
      private int sigRing_Idx;
      private int maxIdx_ring1;
      private int maxIdx_ring2;
      private int maxIdx_ring3;
      private int maxIdx_ring4;
      private int maxIdx_sigRing;
      private int ringPos_den1;
      private int ringCap_den1;
      private double[] ring_den1_inReal;
      private int ringPos_den2;
      private int ringCap_den2;
      private double[] ring_den2_inReal;
      private int ringPos_den3;
      private int ringCap_den3;
      private double[] ring_den3_inReal;
      private int ringPos_den4;
      private int ringCap_den4;
      private double[] ring_den4_inReal;
      private int cbSize_ring1;
      private double[] cb_ring1;
      private int cbSize_ring2;
      private double[] cb_ring2;
      private int cbSize_ring3;
      private double[] cb_ring3;
      private int cbSize_ring4;
      private double[] cb_ring4;
      private int cbSize_sigRing;
      private double[] cb_sigRing;
      private double cur_outKST;
      private double cur_outKSTSignal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private KstStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#kst} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(KstOut)} keeps answering the previous
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
            throw failure("KST advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private KstStream( KstStream other ) {
         this.core = other.core;
         this.optInROC1Period = other.optInROC1Period;
         this.optInROC2Period = other.optInROC2Period;
         this.optInROC3Period = other.optInROC3Period;
         this.optInROC4Period = other.optInROC4Period;
         this.optInSMA1Period = other.optInSMA1Period;
         this.optInSMA2Period = other.optInSMA2Period;
         this.optInSMA3Period = other.optInSMA3Period;
         this.optInSMA4Period = other.optInSMA4Period;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.total1 = other.total1;
         this.total2 = other.total2;
         this.total3 = other.total3;
         this.total4 = other.total4;
         this.sigTotal = other.sigTotal;
         this.ring1_Idx = other.ring1_Idx;
         this.ring2_Idx = other.ring2_Idx;
         this.ring3_Idx = other.ring3_Idx;
         this.ring4_Idx = other.ring4_Idx;
         this.sigRing_Idx = other.sigRing_Idx;
         this.maxIdx_ring1 = other.maxIdx_ring1;
         this.maxIdx_ring2 = other.maxIdx_ring2;
         this.maxIdx_ring3 = other.maxIdx_ring3;
         this.maxIdx_ring4 = other.maxIdx_ring4;
         this.maxIdx_sigRing = other.maxIdx_sigRing;
         this.ringPos_den1 = other.ringPos_den1;
         this.ringCap_den1 = other.ringCap_den1;
         this.ring_den1_inReal = other.ring_den1_inReal.clone();
         this.ringPos_den2 = other.ringPos_den2;
         this.ringCap_den2 = other.ringCap_den2;
         this.ring_den2_inReal = other.ring_den2_inReal.clone();
         this.ringPos_den3 = other.ringPos_den3;
         this.ringCap_den3 = other.ringCap_den3;
         this.ring_den3_inReal = other.ring_den3_inReal.clone();
         this.ringPos_den4 = other.ringPos_den4;
         this.ringCap_den4 = other.ringCap_den4;
         this.ring_den4_inReal = other.ring_den4_inReal.clone();
         this.cbSize_ring1 = other.cbSize_ring1;
         this.cb_ring1 = other.cb_ring1.clone();
         this.cbSize_ring2 = other.cbSize_ring2;
         this.cb_ring2 = other.cb_ring2.clone();
         this.cbSize_ring3 = other.cbSize_ring3;
         this.cb_ring3 = other.cb_ring3.clone();
         this.cbSize_ring4 = other.cbSize_ring4;
         this.cb_ring4 = other.cb_ring4.clone();
         this.cbSize_sigRing = other.cbSize_sigRing;
         this.cb_sigRing = other.cb_sigRing.clone();
         this.cur_outKST = other.cur_outKST;
         this.cur_outKSTSignal = other.cur_outKSTSignal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(KstOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inReal, KstOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("KST update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("KST update", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("KST update", "inReal");
         core.kstStepImpl(this, inReal);
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
      public void peek( double inReal, KstOut out ) {
         requireArgument("KST peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("KST peek", "inReal");
         KstStream sp = this;
         double prior = 0.0;
         double roc = 0.0;
         double kst = 0.0;
         double sig = 0.0;
         double rcma1 = 0.0;
         double rcma2 = 0.0;
         double rcma3 = 0.0;
         double rcma4 = 0.0;
         double cur_outKST = 0.0;
         double cur_outKSTSignal = 0.0;
         int ring1_Idx = sp.ring1_Idx;
         int ring2_Idx = sp.ring2_Idx;
         int ring3_Idx = sp.ring3_Idx;
         int ring4_Idx = sp.ring4_Idx;
         int sigRing_Idx = sp.sigRing_Idx;
         double sigTotal = sp.sigTotal;
         double total1 = sp.total1;
         double total2 = sp.total2;
         double total3 = sp.total3;
         double total4 = sp.total4;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         int pkSlot3 = -1;
         double pkVal3 = 0.0;
         int pkSlot4 = -1;
         double pkVal4 = 0.0;
         int pkSlot5 = -1;
         double pkVal5 = 0.0;
         int pkSlot6 = -1;
         double pkVal6 = 0.0;
         int pkSlot7 = -1;
         double pkVal7 = 0.0;
         int pkSlot8 = -1;
         double pkVal8 = 0.0;
         if( sp.ringCap_den1 == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         if( sp.ringCap_den2 == 0 ) {
            pkSlot1 = 0;
            pkVal1 = inReal;
         }
         if( sp.ringCap_den3 == 0 ) {
            pkSlot2 = 0;
            pkVal2 = inReal;
         }
         if( sp.ringCap_den4 == 0 ) {
            pkSlot3 = 0;
            pkVal3 = inReal;
         }
         prior = (sp.ringPos_den1 != pkSlot0) ? sp.ring_den1_inReal[sp.ringPos_den1] : pkVal0;
         roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)sp.optInSMA1Period;
         pkSlot4 = ring1_Idx;
         pkVal4 = roc;
         ring1_Idx = ring1_Idx + 1;
         if( ring1_Idx > sp.maxIdx_ring1 ) {
            ring1_Idx = 0;
         }
         total1 -= (ring1_Idx != pkSlot4) ? sp.cb_ring1[ring1_Idx] : pkVal4;
         prior = (sp.ringPos_den2 != pkSlot1) ? sp.ring_den2_inReal[sp.ringPos_den2] : pkVal1;
         roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)sp.optInSMA2Period;
         pkSlot5 = ring2_Idx;
         pkVal5 = roc;
         ring2_Idx = ring2_Idx + 1;
         if( ring2_Idx > sp.maxIdx_ring2 ) {
            ring2_Idx = 0;
         }
         total2 -= (ring2_Idx != pkSlot5) ? sp.cb_ring2[ring2_Idx] : pkVal5;
         prior = (sp.ringPos_den3 != pkSlot2) ? sp.ring_den3_inReal[sp.ringPos_den3] : pkVal2;
         roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)sp.optInSMA3Period;
         pkSlot6 = ring3_Idx;
         pkVal6 = roc;
         ring3_Idx = ring3_Idx + 1;
         if( ring3_Idx > sp.maxIdx_ring3 ) {
            ring3_Idx = 0;
         }
         total3 -= (ring3_Idx != pkSlot6) ? sp.cb_ring3[ring3_Idx] : pkVal6;
         prior = (sp.ringPos_den4 != pkSlot3) ? sp.ring_den4_inReal[sp.ringPos_den4] : pkVal3;
         roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)sp.optInSMA4Period;
         pkSlot7 = ring4_Idx;
         pkVal7 = roc;
         ring4_Idx = ring4_Idx + 1;
         if( ring4_Idx > sp.maxIdx_ring4 ) {
            ring4_Idx = 0;
         }
         total4 -= (ring4_Idx != pkSlot7) ? sp.cb_ring4[ring4_Idx] : pkVal7;
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sig = sigTotal / (double)sp.optInSignalPeriod;
         pkSlot8 = sigRing_Idx;
         pkVal8 = kst;
         sigRing_Idx = sigRing_Idx + 1;
         if( sigRing_Idx > sp.maxIdx_sigRing ) {
            sigRing_Idx = 0;
         }
         sigTotal -= (sigRing_Idx != pkSlot8) ? sp.cb_sigRing[sigRing_Idx] : pkVal8;
         cur_outKST = kst;
         cur_outKSTSignal = sig;
         out.kst = cur_outKST;
         out.kstSignal = cur_outKSTSignal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( KstOut out ) {
         requireArgument("KST value", "out", out);
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
      public KstStream clone() {
         return new KstStream(this);
      }
   }

   /**
    * The outputs of one KST bar, written by the stream into an object the
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
   public static final class KstOut {
      /** Know Sure Thing line. */
      public double kst;
      /** Simple moving average of the line. */
      public double kstSignal;
   }
   private void kstStepImpl( KstStream sp, double inReal )
   {
      double prior = 0.0;
      double roc = 0.0;
      double kst = 0.0;
      double sig = 0.0;
      double rcma1 = 0.0;
      double rcma2 = 0.0;
      double rcma3 = 0.0;
      double rcma4 = 0.0;
      if( sp.ringCap_den1 == 0 ) {
         sp.ring_den1_inReal[0] = inReal;
      }
      if( sp.ringCap_den2 == 0 ) {
         sp.ring_den2_inReal[0] = inReal;
      }
      if( sp.ringCap_den3 == 0 ) {
         sp.ring_den3_inReal[0] = inReal;
      }
      if( sp.ringCap_den4 == 0 ) {
         sp.ring_den4_inReal[0] = inReal;
      }
      prior = sp.ring_den1_inReal[sp.ringPos_den1];
      roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
      sp.total1 += roc;
      rcma1 = sp.total1 / (double)sp.optInSMA1Period;
      sp.cb_ring1[sp.ring1_Idx] = roc;
      sp.ring1_Idx = sp.ring1_Idx + 1;
      if( sp.ring1_Idx > sp.maxIdx_ring1 ) {
         sp.ring1_Idx = 0;
      }
      sp.total1 -= sp.cb_ring1[sp.ring1_Idx];
      prior = sp.ring_den2_inReal[sp.ringPos_den2];
      roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
      sp.total2 += roc;
      rcma2 = sp.total2 / (double)sp.optInSMA2Period;
      sp.cb_ring2[sp.ring2_Idx] = roc;
      sp.ring2_Idx = sp.ring2_Idx + 1;
      if( sp.ring2_Idx > sp.maxIdx_ring2 ) {
         sp.ring2_Idx = 0;
      }
      sp.total2 -= sp.cb_ring2[sp.ring2_Idx];
      prior = sp.ring_den3_inReal[sp.ringPos_den3];
      roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
      sp.total3 += roc;
      rcma3 = sp.total3 / (double)sp.optInSMA3Period;
      sp.cb_ring3[sp.ring3_Idx] = roc;
      sp.ring3_Idx = sp.ring3_Idx + 1;
      if( sp.ring3_Idx > sp.maxIdx_ring3 ) {
         sp.ring3_Idx = 0;
      }
      sp.total3 -= sp.cb_ring3[sp.ring3_Idx];
      prior = sp.ring_den4_inReal[sp.ringPos_den4];
      roc = (prior != 0.0) ? (inReal / prior - 1.0) * 100.0 : 0.0;
      sp.total4 += roc;
      rcma4 = sp.total4 / (double)sp.optInSMA4Period;
      sp.cb_ring4[sp.ring4_Idx] = roc;
      sp.ring4_Idx = sp.ring4_Idx + 1;
      if( sp.ring4_Idx > sp.maxIdx_ring4 ) {
         sp.ring4_Idx = 0;
      }
      sp.total4 -= sp.cb_ring4[sp.ring4_Idx];
      kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
      sp.sigTotal += kst;
      sig = sp.sigTotal / (double)sp.optInSignalPeriod;
      sp.cb_sigRing[sp.sigRing_Idx] = kst;
      sp.sigRing_Idx = sp.sigRing_Idx + 1;
      if( sp.sigRing_Idx > sp.maxIdx_sigRing ) {
         sp.sigRing_Idx = 0;
      }
      sp.sigTotal -= sp.cb_sigRing[sp.sigRing_Idx];
      sp.cur_outKST = kst;
      sp.cur_outKSTSignal = sig;
      sp.ring_den1_inReal[sp.ringPos_den1] = inReal;
      sp.ringPos_den1 = sp.ringPos_den1 + 1;
      if( sp.ringPos_den1 >= sp.ringCap_den1 ) {
         sp.ringPos_den1 = 0;
      }
      sp.ring_den2_inReal[sp.ringPos_den2] = inReal;
      sp.ringPos_den2 = sp.ringPos_den2 + 1;
      if( sp.ringPos_den2 >= sp.ringCap_den2 ) {
         sp.ringPos_den2 = 0;
      }
      sp.ring_den3_inReal[sp.ringPos_den3] = inReal;
      sp.ringPos_den3 = sp.ringPos_den3 + 1;
      if( sp.ringPos_den3 >= sp.ringCap_den3 ) {
         sp.ringPos_den3 = 0;
      }
      sp.ring_den4_inReal[sp.ringPos_den4] = inReal;
      sp.ringPos_den4 = sp.ringPos_den4 + 1;
      if( sp.ringPos_den4 >= sp.ringCap_den4 ) {
         sp.ringPos_den4 = 0;
      }
   }
   private RetCode kstOpenImpl( KstStream sp, double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outKST[], double outKSTSignal[], int outStride )
   {
      int outIdx = 0;
      int inIdx = 0;
      int lookbackTotal = 0;
      int sigStart = 0;
      int den1 = 0;
      int den2 = 0;
      int den3 = 0;
      int den4 = 0;
      double prior = 0;
      double roc = 0;
      double kst = 0;
      double sig = 0;
      double total1 = 0;
      double total2 = 0;
      double total3 = 0;
      double total4 = 0;
      double sigTotal = 0;
      double rcma1 = 0;
      double rcma2 = 0;
      double rcma3 = 0;
      double rcma4 = 0;
      double[] ring1;
      int ring1_Idx = 0;
      int maxIdx_ring1 = (30)-1;
      double[] ring2;
      int ring2_Idx = 0;
      int maxIdx_ring2 = (30)-1;
      double[] ring3;
      int ring3_Idx = 0;
      int maxIdx_ring3 = (30)-1;
      double[] ring4;
      int ring4_Idx = 0;
      int maxIdx_ring4 = (30)-1;
      double[] sigRing;
      int sigRing_Idx = 0;
      int maxIdx_sigRing = (30)-1;
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
      if( optInSMA1Period == Integer.MIN_VALUE ) {
         optInSMA1Period = 10;
      } else if( optInSMA1Period < 1 || optInSMA1Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA2Period == Integer.MIN_VALUE ) {
         optInSMA2Period = 10;
      } else if( optInSMA2Period < 1 || optInSMA2Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA3Period == Integer.MIN_VALUE ) {
         optInSMA3Period = 10;
      } else if( optInSMA3Period < 1 || optInSMA3Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSMA4Period == Integer.MIN_VALUE ) {
         optInSMA4Period = 15;
      } else if( optInSMA4Period < 1 || optInSMA4Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Bit-exact with TA_ROC per leg into TA_SMA, each SMA called at the start
       * its consumer needs: keep every running sum's add/subtract order and the
       * left-to-right weighted sum, or the composite differential stops being
       * exact. Each ring holds its stage's last SMA-period values, so the only
       * inputs read are the current bar and each leg's lagged denominator,
       * both at or after the slot the outputs write: outKST or outKSTSignal
       * may alias inReal.
       */
      lookbackTotal = kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInSMA1Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring1 = new double[optInSMA1Period];
      maxIdx_ring1 = (optInSMA1Period)-1;
      ring1_Idx = 0;
      if( optInSMA2Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring2 = new double[optInSMA2Period];
      maxIdx_ring2 = (optInSMA2Period)-1;
      ring2_Idx = 0;
      if( optInSMA3Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring3 = new double[optInSMA3Period];
      maxIdx_ring3 = (optInSMA3Period)-1;
      ring3_Idx = 0;
      if( optInSMA4Period < 1 ) return RetCode.INTERNAL_ERROR;
      ring4 = new double[optInSMA4Period];
      maxIdx_ring4 = (optInSMA4Period)-1;
      ring4_Idx = 0;
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      sigRing = new double[optInSignalPeriod];
      maxIdx_sigRing = (optInSignalPeriod)-1;
      sigRing_Idx = 0;
      sigStart = startIdx - (optInSignalPeriod - 1);
      sigTotal = 0.0;
      total4 = sigTotal;
      total3 = total4;
      total2 = total3;
      total1 = total2;
      inIdx = sigStart - (optInSMA1Period - 1);
      den1 = inIdx - optInROC1Period;
      while( inIdx < sigStart ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA2Period - 1);
      den2 = inIdx - optInROC2Period;
      while( inIdx < sigStart ) {
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA3Period - 1);
      den3 = inIdx - optInROC3Period;
      while( inIdx < sigStart ) {
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart - (optInSMA4Period - 1);
      den4 = inIdx - optInROC4Period;
      while( inIdx < sigStart ) {
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         inIdx += 1;
      }
      inIdx = sigStart;
      while( inIdx < startIdx ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         inIdx += 1;
      }
      outIdx = 0;
      while( inIdx <= endIdx ) {
         prior = inReal[den1];
         den1 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total1 += roc;
         rcma1 = total1 / (double)optInSMA1Period;
         ring1[ring1_Idx] = roc;
         ring1_Idx++;
         if( ring1_Idx > maxIdx_ring1 ) { ring1_Idx = 0; }
         total1 -= ring1[ring1_Idx];
         prior = inReal[den2];
         den2 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total2 += roc;
         rcma2 = total2 / (double)optInSMA2Period;
         ring2[ring2_Idx] = roc;
         ring2_Idx++;
         if( ring2_Idx > maxIdx_ring2 ) { ring2_Idx = 0; }
         total2 -= ring2[ring2_Idx];
         prior = inReal[den3];
         den3 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total3 += roc;
         rcma3 = total3 / (double)optInSMA3Period;
         ring3[ring3_Idx] = roc;
         ring3_Idx++;
         if( ring3_Idx > maxIdx_ring3 ) { ring3_Idx = 0; }
         total3 -= ring3[ring3_Idx];
         prior = inReal[den4];
         den4 += 1;
         roc = (prior != 0.0) ? (inReal[inIdx] / prior - 1.0) * 100.0 : 0.0;
         total4 += roc;
         rcma4 = total4 / (double)optInSMA4Period;
         ring4[ring4_Idx] = roc;
         ring4_Idx++;
         if( ring4_Idx > maxIdx_ring4 ) { ring4_Idx = 0; }
         total4 -= ring4[ring4_Idx];
         kst = Math.fma(4.0, rcma4, Math.fma(3.0, rcma3, Math.fma(2.0, rcma2, rcma1)));
         sigTotal += kst;
         sig = sigTotal / (double)optInSignalPeriod;
         sigRing[sigRing_Idx] = kst;
         sigRing_Idx++;
         if( sigRing_Idx > maxIdx_sigRing ) { sigRing_Idx = 0; }
         sigTotal -= sigRing[sigRing_Idx];
         outKST[outIdx * outStride] = kst;
         outKSTSignal[outIdx * outStride] = sig;
         outIdx += 1;
         inIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_den1 = inIdx - den1;
      if( cap_den1 < 0 || cap_den1 > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_den1 = (cap_den1 > 0)? cap_den1 : 1;
      double[] capRing_den1_inReal = new double[allocN_den1];
      System.arraycopy(inReal, historyLen - cap_den1, capRing_den1_inReal, 0, cap_den1);
      int cap_den2 = inIdx - den2;
      if( cap_den2 < 0 || cap_den2 > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_den2 = (cap_den2 > 0)? cap_den2 : 1;
      double[] capRing_den2_inReal = new double[allocN_den2];
      System.arraycopy(inReal, historyLen - cap_den2, capRing_den2_inReal, 0, cap_den2);
      int cap_den3 = inIdx - den3;
      if( cap_den3 < 0 || cap_den3 > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_den3 = (cap_den3 > 0)? cap_den3 : 1;
      double[] capRing_den3_inReal = new double[allocN_den3];
      System.arraycopy(inReal, historyLen - cap_den3, capRing_den3_inReal, 0, cap_den3);
      int cap_den4 = inIdx - den4;
      if( cap_den4 < 0 || cap_den4 > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int allocN_den4 = (cap_den4 > 0)? cap_den4 : 1;
      double[] capRing_den4_inReal = new double[allocN_den4];
      System.arraycopy(inReal, historyLen - cap_den4, capRing_den4_inReal, 0, cap_den4);
      int capCb_ring1 = maxIdx_ring1 + 1;
      if( capCb_ring1 > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_ring2 = maxIdx_ring2 + 1;
      if( capCb_ring2 > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_ring3 = maxIdx_ring3 + 1;
      if( capCb_ring3 > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_ring4 = maxIdx_ring4 + 1;
      if( capCb_ring4 > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_sigRing = maxIdx_sigRing + 1;
      if( capCb_sigRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInROC1Period = optInROC1Period;
      sp.optInROC2Period = optInROC2Period;
      sp.optInROC3Period = optInROC3Period;
      sp.optInROC4Period = optInROC4Period;
      sp.optInSMA1Period = optInSMA1Period;
      sp.optInSMA2Period = optInSMA2Period;
      sp.optInSMA3Period = optInSMA3Period;
      sp.optInSMA4Period = optInSMA4Period;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.total1 = total1;
      sp.total2 = total2;
      sp.total3 = total3;
      sp.total4 = total4;
      sp.sigTotal = sigTotal;
      sp.ring1_Idx = ring1_Idx;
      sp.ring2_Idx = ring2_Idx;
      sp.ring3_Idx = ring3_Idx;
      sp.ring4_Idx = ring4_Idx;
      sp.sigRing_Idx = sigRing_Idx;
      sp.maxIdx_ring1 = maxIdx_ring1;
      sp.maxIdx_ring2 = maxIdx_ring2;
      sp.maxIdx_ring3 = maxIdx_ring3;
      sp.maxIdx_ring4 = maxIdx_ring4;
      sp.maxIdx_sigRing = maxIdx_sigRing;
      sp.ringPos_den1 = 0;
      sp.ringCap_den1 = cap_den1;
      sp.ring_den1_inReal = capRing_den1_inReal;
      sp.ringPos_den2 = 0;
      sp.ringCap_den2 = cap_den2;
      sp.ring_den2_inReal = capRing_den2_inReal;
      sp.ringPos_den3 = 0;
      sp.ringCap_den3 = cap_den3;
      sp.ring_den3_inReal = capRing_den3_inReal;
      sp.ringPos_den4 = 0;
      sp.ringCap_den4 = cap_den4;
      sp.ring_den4_inReal = capRing_den4_inReal;
      sp.cbSize_ring1 = capCb_ring1;
      sp.cb_ring1 = ring1;
      sp.cbSize_ring2 = capCb_ring2;
      sp.cb_ring2 = ring2;
      sp.cbSize_ring3 = capCb_ring3;
      sp.cb_ring3 = ring3;
      sp.cbSize_ring4 = capCb_ring4;
      sp.cb_ring4 = ring4;
      sp.cbSize_sigRing = capCb_sigRing;
      sp.cb_sigRing = sigRing;
      sp.cur_outKST = outKST[(outNBElement.value - 1) * outStride];
      sp.cur_outKSTSignal = outKSTSignal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* kstOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KstStream kstOpenAndFillInternal( double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outKST[], double outKSTSignal[] )
   {
      KstStream sp = new KstStream(this);
      RetCode retCode = kstOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KST openAndFill", inReal.length, startIdx, kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod));
      }
      throw streamFailure("KST openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind kstOpen (composition seam). */
   KstStream kstOpenInternal( double inReal[], int startIdx, int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod )
   {
      KstStream sp = new KstStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outKST = new double[1];
      double[] sink_outKSTSignal = new double[1];
      RetCode retCode = kstOpenImpl(sp, inReal, startIdx, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, sink_outKST, sink_outKSTSignal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KST open", inReal.length, startIdx, kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod));
      }
      throw streamFailure("KST open", retCode);
   }
   /**
    * Open a live KST stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#kst} at that bar.
    * <p>The history must hold at least {@code kstLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KstStream kstOpen( double inReal[], int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod )
   {
      requireArgument("KST open", "inReal", inReal);
      requireHistory("KST open", inReal.length);
      return kstOpenInternal(inReal, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod);
   }
   /**
    * {@link Core#kstOpen} that also fills the output array(s) bit-identically
    * to {@link Core#kst} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KstStream#outRange()}.
    */
   public KstStream kstOpenAndFill( double inReal[], int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod, double outKST[], double outKSTSignal[] )
   {
      requireArgument("KST openAndFill", "inReal", inReal);
      requireHistory("KST openAndFill", inReal.length);
      int guardOutLen = openFillCount("KST openAndFill", inReal.length, kstLookback(optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod));
      requireLength("KST openAndFill", "outKST", outKST, guardOutLen);
      requireLength("KST openAndFill", "outKSTSignal", outKSTSignal, guardOutLen);
      if( (Object)outKST == (Object)inReal || (Object)outKSTSignal == (Object)inReal || (Object)outKST == (Object)outKSTSignal ) {
         throw streamFailure("KST openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kstOpenAndFillInternal(inReal, 0, optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod, outBegIdx, outNBElement, outKST, outKSTSignal);
   }
