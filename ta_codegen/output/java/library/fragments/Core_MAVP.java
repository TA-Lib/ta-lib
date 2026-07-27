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
 */

   /**
    * Number of leading input bars {@link Core#movingAverageVariablePeriod}
    * consumes before it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInMinPeriod Lower clamp for the per-bar period (default 2; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMaxPeriod Upper clamp for the per-bar period (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving-average type applied (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int movingAverageVariablePeriodLookback( int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return -1;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return -1;
      }
      return movingAverageLookback(optInMaxPeriod, optInMAType) ;

   }
   RetCode movingAverageVariablePeriodInternal( int startIdx,
                                                int endIdx,
                                                double inReal[],
                                                double inPeriods[],
                                                int optInMinPeriod,
                                                int optInMaxPeriod,
                                                MAType optInMAType,
                                                MInteger outBegIdx,
                                                MInteger outNBElement,
                                                double outReal[] )
   {
      int i = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int firstOccurrence = 0;
      int lastOccurrence = 0;
      int bucketStart = 0;
      int bucketEnd = 0;
      int minUsed = 0;
      int maxUsed = 0;
      int[] localPeriodArray;
      int[] sortedIdx;
      int[] bucketOfs;
      double[] localOutputArray;
      double[] localFinalArray;
      int finalIsAllocated = 0;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* An inverted period window (min above max) is an invalid parameter
       * combination: the per-bar clamp below would push a period above
       * optInMaxPeriod, exceeding the lookback and reading uninitialized
       * results. Reject it cleanly instead of returning garbage.
       */
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Calculate exact output size */
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         /* No output */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      /* Allocate intermediate local buffer. */
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      /* Output indices grouped by clamped period (counting sort below). */
      sortedIdx = new int[(int)(outputSize * 1)];
      /* In-place defence (issue #130): each ma() pass below re-reads inReal over
       * the full range, so with outReal==inReal the results are staged in a
       * scratch buffer and copied once at the end. A regular call writes
       * straight to outReal and skips both the allocation and the copy.
       */
      finalIsAllocated = 0;
      if( outReal == inReal ) {
         finalIsAllocated = 1;
         localFinalArray = new double[(int)(outputSize * 1)];
      } else {
         localFinalArray = outReal;
      }
      /* Read the caller array of period, truncate to min/max, and track the
       * range of periods actually used so all later work is sized by the data,
       * not by optInMaxPeriod. The floor at 1 (and on minUsed's start value) is
       * inert through the guarded API (optInMinPeriod >= 1); it keeps an
       * off-contract unguarded call with a period below 1 from indexing the
       * occurrence tables out of range. The high side is not floored here: an
       * out-of-range optInMaxPeriod violates the unguarded precondition (every
       * optional parameter resolved and in-range), and the bucket-table bound
       * below is what keeps that from becoming undefined behaviour.
       */
      minUsed = optInMaxPeriod;
      if( minUsed < 1 ) {
         minUsed = 1;
      }
      maxUsed = 1;
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         if( tempInt < 1 ) {
            tempInt = 1;
         }
         localPeriodArray[i] = tempInt;
         if( tempInt < minUsed ) {
            minUsed = tempInt;
         }
         if( tempInt > maxUsed ) {
            maxUsed = tempInt;
         }
      }
      /* Bound the bucket table before sizing it. Inert through the guarded API,
       * where both periods are capped at 100000 (mavp.yaml) so the spread cannot
       * reach the bound. It exists for an off-contract UNGUARDED call carrying a
       * near-INT_MAX period, where the size expression below would otherwise
       * overflow: signed-overflow UB in C, a wrapped negative in Java, a usize
       * underflow panic in Rust. Written as a plain integer comparison on
       * purpose — that is the only construct that means the same thing in every
       * backend. A (size_t) cast would NOT help: this dialect's size_t parses to
       * the generic index type and renders back as int in both the C and the
       * Java output (it is a Rust-only annotation).
       */
      if( maxUsed < minUsed || maxUsed - minUsed > 100000 ) {
         if( (finalIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      /* Per-period bucket cursor for the counting sort. Indexed RELATIVE to
       * minUsed: only [minUsed, maxUsed+1] is ever touched, so sizing from the
       * largest period used allocated up to 400KB for a band of periods that
       * may be a handful wide — and allocated it even on the single-period
       * fast path below.
       */
      bucketOfs = new int[(int)((maxUsed - minUsed + 2) * 1)];
      if( minUsed == maxUsed ) {
         /* Single distinct period: one MA pass, written straight into the
          * destination buffer. Nothing to group or copy.
          */
         retCode = movingAverageUnguardedInternal(startIdx, endIdx, inReal, minUsed, optInMAType, localBegIdx, localNbElement, localFinalArray);
         if( retCode != RetCode.Success ) {
            if( (finalIsAllocated) != 0 ) {
            }
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return retCode ;
         }
      } else {
         /* Counting sort: sortedIdx ends up holding the output indices ordered
          * by period, one contiguous ascending slice per distinct period, with
          * bucketOfs[p] the end of period p's slice.
          */
         for( curPeriod = minUsed; curPeriod <= maxUsed + 1; curPeriod += 1 ) {
            bucketOfs[curPeriod - minUsed] = 0;
         }
         for( i = 0; i < outputSize; i += 1 ) {
            /* Staged through tempInt, not indexed inline: the Rust backend only
             * coerces an int-array read to the index type when it is a DIRECT
             * operand, so bucketOfs[localPeriodArray[i]+1-minUsed] would mix
             * i32 with usize and fail to compile.
             */
            tempInt = localPeriodArray[i];
            bucketOfs[tempInt + 1 - minUsed] = bucketOfs[tempInt + 1 - minUsed] + 1;
         }
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketOfs[curPeriod + 1 - minUsed] = bucketOfs[curPeriod + 1 - minUsed] + bucketOfs[curPeriod - minUsed];
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            sortedIdx[bucketOfs[tempInt - minUsed]] = i;
            bucketOfs[tempInt - minUsed] = bucketOfs[tempInt - minUsed] + 1;
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
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketEnd = bucketOfs[curPeriod - minUsed];
            if( bucketEnd > bucketStart ) {
               firstOccurrence = sortedIdx[bucketStart];
               lastOccurrence = sortedIdx[bucketEnd - 1];
               /* Calculation of the MA required. */
               retCode = movingAverageUnguardedInternal(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
               if( retCode != RetCode.Success ) {
                  if( (finalIsAllocated) != 0 ) {
                  }
                  outBegIdx.value = 0;
                  outNBElement.value = 0;
                  return retCode ;
               }
               if( lastOccurrence - firstOccurrence == bucketEnd - 1 - bucketStart ) {
                  /* The period's outputs form one contiguous run: block copy. */
                  System.arraycopy(localOutputArray, firstOccurrence, localFinalArray, firstOccurrence, (bucketEnd - bucketStart) * 1);
               } else {
                  for( i = bucketStart; i < bucketEnd; i += 1 ) {
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
       * always run; in C/Java the non-aliased self-copy is skipped.
       */
      if( localFinalArray != outReal ) {
         System.arraycopy(localFinalArray, 0, outReal, 0, outputSize * 1);
      }
      if( (finalIsAllocated) != 0 ) {
      }
      /* Done. Inform the caller of the success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   RetCode movingAverageVariablePeriodUnguardedInternal( int startIdx,
                                                         int endIdx,
                                                         double inReal[],
                                                         double inPeriods[],
                                                         int optInMinPeriod,
                                                         int optInMaxPeriod,
                                                         MAType optInMAType,
                                                         MInteger outBegIdx,
                                                         MInteger outNBElement,
                                                         double outReal[] )
   {
      int i = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int firstOccurrence = 0;
      int lastOccurrence = 0;
      int bucketStart = 0;
      int bucketEnd = 0;
      int minUsed = 0;
      int maxUsed = 0;
      int[] localPeriodArray;
      int[] sortedIdx;
      int[] bucketOfs;
      double[] localOutputArray;
      double[] localFinalArray;
      int finalIsAllocated = 0;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      sortedIdx = new int[(int)(outputSize * 1)];
      finalIsAllocated = 0;
      if( outReal == inReal ) {
         finalIsAllocated = 1;
         localFinalArray = new double[(int)(outputSize * 1)];
      } else {
         localFinalArray = outReal;
      }
      minUsed = optInMaxPeriod;
      if( minUsed < 1 ) {
         minUsed = 1;
      }
      maxUsed = 1;
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         if( tempInt < 1 ) {
            tempInt = 1;
         }
         localPeriodArray[i] = tempInt;
         if( tempInt < minUsed ) {
            minUsed = tempInt;
         }
         if( tempInt > maxUsed ) {
            maxUsed = tempInt;
         }
      }
      if( maxUsed < minUsed || maxUsed - minUsed > 100000 ) {
         if( (finalIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      bucketOfs = new int[(int)((maxUsed - minUsed + 2) * 1)];
      if( minUsed == maxUsed ) {
         retCode = movingAverageUnguardedInternal(startIdx, endIdx, inReal, minUsed, optInMAType, localBegIdx, localNbElement, localFinalArray);
         if( retCode != RetCode.Success ) {
            if( (finalIsAllocated) != 0 ) {
            }
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return retCode ;
         }
      } else {
         for( curPeriod = minUsed; curPeriod <= maxUsed + 1; curPeriod += 1 ) {
            bucketOfs[curPeriod - minUsed] = 0;
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            bucketOfs[tempInt + 1 - minUsed] = bucketOfs[tempInt + 1 - minUsed] + 1;
         }
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketOfs[curPeriod + 1 - minUsed] = bucketOfs[curPeriod + 1 - minUsed] + bucketOfs[curPeriod - minUsed];
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            sortedIdx[bucketOfs[tempInt - minUsed]] = i;
            bucketOfs[tempInt - minUsed] = bucketOfs[tempInt - minUsed] + 1;
         }
         bucketStart = 0;
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketEnd = bucketOfs[curPeriod - minUsed];
            if( bucketEnd > bucketStart ) {
               firstOccurrence = sortedIdx[bucketStart];
               lastOccurrence = sortedIdx[bucketEnd - 1];
               retCode = movingAverageUnguardedInternal(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
               if( retCode != RetCode.Success ) {
                  if( (finalIsAllocated) != 0 ) {
                  }
                  outBegIdx.value = 0;
                  outNBElement.value = 0;
                  return retCode ;
               }
               if( lastOccurrence - firstOccurrence == bucketEnd - 1 - bucketStart ) {
                  System.arraycopy(localOutputArray, firstOccurrence, localFinalArray, firstOccurrence, (bucketEnd - bucketStart) * 1);
               } else {
                  for( i = bucketStart; i < bucketEnd; i += 1 ) {
                     tempInt = sortedIdx[i];
                     localFinalArray[tempInt] = localOutputArray[tempInt];
                  }
               }
            }
            bucketStart = bucketEnd;
         }
      }
      if( localFinalArray != outReal ) {
         System.arraycopy(localFinalArray, 0, outReal, 0, outputSize * 1);
      }
      if( (finalIsAllocated) != 0 ) {
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   RetCode movingAverageVariablePeriodInternal( int startIdx,
                                                int endIdx,
                                                float inReal[],
                                                float inPeriods[],
                                                int optInMinPeriod,
                                                int optInMaxPeriod,
                                                MAType optInMAType,
                                                MInteger outBegIdx,
                                                MInteger outNBElement,
                                                double outReal[] )
   {
      int i = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int firstOccurrence = 0;
      int lastOccurrence = 0;
      int bucketStart = 0;
      int bucketEnd = 0;
      int minUsed = 0;
      int maxUsed = 0;
      int[] localPeriodArray;
      int[] sortedIdx;
      int[] bucketOfs;
      double[] localOutputArray;
      double[] localFinalArray;
      int finalIsAllocated = 0;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      sortedIdx = new int[(int)(outputSize * 1)];
      finalIsAllocated = 0;
      if( false ) {
         finalIsAllocated = 1;
         localFinalArray = new double[(int)(outputSize * 1)];
      } else {
         localFinalArray = outReal;
      }
      minUsed = optInMaxPeriod;
      if( minUsed < 1 ) {
         minUsed = 1;
      }
      maxUsed = 1;
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)(double)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         if( tempInt < 1 ) {
            tempInt = 1;
         }
         localPeriodArray[i] = tempInt;
         if( tempInt < minUsed ) {
            minUsed = tempInt;
         }
         if( tempInt > maxUsed ) {
            maxUsed = tempInt;
         }
      }
      if( maxUsed < minUsed || maxUsed - minUsed > 100000 ) {
         if( (finalIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      bucketOfs = new int[(int)((maxUsed - minUsed + 2) * 1)];
      if( minUsed == maxUsed ) {
         retCode = movingAverageUnguardedInternal(startIdx, endIdx, inReal, minUsed, optInMAType, localBegIdx, localNbElement, localFinalArray);
         if( retCode != RetCode.Success ) {
            if( (finalIsAllocated) != 0 ) {
            }
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return retCode ;
         }
      } else {
         for( curPeriod = minUsed; curPeriod <= maxUsed + 1; curPeriod += 1 ) {
            bucketOfs[curPeriod - minUsed] = 0;
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            bucketOfs[tempInt + 1 - minUsed] = bucketOfs[tempInt + 1 - minUsed] + 1;
         }
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketOfs[curPeriod + 1 - minUsed] = bucketOfs[curPeriod + 1 - minUsed] + bucketOfs[curPeriod - minUsed];
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            sortedIdx[bucketOfs[tempInt - minUsed]] = i;
            bucketOfs[tempInt - minUsed] = bucketOfs[tempInt - minUsed] + 1;
         }
         bucketStart = 0;
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketEnd = bucketOfs[curPeriod - minUsed];
            if( bucketEnd > bucketStart ) {
               firstOccurrence = sortedIdx[bucketStart];
               lastOccurrence = sortedIdx[bucketEnd - 1];
               retCode = movingAverageUnguardedInternal(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
               if( retCode != RetCode.Success ) {
                  if( (finalIsAllocated) != 0 ) {
                  }
                  outBegIdx.value = 0;
                  outNBElement.value = 0;
                  return retCode ;
               }
               if( lastOccurrence - firstOccurrence == bucketEnd - 1 - bucketStart ) {
                  System.arraycopy(localOutputArray, firstOccurrence, localFinalArray, firstOccurrence, (bucketEnd - bucketStart) * 1);
               } else {
                  for( i = bucketStart; i < bucketEnd; i += 1 ) {
                     tempInt = sortedIdx[i];
                     localFinalArray[tempInt] = localOutputArray[tempInt];
                  }
               }
            }
            bucketStart = bucketEnd;
         }
      }
      if( localFinalArray != outReal ) {
         System.arraycopy(localFinalArray, 0, outReal, 0, outputSize * 1);
      }
      if( (finalIsAllocated) != 0 ) {
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   RetCode movingAverageVariablePeriodUnguardedInternal( int startIdx,
                                                         int endIdx,
                                                         float inReal[],
                                                         float inPeriods[],
                                                         int optInMinPeriod,
                                                         int optInMaxPeriod,
                                                         MAType optInMAType,
                                                         MInteger outBegIdx,
                                                         MInteger outNBElement,
                                                         double outReal[] )
   {
      int i = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int firstOccurrence = 0;
      int lastOccurrence = 0;
      int bucketStart = 0;
      int bucketEnd = 0;
      int minUsed = 0;
      int maxUsed = 0;
      int[] localPeriodArray;
      int[] sortedIdx;
      int[] bucketOfs;
      double[] localOutputArray;
      double[] localFinalArray;
      int finalIsAllocated = 0;
      MInteger localBegIdx = new MInteger();
      MInteger localNbElement = new MInteger();
      RetCode retCode;
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         tempInt = lookbackTotal;
      } else {
         tempInt = startIdx;
      }
      if( tempInt > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - tempInt + 1;
      localOutputArray = new double[(int)(outputSize * 1)];
      localPeriodArray = new int[(int)(outputSize * 1)];
      sortedIdx = new int[(int)(outputSize * 1)];
      finalIsAllocated = 0;
      if( false ) {
         finalIsAllocated = 1;
         localFinalArray = new double[(int)(outputSize * 1)];
      } else {
         localFinalArray = outReal;
      }
      minUsed = optInMaxPeriod;
      if( minUsed < 1 ) {
         minUsed = 1;
      }
      maxUsed = 1;
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)(double)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         if( tempInt < 1 ) {
            tempInt = 1;
         }
         localPeriodArray[i] = tempInt;
         if( tempInt < minUsed ) {
            minUsed = tempInt;
         }
         if( tempInt > maxUsed ) {
            maxUsed = tempInt;
         }
      }
      if( maxUsed < minUsed || maxUsed - minUsed > 100000 ) {
         if( (finalIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      bucketOfs = new int[(int)((maxUsed - minUsed + 2) * 1)];
      if( minUsed == maxUsed ) {
         retCode = movingAverageUnguardedInternal(startIdx, endIdx, inReal, minUsed, optInMAType, localBegIdx, localNbElement, localFinalArray);
         if( retCode != RetCode.Success ) {
            if( (finalIsAllocated) != 0 ) {
            }
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return retCode ;
         }
      } else {
         for( curPeriod = minUsed; curPeriod <= maxUsed + 1; curPeriod += 1 ) {
            bucketOfs[curPeriod - minUsed] = 0;
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            bucketOfs[tempInt + 1 - minUsed] = bucketOfs[tempInt + 1 - minUsed] + 1;
         }
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketOfs[curPeriod + 1 - minUsed] = bucketOfs[curPeriod + 1 - minUsed] + bucketOfs[curPeriod - minUsed];
         }
         for( i = 0; i < outputSize; i += 1 ) {
            tempInt = localPeriodArray[i];
            sortedIdx[bucketOfs[tempInt - minUsed]] = i;
            bucketOfs[tempInt - minUsed] = bucketOfs[tempInt - minUsed] + 1;
         }
         bucketStart = 0;
         for( curPeriod = minUsed; curPeriod <= maxUsed; curPeriod += 1 ) {
            bucketEnd = bucketOfs[curPeriod - minUsed];
            if( bucketEnd > bucketStart ) {
               firstOccurrence = sortedIdx[bucketStart];
               lastOccurrence = sortedIdx[bucketEnd - 1];
               retCode = movingAverageUnguardedInternal(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
               if( retCode != RetCode.Success ) {
                  if( (finalIsAllocated) != 0 ) {
                  }
                  outBegIdx.value = 0;
                  outNBElement.value = 0;
                  return retCode ;
               }
               if( lastOccurrence - firstOccurrence == bucketEnd - 1 - bucketStart ) {
                  System.arraycopy(localOutputArray, firstOccurrence, localFinalArray, firstOccurrence, (bucketEnd - bucketStart) * 1);
               } else {
                  for( i = bucketStart; i < bucketEnd; i += 1 ) {
                     tempInt = sortedIdx[i];
                     localFinalArray[tempInt] = localOutputArray[tempInt];
                  }
               }
            }
            bucketStart = bucketEnd;
         }
      }
      if( localFinalArray != outReal ) {
         System.arraycopy(localFinalArray, 0, outReal, 0, outputSize * 1);
      }
      if( (finalIsAllocated) != 0 ) {
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   /**
    * Moving average whose period varies per bar, driven by a companion period
    * series. For each bar it computes an MA of the selected type over the
    * (clamped) period given by inPeriods.
    * <p><b>Formula</b>
    * <pre>{@code
    * p_i = clamp((int)inPeriods[startIdx+i], optInMinPeriod, optInMaxPeriod); outReal[i] = MA(inReal, p_i, optInMAType) at bar startIdx+i
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Fractional per-bar periods are truncated to whole numbers before being clamped to the minimum and maximum period.</li>
    * <li>Period values of 1 perform no smoothing (the bar's output equals its input); the minimum allowed period is 1 since 0.6.5.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#movingAverageVariablePeriodLookback}
    * is a <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal series to be averaged.
    * @param inPeriods per-bar desired MA period.
    * @param optInMinPeriod Lower clamp for the per-bar period (default 2; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMaxPeriod Upper clamp for the per-bar period (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving-average type applied (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED).
    * @param outReal variable-period moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#movingAverage
    * @see Core#sma
    * @see Core#mama
    * @see Core#t3
    */
   public OutRange movingAverageVariablePeriod( int startIdx,
                                                int endIdx,
                                                double inReal[],
                                                double inPeriods[],
                                                int optInMinPeriod,
                                                int optInMaxPeriod,
                                                MAType optInMAType,
                                                double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageVariablePeriodInternal(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAVP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Moving average whose period varies per bar, driven by a companion period
    * series. For each bar it computes an MA of the selected type over the
    * (clamped) period given by inPeriods. — <b>unchecked</b> variant of
    * {@link Core#movingAverageVariablePeriod}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange movingAverageVariablePeriodUnguarded( int startIdx,
                                                         int endIdx,
                                                         double inReal[],
                                                         double inPeriods[],
                                                         int optInMinPeriod,
                                                         int optInMaxPeriod,
                                                         MAType optInMAType,
                                                         double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      movingAverageVariablePeriodUnguardedInternal(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Moving average whose period varies per bar, driven by a companion period
    * series. For each bar it computes an MA of the selected type over the
    * (clamped) period given by inPeriods.
    * <p><b>Formula</b>
    * <pre>{@code
    * p_i = clamp((int)inPeriods[startIdx+i], optInMinPeriod, optInMaxPeriod); outReal[i] = MA(inReal, p_i, optInMAType) at bar startIdx+i
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Fractional per-bar periods are truncated to whole numbers before being clamped to the minimum and maximum period.</li>
    * <li>Period values of 1 perform no smoothing (the bar's output equals its input); the minimum allowed period is 1 since 0.6.5.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#movingAverageVariablePeriodLookback}
    * is a <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal series to be averaged.
    * @param inPeriods per-bar desired MA period.
    * @param optInMinPeriod Lower clamp for the per-bar period (default 2; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMaxPeriod Upper clamp for the per-bar period (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving-average type applied (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED).
    * @param outReal variable-period moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#movingAverage
    * @see Core#sma
    * @see Core#mama
    * @see Core#t3
    */
   public OutRange movingAverageVariablePeriod( int startIdx,
                                                int endIdx,
                                                float inReal[],
                                                float inPeriods[],
                                                int optInMinPeriod,
                                                int optInMaxPeriod,
                                                MAType optInMAType,
                                                double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageVariablePeriodInternal(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAVP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Moving average whose period varies per bar, driven by a companion period
    * series. For each bar it computes an MA of the selected type over the
    * (clamped) period given by inPeriods. — <b>unchecked</b> variant of
    * {@link Core#movingAverageVariablePeriod}.
    * <p>Validates nothing and never throws. The caller guarantees: non-negative
    * {@code startIdx}, {@code endIdx >= startIdx}, non-null arrays, output
    * arrays distinct from each other, and every optional parameter already
    * resolved and within its documented range — a sentinel such as
    * {@code Integer.MIN_VALUE} is <b>not</b> substituted here.
    * <p>Breaking any of those yields an empty {@link OutRange} or undefined
    * output rather than a diagnostic. (C and Rust return a status code from
    * this tier, so their callers can detect it; this one has nowhere to report
    * it.) Use the guarded method unless the arguments are already known good.
    * <p>This is the {@code float[]} overload; see the guarded method.
    *
    * @return The range written, exactly as the guarded method reports it.
    */
   public OutRange movingAverageVariablePeriodUnguarded( int startIdx,
                                                         int endIdx,
                                                         float inReal[],
                                                         float inPeriods[],
                                                         int optInMinPeriod,
                                                         int optInMaxPeriod,
                                                         MAType optInMAType,
                                                         double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      movingAverageVariablePeriodUnguardedInternal(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MAVP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#movingAverageVariablePeriod} over the same series.
    * Open with {@link Core#movingAverageVariablePeriodOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MovingAverageVariablePeriodStream {
      final Core core;
      int optInMinPeriod;
      int optInMaxPeriod;
      MAType optInMAType;
      double cur_outReal;
      // One sub-MA stream per period in [optInMinPeriod, optInMaxPeriod], advanced in lockstep.
      MovingAverageStream[] bank;
      OutRange fillRange;

      MovingAverageVariablePeriodStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#movingAverageVariablePeriodOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      MovingAverageVariablePeriodStream( MovingAverageVariablePeriodStream other ) {
         this.core = other.core;
         this.optInMinPeriod = other.optInMinPeriod;
         this.optInMaxPeriod = other.optInMaxPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.bank = new MovingAverageStream[other.bank.length];
         for( int bankIdx = 0; bankIdx < other.bank.length; bankIdx++ ) {
            this.bank[bankIdx] = new MovingAverageStream(other.bank[bankIdx]);
         }
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal, double inPeriods ) {
         core.movingAverageVariablePeriodStreamStep(this, inReal, inPeriods);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal, double inPeriods ) {
         MovingAverageVariablePeriodStream scratch = new MovingAverageVariablePeriodStream(this);
         core.movingAverageVariablePeriodStreamStep(scratch, inReal, inPeriods);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MovingAverageVariablePeriodStream copy() {
         return new MovingAverageVariablePeriodStream(this);
      }
   }
   void movingAverageVariablePeriodStreamStep( MovingAverageVariablePeriodStream sp, double inReal, double inPeriods )
   {
      int cp = (int)inPeriods;
      if( cp < sp.optInMinPeriod ) {
         cp = sp.optInMinPeriod;
      } else if( cp > sp.optInMaxPeriod ) {
         cp = sp.optInMaxPeriod;
      }
      int slot = cp - sp.optInMinPeriod;
      for( int bankIdx = 0; bankIdx < sp.bank.length; bankIdx++ ) {
         double subValue = sp.bank[bankIdx].update(inReal);
         if( bankIdx == slot ) {
            sp.cur_outReal = subValue;
         }
      }
   }
   private RetCode movingAverageVariablePeriodOpenBody( MovingAverageVariablePeriodStream sp, double inReal[], double inPeriods[], int startIdx, int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 || inPeriods.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* An inverted [min, max] period window is invalid (batch rejects). */
      if( optInMinPeriod > optInMaxPeriod ) {
         return RetCode.BadParam;
      }
      if( historyLen < movingAverageVariablePeriodLookback(optInMinPeriod, optInMaxPeriod, optInMAType) + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      /* Seed EVERY sub at the SHARED max-period lookback, exactly as batch
       * does: it clamps startIdx up to lookback(maxPeriod) and calls the callee
       * with that same start for every period. Seeding each sub at its own
       * (smaller) lookback would seed the recurrence from a different bar and
       * diverge for every period < maxPeriod. */
      int lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      int subStart = (startIdx < lookbackTotal)? lookbackTotal : startIdx;
      int nBank = optInMaxPeriod - optInMinPeriod + 1;
      MovingAverageStream[] bank = new MovingAverageStream[nBank];
      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {
         bank[bankIdx] = movingAverageOpenInternal(inReal, subStart, optInMinPeriod + bankIdx, optInMAType);
      }
      int cp = (int)inPeriods[historyLen - 1];
      if( cp < optInMinPeriod ) {
         cp = optInMinPeriod;
      } else if( cp > optInMaxPeriod ) {
         cp = optInMaxPeriod;
      }
      sp.optInMinPeriod = optInMinPeriod;
      sp.optInMaxPeriod = optInMaxPeriod;
      sp.optInMAType = optInMAType;
      sp.bank = bank;
      sp.cur_outReal = bank[cp - optInMinPeriod].cur_outReal;
      return RetCode.Success;
   }
   private RetCode movingAverageVariablePeriodOpenAndFillBody( MovingAverageVariablePeriodStream sp, double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 || inPeriods.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( optInMinPeriod == Integer.MIN_VALUE ) {
         optInMinPeriod = 2;
      } else if( optInMinPeriod < 1 || optInMinPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMaxPeriod == Integer.MIN_VALUE ) {
         optInMaxPeriod = 30;
      } else if( optInMaxPeriod < 1 || optInMaxPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inPeriods ) {
         return RetCode.BadParam;
      }
      /* An inverted [min, max] period window is invalid (batch rejects). */
      if( optInMinPeriod > optInMaxPeriod ) {
         return RetCode.BadParam;
      }
      int lookbackTotal = movingAverageLookback(optInMaxPeriod, optInMAType);
      if( historyLen < lookbackTotal + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int nBank = optInMaxPeriod - optInMinPeriod + 1;
      /* Seed each sub at the first output bar (lookbackTotal), NOT the last. */
      MovingAverageStream[] bank = new MovingAverageStream[nBank];
      double[] scratch = new double[nBank];
      double[] seedPrefix = java.util.Arrays.copyOfRange(inReal, 0, lookbackTotal + 1);
      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {
         MovingAverageStream sub = movingAverageOpenInternal(seedPrefix, lookbackTotal, optInMinPeriod + bankIdx, optInMAType);
         bank[bankIdx] = sub;
         scratch[bankIdx] = sub.cur_outReal;
      }
      /* First output bar (lookbackTotal), then replay the remaining history. */
      int cp = (int)inPeriods[lookbackTotal];
      if( cp < optInMinPeriod ) {
         cp = optInMinPeriod;
      } else if( cp > optInMaxPeriod ) {
         cp = optInMaxPeriod;
      }
      outReal[0] = scratch[cp - optInMinPeriod];
      for( int t = lookbackTotal + 1; t < historyLen; t++ ) {
         for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {
            scratch[bankIdx] = bank[bankIdx].update(inReal[t]);
         }
         cp = (int)inPeriods[t];
         if( cp < optInMinPeriod ) {
            cp = optInMinPeriod;
         } else if( cp > optInMaxPeriod ) {
            cp = optInMaxPeriod;
         }
         outReal[t - lookbackTotal] = scratch[cp - optInMinPeriod];
      }
      outBegIdx.value = lookbackTotal;
      outNBElement.value = historyLen - lookbackTotal;
      sp.optInMinPeriod = optInMinPeriod;
      sp.optInMaxPeriod = optInMaxPeriod;
      sp.optInMAType = optInMAType;
      sp.bank = bank;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind movingAverageVariablePeriodOpen (composition seam). */
   MovingAverageVariablePeriodStream movingAverageVariablePeriodOpenInternal( double inReal[], double inPeriods[], int startIdx, int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      MovingAverageVariablePeriodStream sp = new MovingAverageVariablePeriodStream(this);
      RetCode retCode = movingAverageVariablePeriodOpenBody(sp, inReal, inPeriods, startIdx, optInMinPeriod, optInMaxPeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MAVP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MAVP open: internal error");
      }
      throw new IllegalArgumentException("TA_MAVP open: " + retCode);
   }
   /**
    * Open a live MAVP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#movingAverageVariablePeriod} at that bar.
    * <p>The history must hold at least {@code movingAverageVariablePeriodLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MovingAverageVariablePeriodStream movingAverageVariablePeriodOpen( double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      return movingAverageVariablePeriodOpenInternal(inReal, inPeriods, 0, optInMinPeriod, optInMaxPeriod, optInMAType);
   }
   /**
    * {@link Core#movingAverageVariablePeriodOpen} that also fills the output array(s) bit-identically
    * to {@link Core#movingAverageVariablePeriod} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MovingAverageVariablePeriodStream#fillRange()}.
    */
   public MovingAverageVariablePeriodStream movingAverageVariablePeriodOpenAndFill( double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType, double outReal[] )
   {
      MovingAverageVariablePeriodStream sp = new MovingAverageVariablePeriodStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = movingAverageVariablePeriodOpenAndFillBody(sp, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MAVP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MAVP openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MAVP openAndFill: " + retCode);
   }
