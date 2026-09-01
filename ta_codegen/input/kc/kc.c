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
 *  083126 MF,CC  First version (issue #273).
 */

int kc_lookback(int optInTimePeriod, int optInATRPeriod, double optInNbDev)
{
   int emaLookback;
   int atrLookback;

   (void)optInNbDev;

   /* A band value needs BOTH the centre line and the ATR at the same bar, so the
    * first valid output is the later of the two lookbacks. Each term is exactly
    * the lookback of the function it comes from and is never restated here, which
    * is what makes KC inherit TA_FUNC_UNST_EMA and TA_FUNC_UNST_ATR from its two
    * callees. Reporting the honest max keeps outBegIdx == lookback (issue #99),
    * which streaming's Open depends on.
    */
   emaLookback = ema_lookback( optInTimePeriod );
   atrLookback = atr_lookback( optInATRPeriod );
   return emaLookback > atrLookback ? emaLookback : atrLookback;
}

TA_RetCode kc(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   int optInTimePeriod,
   int optInATRPeriod,
   double optInNbDev,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   TA_RetCode retCode;
   int i;
   int lookbackTotal;
   int emaLookback;
   int tpStartIdx;
   int tempBegIdx, tempNbElement;
   double tempReal, middle;
   double *tempTP;
   double *tempATR;

   emaLookback = ema_lookback( optInTimePeriod );
   lookbackTotal = kc_lookback( optInTimePeriod, optInATRPeriod, optInNbDev );

   /* Nothing to produce: the range is shorter than the lookback. Return before
    * touching anything, so that a caller-supplied input which stops short of
    * endIdx is never read past its end.
    */
   if( lookbackTotal > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Each leg is entered at its OWN lookback, so each is the shipped function
    * over this range and nothing here over-warms the shorter one: a caller who
    * wants the band converged sets TA_FUNC_UNST_ATR, exactly as they would when
    * calling TA_ATR directly. The typical price is a derived input, so it is
    * materialized only over the window the moving average reads.
    */
   tpStartIdx = startIdx - emaLookback;

   tempTP = malloc((endIdx-tpStartIdx+1) * sizeof(double));
   if( !tempTP )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempATR = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempATR )
   {
      free( tempTP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = typprice( tpStartIdx, endIdx, inHigh, inLow, inClose,
      &tempBegIdx, &tempNbElement, tempTP );

   if( retCode != TA_SUCCESS )
   {
      free( tempTP );
      free( tempATR );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* The ATR consumes the price inputs before the moving average below writes
    * the middle band, which may be aliased onto one of them.
    */
   retCode = atr( startIdx, endIdx, inHigh, inLow, inClose,
      optInATRPeriod,
      &tempBegIdx, &tempNbElement, tempATR );

   if( retCode != TA_SUCCESS )
   {
      free( tempTP );
      free( tempATR );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* tempTP is bar-tpStartIdx relative, so entering the moving average at its
    * own lookback puts its first output on startIdx, where the ATR's already is.
    */
   retCode = ema( emaLookback, endIdx-tpStartIdx, tempTP,
      optInTimePeriod,
      outBegIdx, outNBElement, outRealMiddleBand );

   if( (retCode != TA_SUCCESS) || ((int)*outNBElement == 0) )
   {
      free( tempTP );
      free( tempATR );
      *outNBElement = 0;
      return retCode;
   }

   *outBegIdx = startIdx;

   for( i=0; i < (int)*outNBElement; i++ )
   {
      middle = outRealMiddleBand[i];
      tempReal = tempATR[i] * optInNbDev;
      outRealUpperBand[i] = middle + tempReal;
      outRealLowerBand[i] = middle - tempReal;
   }

   free( tempTP );
   free( tempATR );

   return TA_SUCCESS;
}
