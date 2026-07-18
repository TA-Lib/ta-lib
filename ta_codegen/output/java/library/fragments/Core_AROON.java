/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

   public int aroonLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   public RetCode aroon( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outAroonDown[],
                         double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
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
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode aroonUnguarded( int startIdx,
                                  int endIdx,
                                  double inHigh[],
                                  double inLow[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outAroonDown[],
                                  double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode aroon( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outAroonDown[],
                         double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
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
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode aroonUnguarded( int startIdx,
                                  int endIdx,
                                  float inHigh[],
                                  float inLow[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outAroonDown[],
                                  double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live AROON stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#aroon} over the same series.
    * Open with {@link Core#aroonOpen}; there is no close — the handle is
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
   public static final class AroonStream {
      final Core core;
      int optInTimePeriod;
      double lowest;
      double highest;
      double factor;
      int trailingIdx;
      int lowestIdx;
      int highestIdx;
      int i;
      int today;
      int xCap;
      double[] x_inHigh;
      double[] x_inLow;
      double cur_outAroonDown;
      double cur_outAroonUp;
      Value cachedValue;

      AroonStream( Core core ) { this.core = core; }

      AroonStream( AroonStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.factor = other.factor;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xCap = other.xCap;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.cur_outAroonDown = other.cur_outAroonDown;
         this.cur_outAroonUp = other.cur_outAroonUp;
         this.cachedValue = other.cachedValue;
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double aroonDown;
         public final double aroonUp;
         Value( double aroonDown, double aroonUp ) {
            this.aroonDown = aroonDown;
            this.aroonUp = aroonUp;
         }
         @Override public String toString() {
            return "Value[" + "aroonDown=" + aroonDown + ", " + "aroonUp=" + aroonUp + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.aroonDown) == Double.doubleToLongBits(v.aroonDown) && Double.doubleToLongBits(this.aroonUp) == Double.doubleToLongBits(v.aroonUp);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(aroonDown);
            h = 31 * h + Double.hashCode(aroonUp);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inHigh, double inLow ) {
         core.aroonStreamStep(this, inHigh, inLow);
         this.cachedValue = new Value(this.cur_outAroonDown, this.cur_outAroonUp);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inHigh, double inLow ) {
         AroonStream scratch = new AroonStream(this);
         core.aroonStreamStep(scratch, inHigh, inLow);
         return new Value(scratch.cur_outAroonDown, scratch.cur_outAroonUp);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public AroonStream copy() {
         return new AroonStream(this);
      }
   }
   void aroonStreamStep( AroonStream sp, double inHigh, double inLow )
   {
      double tmp = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = (sp.trailingIdx / sp.xCap) * sp.xCap;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inHigh[sp.today % sp.xCap] = inHigh;
      sp.x_inLow[sp.today % sp.xCap] = inLow;
      /* Keep track of the lowestIdx */
      tmp = sp.x_inLow[sp.today % sp.xCap];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx % sp.xCap];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i % sp.xCap];
            if( tmp <= sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      /* Keep track of the highestIdx */
      tmp = sp.x_inHigh[sp.today % sp.xCap];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx % sp.xCap];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i % sp.xCap];
            if( tmp >= sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      /* Note: Do not forget that input and output buffer can be the same,
       *       so writing to the output is the last thing being done here.
       */
      sp.cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - sp.highestIdx));
      sp.cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - sp.lowestIdx));
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode aroonOpenBody( AroonStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outAroonDown = 0.0;
      double lastValue_outAroonUp = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         lastValue_outAroonUp = factor * (optInTimePeriod - (today - highestIdx));
         lastValue_outAroonDown = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capX_inHigh = new double[capX];
      double[] capX_inLow = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ % capX] = inHigh[fillJ];
         capX_inLow[fillJ % capX] = inLow[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.factor = factor;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outAroonDown = lastValue_outAroonDown;
      sp.cur_outAroonUp = lastValue_outAroonUp;
      sp.cachedValue = new AroonStream.Value(sp.cur_outAroonDown, sp.cur_outAroonUp);
      return RetCode.Success;
   }
   private RetCode aroonOpenAndFillBody( AroonStream sp, double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outAroonDown == (Object)inHigh || (Object)outAroonDown == (Object)inLow || (Object)outAroonUp == (Object)inHigh || (Object)outAroonUp == (Object)inLow || (Object)outAroonDown == (Object)outAroonUp ) {
         return RetCode.BadParam;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capX_inHigh = new double[capX];
      double[] capX_inLow = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ % capX] = inHigh[fillJ];
         capX_inLow[fillJ % capX] = inLow[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.factor = factor;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outAroonDown = outAroonDown[outNBElement.value - 1];
      sp.cur_outAroonUp = outAroonUp[outNBElement.value - 1];
      sp.cachedValue = new AroonStream.Value(sp.cur_outAroonDown, sp.cur_outAroonUp);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind aroonOpen (composition seam). */
   AroonStream aroonOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      AroonStream sp = new AroonStream(this);
      RetCode retCode = aroonOpenBody(sp, inHigh, inLow, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AROON open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AROON open: internal error");
      }
      throw new IllegalArgumentException("TA_AROON open: " + retCode);
   }
   /**
    * Open a live AROON stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#aroon} at that bar.
    * <p>The history must hold at least {@code aroonLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AroonStream aroonOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      return aroonOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#aroonOpen} that also fills the output array(s) bit-identically
    * to {@link Core#aroon} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public AroonStream aroonOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[] )
   {
      AroonStream sp = new AroonStream(this);
      RetCode retCode = aroonOpenAndFillBody(sp, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_AROON openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_AROON openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_AROON openAndFill: " + retCode);
   }
