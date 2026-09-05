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
 *  090526 MF,CC  First version (issue #366).
 */

   /**
    * Number of leading input bars {@link Core#RVI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Wilder smoothing period applied to both legs
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInStdDevPeriod Number of trailing values the standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int RVI_Lookback( int optInTimePeriod, int optInStdDevPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return -1;
      }
      return optInStdDevPeriod - 1 + (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.RVI.ordinal()] ;

   }
   RetCode RVI_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     int optInStdDevPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double sigma = 0;
      double delta = 0;
      double upValue = 0;
      double dnValue = 0;
      double upTotal = 0;
      double dnTotal = 0;
      double prevUp = 0;
      double prevDn = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double total = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int today = 0;
      int anchorIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbInitialElementNeeded = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVI_Lookback(optInTimePeriod, optInStdDevPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* wAlpha is derived FROM wBeta, never the reverse (rma.c): only that order
       * makes the pair sum to exactly 1, and TA_RMA over this function's two legs
       * has to be bit for bit what the fused step below computes.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      /* The per-bar sigma is var.c's step transcribed unchanged -- shifted running
       * sums against a near-window anchor, the reseed trigger, its floor, and the
       * re-remove under the new shift. Any algebraically equal spelling is a
       * different double, and the reference this is differenced against is
       * TA_STDDEV anchored at this call's own start.
       */
      nbInitialElementNeeded = optInStdDevPeriod - 1;
      invPeriod = 1.0 / (double)optInStdDevPeriod;
      anchorIdx = startIdx - lookbackTotal;
      trailingIdx = anchorIdx;
      shift = inReal[anchorIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      today = anchorIdx + nbInitialElementNeeded;
      for( j = anchorIdx; j < today; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      barsSinceReseed = 32 * optInStdDevPeriod;
      /* Seed both legs with the simple average of the first 'optInTimePeriod'
       * volatilities, as rma.c seeds. optInStdDevPeriod >= 2 is what keeps the
       * inReal[today-1] below in bounds on the very first bar.
       */
      upTotal = 0.0;
      dnTotal = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         if( delta > 0.0 ) {
            upTotal += sigma;
         } else if( delta < 0.0 ) {
            dnTotal += sigma;
         }
         today += 1;
      }
      prevUp = upTotal / optInTimePeriod;
      prevDn = dnTotal / optInTimePeriod;
      /* Skip the unstable period. Same step, smoothed but not stored. */
      i = this.unstablePeriod[FuncUnstId.RVI.ordinal()];
      while( i != 0 ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         today += 1;
         i -= 1;
      }
      /* A tie feeds neither leg, so both can be exactly zero at the same bar --
       * reachable on real data whenever the smoothing has no memory. Test the
       * total exactly: a band would carry the quote unit and zero the oscillator
       * for any instrument priced under it.
       */
      total = prevUp + prevDn;
      outReal[0] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         total = prevUp + prevDn;
         outReal[outIdx++] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode RVI_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     int optInStdDevPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double sigma = 0;
      double delta = 0;
      double upValue = 0;
      double dnValue = 0;
      double upTotal = 0;
      double dnTotal = 0;
      double prevUp = 0;
      double prevDn = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double total = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int today = 0;
      int anchorIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbInitialElementNeeded = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVI_Lookback(optInTimePeriod, optInStdDevPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      nbInitialElementNeeded = optInStdDevPeriod - 1;
      invPeriod = 1.0 / (double)optInStdDevPeriod;
      anchorIdx = startIdx - lookbackTotal;
      trailingIdx = anchorIdx;
      shift = (double)inReal[anchorIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      today = anchorIdx + nbInitialElementNeeded;
      for( j = anchorIdx; j < today; j += 1 ) {
         tempReal = (double)inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      barsSinceReseed = 32 * optInStdDevPeriod;
      upTotal = 0.0;
      dnTotal = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempReal = (double)inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = (double)inReal[today] - (double)inReal[today - 1];
         if( delta > 0.0 ) {
            upTotal += sigma;
         } else if( delta < 0.0 ) {
            dnTotal += sigma;
         }
         today += 1;
      }
      prevUp = upTotal / optInTimePeriod;
      prevDn = dnTotal / optInTimePeriod;
      i = this.unstablePeriod[FuncUnstId.RVI.ordinal()];
      while( i != 0 ) {
         tempReal = (double)inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = (double)inReal[today] - (double)inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         today += 1;
         i -= 1;
      }
      total = prevUp + prevDn;
      outReal[0] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = (double)inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = (double)inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = (double)inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = (double)inReal[today] - (double)inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         total = prevUp + prevDn;
         outReal[outIdx++] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Relative Volatility Index: Donald Dorsey's volatility oscillator, built
    * exactly like RSI except that the quantity routed to the up and down
    * buckets is the rolling standard deviation of price rather than the size of
    * the move. The direction of the close-to-close change still decides which
    * bucket a bar feeds. Bounded in 0..100. High values mean the recent
    * volatility arrived mostly on up bars, low values that it arrived mostly on
    * down bars. Dorsey proposed it as a confirming filter rather than a
    * stand-alone signal: take a long entry only while RVI is above 50, a short
    * only while it is below.
    * <p><b>Formula</b>
    * <pre>{@code
    * With `S` the standard deviation of the last `optInStdDevPeriod` values of `inReal`, and `C` the input series:
    * U[i] = S[i] if C[i] > C[i-1], else 0
    * D[i] = S[i] if C[i] < C[i-1], else 0
    * RVI  = 100 * RMA(U, optInTimePeriod) / ( RMA(U, optInTimePeriod) + RMA(D, optInTimePeriod) )
    * `RMA` is Wilder's smoothed moving average, seeded with the simple average of its first `optInTimePeriod` inputs. A bar whose close equals the previous close feeds neither bucket.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>This is Dorsey's 1993 original, which measures the closes alone. His 1995 revision averages the index of the highs with the index of the lows; some vendors reserve the name RVI for that revision and call this one RVIorig. It is not implemented here.</li>
    * <li>A tie contributes to neither bucket, matching RSI's treatment of an unchanged close. Descriptions that write the denominator as a smoothed {@code S} instead of {@code U + D} are counting ties as down bars, which is a different indicator.</li>
    * <li>Both smoothed legs can be exactly zero at the same bar, which happens whenever the smoothing carries no memory and the bar is a tie. RVI reports its neutral centre, 50, there rather than a non-finite value.</li>
    * <li>The standard deviation is the population form. The sample form differs by a constant factor that cancels in the ratio, so it is not a variant.</li>
    * <li>Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 11.6 index points; one shared period for both the deviation and the smoothing, up to 15.6; an RSI taken over the standard-deviation series, up to 35.2; a linear-regression residual, up to 36.0. These are different indicators, not errors.</li>
    * <li>Unrelated to the Relative Vigor Index, which several platforms also abbreviate RVI.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Wilder smoothing period applied to both legs
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInStdDevPeriod Number of trailing values the standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Relative Volatility Index value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#RSI
    * @see Core#RMA
    * @see Core#STDDEV
    * @see Core#CMO
    */
   public OutRange RVI( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        int optInStdDevPeriod,
                        double outReal[] )
   {
      requireIndexRange("RVI", startIdx, endIdx);
      int guardStart = clampedStart("RVI", startIdx, RVI_Lookback(optInTimePeriod, optInStdDevPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVI", "inReal", inReal, guardInLen);
      requireLength("RVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVI_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Relative Volatility Index: Donald Dorsey's volatility oscillator, built
    * exactly like RSI except that the quantity routed to the up and down
    * buckets is the rolling standard deviation of price rather than the size of
    * the move. The direction of the close-to-close change still decides which
    * bucket a bar feeds. Bounded in 0..100. High values mean the recent
    * volatility arrived mostly on up bars, low values that it arrived mostly on
    * down bars. Dorsey proposed it as a confirming filter rather than a
    * stand-alone signal: take a long entry only while RVI is above 50, a short
    * only while it is below.
    * <p><b>Formula</b>
    * <pre>{@code
    * With `S` the standard deviation of the last `optInStdDevPeriod` values of `inReal`, and `C` the input series:
    * U[i] = S[i] if C[i] > C[i-1], else 0
    * D[i] = S[i] if C[i] < C[i-1], else 0
    * RVI  = 100 * RMA(U, optInTimePeriod) / ( RMA(U, optInTimePeriod) + RMA(D, optInTimePeriod) )
    * `RMA` is Wilder's smoothed moving average, seeded with the simple average of its first `optInTimePeriod` inputs. A bar whose close equals the previous close feeds neither bucket.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>This is Dorsey's 1993 original, which measures the closes alone. His 1995 revision averages the index of the highs with the index of the lows; some vendors reserve the name RVI for that revision and call this one RVIorig. It is not implemented here.</li>
    * <li>A tie contributes to neither bucket, matching RSI's treatment of an unchanged close. Descriptions that write the denominator as a smoothed {@code S} instead of {@code U + D} are counting ties as down bars, which is a different indicator.</li>
    * <li>Both smoothed legs can be exactly zero at the same bar, which happens whenever the smoothing carries no memory and the bar is a tie. RVI reports its neutral centre, 50, there rather than a non-finite value.</li>
    * <li>The standard deviation is the population form. The sample form differs by a constant factor that cancels in the ratio, so it is not a variant.</li>
    * <li>Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 11.6 index points; one shared period for both the deviation and the smoothing, up to 15.6; an RSI taken over the standard-deviation series, up to 35.2; a linear-regression residual, up to 36.0. These are different indicators, not errors.</li>
    * <li>Unrelated to the Relative Vigor Index, which several platforms also abbreviate RVI.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series, canonically the close.
    * @param optInTimePeriod Wilder smoothing period applied to both legs
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInStdDevPeriod Number of trailing values the standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Relative Volatility Index value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#RSI
    * @see Core#RMA
    * @see Core#STDDEV
    * @see Core#CMO
    */
   public OutRange RVI( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        int optInStdDevPeriod,
                        double outReal[] )
   {
      requireIndexRange("RVI", startIdx, endIdx);
      int guardStart = clampedStart("RVI", startIdx, RVI_Lookback(optInTimePeriod, optInStdDevPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVI", "inReal", inReal, guardInLen);
      requireLength("RVI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVI_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live RVI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#RVI} over the same series.
    * Open with {@link Core#rviOpen}; there is no close — the handle is
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
   public static final class RviStream {
      Core core;
      int optInTimePeriod;
      int optInStdDevPeriod;
      double shift;
      double periodTotal1;
      double periodTotal2;
      double invPeriod;
      double prevUp;
      double prevDn;
      double wAlpha;
      double wBeta;
      int trailingIdx;
      int barsSinceReseed;
      int nbInitialElementNeeded;
      int j;
      int windowStart;
      int today;
      double lag1_inReal;
      int xMask;
      double[] x_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      RviStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#RVI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      RviStream( RviStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInStdDevPeriod = other.optInStdDevPeriod;
         this.shift = other.shift;
         this.periodTotal1 = other.periodTotal1;
         this.periodTotal2 = other.periodTotal2;
         this.invPeriod = other.invPeriod;
         this.prevUp = other.prevUp;
         this.prevDn = other.prevDn;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.trailingIdx = other.trailingIdx;
         this.barsSinceReseed = other.barsSinceReseed;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.j = other.j;
         this.windowStart = other.windowStart;
         this.today = other.today;
         this.lag1_inReal = other.lag1_inReal;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("RVI update: BadParam", RetCode.BadParam);
         }
         core.rviStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("RVI updateAndFill", "inReal", inReal);
         requireArgument("RVI updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("RVI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("RVI updateAndFill: BadParam", RetCode.BadParam);
            }
            core.rviStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("RVI peek: BadParam", RetCode.BadParam);
         RviStream sp = this;
         double tempReal = 0.0;
         double meanValue1 = 0.0;
         double variance = 0.0;
         double sigma = 0.0;
         double delta = 0.0;
         double upValue = 0.0;
         double dnValue = 0.0;
         double total = 0.0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double periodTotal1 = sp.periodTotal1;
         double periodTotal2 = sp.periodTotal2;
         double prevDn = sp.prevDn;
         double prevUp = sp.prevUp;
         double shift = sp.shift;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         int windowStart = sp.windowStart;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            j -= rebaseShift;
            windowStart -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inReal;
         tempReal = (((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0) - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * sp.invPeriod;
         variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
         tempReal = (((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0) - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * sp.invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInStdDevPeriod;
            windowStart = today - sp.nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
            }
            shift = tempReal * sp.invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * sp.invPeriod;
            variance = periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * sp.invPeriod) ) {
               variance = 0.0;
            }
            tempReal = (((windowStart & sp.xMask) != pkSlot0) ? sp.x_inReal[windowStart & sp.xMask] : pkVal0) - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = (((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0) - ((((today - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(today - 1) & sp.xMask] : pkVal0);
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(sp.wBeta, prevUp, sp.wAlpha * upValue);
         prevDn = Math.fma(sp.wBeta, prevDn, sp.wAlpha * dnValue);
         total = prevUp + prevDn;
         cur_outReal = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
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
      public RviStream clone() {
         return new RviStream(this);
      }
   }
   void rviStepImpl( RviStream sp, double inReal )
   {
      double tempReal = 0.0;
      double meanValue1 = 0.0;
      double variance = 0.0;
      double sigma = 0.0;
      double delta = 0.0;
      double upValue = 0.0;
      double dnValue = 0.0;
      double total = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
         sp.windowStart -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      tempReal = sp.x_inReal[sp.today & sp.xMask] - sp.shift;
      sp.periodTotal1 += tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 += tempReal;
      meanValue1 = sp.periodTotal1 * sp.invPeriod;
      variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
      tempReal = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      sp.periodTotal1 -= tempReal;
      tempReal *= tempReal;
      sp.periodTotal2 -= tempReal;
      sp.trailingIdx += 1;
      sp.barsSinceReseed -= 1;
      if( variance < 0.000001 * (sp.periodTotal2 * sp.invPeriod) || tempReal > 1000000.0 * sp.periodTotal2 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInStdDevPeriod;
         sp.windowStart = sp.today - sp.nbInitialElementNeeded;
         tempReal = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.today; sp.j += 1 ) {
            tempReal += sp.x_inReal[sp.j & sp.xMask];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.periodTotal1 = 0.0;
         sp.periodTotal2 = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.today; sp.j += 1 ) {
            tempReal = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
            sp.periodTotal1 += tempReal;
            tempReal *= tempReal;
            sp.periodTotal2 += tempReal;
         }
         meanValue1 = sp.periodTotal1 * sp.invPeriod;
         variance = sp.periodTotal2 * sp.invPeriod - meanValue1 * meanValue1;
         if( variance < 0.000000000001 * (sp.periodTotal2 * sp.invPeriod) ) {
            variance = 0.0;
         }
         tempReal = sp.x_inReal[sp.windowStart & sp.xMask] - sp.shift;
         sp.periodTotal1 -= tempReal;
         tempReal *= tempReal;
         sp.periodTotal2 -= tempReal;
      }
      sigma = Math.sqrt(variance);
      delta = sp.x_inReal[sp.today & sp.xMask] - sp.x_inReal[(sp.today - 1) & sp.xMask];
      upValue = 0.0;
      dnValue = 0.0;
      if( delta > 0.0 ) {
         upValue = sigma;
      } else if( delta < 0.0 ) {
         dnValue = sigma;
      }
      sp.prevUp = Math.fma(sp.wBeta, sp.prevUp, sp.wAlpha * upValue);
      sp.prevDn = Math.fma(sp.wBeta, sp.prevDn, sp.wAlpha * dnValue);
      total = sp.prevUp + sp.prevDn;
      sp.cur_outReal = (total == 0.0) ? 50.0 : 100.0 * (sp.prevUp / total);
      sp.today += 1;
      sp.lag1_inReal = inReal;
   }
   private RetCode rviOpenImpl( RviStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInStdDevPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double shift = 0;
      double periodTotal1 = 0;
      double periodTotal2 = 0;
      double meanValue1 = 0;
      double variance = 0;
      double invPeriod = 0;
      double sigma = 0;
      double delta = 0;
      double upValue = 0;
      double dnValue = 0;
      double upTotal = 0;
      double dnTotal = 0;
      double prevUp = 0;
      double prevDn = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double total = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int today = 0;
      int anchorIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int nbInitialElementNeeded = 0;
      int lookbackTotal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVI_Lookback(optInTimePeriod, optInStdDevPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* wAlpha is derived FROM wBeta, never the reverse (rma.c): only that order
       * makes the pair sum to exactly 1, and TA_RMA over this function's two legs
       * has to be bit for bit what the fused step below computes.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      /* The per-bar sigma is var.c's step transcribed unchanged -- shifted running
       * sums against a near-window anchor, the reseed trigger, its floor, and the
       * re-remove under the new shift. Any algebraically equal spelling is a
       * different double, and the reference this is differenced against is
       * TA_STDDEV anchored at this call's own start.
       */
      nbInitialElementNeeded = optInStdDevPeriod - 1;
      invPeriod = 1.0 / (double)optInStdDevPeriod;
      anchorIdx = startIdx - lookbackTotal;
      trailingIdx = anchorIdx;
      shift = inReal[anchorIdx];
      periodTotal1 = 0.0;
      periodTotal2 = 0.0;
      today = anchorIdx + nbInitialElementNeeded;
      for( j = anchorIdx; j < today; j += 1 ) {
         tempReal = inReal[j] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
      }
      barsSinceReseed = 32 * optInStdDevPeriod;
      /* Seed both legs with the simple average of the first 'optInTimePeriod'
       * volatilities, as rma.c seeds. optInStdDevPeriod >= 2 is what keeps the
       * inReal[today-1] below in bounds on the very first bar.
       */
      upTotal = 0.0;
      dnTotal = 0.0;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         if( delta > 0.0 ) {
            upTotal += sigma;
         } else if( delta < 0.0 ) {
            dnTotal += sigma;
         }
         today += 1;
      }
      prevUp = upTotal / optInTimePeriod;
      prevDn = dnTotal / optInTimePeriod;
      /* Skip the unstable period. Same step, smoothed but not stored. */
      i = this.unstablePeriod[FuncUnstId.RVI.ordinal()];
      while( i != 0 ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         today += 1;
         i -= 1;
      }
      /* A tie feeds neither leg, so both can be exactly zero at the same bar --
       * reachable on real data whenever the smoothing has no memory. Test the
       * total exactly: a band would carry the quote unit and zero the oscillator
       * for any instrument priced under it.
       */
      total = prevUp + prevDn;
      outReal[0 * outStride] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today] - shift;
         periodTotal1 += tempReal;
         tempReal *= tempReal;
         periodTotal2 += tempReal;
         meanValue1 = periodTotal1 * invPeriod;
         variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
         tempReal = inReal[trailingIdx] - shift;
         periodTotal1 -= tempReal;
         tempReal *= tempReal;
         periodTotal2 -= tempReal;
         trailingIdx += 1;
         barsSinceReseed -= 1;
         if( variance < 0.000001 * (periodTotal2 * invPeriod) || tempReal > 1000000.0 * periodTotal2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInStdDevPeriod;
            windowStart = today - nbInitialElementNeeded;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            periodTotal1 = 0.0;
            periodTotal2 = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal = inReal[j] - shift;
               periodTotal1 += tempReal;
               tempReal *= tempReal;
               periodTotal2 += tempReal;
            }
            meanValue1 = periodTotal1 * invPeriod;
            variance = periodTotal2 * invPeriod - meanValue1 * meanValue1;
            if( variance < 0.000000000001 * (periodTotal2 * invPeriod) ) {
               variance = 0.0;
            }
            tempReal = inReal[windowStart] - shift;
            periodTotal1 -= tempReal;
            tempReal *= tempReal;
            periodTotal2 -= tempReal;
         }
         sigma = Math.sqrt(variance);
         delta = inReal[today] - inReal[today - 1];
         upValue = 0.0;
         dnValue = 0.0;
         if( delta > 0.0 ) {
            upValue = sigma;
         } else if( delta < 0.0 ) {
            dnValue = sigma;
         }
         prevUp = Math.fma(wBeta, prevUp, wAlpha * upValue);
         prevDn = Math.fma(wBeta, prevDn, wAlpha * dnValue);
         total = prevUp + prevDn;
         outReal[outIdx++ * outStride] = (total == 0.0) ? 50.0 : 100.0 * (prevUp / total);
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
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
      sp.optInStdDevPeriod = optInStdDevPeriod;
      sp.shift = shift;
      sp.periodTotal1 = periodTotal1;
      sp.periodTotal2 = periodTotal2;
      sp.invPeriod = invPeriod;
      sp.prevUp = prevUp;
      sp.prevDn = prevDn;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.trailingIdx = trailingIdx;
      sp.barsSinceReseed = barsSinceReseed;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.today = today;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* rviOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RviStream rviOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, int optInStdDevPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RviStream sp = new RviStream(this);
      RetCode retCode = rviOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("RVI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind rviOpen (composition seam). */
   RviStream rviOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInStdDevPeriod )
   {
      RviStream sp = new RviStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rviOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVI open: internal error", retCode);
      }
      throw new TaLibArgumentException("RVI open: " + retCode, retCode);
   }
   /**
    * Open a live RVI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#RVI} at that bar.
    * <p>The history must hold at least {@code RVI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public RviStream rviOpen( double inReal[], int optInTimePeriod, int optInStdDevPeriod )
   {
      requireArgument("RVI open", "inReal", inReal);
      requireHistory("RVI open", inReal.length);
      return rviOpenInternal(inReal, 0, optInTimePeriod, optInStdDevPeriod);
   }
   /**
    * {@link Core#rviOpen} that also fills the output array(s) bit-identically
    * to {@link Core#RVI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link RviStream#outRange()}.
    */
   public RviStream rviOpenAndFill( double inReal[], int optInTimePeriod, int optInStdDevPeriod, double outReal[] )
   {
      requireArgument("RVI openAndFill", "inReal", inReal);
      requireHistory("RVI openAndFill", inReal.length);
      int guardOutLen = openFillCount("RVI openAndFill", inReal.length, RVI_Lookback(optInTimePeriod, optInStdDevPeriod));
      requireLength("RVI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("RVI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rviOpenAndFillInternal(inReal, 0, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
   }
