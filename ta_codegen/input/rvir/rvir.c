/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091126 KL     First version (proposal-drafts issue #68).
 */

int rvir_lookback(int optInTimePeriod, int optInStdDevPeriod)
{
   /* Both legs are the shipped TA_RVI at the same parameters, so they warm on
    * the same bar and there is no max() to take. Stated as the callee's
    * lookback rather than restating the arithmetic, which is what makes this
    * function inherit TA_FUNC_UNST_RVI the way KC inherits its two (kc.c).
    */
   return rvi_lookback( optInTimePeriod, optInStdDevPeriod );
}

TA_RetCode rvir(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   int optInTimePeriod,
   int optInStdDevPeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *tempHigh;
   TA_RetCode retCode;
   int i, lookbackTotal, tempBegIdx, tempNbElement;

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = rvir_lookback( optInTimePeriod, optInStdDevPeriod );

   /* Nothing is allocated and no input is read when the range cannot produce a
    * value, so a caller-supplied input that stops short of endIdx is never
    * read past its end (kc.c).
    */
   if( lookbackTotal > endIdx )
      return TA_SUCCESS;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   if( startIdx > endIdx )
      return TA_SUCCESS;

   /* Each leg is the shipped TA_RVI over this range, entered at the same
    * startIdx: same function, same parameters, same lookback, so the two
    * outputs are aligned bar for bar with no anchor to reconcile.
    *
    * Two calls rather than one fused loop. The fused body is expressible --
    * two copies of TA_RVI's state driven by one bar cursor -- but it carries
    * two window-start cursors, and the streaming analyzer accepts exactly one
    * ("extrema automaton: expected exactly one window-start variable"), so
    * fusing would cost this function the streaming tier. Composing keeps it,
    * for the reason KC's legs do: each leg streams as itself.
    */
   tempHigh = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempHigh )
      return TA_ALLOC_ERR;

   /* Either input may be aliased onto outReal. Both legs are safe against that
    * for TA_RVI's own reason -- its write index trails its read index by the
    * lookback and never overtakes it -- so the order of the two calls is an
    * implementation detail, not what makes the aliasing safe. Swapping them
    * leaves every value identical, which is why the aliasing test earns its
    * keep against the scratch buffer's extent rather than against this order.
    */
   retCode = rvi( startIdx, endIdx, inHigh,
      optInTimePeriod, optInStdDevPeriod,
      &tempBegIdx, &tempNbElement, tempHigh );

   if( retCode != TA_SUCCESS )
   {
      free( tempHigh );
      return retCode;
   }

   retCode = rvi( startIdx, endIdx, inLow,
      optInTimePeriod, optInStdDevPeriod,
      outBegIdx, outNBElement, outReal );

   if( retCode != TA_SUCCESS )
   {
      free( tempHigh );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
    * 50 before the average is taken, so a tie in one series never drags the
    * other. Scaling by one half is exact, so the spelling of the average is
    * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
    * 0.5*a + 0.5*b.
    */
   for( i=0; i < (*outNBElement); i++ )
      outReal[i] = 0.5 * ( tempHigh[i] + outReal[i] );

   free( tempHigh );

   return TA_SUCCESS;
}
