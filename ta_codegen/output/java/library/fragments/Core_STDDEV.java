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
    * Number of leading input bars {@link Core#stdDev} consumes before it can
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
   public int stdDevLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == TA_REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( optInNbDev < TA_REAL_MIN || optInNbDev > TA_REAL_MAX ) {
         return -1;
      }
      /* Lookback is driven by the variance. */
      return varianceLookback(optInTimePeriod, optInNbDev) ;

   }
   RetCode stdDevInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == TA_REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( optInNbDev < TA_REAL_MIN || optInNbDev > TA_REAL_MAX ) {
         return RetCode.BadParam;
      }
      /* Calculate the variance. */
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   RetCode stdDevUnguardedInternal( int startIdx,
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
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   RetCode stdDevInternal( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == TA_REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( optInNbDev < TA_REAL_MIN || optInNbDev > TA_REAL_MAX ) {
         return RetCode.BadParam;
      }
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   RetCode stdDevUnguardedInternal( int startIdx,
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
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
    * valid range shorter than {@link Core#stdDevLookback} is a <b>success with
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#variance
    * @see Core#bbands
    * @see Core#sma
    */
   public OutRange stdDev( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = stdDevInternal(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root. —
    * <b>unchecked</b> variant of {@link Core#stdDev}.
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
   public OutRange stdDevUnguarded( int startIdx,
                                    int endIdx,
                                    double inReal[],
                                    int optInTimePeriod,
                                    double optInNbDev,
                                    double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      stdDevUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
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
    * valid range shorter than {@link Core#stdDevLookback} is a <b>success with
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
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#variance
    * @see Core#bbands
    * @see Core#sma
    */
   public OutRange stdDev( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = stdDevInternal(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root. —
    * <b>unchecked</b> variant of {@link Core#stdDev}.
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
   public OutRange stdDevUnguarded( int startIdx,
                                    int endIdx,
                                    float inReal[],
                                    int optInTimePeriod,
                                    double optInNbDev,
                                    double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      stdDevUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STDDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#stdDev} over the same series.
    * Open with {@link Core#stdDevOpen}; there is no close — the handle is
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
   public static final class StdDevStream {
      final Core core;
      int optInTimePeriod;
      double optInNbDev;
      double cur_outReal;
      VarianceStream sub0;
      OutRange fillRange;

      StdDevStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#stdDevOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      StdDevStream( StdDevStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new VarianceStream(other.sub0);
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.stdDevStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         StdDevStream scratch = new StdDevStream(this);
         core.stdDevStreamStep(scratch, inReal);
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
      public StdDevStream copy() {
         return new StdDevStream(this);
      }
   }
   void stdDevStreamStep( StdDevStream sp, double inReal )
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
   private RetCode stdDevOpenBody( StdDevStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == TA_REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( optInNbDev < TA_REAL_MIN || optInNbDev > TA_REAL_MAX ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarianceStream sub0 = varianceOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, 1.0);
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
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
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDev = optInNbDev;
      sp.sub0 = sub0;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   private RetCode stdDevOpenAndFillBody( StdDevStream sp, double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == TA_REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( optInNbDev < TA_REAL_MIN || optInNbDev > TA_REAL_MAX ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarianceStream sub0 = varianceOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, 1.0);
      retCode = varianceUnguardedInternal(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
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
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDev = optInNbDev;
      sp.sub0 = sub0;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      System.arraycopy(sc_outReal, 0, outReal, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind stdDevOpen (composition seam). */
   StdDevStream stdDevOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      StdDevStream sp = new StdDevStream(this);
      RetCode retCode = stdDevOpenBody(sp, inReal, startIdx, optInTimePeriod, optInNbDev);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STDDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STDDEV open: internal error");
      }
      throw new IllegalArgumentException("TA_STDDEV open: " + retCode);
   }
   /**
    * Open a live STDDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#stdDev} at that bar.
    * <p>The history must hold at least {@code stdDevLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public StdDevStream stdDevOpen( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      return stdDevOpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#stdDevOpen} that also fills the output array(s) bit-identically
    * to {@link Core#stdDev} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link StdDevStream#fillRange()}.
    */
   public StdDevStream stdDevOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, double outReal[] )
   {
      StdDevStream sp = new StdDevStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = stdDevOpenAndFillBody(sp, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STDDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STDDEV openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_STDDEV openAndFill: " + retCode);
   }
