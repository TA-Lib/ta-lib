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

int kst_lookback(int optInROC1Period, int optInROC2Period, int optInROC3Period, int optInROC4Period, int optInSMA1Period, int optInSMA2Period, int optInSMA3Period, int optInSMA4Period, int optInSignalPeriod)
{
   int legMax, leg;

   legMax = optInROC1Period + optInSMA1Period - 1;
   leg = optInROC2Period + optInSMA2Period - 1;
   if( leg > legMax ) legMax = leg;
   leg = optInROC3Period + optInSMA3Period - 1;
   if( leg > legMax ) legMax = leg;
   leg = optInROC4Period + optInSMA4Period - 1;
   if( leg > legMax ) legMax = leg;
   return legMax + optInSignalPeriod - 1;
}

TA_RetCode kst(int startIdx, int endIdx,
   const double inReal[],
   int optInROC1Period,
   int optInROC2Period,
   int optInROC3Period,
   int optInROC4Period,
   int optInSMA1Period,
   int optInSMA2Period,
   int optInSMA3Period,
   int optInSMA4Period,
   int optInSignalPeriod,
   int *outBegIdx,
   int *outNBElement,
   double outKST[],
   double outKSTSignal[])
{
   int outIdx, inIdx, lookbackTotal, sigStart;
   int den1, den2, den3, den4;
   double prior, roc, kst, sig;
   double total1, total2, total3, total4, sigTotal;
   double rcma1, rcma2, rcma3, rcma4;
   CIRCBUF_PROLOG(ring1,double,30);
   CIRCBUF_PROLOG(ring2,double,30);
   CIRCBUF_PROLOG(ring3,double,30);
   CIRCBUF_PROLOG(ring4,double,30);
   CIRCBUF_PROLOG(sigRing,double,30);

   /* Bit-exact with TA_ROC per leg into TA_SMA, each SMA called at the start
    * its consumer needs: keep every running sum's add/subtract order and the
    * left-to-right weighted sum, or the composite differential stops being
    * exact. Each ring holds its stage's last SMA-period values, so the only
    * inputs read are the current bar and each leg's lagged denominator,
    * both at or after the slot the outputs write: outKST or outKSTSignal
    * may alias inReal.
    */

   lookbackTotal = kst_lookback( optInROC1Period, optInROC2Period, optInROC3Period, optInROC4Period, optInSMA1Period, optInSMA2Period, optInSMA3Period, optInSMA4Period, optInSignalPeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx    = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   CIRCBUF_INIT(ring1,double,optInSMA1Period);
   CIRCBUF_INIT(ring2,double,optInSMA2Period);
   CIRCBUF_INIT(ring3,double,optInSMA3Period);
   CIRCBUF_INIT(ring4,double,optInSMA4Period);
   CIRCBUF_INIT(sigRing,double,optInSignalPeriod);

   sigStart = startIdx - (optInSignalPeriod-1);
   total1 = total2 = total3 = total4 = sigTotal = 0.0;

   inIdx = sigStart - (optInSMA1Period-1);
   den1 = inIdx - optInROC1Period;
   while( inIdx < sigStart )
   {
      prior = inReal[den1];
      den1++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total1 += roc;
      ring1[ring1_Idx] = roc;
      CIRCBUF_NEXT(ring1);
      inIdx++;
   }
   inIdx = sigStart - (optInSMA2Period-1);
   den2 = inIdx - optInROC2Period;
   while( inIdx < sigStart )
   {
      prior = inReal[den2];
      den2++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total2 += roc;
      ring2[ring2_Idx] = roc;
      CIRCBUF_NEXT(ring2);
      inIdx++;
   }
   inIdx = sigStart - (optInSMA3Period-1);
   den3 = inIdx - optInROC3Period;
   while( inIdx < sigStart )
   {
      prior = inReal[den3];
      den3++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total3 += roc;
      ring3[ring3_Idx] = roc;
      CIRCBUF_NEXT(ring3);
      inIdx++;
   }
   inIdx = sigStart - (optInSMA4Period-1);
   den4 = inIdx - optInROC4Period;
   while( inIdx < sigStart )
   {
      prior = inReal[den4];
      den4++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total4 += roc;
      ring4[ring4_Idx] = roc;
      CIRCBUF_NEXT(ring4);
      inIdx++;
   }

   inIdx = sigStart;
   while( inIdx < startIdx )
   {
      prior = inReal[den1];
      den1++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total1 += roc;
      rcma1 = total1 / (double)optInSMA1Period;
      ring1[ring1_Idx] = roc;
      CIRCBUF_NEXT(ring1);
      total1 -= ring1[ring1_Idx];
      prior = inReal[den2];
      den2++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total2 += roc;
      rcma2 = total2 / (double)optInSMA2Period;
      ring2[ring2_Idx] = roc;
      CIRCBUF_NEXT(ring2);
      total2 -= ring2[ring2_Idx];
      prior = inReal[den3];
      den3++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total3 += roc;
      rcma3 = total3 / (double)optInSMA3Period;
      ring3[ring3_Idx] = roc;
      CIRCBUF_NEXT(ring3);
      total3 -= ring3[ring3_Idx];
      prior = inReal[den4];
      den4++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total4 += roc;
      rcma4 = total4 / (double)optInSMA4Period;
      ring4[ring4_Idx] = roc;
      CIRCBUF_NEXT(ring4);
      total4 -= ring4[ring4_Idx];
      kst = rcma1 + 2.0*rcma2 + 3.0*rcma3 + 4.0*rcma4;
      sigTotal += kst;
      sigRing[sigRing_Idx] = kst;
      CIRCBUF_NEXT(sigRing);
      inIdx++;
   }

   outIdx = 0;
   while( inIdx <= endIdx )
   {
      prior = inReal[den1];
      den1++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total1 += roc;
      rcma1 = total1 / (double)optInSMA1Period;
      ring1[ring1_Idx] = roc;
      CIRCBUF_NEXT(ring1);
      total1 -= ring1[ring1_Idx];
      prior = inReal[den2];
      den2++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total2 += roc;
      rcma2 = total2 / (double)optInSMA2Period;
      ring2[ring2_Idx] = roc;
      CIRCBUF_NEXT(ring2);
      total2 -= ring2[ring2_Idx];
      prior = inReal[den3];
      den3++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total3 += roc;
      rcma3 = total3 / (double)optInSMA3Period;
      ring3[ring3_Idx] = roc;
      CIRCBUF_NEXT(ring3);
      total3 -= ring3[ring3_Idx];
      prior = inReal[den4];
      den4++;
      roc = (prior != 0.0) ? ((inReal[inIdx]/prior)-1.0)*100.0 : 0.0;
      total4 += roc;
      rcma4 = total4 / (double)optInSMA4Period;
      ring4[ring4_Idx] = roc;
      CIRCBUF_NEXT(ring4);
      total4 -= ring4[ring4_Idx];
      kst = rcma1 + 2.0*rcma2 + 3.0*rcma3 + 4.0*rcma4;
      sigTotal += kst;
      sig = sigTotal / (double)optInSignalPeriod;
      sigRing[sigRing_Idx] = kst;
      CIRCBUF_NEXT(sigRing);
      sigTotal -= sigRing[sigRing_Idx];
      outKST[outIdx] = kst;
      outKSTSignal[outIdx] = sig;
      outIdx++;
      inIdx++;
   }

   CIRCBUF_DESTROY(ring1);
   CIRCBUF_DESTROY(ring2);
   CIRCBUF_DESTROY(ring3);
   CIRCBUF_DESTROY(ring4);
   CIRCBUF_DESTROY(sigRing);

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
