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
 *  092926 MF,CC  First version (issue #471).
 */

int mcgd_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1 + TA_GetUnstablePeriod(TA_FUNC_UNST_MCGD);
}

TA_RetCode mcgd(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, today, lookbackTotal;
   int nbMCGD;

   double prevMD, tempMD, tempReal, ratio, period;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = mcgd_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   period = (double)optInTimePeriod;

   /* Keep the denominator free of pow(): a transcendental would break the
    * four languages' bit-identity.
    *
    * A step whose value is not finite (a zero price, a zero line meeting a
    * zero price, or a price so small against the line that the step
    * overflows) holds the previous value, since every later bar reads it.
    */
   today = startIdx - lookbackTotal;
   prevMD = inReal[today];
   today++;

   i = lookbackTotal;
   while( i != 0 )
   {
      tempReal = inReal[today];
      ratio  = tempReal / prevMD;
      tempMD = prevMD + (tempReal - prevMD) / (period * ((ratio*ratio)*(ratio*ratio)));
      if( IS_FINITE(tempMD) )
         prevMD = tempMD;
      today++;
      i--;
   }

   outIdx = 1;
   outReal[0] = prevMD;

   nbMCGD = (endIdx - startIdx)+1;

   while( --nbMCGD != 0 )
   {
      tempReal = inReal[today];
      ratio  = tempReal / prevMD;
      tempMD = prevMD + (tempReal - prevMD) / (period * ((ratio*ratio)*(ratio*ratio)));
      if( IS_FINITE(tempMD) )
         prevMD = tempMD;
      outReal[outIdx++] = prevMD;
      today++;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
