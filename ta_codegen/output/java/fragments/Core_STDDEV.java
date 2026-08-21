/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  090404 MF   Fix #978056. Trap sqrt with negative zero values.
 */

   /**
    * Number of leading input bars {@link Core#STDDEV} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int STDDEV_Lookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return -1;
      }
      /* Lookback is driven by the variance. */
      return VAR_Lookback(optInTimePeriod, optInNbDev) ;

   }
   RetCode STDDEV_Impl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Same shape as the guard in apo and bbands: the variance below runs on the
       * same range and its lookback IS stddev's, so it declines and yields 0,0
       * without reading. Observably identical, but it makes "a range shorter than
       * the lookback reads nothing" true of stddev itself rather than only of var.
       * Pinned by the zero-length no-I/O probe over every guarded core.
       */
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Calculate the variance. */
      OutRange _xr0 = VAR(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the square root of each variance, this
       * is the standard deviation.
       *
       * Multiply also by the ratio specified.
       */
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
   RetCode STDDEV_Impl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      OutRange _xr0 = VAR(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STDDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to measure dispersion of.
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @param outReal Standard deviation at each bar, scaled by optInNbDev. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#VAR
    * @see Core#BBANDS
    * @see Core#SMA
    */
   public OutRange STDDEV( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      requireIndexRange("STDDEV", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, STDDEV_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STDDEV", "inReal", inReal, guardInLen);
      requireLength("STDDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STDDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STDDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to measure dispersion of.
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @param outReal Standard deviation at each bar, scaled by optInNbDev. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#VAR
    * @see Core#BBANDS
    * @see Core#SMA
    */
   public OutRange STDDEV( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      requireIndexRange("STDDEV", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, STDDEV_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STDDEV", "inReal", inReal, guardInLen);
      requireLength("STDDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STDDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STDDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STDDEV} over the same series.
    * Open with {@link Core#STDDEV_Open}; there is no close — the handle is
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
   public static final class STDDEV_Stream {
      Core core;
      int optInTimePeriod;
      double optInNbDev;
      double cur_outReal;
      VAR_Stream sub0;
      OutRange fillRange = OutRange.EMPTY;

      STDDEV_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#STDDEV_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      STDDEV_Stream( STDDEV_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new VAR_Stream(other.sub0);
         this.fillRange = other.fillRange;
      }

      void copyFrom( STDDEV_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outReal = other.cur_outReal;
         if( this.sub0 == null ) {
            this.sub0 = new VAR_Stream(other.sub0);
         } else {
            this.sub0.copyFrom(other.sub0);
         }
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
            throw new TaLibArgumentException("STDDEV update: BadParam", RetCode.BadParam);
         core.STDDEV_StreamStep(this, inReal);
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
            throw new TaLibArgumentException("STDDEV peek: BadParam", RetCode.BadParam);
         STDDEV_Stream scratch = new STDDEV_Stream(this);
         core.STDDEV_StreamStep(scratch, inReal);
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
      public STDDEV_Stream copy() {
         return new STDDEV_Stream(this);
      }
   }
   void STDDEV_StreamStep( STDDEV_Stream sp, double inReal )
   {
      double tempReal = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_outReal = sp.sub0.update(inReal);
      /* Combine map (batch tail, per bar). */
      if( sp.optInNbDev != 1.0 ) {
         tempReal = cur_outReal;
         if( !(tempReal < 0.00000000000001) ) {
            cur_outReal = Math.sqrt(tempReal) * sp.optInNbDev;
         } else {
            cur_outReal = (double)0.0;
         }
      } else {
         tempReal = cur_outReal;
         if( !(tempReal < 0.00000000000001) ) {
            cur_outReal = Math.sqrt(tempReal);
         } else {
            cur_outReal = (double)0.0;
         }
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode STDDEV_OpenPass( STDDEV_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( historyLen < STDDEV_Lookback(optInTimePeriod, optInNbDev) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Same shape as the guard in apo and bbands: the variance below runs on the
       * same range and its lookback IS stddev's, so it declines and yields 0,0
       * without reading. Observably identical, but it makes "a range shorter than
       * the lookback reads nothing" true of stddev itself rather than only of var.
       * Pinned by the zero-length no-I/O probe over every guarded core.
       */
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VAR_Stream sub0 = VAR_OpenAndFillInternal(inReal, startIdx, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.Success;
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the square root of each variance, this
       * is the standard deviation.
       *
       * Multiply also by the ratio specified.
       */
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = sc_outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               sc_outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               sc_outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = sc_outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               sc_outReal[i] = Math.sqrt(tempReal);
            } else {
               sc_outReal[i] = (double)0.0;
            }
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDev = optInNbDev;
      sp.sub0 = sub0;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   private RetCode STDDEV_OpenImpl( STDDEV_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return STDDEV_OpenPass( sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode STDDEV_OpenAndFillImpl( STDDEV_Stream sp, double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return STDDEV_OpenPass( sp, inReal, 0, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode STDDEV_OpenAndFillInternalImpl( STDDEV_Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return STDDEV_OpenPass(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
   }
   /* STDDEV_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   STDDEV_Stream STDDEV_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      STDDEV_Stream sp = new STDDEV_Stream(this);
      RetCode retCode = STDDEV_OpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STDDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STDDEV openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STDDEV openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind STDDEV_Open (composition seam). */
   STDDEV_Stream STDDEV_OpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      STDDEV_Stream sp = new STDDEV_Stream(this);
      RetCode retCode = STDDEV_OpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STDDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STDDEV open: internal error", retCode);
      }
      throw new TaLibArgumentException("STDDEV open: " + retCode, retCode);
   }
   /**
    * Open a live STDDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STDDEV} at that bar.
    * <p>The history must hold at least {@code STDDEV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public STDDEV_Stream STDDEV_Open( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      return STDDEV_OpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#STDDEV_Open} that also fills the output array(s) bit-identically
    * to {@link Core#STDDEV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link STDDEV_Stream#fillRange()}.
    */
   public STDDEV_Stream STDDEV_OpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, double outReal[] )
   {
      STDDEV_Stream sp = new STDDEV_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STDDEV_OpenAndFillImpl(sp, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STDDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STDDEV openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STDDEV openAndFill: " + retCode, retCode);
   }
