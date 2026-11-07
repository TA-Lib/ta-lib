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
 *  110726 MF,CC  Initial version (#572).
 */

   /**
    * Number of leading input bars {@link Core#sortino} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInMAR Minimum acceptable return per bar; returns below it are
    *        the downside (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int sortinoLookback( int optInTimePeriod, double optInMAR, double optInAnnualization )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInMAR == REAL_DEFAULT ) {
         optInMAR = 0e0;
      } else if( !(optInMAR >= REAL_MIN && optInMAR <= REAL_MAX) ) {
         return -1;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#sortino}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInMAR Minimum acceptable return per bar; returns below it are
    *        the downside (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int sortinoDisplayShift( int optInTimePeriod, double optInMAR, double optInAnnualization, int outputIdx )
   {
      if( sortinoLookback( optInTimePeriod, optInMAR, optInAnnualization ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode sortinoImpl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInMAR,
                        double optInAnnualization,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double periodTotal = 0;
      double downTotal = 0;
      double absTotal = 0;
      double peakDown = 0;
      double peakAbs = 0;
      double windowTotal = 0;
      double windowDown = 0;
      double windowAbs = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double invPeriod = 0;
      double sqrtAnnual = 0;
      double excess = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbBelow = 0;
      int nbAbove = 0;
      int windowBelow = 0;
      int windowAbove = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAR == REAL_DEFAULT ) {
         optInMAR = 0e0;
      } else if( !(optInMAR >= REAL_MIN && optInMAR <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      /* The window holds the returns of the bars after trailingIdx up to i; a
       * bar's return reads the price before it. A return needs a non-zero base
       * price and nothing more: a zero base gives a zero return.
       *
       * The counts of returns below and above the MAR are exact, so they, not
       * the running sums, decide when the window has no downside.
       */
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = inReal[trailingIdx];
      periodTotal = 0.0;
      downTotal = 0.0;
      absTotal = 0.0;
      nbBelow = 0;
      nbAbove = 0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakDown = downTotal;
      peakAbs = absTotal;
      do {
         price = inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
         peakDown = (downTotal > peakDown) ? downTotal : peakDown;
         peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
         windowTotal = periodTotal;
         windowDown = downTotal;
         windowAbs = absTotal;
         windowBelow = nbBelow;
         windowAbove = nbAbove;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write.
          */
         trailingPrevPrice = inReal[trailingIdx];
         trailingIdx += 1;
         price = inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         periodTotal -= ret;
         absTotal -= Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal -= dev * dev;
            nbBelow -= 1;
         } else if( dev > 0.0 ) {
            nbAbove -= 1;
         }
         /* Rebuild when the downside sum, or the sum of absolute returns, has
          * shrunk below 1% of the largest one held since the last rebuild, or at
          * least every 32 windows. The rounding a running sum carries scales
          * with that peak, so once a large return leaves, the mean or the
          * divisor can be all rounding.
          */
         barsSinceReseed -= 1;
         if( windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = inReal[windowStart - 1];
            windowTotal = 0.0;
            windowDown = 0.0;
            windowAbs = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               windowTotal += ret;
               windowAbs += Math.abs(ret);
               dev = ret - optInMAR;
               if( dev < 0.0 ) {
                  windowDown += dev * dev;
               }
            }
            peakDown = windowDown;
            peakAbs = windowAbs;
            /* Re-remove the trailing return so the carried state matches the
             * path without a rebuild; the counts never drift.
             */
            price = inReal[windowStart];
            ret = 0.0;
            if( inReal[windowStart - 1] != 0.0 ) {
               ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
            }
            periodTotal = windowTotal - ret;
            absTotal = windowAbs - Math.abs(ret);
            downTotal = windowDown;
            dev = ret - optInMAR;
            if( dev < 0.0 ) {
               downTotal -= dev * dev;
            }
         }
         /* No downside is the plain quotient (ruled on #572): the mean excess
          * is then positive exactly when some return beats the MAR, giving
          * +Inf, and zero when every return equals it, giving NaN. The count
          * stands in for the mean so rounding cannot flip either case.
          * Annualize the per-bar ratio last, so an annualization of 1 is that
          * ratio bit for bit.
          */
         excess = windowTotal * invPeriod - optInMAR;
         if( windowBelow == 0 ) {
            windowDown = 0.0;
            excess = (double)windowAbove;
         }
         ratio = excess / Math.sqrt(windowDown * invPeriod);
         outReal[outIdx++] = sqrtAnnual * ratio;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode sortinoImpl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInMAR,
                        double optInAnnualization,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double periodTotal = 0;
      double downTotal = 0;
      double absTotal = 0;
      double peakDown = 0;
      double peakAbs = 0;
      double windowTotal = 0;
      double windowDown = 0;
      double windowAbs = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double invPeriod = 0;
      double sqrtAnnual = 0;
      double excess = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbBelow = 0;
      int nbAbove = 0;
      int windowBelow = 0;
      int windowAbove = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAR == REAL_DEFAULT ) {
         optInMAR = 0e0;
      } else if( !(optInMAR >= REAL_MIN && optInMAR <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = (double)inReal[trailingIdx];
      periodTotal = 0.0;
      downTotal = 0.0;
      absTotal = 0.0;
      nbBelow = 0;
      nbAbove = 0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = (double)inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakDown = downTotal;
      peakAbs = absTotal;
      do {
         price = (double)inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
         peakDown = (downTotal > peakDown) ? downTotal : peakDown;
         peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
         windowTotal = periodTotal;
         windowDown = downTotal;
         windowAbs = absTotal;
         windowBelow = nbBelow;
         windowAbove = nbAbove;
         trailingPrevPrice = (double)inReal[trailingIdx];
         trailingIdx += 1;
         price = (double)inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         periodTotal -= ret;
         absTotal -= Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal -= dev * dev;
            nbBelow -= 1;
         } else if( dev > 0.0 ) {
            nbAbove -= 1;
         }
         barsSinceReseed -= 1;
         if( windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = (double)inReal[windowStart - 1];
            windowTotal = 0.0;
            windowDown = 0.0;
            windowAbs = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = (double)inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               windowTotal += ret;
               windowAbs += Math.abs(ret);
               dev = ret - optInMAR;
               if( dev < 0.0 ) {
                  windowDown += dev * dev;
               }
            }
            peakDown = windowDown;
            peakAbs = windowAbs;
            price = (double)inReal[windowStart];
            ret = 0.0;
            if( (double)inReal[windowStart - 1] != 0.0 ) {
               ret = (price - (double)inReal[windowStart - 1]) / (double)inReal[windowStart - 1];
            }
            periodTotal = windowTotal - ret;
            absTotal = windowAbs - Math.abs(ret);
            downTotal = windowDown;
            dev = ret - optInMAR;
            if( dev < 0.0 ) {
               downTotal -= dev * dev;
            }
         }
         excess = windowTotal * invPeriod - optInMAR;
         if( windowBelow == 0 ) {
            windowDown = 0.0;
            excess = (double)windowAbove;
         }
         ratio = excess / Math.sqrt(windowDown * invPeriod);
         outReal[outIdx++] = sqrtAnnual * ratio;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * The rolling Sortino ratio: the mean return in excess of a minimum
    * acceptable return (MAR), divided by the downside deviation, over a
    * trailing window, scaled to a year. It is the Sharpe ratio with upside
    * volatility taken out of the denominator: only returns below the MAR count
    * as risk. Above 0 the window's returns beat the MAR on average; the larger
    * the value, the more return per unit of downside.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sortino">ta-lib.org/functions/sortino</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The downside deviation divides by the full window {@code n}: a return above the MAR counts in {@code n} and adds zero to the sum. This is the definition of Sortino and Price (1994) and the divisor of Bacon's worked example; dividing by {@code n−1}, or by the number of returns below the MAR, gives a different value.</li>
    * <li>The mean is arithmetic, not compounded.</li>
    * <li>A window with no return below the MAR has no downside: the result is +Inf, and NaN when every return equals the MAR. In a rising market this is common. With an annualization of 0 it is NaN there and 0 elsewhere.</li>
    * <li>The MAR is per bar, in the same unit as the returns. For an annual rate {@code R}, pass {@code (1+R)^(1/A) − 1}, where {@code A} is the annualization; it is not de-annualized for you.</li>
    * <li>An annualization of 1 gives the raw per-bar ratio.</li>
    * <li>A zero price leaves no return to measure from it: the return of the bar after it counts as 0.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sortinoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The price series; its returns are computed internally.
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInMAR Minimum acceptable return per bar; returns below it are
    *        the downside (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outReal Annualized Sortino ratio of the trailing window, or +Inf or
    *        NaN where the window has no return below the MAR. Must hold at least
    *        {@code endIdx - max(startIdx, sortinoLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#sharpe
    * @see Core#stddev
    * @see Core#var
    * @see Core#rocp
    */
   public OutRange sortino( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInTimePeriod,
                            double optInMAR,
                            double optInAnnualization,
                            double outReal[] )
   {
      requireIndexRange("SORTINO", startIdx, endIdx);
      int guardStart = clampedStart("SORTINO", startIdx, sortinoLookback(optInTimePeriod, optInMAR, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SORTINO", "inReal", inReal, guardInLen);
      requireLength("SORTINO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sortinoImpl(startIdx, endIdx, inReal, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SORTINO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The rolling Sortino ratio: the mean return in excess of a minimum
    * acceptable return (MAR), divided by the downside deviation, over a
    * trailing window, scaled to a year. It is the Sharpe ratio with upside
    * volatility taken out of the denominator: only returns below the MAR count
    * as risk. Above 0 the window's returns beat the MAR on average; the larger
    * the value, the more return per unit of downside.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sortino">ta-lib.org/functions/sortino</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The downside deviation divides by the full window {@code n}: a return above the MAR counts in {@code n} and adds zero to the sum. This is the definition of Sortino and Price (1994) and the divisor of Bacon's worked example; dividing by {@code n−1}, or by the number of returns below the MAR, gives a different value.</li>
    * <li>The mean is arithmetic, not compounded.</li>
    * <li>A window with no return below the MAR has no downside: the result is +Inf, and NaN when every return equals the MAR. In a rising market this is common. With an annualization of 0 it is NaN there and 0 elsewhere.</li>
    * <li>The MAR is per bar, in the same unit as the returns. For an annual rate {@code R}, pass {@code (1+R)^(1/A) − 1}, where {@code A} is the annualization; it is not de-annualized for you.</li>
    * <li>An annualization of 1 gives the raw per-bar ratio.</li>
    * <li>A zero price leaves no return to measure from it: the return of the bar after it counts as 0.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sortinoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The price series; its returns are computed internally.
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInMAR Minimum acceptable return per bar; returns below it are
    *        the downside (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outReal Annualized Sortino ratio of the trailing window, or +Inf or
    *        NaN where the window has no return below the MAR. Must hold at least
    *        {@code endIdx - max(startIdx, sortinoLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#sharpe
    * @see Core#stddev
    * @see Core#var
    * @see Core#rocp
    */
   public OutRange sortino( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInTimePeriod,
                            double optInMAR,
                            double optInAnnualization,
                            double outReal[] )
   {
      requireIndexRange("SORTINO", startIdx, endIdx);
      int guardStart = clampedStart("SORTINO", startIdx, sortinoLookback(optInTimePeriod, optInMAR, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SORTINO", "inReal", inReal, guardInLen);
      requireLength("SORTINO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sortinoImpl(startIdx, endIdx, inReal, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SORTINO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SORTINO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#sortino} over the same series.
    * Open with {@link Core#sortinoOpen}; there is no close — the handle is
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
   public static final class SortinoStream {
      private Core core;
      private int optInTimePeriod;
      private double optInMAR;
      private double optInAnnualization;
      private double periodTotal;
      private double downTotal;
      private double absTotal;
      private double peakDown;
      private double peakAbs;
      private double prevPrice;
      private double invPeriod;
      private double sqrtAnnual;
      private int trailingIdx;
      private int barsSinceReseed;
      private int nbBelow;
      private int nbAbove;
      private int j;
      private int windowStart;
      private int i;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SortinoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#sortino} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SORTINO advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SortinoStream( SortinoStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMAR = other.optInMAR;
         this.optInAnnualization = other.optInAnnualization;
         this.periodTotal = other.periodTotal;
         this.downTotal = other.downTotal;
         this.absTotal = other.absTotal;
         this.peakDown = other.peakDown;
         this.peakAbs = other.peakAbs;
         this.prevPrice = other.prevPrice;
         this.invPeriod = other.invPeriod;
         this.sqrtAnnual = other.sqrtAnnual;
         this.trailingIdx = other.trailingIdx;
         this.barsSinceReseed = other.barsSinceReseed;
         this.nbBelow = other.nbBelow;
         this.nbAbove = other.nbAbove;
         this.j = other.j;
         this.windowStart = other.windowStart;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SORTINO update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SORTINO update", "inReal");
         core.sortinoStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SORTINO peek", "inReal");
         SortinoStream sp = this;
         double windowTotal = 0.0;
         double windowDown = 0.0;
         double windowAbs = 0.0;
         double price = 0.0;
         double trailingPrevPrice = 0.0;
         double windowPrevPrice = 0.0;
         double ret = 0.0;
         double dev = 0.0;
         double excess = 0.0;
         double ratio = 0.0;
         int windowBelow = 0;
         int windowAbove = 0;
         double absTotal = sp.absTotal;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         double downTotal = sp.downTotal;
         int j = sp.j;
         int nbAbove = sp.nbAbove;
         int nbBelow = sp.nbBelow;
         double peakAbs = sp.peakAbs;
         double peakDown = sp.peakDown;
         double periodTotal = sp.periodTotal;
         double prevPrice = sp.prevPrice;
         int trailingIdx = sp.trailingIdx;
         int windowStart = sp.windowStart;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.i & sp.xMask;
         pkVal0 = inReal;
         price = ((sp.i & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.i & sp.xMask] : pkVal0;
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - sp.optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
         peakDown = (downTotal > peakDown) ? downTotal : peakDown;
         peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
         windowTotal = periodTotal;
         windowDown = downTotal;
         windowAbs = absTotal;
         windowBelow = nbBelow;
         windowAbove = nbAbove;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write.
          */
         trailingPrevPrice = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         trailingIdx += 1;
         price = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         periodTotal -= ret;
         absTotal -= Math.abs(ret);
         dev = ret - sp.optInMAR;
         if( dev < 0.0 ) {
            downTotal -= dev * dev;
            nbBelow -= 1;
         } else if( dev > 0.0 ) {
            nbAbove -= 1;
         }
         /* Rebuild when the downside sum, or the sum of absolute returns, has
          * shrunk below 1% of the largest one held since the last rebuild, or at
          * least every 32 windows. The rounding a running sum carries scales
          * with that peak, so once a large return leaves, the mean or the
          * divisor can be all rounding.
          */
         barsSinceReseed -= 1;
         if( windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = sp.i - sp.optInTimePeriod + 1;
            windowPrevPrice = (((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
            windowTotal = 0.0;
            windowDown = 0.0;
            windowAbs = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               price = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               windowTotal += ret;
               windowAbs += Math.abs(ret);
               dev = ret - sp.optInMAR;
               if( dev < 0.0 ) {
                  windowDown += dev * dev;
               }
            }
            peakDown = windowDown;
            peakAbs = windowAbs;
            /* Re-remove the trailing return so the carried state matches the
             * path without a rebuild; the counts never drift.
             */
            price = ((windowStart & sp.xMask) != pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0;
            ret = 0.0;
            if( ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0) != 0.0 ) {
               ret = (price - ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0)) / ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0);
            }
            periodTotal = windowTotal - ret;
            absTotal = windowAbs - Math.abs(ret);
            downTotal = windowDown;
            dev = ret - sp.optInMAR;
            if( dev < 0.0 ) {
               downTotal -= dev * dev;
            }
         }
         /* No downside is the plain quotient (ruled on #572): the mean excess
          * is then positive exactly when some return beats the MAR, giving
          * +Inf, and zero when every return equals it, giving NaN. The count
          * stands in for the mean so rounding cannot flip either case.
          * Annualize the per-bar ratio last, so an annualization of 1 is that
          * ratio bit for bit.
          */
         excess = windowTotal * sp.invPeriod - sp.optInMAR;
         if( windowBelow == 0 ) {
            windowDown = 0.0;
            excess = (double)windowAbove;
         }
         ratio = excess / Math.sqrt(windowDown * sp.invPeriod);
         cur_outReal = sp.sqrtAnnual * ratio;
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
      public SortinoStream clone() {
         return new SortinoStream(this);
      }
   }
   private void sortinoStepImpl( SortinoStream sp, double inReal )
   {
      double windowTotal = 0.0;
      double windowDown = 0.0;
      double windowAbs = 0.0;
      double price = 0.0;
      double trailingPrevPrice = 0.0;
      double windowPrevPrice = 0.0;
      double ret = 0.0;
      double dev = 0.0;
      double excess = 0.0;
      double ratio = 0.0;
      int windowBelow = 0;
      int windowAbove = 0;
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      price = sp.x_inReal[sp.i & sp.xMask];
      ret = 0.0;
      if( sp.prevPrice != 0.0 ) {
         ret = (price - sp.prevPrice) / sp.prevPrice;
      }
      sp.prevPrice = price;
      sp.periodTotal += ret;
      sp.absTotal += Math.abs(ret);
      dev = ret - sp.optInMAR;
      if( dev < 0.0 ) {
         sp.downTotal += dev * dev;
         sp.nbBelow += 1;
      } else if( dev > 0.0 ) {
         sp.nbAbove += 1;
      }
      sp.peakDown = (sp.downTotal > sp.peakDown) ? sp.downTotal : sp.peakDown;
      sp.peakAbs = (sp.absTotal > sp.peakAbs) ? sp.absTotal : sp.peakAbs;
      windowTotal = sp.periodTotal;
      windowDown = sp.downTotal;
      windowAbs = sp.absTotal;
      windowBelow = sp.nbBelow;
      windowAbove = sp.nbAbove;
      /* inReal and outReal may be the same buffer, and the trailing prices
       * sit in slots the output reaches: every read of the window happens
       * before this bar's write.
       */
      trailingPrevPrice = sp.x_inReal[sp.trailingIdx & sp.xMask];
      sp.trailingIdx += 1;
      price = sp.x_inReal[sp.trailingIdx & sp.xMask];
      ret = 0.0;
      if( trailingPrevPrice != 0.0 ) {
         ret = (price - trailingPrevPrice) / trailingPrevPrice;
      }
      sp.periodTotal -= ret;
      sp.absTotal -= Math.abs(ret);
      dev = ret - sp.optInMAR;
      if( dev < 0.0 ) {
         sp.downTotal -= dev * dev;
         sp.nbBelow -= 1;
      } else if( dev > 0.0 ) {
         sp.nbAbove -= 1;
      }
      /* Rebuild when the downside sum, or the sum of absolute returns, has
       * shrunk below 1% of the largest one held since the last rebuild, or at
       * least every 32 windows. The rounding a running sum carries scales
       * with that peak, so once a large return leaves, the mean or the
       * divisor can be all rounding.
       */
      sp.barsSinceReseed -= 1;
      if( windowDown < 0.01 * sp.peakDown || windowAbs < 0.01 * sp.peakAbs || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         sp.windowStart = sp.i - sp.optInTimePeriod + 1;
         windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
         windowTotal = 0.0;
         windowDown = 0.0;
         windowAbs = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            price = sp.x_inReal[sp.j & sp.xMask];
            ret = 0.0;
            if( windowPrevPrice != 0.0 ) {
               ret = (price - windowPrevPrice) / windowPrevPrice;
            }
            windowPrevPrice = price;
            windowTotal += ret;
            windowAbs += Math.abs(ret);
            dev = ret - sp.optInMAR;
            if( dev < 0.0 ) {
               windowDown += dev * dev;
            }
         }
         sp.peakDown = windowDown;
         sp.peakAbs = windowAbs;
         /* Re-remove the trailing return so the carried state matches the
          * path without a rebuild; the counts never drift.
          */
         price = sp.x_inReal[sp.windowStart & sp.xMask];
         ret = 0.0;
         if( sp.x_inReal[(sp.windowStart - 1) & sp.xMask] != 0.0 ) {
            ret = (price - sp.x_inReal[(sp.windowStart - 1) & sp.xMask]) / sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
         }
         sp.periodTotal = windowTotal - ret;
         sp.absTotal = windowAbs - Math.abs(ret);
         sp.downTotal = windowDown;
         dev = ret - sp.optInMAR;
         if( dev < 0.0 ) {
            sp.downTotal -= dev * dev;
         }
      }
      /* No downside is the plain quotient (ruled on #572): the mean excess
       * is then positive exactly when some return beats the MAR, giving
       * +Inf, and zero when every return equals it, giving NaN. The count
       * stands in for the mean so rounding cannot flip either case.
       * Annualize the per-bar ratio last, so an annualization of 1 is that
       * ratio bit for bit.
       */
      excess = windowTotal * sp.invPeriod - sp.optInMAR;
      if( windowBelow == 0 ) {
         windowDown = 0.0;
         excess = (double)windowAbove;
      }
      ratio = excess / Math.sqrt(windowDown * sp.invPeriod);
      sp.cur_outReal = sp.sqrtAnnual * ratio;
      sp.i += 1;
   }
   private RetCode sortinoOpenImpl( SortinoStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInMAR, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double periodTotal = 0;
      double downTotal = 0;
      double absTotal = 0;
      double peakDown = 0;
      double peakAbs = 0;
      double windowTotal = 0;
      double windowDown = 0;
      double windowAbs = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double invPeriod = 0;
      double sqrtAnnual = 0;
      double excess = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbBelow = 0;
      int nbAbove = 0;
      int windowBelow = 0;
      int windowAbove = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAR == REAL_DEFAULT ) {
         optInMAR = 0e0;
      } else if( !(optInMAR >= REAL_MIN && optInMAR <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx >= historyLen ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      /* The window holds the returns of the bars after trailingIdx up to i; a
       * bar's return reads the price before it. A return needs a non-zero base
       * price and nothing more: a zero base gives a zero return.
       *
       * The counts of returns below and above the MAR are exact, so they, not
       * the running sums, decide when the window has no downside.
       */
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = inReal[trailingIdx];
      periodTotal = 0.0;
      downTotal = 0.0;
      absTotal = 0.0;
      nbBelow = 0;
      nbAbove = 0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakDown = downTotal;
      peakAbs = absTotal;
      do {
         price = inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         periodTotal += ret;
         absTotal += Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal += dev * dev;
            nbBelow += 1;
         } else if( dev > 0.0 ) {
            nbAbove += 1;
         }
         peakDown = (downTotal > peakDown) ? downTotal : peakDown;
         peakAbs = (absTotal > peakAbs) ? absTotal : peakAbs;
         windowTotal = periodTotal;
         windowDown = downTotal;
         windowAbs = absTotal;
         windowBelow = nbBelow;
         windowAbove = nbAbove;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write.
          */
         trailingPrevPrice = inReal[trailingIdx];
         trailingIdx += 1;
         price = inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         periodTotal -= ret;
         absTotal -= Math.abs(ret);
         dev = ret - optInMAR;
         if( dev < 0.0 ) {
            downTotal -= dev * dev;
            nbBelow -= 1;
         } else if( dev > 0.0 ) {
            nbAbove -= 1;
         }
         /* Rebuild when the downside sum, or the sum of absolute returns, has
          * shrunk below 1% of the largest one held since the last rebuild, or at
          * least every 32 windows. The rounding a running sum carries scales
          * with that peak, so once a large return leaves, the mean or the
          * divisor can be all rounding.
          */
         barsSinceReseed -= 1;
         if( windowDown < 0.01 * peakDown || windowAbs < 0.01 * peakAbs || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = inReal[windowStart - 1];
            windowTotal = 0.0;
            windowDown = 0.0;
            windowAbs = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               windowTotal += ret;
               windowAbs += Math.abs(ret);
               dev = ret - optInMAR;
               if( dev < 0.0 ) {
                  windowDown += dev * dev;
               }
            }
            peakDown = windowDown;
            peakAbs = windowAbs;
            /* Re-remove the trailing return so the carried state matches the
             * path without a rebuild; the counts never drift.
             */
            price = inReal[windowStart];
            ret = 0.0;
            if( inReal[windowStart - 1] != 0.0 ) {
               ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
            }
            periodTotal = windowTotal - ret;
            absTotal = windowAbs - Math.abs(ret);
            downTotal = windowDown;
            dev = ret - optInMAR;
            if( dev < 0.0 ) {
               downTotal -= dev * dev;
            }
         }
         /* No downside is the plain quotient (ruled on #572): the mean excess
          * is then positive exactly when some return beats the MAR, giving
          * +Inf, and zero when every return equals it, giving NaN. The count
          * stands in for the mean so rounding cannot flip either case.
          * Annualize the per-bar ratio last, so an annualization of 1 is that
          * ratio bit for bit.
          */
         excess = windowTotal * invPeriod - optInMAR;
         if( windowBelow == 0 ) {
            windowDown = 0.0;
            excess = (double)windowAbove;
         }
         ratio = excess / Math.sqrt(windowDown * invPeriod);
         outReal[outIdx++ * outStride] = sqrtAnnual * ratio;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMAR = optInMAR;
      sp.optInAnnualization = optInAnnualization;
      sp.periodTotal = periodTotal;
      sp.downTotal = downTotal;
      sp.absTotal = absTotal;
      sp.peakDown = peakDown;
      sp.peakAbs = peakAbs;
      sp.prevPrice = prevPrice;
      sp.invPeriod = invPeriod;
      sp.sqrtAnnual = sqrtAnnual;
      sp.trailingIdx = trailingIdx;
      sp.barsSinceReseed = barsSinceReseed;
      sp.nbBelow = nbBelow;
      sp.nbAbove = nbAbove;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* sortinoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SortinoStream sortinoOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInMAR, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SortinoStream sp = new SortinoStream(this);
      RetCode retCode = sortinoOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SORTINO openAndFill", inReal.length, startIdx, sortinoLookback(optInTimePeriod, optInMAR, optInAnnualization));
      }
      throw streamFailure("SORTINO openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind sortinoOpen (composition seam). */
   SortinoStream sortinoOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInMAR, double optInAnnualization )
   {
      SortinoStream sp = new SortinoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = sortinoOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SORTINO open", inReal.length, startIdx, sortinoLookback(optInTimePeriod, optInMAR, optInAnnualization));
      }
      throw streamFailure("SORTINO open", retCode);
   }
   /**
    * Open a live SORTINO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#sortino} at that bar.
    * <p>The history must hold at least {@code sortinoLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public SortinoStream sortinoOpen( double inReal[], int optInTimePeriod, double optInMAR, double optInAnnualization )
   {
      requireArgument("SORTINO open", "inReal", inReal);
      requireHistory("SORTINO open", inReal.length);
      return sortinoOpenInternal(inReal, 0, optInTimePeriod, optInMAR, optInAnnualization);
   }
   /**
    * {@link Core#sortinoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#sortino} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SortinoStream#outRange()}.
    */
   public SortinoStream sortinoOpenAndFill( double inReal[], int optInTimePeriod, double optInMAR, double optInAnnualization, double outReal[] )
   {
      requireArgument("SORTINO openAndFill", "inReal", inReal);
      requireHistory("SORTINO openAndFill", inReal.length);
      int guardOutLen = openFillCount("SORTINO openAndFill", inReal.length, sortinoLookback(optInTimePeriod, optInMAR, optInAnnualization));
      requireLength("SORTINO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SORTINO openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return sortinoOpenAndFillInternal(inReal, 0, optInTimePeriod, optInMAR, optInAnnualization, outBegIdx, outNBElement, outReal);
   }
