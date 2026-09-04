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

   /**
    * Number of leading input bars {@link Core#MAVP} consumes before it can
    * produce its first value.
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
    *        10=DISABLED, 11=DEFAULT, 12=ZLEMA; {@code MAType.DEFAULT} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MAVP_Lookback( int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
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
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      /* The same cross-parameter constraint mavp() rejects on. Each period is in
       * range on its own, so no prologue check can catch it, and without this the
       * lookback answers a usable number for a call that cannot run.
       */
      if( optInMinPeriod > optInMaxPeriod ) {
         return 0 - 1 ;
      }
      return MA_Lookback(optInMaxPeriod, optInMAType) ;

   }
   RetCode MAVP_Impl( int startIdx,
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
      int firstOut = 0;
      int tempInt = 0;
      int curPeriod = 0;
      double tempPeriod = 0;
      double minPeriodReal = 0;
      double maxPeriodReal = 0;
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
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
      lookbackTotal = MA_Lookback(optInMaxPeriod, optInMAType);
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
      /* Calculate exact output size. A dedicated temp: tempInt is the cast-fed
       * period below, which the Rust backend types SIGNED (#160) — reusing it
       * here would drag this index arithmetic into i32.
       */
      if( lookbackTotal > startIdx ) {
         firstOut = lookbackTotal;
      } else {
         firstOut = startIdx;
      }
      if( firstOut > endIdx ) {
         /* No output */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - firstOut + 1;
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
       * not by optInMaxPeriod. The floor at 1 (and on minUsed's start value)
       * keeps a period below 1 from indexing the occurrence tables out of range.
       * mavp.yaml caps both periods at [1, 100000], so it is inert through the
       * API; it is kept because this file is the source of truth for four
       * backends and it makes the shared source safe by construction rather than
       * by trusting each backend's prologue to be identical.
       */
      minUsed = optInMaxPeriod;
      if( minUsed < 1 ) {
         minUsed = 1;
      }
      maxUsed = 1;
      /* Both bounds widened once, outside the loop. In the C backend, left to the
       * compiler, only the first of the two is hoisted.
       */
      minPeriodReal = optInMinPeriod;
      maxPeriodReal = optInMaxPeriod;
      for( i = 0; i < outputSize; i += 1 ) {
         /* Clamp in the real domain, then narrow -- the order matters in the C
          * backend, and only there. C leaves an out-of-range narrowing undefined
          * and x86 lands EVERY such value on INT_MIN, so clamping afterwards pulls
          * a huge POSITIVE period down to the minimum. Java, C# and Rust saturate
          * to their maximum instead, so they were already right and this form
          * simply keeps them so.
          * `!(x >= min)` rather than `x < min`: both plain comparisons are false
          * for NaN, so only the inverted spelling catches it.
          */
         tempPeriod = inPeriods[startIdx + i];
         if( !(tempPeriod >= minPeriodReal) ) {
            tempInt = optInMinPeriod;
         } else if( tempPeriod > maxPeriodReal ) {
            tempInt = optInMaxPeriod;
         } else {
            tempInt = (int)tempPeriod;
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
      if( maxUsed < minUsed || maxUsed - minUsed > 100000 ) {
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
         OutRange _xr0 = MA(startIdx, endIdx, inReal, minUsed, optInMAType, localFinalArray);
         localBegIdx.value = _xr0.begIdx();
         localNbElement.value = _xr0.count();
         retCode = RetCode.Success;
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
               OutRange _xr1 = MA(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localOutputArray);
               localBegIdx.value = _xr1.begIdx();
               localNbElement.value = _xr1.count();
               retCode = RetCode.Success;
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
      /* Done. Inform the caller of the success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   RetCode MAVP_Impl( int startIdx,
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
      int firstOut = 0;
      int tempInt = 0;
      int curPeriod = 0;
      double tempPeriod = 0;
      double minPeriodReal = 0;
      double maxPeriodReal = 0;
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( optInMinPeriod > optInMaxPeriod ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      lookbackTotal = MA_Lookback(optInMaxPeriod, optInMAType);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( lookbackTotal > startIdx ) {
         firstOut = lookbackTotal;
      } else {
         firstOut = startIdx;
      }
      if( firstOut > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outputSize = endIdx - firstOut + 1;
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
      minPeriodReal = optInMinPeriod;
      maxPeriodReal = optInMaxPeriod;
      for( i = 0; i < outputSize; i += 1 ) {
         tempPeriod = (double)inPeriods[startIdx + i];
         if( !(tempPeriod >= minPeriodReal) ) {
            tempInt = optInMinPeriod;
         } else if( tempPeriod > maxPeriodReal ) {
            tempInt = optInMaxPeriod;
         } else {
            tempInt = (int)tempPeriod;
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
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      bucketOfs = new int[(int)((maxUsed - minUsed + 2) * 1)];
      if( minUsed == maxUsed ) {
         OutRange _xr0 = MA(startIdx, endIdx, inReal, minUsed, optInMAType, localFinalArray);
         localBegIdx.value = _xr0.begIdx();
         localNbElement.value = _xr0.count();
         retCode = RetCode.Success;
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
               OutRange _xr1 = MA(startIdx, startIdx + lastOccurrence, inReal, curPeriod, optInMAType, localOutputArray);
               localBegIdx.value = _xr1.begIdx();
               localNbElement.value = _xr1.count();
               retCode = RetCode.Success;
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
    * valid range shorter than {@link Core#MAVP_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    *        10=DISABLED, 11=DEFAULT, 12=ZLEMA; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outReal variable-period moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#MA
    * @see Core#SMA
    * @see Core#MAMA
    * @see Core#T3
    */
   public OutRange MAVP( int startIdx,
                         int endIdx,
                         double inReal[],
                         double inPeriods[],
                         int optInMinPeriod,
                         int optInMaxPeriod,
                         MAType optInMAType,
                         double outReal[] )
   {
      requireIndexRange("MAVP", startIdx, endIdx);
      requireArgument("MAVP", "optInMAType", optInMAType);
      int guardStart = clampedStart("MAVP", startIdx, MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAVP", "inReal", inReal, guardInLen);
      requireLength("MAVP", "inPeriods", inPeriods, guardInLen);
      requireLength("MAVP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAVP_Impl(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAVP", retCode);
      }
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
    * valid range shorter than {@link Core#MAVP_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
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
    *        10=DISABLED, 11=DEFAULT, 12=ZLEMA; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outReal variable-period moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#MA
    * @see Core#SMA
    * @see Core#MAMA
    * @see Core#T3
    */
   public OutRange MAVP( int startIdx,
                         int endIdx,
                         float inReal[],
                         float inPeriods[],
                         int optInMinPeriod,
                         int optInMaxPeriod,
                         MAType optInMAType,
                         double outReal[] )
   {
      requireIndexRange("MAVP", startIdx, endIdx);
      requireArgument("MAVP", "optInMAType", optInMAType);
      int guardStart = clampedStart("MAVP", startIdx, MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAVP", "inReal", inReal, guardInLen);
      requireLength("MAVP", "inPeriods", inPeriods, guardInLen);
      requireLength("MAVP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAVP_Impl(startIdx, endIdx, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAVP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MAVP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MAVP} over the same series.
    * Open with {@link Core#mavpOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MavpStream {
      Core core;
      int optInMinPeriod;
      int optInMaxPeriod;
      MAType optInMAType;
      double cur_outReal;
      // One sub-MA stream per period in [optInMinPeriod, optInMaxPeriod], advanced in lockstep.
      MaStream[] bank;
      int outRangeBegIdx;
      int outRangeCount;

      MavpStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MAVP} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MavpStream( MavpStream other ) {
         this.core = other.core;
         this.optInMinPeriod = other.optInMinPeriod;
         this.optInMaxPeriod = other.optInMaxPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.bank = new MaStream[other.bank.length];
         for( int bankIdx = 0; bankIdx < other.bank.length; bankIdx++ ) {
            this.bank[bankIdx] = new MaStream(other.bank[bankIdx]);
         }
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal, double inPeriods ) {
         if( !Double.isFinite(inReal) || !Double.isFinite(inPeriods) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MAVP update: BadParam", RetCode.BadParam);
         }
         core.mavpStepImpl(this, inReal, inPeriods);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double inPeriods[], double outReal[] ) {
         requireArgument("MAVP updateAndFill", "inReal", inReal);
         requireArgument("MAVP updateAndFill", "inPeriods", inPeriods);
         requireArgument("MAVP updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( inPeriods.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inReal || (Object)outReal == (Object)inPeriods )
            throw new TaLibArgumentException("MAVP updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) || !Double.isFinite(inPeriods[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("MAVP updateAndFill: BadParam", RetCode.BadParam);
            }
            core.mavpStepImpl(this, inReal[i], inPeriods[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal, double inPeriods ) {
         if( !Double.isFinite(inReal) || !Double.isFinite(inPeriods) )
            throw new TaLibArgumentException("MAVP peek: BadParam", RetCode.BadParam);
         MavpStream sp = this;
         int cp = (int)inPeriods;
         if( cp < sp.optInMinPeriod ) {
            cp = sp.optInMinPeriod;
         } else if( cp > sp.optInMaxPeriod ) {
            cp = sp.optInMaxPeriod;
         }
         int slot = cp - sp.optInMinPeriod;
         double cur_outReal = sp.bank[slot].peek(inReal);
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public MavpStream clone() {
         return new MavpStream(this);
      }
   }
   void mavpStepImpl( MavpStream sp, double inReal, double inPeriods )
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
   private RetCode mavpOpenImpl( MavpStream sp, double inReal[], double inPeriods[], int startIdx, int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inPeriods.length != inReal.length ) {
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
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      /* An inverted [min, max] period window is invalid (batch rejects). */
      if( optInMinPeriod > optInMaxPeriod ) {
         return RetCode.BadParam;
      }
      if( historyLen < MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      /* Seed EVERY sub at the SHARED max-period lookback, exactly as batch
       * does: it clamps startIdx up to lookback(maxPeriod) and calls the callee
       * with that same start for every period. Seeding each sub at its own
       * (smaller) lookback would seed the recurrence from a different bar and
       * diverge for every period < maxPeriod. */
      int lookbackTotal = MA_Lookback(optInMaxPeriod, optInMAType);
      int subStart = (startIdx < lookbackTotal)? lookbackTotal : startIdx;
      if( historyLen < subStart + 1 ) {
         return RetCode.InsufficientHistory;
      }
      int nBank = optInMaxPeriod - optInMinPeriod + 1;
      MaStream[] bank = new MaStream[nBank];
      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {
         bank[bankIdx] = maOpenInternal(inReal, subStart, optInMinPeriod + bankIdx, optInMAType);
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
      sp.outRangeBegIdx = subStart;
      sp.outRangeCount = historyLen - subStart;
      return RetCode.Success;
   }
   private RetCode mavpOpenAndFillImpl( MavpStream sp, double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inPeriods.length != inReal.length ) {
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
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inPeriods ) {
         return RetCode.BadParam;
      }
      /* An inverted [min, max] period window is invalid (batch rejects). */
      if( optInMinPeriod > optInMaxPeriod ) {
         return RetCode.BadParam;
      }
      int lookbackTotal = MA_Lookback(optInMaxPeriod, optInMAType);
      if( historyLen < lookbackTotal + 1 ) {
         return RetCode.InsufficientHistory;
      }
      int nBank = optInMaxPeriod - optInMinPeriod + 1;
      /* Seed each sub at the first output bar (lookbackTotal), NOT the last. */
      MaStream[] bank = new MaStream[nBank];
      double[] scratch = new double[nBank];
      double[] seedPrefix = java.util.Arrays.copyOfRange(inReal, 0, lookbackTotal + 1);
      for( int bankIdx = 0; bankIdx < nBank; bankIdx++ ) {
         MaStream sub = maOpenInternal(seedPrefix, lookbackTotal, optInMinPeriod + bankIdx, optInMAType);
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
   /* Internal startIdx-anchored open behind mavpOpen (composition seam). */
   MavpStream mavpOpenInternal( double inReal[], double inPeriods[], int startIdx, int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      MavpStream sp = new MavpStream(this);
      RetCode retCode = mavpOpenImpl(sp, inReal, inPeriods, startIdx, optInMinPeriod, optInMaxPeriod, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MAVP open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MAVP open: internal error", retCode);
      }
      throw new TaLibArgumentException("MAVP open: " + retCode, retCode);
   }
   /**
    * Open a live MAVP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MAVP} at that bar.
    * <p>The history must hold at least {@code MAVP_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MavpStream mavpOpen( double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType )
   {
      requireArgument("MAVP open", "inReal", inReal);
      requireHistory("MAVP open", inReal.length);
      requireArgument("MAVP open", "optInMAType", optInMAType);
      requireArgument("MAVP open", "inPeriods", inPeriods);
      requireHistoryLength("MAVP open", "inPeriods", inPeriods.length, inReal.length);
      return mavpOpenInternal(inReal, inPeriods, 0, optInMinPeriod, optInMaxPeriod, optInMAType);
   }
   /**
    * {@link Core#mavpOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MAVP} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MavpStream#outRange()}.
    */
   public MavpStream mavpOpenAndFill( double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType, double outReal[] )
   {
      requireArgument("MAVP openAndFill", "inReal", inReal);
      requireHistory("MAVP openAndFill", inReal.length);
      requireArgument("MAVP openAndFill", "optInMAType", optInMAType);
      requireArgument("MAVP openAndFill", "inPeriods", inPeriods);
      int guardOutLen = openFillCount("MAVP openAndFill", inReal.length, MAVP_Lookback(optInMinPeriod, optInMaxPeriod, optInMAType));
      requireHistoryLength("MAVP openAndFill", "inPeriods", inPeriods.length, inReal.length);
      requireLength("MAVP openAndFill", "outReal", outReal, guardOutLen);
      MavpStream sp = new MavpStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = mavpOpenAndFillImpl(sp, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MAVP openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MAVP openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MAVP openAndFill: " + retCode, retCode);
   }
