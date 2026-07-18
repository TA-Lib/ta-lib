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
   public RetCode movingAverageVariablePeriod( int startIdx,
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
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
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
      /* Copy caller array of period into local buffer.
       * At the same time, truncate to min/max.
       */
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
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
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            /* TODO: This portion of the function can be slightly speed
             *       optimized by making the function without unstable period
             *       start their calculation at 'startIdx+i' instead of startIdx.
             */
            /* Calculation of the MA required. */
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  /* Flag to avoid recalculation */
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      /* Done. Inform the caller of the success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriodUnguarded( int startIdx,
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
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
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
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriod( int startIdx,
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
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
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
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)(double)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
   }
   public RetCode movingAverageVariablePeriodUnguarded( int startIdx,
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
      int j = 0;
      int lookbackTotal = 0;
      int outputSize = 0;
      int tempInt = 0;
      int curPeriod = 0;
      int[] localPeriodArray;
      double[] localOutputArray;
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
      for( i = 0; i < outputSize; i += 1 ) {
         tempInt = (int)(double)inPeriods[startIdx + i];
         if( tempInt < optInMinPeriod ) {
            tempInt = optInMinPeriod;
         } else if( tempInt > optInMaxPeriod ) {
            tempInt = optInMaxPeriod;
         }
         localPeriodArray[i] = tempInt;
      }
      for( i = 0; i < outputSize; i += 1 ) {
         curPeriod = localPeriodArray[i];
         if( curPeriod != 0 ) {
            retCode = movingAverageUnguarded(startIdx, endIdx, inReal, curPeriod, optInMAType, localBegIdx, localNbElement, localOutputArray);
            if( retCode != RetCode.Success ) {
               outBegIdx.value = 0;
               outNBElement.value = 0;
               return retCode ;
            }
            outReal[i] = localOutputArray[i];
            for( j = i + 1; j < outputSize; j += 1 ) {
               if( localPeriodArray[j] == curPeriod ) {
                  localPeriodArray[j] = 0;
                  outReal[j] = localOutputArray[j];
               }
            }
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outputSize;
      return RetCode.Success ;
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

      MovingAverageVariablePeriodStream( Core core ) { this.core = core; }

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
    */
   public MovingAverageVariablePeriodStream movingAverageVariablePeriodOpenAndFill( double inReal[], double inPeriods[], int optInMinPeriod, int optInMaxPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MovingAverageVariablePeriodStream sp = new MovingAverageVariablePeriodStream(this);
      RetCode retCode = movingAverageVariablePeriodOpenAndFillBody(sp, inReal, inPeriods, optInMinPeriod, optInMaxPeriod, optInMAType, outBegIdx, outNBElement, outReal);
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
