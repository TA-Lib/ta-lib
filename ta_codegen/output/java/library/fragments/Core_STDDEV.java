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

   public int stdDevLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Lookback is driven by the variance. */
      return varianceLookback(optInTimePeriod, optInNbDev) ;

   }
   public RetCode stdDev( int startIdx,
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Calculate the variance. */
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   public RetCode stdDevUnguarded( int startIdx,
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
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   public RetCode stdDev( int startIdx,
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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
   public RetCode stdDevUnguarded( int startIdx,
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
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
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

      StdDevStream( Core core ) { this.core = core; }

      StdDevStream( StdDevStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new VarianceStream(other.sub0);
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      double[] sc_outReal = new double[historyLen];
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarianceStream sub0 = varianceOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, 1.0);
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
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
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      double[] sc_outReal = new double[historyLen];
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarianceStream sub0 = varianceOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, 1.0);
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
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
    */
   public StdDevStream stdDevOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      StdDevStream sp = new StdDevStream(this);
      RetCode retCode = stdDevOpenAndFillBody(sp, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
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
