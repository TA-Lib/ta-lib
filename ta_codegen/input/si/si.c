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
 *  092726 MF,CC  Initial version (#451).
 */

int si_lookback(double optInLimitMove)
{
   (void)optInLimitMove;
   return 1;
}

TA_RetCode si(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInLimitMove,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx, lookbackTotal;
   double prevClose, prevBody, tempOpen, tempHigh, tempLow, tempClose;
   double body, n, up, dn, rg, k, r, swing;

   lookbackTotal = si_lookback( optInLimitMove );
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   prevClose = inClose[startIdx-1];
   prevBody  = prevClose - inOpen[startIdx-1];
   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      tempOpen  = inOpen[i];
      tempHigh  = inHigh[i];
      tempLow   = inLow[i];
      tempClose = inClose[i];
      body = tempClose - tempOpen;

      n  = tempClose - prevClose;
      n += 0.5*body;
      n += 0.25*prevBody;

      up = fabs(tempHigh - prevClose);
      dn = fabs(tempLow - prevClose);
      rg = fabs(tempHigh - tempLow);
      k  = max(up, dn);

      /* Wilder's three cases of R are this one max, since rg is up + dn or
       * |up - dn|. Keep it branch-free: the case chain mispredicts on most
       * bars of real data. The four backends' max builtins agree only while
       * no operand is -0.0 or NaN, which holds while up and dn are finite.
       */
      r  = max(up - 0.5*dn, dn - 0.5*up);
      r  = max(r, rg);
      r += 0.25*fabs(prevBody);

      if( r == 0.0 )
         swing = 0.0;
      else
         swing = 50.0*(n/r)*(k/optInLimitMove);
      outReal[outIdx] = swing;
      outIdx++;

      prevClose = tempClose;
      prevBody  = body;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
