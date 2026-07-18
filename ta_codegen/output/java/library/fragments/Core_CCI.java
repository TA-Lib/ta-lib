/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AF       Alexander Trufanov (github @trufanov-nok)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  031202 MF     Template creation.
 *  052603 MF     Port to managed C++. Change to use CIRCBUF macros.
 *  061704 MF     Lower limit for period to 2, and correct algorithm
 *                to avoid cummulative error when value are close to
 *                the floating point epsilon.
 *  070626 AF,CC  Guard the final division with TA_IS_ZERO instead of an exact
 *                "!= 0.0" check: identical prices over the period leave
 *                sub-epsilon residue that the exact check divided into a
 *                spurious value (issue #7 / SF bug #107). Now returns 0.0.
 */

   public int cciLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode cci( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
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
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      return RetCode.Success ;
   }
   public RetCode cciUnguarded( int startIdx,
                                int endIdx,
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cci( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cciUnguarded( int startIdx,
                                int endIdx,
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CCI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cci} over the same series.
    * Open with {@link Core#cciOpen}; there is no close — the handle is
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
   public static final class CciStream {
      final Core core;
      int optInTimePeriod;
      double tempReal;
      double tempReal2;
      double theAverage;
      int j;
      int circBuffer_Idx;
      int maxIdx_circBuffer;
      int cbSize_circBuffer;
      double[] cb_circBuffer;
      double cur_outReal;

      CciStream( Core core ) { this.core = core; }

      CciStream( CciStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.tempReal = other.tempReal;
         this.tempReal2 = other.tempReal2;
         this.theAverage = other.theAverage;
         this.j = other.j;
         this.circBuffer_Idx = other.circBuffer_Idx;
         this.maxIdx_circBuffer = other.maxIdx_circBuffer;
         this.cbSize_circBuffer = other.cbSize_circBuffer;
         this.cb_circBuffer = other.cb_circBuffer.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.cciStreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         CciStream scratch = new CciStream(this);
         core.cciStreamStep(scratch, inHigh, inLow, inClose);
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
      public CciStream copy() {
         return new CciStream(this);
      }
   }
   void cciStreamStep( CciStream sp, double inHigh, double inLow, double inClose )
   {
      double lastValue = 0.0;
      lastValue = (inHigh + inLow + inClose) / 3;
      sp.cb_circBuffer[sp.circBuffer_Idx] = lastValue;
      /* Calculate the average for the whole period. */
      sp.theAverage = 0;
      for( sp.j = 0; sp.j < sp.optInTimePeriod; sp.j += 1 ) {
         sp.theAverage += sp.cb_circBuffer[sp.j];
      }
      sp.theAverage /= sp.optInTimePeriod;
      /* Do the summation of the ABS(TypePrice-average)
       * for the whole period.
       */
      sp.tempReal2 = 0;
      for( sp.j = 0; sp.j < sp.optInTimePeriod; sp.j += 1 ) {
         sp.tempReal2 += Math.abs(sp.cb_circBuffer[sp.j] - sp.theAverage);
      }
      /* And finally, the CCI... */
      sp.tempReal = lastValue - sp.theAverage;
      if( !((-0.00000000000001 < sp.tempReal) && (sp.tempReal < 0.00000000000001)) && !((-0.00000000000001 < sp.tempReal2) && (sp.tempReal2 < 0.00000000000001)) ) {
         sp.cur_outReal = sp.tempReal / (0.015 * (sp.tempReal2 / sp.optInTimePeriod));
      } else {
         sp.cur_outReal = 0.0;
      }
      /* Move forward the circular buffer indexes. */
      sp.circBuffer_Idx = sp.circBuffer_Idx + 1;
      if( sp.circBuffer_Idx > sp.maxIdx_circBuffer ) {
         sp.circBuffer_Idx = 0;
      }
   }
   private RetCode cciOpenBody( CciStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
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
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            lastValue_outReal = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            lastValue_outReal = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      /* Capture the live batch state into the handle. */
      int capCb_circBuffer = maxIdx_circBuffer + 1;
      if( capCb_circBuffer > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.tempReal = tempReal;
      sp.tempReal2 = tempReal2;
      sp.theAverage = theAverage;
      sp.j = j;
      sp.circBuffer_Idx = circBuffer_Idx;
      sp.maxIdx_circBuffer = maxIdx_circBuffer;
      sp.cbSize_circBuffer = capCb_circBuffer;
      sp.cb_circBuffer = circBuffer;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode cciOpenAndFillBody( CciStream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
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
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) && !((-0.00000000000001 < tempReal2) && (tempReal2 < 0.00000000000001)) ) {
            outReal[outIdx++] = tempReal / (0.015 * (tempReal2 / optInTimePeriod));
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      /* Capture the live batch state into the handle. */
      int capCb_circBuffer = maxIdx_circBuffer + 1;
      if( capCb_circBuffer > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.tempReal = tempReal;
      sp.tempReal2 = tempReal2;
      sp.theAverage = theAverage;
      sp.j = j;
      sp.circBuffer_Idx = circBuffer_Idx;
      sp.maxIdx_circBuffer = maxIdx_circBuffer;
      sp.cbSize_circBuffer = capCb_circBuffer;
      sp.cb_circBuffer = circBuffer;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cciOpen (composition seam). */
   CciStream cciOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      CciStream sp = new CciStream(this);
      RetCode retCode = cciOpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CCI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CCI open: internal error");
      }
      throw new IllegalArgumentException("TA_CCI open: " + retCode);
   }
   /**
    * Open a live CCI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cci} at that bar.
    * <p>The history must hold at least {@code cciLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CciStream cciOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return cciOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cciOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cci} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CciStream cciOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CciStream sp = new CciStream(this);
      RetCode retCode = cciOpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CCI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CCI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CCI openAndFill: " + retCode);
   }
