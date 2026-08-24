/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  112605 MF    Initial coding.
 *  082326 MF,CC Fix #253. Test the bar range exactly instead of against the
 *               fixed TA_IS_ZERO_OR_NEG band, which zeroed the output for any
 *               instrument quoted small enough to fall under it.
 *
 */

int bop_lookback(void)
{
   return 0;
}

TA_RetCode bop(int startIdx, int endIdx,
   const double inOpen[],
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;
   double tempReal;

   /* BOP = (Close - Open)/(High - Low) */

   outIdx = 0;

   for( i=startIdx; i <= endIdx; i++ )
   {
      /* BOP is a fraction of the bar's own range, so it is scale-free and the
       * divisor only has to be positive. An exact test, not the fixed
       * TA_IS_ZERO_OR_NEG band it used to be: the range carries the quote unit,
       * and that band zeroed the output for any instrument quoted below it
       * (issue #253).
       */
      tempReal = inHigh[i]-inLow[i];
      if( tempReal <= 0.0 )
         outReal[outIdx++] = 0.0;
      else
         outReal[outIdx++] = (inClose[i] - inOpen[i])/tempReal;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
