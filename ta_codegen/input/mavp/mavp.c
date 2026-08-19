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
 *  072726 MF,CC  #145. Index the bucket table relative to the smallest period
 *                used, and bound it so an off-contract period cannot overflow.
 *  080326 MF,CC  Split the size temp from the cast-fed period temp (#160).
 */

int mavp_lookback(int optInMinPeriod, int optInMaxPeriod, TA_MAType optInMAType)
{
   /* The same cross-parameter constraint mavp() rejects on. Each period is in
    * range on its own, so no prologue check can catch it, and without this the
    * lookback answers a usable number for a call that cannot run. */
   if( optInMinPeriod > optInMaxPeriod )
      return -1;

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
   int i, lookbackTotal, outputSize, firstOut, tempInt, curPeriod;
   double tempPeriod, minPeriodReal, maxPeriodReal;
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

   /* Calculate exact output size. A dedicated temp: tempInt is the cast-fed
    * period below, which the Rust backend types SIGNED (#160) — reusing it
    * here would drag this index arithmetic into i32. */
   if( lookbackTotal > startIdx )
      firstOut = lookbackTotal;
   else
      firstOut = startIdx;
   if( firstOut > endIdx )
   {
      /* No output */
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   outputSize = endIdx - firstOut + 1;

   /* Allocate intermediate local buffer. */
   double *localOutputArray = malloc((outputSize) * sizeof(double));
   int *localPeriodArray = malloc((outputSize) * sizeof(int));

   /* Output indices grouped by clamped period (counting sort below). */
   sortedIdx = malloc((outputSize) * sizeof(int));
   if( localOutputArray == NULL || localPeriodArray == NULL || sortedIdx == NULL )
   {
      free(localOutputArray);
      free(localPeriodArray);
      free(sortedIdx);
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
      if( localFinalArray == NULL )
      {
         free(localOutputArray);
         free(localPeriodArray);
         free(sortedIdx);
         *outBegIdx = 0;
         *outNBElement = 0;
         return TA_ALLOC_ERR;
      }
   }
   else
   {
      localFinalArray = outReal;
   }

   /* Read the caller array of period, truncate to min/max, and track the
    * range of periods actually used so all later work is sized by the data,
    * not by optInMaxPeriod. The floor at 1 (and on minUsed's start value)
    * keeps a period below 1 from indexing the occurrence tables out of range.
    * mavp.yaml caps both periods at [1, 100000], so it is inert through the
    * API; it is kept because this file is the source of truth for four
    * backends and it makes the shared source safe by construction rather than
    * by trusting each backend's prologue to be identical.
    */
   minUsed = optInMaxPeriod;
   if( minUsed < 1 )
      minUsed = 1;
   maxUsed = 1;
   /* Both bounds widened once, outside the loop. In the C backend, left to the
    * compiler, only the first of the two is hoisted.
    */
   minPeriodReal = optInMinPeriod;
   maxPeriodReal = optInMaxPeriod;
   for( i=0; i < outputSize; i++ )
   {
      /* Clamp in the real domain, then narrow -- the order matters in the C
       * backend, and only there. C leaves an out-of-range narrowing undefined
       * and x86 lands EVERY such value on INT_MIN, so clamping afterwards pulls
       * a huge POSITIVE period down to the minimum. Java, C# and Rust saturate
       * to their maximum instead, so they were already right and this form
       * simply keeps them so.
       * `!(x >= min)` rather than `x < min`: both plain comparisons are false
       * for NaN, so only the inverted spelling catches it.
       */
      tempPeriod = inPeriods[startIdx+i];
      if( !(tempPeriod >= minPeriodReal) )
         tempInt = optInMinPeriod;
      else if( tempPeriod > maxPeriodReal )
         tempInt = optInMaxPeriod;
      else
         tempInt = (int)tempPeriod;
      if( tempInt < 1 )
         tempInt = 1;
      localPeriodArray[i] = tempInt;
      if( tempInt < minUsed )
         minUsed = tempInt;
      if( tempInt > maxUsed )
         maxUsed = tempInt;
   }

   /* Bound the bucket table before sizing it.
    *
    * Unreachable through the API: mavp.yaml caps both periods at 100000, so
    * the widest spread expressible is 99999. It is kept because it protects a
    * memory-safety property and this file is the source of truth for four
    * backends — without it the size expression below can overflow (signed
    * overflow in C, a wrapped negative in Java, a usize underflow panic in
    * Rust), and the four bodies would rest on each backend's prologue being
    * byte-for-byte equivalent, which nothing here states or checks. One
    * integer comparison per call is a cheap way not to depend on that.
    *
    * Written as a plain integer comparison on purpose — that is the only
    * construct that means the same thing in every backend. A (size_t) cast
    * would NOT help: this dialect's size_t parses to the generic index type
    * and renders back as int in both the C and the Java output (it is a
    * Rust-only annotation).
    *
    * If you delete this, delete the clamps and the comments together.
    */
   if( maxUsed < minUsed || maxUsed - minUsed > 100000 )
   {
      free(localOutputArray);
      free(localPeriodArray);
      free(sortedIdx);
      if( finalIsAllocated ) { free(localFinalArray); }
         *outBegIdx = 0;
      *outNBElement = 0;
      return TA_BAD_PARAM;
   }

   /* Per-period bucket cursor for the counting sort. Indexed RELATIVE to
    * minUsed: only [minUsed, maxUsed+1] is ever touched, so sizing from the
    * largest period used allocated up to 400KB for a band of periods that
    * may be a handful wide — and allocated it even on the single-period
    * fast path below.
    */
   bucketOfs = malloc((maxUsed-minUsed+2) * sizeof(int));
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
         bucketOfs[curPeriod-minUsed] = 0;
      for( i=0; i < outputSize; i++ )
      {
         /* Staged through tempInt, not indexed inline: the Rust backend only
          * coerces an int-array read to the index type when it is a DIRECT
          * operand, so bucketOfs[localPeriodArray[i]+1-minUsed] would mix
          * i32 with usize and fail to compile. */
         tempInt = localPeriodArray[i];
         bucketOfs[tempInt+1-minUsed] = bucketOfs[tempInt+1-minUsed] + 1;
      }
      for( curPeriod=minUsed; curPeriod <= maxUsed; curPeriod++ )
         bucketOfs[curPeriod+1-minUsed] = bucketOfs[curPeriod+1-minUsed] + bucketOfs[curPeriod-minUsed];
      for( i=0; i < outputSize; i++ )
      {
         tempInt = localPeriodArray[i];
         sortedIdx[bucketOfs[tempInt-minUsed]] = i;
         bucketOfs[tempInt-minUsed] = bucketOfs[tempInt-minUsed] + 1;
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
         bucketEnd = bucketOfs[curPeriod-minUsed];
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
