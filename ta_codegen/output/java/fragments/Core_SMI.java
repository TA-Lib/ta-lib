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
 *  082026 MF,CC  Initial version (#238).
 */

   /**
    * Number of leading input bars {@link Core#SMI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Period of the high/low range (default 13; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastPeriod Period of the second smoothing, applied to the
    *        first (default 2; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SMI_Lookback( int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 2;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* One high/low window, then the three EMA warm-ups the pipeline stacks on
       * top of it: slow smooths the raw momentum, fast smooths that, and signal
       * smooths the finished SMI line. Every term is exactly the lookback of the
       * function it comes from, so none of them is restated here -- which is also
       * what makes SMI inherit TA_FUNC_UNST_EMA from its callee.
       */
      return optInTimePeriod - 1 + EMA_Lookback(optInSlowPeriod) + EMA_Lookback(optInFastPeriod) + EMA_Lookback(optInSignalPeriod) ;

   }
   RetCode SMI_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     int optInFastPeriod,
                     int optInSlowPeriod,
                     int optInSignalPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outSMI[],
                     double outSMISignal[] )
   {
      double kSlow = 0;
      double kFast = 0;
      double kSignal = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double emaSlowNum = 0;
      double emaSlowDen = 0;
      double emaFastNum = 0;
      double emaFastDen = 0;
      double sumSlowNum = 0;
      double sumSlowDen = 0;
      double sumFastNum = 0;
      double sumFastDen = 0;
      double sumSignal = 0;
      double num = 0;
      double den = 0;
      double halfDen = 0;
      double smiValue = 0;
      double prevSignal = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int nFast = 0;
      int nSignal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 2;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outSMI == outSMISignal ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      /* Blau's pipeline in one pass. A composed form is not available: the
       * streaming producer model carries exactly one intermediate series
       * (streaming.rs:2076) and SMI has two, the numerator and the denominator,
       * which must be smoothed separately before they are divided.
       *
       * Each of the three stages seeds the way ema.c does -- a simple average of
       * that stage's first 'period' inputs -- so the result is bit-identical to
       * TA_MAX + TA_MIN followed by four TA_EMA calls. The stage boundaries below
       * are the callee LOOKBACKS, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: each stage then seeds
       * from the values its predecessor would have published, exactly as the
       * composed form does. The seed sums accumulate from 0.0 in production
       * order; do not reorder or fuse them (0.0+x is not x for x=-0.0).
       *
       * TA_GetCompatibility() is deliberately NOT consulted, for the reason
       * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
       * preserved for the functions that already shipped with it and dropped from
       * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
       * The seeding choice itself is measured in docs/studies/ema-seeding/README.md.
       */
      kSlow = 2.0 / (double)(optInSlowPeriod + 1);
      kFast = 2.0 / (double)(optInFastPeriod + 1);
      kSignal = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSlow = EMA_Lookback(optInSlowPeriod);
      lookbackFast = EMA_Lookback(optInFastPeriod);
      emaSlowNum = 0.0;
      emaSlowDen = 0.0;
      emaFastNum = 0.0;
      emaFastDen = 0.0;
      prevSignal = 0.0;
      smiValue = 0.0;
      sumSlowNum = 0.0;
      sumSlowDen = 0.0;
      sumFastNum = 0.0;
      sumFastDen = 0.0;
      sumSignal = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = 0 - 1;
      lowestIdx = 0 - 1;
      /* The first bar carrying a full high/low window. */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInTimePeriod - 1);
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
       * that completes the signal seed, so it produces the first output pair.
       */
      while( today <= startIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = inClose[today] - (highest + lowest) * 0.5;
         /* Stage 1: the slow EMA, over the raw momentum. */
         if( nBar < optInSlowPeriod ) {
            sumSlowNum = sumSlowNum + num;
            sumSlowDen = sumSlowDen + den;
            if( nBar == optInSlowPeriod - 1 ) {
               emaSlowNum = sumSlowNum / optInSlowPeriod;
               emaSlowDen = sumSlowDen / optInSlowPeriod;
            }
         } else {
            emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
            emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         }
         /* Stage 2: the fast EMA, over what stage 1 publishes.
          *
          * The stage counters are compared BEFORE they are subtracted, never
          * after. Writing this as `nFast = nBar - lookbackSlow; if( nFast >= 0 )`
          * is correct in C, where the counters are signed, and broken everywhere
          * else: the Rust backend renders them as `usize`, so the subtraction
          * underflows for the first lookbackSlow bars -- a panic in a debug build
          * and a wrap in release. It would also be invisible to the cross-language
          * gate, which runs release servers at unstable period 0, where the branch
          * the wrap wrongly takes happens to be a no-op because both accumulators
          * are still 0.0.
          */
         if( nBar >= lookbackSlow ) {
            nFast = nBar - lookbackSlow;
            if( nFast < optInFastPeriod ) {
               sumFastNum = sumFastNum + emaSlowNum;
               sumFastDen = sumFastDen + emaSlowDen;
               if( nFast == optInFastPeriod - 1 ) {
                  emaFastNum = sumFastNum / optInFastPeriod;
                  emaFastDen = sumFastDen / optInFastPeriod;
               }
            } else {
               emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
               emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
            }
         }
         /* Stage 3: the SMI line, then the signal EMA over it. */
         if( nBar >= lookbackSlow + lookbackFast ) {
            nSignal = nBar - lookbackSlow - lookbackFast;
            halfDen = 0.5 * emaFastDen;
            if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
               smiValue = 100.0 * emaFastNum / halfDen;
            } else {
               smiValue = 0.0;
            }
            if( nSignal < optInSignalPeriod ) {
               sumSignal = sumSignal + smiValue;
               if( nSignal == optInSignalPeriod - 1 ) {
                  prevSignal = sumSignal / optInSignalPeriod;
               }
            } else {
               prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outSMI[0] = smiValue;
      outSMISignal[0] = prevSignal;
      outIdx = 1;
      /* Stable zone. Every stage is a pure recursion from here on. */
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = inClose[today] - (highest + lowest) * 0.5;
         emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
         emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
         emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
         /* Guard with TA_IS_ZERO, not an exact `halfDen != 0.0`: a machine-flat
          * window leaves a sub-epsilon residue that an exact check would divide
          * into noise (issue #107 / STOCHRSI). A window whose bars are all
          * H == L makes num zero too, so this is 0/0, and the neutral 0.0 is the
          * CCI (#7) and IMI (#112) convention.
          */
         halfDen = 0.5 * emaFastDen;
         if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
            smiValue = 100.0 * emaFastNum / halfDen;
         } else {
            smiValue = 0.0;
         }
         prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
         outSMI[outIdx] = smiValue;
         outSMISignal[outIdx] = prevSignal;
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode SMI_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     int optInFastPeriod,
                     int optInSlowPeriod,
                     int optInSignalPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outSMI[],
                     double outSMISignal[] )
   {
      double kSlow = 0;
      double kFast = 0;
      double kSignal = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double emaSlowNum = 0;
      double emaSlowDen = 0;
      double emaFastNum = 0;
      double emaFastDen = 0;
      double sumSlowNum = 0;
      double sumSlowDen = 0;
      double sumFastNum = 0;
      double sumFastDen = 0;
      double sumSignal = 0;
      double num = 0;
      double den = 0;
      double halfDen = 0;
      double smiValue = 0;
      double prevSignal = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int nFast = 0;
      int nSignal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 2;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outSMI == outSMISignal ) {
         return RetCode.BadParam ;
      }
      lookbackTotal = SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      kSlow = 2.0 / (double)(optInSlowPeriod + 1);
      kFast = 2.0 / (double)(optInFastPeriod + 1);
      kSignal = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSlow = EMA_Lookback(optInSlowPeriod);
      lookbackFast = EMA_Lookback(optInFastPeriod);
      emaSlowNum = 0.0;
      emaSlowDen = 0.0;
      emaFastNum = 0.0;
      emaFastDen = 0.0;
      prevSignal = 0.0;
      smiValue = 0.0;
      sumSlowNum = 0.0;
      sumSlowDen = 0.0;
      sumFastNum = 0.0;
      sumFastDen = 0.0;
      sumSignal = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = 0 - 1;
      lowestIdx = 0 - 1;
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInTimePeriod - 1);
      nBar = 0;
      while( today <= startIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp < lowest ) {
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
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = (double)inClose[today] - (highest + lowest) * 0.5;
         if( nBar < optInSlowPeriod ) {
            sumSlowNum = sumSlowNum + num;
            sumSlowDen = sumSlowDen + den;
            if( nBar == optInSlowPeriod - 1 ) {
               emaSlowNum = sumSlowNum / optInSlowPeriod;
               emaSlowDen = sumSlowDen / optInSlowPeriod;
            }
         } else {
            emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
            emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         }
         if( nBar >= lookbackSlow ) {
            nFast = nBar - lookbackSlow;
            if( nFast < optInFastPeriod ) {
               sumFastNum = sumFastNum + emaSlowNum;
               sumFastDen = sumFastDen + emaSlowDen;
               if( nFast == optInFastPeriod - 1 ) {
                  emaFastNum = sumFastNum / optInFastPeriod;
                  emaFastDen = sumFastDen / optInFastPeriod;
               }
            } else {
               emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
               emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
            }
         }
         if( nBar >= lookbackSlow + lookbackFast ) {
            nSignal = nBar - lookbackSlow - lookbackFast;
            halfDen = 0.5 * emaFastDen;
            if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
               smiValue = 100.0 * emaFastNum / halfDen;
            } else {
               smiValue = 0.0;
            }
            if( nSignal < optInSignalPeriod ) {
               sumSignal = sumSignal + smiValue;
               if( nSignal == optInSignalPeriod - 1 ) {
                  prevSignal = sumSignal / optInSignalPeriod;
               }
            } else {
               prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outSMI[0] = smiValue;
      outSMISignal[0] = prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp < lowest ) {
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
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = (double)inClose[today] - (highest + lowest) * 0.5;
         emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
         emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
         emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
         halfDen = 0.5 * emaFastDen;
         if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
            smiValue = 100.0 * emaFastNum / halfDen;
         } else {
            smiValue = 0.0;
         }
         prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
         outSMI[outIdx] = smiValue;
         outSMISignal[outIdx] = prevSignal;
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Stochastic Momentum Index: where the close sits relative to the
    * **midpoint** of the recent high/low range, double-smoothed. Lane's
    * stochastic measures the close against the bottom of the range; Blau
    * measures it against the middle, then smooths numerator and denominator
    * separately with two exponential averages before dividing, which is what
    * buys the low-lag, smooth-contoured curve. The result runs -100 to +100
    * rather than 0 to 100, so the zero line is the reference: positive means
    * the close is above the midpoint of its range, negative below. Extreme
    * readings mark overbought and oversold conditions, and crossings of the
    * signal line are the usual trade trigger.
    * <p><b>Formula</b>
    * <pre>{@code
    * HH = MAX(high, timePeriod);  LL = MIN(low, timePeriod)
    * num = close - 0.5 * (HH + LL);  den = HH - LL
    * SMI = 100 * EMA(EMA(num, slowPeriod), fastPeriod) / (0.5 * EMA(EMA(den, slowPeriod), fastPeriod))
    * Signal = EMA(SMI, signalPeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose bars are all flat (every high equal to its low) leaves both the numerator and the denominator at zero. Rather than divide, SMI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up. Implementations seeding from a single first sample — Tulip and TradingView among them — differ over the transient and agree once it decays.</li>
    * <li>One output range covers both outputs, so the SMI values consumed by the signal line's own warm-up are not published.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SMI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Period of the high/low range (default 13; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastPeriod Period of the second smoothing, applied to the
    *        first (default 2; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outSMI Stochastic Momentum Index, -100 to +100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSMISignal Exponential average of the SMI line. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCH
    * @see Core#STOCHRSI
    * @see Core#WILLR
    * @see Core#MACD
    */
   public OutRange SMI( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        double outSMI[],
                        double outSMISignal[] )
   {
      requireIndexRange("SMI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SMI", "inHigh", inHigh, guardInLen);
      requireLength("SMI", "inLow", inLow, guardInLen);
      requireLength("SMI", "inClose", inClose, guardInLen);
      requireLength("SMI", "outSMI", outSMI, guardOutLen);
      requireLength("SMI", "outSMISignal", outSMISignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SMI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal);
      if( retCode != RetCode.Success ) {
         throw failure("SMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Stochastic Momentum Index: where the close sits relative to the
    * **midpoint** of the recent high/low range, double-smoothed. Lane's
    * stochastic measures the close against the bottom of the range; Blau
    * measures it against the middle, then smooths numerator and denominator
    * separately with two exponential averages before dividing, which is what
    * buys the low-lag, smooth-contoured curve. The result runs -100 to +100
    * rather than 0 to 100, so the zero line is the reference: positive means
    * the close is above the midpoint of its range, negative below. Extreme
    * readings mark overbought and oversold conditions, and crossings of the
    * signal line are the usual trade trigger.
    * <p><b>Formula</b>
    * <pre>{@code
    * HH = MAX(high, timePeriod);  LL = MIN(low, timePeriod)
    * num = close - 0.5 * (HH + LL);  den = HH - LL
    * SMI = 100 * EMA(EMA(num, slowPeriod), fastPeriod) / (0.5 * EMA(EMA(den, slowPeriod), fastPeriod))
    * Signal = EMA(SMI, signalPeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose bars are all flat (every high equal to its low) leaves both the numerator and the denominator at zero. Rather than divide, SMI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up. Implementations seeding from a single first sample — Tulip and TradingView among them — differ over the transient and agree once it decays.</li>
    * <li>One output range covers both outputs, so the SMI values consumed by the signal line's own warm-up are not published.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SMI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInTimePeriod Period of the high/low range (default 13; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastPeriod Period of the second smoothing, applied to the
    *        first (default 2; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the first smoothing, applied to the raw
    *        momentum (default 25; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outSMI Stochastic Momentum Index, -100 to +100. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSMISignal Exponential average of the SMI line. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCH
    * @see Core#STOCHRSI
    * @see Core#WILLR
    * @see Core#MACD
    */
   public OutRange SMI( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        double outSMI[],
                        double outSMISignal[] )
   {
      requireIndexRange("SMI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SMI", "inHigh", inHigh, guardInLen);
      requireLength("SMI", "inLow", inLow, guardInLen);
      requireLength("SMI", "inClose", inClose, guardInLen);
      requireLength("SMI", "outSMI", outSMI, guardOutLen);
      requireLength("SMI", "outSMISignal", outSMISignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SMI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal);
      if( retCode != RetCode.Success ) {
         throw failure("SMI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SMI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SMI} over the same series.
    * Open with {@link Core#SMI_Open}; there is no close — the handle is
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
   public static final class SMI_Stream {
      Core core;
      int optInTimePeriod;
      int optInFastPeriod;
      int optInSlowPeriod;
      int optInSignalPeriod;
      double kSlow;
      double kFast;
      double kSignal;
      double highest;
      double lowest;
      double emaSlowNum;
      double emaSlowDen;
      double emaFastNum;
      double emaFastDen;
      double prevSignal;
      int trailingIdx;
      int highestIdx;
      int lowestIdx;
      int i;
      int today;
      int xMask;
      double[] x_inHigh;
      double[] x_inLow;
      double[] x_inClose;
      double cur_outSMI;
      double cur_outSMISignal;
      Value cachedValue;
      int outRangeBegIdx;
      int outRangeCount;

      SMI_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#SMI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      SMI_Stream( SMI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.kSlow = other.kSlow;
         this.kFast = other.kFast;
         this.kSignal = other.kSignal;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.emaSlowNum = other.emaSlowNum;
         this.emaSlowDen = other.emaSlowDen;
         this.emaFastNum = other.emaFastNum;
         this.emaFastDen = other.emaFastDen;
         this.prevSignal = other.prevSignal;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
         this.cur_outSMI = other.cur_outSMI;
         this.cur_outSMISignal = other.cur_outSMISignal;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( SMI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.kSlow = other.kSlow;
         this.kFast = other.kFast;
         this.kSignal = other.kSignal;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.emaSlowNum = other.emaSlowNum;
         this.emaSlowDen = other.emaSlowDen;
         this.emaFastNum = other.emaFastNum;
         this.emaFastDen = other.emaFastDen;
         this.prevSignal = other.prevSignal;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inHigh != null && this.x_inHigh.length == other.x_inHigh.length ) {
            System.arraycopy( other.x_inHigh, 0, this.x_inHigh, 0, other.x_inHigh.length );
         } else {
            this.x_inHigh = other.x_inHigh.clone();
         }
         if( this.x_inLow != null && this.x_inLow.length == other.x_inLow.length ) {
            System.arraycopy( other.x_inLow, 0, this.x_inLow, 0, other.x_inLow.length );
         } else {
            this.x_inLow = other.x_inLow.clone();
         }
         if( this.x_inClose != null && this.x_inClose.length == other.x_inClose.length ) {
            System.arraycopy( other.x_inClose, 0, this.x_inClose, 0, other.x_inClose.length );
         } else {
            this.x_inClose = other.x_inClose.clone();
         }
         this.cur_outSMI = other.cur_outSMI;
         this.cur_outSMISignal = other.cur_outSMISignal;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<SMI_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param smi Stochastic Momentum Index, -100 to +100.
       * @param smiSignal Exponential average of the SMI line.
       */
      public record Value(double smi, double smiSignal) { }

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
      public Value update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("SMI update: BadParam", RetCode.BadParam);
         core.SMI_StepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         this.cachedValue = new Value(this.cur_outSMI, this.cur_outSMISignal);
         return this.cachedValue;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outSMI[], double outSMISignal[] ) {
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outSMI.length < barCount || outSMISignal.length < barCount || (Object)outSMI == (Object)inHigh || (Object)outSMI == (Object)inLow || (Object)outSMI == (Object)inClose || (Object)outSMISignal == (Object)inHigh || (Object)outSMISignal == (Object)inLow || (Object)outSMISignal == (Object)inClose || (Object)outSMI == (Object)outSMISignal )
            throw new TaLibArgumentException("SMI updateAndFill: BadParam", RetCode.BadParam);
         int done = 0;
         try {
            for( int i = 0; i < barCount; i++ ) {
               if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
                  throw new TaLibArgumentException("SMI updateAndFill: BadParam", RetCode.BadParam);
               core.SMI_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
               outSMI[i] = this.cur_outSMI;
               outSMISignal[i] = this.cur_outSMISignal;
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               done = i + 1;
            }
         } finally {
            if( done > 0 ) this.cachedValue = new Value(this.cur_outSMI, this.cur_outSMISignal);
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public Value peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("SMI peek: BadParam", RetCode.BadParam);
         SMI_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new SMI_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.SMI_StepImpl(scratch, inHigh, inLow, inClose);
         return new Value(scratch.cur_outSMI, scratch.cur_outSMISignal);
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
      public SMI_Stream copy() {
         return new SMI_Stream(this);
      }
   }
   void SMI_StepImpl( SMI_Stream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double num = 0.0;
      double den = 0.0;
      double halfDen = 0.0;
      double smiValue = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inHigh[sp.today & sp.xMask] = inHigh;
      sp.x_inLow[sp.today & sp.xMask] = inLow;
      sp.x_inClose[sp.today & sp.xMask] = inClose;
      /* Set the lowest low */
      tmp = sp.x_inLow[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i & sp.xMask];
            if( tmp < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      /* Set the highest high */
      tmp = sp.x_inHigh[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i & sp.xMask];
            if( tmp > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      den = sp.highest - sp.lowest;
      num = sp.x_inClose[sp.today & sp.xMask] - (sp.highest + sp.lowest) * 0.5;
      sp.emaSlowNum = Math.fma(num - sp.emaSlowNum, sp.kSlow, sp.emaSlowNum);
      sp.emaSlowDen = Math.fma(den - sp.emaSlowDen, sp.kSlow, sp.emaSlowDen);
      sp.emaFastNum = Math.fma(sp.emaSlowNum - sp.emaFastNum, sp.kFast, sp.emaFastNum);
      sp.emaFastDen = Math.fma(sp.emaSlowDen - sp.emaFastDen, sp.kFast, sp.emaFastDen);
      /* Guard with TA_IS_ZERO, not an exact `halfDen != 0.0`: a machine-flat
       * window leaves a sub-epsilon residue that an exact check would divide
       * into noise (issue #107 / STOCHRSI). A window whose bars are all
       * H == L makes num zero too, so this is 0/0, and the neutral 0.0 is the
       * CCI (#7) and IMI (#112) convention.
       */
      halfDen = 0.5 * sp.emaFastDen;
      if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
         smiValue = 100.0 * sp.emaFastNum / halfDen;
      } else {
         smiValue = 0.0;
      }
      sp.prevSignal = Math.fma(smiValue - sp.prevSignal, sp.kSignal, sp.prevSignal);
      sp.cur_outSMI = smiValue;
      sp.cur_outSMISignal = sp.prevSignal;
      sp.trailingIdx = sp.trailingIdx + 1;
      sp.today = sp.today + 1;
   }
   private RetCode SMI_OpenImpl( SMI_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outSMI[], double outSMISignal[], int outStride )
   {
      double kSlow = 0;
      double kFast = 0;
      double kSignal = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double emaSlowNum = 0;
      double emaSlowDen = 0;
      double emaFastNum = 0;
      double emaFastDen = 0;
      double sumSlowNum = 0;
      double sumSlowDen = 0;
      double sumFastNum = 0;
      double sumFastDen = 0;
      double sumSignal = 0;
      double num = 0;
      double den = 0;
      double halfDen = 0;
      double smiValue = 0;
      double prevSignal = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int nFast = 0;
      int nSignal = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 2;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 25;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      lookbackTotal = SMI_Lookback(optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      /* Blau's pipeline in one pass. A composed form is not available: the
       * streaming producer model carries exactly one intermediate series
       * (streaming.rs:2076) and SMI has two, the numerator and the denominator,
       * which must be smoothed separately before they are divided.
       *
       * Each of the three stages seeds the way ema.c does -- a simple average of
       * that stage's first 'period' inputs -- so the result is bit-identical to
       * TA_MAX + TA_MIN followed by four TA_EMA calls. The stage boundaries below
       * are the callee LOOKBACKS, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: each stage then seeds
       * from the values its predecessor would have published, exactly as the
       * composed form does. The seed sums accumulate from 0.0 in production
       * order; do not reorder or fuse them (0.0+x is not x for x=-0.0).
       *
       * TA_GetCompatibility() is deliberately NOT consulted, for the reason
       * spelled out in efi.c: ema.c's TA_COMPATIBILITY_METASTOCK seeding arm is
       * preserved for the functions that already shipped with it and dropped from
       * new ones, and it is not reachable at all from the Rust, Java and C# APIs.
       * The seeding choice itself is measured in docs/studies/ema-seeding/README.md.
       */
      kSlow = 2.0 / (double)(optInSlowPeriod + 1);
      kFast = 2.0 / (double)(optInFastPeriod + 1);
      kSignal = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSlow = EMA_Lookback(optInSlowPeriod);
      lookbackFast = EMA_Lookback(optInFastPeriod);
      emaSlowNum = 0.0;
      emaSlowDen = 0.0;
      emaFastNum = 0.0;
      emaFastDen = 0.0;
      prevSignal = 0.0;
      smiValue = 0.0;
      sumSlowNum = 0.0;
      sumSlowDen = 0.0;
      sumFastNum = 0.0;
      sumFastDen = 0.0;
      sumSignal = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = 0 - 1;
      lowestIdx = 0 - 1;
      /* The first bar carrying a full high/low window. */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInTimePeriod - 1);
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
       * that completes the signal seed, so it produces the first output pair.
       */
      while( today <= startIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = inClose[today] - (highest + lowest) * 0.5;
         /* Stage 1: the slow EMA, over the raw momentum. */
         if( nBar < optInSlowPeriod ) {
            sumSlowNum = sumSlowNum + num;
            sumSlowDen = sumSlowDen + den;
            if( nBar == optInSlowPeriod - 1 ) {
               emaSlowNum = sumSlowNum / optInSlowPeriod;
               emaSlowDen = sumSlowDen / optInSlowPeriod;
            }
         } else {
            emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
            emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         }
         /* Stage 2: the fast EMA, over what stage 1 publishes.
          *
          * The stage counters are compared BEFORE they are subtracted, never
          * after. Writing this as `nFast = nBar - lookbackSlow; if( nFast >= 0 )`
          * is correct in C, where the counters are signed, and broken everywhere
          * else: the Rust backend renders them as `usize`, so the subtraction
          * underflows for the first lookbackSlow bars -- a panic in a debug build
          * and a wrap in release. It would also be invisible to the cross-language
          * gate, which runs release servers at unstable period 0, where the branch
          * the wrap wrongly takes happens to be a no-op because both accumulators
          * are still 0.0.
          */
         if( nBar >= lookbackSlow ) {
            nFast = nBar - lookbackSlow;
            if( nFast < optInFastPeriod ) {
               sumFastNum = sumFastNum + emaSlowNum;
               sumFastDen = sumFastDen + emaSlowDen;
               if( nFast == optInFastPeriod - 1 ) {
                  emaFastNum = sumFastNum / optInFastPeriod;
                  emaFastDen = sumFastDen / optInFastPeriod;
               }
            } else {
               emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
               emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
            }
         }
         /* Stage 3: the SMI line, then the signal EMA over it. */
         if( nBar >= lookbackSlow + lookbackFast ) {
            nSignal = nBar - lookbackSlow - lookbackFast;
            halfDen = 0.5 * emaFastDen;
            if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
               smiValue = 100.0 * emaFastNum / halfDen;
            } else {
               smiValue = 0.0;
            }
            if( nSignal < optInSignalPeriod ) {
               sumSignal = sumSignal + smiValue;
               if( nSignal == optInSignalPeriod - 1 ) {
                  prevSignal = sumSignal / optInSignalPeriod;
               }
            } else {
               prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outSMI[0 * outStride] = smiValue;
      outSMISignal[0 * outStride] = prevSignal;
      outIdx = 1;
      /* Stable zone. Every stage is a pure recursion from here on. */
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         den = highest - lowest;
         num = inClose[today] - (highest + lowest) * 0.5;
         emaSlowNum = Math.fma(num - emaSlowNum, kSlow, emaSlowNum);
         emaSlowDen = Math.fma(den - emaSlowDen, kSlow, emaSlowDen);
         emaFastNum = Math.fma(emaSlowNum - emaFastNum, kFast, emaFastNum);
         emaFastDen = Math.fma(emaSlowDen - emaFastDen, kFast, emaFastDen);
         /* Guard with TA_IS_ZERO, not an exact `halfDen != 0.0`: a machine-flat
          * window leaves a sub-epsilon residue that an exact check would divide
          * into noise (issue #107 / STOCHRSI). A window whose bars are all
          * H == L makes num zero too, so this is 0/0, and the neutral 0.0 is the
          * CCI (#7) and IMI (#112) convention.
          */
         halfDen = 0.5 * emaFastDen;
         if( !((-0.00000000000001 < halfDen) && (halfDen < 0.00000000000001)) ) {
            smiValue = 100.0 * emaFastNum / halfDen;
         } else {
            smiValue = 0.0;
         }
         prevSignal = Math.fma(smiValue - prevSignal, kSignal, prevSignal);
         outSMI[outIdx * outStride] = smiValue;
         outSMISignal[outIdx * outStride] = prevSignal;
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
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
      double[] capX_inHigh = new double[physX];
      double[] capX_inLow = new double[physX];
      double[] capX_inClose = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
         capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.kSlow = kSlow;
      sp.kFast = kFast;
      sp.kSignal = kSignal;
      sp.highest = highest;
      sp.lowest = lowest;
      sp.emaSlowNum = emaSlowNum;
      sp.emaSlowDen = emaSlowDen;
      sp.emaFastNum = emaFastNum;
      sp.emaFastDen = emaFastDen;
      sp.prevSignal = prevSignal;
      sp.trailingIdx = trailingIdx;
      sp.highestIdx = highestIdx;
      sp.lowestIdx = lowestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.cur_outSMI = outSMI[(outNBElement.value - 1) * outStride];
      sp.cur_outSMISignal = outSMISignal[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new SMI_Stream.Value(sp.cur_outSMI, sp.cur_outSMISignal);
      return RetCode.Success;
   }
   /* SMI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SMI_Stream SMI_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outSMI[], double outSMISignal[] )
   {
      SMI_Stream sp = new SMI_Stream(this);
      RetCode retCode = SMI_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SMI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SMI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SMI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind SMI_Open (composition seam). */
   SMI_Stream SMI_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      SMI_Stream sp = new SMI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outSMI = new double[1];
      double[] sink_outSMISignal = new double[1];
      RetCode retCode = SMI_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outSMI, sink_outSMISignal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SMI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SMI open: internal error", retCode);
      }
      throw new TaLibArgumentException("SMI open: " + retCode, retCode);
   }
   /**
    * Open a live SMI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SMI} at that bar.
    * <p>The history must hold at least {@code SMI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public SMI_Stream SMI_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      return SMI_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#SMI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#SMI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link SMI_Stream#outRange()}.
    */
   public SMI_Stream SMI_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, double outSMI[], double outSMISignal[] )
   {
      if( (Object)outSMI == (Object)inHigh || (Object)outSMI == (Object)inLow || (Object)outSMI == (Object)inClose || (Object)outSMISignal == (Object)inHigh || (Object)outSMISignal == (Object)inLow || (Object)outSMISignal == (Object)inClose || (Object)outSMI == (Object)outSMISignal ) {
         throw new TaLibArgumentException("SMI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return SMI_OpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outSMI, outSMISignal);
   }
