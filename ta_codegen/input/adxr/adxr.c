/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  082303 MF     Fix #792298. Remove rounding. Bug reported by AM.
 *  071126 MF,CC  Rewrite the ADX combine as a single cursor: outReal[k] =
 *                (adx[k+(period-1)] + adx[k])/2 (current ADX + ADX lagged by
 *                period-1). Bit-identical to the two-cursor form, and the
 *                streamable-source form (a sub-output lag ring).
 */

int adxr_lookback(int optInTimePeriod)
{
   if( optInTimePeriod > 1 )
      return optInTimePeriod + adx_lookback( optInTimePeriod) - 1;
   else
      return 3;
}

TA_RetCode adxr(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *adx;
   int adxrLookback, outIdx, nbElement;
   TA_RetCode retCode;

   /* Original implementation from Wilder's book was doing some integer
    * rounding in its calculations.
    *
    * This was understandable in the context that at the time the book
    * was written, most user were doing the calculation by hand.
    *
    * For a computer, rounding is unnecessary (and even problematic when inputs
    * are close to 1).
    *
    * TA-Lib does not do the rounding. Still, if you want to reproduce Wilder's examples,
    * you can comment out the following #undef/#define and rebuild the library.
    */
   /* Move up the start index if there is not
    * enough initial data.
    * Always one price bar gets consumed.
    */
   adxrLookback = adxr_lookback( optInTimePeriod );

   if( startIdx < adxrLookback )
      startIdx = adxrLookback;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   adx = malloc((endIdx-startIdx+optInTimePeriod) * sizeof(double));
   if( !adx )
      return TA_ALLOC_ERR;

   /* Compute ADX over a range that starts (period-1) bars earlier, so each
    * ADXR bar can pair the current ADX with the ADX from (period-1) bars ago.
    */
   retCode = adx( startIdx-(optInTimePeriod-1), endIdx,
      inHigh, inLow, inClose,
      optInTimePeriod, outBegIdx, outNBElement, adx );

   if( retCode != TA_SUCCESS )
   {
      free(adx);
      return retCode;
   }

   /* ADXR[k] = (ADX[k] + ADX[k-(period-1)]) / 2. Walking a single cursor over
    * the ADXR output, the current ADX is adx[k+(period-1)] and the lagged one
    * is adx[k]; the ADX range holds (period-1) more elements than the output.
    */
   nbElement = *outNBElement - (optInTimePeriod-1);
   for( outIdx = 0; outIdx < nbElement; outIdx++ )
      outReal[outIdx] = ta_round_pos( (adx[outIdx+(optInTimePeriod-1)] + adx[outIdx]) / 2.0 );

   free(adx);

   *outBegIdx    = startIdx;
   *outNBElement = nbElement;

   return TA_SUCCESS;
}
