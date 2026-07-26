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
 *  072626 MF,CC  #143. Group outputs by clamped period (counting sort) and
 *                bound each ma() pass at its period's last use.
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
   int firstOccurrence, lastOccurrence, bucketStart, bucketEnd;
   int minUsed, maxUsed;
   int *localPeriodArray;
   int *sortedIdx;
   int *bucketOfs;
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

   /* Output indices grouped by clamped period (counting sort below). */
   sortedIdx = malloc((outputSize) * sizeof(int));
   if( sortedIdx == NULL )
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

   /* Read the caller array of period, truncate to min/max, and track the
    * range of periods actually used so all later work is sized by the data,
    * not by optInMaxPeriod. The floor at 1 (and on minUsed's start value) is
    * inert through the guarded API (optInMinPeriod >= 1); it keeps an
    * off-contract unguarded call with a period below 1 from indexing the
    * occurrence tables out of range.
    */
   minUsed = optInMaxPeriod;
   if( minUsed < 1 )
      minUsed = 1;
   maxUsed = 1;
   for( i=0; i < outputSize; i++ )
   {
      tempInt = (int)(inPeriods[startIdx+i]);
      if( tempInt < optInMinPeriod )
         tempInt = optInMinPeriod;
      else if( tempInt > optInMaxPeriod )
         tempInt = optInMaxPeriod;
      if( tempInt < 1 )
         tempInt = 1;
      localPeriodArray[i] = tempInt;
      if( tempInt < minUsed )
         minUsed = tempInt;
      if( tempInt > maxUsed )
         maxUsed = tempInt;
   }

   /* Per-period bucket cursor for the counting sort; indexed by absolute
    * period value, so sized by the largest period actually used.
    */
   bucketOfs = malloc((maxUsed+2) * sizeof(int));
   if( bucketOfs == NULL )
   {
      free(localOutputArray);
      free(localPeriodArray);
      free(sortedIdx);
      if( finalIsAllocated ) { free(localFinalArray); }
         *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   if( minUsed == maxUsed )
   {
      /* Single distinct period: one MA pass, written straight into the
       * destination buffer. Nothing to group or copy.
       */
      retCode = ma( startIdx, endIdx, inReal,
         minUsed, optInMAType,
         &localBegIdx,&localNbElement,localFinalArray );

      if( retCode != TA_SUCCESS )
      {
         free(localOutputArray);
         free(localPeriodArray);
         free(sortedIdx);
         free(bucketOfs);
         if( finalIsAllocated ) { free(localFinalArray); }
            *outBegIdx = 0;
         *outNBElement = 0;
         return retCode;
      }
   }
   else
   {
      /* Counting sort: sortedIdx ends up holding the output indices ordered
       * by period, one contiguous ascending slice per distinct period, with
       * bucketOfs[p] the end of period p's slice.
       */
      for( curPeriod=minUsed; curPeriod <= maxUsed+1; curPeriod++ )
         bucketOfs[curPeriod] = 0;
      for( i=0; i < outputSize; i++ )
         bucketOfs[localPeriodArray[i]+1] = bucketOfs[localPeriodArray[i]+1] + 1;
      for( curPeriod=minUsed; curPeriod <= maxUsed; curPeriod++ )
         bucketOfs[curPeriod+1] = bucketOfs[curPeriod+1] + bucketOfs[curPeriod];
      for( i=0; i < outputSize; i++ )
      {
         tempInt = localPeriodArray[i];
         sortedIdx[bucketOfs[tempInt]] = i;
         bucketOfs[tempInt] = bucketOfs[tempInt] + 1;
      }

      /* One MA pass per period actually requested, ending at the last output
       * that uses it: outputs before that point cannot depend on later input
       * (every MA here fills forward), so the shorter range is bit-identical.
       * The pass must still START at startIdx: the window MAs (SMA, WMA,
       * TRIMA, HMA) slide a running accumulator seeded at startIdx-lookback,
       * so starting at the period's first use would change the rounding path
       * and break bit-identity, as would moving any recursive MA's warm-up.
       * The direct indexing of localOutputArray also relies on ma_lookback
       * being non-decreasing in the period, so the inner call never moves
       * its own start up. Both properties are pinned by the MAVP/GROUPING
       * regression test.
       */
      bucketStart = 0;
      for( curPeriod=minUsed; curPeriod <= maxUsed; curPeriod++ )
      {
         bucketEnd = bucketOfs[curPeriod];
         if( bucketEnd > bucketStart )
         {
            firstOccurrence = sortedIdx[bucketStart];
            lastOccurrence = sortedIdx[bucketEnd-1];

            /* Calculation of the MA required. */
            retCode = ma( startIdx, startIdx+lastOccurrence, inReal,
               curPeriod, optInMAType,
               &localBegIdx,&localNbElement,localOutputArray );

            if( retCode != TA_SUCCESS )
            {
               free(localOutputArray);
               free(localPeriodArray);
               free(sortedIdx);
               free(bucketOfs);
               if( finalIsAllocated ) { free(localFinalArray); }
                  *outBegIdx = 0;
               *outNBElement = 0;
               return retCode;
            }

            if( lastOccurrence - firstOccurrence == bucketEnd - 1 - bucketStart )
            {
               /* The period's outputs form one contiguous run: block copy. */
               memcpy( &localFinalArray[firstOccurrence],
                  &localOutputArray[firstOccurrence],
                  (bucketEnd-bucketStart) * sizeof(double) );
            }
            else
            {
               for( i=bucketStart; i < bucketEnd; i++ )
               {
                  tempInt = sortedIdx[i];
                  localFinalArray[tempInt] = localOutputArray[tempInt];
               }
            }
         }
         bucketStart = bucketEnd;
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
   free(sortedIdx);
   free(bucketOfs);
   if( finalIsAllocated ) { free(localFinalArray); }

      /* Done. Inform the caller of the success. */
   *outBegIdx = startIdx;
   *outNBElement = outputSize;
   return TA_SUCCESS;
}
