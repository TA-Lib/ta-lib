/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  021807 MF     Initial Version
 *  072026 MF,CC  Fix #130. Stage results locally so in-place (outReal==inReal)
 *                calls no longer corrupt the input the ma() passes re-read.
 */

int mavp_lookback(int optInMinPeriod, int optInMaxPeriod, TA_MAType optInMAType)
{
   return ma_lookback(optInMaxPeriod, optInMAType);
}

TA_RetCode mavp(int startIdx, int endIdx,
   const double inReal[],
   const double inPeriods[],
   int optInMinPeriod,
   int optInMaxPeriod,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int i, j, lookbackTotal, outputSize, tempInt, curPeriod;
   int *localPeriodArray;
   double *localOutputArray;
   double *localFinalArray;
   int finalIsAllocated;
   int localBegIdx;
   int localNbElement;
   TA_RetCode retCode;

   /* An inverted period window (min above max) is an invalid parameter
    * combination: the per-bar clamp below would push a period above
    * optInMaxPeriod, exceeding the lookback and reading uninitialized
    * results. Reject it cleanly instead of returning garbage.
    */
   if( optInMinPeriod > optInMaxPeriod )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_BAD_PARAM;
   }

   /* Identify the minimum number of price bar needed
    * to calculate at least one output.
    */
   lookbackTotal = ma_lookback(optInMaxPeriod,optInMAType);

   /* Move up the start index if there is not
    * enough initial data.
    */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Calculate exact output size */
   if( lookbackTotal > startIdx )
      tempInt = lookbackTotal;
   else
      tempInt = startIdx;
   if( tempInt > endIdx )
   {
      /* No output */
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   outputSize = endIdx - tempInt + 1;

   /* Allocate intermediate local buffer. */
   double *localOutputArray = malloc((outputSize) * sizeof(double));
   int *localPeriodArray = malloc((outputSize) * sizeof(int));

   /* In-place defence (issue #130): each ma() pass below re-reads inReal over
    * the full range, so with outReal==inReal the results are staged in a
    * scratch buffer and copied once at the end. A regular call writes
    * straight to outReal and skips both the allocation and the copy. */
   finalIsAllocated = 0;
   if( outReal == inReal )
   {
      finalIsAllocated = 1;
      localFinalArray = malloc((outputSize) * sizeof(double));
   }
   else
   {
      localFinalArray = outReal;
   }

   /* Copy caller array of period into local buffer.
    * At the same time, truncate to min/max.
    */
   for( i=0; i < outputSize; i++ )
   {
      tempInt = (int)(inPeriods[startIdx+i]);
      if( tempInt < optInMinPeriod )
         tempInt = optInMinPeriod;
      else if( tempInt > optInMaxPeriod )
         tempInt = optInMaxPeriod;
      localPeriodArray[i] = tempInt;
   }

   /* Process each element of the input.
    * For each possible period value, the MA is calculated
    * only once.
    * The outReal is then fill up for all element with
    * the same period.
    * A local flag (value 0) is set in localPeriodArray
    * to avoid doing a second time the same calculation.
    */
   for( i=0; i < outputSize; i++ )
   {
      curPeriod = localPeriodArray[i];
      if( curPeriod != 0 )
      {
         /* TODO: This portion of the function can be slightly speed
          *       optimized by making the function without unstable period
          *       start their calculation at 'startIdx+i' instead of startIdx.
          */

         /* Calculation of the MA required. */
         retCode = ma( startIdx, endIdx, inReal,
            curPeriod, optInMAType,
            &localBegIdx,&localNbElement,localOutputArray );

         if( retCode != TA_SUCCESS )
         {
            free(localOutputArray);
            free(localPeriodArray);
            if( finalIsAllocated ) { free(localFinalArray); }
               *outBegIdx = 0;
            *outNBElement = 0;
            return retCode;
         }

         localFinalArray[i] = localOutputArray[i];
         for( j=i+1; j < outputSize; j++ )
         {
            if( localPeriodArray[j] == curPeriod )
            {
               localPeriodArray[j] = 0; /* Flag to avoid recalculation */
               localFinalArray[j] = localOutputArray[j];
            }
         }
      }
   }

   /* Pointer-inequality guard, not finalIsAllocated: in backends where the
    * scratch election materializes as a copy (Rust), the copy-back must
    * always run; in C/Java the non-aliased self-copy is skipped. */
   if( localFinalArray != outReal )
   {
      memcpy(outReal, localFinalArray, outputSize * sizeof(double));
   }

   free(localOutputArray);
   free(localPeriodArray);
   if( finalIsAllocated ) { free(localFinalArray); }

      /* Done. Inform the caller of the success. */
   *outBegIdx = startIdx;
   *outNBElement = outputSize;
   return TA_SUCCESS;
}
