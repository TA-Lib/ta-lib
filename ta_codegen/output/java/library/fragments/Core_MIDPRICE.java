/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Speed optimization: for periods above 20, cache the
 *                highest/lowest index instead of rescanning the window
 *                on every bar (same approach as MIN/MAX/WILLR). Smaller
 *                periods keep the simple scan, which auto-vectorizes
 *                and is faster there. Both paths produce identical
 *                output.
 */

   public int midPriceLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode midPrice( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
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
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
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
       *
       * Two equivalent algorithms, picked by period. Their outputs are
       * bit-identical; only the scan strategy differs:
       *
       * - Small periods (<= 20): rescan the whole window on every bar.
       *   The two independent comparison chains auto-vectorize on modern
       *   compilers, which beats any per-bar bookkeeping while the window
       *   is short. The threshold sits near the measured crossover
       *   (~period 19-20 with gcc/clang -O3 on x86-64).
       *
       * - Larger periods: cache the highest high/lowest low with its
       *   index; a rescan of the window is needed only when the cached
       *   extremum drops out of the window (amortized O(1) per bar
       *   instead of O(period)).
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = inLow[trailingIdx];
            highest = inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = inHigh[today];
            tmpLow = inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPriceUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = inLow[trailingIdx];
            highest = inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = inHigh[today];
            tmpLow = inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPrice( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            int optInTimePeriod,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
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
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = (double)inLow[trailingIdx];
            highest = (double)inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = (double)inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = (double)inHigh[today];
            tmpLow = (double)inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = (double)inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = (double)inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = (double)inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = (double)inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode midPriceUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     int optInTimePeriod,
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod <= 20 ) {
         while( today <= endIdx ) {
            lowest = (double)inLow[trailingIdx];
            highest = (double)inHigh[trailingIdx];
            trailingIdx += 1;
            for( i = trailingIdx; i <= today; i += 1 ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
               }
               tmpHigh = (double)inHigh[i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            today += 1;
         }
      } else {
         highestIdx = 0 - 1;
         highest = 0.0;
         lowestIdx = 0 - 1;
         lowest = 0.0;
         while( today <= endIdx ) {
            tmpHigh = (double)inHigh[today];
            tmpLow = (double)inLow[today];
            if( highestIdx < trailingIdx ) {
               highestIdx = trailingIdx;
               highest = (double)inHigh[highestIdx];
               i = highestIdx;
               while( ++i <= today ) {
                  tmpHigh = (double)inHigh[i];
                  if( tmpHigh > highest ) {
                     highestIdx = i;
                     highest = tmpHigh;
                  }
               }
            } else if( tmpHigh >= highest ) {
               highestIdx = today;
               highest = tmpHigh;
            }
            if( lowestIdx < trailingIdx ) {
               lowestIdx = trailingIdx;
               lowest = (double)inLow[lowestIdx];
               i = lowestIdx;
               while( ++i <= today ) {
                  tmpLow = (double)inLow[i];
                  if( tmpLow < lowest ) {
                     lowestIdx = i;
                     lowest = tmpLow;
                  }
               }
            } else if( tmpLow <= lowest ) {
               lowestIdx = today;
               lowest = tmpLow;
            }
            outReal[outIdx++] = (highest + lowest) / 2.0;
            trailingIdx += 1;
            today += 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live MIDPRICE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#midPrice} over the same series.
    * Open with {@link Core#midPriceOpen}; there is no close — the handle is
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
   public static final class MidPriceStream {
      final Core core;
      int optInTimePeriod;
      double lowest;
      double highest;
      int trailingIdx;
      int lowestIdx;
      int highestIdx;
      int i;
      int today;
      int xCap;
      double[] x_inHigh;
      double[] x_inLow;
      double cur_outReal;

      MidPriceStream( Core core ) { this.core = core; }

      MidPriceStream( MidPriceStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xCap = other.xCap;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow ) {
         core.midPriceStreamStep(this, inHigh, inLow);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow ) {
         MidPriceStream scratch = new MidPriceStream(this);
         core.midPriceStreamStep(scratch, inHigh, inLow);
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
      public MidPriceStream copy() {
         return new MidPriceStream(this);
      }
   }
   void midPriceStreamStep( MidPriceStream sp, double inHigh, double inLow )
   {
      double tmpLow = 0.0;
      double tmpHigh = 0.0;
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
      tmpHigh = sp.x_inHigh[sp.today % sp.xCap];
      tmpLow = sp.x_inLow[sp.today % sp.xCap];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx % sp.xCap];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmpHigh = sp.x_inHigh[sp.i % sp.xCap];
            if( tmpHigh > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmpHigh;
            }
         }
      } else if( tmpHigh >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmpHigh;
      }
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx % sp.xCap];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmpLow = sp.x_inLow[sp.i % sp.xCap];
            if( tmpLow < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmpLow;
            }
         }
      } else if( tmpLow <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmpLow;
      }
      sp.cur_outReal = (sp.highest + sp.lowest) / 2.0;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode midPriceOpenBody( MidPriceStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
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
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
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
       *
       * Two equivalent algorithms, picked by period. Their outputs are
       * bit-identical; only the scan strategy differs:
       *
       * - Small periods (<= 20): rescan the whole window on every bar.
       *   The two independent comparison chains auto-vectorize on modern
       *   compilers, which beats any per-bar bookkeeping while the window
       *   is short. The threshold sits near the measured crossover
       *   (~period 19-20 with gcc/clang -O3 on x86-64).
       *
       * - Larger periods: cache the highest high/lowest low with its
       *   index; a rescan of the window is needed only when the cached
       *   extremum drops out of the window (amortized O(1) per bar
       *   instead of O(period)).
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = today;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         lastValue_outReal = (highest + lowest) / 2.0;
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
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode midPriceOpenAndFillBody( MidPriceStream sp, double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
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
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         return RetCode.BadParam;
      }
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
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
       *
       * Two equivalent algorithms, picked by period. Their outputs are
       * bit-identical; only the scan strategy differs:
       *
       * - Small periods (<= 20): rescan the whole window on every bar.
       *   The two independent comparison chains auto-vectorize on modern
       *   compilers, which beats any per-bar bookkeeping while the window
       *   is short. The threshold sits near the measured crossover
       *   (~period 19-20 with gcc/clang -O3 on x86-64).
       *
       * - Larger periods: cache the highest high/lowest low with its
       *   index; a rescan of the window is needed only when the cached
       *   extremum drops out of the window (amortized O(1) per bar
       *   instead of O(period)).
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = today;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outReal[outIdx++] = (highest + lowest) / 2.0;
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
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind midPriceOpen (composition seam). */
   MidPriceStream midPriceOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      MidPriceStream sp = new MidPriceStream(this);
      RetCode retCode = midPriceOpenBody(sp, inHigh, inLow, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MIDPRICE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MIDPRICE open: internal error");
      }
      throw new IllegalArgumentException("TA_MIDPRICE open: " + retCode);
   }
   /**
    * Open a live MIDPRICE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#midPrice} at that bar.
    * <p>The history must hold at least {@code midPriceLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MidPriceStream midPriceOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      return midPriceOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#midPriceOpen} that also fills the output array(s) bit-identically
    * to {@link Core#midPrice} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MidPriceStream midPriceOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MidPriceStream sp = new MidPriceStream(this);
      RetCode retCode = midPriceOpenAndFillBody(sp, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MIDPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MIDPRICE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MIDPRICE openAndFill: " + retCode);
   }
