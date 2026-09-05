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
 *  090526 MF,CC  First version (issue #373).
 */

int ha_lookback(void)
{
   return TA_GetUnstablePeriod(TA_FUNC_UNST_HA);
}

TA_RetCode ha(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outHAOpen[],
   double outHAHigh[],
   double outHALow[],
   double outHAClose[])
{
   int i, outIdx, today, lookbackTotal;
   double haOpen, haClose, haHigh, haLow;
   double tempOpen, tempHigh, tempLow, tempClose;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = ha_lookback();

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* The summation order ((O+H)+L)+C is the bit-exactness contract with every
    * external implementation of this indicator. TA_AVGPRICE is documented as
    * the same quantity but sums ((H+L)+C)+O, which is a different double on
    * some bars; the two are deliberately not unified.
    */
   today = startIdx - lookbackTotal;
   haOpen = (inOpen[today] + inClose[today]) / 2.0;
   haClose = (((inOpen[today] + inHigh[today]) + inLow[today]) + inClose[today]) / 4.0;
   today++;

   /* Skip the unstable period. */
   i = lookbackTotal;
   while( i != 0 )
   {
      /* haOpen consumes the PREVIOUS candle on both sides of the midpoint, so
       * it must advance before haClose is overwritten. Swapping these two
       * still yields a plausible smoothed series.
       */
      haOpen = (haOpen + haClose) / 2.0;
      haClose = (((inOpen[today] + inHigh[today]) + inLow[today]) + inClose[today]) / 4.0;
      today++;
      i--;
   }

   tempHigh = inHigh[startIdx];
   tempLow = inLow[startIdx];

   /* The three-way extremum is spelled with plain comparisons, never the
    * max/min builtins: C's macros return their SECOND operand on a tie where
    * Rust, Java and .NET return the negative zero, so a bar of signed zeros
    * emits different bytes per backend and the cross-language gate compares
    * bytes. Measured on (O,H,L,C) = (-0.0, +0.0, -0.0, -0.0).
    */
   haHigh = tempHigh;
   if( haOpen > haHigh )
      haHigh = haOpen;
   if( haClose > haHigh )
      haHigh = haClose;

   haLow = tempLow;
   if( haOpen < haLow )
      haLow = haOpen;
   if( haClose < haLow )
      haLow = haClose;

   outHAOpen[0] = haOpen;
   outHAHigh[0] = haHigh;
   outHALow[0] = haLow;
   outHAClose[0] = haClose;
   outIdx = 1;

   while( today <= endIdx )
   {
      /* Every price of the bar is read into a local before any output is
       * written, which is what lets an output alias any input: at startIdx 0
       * the store lands on the very slot the high and low were just read from.
       */
      tempOpen = inOpen[today];
      tempHigh = inHigh[today];
      tempLow = inLow[today];
      tempClose = inClose[today];

      haOpen = (haOpen + haClose) / 2.0;
      haClose = (((tempOpen + tempHigh) + tempLow) + tempClose) / 4.0;

      haHigh = tempHigh;
      if( haOpen > haHigh )
         haHigh = haOpen;
      if( haClose > haHigh )
         haHigh = haClose;

      haLow = tempLow;
      if( haOpen < haLow )
         haLow = haOpen;
      if( haClose < haLow )
         haLow = haClose;

      outHAOpen[outIdx] = haOpen;
      outHAHigh[outIdx] = haHigh;
      outHALow[outIdx] = haLow;
      outHAClose[outIdx] = haClose;
      outIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
