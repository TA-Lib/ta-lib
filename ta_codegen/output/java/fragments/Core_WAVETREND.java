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
 *  100626 MF,CC  Initial version (#476).
 */

   /**
    * Number of leading input bars {@link Core#wavetrend} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInChannelPeriod Period of the price channel, used by both the
    *        average and the deviation (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInAveragePeriod Smoothing for the oscillator line (default 21;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the simple average making the signal
    *        line (default 4; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int wavetrendLookback( int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod )
   {
      if( optInChannelPeriod == Integer.MIN_VALUE ) {
         optInChannelPeriod = 10;
      } else if( optInChannelPeriod < 2 || optInChannelPeriod > 100000 ) {
         return -1;
      }
      if( optInAveragePeriod == Integer.MIN_VALUE ) {
         optInAveragePeriod = 21;
      } else if( optInAveragePeriod < 1 || optInAveragePeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 4;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* Two exponential averages over the channel period -- one of the typical
       * price, one of the absolute distance from it -- then the oscillator's own
       * smoothing and the simple average that makes the signal line. Every term
       * is exactly the lookback of the function it comes from, so none of them is
       * restated here: that is what makes WAVETREND inherit TA_FUNC_UNST_EMA from
       * its callees rather than take an id of its own.
       *
       * The EMA term appears THREE times, so a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, k) moves the lookback by 3k.
       */
      return emaLookback(optInChannelPeriod) + emaLookback(optInChannelPeriod) + emaLookback(optInAveragePeriod) + smaLookback(optInSignalPeriod) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#wavetrend}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInChannelPeriod Period of the price channel, used by both the
    *        average and the deviation (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInAveragePeriod Smoothing for the oscillator line (default 21;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the simple average making the signal
    *        line (default 4; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int wavetrendDisplayShift( int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod, int outputIdx )
   {
      if( wavetrendLookback( optInChannelPeriod, optInAveragePeriod, optInSignalPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode wavetrendImpl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInChannelPeriod,
                          int optInAveragePeriod,
                          int optInSignalPeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outWT1[],
                          double outWT2[] )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double ap = 0;
      double esa = 0;
      double dev = 0;
      double d = 0;
      double ci = 0;
      double wt1 = 0;
      double prevAp = 0;
      double prevEsa = 0;
      double num = 0;
      double scaledDev = 0;
      double sumEsa = 0;
      double sumD = 0;
      double sumCi = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackChannel = 0;
      int lookbackAverage = 0;
      int today = 0;
      int outIdx = 0;
      int nAp = 0;
      int nDev = 0;
      int nCi = 0;
      int nSig = 0;
      double[] wtBuffer;
      int wtBuffer_Idx = 0;
      int maxIdx_wtBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInChannelPeriod == Integer.MIN_VALUE ) {
         optInChannelPeriod = 10;
      } else if( optInChannelPeriod < 2 || optInChannelPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAveragePeriod == Integer.MIN_VALUE ) {
         optInAveragePeriod = 21;
      } else if( optInAveragePeriod < 1 || optInAveragePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 4;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outWT1 == outWT2 ) {
         return RetCode.BAD_PARAM ;
      }
      /* LazyBear's WaveTrend Oscillator (TradingView, 2014), lines 14 to 21: the
       * typical price's distance from its own exponential average, normalised by
       * an exponential average of that distance's absolute value and by Lambert's
       * CCI constant, then smoothed.
       *
       *    ap  = (H + L + C)/3
       *    esa = EMA(ap, n1)
       *    d   = EMA(|ap - esa|, n1)
       *    ci  = (ap - esa) / (0.015 * d)
       *    WT1 = EMA(ci, n2)        WT2 = SMA(WT1, n3)
       *
       * The middle stage is an exponential CCI, not TA_CCI: cci.c averages with
       * an SMA and takes the mean deviation around that window's own SMA, where
       * this uses two exponential averages.
       *
       * Each stage seeds the way ema.c and sma.c seed, and each stage boundary
       * below is the callee's LOOKBACK rather than (period-1), so the result is
       * bit-identical to the composed chain on moving data and a warm unstable
       * period folds in. The two guards are what the chain cannot express; they
       * are the reason this ships as a function.
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      lookbackTotal = wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
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
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      wtBuffer = new double[optInSignalPeriod];
      maxIdx_wtBuffer = (optInSignalPeriod)-1;
      wtBuffer_Idx = 0;
      lookbackChannel = emaLookback(optInChannelPeriod);
      lookbackAverage = emaLookback(optInAveragePeriod);
      beta1 = (double)(optInChannelPeriod - 1) / (double)(optInChannelPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInAveragePeriod - 1) / (double)(optInAveragePeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      esa = 0.0;
      d = 0.0;
      ci = 0.0;
      wt1 = 0.0;
      sumEsa = 0.0;
      sumD = 0.0;
      sumCi = 0.0;
      sumSignal = 0.0;
      nAp = 0;
      /* The fixpoint test compares against the previous bar's pair. Starting both
       * at 0.0 cannot make it fire spuriously: it would need a bar whose typical
       * price and whose exponential average are both exactly zero, and a zero
       * typical price makes the numerator zero anyway, which is what the test
       * would have substituted.
       */
      prevAp = 0.0;
      prevEsa = 0.0;
      today = startIdx - lookbackTotal;
      /* Warm-up. Runs through startIdx inclusive: the last pass is the one that
       * completes the signal window, so it leaves the first output in the state.
       */
      while( today <= startIdx ) {
         ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         /* Stage 1: the exponential average of the typical price. */
         if( nAp < optInChannelPeriod ) {
            sumEsa = sumEsa + ap;
            if( nAp == optInChannelPeriod - 1 ) {
               esa = sumEsa / optInChannelPeriod;
            }
         } else {
            esa = Math.fma(beta1, esa, k1 * ap);
         }
         /* Stage 2: the exponential average of the absolute distance, over what
          * stage 1 publishes. The counter is compared before it is subtracted,
          * never after: the Rust backend renders these as usize.
          */
         if( nAp >= lookbackChannel ) {
            nDev = nAp - lookbackChannel;
            /* GUARD 1, the fixpoint test. With a constant input the exponential
             * step stops moving once k*|ap - esa| falls under half an ulp, and
             * esa then FREEZES up to (n1+1)/4 ulps away from the price. That
             * frozen residue is a real non-zero distance, so the naive form
             * divides it by its own exponential average and walks to
             * +/-1/0.015 = +/-66.67 -- an extreme reading produced by nothing
             * but rounding. When the pair has not moved, the average has reached
             * its fixpoint and the distance is exactly that residue, so the
             * numerator is taken as zero. The test is exact, so it is
             * independent of scale and period, unlike a fixed epsilon band.
             */
            num = ap - esa;
            if( ap == prevAp && esa == prevEsa ) {
               num = 0.0;
            }
            dev = num;
            if( dev < 0.0 ) {
               dev = -dev;
            }
            if( nDev < optInChannelPeriod ) {
               sumD = sumD + dev;
               if( nDev == optInChannelPeriod - 1 ) {
                  d = sumD / optInChannelPeriod;
               }
            } else {
               d = Math.fma(beta1, d, k1 * dev);
            }
            /* Stage 3: the oscillator, then its own smoothing. */
            if( nDev >= lookbackChannel ) {
               nCi = nDev - lookbackChannel;
               /* GUARD 2, the exact divisor. Test the PRODUCT the division uses,
                * not the deviation: 0.015*d underflows to zero while d is still
                * non-zero (#395). A zero divisor is 0/0 -- no distance against no
                * average distance -- so the oscillator reads its neutral 0.0
                * (#112), as tsi.c does.
                */
               scaledDev = 0.015 * d;
               if( scaledDev > 0.0 ) {
                  ci = num / scaledDev;
               } else {
                  ci = 0.0;
               }
               if( nCi < optInAveragePeriod ) {
                  sumCi = sumCi + ci;
                  if( nCi == optInAveragePeriod - 1 ) {
                     wt1 = sumCi / optInAveragePeriod;
                  }
               } else {
                  wt1 = Math.fma(beta2, wt1, k2 * ci);
               }
               if( nCi >= lookbackAverage ) {
                  wtBuffer[wtBuffer_Idx] = wt1;
                  sumSignal = sumSignal + wt1;
                  wtBuffer_Idx++;
                  if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
               }
            }
         }
         prevAp = ap;
         prevEsa = esa;
         nAp = nAp + 1;
         today = today + 1;
      }
      /* The first output. The warm-up's last pass stored this bar's WT1 into the
       * ring and added it to the running sum, so the average is ready here.
       */
      outWT1[0] = wt1;
      outWT2[0] = sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
      /* Stable zone. One bar past the first output every stage is past its seed,
       * at every reachable parameter triple: the shortest case, 2/1/1, arrives
       * with the deviation average and the oscillator smoothing both one bar
       * into their recursions. So nothing here branches on a counter, and the
       * stores always run -- which is what keeps the managed backends' peek
       * frames from carrying a seeded output local.
       */
      while( today <= endIdx ) {
         ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         esa = Math.fma(beta1, esa, k1 * ap);
         num = ap - esa;
         if( ap == prevAp && esa == prevEsa ) {
            num = 0.0;
         }
         dev = num;
         if( dev < 0.0 ) {
            dev = -dev;
         }
         d = Math.fma(beta1, d, k1 * dev);
         scaledDev = 0.015 * d;
         if( scaledDev > 0.0 ) {
            ci = num / scaledDev;
         } else {
            ci = 0.0;
         }
         wt1 = Math.fma(beta2, wt1, k2 * ci);
         wtBuffer[wtBuffer_Idx] = wt1;
         sumSignal = sumSignal + wt1;
         outWT1[outIdx] = wt1;
         outWT2[outIdx] = sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         wtBuffer_Idx++;
         if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
         sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
         prevAp = ap;
         prevEsa = esa;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode wavetrendImpl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInChannelPeriod,
                          int optInAveragePeriod,
                          int optInSignalPeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outWT1[],
                          double outWT2[] )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double ap = 0;
      double esa = 0;
      double dev = 0;
      double d = 0;
      double ci = 0;
      double wt1 = 0;
      double prevAp = 0;
      double prevEsa = 0;
      double num = 0;
      double scaledDev = 0;
      double sumEsa = 0;
      double sumD = 0;
      double sumCi = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackChannel = 0;
      int lookbackAverage = 0;
      int today = 0;
      int outIdx = 0;
      int nAp = 0;
      int nDev = 0;
      int nCi = 0;
      int nSig = 0;
      double[] wtBuffer;
      int wtBuffer_Idx = 0;
      int maxIdx_wtBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInChannelPeriod == Integer.MIN_VALUE ) {
         optInChannelPeriod = 10;
      } else if( optInChannelPeriod < 2 || optInChannelPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAveragePeriod == Integer.MIN_VALUE ) {
         optInAveragePeriod = 21;
      } else if( optInAveragePeriod < 1 || optInAveragePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 4;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outWT1 == outWT2 ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      wtBuffer = new double[optInSignalPeriod];
      maxIdx_wtBuffer = (optInSignalPeriod)-1;
      wtBuffer_Idx = 0;
      lookbackChannel = emaLookback(optInChannelPeriod);
      lookbackAverage = emaLookback(optInAveragePeriod);
      beta1 = (double)(optInChannelPeriod - 1) / (double)(optInChannelPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInAveragePeriod - 1) / (double)(optInAveragePeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      esa = 0.0;
      d = 0.0;
      ci = 0.0;
      wt1 = 0.0;
      sumEsa = 0.0;
      sumD = 0.0;
      sumCi = 0.0;
      sumSignal = 0.0;
      nAp = 0;
      prevAp = 0.0;
      prevEsa = 0.0;
      today = startIdx - lookbackTotal;
      while( today <= startIdx ) {
         ap = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         if( nAp < optInChannelPeriod ) {
            sumEsa = sumEsa + ap;
            if( nAp == optInChannelPeriod - 1 ) {
               esa = sumEsa / optInChannelPeriod;
            }
         } else {
            esa = Math.fma(beta1, esa, k1 * ap);
         }
         if( nAp >= lookbackChannel ) {
            nDev = nAp - lookbackChannel;
            num = ap - esa;
            if( ap == prevAp && esa == prevEsa ) {
               num = 0.0;
            }
            dev = num;
            if( dev < 0.0 ) {
               dev = -dev;
            }
            if( nDev < optInChannelPeriod ) {
               sumD = sumD + dev;
               if( nDev == optInChannelPeriod - 1 ) {
                  d = sumD / optInChannelPeriod;
               }
            } else {
               d = Math.fma(beta1, d, k1 * dev);
            }
            if( nDev >= lookbackChannel ) {
               nCi = nDev - lookbackChannel;
               scaledDev = 0.015 * d;
               if( scaledDev > 0.0 ) {
                  ci = num / scaledDev;
               } else {
                  ci = 0.0;
               }
               if( nCi < optInAveragePeriod ) {
                  sumCi = sumCi + ci;
                  if( nCi == optInAveragePeriod - 1 ) {
                     wt1 = sumCi / optInAveragePeriod;
                  }
               } else {
                  wt1 = Math.fma(beta2, wt1, k2 * ci);
               }
               if( nCi >= lookbackAverage ) {
                  wtBuffer[wtBuffer_Idx] = wt1;
                  sumSignal = sumSignal + wt1;
                  wtBuffer_Idx++;
                  if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
               }
            }
         }
         prevAp = ap;
         prevEsa = esa;
         nAp = nAp + 1;
         today = today + 1;
      }
      outWT1[0] = wt1;
      outWT2[0] = sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
      while( today <= endIdx ) {
         ap = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         esa = Math.fma(beta1, esa, k1 * ap);
         num = ap - esa;
         if( ap == prevAp && esa == prevEsa ) {
            num = 0.0;
         }
         dev = num;
         if( dev < 0.0 ) {
            dev = -dev;
         }
         d = Math.fma(beta1, d, k1 * dev);
         scaledDev = 0.015 * d;
         if( scaledDev > 0.0 ) {
            ci = num / scaledDev;
         } else {
            ci = 0.0;
         }
         wt1 = Math.fma(beta2, wt1, k2 * ci);
         wtBuffer[wtBuffer_Idx] = wt1;
         sumSignal = sumSignal + wt1;
         outWT1[outIdx] = wt1;
         outWT2[outIdx] = sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         wtBuffer_Idx++;
         if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
         sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
         prevAp = ap;
         prevEsa = esa;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * WaveTrend Oscillator: how far the typical price sits from its own
    * exponential average, divided by an exponential average of that distance
    * and by Lambert's CCI constant, then smoothed. The reading is the plain
    * stochastic's complaint answered a different way — rather than bounding the
    * oscillator by construction, it scales it by how far price has recently
    * been travelling, so the same numeric level means the same thing in a quiet
    * market and a fast one. The oscillator line and its short simple average
    * cross; the crossings that matter are the ones beyond the extremes, which
    * the author draws at ±53 and ±60.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/wavetrend">ta-lib.org/functions/wavetrend</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The middle stage is an exponential CCI, not {@code TA_CCI}: CCI averages with a simple moving average and takes the mean deviation around that window's own average, where this uses two exponential averages.</li>
    * <li>On a market that has stopped moving, the exponential average stops moving too — once its step falls under half an ulp it freezes, a few ulps away from the price. That frozen gap is a real non-zero distance, so dividing by its own average walks the oscillator to ±66.67, an extreme reading produced by nothing but rounding. This answers 0 instead, by taking the distance as zero whenever the price and its average both repeat. Implementations without that test drift to the extreme on flat data, at a bar that depends on their arithmetic.</li>
    * <li>A zero divisor is tested on the scaled deviation, the quantity the division actually uses, and answers the neutral 0 rather than dividing.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage before it publishes. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up, and it counts three times because there are three exponential stages.</li>
    * <li>The difference the author also plots is {@code TA_SUB(outWT1, outWT2)}.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#wavetrendLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInChannelPeriod Period of the price channel, used by both the
    *        average and the deviation (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInAveragePeriod Smoothing for the oscillator line (default 21;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the simple average making the signal
    *        line (default 4; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outWT1 WaveTrend oscillator line. Must hold at least
    *        {@code endIdx - max(startIdx, wavetrendLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outWT2 Simple average of the oscillator line. Must hold at least
    *        {@code endIdx - max(startIdx, wavetrendLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#cci
    * @see Core#smi
    * @see Core#stochrsi
    * @see Core#tsi
    */
   public OutRange wavetrend( int startIdx,
                              int endIdx,
                              double inHigh[],
                              double inLow[],
                              double inClose[],
                              int optInChannelPeriod,
                              int optInAveragePeriod,
                              int optInSignalPeriod,
                              double outWT1[],
                              double outWT2[] )
   {
      requireIndexRange("WAVETREND", startIdx, endIdx);
      int guardStart = clampedStart("WAVETREND", startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WAVETREND", "inHigh", inHigh, guardInLen);
      requireLength("WAVETREND", "inLow", inLow, guardInLen);
      requireLength("WAVETREND", "inClose", inClose, guardInLen);
      requireLength("WAVETREND", "outWT1", outWT1, guardOutLen);
      requireLength("WAVETREND", "outWT2", outWT2, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = wavetrendImpl(startIdx, endIdx, inHigh, inLow, inClose, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("WAVETREND", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * WaveTrend Oscillator: how far the typical price sits from its own
    * exponential average, divided by an exponential average of that distance
    * and by Lambert's CCI constant, then smoothed. The reading is the plain
    * stochastic's complaint answered a different way — rather than bounding the
    * oscillator by construction, it scales it by how far price has recently
    * been travelling, so the same numeric level means the same thing in a quiet
    * market and a fast one. The oscillator line and its short simple average
    * cross; the crossings that matter are the ones beyond the extremes, which
    * the author draws at ±53 and ±60.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/wavetrend">ta-lib.org/functions/wavetrend</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The middle stage is an exponential CCI, not {@code TA_CCI}: CCI averages with a simple moving average and takes the mean deviation around that window's own average, where this uses two exponential averages.</li>
    * <li>On a market that has stopped moving, the exponential average stops moving too — once its step falls under half an ulp it freezes, a few ulps away from the price. That frozen gap is a real non-zero distance, so dividing by its own average walks the oscillator to ±66.67, an extreme reading produced by nothing but rounding. This answers 0 instead, by taking the distance as zero whenever the price and its average both repeat. Implementations without that test drift to the extreme on flat data, at a bar that depends on their arithmetic.</li>
    * <li>A zero divisor is tested on the scaled deviation, the quantity the division actually uses, and answers the neutral 0 rather than dividing.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage before it publishes. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up, and it counts three times because there are three exponential stages.</li>
    * <li>The difference the author also plots is {@code TA_SUB(outWT1, outWT2)}.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#wavetrendLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param optInChannelPeriod Period of the price channel, used by both the
    *        average and the deviation (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInAveragePeriod Smoothing for the oscillator line (default 21;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Period of the simple average making the signal
    *        line (default 4; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outWT1 WaveTrend oscillator line. Must hold at least
    *        {@code endIdx - max(startIdx, wavetrendLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outWT2 Simple average of the oscillator line. Must hold at least
    *        {@code endIdx - max(startIdx, wavetrendLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#cci
    * @see Core#smi
    * @see Core#stochrsi
    * @see Core#tsi
    */
   public OutRange wavetrend( int startIdx,
                              int endIdx,
                              float inHigh[],
                              float inLow[],
                              float inClose[],
                              int optInChannelPeriod,
                              int optInAveragePeriod,
                              int optInSignalPeriod,
                              double outWT1[],
                              double outWT2[] )
   {
      requireIndexRange("WAVETREND", startIdx, endIdx);
      int guardStart = clampedStart("WAVETREND", startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WAVETREND", "inHigh", inHigh, guardInLen);
      requireLength("WAVETREND", "inLow", inLow, guardInLen);
      requireLength("WAVETREND", "inClose", inClose, guardInLen);
      requireLength("WAVETREND", "outWT1", outWT1, guardOutLen);
      requireLength("WAVETREND", "outWT2", outWT2, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = wavetrendImpl(startIdx, endIdx, inHigh, inLow, inClose, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("WAVETREND", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live WAVETREND stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#wavetrend} over the same series.
    * Open with {@link Core#wavetrendOpen}; there is no close — the handle is
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
   public static final class WavetrendStream {
      private Core core;
      private int optInChannelPeriod;
      private int optInAveragePeriod;
      private int optInSignalPeriod;
      private double k1;
      private double beta1;
      private double k2;
      private double beta2;
      private double esa;
      private double d;
      private double wt1;
      private double prevAp;
      private double prevEsa;
      private double sumSignal;
      private int wtBuffer_Idx;
      private int maxIdx_wtBuffer;
      private int cbSize_wtBuffer;
      private double[] cb_wtBuffer;
      private double cur_outWT1;
      private double cur_outWT2;
      private int outRangeBegIdx;
      private int outRangeCount;

      private WavetrendStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#wavetrend} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(WavetrendOut)} keeps answering the previous
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
            throw failure("WAVETREND advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private WavetrendStream( WavetrendStream other ) {
         this.core = other.core;
         this.optInChannelPeriod = other.optInChannelPeriod;
         this.optInAveragePeriod = other.optInAveragePeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.k1 = other.k1;
         this.beta1 = other.beta1;
         this.k2 = other.k2;
         this.beta2 = other.beta2;
         this.esa = other.esa;
         this.d = other.d;
         this.wt1 = other.wt1;
         this.prevAp = other.prevAp;
         this.prevEsa = other.prevEsa;
         this.sumSignal = other.sumSignal;
         this.wtBuffer_Idx = other.wtBuffer_Idx;
         this.maxIdx_wtBuffer = other.maxIdx_wtBuffer;
         this.cbSize_wtBuffer = other.cbSize_wtBuffer;
         this.cb_wtBuffer = other.cb_wtBuffer.clone();
         this.cur_outWT1 = other.cur_outWT1;
         this.cur_outWT2 = other.cur_outWT2;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(WavetrendOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, WavetrendOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("WAVETREND update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("WAVETREND update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("WAVETREND update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.wavetrendStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         out.wT1 = this.cur_outWT1;
         out.wT2 = this.cur_outWT2;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, WavetrendOut out ) {
         requireArgument("WAVETREND peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("WAVETREND peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         WavetrendStream sp = this;
         double ap = 0.0;
         double dev = 0.0;
         double ci = 0.0;
         double num = 0.0;
         double scaledDev = 0.0;
         double cur_outWT1 = 0.0;
         double cur_outWT2 = 0.0;
         double d = sp.d;
         double esa = sp.esa;
         double sumSignal = sp.sumSignal;
         double wt1 = sp.wt1;
         ap = (inHigh + inLow + inClose) / 3.0;
         esa = Math.fma(sp.beta1, esa, sp.k1 * ap);
         num = ap - esa;
         if( ap == sp.prevAp && esa == sp.prevEsa ) {
            num = 0.0;
         }
         dev = num;
         if( dev < 0.0 ) {
            dev = -dev;
         }
         d = Math.fma(sp.beta1, d, sp.k1 * dev);
         scaledDev = 0.015 * d;
         if( scaledDev > 0.0 ) {
            ci = num / scaledDev;
         } else {
            ci = 0.0;
         }
         wt1 = Math.fma(sp.beta2, wt1, sp.k2 * ci);
         sumSignal = sumSignal + wt1;
         cur_outWT1 = wt1;
         cur_outWT2 = sumSignal / (double)sp.optInSignalPeriod;
         out.wT1 = cur_outWT1;
         out.wT2 = cur_outWT2;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( WavetrendOut out ) {
         requireArgument("WAVETREND value", "out", out);
         out.wT1 = this.cur_outWT1;
         out.wT2 = this.cur_outWT2;
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
      public WavetrendStream clone() {
         return new WavetrendStream(this);
      }
   }

   /**
    * The outputs of one WAVETREND bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class WavetrendOut {
      /** WaveTrend oscillator line. */
      public double wT1;
      /** Simple average of the oscillator line. */
      public double wT2;
   }
   private void wavetrendStepImpl( WavetrendStream sp, double inHigh, double inLow, double inClose )
   {
      double ap = 0.0;
      double dev = 0.0;
      double ci = 0.0;
      double num = 0.0;
      double scaledDev = 0.0;
      ap = (inHigh + inLow + inClose) / 3.0;
      sp.esa = Math.fma(sp.beta1, sp.esa, sp.k1 * ap);
      num = ap - sp.esa;
      if( ap == sp.prevAp && sp.esa == sp.prevEsa ) {
         num = 0.0;
      }
      dev = num;
      if( dev < 0.0 ) {
         dev = -dev;
      }
      sp.d = Math.fma(sp.beta1, sp.d, sp.k1 * dev);
      scaledDev = 0.015 * sp.d;
      if( scaledDev > 0.0 ) {
         ci = num / scaledDev;
      } else {
         ci = 0.0;
      }
      sp.wt1 = Math.fma(sp.beta2, sp.wt1, sp.k2 * ci);
      sp.cb_wtBuffer[sp.wtBuffer_Idx] = sp.wt1;
      sp.sumSignal = sp.sumSignal + sp.wt1;
      sp.cur_outWT1 = sp.wt1;
      sp.cur_outWT2 = sp.sumSignal / (double)sp.optInSignalPeriod;
      sp.wtBuffer_Idx = sp.wtBuffer_Idx + 1;
      if( sp.wtBuffer_Idx > sp.maxIdx_wtBuffer ) {
         sp.wtBuffer_Idx = 0;
      }
      sp.sumSignal = sp.sumSignal - sp.cb_wtBuffer[sp.wtBuffer_Idx];
      sp.prevAp = ap;
      sp.prevEsa = sp.esa;
   }
   private RetCode wavetrendOpenImpl( WavetrendStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outWT1[], double outWT2[], int outStride )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double ap = 0;
      double esa = 0;
      double dev = 0;
      double d = 0;
      double ci = 0;
      double wt1 = 0;
      double prevAp = 0;
      double prevEsa = 0;
      double num = 0;
      double scaledDev = 0;
      double sumEsa = 0;
      double sumD = 0;
      double sumCi = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackChannel = 0;
      int lookbackAverage = 0;
      int today = 0;
      int outIdx = 0;
      int nAp = 0;
      int nDev = 0;
      int nCi = 0;
      int nSig = 0;
      double[] wtBuffer;
      int wtBuffer_Idx = 0;
      int maxIdx_wtBuffer = (32)-1;
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
      if( optInChannelPeriod == Integer.MIN_VALUE ) {
         optInChannelPeriod = 10;
      } else if( optInChannelPeriod < 2 || optInChannelPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAveragePeriod == Integer.MIN_VALUE ) {
         optInAveragePeriod = 21;
      } else if( optInAveragePeriod < 1 || optInAveragePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 4;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* LazyBear's WaveTrend Oscillator (TradingView, 2014), lines 14 to 21: the
       * typical price's distance from its own exponential average, normalised by
       * an exponential average of that distance's absolute value and by Lambert's
       * CCI constant, then smoothed.
       *
       *    ap  = (H + L + C)/3
       *    esa = EMA(ap, n1)
       *    d   = EMA(|ap - esa|, n1)
       *    ci  = (ap - esa) / (0.015 * d)
       *    WT1 = EMA(ci, n2)        WT2 = SMA(WT1, n3)
       *
       * The middle stage is an exponential CCI, not TA_CCI: cci.c averages with
       * an SMA and takes the mean deviation around that window's own SMA, where
       * this uses two exponential averages.
       *
       * Each stage seeds the way ema.c and sma.c seed, and each stage boundary
       * below is the callee's LOOKBACK rather than (period-1), so the result is
       * bit-identical to the composed chain on moving data and a warm unstable
       * period folds in. The two guards are what the chain cannot express; they
       * are the reason this ships as a function.
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      lookbackTotal = wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
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
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      wtBuffer = new double[optInSignalPeriod];
      maxIdx_wtBuffer = (optInSignalPeriod)-1;
      wtBuffer_Idx = 0;
      lookbackChannel = emaLookback(optInChannelPeriod);
      lookbackAverage = emaLookback(optInAveragePeriod);
      beta1 = (double)(optInChannelPeriod - 1) / (double)(optInChannelPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInAveragePeriod - 1) / (double)(optInAveragePeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      esa = 0.0;
      d = 0.0;
      ci = 0.0;
      wt1 = 0.0;
      sumEsa = 0.0;
      sumD = 0.0;
      sumCi = 0.0;
      sumSignal = 0.0;
      nAp = 0;
      /* The fixpoint test compares against the previous bar's pair. Starting both
       * at 0.0 cannot make it fire spuriously: it would need a bar whose typical
       * price and whose exponential average are both exactly zero, and a zero
       * typical price makes the numerator zero anyway, which is what the test
       * would have substituted.
       */
      prevAp = 0.0;
      prevEsa = 0.0;
      today = startIdx - lookbackTotal;
      /* Warm-up. Runs through startIdx inclusive: the last pass is the one that
       * completes the signal window, so it leaves the first output in the state.
       */
      while( today <= startIdx ) {
         ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         /* Stage 1: the exponential average of the typical price. */
         if( nAp < optInChannelPeriod ) {
            sumEsa = sumEsa + ap;
            if( nAp == optInChannelPeriod - 1 ) {
               esa = sumEsa / optInChannelPeriod;
            }
         } else {
            esa = Math.fma(beta1, esa, k1 * ap);
         }
         /* Stage 2: the exponential average of the absolute distance, over what
          * stage 1 publishes. The counter is compared before it is subtracted,
          * never after: the Rust backend renders these as usize.
          */
         if( nAp >= lookbackChannel ) {
            nDev = nAp - lookbackChannel;
            /* GUARD 1, the fixpoint test. With a constant input the exponential
             * step stops moving once k*|ap - esa| falls under half an ulp, and
             * esa then FREEZES up to (n1+1)/4 ulps away from the price. That
             * frozen residue is a real non-zero distance, so the naive form
             * divides it by its own exponential average and walks to
             * +/-1/0.015 = +/-66.67 -- an extreme reading produced by nothing
             * but rounding. When the pair has not moved, the average has reached
             * its fixpoint and the distance is exactly that residue, so the
             * numerator is taken as zero. The test is exact, so it is
             * independent of scale and period, unlike a fixed epsilon band.
             */
            num = ap - esa;
            if( ap == prevAp && esa == prevEsa ) {
               num = 0.0;
            }
            dev = num;
            if( dev < 0.0 ) {
               dev = -dev;
            }
            if( nDev < optInChannelPeriod ) {
               sumD = sumD + dev;
               if( nDev == optInChannelPeriod - 1 ) {
                  d = sumD / optInChannelPeriod;
               }
            } else {
               d = Math.fma(beta1, d, k1 * dev);
            }
            /* Stage 3: the oscillator, then its own smoothing. */
            if( nDev >= lookbackChannel ) {
               nCi = nDev - lookbackChannel;
               /* GUARD 2, the exact divisor. Test the PRODUCT the division uses,
                * not the deviation: 0.015*d underflows to zero while d is still
                * non-zero (#395). A zero divisor is 0/0 -- no distance against no
                * average distance -- so the oscillator reads its neutral 0.0
                * (#112), as tsi.c does.
                */
               scaledDev = 0.015 * d;
               if( scaledDev > 0.0 ) {
                  ci = num / scaledDev;
               } else {
                  ci = 0.0;
               }
               if( nCi < optInAveragePeriod ) {
                  sumCi = sumCi + ci;
                  if( nCi == optInAveragePeriod - 1 ) {
                     wt1 = sumCi / optInAveragePeriod;
                  }
               } else {
                  wt1 = Math.fma(beta2, wt1, k2 * ci);
               }
               if( nCi >= lookbackAverage ) {
                  wtBuffer[wtBuffer_Idx] = wt1;
                  sumSignal = sumSignal + wt1;
                  wtBuffer_Idx++;
                  if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
               }
            }
         }
         prevAp = ap;
         prevEsa = esa;
         nAp = nAp + 1;
         today = today + 1;
      }
      /* The first output. The warm-up's last pass stored this bar's WT1 into the
       * ring and added it to the running sum, so the average is ready here.
       */
      outWT1[0 * outStride] = wt1;
      outWT2[0 * outStride] = sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
      /* Stable zone. One bar past the first output every stage is past its seed,
       * at every reachable parameter triple: the shortest case, 2/1/1, arrives
       * with the deviation average and the oscillator smoothing both one bar
       * into their recursions. So nothing here branches on a counter, and the
       * stores always run -- which is what keeps the managed backends' peek
       * frames from carrying a seeded output local.
       */
      while( today <= endIdx ) {
         ap = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         esa = Math.fma(beta1, esa, k1 * ap);
         num = ap - esa;
         if( ap == prevAp && esa == prevEsa ) {
            num = 0.0;
         }
         dev = num;
         if( dev < 0.0 ) {
            dev = -dev;
         }
         d = Math.fma(beta1, d, k1 * dev);
         scaledDev = 0.015 * d;
         if( scaledDev > 0.0 ) {
            ci = num / scaledDev;
         } else {
            ci = 0.0;
         }
         wt1 = Math.fma(beta2, wt1, k2 * ci);
         wtBuffer[wtBuffer_Idx] = wt1;
         sumSignal = sumSignal + wt1;
         outWT1[outIdx * outStride] = wt1;
         outWT2[outIdx * outStride] = sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         wtBuffer_Idx++;
         if( wtBuffer_Idx > maxIdx_wtBuffer ) { wtBuffer_Idx = 0; }
         sumSignal = sumSignal - wtBuffer[wtBuffer_Idx];
         prevAp = ap;
         prevEsa = esa;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_wtBuffer = maxIdx_wtBuffer + 1;
      if( capCb_wtBuffer > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInChannelPeriod = optInChannelPeriod;
      sp.optInAveragePeriod = optInAveragePeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.k1 = k1;
      sp.beta1 = beta1;
      sp.k2 = k2;
      sp.beta2 = beta2;
      sp.esa = esa;
      sp.d = d;
      sp.wt1 = wt1;
      sp.prevAp = prevAp;
      sp.prevEsa = prevEsa;
      sp.sumSignal = sumSignal;
      sp.wtBuffer_Idx = wtBuffer_Idx;
      sp.maxIdx_wtBuffer = maxIdx_wtBuffer;
      sp.cbSize_wtBuffer = capCb_wtBuffer;
      sp.cb_wtBuffer = wtBuffer;
      sp.cur_outWT1 = outWT1[(outNBElement.value - 1) * outStride];
      sp.cur_outWT2 = outWT2[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* wavetrendOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WavetrendStream wavetrendOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outWT1[], double outWT2[] )
   {
      WavetrendStream sp = new WavetrendStream(this);
      RetCode retCode = wavetrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("WAVETREND openAndFill", inHigh.length, startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod));
      }
      throw streamFailure("WAVETREND openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind wavetrendOpen (composition seam). */
   WavetrendStream wavetrendOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod )
   {
      WavetrendStream sp = new WavetrendStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outWT1 = new double[1];
      double[] sink_outWT2 = new double[1];
      RetCode retCode = wavetrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outWT1, sink_outWT2, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("WAVETREND open", inHigh.length, startIdx, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod));
      }
      throw streamFailure("WAVETREND open", retCode);
   }
   /**
    * Open a live WAVETREND stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#wavetrend} at that bar.
    * <p>The history must hold at least {@code wavetrendLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public WavetrendStream wavetrendOpen( double inHigh[], double inLow[], double inClose[], int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod )
   {
      requireArgument("WAVETREND open", "inHigh", inHigh);
      requireHistory("WAVETREND open", inHigh.length);
      requireArgument("WAVETREND open", "inLow", inLow);
      requireArgument("WAVETREND open", "inClose", inClose);
      requireHistoryLength("WAVETREND open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WAVETREND open", "inClose", inClose.length, inHigh.length);
      return wavetrendOpenInternal(inHigh, inLow, inClose, 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#wavetrendOpen} that also fills the output array(s) bit-identically
    * to {@link Core#wavetrend} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link WavetrendStream#outRange()}.
    */
   public WavetrendStream wavetrendOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInChannelPeriod, int optInAveragePeriod, int optInSignalPeriod, double outWT1[], double outWT2[] )
   {
      requireArgument("WAVETREND openAndFill", "inHigh", inHigh);
      requireHistory("WAVETREND openAndFill", inHigh.length);
      requireArgument("WAVETREND openAndFill", "inLow", inLow);
      requireArgument("WAVETREND openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("WAVETREND openAndFill", inHigh.length, wavetrendLookback(optInChannelPeriod, optInAveragePeriod, optInSignalPeriod));
      requireHistoryLength("WAVETREND openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WAVETREND openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("WAVETREND openAndFill", "outWT1", outWT1, guardOutLen);
      requireLength("WAVETREND openAndFill", "outWT2", outWT2, guardOutLen);
      if( (Object)outWT1 == (Object)inHigh || (Object)outWT1 == (Object)inLow || (Object)outWT1 == (Object)inClose || (Object)outWT2 == (Object)inHigh || (Object)outWT2 == (Object)inLow || (Object)outWT2 == (Object)inClose || (Object)outWT1 == (Object)outWT2 ) {
         throw streamFailure("WAVETREND openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return wavetrendOpenAndFillInternal(inHigh, inLow, inClose, 0, optInChannelPeriod, optInAveragePeriod, optInSignalPeriod, outBegIdx, outNBElement, outWT1, outWT2);
   }
