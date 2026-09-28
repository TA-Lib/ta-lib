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

int asi_lookback(double optInLimitMove)
{
   (void)optInLimitMove;
   return 0;
}

TA_RetCode asi(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   double optInLimitMove,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, outIdx;
   double prevClose, prevBody, tempOpen, tempHigh, tempLow, tempClose;
   double body, n, up, dn, rg, k, r, swing, sum;

   /* SI's per-bar statements, kept identical: the regression test holds this
    * function bit for bit to CUMSUM over SI. Bar startIdx is read before the
    * anchor store, which may overwrite it when outReal aliases an input.
    */
   prevClose = inClose[startIdx];
   prevBody  = prevClose - inOpen[startIdx];
   sum = 0.0;
   outReal[0] = sum;
   outIdx = 1;
   for( i = startIdx+1; i <= endIdx; i++ )
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

      r  = max(up - 0.5*dn, dn - 0.5*up);
      r  = max(r, rg);
      r += 0.25*fabs(prevBody);

      if( r == 0.0 )
         swing = 0.0;
      else
         swing = 50.0*(n/r)*(k/optInLimitMove);
      sum += swing;
      outReal[outIdx] = sum;
      outIdx++;

      prevClose = tempClose;
      prevBody  = body;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
