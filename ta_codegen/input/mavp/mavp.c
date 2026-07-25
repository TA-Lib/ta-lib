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
   int i, lookbackTotal, outputSize, tempInt, curPeriod;
   int lastOccurrence, chainIdx;
   int *localPeriodArray;
   int *periodIndex;
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

   /* Occurrence index for the period grouping below, one pair of entries per
    * representable period: periodIndex[period] is the first output using that
    * period and periodIndex[optInMaxPeriod+1+period] the last one.
    */
   periodIndex = malloc((2*(optInMaxPeriod+1)) * sizeof(int));
   if( periodIndex == NULL )
   {
      free(localOutputArray);
      free(localPeriodArray);
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

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

   /* Read the caller array of period, truncate to min/max, and group the
    * outputs by that truncated period in the same pass. localPeriodArray now
    * holds, for each output, the next output using the same period (-1 ends
    * the chain), so the outputs of one period form a linked list rooted at
    * periodIndex[period]. This replaces the flag-and-rescan below, which
    * walked the rest of the range once per distinct period.
    */
   for( i=0; i <= optInMaxPeriod; i++ )
      periodIndex[i] = -1;

   for( i=0; i < outputSize; i++ )
   {
      tempInt = (int)(inPeriods[startIdx+i]);
      if( tempInt < optInMinPeriod )
         tempInt = optInMinPeriod;
      else if( tempInt > optInMaxPeriod )
         tempInt = optInMaxPeriod;

      if( periodIndex[tempInt] == -1 )
         periodIndex[tempInt] = i;
      else
         localPeriodArray[periodIndex[optInMaxPeriod+1+tempInt]] = i;
      periodIndex[optInMaxPeriod+1+tempInt] = i;
      localPeriodArray[i] = -1;
   }

   /* Process each period actually requested.
    * For each possible period value, the MA is calculated
    * only once, and only as far as the last output using it.
    * The outReal is then fill up for all element with
    * the same period by walking that period's chain.
    */
   for( curPeriod=optInMinPeriod; curPeriod <= optInMaxPeriod; curPeriod++ )
   {
      if( periodIndex[curPeriod] != -1 )
      {
         /* TODO: This portion of the function can be slightly speed
          *       optimized by making the function without unstable period
          *       start their calculation at the first output using this
          *       period instead of startIdx.
          */

         lastOccurrence = periodIndex[optInMaxPeriod+1+curPeriod];

         /* Calculation of the MA required. */
         retCode = ma( startIdx, startIdx+lastOccurrence, inReal,
            curPeriod, optInMAType,
            &localBegIdx,&localNbElement,localOutputArray );

         if( retCode != TA_SUCCESS )
         {
            free(localOutputArray);
            free(localPeriodArray);
            free(periodIndex);
            if( finalIsAllocated ) { free(localFinalArray); }
               *outBegIdx = 0;
            *outNBElement = 0;
            return retCode;
         }

         chainIdx = periodIndex[curPeriod];
         while( chainIdx != -1 )
         {
            localFinalArray[chainIdx] = localOutputArray[chainIdx];
            chainIdx = localPeriodArray[chainIdx];
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
   free(periodIndex);
   if( finalIsAllocated ) { free(localFinalArray); }

      /* Done. Inform the caller of the success. */
   *outBegIdx = startIdx;
   *outNBElement = outputSize;
   return TA_SUCCESS;
}
