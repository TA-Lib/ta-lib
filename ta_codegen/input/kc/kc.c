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
   int emaLookback, atrLookback;
   int anchorIdx, emaOffset, atrOffset;
   int tempBegIdx, tempNbElement;
   double tempReal, middle;
   double *tempTP;
   double *tempEMA;
   double *tempATR;

   emaLookback = ema_lookback( optInTimePeriod );
   atrLookback = atr_lookback( optInATRPeriod );
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

   /* Both legs are recursive, and their lookbacks differ. Seeding each one at
    * its OWN lookback would leave the shorter leg cold: it would restart from a
    * fresh seed a few bars before startIdx while the longer leg had been
    * recursing since startIdx-lookbackTotal, so the shorter leg's warm-up error
    * would not shrink as the unstable period grows. Anchor both at
    * startIdx-lookbackTotal instead -- data this function already requires the
    * caller to hold -- and each leg is then warmed by the whole lookback budget,
    * so a single unstable period bounds the residual of both. Without this the
    * codegen range gate measures KC moving ~1.8% across startIdx at unstable
    * period 140, where the convergence envelope allows 0.15%.
    */
   anchorIdx = startIdx - lookbackTotal;
   emaOffset = lookbackTotal - emaLookback;
   atrOffset = lookbackTotal - atrLookback;

   tempTP = malloc((endIdx-anchorIdx+1) * sizeof(double));
   if( !tempTP )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempEMA = malloc((endIdx-anchorIdx-emaLookback+1) * sizeof(double));
   if( !tempEMA )
   {
      free( tempTP );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempATR = malloc((endIdx-anchorIdx-atrLookback+1) * sizeof(double));
   if( !tempATR )
   {
      free( tempTP );
      free( tempEMA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = typprice( anchorIdx, endIdx, inHigh, inLow, inClose,
      &tempBegIdx, &tempNbElement, tempTP );

   if( retCode != TA_SUCCESS )
   {
      free( tempTP );
      free( tempEMA );
      free( tempATR );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* tempTP is bar-anchorIdx relative, so entering the moving average at its own
    * lookback seeds it on the first typical price available.
    */
   retCode = ema( emaLookback, endIdx-anchorIdx, tempTP,
      optInTimePeriod,
      &tempBegIdx, &tempNbElement, tempEMA );

   if( retCode != TA_SUCCESS )
   {
      free( tempTP );
      free( tempEMA );
      free( tempATR );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = atr( anchorIdx+atrLookback, endIdx, inHigh, inLow, inClose,
      optInATRPeriod,
      &tempBegIdx, &tempNbElement, tempATR );

   if( retCode != TA_SUCCESS )
   {
      free( tempTP );
      free( tempEMA );
      free( tempATR );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   *outBegIdx = startIdx;
   *outNBElement = endIdx-startIdx+1;

   /* Each leg begins at its own lookback past the common anchor, so drop the
    * warm-up head of both and pair them index for index from startIdx.
    */
   memmove( outRealMiddleBand, &tempEMA[emaOffset], (*outNBElement) * sizeof(double) );
   memmove( outRealLowerBand, &tempATR[atrOffset], (*outNBElement) * sizeof(double) );

   for( i=0; i < (int)*outNBElement; i++ )
   {
      middle = outRealMiddleBand[i];
      tempReal = outRealLowerBand[i] * optInNbDev;
      outRealUpperBand[i] = middle + tempReal;
      outRealLowerBand[i] = middle - tempReal;
   }

   free( tempTP );
   free( tempEMA );
   free( tempATR );

   return TA_SUCCESS;
}
