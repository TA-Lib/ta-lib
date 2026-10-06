/* Schaff Trend Cycle, created by Doug Schaff (FX-Strategy.com), who released
 * its code in "Releasing the Code to the Schaff Trend Cycle", FXStreet.com,
 * February 15, 2008,
 * https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf
 *
 * List of contributors:
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
 *  092926 MF,CC  Initial version (#478).
 *
 */

int stc_lookback(int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod)
{
   int tempInteger;

   if( optInSlowPeriod < optInFastPeriod )
   {
      tempInteger     = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   /* The MACD line's own lookback, which is what inherits TA_FUNC_UNST_EMA,
    * then one window per stochastic stage. The two 0.5 smoothers seed on
    * their first input, so they add only the unstable period.
    */
   return ema_lookback( optInSlowPeriod )
   + 2 * (optInCyclePeriod - 1)
   + TA_GetUnstablePeriod(TA_FUNC_UNST_STC);
}

TA_RetCode stc(int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod,
   int optInSlowPeriod,
   int optInCyclePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   CIRCBUF_PROLOG(lineRing,double,30);
   CIRCBUF_PROLOG(lineSufHi,double,30);
   CIRCBUF_PROLOG(lineSufLo,double,30);
   CIRCBUF_PROLOG(pfRing,double,30);
   CIRCBUF_PROLOG(pfSufHi,double,30);
   CIRCBUF_PROLOG(pfSufLo,double,30);
   double prevFast, prevSlow, fastK, slowK, tempReal, lineValue;
   double fastBeta, slowBeta;
   double lowest, highest, range, frac1, frac2, pf, pff;
   double lineHi, lineLo, pfHi, pfLo, sufHi, sufLo;
   int i, today, lineStart, outIdx, tempInteger, lookbackTotal, lastIdx;
   int nLine, nPF;

   if( optInSlowPeriod < optInFastPeriod )
   {
      tempInteger     = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   lookbackTotal = stc_lookback( optInFastPeriod, optInSlowPeriod, optInCyclePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;

   fastBeta = ((double)(optInFastPeriod - 1)) / ((double)(optInFastPeriod + 1));
   fastK = 1.0 - fastBeta;
   fastBeta = 1.0 - fastK;
   slowBeta = ((double)(optInSlowPeriod - 1)) / ((double)(optInSlowPeriod + 1));
   slowK = 1.0 - slowBeta;
   slowBeta = 1.0 - slowK;

   /* Rolling extrema, van Herk / Gil-Werman: the window ending in slot j is
    * the current block's prefix extremum joined with the previous block's
    * suffix extremum from slot j+1. The extrema are exact, so the output must
    * stay bit-identical to a full rescan of each window.
    */
   CIRCBUF_INIT(lineRing,double,optInCyclePeriod);
   CIRCBUF_INIT(lineSufHi,double,optInCyclePeriod);
   CIRCBUF_INIT(lineSufLo,double,optInCyclePeriod);
   CIRCBUF_INIT(pfRing,double,optInCyclePeriod);
   CIRCBUF_INIT(pfSufHi,double,optInCyclePeriod);
   CIRCBUF_INIT(pfSufLo,double,optInCyclePeriod);
   lastIdx = optInCyclePeriod - 1;

   /* The line is TA_MACD's: co-terminal SMA seeds, then both EMAs advanced
    * through TA_FUNC_UNST_EMA to lineStart, so that from lineStart on it is
    * TA_EMA(fast) - TA_EMA(slow) bit for bit. The chain is fed from
    * lineStart, not from the EMA seed: TA_FUNC_UNST_EMA then reaches only the
    * line, while TA_FUNC_UNST_STC moves the whole chain back and so warms the
    * line and both smoothers.
    */
   lineStart = startIdx - (lookbackTotal - ema_lookback( optInSlowPeriod ));

   today = startIdx - lookbackTotal;
   tempReal = 0.0;
   i = optInSlowPeriod - optInFastPeriod;
   while( i-- > 0 )
      tempReal += inReal[today++];

   prevFast = 0.0;
   i = optInFastPeriod;
   while( i-- > 0 )
   {
      prevFast += inReal[today];
      tempReal += inReal[today++];
   }
   prevSlow = tempReal / optInSlowPeriod;
   prevFast = prevFast / optInFastPeriod;

   while( today <= lineStart )
   {
      tempReal = inReal[today++];
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
   }

   /* A zero range holds the previous fraction (0.0 before any), and the test
    * is exact: in a sustained trend PF saturates at 100 and the second
    * range reaches exactly 0 while the output must stay at 100.
    */
   frac1  = 0.0;
   frac2  = 0.0;
   pf     = 0.0;
   pff    = 0.0;
   pfHi   = 0.0;
   pfLo   = 0.0;
   nPF    = 0;

   lineValue = prevFast - prevSlow;
   lineRing[lineRing_Idx] = lineValue;
   CIRCBUF_NEXT(lineRing);
   lineHi = lineValue;
   lineLo = lineValue;
   nLine  = 1;

   /* Warm-up, through startIdx inclusive. Each stage starts once its
    * window is full, and each smoother is seeded on its first input.
    */
   while( today <= startIdx )
   {
      tempReal = inReal[today];
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      lineValue = prevFast - prevSlow;
      nLine = nLine + 1;
      lineRing[lineRing_Idx] = lineValue;
      if( lineRing_Idx == 0 )
      {
         lineHi = lineValue;
         lineLo = lineValue;
      }
      else
      {
         if( lineValue > lineHi )
            lineHi = lineValue;
         if( lineValue < lineLo )
            lineLo = lineValue;
      }
      highest = lineHi;
      lowest  = lineLo;
      if( (nLine >= optInCyclePeriod) && (lineRing_Idx < lastIdx) )
      {
         tempReal = lineSufHi[lineRing_Idx + 1];
         if( tempReal > highest )
            highest = tempReal;
         tempReal = lineSufLo[lineRing_Idx + 1];
         if( tempReal < lowest )
            lowest = tempReal;
      }
      CIRCBUF_NEXT(lineRing);
      if( lineRing_Idx == 0 )
      {
         sufHi = lineRing[lastIdx];
         sufLo = sufHi;
         lineSufHi[lastIdx] = sufHi;
         lineSufLo[lastIdx] = sufLo;
         i = lastIdx;
         while( i > 0 )
         {
            i--;
            tempReal = lineRing[i];
            if( tempReal > sufHi )
               sufHi = tempReal;
            if( tempReal < sufLo )
               sufLo = tempReal;
            lineSufHi[i] = sufHi;
            lineSufLo[i] = sufLo;
         }
      }

      if( nLine >= optInCyclePeriod )
      {
         range = highest - lowest;
         if( range > 0.0 )
            frac1 = ((lineValue - lowest) / range) * 100.0;

         if( nPF == 0 )
            pf = frac1;
         else
            pf = pf + (0.5 * (frac1 - pf));
         nPF = nPF + 1;
         pfRing[pfRing_Idx] = pf;
         if( pfRing_Idx == 0 )
         {
            pfHi = pf;
            pfLo = pf;
         }
         else
         {
            if( pf > pfHi )
               pfHi = pf;
            if( pf < pfLo )
               pfLo = pf;
         }
         highest = pfHi;
         lowest  = pfLo;
         if( (nPF >= optInCyclePeriod) && (pfRing_Idx < lastIdx) )
         {
            tempReal = pfSufHi[pfRing_Idx + 1];
            if( tempReal > highest )
               highest = tempReal;
            tempReal = pfSufLo[pfRing_Idx + 1];
            if( tempReal < lowest )
               lowest = tempReal;
         }
         CIRCBUF_NEXT(pfRing);
         if( pfRing_Idx == 0 )
         {
            sufHi = pfRing[lastIdx];
            sufLo = sufHi;
            pfSufHi[lastIdx] = sufHi;
            pfSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 )
            {
               i--;
               tempReal = pfRing[i];
               if( tempReal > sufHi )
                  sufHi = tempReal;
               if( tempReal < sufLo )
                  sufLo = tempReal;
               pfSufHi[i] = sufHi;
               pfSufLo[i] = sufLo;
            }
         }

         if( nPF >= optInCyclePeriod )
         {
            range = highest - lowest;
            if( range > 0.0 )
               frac2 = ((pf - lowest) / range) * 100.0;

            if( nPF == optInCyclePeriod )
               pff = frac2;
            else
               pff = pff + (0.5 * (frac2 - pff));
         }
      }

      today = today + 1;
   }

   outReal[0] = pff;
   outIdx = 1;

   while( today <= endIdx )
   {
      tempReal = inReal[today];
      prevFast = fastK * tempReal + fastBeta * prevFast;
      prevSlow = slowK * tempReal + slowBeta * prevSlow;
      lineValue = prevFast - prevSlow;
      lineRing[lineRing_Idx] = lineValue;
      if( lineRing_Idx == 0 )
      {
         lineHi = lineValue;
         lineLo = lineValue;
      }
      else
      {
         if( lineValue > lineHi )
            lineHi = lineValue;
         if( lineValue < lineLo )
            lineLo = lineValue;
      }
      highest = lineHi;
      lowest  = lineLo;
      if( lineRing_Idx < lastIdx )
      {
         tempReal = lineSufHi[lineRing_Idx + 1];
         if( tempReal > highest )
            highest = tempReal;
         tempReal = lineSufLo[lineRing_Idx + 1];
         if( tempReal < lowest )
            lowest = tempReal;
      }
      CIRCBUF_NEXT(lineRing);
      if( lineRing_Idx == 0 )
      {
         sufHi = lineRing[lastIdx];
         sufLo = sufHi;
         lineSufHi[lastIdx] = sufHi;
         lineSufLo[lastIdx] = sufLo;
         i = lastIdx;
         while( i > 0 )
         {
            i--;
            tempReal = lineRing[i];
            if( tempReal > sufHi )
               sufHi = tempReal;
            if( tempReal < sufLo )
               sufLo = tempReal;
            lineSufHi[i] = sufHi;
            lineSufLo[i] = sufLo;
         }
      }

      range = highest - lowest;
      if( range > 0.0 )
         frac1 = ((lineValue - lowest) / range) * 100.0;
      pf = pf + (0.5 * (frac1 - pf));
      pfRing[pfRing_Idx] = pf;
      if( pfRing_Idx == 0 )
      {
         pfHi = pf;
         pfLo = pf;
      }
      else
      {
         if( pf > pfHi )
            pfHi = pf;
         if( pf < pfLo )
            pfLo = pf;
      }
      highest = pfHi;
      lowest  = pfLo;
      if( pfRing_Idx < lastIdx )
      {
         tempReal = pfSufHi[pfRing_Idx + 1];
         if( tempReal > highest )
            highest = tempReal;
         tempReal = pfSufLo[pfRing_Idx + 1];
         if( tempReal < lowest )
            lowest = tempReal;
      }
      CIRCBUF_NEXT(pfRing);
      if( pfRing_Idx == 0 )
      {
         sufHi = pfRing[lastIdx];
         sufLo = sufHi;
         pfSufHi[lastIdx] = sufHi;
         pfSufLo[lastIdx] = sufLo;
         i = lastIdx;
         while( i > 0 )
         {
            i--;
            tempReal = pfRing[i];
            if( tempReal > sufHi )
               sufHi = tempReal;
            if( tempReal < sufLo )
               sufLo = tempReal;
            pfSufHi[i] = sufHi;
            pfSufLo[i] = sufLo;
         }
      }

      range = highest - lowest;
      if( range > 0.0 )
         frac2 = ((pf - lowest) / range) * 100.0;
      pff = pff + (0.5 * (frac2 - pff));

      outReal[outIdx++] = pff;
      today = today + 1;
   }

   CIRCBUF_DESTROY(lineRing);
   CIRCBUF_DESTROY(lineSufHi);
   CIRCBUF_DESTROY(lineSufLo);
   CIRCBUF_DESTROY(pfRing);
   CIRCBUF_DESTROY(pfSufHi);
   CIRCBUF_DESTROY(pfSufLo);

   *outNBElement = outIdx;

   return TA_SUCCESS;
}
