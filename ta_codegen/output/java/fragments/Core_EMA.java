/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  080926 MF,CC Explicit no-smoothing copy at a period of 1.
 *  081026 MF,CC Fold the internal variant into EMA (issue #183).
 */

   /**
    * Number of leading input bars {@link Core#EMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int EMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 + this.unstablePeriod[FuncUnstId.EMA.ordinal()] ;

   }
   RetCode EMA_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = EMA_Lookback(optInTimePeriod);
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
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because at
       * period 1 optInK_1 is exactly 1.0, so the recursion below reduces to
       * (x-prev)+prev -- which returns x only while consecutive values stay
       * within a factor of two of each other. Two-decimal prices already
       * spend a full mantissa, so a single 3x move breaks it. The unstable
       * period still delays the first output.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma(inReal[today++] - prevMA, optInK_1, prevMA);
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(inReal[today++] - prevMA, optInK_1, prevMA);
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode EMA_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      lookbackTotal = EMA_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma((double)inReal[today++] - prevMA, optInK_1, prevMA);
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma((double)inReal[today++] - prevMA, optInK_1, prevMA);
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#EMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal price/data series to smooth.
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal the exponential moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#SMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#MA
    * @see Core#MACD
    * @see Core#T3
    */
   public OutRange EMA( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("EMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, EMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EMA", "inReal", inReal, guardInLen);
      requireLength("EMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = EMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Exponential moving average that weights recent prices more heavily via a
    * recursive smoothing factor. A core building block seeding or composing
    * many other indicators. Reacts faster than SMA; price above/below EMA
    * suggests up/down trend.
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#EMA_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal price/data series to smooth.
    * @param optInTimePeriod Number of bars in the average; sets smoothing k =
    *        2/(period+1) (default 30; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal the exponential moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#SMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#MA
    * @see Core#MACD
    * @see Core#T3
    */
   public OutRange EMA( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("EMA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, EMA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EMA", "inReal", inReal, guardInLen);
      requireLength("EMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = EMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live EMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#EMA} over the same series.
    * Open with {@link Core#EMA_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class EMA_Stream {
      Core core;
      int optInTimePeriod;
      double optInK_1;
      double prevMA;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      EMA_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#EMA_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      EMA_Stream( EMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( EMA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("EMA update: BadParam", RetCode.BadParam);
         core.EMA_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("EMA peek: BadParam", RetCode.BadParam);
         EMA_Stream scratch = new EMA_Stream(this);
         core.EMA_StreamStep(scratch, inReal);
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
      public EMA_Stream copy() {
         return new EMA_Stream(this);
      }
   }
   void EMA_StreamStep( EMA_Stream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      sp.prevMA = Math.fma(inReal - sp.prevMA, sp.optInK_1, sp.prevMA);
      sp.cur_outReal = sp.prevMA;
   }
   private RetCode EMA_OpenPass( EMA_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < EMA_Lookback(optInTimePeriod) + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInK_1 = 0.0;
         sp.prevMA = 0.0;
         int fillLb = EMA_Lookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = EMA_Lookback(optInTimePeriod);
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
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      today = startIdx - lookbackTotal;
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         prevMA = Math.fma(inReal[today++] - prevMA, optInK_1, prevMA);
      }
      outReal[0 * outStride] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = Math.fma(inReal[today++] - prevMA, optInK_1, prevMA);
         outReal[outIdx++ * outStride] = prevMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInK_1 = optInK_1;
      sp.prevMA = prevMA;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode EMA_OpenImpl( EMA_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return EMA_OpenPass( sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode EMA_OpenAndFillImpl( EMA_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return EMA_OpenPass( sp, inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode EMA_OpenAndFillInternalImpl( EMA_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return EMA_OpenPass(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* EMA_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   EMA_Stream EMA_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      EMA_Stream sp = new EMA_Stream(this);
      RetCode retCode = EMA_OpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("EMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("EMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("EMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind EMA_Open (composition seam). */
   EMA_Stream EMA_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      EMA_Stream sp = new EMA_Stream(this);
      RetCode retCode = EMA_OpenImpl(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("EMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("EMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("EMA open: " + retCode, retCode);
   }
   /**
    * Open a live EMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#EMA} at that bar.
    * <p>The history must hold at least {@code EMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public EMA_Stream EMA_Open( double inReal[], int optInTimePeriod )
   {
      return EMA_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#EMA_Open} that also fills the output array(s) bit-identically
    * to {@link Core#EMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link EMA_Stream#fillRange()}.
    */
   public EMA_Stream EMA_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      EMA_Stream sp = new EMA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = EMA_OpenAndFillImpl(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("EMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("EMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("EMA openAndFill: " + retCode, retCode);
   }
