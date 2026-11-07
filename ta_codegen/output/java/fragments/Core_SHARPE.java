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
 *  110626 MF,CC  Initial version (#571).
 */

   /**
    * Number of leading input bars {@link Core#sharpe} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRiskFreeRate Risk-free return per bar, subtracted from each
    *        return (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int sharpeLookback( int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInRiskFreeRate == REAL_DEFAULT ) {
         optInRiskFreeRate = 0e0;
      } else if( !(optInRiskFreeRate >= REAL_MIN && optInRiskFreeRate <= REAL_MAX) ) {
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
    * computed it a chart draws one output of {@link Core#sharpe}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRiskFreeRate Risk-free return per bar, subtracted from each
    *        return (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int sharpeDisplayShift( int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization, int outputIdx )
   {
      if( sharpeLookback( optInTimePeriod, optInRiskFreeRate, optInAnnualization ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode sharpeImpl( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double optInRiskFreeRate,
                       double optInAnnualization,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double peakTotal2 = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double dPeriod = 0;
      double invPeriod = 0;
      double meanDev = 0;
      double moment2 = 0;
      double excess = 0;
      double sqrtAnnual = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
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
      if( optInRiskFreeRate == REAL_DEFAULT ) {
         optInRiskFreeRate = 0e0;
      } else if( !(optInRiskFreeRate >= REAL_MIN && optInRiskFreeRate <= REAL_MAX) ) {
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
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      /* The window holds the returns of the bars after trailingIdx up to i; a
       * bar's return reads the price before it.
       *
       * The variance of the excess returns is the variance of the returns, so
       * the risk-free rate enters only the mean. That keeps the sums at return
       * scale whatever the rate, and lets a flat price window reach an exact
       * zero spread.
       */
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = inReal[trailingIdx];
      /* A return needs a non-zero base price and nothing more: the test is
       * exact, never a band, since a price carries the quote unit. A zero base
       * gives a zero return.
       */
      price = inReal[trailingIdx + 1];
      ret = 0.0;
      if( prevPrice != 0.0 ) {
         ret = (price - prevPrice) / prevPrice;
      }
      shift = ret;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         price = inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanDev = periodTotal1 * invPeriod;
         moment2 = periodTotal2 - meanDev * periodTotal1;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write. The oldest price read is the trailing
          * return's base, so the index stays on it.
          */
         trailingPrevPrice = inReal[trailingIdx];
         trailingIdx += 1;
         price = inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         dev = ret - shift;
         periodTotal1 -= dev;
         periodTotal2 -= dev * dev;
         /* Rebuild against the window mean when the spread has shrunk below 1% of
          * the largest sum of squares held since the last rebuild, or at least
          * every 32 windows. The rounding the running sums carry scales with that
          * peak, so once a large return leaves, the current sum can be all
          * rounding; a flat run after a move must still come out exactly flat.
          * The spread is a divisor, so its relative error reaches the ratio.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = inReal[windowStart - 1];
            dev = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev += ret;
            }
            shift = dev * invPeriod;
            windowPrevPrice = inReal[windowStart - 1];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev = ret - shift;
               periodTotal1 += dev;
               periodTotal2 += dev * dev;
            }
            meanDev = periodTotal1 * invPeriod;
            moment2 = periodTotal2 - meanDev * periodTotal1;
            /* A window of equal returns can sit off this mean by rounding, which
             * leaves a spread of rounding where the ratio must be the plain
             * quotient, and fires the trigger again on every bar. Anchored on
             * one of its own returns, its deviations are exactly zero.
             */
            if( moment2 < 0.000001 * periodTotal2 ) {
               shift = ret;
               windowPrevPrice = inReal[windowStart - 1];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  price = inReal[j];
                  ret = 0.0;
                  if( windowPrevPrice != 0.0 ) {
                     ret = (price - windowPrevPrice) / windowPrevPrice;
                  }
                  windowPrevPrice = price;
                  dev = ret - shift;
                  periodTotal1 += dev;
                  periodTotal2 += dev * dev;
               }
               meanDev = periodTotal1 * invPeriod;
               moment2 = periodTotal2 - meanDev * periodTotal1;
            }
            peakTotal2 = periodTotal2;
            /* Re-remove the trailing return under the new shift so the carried
             * state matches the path without a rebuild.
             */
            price = inReal[windowStart];
            ret = 0.0;
            if( inReal[windowStart - 1] != 0.0 ) {
               ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
            }
            dev = ret - shift;
            periodTotal1 -= dev;
            periodTotal2 -= dev * dev;
         }
         /* No spread is the plain quotient: +/-Inf by the sign of the excess
          * mean, NaN when it is exactly zero (ruled on #571). Annualize the
          * per-bar ratio last, so an annualization of 1 is that ratio bit for
          * bit.
          */
         excess = shift + meanDev - optInRiskFreeRate;
         ratio = excess / Math.sqrt(moment2 / (dPeriod - 1.0));
         outReal[outIdx++] = sqrtAnnual * ratio;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode sharpeImpl( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double optInRiskFreeRate,
                       double optInAnnualization,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double peakTotal2 = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double dPeriod = 0;
      double invPeriod = 0;
      double meanDev = 0;
      double moment2 = 0;
      double excess = 0;
      double sqrtAnnual = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
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
      if( optInRiskFreeRate == REAL_DEFAULT ) {
         optInRiskFreeRate = 0e0;
      } else if( !(optInRiskFreeRate >= REAL_MIN && optInRiskFreeRate <= REAL_MAX) ) {
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
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = (double)inReal[trailingIdx];
      price = (double)inReal[trailingIdx + 1];
      ret = 0.0;
      if( prevPrice != 0.0 ) {
         ret = (price - prevPrice) / prevPrice;
      }
      shift = ret;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = (double)inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         price = (double)inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanDev = periodTotal1 * invPeriod;
         moment2 = periodTotal2 - meanDev * periodTotal1;
         trailingPrevPrice = (double)inReal[trailingIdx];
         trailingIdx += 1;
         price = (double)inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         dev = ret - shift;
         periodTotal1 -= dev;
         periodTotal2 -= dev * dev;
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = (double)inReal[windowStart - 1];
            dev = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = (double)inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev += ret;
            }
            shift = dev * invPeriod;
            windowPrevPrice = (double)inReal[windowStart - 1];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = (double)inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev = ret - shift;
               periodTotal1 += dev;
               periodTotal2 += dev * dev;
            }
            meanDev = periodTotal1 * invPeriod;
            moment2 = periodTotal2 - meanDev * periodTotal1;
            if( moment2 < 0.000001 * periodTotal2 ) {
               shift = ret;
               windowPrevPrice = (double)inReal[windowStart - 1];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  price = (double)inReal[j];
                  ret = 0.0;
                  if( windowPrevPrice != 0.0 ) {
                     ret = (price - windowPrevPrice) / windowPrevPrice;
                  }
                  windowPrevPrice = price;
                  dev = ret - shift;
                  periodTotal1 += dev;
                  periodTotal2 += dev * dev;
               }
               meanDev = periodTotal1 * invPeriod;
               moment2 = periodTotal2 - meanDev * periodTotal1;
            }
            peakTotal2 = periodTotal2;
            price = (double)inReal[windowStart];
            ret = 0.0;
            if( (double)inReal[windowStart - 1] != 0.0 ) {
               ret = (price - (double)inReal[windowStart - 1]) / (double)inReal[windowStart - 1];
            }
            dev = ret - shift;
            periodTotal1 -= dev;
            periodTotal2 -= dev * dev;
         }
         excess = shift + meanDev - optInRiskFreeRate;
         ratio = excess / Math.sqrt(moment2 / (dPeriod - 1.0));
         outReal[outIdx++] = sqrtAnnual * ratio;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * The rolling Sharpe ratio: the mean return in excess of a risk-free rate,
    * divided by the standard deviation of that excess return, over a trailing
    * window. A drift-over-volatility measure of the series itself, scaled to a
    * year. Above 0 the window's returns beat the risk-free rate on average; the
    * larger the value, the more return per unit of volatility.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sharpe">ta-lib.org/functions/sharpe</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is Sharpe's 1994 <i>ex post</i> ratio with a constant benchmark, over simple returns and the sample ({@code n−1}) standard deviation.</li>
    * <li>A window whose returns are all equal has no spread: the result is +Inf or -Inf by the sign of the mean excess return, and NaN when that mean is exactly 0. With an annualization of 0 it is NaN there and 0 elsewhere.</li>
    * <li>The risk-free rate is per bar, in the same unit as the returns. For an annual rate {@code R}, pass {@code (1+R)^(1/A) − 1}, where {@code A} is the annualization; it is not de-annualized for you.</li>
    * <li>An annualization of 1 gives the raw per-bar ratio.</li>
    * <li>A zero price leaves no return to measure from it: the return of the bar after it counts as 0.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sharpeLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The price series; its returns are computed internally.
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRiskFreeRate Risk-free return per bar, subtracted from each
    *        return (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outReal Annualized Sharpe ratio of the trailing window, or ±Inf or
    *        NaN where the window has no spread. Must hold at least
    *        {@code endIdx - max(startIdx, sharpeLookback(...)) + 1} values, and never
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
    * @see Core#stddev
    * @see Core#var
    * @see Core#beta
    * @see Core#rocp
    */
   public OutRange sharpe( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double optInRiskFreeRate,
                           double optInAnnualization,
                           double outReal[] )
   {
      requireIndexRange("SHARPE", startIdx, endIdx);
      int guardStart = clampedStart("SHARPE", startIdx, sharpeLookback(optInTimePeriod, optInRiskFreeRate, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SHARPE", "inReal", inReal, guardInLen);
      requireLength("SHARPE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sharpeImpl(startIdx, endIdx, inReal, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SHARPE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The rolling Sharpe ratio: the mean return in excess of a risk-free rate,
    * divided by the standard deviation of that excess return, over a trailing
    * window. A drift-over-volatility measure of the series itself, scaled to a
    * year. Above 0 the window's returns beat the risk-free rate on average; the
    * larger the value, the more return per unit of volatility.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sharpe">ta-lib.org/functions/sharpe</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>This is Sharpe's 1994 <i>ex post</i> ratio with a constant benchmark, over simple returns and the sample ({@code n−1}) standard deviation.</li>
    * <li>A window whose returns are all equal has no spread: the result is +Inf or -Inf by the sign of the mean excess return, and NaN when that mean is exactly 0. With an annualization of 0 it is NaN there and 0 elsewhere.</li>
    * <li>The risk-free rate is per bar, in the same unit as the returns. For an annual rate {@code R}, pass {@code (1+R)^(1/A) − 1}, where {@code A} is the annualization; it is not de-annualized for you.</li>
    * <li>An annualization of 1 gives the raw per-bar ratio.</li>
    * <li>A zero price leaves no return to measure from it: the return of the bar after it counts as 0.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sharpeLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The price series; its returns are computed internally.
    * @param optInTimePeriod Number of returns in the window; one price more is
    *        read (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRiskFreeRate Risk-free return per bar, subtracted from each
    *        return (default 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInAnnualization Bars per year; the per-bar ratio is multiplied
    *        by its square root (default 252; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param outReal Annualized Sharpe ratio of the trailing window, or ±Inf or
    *        NaN where the window has no spread. Must hold at least
    *        {@code endIdx - max(startIdx, sharpeLookback(...)) + 1} values, and never
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
    * @see Core#stddev
    * @see Core#var
    * @see Core#beta
    * @see Core#rocp
    */
   public OutRange sharpe( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double optInRiskFreeRate,
                           double optInAnnualization,
                           double outReal[] )
   {
      requireIndexRange("SHARPE", startIdx, endIdx);
      int guardStart = clampedStart("SHARPE", startIdx, sharpeLookback(optInTimePeriod, optInRiskFreeRate, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SHARPE", "inReal", inReal, guardInLen);
      requireLength("SHARPE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sharpeImpl(startIdx, endIdx, inReal, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SHARPE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SHARPE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#sharpe} over the same series.
    * Open with {@link Core#sharpeOpen}; there is no close — the handle is
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
   public static final class SharpeStream {
      private Core core;
      private int optInTimePeriod;
      private double optInRiskFreeRate;
      private double optInAnnualization;
      private double shift;
      private double periodTotal1;
      private double periodTotal2;
      private double peakTotal2;
      private double prevPrice;
      private double dPeriod;
      private double invPeriod;
      private double sqrtAnnual;
      private int trailingIdx;
      private int barsSinceReseed;
      private int j;
      private int windowStart;
      private int i;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SharpeStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#sharpe} reports over the same bars: the
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
            throw failure("SHARPE advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SharpeStream( SharpeStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInRiskFreeRate = other.optInRiskFreeRate;
         this.optInAnnualization = other.optInAnnualization;
         this.shift = other.shift;
         this.periodTotal1 = other.periodTotal1;
         this.periodTotal2 = other.periodTotal2;
         this.peakTotal2 = other.peakTotal2;
         this.prevPrice = other.prevPrice;
         this.dPeriod = other.dPeriod;
         this.invPeriod = other.invPeriod;
         this.sqrtAnnual = other.sqrtAnnual;
         this.trailingIdx = other.trailingIdx;
         this.barsSinceReseed = other.barsSinceReseed;
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
            throw failure("SHARPE update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SHARPE update", "inReal");
         core.sharpeStepImpl(this, inReal);
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
            throw nonFiniteBar("SHARPE peek", "inReal");
         SharpeStream sp = this;
         double price = 0.0;
         double trailingPrevPrice = 0.0;
         double windowPrevPrice = 0.0;
         double ret = 0.0;
         double dev = 0.0;
         double meanDev = 0.0;
         double moment2 = 0.0;
         double excess = 0.0;
         double ratio = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double peakTotal2 = sp.peakTotal2;
         double periodTotal1 = sp.periodTotal1;
         double periodTotal2 = sp.periodTotal2;
         double prevPrice = sp.prevPrice;
         double shift = sp.shift;
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
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanDev = periodTotal1 * sp.invPeriod;
         moment2 = periodTotal2 - meanDev * periodTotal1;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write. The oldest price read is the trailing
          * return's base, so the index stays on it.
          */
         trailingPrevPrice = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         trailingIdx += 1;
         price = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         dev = ret - shift;
         periodTotal1 -= dev;
         periodTotal2 -= dev * dev;
         /* Rebuild against the window mean when the spread has shrunk below 1% of
          * the largest sum of squares held since the last rebuild, or at least
          * every 32 windows. The rounding the running sums carry scales with that
          * peak, so once a large return leaves, the current sum can be all
          * rounding; a flat run after a move must still come out exactly flat.
          * The spread is a divisor, so its relative error reaches the ratio.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = sp.i - sp.optInTimePeriod + 1;
            windowPrevPrice = (((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
            dev = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               price = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev += ret;
            }
            shift = dev * sp.invPeriod;
            windowPrevPrice = (((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               price = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev = ret - shift;
               periodTotal1 += dev;
               periodTotal2 += dev * dev;
            }
            meanDev = periodTotal1 * sp.invPeriod;
            moment2 = periodTotal2 - meanDev * periodTotal1;
            /* A window of equal returns can sit off this mean by rounding, which
             * leaves a spread of rounding where the ratio must be the plain
             * quotient, and fires the trigger again on every bar. Anchored on
             * one of its own returns, its deviations are exactly zero.
             */
            if( moment2 < 0.000001 * periodTotal2 ) {
               shift = ret;
               windowPrevPrice = (((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0;
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= sp.i; j += 1 ) {
                  price = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
                  ret = 0.0;
                  if( windowPrevPrice != 0.0 ) {
                     ret = (price - windowPrevPrice) / windowPrevPrice;
                  }
                  windowPrevPrice = price;
                  dev = ret - shift;
                  periodTotal1 += dev;
                  periodTotal2 += dev * dev;
               }
               meanDev = periodTotal1 * sp.invPeriod;
               moment2 = periodTotal2 - meanDev * periodTotal1;
            }
            peakTotal2 = periodTotal2;
            /* Re-remove the trailing return under the new shift so the carried
             * state matches the path without a rebuild.
             */
            price = ((windowStart & sp.xMask) != pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0;
            ret = 0.0;
            if( ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0) != 0.0 ) {
               ret = (price - ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0)) / ((((windowStart - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(windowStart - 1) & sp.xMask] : pkVal0);
            }
            dev = ret - shift;
            periodTotal1 -= dev;
            periodTotal2 -= dev * dev;
         }
         /* No spread is the plain quotient: +/-Inf by the sign of the excess
          * mean, NaN when it is exactly zero (ruled on #571). Annualize the
          * per-bar ratio last, so an annualization of 1 is that ratio bit for
          * bit.
          */
         excess = shift + meanDev - sp.optInRiskFreeRate;
         ratio = excess / Math.sqrt(moment2 / (sp.dPeriod - 1.0));
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
      public SharpeStream clone() {
         return new SharpeStream(this);
      }
   }
   private void sharpeStepImpl( SharpeStream sp, double inReal )
   {
      double price = 0.0;
      double trailingPrevPrice = 0.0;
      double windowPrevPrice = 0.0;
      double ret = 0.0;
      double dev = 0.0;
      double meanDev = 0.0;
      double moment2 = 0.0;
      double excess = 0.0;
      double ratio = 0.0;
      sp.x_inReal[sp.i & sp.xMask] = inReal;
      price = sp.x_inReal[sp.i & sp.xMask];
      ret = 0.0;
      if( sp.prevPrice != 0.0 ) {
         ret = (price - sp.prevPrice) / sp.prevPrice;
      }
      sp.prevPrice = price;
      dev = ret - sp.shift;
      sp.periodTotal1 += dev;
      sp.periodTotal2 += dev * dev;
      sp.peakTotal2 = (sp.periodTotal2 > sp.peakTotal2) ? sp.periodTotal2 : sp.peakTotal2;
      meanDev = sp.periodTotal1 * sp.invPeriod;
      moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
      /* inReal and outReal may be the same buffer, and the trailing prices
       * sit in slots the output reaches: every read of the window happens
       * before this bar's write. The oldest price read is the trailing
       * return's base, so the index stays on it.
       */
      trailingPrevPrice = sp.x_inReal[sp.trailingIdx & sp.xMask];
      sp.trailingIdx += 1;
      price = sp.x_inReal[sp.trailingIdx & sp.xMask];
      ret = 0.0;
      if( trailingPrevPrice != 0.0 ) {
         ret = (price - trailingPrevPrice) / trailingPrevPrice;
      }
      dev = ret - sp.shift;
      sp.periodTotal1 -= dev;
      sp.periodTotal2 -= dev * dev;
      /* Rebuild against the window mean when the spread has shrunk below 1% of
       * the largest sum of squares held since the last rebuild, or at least
       * every 32 windows. The rounding the running sums carry scales with that
       * peak, so once a large return leaves, the current sum can be all
       * rounding; a flat run after a move must still come out exactly flat.
       * The spread is a divisor, so its relative error reaches the ratio.
       */
      sp.barsSinceReseed -= 1;
      if( moment2 < 0.01 * sp.peakTotal2 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         sp.windowStart = sp.i - sp.optInTimePeriod + 1;
         windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
         dev = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            price = sp.x_inReal[sp.j & sp.xMask];
            ret = 0.0;
            if( windowPrevPrice != 0.0 ) {
               ret = (price - windowPrevPrice) / windowPrevPrice;
            }
            windowPrevPrice = price;
            dev += ret;
         }
         sp.shift = dev * sp.invPeriod;
         windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
         sp.periodTotal1 = 0.0;
         sp.periodTotal2 = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            price = sp.x_inReal[sp.j & sp.xMask];
            ret = 0.0;
            if( windowPrevPrice != 0.0 ) {
               ret = (price - windowPrevPrice) / windowPrevPrice;
            }
            windowPrevPrice = price;
            dev = ret - sp.shift;
            sp.periodTotal1 += dev;
            sp.periodTotal2 += dev * dev;
         }
         meanDev = sp.periodTotal1 * sp.invPeriod;
         moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
         /* A window of equal returns can sit off this mean by rounding, which
          * leaves a spread of rounding where the ratio must be the plain
          * quotient, and fires the trigger again on every bar. Anchored on
          * one of its own returns, its deviations are exactly zero.
          */
         if( moment2 < 0.000001 * sp.periodTotal2 ) {
            sp.shift = ret;
            windowPrevPrice = sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
            sp.periodTotal1 = 0.0;
            sp.periodTotal2 = 0.0;
            for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
               price = sp.x_inReal[sp.j & sp.xMask];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev = ret - sp.shift;
               sp.periodTotal1 += dev;
               sp.periodTotal2 += dev * dev;
            }
            meanDev = sp.periodTotal1 * sp.invPeriod;
            moment2 = sp.periodTotal2 - meanDev * sp.periodTotal1;
         }
         sp.peakTotal2 = sp.periodTotal2;
         /* Re-remove the trailing return under the new shift so the carried
          * state matches the path without a rebuild.
          */
         price = sp.x_inReal[sp.windowStart & sp.xMask];
         ret = 0.0;
         if( sp.x_inReal[(sp.windowStart - 1) & sp.xMask] != 0.0 ) {
            ret = (price - sp.x_inReal[(sp.windowStart - 1) & sp.xMask]) / sp.x_inReal[(sp.windowStart - 1) & sp.xMask];
         }
         dev = ret - sp.shift;
         sp.periodTotal1 -= dev;
         sp.periodTotal2 -= dev * dev;
      }
      /* No spread is the plain quotient: +/-Inf by the sign of the excess
       * mean, NaN when it is exactly zero (ruled on #571). Annualize the
       * per-bar ratio last, so an annualization of 1 is that ratio bit for
       * bit.
       */
      excess = sp.shift + meanDev - sp.optInRiskFreeRate;
      ratio = excess / Math.sqrt(moment2 / (sp.dPeriod - 1.0));
      sp.cur_outReal = sp.sqrtAnnual * ratio;
      sp.i += 1;
   }
   private RetCode sharpeOpenImpl( SharpeStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double peakTotal2 = 0;
      double price = 0;
      double prevPrice = 0;
      double trailingPrevPrice = 0;
      double windowPrevPrice = 0;
      double ret = 0;
      double dev = 0;
      double dPeriod = 0;
      double invPeriod = 0;
      double meanDev = 0;
      double moment2 = 0;
      double excess = 0;
      double sqrtAnnual = 0;
      double ratio = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
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
      if( optInRiskFreeRate == REAL_DEFAULT ) {
         optInRiskFreeRate = 0e0;
      } else if( !(optInRiskFreeRate >= REAL_MIN && optInRiskFreeRate <= REAL_MAX) ) {
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
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      sqrtAnnual = Math.sqrt(optInAnnualization);
      /* The window holds the returns of the bars after trailingIdx up to i; a
       * bar's return reads the price before it.
       *
       * The variance of the excess returns is the variance of the returns, so
       * the risk-free rate enters only the mean. That keeps the sums at return
       * scale whatever the rate, and lets a flat price window reach an exact
       * zero spread.
       */
      trailingIdx = startIdx - optInTimePeriod;
      prevPrice = inReal[trailingIdx];
      /* A return needs a non-zero base price and nothing more: the test is
       * exact, never a band, since a price carries the quote unit. A zero base
       * gives a zero return.
       */
      price = inReal[trailingIdx + 1];
      ret = 0.0;
      if( prevPrice != 0.0 ) {
         ret = (price - prevPrice) / prevPrice;
      }
      shift = ret;
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      for( j = trailingIdx + 1; j < startIdx; j += 1 ) {
         price = inReal[j];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      peakTotal2 = periodTotal2;
      do {
         price = inReal[i];
         ret = 0.0;
         if( prevPrice != 0.0 ) {
            ret = (price - prevPrice) / prevPrice;
         }
         prevPrice = price;
         dev = ret - shift;
         periodTotal1 += dev;
         periodTotal2 += dev * dev;
         peakTotal2 = (periodTotal2 > peakTotal2) ? periodTotal2 : peakTotal2;
         meanDev = periodTotal1 * invPeriod;
         moment2 = periodTotal2 - meanDev * periodTotal1;
         /* inReal and outReal may be the same buffer, and the trailing prices
          * sit in slots the output reaches: every read of the window happens
          * before this bar's write. The oldest price read is the trailing
          * return's base, so the index stays on it.
          */
         trailingPrevPrice = inReal[trailingIdx];
         trailingIdx += 1;
         price = inReal[trailingIdx];
         ret = 0.0;
         if( trailingPrevPrice != 0.0 ) {
            ret = (price - trailingPrevPrice) / trailingPrevPrice;
         }
         dev = ret - shift;
         periodTotal1 -= dev;
         periodTotal2 -= dev * dev;
         /* Rebuild against the window mean when the spread has shrunk below 1% of
          * the largest sum of squares held since the last rebuild, or at least
          * every 32 windows. The rounding the running sums carry scales with that
          * peak, so once a large return leaves, the current sum can be all
          * rounding; a flat run after a move must still come out exactly flat.
          * The spread is a divisor, so its relative error reaches the ratio.
          */
         barsSinceReseed -= 1;
         if( moment2 < 0.01 * peakTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = i - optInTimePeriod + 1;
            windowPrevPrice = inReal[windowStart - 1];
            dev = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev += ret;
            }
            shift = dev * invPeriod;
            windowPrevPrice = inReal[windowStart - 1];
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               price = inReal[j];
               ret = 0.0;
               if( windowPrevPrice != 0.0 ) {
                  ret = (price - windowPrevPrice) / windowPrevPrice;
               }
               windowPrevPrice = price;
               dev = ret - shift;
               periodTotal1 += dev;
               periodTotal2 += dev * dev;
            }
            meanDev = periodTotal1 * invPeriod;
            moment2 = periodTotal2 - meanDev * periodTotal1;
            /* A window of equal returns can sit off this mean by rounding, which
             * leaves a spread of rounding where the ratio must be the plain
             * quotient, and fires the trigger again on every bar. Anchored on
             * one of its own returns, its deviations are exactly zero.
             */
            if( moment2 < 0.000001 * periodTotal2 ) {
               shift = ret;
               windowPrevPrice = inReal[windowStart - 1];
               periodTotal1 = 0.0;
               periodTotal2 = 0.0;
               for( j = windowStart; j <= i; j += 1 ) {
                  price = inReal[j];
                  ret = 0.0;
                  if( windowPrevPrice != 0.0 ) {
                     ret = (price - windowPrevPrice) / windowPrevPrice;
                  }
                  windowPrevPrice = price;
                  dev = ret - shift;
                  periodTotal1 += dev;
                  periodTotal2 += dev * dev;
               }
               meanDev = periodTotal1 * invPeriod;
               moment2 = periodTotal2 - meanDev * periodTotal1;
            }
            peakTotal2 = periodTotal2;
            /* Re-remove the trailing return under the new shift so the carried
             * state matches the path without a rebuild.
             */
            price = inReal[windowStart];
            ret = 0.0;
            if( inReal[windowStart - 1] != 0.0 ) {
               ret = (price - inReal[windowStart - 1]) / inReal[windowStart - 1];
            }
            dev = ret - shift;
            periodTotal1 -= dev;
            periodTotal2 -= dev * dev;
         }
         /* No spread is the plain quotient: +/-Inf by the sign of the excess
          * mean, NaN when it is exactly zero (ruled on #571). Annualize the
          * per-bar ratio last, so an annualization of 1 is that ratio bit for
          * bit.
          */
         excess = shift + meanDev - optInRiskFreeRate;
         ratio = excess / Math.sqrt(moment2 / (dPeriod - 1.0));
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
      sp.optInRiskFreeRate = optInRiskFreeRate;
      sp.optInAnnualization = optInAnnualization;
      sp.shift = shift;
      sp.periodTotal1 = periodTotal1;
      sp.periodTotal2 = periodTotal2;
      sp.peakTotal2 = peakTotal2;
      sp.prevPrice = prevPrice;
      sp.dPeriod = dPeriod;
      sp.invPeriod = invPeriod;
      sp.sqrtAnnual = sqrtAnnual;
      sp.trailingIdx = trailingIdx;
      sp.barsSinceReseed = barsSinceReseed;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* sharpeOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SharpeStream sharpeOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SharpeStream sp = new SharpeStream(this);
      RetCode retCode = sharpeOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SHARPE openAndFill", inReal.length, startIdx, sharpeLookback(optInTimePeriod, optInRiskFreeRate, optInAnnualization));
      }
      throw streamFailure("SHARPE openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind sharpeOpen (composition seam). */
   SharpeStream sharpeOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization )
   {
      SharpeStream sp = new SharpeStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = sharpeOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SHARPE open", inReal.length, startIdx, sharpeLookback(optInTimePeriod, optInRiskFreeRate, optInAnnualization));
      }
      throw streamFailure("SHARPE open", retCode);
   }
   /**
    * Open a live SHARPE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#sharpe} at that bar.
    * <p>The history must hold at least {@code sharpeLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public SharpeStream sharpeOpen( double inReal[], int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization )
   {
      requireArgument("SHARPE open", "inReal", inReal);
      requireHistory("SHARPE open", inReal.length);
      return sharpeOpenInternal(inReal, 0, optInTimePeriod, optInRiskFreeRate, optInAnnualization);
   }
   /**
    * {@link Core#sharpeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#sharpe} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SharpeStream#outRange()}.
    */
   public SharpeStream sharpeOpenAndFill( double inReal[], int optInTimePeriod, double optInRiskFreeRate, double optInAnnualization, double outReal[] )
   {
      requireArgument("SHARPE openAndFill", "inReal", inReal);
      requireHistory("SHARPE openAndFill", inReal.length);
      int guardOutLen = openFillCount("SHARPE openAndFill", inReal.length, sharpeLookback(optInTimePeriod, optInRiskFreeRate, optInAnnualization));
      requireLength("SHARPE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SHARPE openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return sharpeOpenAndFillInternal(inReal, 0, optInTimePeriod, optInRiskFreeRate, optInAnnualization, outBegIdx, outNBElement, outReal);
   }
