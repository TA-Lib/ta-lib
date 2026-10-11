/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#487).
 *  101126 MF,CC  The extension step is a select; the pivot index is derived (#546).
 */

int zigzag_lookback( double optInSensitivity, int optInMinTrendLength )
{
   (void)optInSensitivity;

   /* The first bar at which a reversal off the seed can fire. The gate counts
    * from the pivot's OWN bar and the seed is a pivot like any other, so the
    * earliest reversal is optInMinTrendLength bars after the seed, which sits
    * that many bars before the first output.
    *
    * Independent of the sensitivity: the threshold decides WHETHER a reversal
    * fires, never how early it may.
    */
   return optInMinTrendLength;
}

TA_RetCode zigzag(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   double optInSensitivity,
   int optInMinTrendLength,
   int *outBegIdx, int *outNBElement,
   double outZigZag[],
   int outTrend[],
   int outPivotIdx[])
{
   int lookbackTotal, today, outIdx, trend, barsSincePivot, barIdx, isExt;
   double s, up, dn, pivot;

   lookbackTotal = zigzag_lookback( optInSensitivity, optInMinTrendLength );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Percent in, fraction inside. Each factor is rounded once, here, rather
    * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
    * the expression order the oracle uses.
    */
   s = optInSensitivity / 100.0;
   up = 1.0 + s;
   dn = 1.0 - s;

   /* The seed: a down leg whose extreme is the low of the bar
    * optInMinTrendLength before the first output. Because the gate counts the
    * seed bar like every other pivot, the first reversal can fire exactly at
    * the first output bar, not one later.
    */
   today = startIdx - lookbackTotal;
   trend = -1;
   pivot = inLow[today];
   barIdx = today;
   barsSincePivot = 0;
   today = today + 1;

   outIdx = 0;
   while( today <= endIdx )
   {
      /* The pivot's bar is barIdx - barsSincePivot. Both are carried: a stream
       * has no cursor to read a bar number from.
       */
      barsSincePivot = barsSincePivot + 1;
      barIdx = barIdx + 1;

      /* Reversal is tested BEFORE extension. On an outside bar that both makes
       * a new extreme and clears the threshold, the leg reverses and the old
       * pivot stays where it was; testing extension first would move the pivot
       * and lose the reversal, which is a whole leg of difference rather than a
       * rounding.
       */
      if( trend == -1 )
      {
         if( inHigh[today] >= pivot*up && barsSincePivot >= optInMinTrendLength )
         {
            trend = 1;
            pivot = inHigh[today];
            barsSincePivot = 0;
         }
         else
         {
            /* Ties extend: at an equal price the pivot moves to the later bar,
             * which only outPivotIdx shows.
             *
             * A select, not a branch: on moving prices the extension is taken
             * on a share of the bars no predictor follows.
             */
            isExt = ( inLow[today] <= pivot ) ? 1 : 0;
            pivot = ( pivot < inLow[today] ) ? pivot : inLow[today];
            barsSincePivot = barsSincePivot - isExt * barsSincePivot;
         }
      }
      else
      {
         if( inLow[today] <= pivot*dn && barsSincePivot >= optInMinTrendLength )
         {
            trend = -1;
            pivot = inLow[today];
            barsSincePivot = 0;
         }
         else
         {
            isExt = ( inHigh[today] >= pivot ) ? 1 : 0;
            pivot = ( pivot > inHigh[today] ) ? pivot : inHigh[today];
            barsSincePivot = barsSincePivot - isExt * barsSincePivot;
         }
      }

      if( today >= startIdx )
      {
         outZigZag[outIdx] = pivot;
         outTrend[outIdx] = trend;
         outPivotIdx[outIdx] = barIdx - barsSincePivot;
         outIdx = outIdx + 1;
      }

      today = today + 1;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
