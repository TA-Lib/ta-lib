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
 *  100626 MF,CC  Initial version (#473).
 */

   /**
    * Number of leading input bars {@link Core#pso} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastK_Period Time period for building the Fast-K line (default
    *        8; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInEMAPeriod Period of each of the two smoothing passes (default
    *        5; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int psoLookback( int optInFastK_Period, int optInEMAPeriod )
   {
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 8;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInEMAPeriod == Integer.MIN_VALUE ) {
         optInEMAPeriod = 5;
      } else if( optInEMAPeriod < 1 || optInEMAPeriod > 100000 ) {
         return -1;
      }
      /* One Fast-K window, then the two EMA warm-ups the author stacks on top of
       * it: the first smooths the normalised Fast-K, the second smooths the
       * first. Both terms are exactly the lookback of the function they come
       * from, so neither is restated here -- which is also what makes PSO
       * inherit TA_FUNC_UNST_EMA from its callee rather than take an id of its
       * own, and what carries the Auto warm-up levels of #492 through both
       * passes without this file knowing their rule.
       */
      return optInFastK_Period - 1 + emaLookback(optInEMAPeriod) + emaLookback(optInEMAPeriod) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#pso}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInFastK_Period Time period for building the Fast-K line (default
    *        8; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInEMAPeriod Period of each of the two smoothing passes (default
    *        5; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int psoDisplayShift( int optInFastK_Period, int optInEMAPeriod, int outputIdx )
   {
      if( psoLookback( optInFastK_Period, optInEMAPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode psoImpl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    double inClose[],
                    int optInFastK_Period,
                    int optInEMAPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double emaK = 0;
      double emaBeta = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double fastK = 0;
      double nsk = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      int lookbackTotal = 0;
      int lookbackEMA = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 8;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInEMAPeriod == Integer.MIN_VALUE ) {
         optInEMAPeriod = 5;
      } else if( optInEMAPeriod < 1 || optInEMAPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = psoLookback(optInFastK_Period, optInEMAPeriod);
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
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      /* Leibfarth's pipeline in one pass: a Fast-K window, the affine step that
       * centres it on zero, two EMA passes and the squash.
       *
       * The affine step comes BEFORE the smoothing, as the article's listing
       * spells it. Moving it after is equal in real arithmetic and differs by up
       * to 6.0e-16 absolute in doubles, which no golden at a sane tolerance can
       * see; only the composite gate against TA_STOCHF + TA_EMA + TA_EMA can.
       *
       * Each pass seeds the way ema.c does -- a simple average of that pass's
       * first optInEMAPeriod inputs, summed from 0.0 in production order -- so
       * the result is bit-identical to that composed chain. The stage boundary
       * below is the callee LOOKBACK, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second pass then
       * seeds from the values the first would have published, exactly as the
       * composed form does.
       *
       * At optInEMAPeriod == 1 ema.c takes an explicit copy path, because its
       * recursion at a k of 1.0 and a beta of 0.0 does not keep the sign of a
       * -0.0 input. The recursion below is left to run instead: its input is
       * 0.1*(fastK - 50.0), and x - x is +0.0 in every rounding mode, so -0.0
       * cannot reach it. The composite gate runs 5/1 and compares bitwise.
       */
      emaBeta = (double)(optInEMAPeriod - 1) / (double)(optInEMAPeriod + 1);
      emaK = 1.0 - emaBeta;
      emaBeta = 1.0 - emaK;
      lookbackEMA = emaLookback(optInEMAPeriod);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = -1;
      lowestIdx = -1;
      /* The first bar carrying a full Fast-K window. */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInFastK_Period - 1);
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
       * that completes the second pass's seed, so it produces the first output.
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
         /* Fast-K, spelled as stochf.c spells it: divide by the range itself and
          * scale by 100.0 after, guarded by the very expression the division
          * uses, against ITS OWN two extremes rather than a fixed constant
          * (issue #253).
          *
          * Where STOCHF answers 0.0 on a flat window, PSO answers 50.0, the
          * Fast-K midpoint, so that a flat market reads PSO 0 instead of
          * -tanh(2.5) = -0.9866, a near-extreme oversold reading that nothing in
          * the window supports (#473 Q4, the neutral-point rule of #112).
          */
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         /* Pass 1, over the normalised Fast-K. */
         if( nBar < optInEMAPeriod ) {
            sum1 = sum1 + nsk;
            if( nBar == optInEMAPeriod - 1 ) {
               ema1 = sum1 / optInEMAPeriod;
            }
         } else {
            ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         }
         /* Pass 2, over what pass 1 publishes.
          *
          * The stage counter is compared BEFORE it is subtracted, never after.
          * Writing this as `n2 = nBar - lookbackEMA; if( n2 >= 0 )` is correct in
          * C, where the counters are signed, and broken everywhere else: the Rust
          * backend renders them as usize, so the subtraction underflows for the
          * first lookbackEMA bars -- a panic in a debug build and a wrap in
          * release (the lesson smi.c records).
          */
         if( nBar >= lookbackEMA ) {
            n2 = nBar - lookbackEMA;
            if( n2 < optInEMAPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInEMAPeriod - 1 ) {
                  ema2 = sum2 / optInEMAPeriod;
               }
            } else {
               ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      /* tanh(ss/2) rather than the published (e^ss - 1)/(e^ss + 1): the same
       * function, equal within 2.2e-16 on normal data, exactly odd and well
       * conditioned at zero. TA-Lib does not validate that a close lies inside
       * its bar, so ss is only bounded by [-5, 5] on well-formed input; the
       * literal form emits NaN from a successful call once ss exceeds 709.78,
       * which the house rule of #112 forbids (#473 Q3).
       */
      outReal[0] = Math.tanh(0.5 * ema2);
      outIdx = 1;
      /* Stable zone. Both passes are pure recursions from here on. */
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
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
         outReal[outIdx] = Math.tanh(0.5 * ema2);
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode psoImpl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    float inClose[],
                    int optInFastK_Period,
                    int optInEMAPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double emaK = 0;
      double emaBeta = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double fastK = 0;
      double nsk = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      int lookbackTotal = 0;
      int lookbackEMA = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 8;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInEMAPeriod == Integer.MIN_VALUE ) {
         optInEMAPeriod = 5;
      } else if( optInEMAPeriod < 1 || optInEMAPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = psoLookback(optInFastK_Period, optInEMAPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      emaBeta = (double)(optInEMAPeriod - 1) / (double)(optInEMAPeriod + 1);
      emaK = 1.0 - emaBeta;
      emaBeta = 1.0 - emaK;
      lookbackEMA = emaLookback(optInEMAPeriod);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = -1;
      lowestIdx = -1;
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInFastK_Period - 1);
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
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = ((double)inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         if( nBar < optInEMAPeriod ) {
            sum1 = sum1 + nsk;
            if( nBar == optInEMAPeriod - 1 ) {
               ema1 = sum1 / optInEMAPeriod;
            }
         } else {
            ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         }
         if( nBar >= lookbackEMA ) {
            n2 = nBar - lookbackEMA;
            if( n2 < optInEMAPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInEMAPeriod - 1 ) {
                  ema2 = sum2 / optInEMAPeriod;
               }
            } else {
               ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outReal[0] = Math.tanh(0.5 * ema2);
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
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = ((double)inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
         outReal[outIdx] = Math.tanh(0.5 * ema2);
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero
    * and rescaled, double-smoothed and then squashed into the open interval -1
    * to +1. Leibfarth's reading is that the plain stochastic spends most of its
    * life pinned at one end or the other, so the extremes stop meaning
    * anything; the two exponential passes strip the bar-to-bar noise out of it,
    * and the squash gives back a scale on which the extremes are rare again.
    * Readings beyond ±0.9 are the extremes, and ±0.2 the band Leibfarth watches
    * for the crossing back toward the middle.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/pso">ta-lib.org/functions/pso</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The affine step comes before the smoothing, as the author's listing spells it. Smoothing first and recentring after is the same value in real arithmetic and differs in the last bit or two in doubles.</li>
    * <li>The squash is computed as {@code tanh(SS/2)}, which is the published quotient rewritten. The quotient form is {@code inf/inf} once {@code SS} exceeds 709.78, which a bar whose close lies outside its own high/low range can reach; {@code tanh} saturates at ±1 instead.</li>
    * <li>A Fast-K window whose range is zero reads 50, the midpoint, so a flat market reads PSO 0 rather than the near-extreme the Fast-K convention of 0 would give it. The flatness test is the one STOCHF applies, against the window's own extremes rather than a fixed band.</li>
    * <li>Each exponential pass is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second pass seeds on what the first publishes. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up, through both passes. Implementations seeding each pass from a single first sample differ over the transient and agree once it decays.</li>
    * <li>Leibfarth parameterises the smoothing as the square root of a longer period, 25 by default, which is 5. The length is taken here directly, as an integer, because the sources that follow the square root disagree over how to round it.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#psoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInFastK_Period Time period for building the Fast-K line (default
    *        8; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInEMAPeriod Period of each of the two smoothing passes (default
    *        5; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Premier Stochastic Oscillator, -1 to +1. Must hold at least
    *        {@code endIdx - max(startIdx, psoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#stochf
    * @see Core#stoch
    * @see Core#smi
    * @see Core#willr
    */
   public OutRange pso( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInFastK_Period,
                        int optInEMAPeriod,
                        double outReal[] )
   {
      requireIndexRange("PSO", startIdx, endIdx);
      int guardStart = clampedStart("PSO", startIdx, psoLookback(optInFastK_Period, optInEMAPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PSO", "inHigh", inHigh, guardInLen);
      requireLength("PSO", "inLow", inLow, guardInLen);
      requireLength("PSO", "inClose", inClose, guardInLen);
      requireLength("PSO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = psoImpl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PSO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Premier Stochastic Oscillator: a short-period Fast %K, recentred on zero
    * and rescaled, double-smoothed and then squashed into the open interval -1
    * to +1. Leibfarth's reading is that the plain stochastic spends most of its
    * life pinned at one end or the other, so the extremes stop meaning
    * anything; the two exponential passes strip the bar-to-bar noise out of it,
    * and the squash gives back a scale on which the extremes are rare again.
    * Readings beyond ±0.9 are the extremes, and ±0.2 the band Leibfarth watches
    * for the crossing back toward the middle.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/pso">ta-lib.org/functions/pso</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The affine step comes before the smoothing, as the author's listing spells it. Smoothing first and recentring after is the same value in real arithmetic and differs in the last bit or two in doubles.</li>
    * <li>The squash is computed as {@code tanh(SS/2)}, which is the published quotient rewritten. The quotient form is {@code inf/inf} once {@code SS} exceeds 709.78, which a bar whose close lies outside its own high/low range can reach; {@code tanh} saturates at ±1 instead.</li>
    * <li>A Fast-K window whose range is zero reads 50, the midpoint, so a flat market reads PSO 0 rather than the near-extreme the Fast-K convention of 0 would give it. The flatness test is the one STOCHF applies, against the window's own extremes rather than a fixed band.</li>
    * <li>Each exponential pass is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second pass seeds on what the first publishes. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up, through both passes. Implementations seeding each pass from a single first sample differ over the transient and agree once it decays.</li>
    * <li>Leibfarth parameterises the smoothing as the square root of a longer period, 25 by default, which is 5. The length is taken here directly, as an integer, because the sources that follow the square root disagree over how to round it.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#psoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInFastK_Period Time period for building the Fast-K line (default
    *        8; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInEMAPeriod Period of each of the two smoothing passes (default
    *        5; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Premier Stochastic Oscillator, -1 to +1. Must hold at least
    *        {@code endIdx - max(startIdx, psoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#stochf
    * @see Core#stoch
    * @see Core#smi
    * @see Core#willr
    */
   public OutRange pso( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInFastK_Period,
                        int optInEMAPeriod,
                        double outReal[] )
   {
      requireIndexRange("PSO", startIdx, endIdx);
      int guardStart = clampedStart("PSO", startIdx, psoLookback(optInFastK_Period, optInEMAPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PSO", "inHigh", inHigh, guardInLen);
      requireLength("PSO", "inLow", inLow, guardInLen);
      requireLength("PSO", "inClose", inClose, guardInLen);
      requireLength("PSO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = psoImpl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PSO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PSO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#pso} over the same series.
    * Open with {@link Core#psoOpen}; there is no close — the handle is
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
   public static final class PsoStream {
      private Core core;
      private int optInFastK_Period;
      private int optInEMAPeriod;
      private double emaK;
      private double emaBeta;
      private double highest;
      private double lowest;
      private double ema1;
      private double ema2;
      private int trailingIdx;
      private int highestIdx;
      private int lowestIdx;
      private int i;
      private int today;
      private int xMask;
      private double[] x_inHigh;
      private double[] x_inLow;
      private double[] x_inClose;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private PsoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#pso} reports over the same bars: the
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
            throw failure("PSO advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private PsoStream( PsoStream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInEMAPeriod = other.optInEMAPeriod;
         this.emaK = other.emaK;
         this.emaBeta = other.emaBeta;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.ema1 = other.ema1;
         this.ema2 = other.ema2;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("PSO update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("PSO update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.psoStepImpl(this, inHigh, inLow, inClose);
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
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("PSO peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         PsoStream sp = this;
         double tmp = 0.0;
         double fastK = 0.0;
         double nsk = 0.0;
         double cur_outReal = 0.0;
         double ema1 = sp.ema1;
         double ema2 = sp.ema2;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inHigh;
         pkSlot1 = sp.today & sp.xMask;
         pkVal1 = inLow;
         pkSlot2 = sp.today & sp.xMask;
         pkVal2 = inClose;
         /* Set the lowest low */
         tmp = ((sp.today & sp.xMask) != pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1;
         if( lowestIdx < sp.trailingIdx ) {
            lowestIdx = sp.trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            i = lowestIdx;
            while( ++i <= sp.today ) {
               tmp = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = sp.today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = ((sp.today & sp.xMask) != pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0;
         if( highestIdx < sp.trailingIdx ) {
            highestIdx = sp.trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= sp.today ) {
               tmp = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = sp.today;
            highest = tmp;
         }
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = ((((sp.today & sp.xMask) != pkSlot2) ? sp.x_inClose[sp.today & sp.xMask] : pkVal2) - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         ema1 = Math.fma(sp.emaBeta, ema1, sp.emaK * nsk);
         ema2 = Math.fma(sp.emaBeta, ema2, sp.emaK * ema1);
         cur_outReal = Math.tanh(0.5 * ema2);
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
      public PsoStream clone() {
         return new PsoStream(this);
      }
   }
   private void psoStepImpl( PsoStream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double fastK = 0.0;
      double nsk = 0.0;
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
      if( !(Math.abs(sp.highest - sp.lowest) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest))) ) {
         fastK = (sp.x_inClose[sp.today & sp.xMask] - sp.lowest) / (sp.highest - sp.lowest) * 100.0;
      } else {
         fastK = 50.0;
      }
      nsk = 0.1 * (fastK - 50.0);
      sp.ema1 = Math.fma(sp.emaBeta, sp.ema1, sp.emaK * nsk);
      sp.ema2 = Math.fma(sp.emaBeta, sp.ema2, sp.emaK * sp.ema1);
      sp.cur_outReal = Math.tanh(0.5 * sp.ema2);
      sp.trailingIdx = sp.trailingIdx + 1;
      sp.today = sp.today + 1;
   }
   private RetCode psoOpenImpl( PsoStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInEMAPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double emaK = 0;
      double emaBeta = 0;
      double highest = 0;
      double lowest = 0;
      double tmp = 0;
      double fastK = 0;
      double nsk = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      int lookbackTotal = 0;
      int lookbackEMA = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      int outIdx = 0;
      int nBar = 0;
      int n2 = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 8;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInEMAPeriod == Integer.MIN_VALUE ) {
         optInEMAPeriod = 5;
      } else if( optInEMAPeriod < 1 || optInEMAPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lookbackTotal = psoLookback(optInFastK_Period, optInEMAPeriod);
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
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      outBegIdx.value = startIdx;
      /* Leibfarth's pipeline in one pass: a Fast-K window, the affine step that
       * centres it on zero, two EMA passes and the squash.
       *
       * The affine step comes BEFORE the smoothing, as the article's listing
       * spells it. Moving it after is equal in real arithmetic and differs by up
       * to 6.0e-16 absolute in doubles, which no golden at a sane tolerance can
       * see; only the composite gate against TA_STOCHF + TA_EMA + TA_EMA can.
       *
       * Each pass seeds the way ema.c does -- a simple average of that pass's
       * first optInEMAPeriod inputs, summed from 0.0 in production order -- so
       * the result is bit-identical to that composed chain. The stage boundary
       * below is the callee LOOKBACK, not (period-1), so that a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA) folds in: the second pass then
       * seeds from the values the first would have published, exactly as the
       * composed form does.
       *
       * At optInEMAPeriod == 1 ema.c takes an explicit copy path, because its
       * recursion at a k of 1.0 and a beta of 0.0 does not keep the sign of a
       * -0.0 input. The recursion below is left to run instead: its input is
       * 0.1*(fastK - 50.0), and x - x is +0.0 in every rounding mode, so -0.0
       * cannot reach it. The composite gate runs 5/1 and compares bitwise.
       */
      emaBeta = (double)(optInEMAPeriod - 1) / (double)(optInEMAPeriod + 1);
      emaK = 1.0 - emaBeta;
      emaBeta = 1.0 - emaK;
      lookbackEMA = emaLookback(optInEMAPeriod);
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      highest = 0.0;
      lowest = 0.0;
      highestIdx = -1;
      lowestIdx = -1;
      /* The first bar carrying a full Fast-K window. */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + (optInFastK_Period - 1);
      nBar = 0;
      /* Warm-up. Runs through startIdx inclusive: the last pass here is the one
       * that completes the second pass's seed, so it produces the first output.
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
         /* Fast-K, spelled as stochf.c spells it: divide by the range itself and
          * scale by 100.0 after, guarded by the very expression the division
          * uses, against ITS OWN two extremes rather than a fixed constant
          * (issue #253).
          *
          * Where STOCHF answers 0.0 on a flat window, PSO answers 50.0, the
          * Fast-K midpoint, so that a flat market reads PSO 0 instead of
          * -tanh(2.5) = -0.9866, a near-extreme oversold reading that nothing in
          * the window supports (#473 Q4, the neutral-point rule of #112).
          */
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         /* Pass 1, over the normalised Fast-K. */
         if( nBar < optInEMAPeriod ) {
            sum1 = sum1 + nsk;
            if( nBar == optInEMAPeriod - 1 ) {
               ema1 = sum1 / optInEMAPeriod;
            }
         } else {
            ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         }
         /* Pass 2, over what pass 1 publishes.
          *
          * The stage counter is compared BEFORE it is subtracted, never after.
          * Writing this as `n2 = nBar - lookbackEMA; if( n2 >= 0 )` is correct in
          * C, where the counters are signed, and broken everywhere else: the Rust
          * backend renders them as usize, so the subtraction underflows for the
          * first lookbackEMA bars -- a panic in a debug build and a wrap in
          * release (the lesson smi.c records).
          */
         if( nBar >= lookbackEMA ) {
            n2 = nBar - lookbackEMA;
            if( n2 < optInEMAPeriod ) {
               sum2 = sum2 + ema1;
               if( n2 == optInEMAPeriod - 1 ) {
                  ema2 = sum2 / optInEMAPeriod;
               }
            } else {
               ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
            }
         }
         nBar = nBar + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      /* tanh(ss/2) rather than the published (e^ss - 1)/(e^ss + 1): the same
       * function, equal within 2.2e-16 on normal data, exactly odd and well
       * conditioned at zero. TA-Lib does not validate that a close lies inside
       * its bar, so ss is only bounded by [-5, 5] on well-formed input; the
       * literal form emits NaN from a successful call once ss exceeds 709.78,
       * which the house rule of #112 forbids (#473 Q3).
       */
      outReal[0 * outStride] = Math.tanh(0.5 * ema2);
      outIdx = 1;
      /* Stable zone. Both passes are pure recursions from here on. */
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
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            fastK = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            fastK = 50.0;
         }
         nsk = 0.1 * (fastK - 50.0);
         ema1 = Math.fma(emaBeta, ema1, emaK * nsk);
         ema2 = Math.fma(emaBeta, ema2, emaK * ema1);
         outReal[outIdx * outStride] = Math.tanh(0.5 * ema2);
         outIdx = outIdx + 1;
         trailingIdx = trailingIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
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
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInEMAPeriod = optInEMAPeriod;
      sp.emaK = emaK;
      sp.emaBeta = emaBeta;
      sp.highest = highest;
      sp.lowest = lowest;
      sp.ema1 = ema1;
      sp.ema2 = ema2;
      sp.trailingIdx = trailingIdx;
      sp.highestIdx = highestIdx;
      sp.lowestIdx = lowestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* psoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PsoStream psoOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInEMAPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PsoStream sp = new PsoStream(this);
      RetCode retCode = psoOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PSO openAndFill", inHigh.length, startIdx, psoLookback(optInFastK_Period, optInEMAPeriod));
      }
      throw streamFailure("PSO openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind psoOpen (composition seam). */
   PsoStream psoOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInEMAPeriod )
   {
      PsoStream sp = new PsoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = psoOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PSO open", inHigh.length, startIdx, psoLookback(optInFastK_Period, optInEMAPeriod));
      }
      throw streamFailure("PSO open", retCode);
   }
   /**
    * Open a live PSO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#pso} at that bar.
    * <p>The history must hold at least {@code psoLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PsoStream psoOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInEMAPeriod )
   {
      requireArgument("PSO open", "inHigh", inHigh);
      requireHistory("PSO open", inHigh.length);
      requireArgument("PSO open", "inLow", inLow);
      requireArgument("PSO open", "inClose", inClose);
      requireHistoryLength("PSO open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("PSO open", "inClose", inClose.length, inHigh.length);
      return psoOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInEMAPeriod);
   }
   /**
    * {@link Core#psoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#pso} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PsoStream#outRange()}.
    */
   public PsoStream psoOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInEMAPeriod, double outReal[] )
   {
      requireArgument("PSO openAndFill", "inHigh", inHigh);
      requireHistory("PSO openAndFill", inHigh.length);
      requireArgument("PSO openAndFill", "inLow", inLow);
      requireArgument("PSO openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("PSO openAndFill", inHigh.length, psoLookback(optInFastK_Period, optInEMAPeriod));
      requireHistoryLength("PSO openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("PSO openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("PSO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw streamFailure("PSO openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return psoOpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInEMAPeriod, outBegIdx, outNBElement, outReal);
   }
