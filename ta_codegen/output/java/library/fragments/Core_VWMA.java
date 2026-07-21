/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  072026 MF,CC  First version.
 */

   public int vwmaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode vwma( int startIdx,
                        int endIdx,
                        double inReal[],
                        double inVolume[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
      /* Add-up the initial period, except for the last value.
       *
       * The price*volume product is kept in its own statement so no compiler may
       * contract it into an FMA: that would make this function disagree with the
       * Rust/Java backends under the cross-language bitwise gate, and with the
       * two-TA_SMA composite reference.
       */
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         /* Snapshot both sums before removing the trailing bar, mirroring the
          * add-new / snapshot / subtract-old order of TA_SMA. That order is what
          * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
          */
         tempPV = sumPV;
         tempV = sumV;
         /* Read the trailing values before writing the output, since the caller
          * may pass the same buffer for an input and the output.
          */
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode vwmaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 double inVolume[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = (int)(optInTimePeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         tempPV = sumPV;
         tempV = sumV;
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode vwma( int startIdx,
                        int endIdx,
                        float inReal[],
                        float inVolume[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = (int)(optInTimePeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = (double)inReal[i] * (double)inVolume[i];
            sumPV += tempReal;
            sumV += (double)inVolume[i];
            i = i + 1;
         }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inReal[i] * (double)inVolume[i];
         sumPV += tempReal;
         sumV += (double)inVolume[i];
         i = i + 1;
         tempPV = sumPV;
         tempV = sumV;
         tempReal = (double)inReal[trailingIdx] * (double)inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= (double)inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode vwmaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 float inVolume[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = (int)(optInTimePeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = (double)inReal[i] * (double)inVolume[i];
            sumPV += tempReal;
            sumV += (double)inVolume[i];
            i = i + 1;
         }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = (double)inReal[i] * (double)inVolume[i];
         sumPV += tempReal;
         sumV += (double)inVolume[i];
         i = i + 1;
         tempPV = sumPV;
         tempV = sumV;
         tempReal = (double)inReal[trailingIdx] * (double)inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= (double)inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live VWMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#vwma} over the same series.
    * Open with {@link Core#vwmaOpen}; there is no close — the handle is
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
   public static final class VwmaStream {
      final Core core;
      int optInTimePeriod;
      double sumPV;
      double sumV;
      double tempPV;
      double tempV;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double[] ring_trailingIdx_inVolume;
      double cur_outReal;

      VwmaStream( Core core ) { this.core = core; }

      VwmaStream( VwmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumPV = other.sumPV;
         this.sumV = other.sumV;
         this.tempPV = other.tempPV;
         this.tempV = other.tempV;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.ring_trailingIdx_inVolume = other.ring_trailingIdx_inVolume.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal, double inVolume ) {
         core.vwmaStreamStep(this, inReal, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal, double inVolume ) {
         VwmaStream scratch = new VwmaStream(this);
         core.vwmaStreamStep(scratch, inReal, inVolume);
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
      public VwmaStream copy() {
         return new VwmaStream(this);
      }
   }
   void vwmaStreamStep( VwmaStream sp, double inReal, double inVolume )
   {
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
         sp.ring_trailingIdx_inVolume[0] = inVolume;
      }
      tempReal = inReal * inVolume;
      sp.sumPV += tempReal;
      sp.sumV += inVolume;
      /* Snapshot both sums before removing the trailing bar, mirroring the
       * add-new / snapshot / subtract-old order of TA_SMA. That order is what
       * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
       */
      sp.tempPV = sp.sumPV;
      sp.tempV = sp.sumV;
      /* Read the trailing values before writing the output, since the caller
       * may pass the same buffer for an input and the output.
       */
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] * sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
      sp.sumPV -= tempReal;
      sp.sumV -= sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx];
      sp.cur_outReal = sp.tempPV / (double)sp.optInTimePeriod / (sp.tempV / (double)sp.optInTimePeriod);
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ring_trailingIdx_inVolume[sp.ringPos_trailingIdx] = inVolume;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode vwmaOpenBody( VwmaStream sp, double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Add-up the initial period, except for the last value.
       *
       * The price*volume product is kept in its own statement so no compiler may
       * contract it into an FMA: that would make this function disagree with the
       * Rust/Java backends under the cross-language bitwise gate, and with the
       * two-TA_SMA composite reference.
       */
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         /* Snapshot both sums before removing the trailing bar, mirroring the
          * add-new / snapshot / subtract-old order of TA_SMA. That order is what
          * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
          */
         tempPV = sumPV;
         tempV = sumV;
         /* Read the trailing values before writing the output, since the caller
          * may pass the same buffer for an input and the output.
          */
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         lastValue_outReal = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inVolume = new double[allocN_trailingIdx];
      System.arraycopy(inVolume, historyLen - cap_trailingIdx, capRing_trailingIdx_inVolume, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumPV = sumPV;
      sp.sumV = sumV;
      sp.tempPV = tempPV;
      sp.tempV = tempV;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode vwmaOpenAndFillBody( VwmaStream sp, double inReal[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double sumPV = 0;
      double sumV = 0;
      double tempPV = 0;
      double tempV = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inVolume.length != inReal.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal || (Object)outReal == (Object)inVolume ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Add-up the initial period, except for the last value.
       *
       * The price*volume product is kept in its own statement so no compiler may
       * contract it into an FMA: that would make this function disagree with the
       * Rust/Java backends under the cross-language bitwise gate, and with the
       * two-TA_SMA composite reference.
       */
      sumPV = 0.0;
      sumV = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            tempReal = inReal[i] * inVolume[i];
            sumPV += tempReal;
            sumV += inVolume[i];
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         tempReal = inReal[i] * inVolume[i];
         sumPV += tempReal;
         sumV += inVolume[i];
         i = i + 1;
         /* Snapshot both sums before removing the trailing bar, mirroring the
          * add-new / snapshot / subtract-old order of TA_SMA. That order is what
          * makes this bit-identical to SMA(inReal*inVolume)/SMA(inVolume).
          */
         tempPV = sumPV;
         tempV = sumV;
         /* Read the trailing values before writing the output, since the caller
          * may pass the same buffer for an input and the output.
          */
         tempReal = inReal[trailingIdx] * inVolume[trailingIdx];
         sumPV -= tempReal;
         sumV -= inVolume[trailingIdx];
         outReal[outIdx] = tempPV / (double)optInTimePeriod / (tempV / (double)optInTimePeriod);
         trailingIdx = trailingIdx + 1;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inVolume = new double[allocN_trailingIdx];
      System.arraycopy(inVolume, historyLen - cap_trailingIdx, capRing_trailingIdx_inVolume, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumPV = sumPV;
      sp.sumV = sumV;
      sp.tempPV = tempPV;
      sp.tempV = tempV;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.ring_trailingIdx_inVolume = capRing_trailingIdx_inVolume;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind vwmaOpen (composition seam). */
   VwmaStream vwmaOpenInternal( double inReal[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      VwmaStream sp = new VwmaStream(this);
      RetCode retCode = vwmaOpenBody(sp, inReal, inVolume, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_VWMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_VWMA open: internal error");
      }
      throw new IllegalArgumentException("TA_VWMA open: " + retCode);
   }
   /**
    * Open a live VWMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#vwma} at that bar.
    * <p>The history must hold at least {@code vwmaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public VwmaStream vwmaOpen( double inReal[], double inVolume[], int optInTimePeriod )
   {
      return vwmaOpenInternal(inReal, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#vwmaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#vwma} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public VwmaStream vwmaOpenAndFill( double inReal[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      VwmaStream sp = new VwmaStream(this);
      RetCode retCode = vwmaOpenAndFillBody(sp, inReal, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_VWMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_VWMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_VWMA openAndFill: " + retCode);
   }
