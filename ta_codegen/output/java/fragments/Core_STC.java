/* Schaff Trend Cycle, created by Doug Schaff (FX-Strategy.com), who released
 * its code in "Releasing the Code to the Schaff Trend Cycle", FXStreet.com,
 * February 15, 2008,
 * https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf
 *
 * List of contributors:
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
 *  092926 MF,CC  Initial version (#478).
 *  100726 MF,CC  #492. The Auto rule keeps its total as the EMA count shortens.
 */

   /**
    * Number of leading input bars {@link Core#stc} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInFastPeriod Period of the fast EMA (default 23; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 50; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCyclePeriod Window of both stochastic stages (default 10;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int stcLookback( int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 23;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 50;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInCyclePeriod == Integer.MIN_VALUE ) {
         optInCyclePeriod = 10;
      } else if( optInCyclePeriod < 2 || optInCyclePeriod > 100000 ) {
         return -1;
      }
      int tempInteger;
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* The MACD line's own lookback, which is what inherits TA_FUNC_UNST_EMA,
       * then one window per stochastic stage. The two 0.5 smoothers seed on
       * their first input, so they add only the unstable period.
       */
      return emaLookback(optInSlowPeriod) + 2 * (optInCyclePeriod - 1) + this.unstableCount(FuncUnstId.STC.ordinal(), (5 * 10 + 1) / 2 + 3 * (optInSlowPeriod + 1), (5 * 19 + 1) / 2 + 3 * (optInSlowPeriod + 1)) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#stc}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInFastPeriod Period of the fast EMA (default 23; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 50; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCyclePeriod Window of both stochastic stages (default 10;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int stcDisplayShift( int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod, int outputIdx )
   {
      if( stcLookback( optInFastPeriod, optInSlowPeriod, optInCyclePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode stcImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInCyclePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double[] lineRing;
      int lineRing_Idx = 0;
      int maxIdx_lineRing = (30)-1;
      double[] lineSufHi;
      int lineSufHi_Idx = 0;
      int maxIdx_lineSufHi = (30)-1;
      double[] lineSufLo;
      int lineSufLo_Idx = 0;
      int maxIdx_lineSufLo = (30)-1;
      double[] pfRing;
      int pfRing_Idx = 0;
      int maxIdx_pfRing = (30)-1;
      double[] pfSufHi;
      int pfSufHi_Idx = 0;
      int maxIdx_pfSufHi = (30)-1;
      double[] pfSufLo;
      int pfSufLo_Idx = 0;
      int maxIdx_pfSufLo = (30)-1;
      double prevFast = 0;
      double prevSlow = 0;
      double fastK = 0;
      double slowK = 0;
      double tempReal = 0;
      double lineValue = 0;
      double fastBeta = 0;
      double slowBeta = 0;
      double lowest = 0;
      double highest = 0;
      double range = 0;
      double frac1 = 0;
      double frac2 = 0;
      double pf = 0;
      double pff = 0;
      double lineHi = 0;
      double lineLo = 0;
      double pfHi = 0;
      double pfLo = 0;
      double sufHi = 0;
      double sufLo = 0;
      int i = 0;
      int today = 0;
      int fastToday = 0;
      int lineStart = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lastIdx = 0;
      int nLine = 0;
      int nPF = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 23;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 50;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCyclePeriod == Integer.MIN_VALUE ) {
         optInCyclePeriod = 10;
      } else if( optInCyclePeriod < 2 || optInCyclePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      lookbackTotal = stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      /* Rolling extrema, van Herk / Gil-Werman: the window ending in slot j is
       * the current block's prefix extremum joined with the previous block's
       * suffix extremum from slot j+1. The extrema are exact, so the output must
       * stay bit-identical to a full rescan of each window.
       */
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineRing = new double[optInCyclePeriod];
      maxIdx_lineRing = (optInCyclePeriod)-1;
      lineRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufHi = new double[optInCyclePeriod];
      maxIdx_lineSufHi = (optInCyclePeriod)-1;
      lineSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufLo = new double[optInCyclePeriod];
      maxIdx_lineSufLo = (optInCyclePeriod)-1;
      lineSufLo_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfRing = new double[optInCyclePeriod];
      maxIdx_pfRing = (optInCyclePeriod)-1;
      pfRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufHi = new double[optInCyclePeriod];
      maxIdx_pfSufHi = (optInCyclePeriod)-1;
      pfSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufLo = new double[optInCyclePeriod];
      maxIdx_pfSufLo = (optInCyclePeriod)-1;
      pfSufLo_Idx = 0;
      lastIdx = optInCyclePeriod - 1;
      /* The line is TA_MACD's: each SMA seed placed by its own EMA lookback, then
       * both EMAs advanced to lineStart, so that from lineStart on it is
       * TA_EMA(fast) - TA_EMA(slow) bit for bit. That placement reads out of
       * bounds or skips bars unless ema_lookback(n) - n never decreases as n
       * grows. The chain is fed from
       * lineStart, not from the EMA seed: TA_FUNC_UNST_EMA then reaches only the
       * line, while TA_FUNC_UNST_STC moves the whole chain back and so warms the
       * line and both smoothers.
       */
      lookbackSlow = emaLookback(optInSlowPeriod);
      lineStart = startIdx - (lookbackTotal - lookbackSlow);
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookback(optInFastPeriod));
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[fastToday++];
      }
      prevFast = prevFast / optInFastPeriod;
      while( today < fastToday ) {
         tempReal = inReal[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      while( today <= lineStart ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      /* A zero range holds the previous fraction (0.0 before any), and the test
       * is exact: in a sustained trend PF saturates at 100 and the second
       * range reaches exactly 0 while the output must stay at 100.
       */
      frac1 = 0.0;
      frac2 = 0.0;
      pf = 0.0;
      pff = 0.0;
      pfHi = 0.0;
      pfLo = 0.0;
      nPF = 0;
      lineValue = prevFast - prevSlow;
      lineRing[lineRing_Idx] = lineValue;
      lineRing_Idx++;
      if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
      lineHi = lineValue;
      lineLo = lineValue;
      nLine = 1;
      /* Warm-up, through startIdx inclusive. Each stage starts once its
       * window is full, and each smoother is seeded on its first input.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         nLine = nLine + 1;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( nLine >= optInCyclePeriod && lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         if( nLine >= optInCyclePeriod ) {
            range = highest - lowest;
            if( range > 0.0 ) {
               frac1 = (lineValue - lowest) / range * 100.0;
            }
            if( nPF == 0 ) {
               pf = frac1;
            } else {
               pf = Math.fma(0.5, frac1 - pf, pf);
            }
            nPF = nPF + 1;
            pfRing[pfRing_Idx] = pf;
            if( pfRing_Idx == 0 ) {
               pfHi = pf;
               pfLo = pf;
            } else {
               if( pf > pfHi ) {
                  pfHi = pf;
               }
               if( pf < pfLo ) {
                  pfLo = pf;
               }
            }
            highest = pfHi;
            lowest = pfLo;
            if( nPF >= optInCyclePeriod && pfRing_Idx < lastIdx ) {
               tempReal = pfSufHi[pfRing_Idx + 1];
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               tempReal = pfSufLo[pfRing_Idx + 1];
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            pfRing_Idx++;
            if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
            if( pfRing_Idx == 0 ) {
               sufHi = pfRing[lastIdx];
               sufLo = sufHi;
               pfSufHi[lastIdx] = sufHi;
               pfSufLo[lastIdx] = sufLo;
               i = lastIdx;
               while( i > 0 ) {
                  i -= 1;
                  tempReal = pfRing[i];
                  if( tempReal > sufHi ) {
                     sufHi = tempReal;
                  }
                  if( tempReal < sufLo ) {
                     sufLo = tempReal;
                  }
                  pfSufHi[i] = sufHi;
                  pfSufLo[i] = sufLo;
               }
            }
            if( nPF >= optInCyclePeriod ) {
               range = highest - lowest;
               if( range > 0.0 ) {
                  frac2 = (pf - lowest) / range * 100.0;
               }
               if( nPF == optInCyclePeriod ) {
                  pff = frac2;
               } else {
                  pff = Math.fma(0.5, frac2 - pff, pff);
               }
            }
         }
         today = today + 1;
      }
      outReal[0] = pff;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac1 = (lineValue - lowest) / range * 100.0;
         }
         pf = Math.fma(0.5, frac1 - pf, pf);
         pfRing[pfRing_Idx] = pf;
         if( pfRing_Idx == 0 ) {
            pfHi = pf;
            pfLo = pf;
         } else {
            if( pf > pfHi ) {
               pfHi = pf;
            }
            if( pf < pfLo ) {
               pfLo = pf;
            }
         }
         highest = pfHi;
         lowest = pfLo;
         if( pfRing_Idx < lastIdx ) {
            tempReal = pfSufHi[pfRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = pfSufLo[pfRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         pfRing_Idx++;
         if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
         if( pfRing_Idx == 0 ) {
            sufHi = pfRing[lastIdx];
            sufLo = sufHi;
            pfSufHi[lastIdx] = sufHi;
            pfSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = pfRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               pfSufHi[i] = sufHi;
               pfSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac2 = (pf - lowest) / range * 100.0;
         }
         pff = Math.fma(0.5, frac2 - pff, pff);
         outReal[outIdx++] = pff;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode stcImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInCyclePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double[] lineRing;
      int lineRing_Idx = 0;
      int maxIdx_lineRing = (30)-1;
      double[] lineSufHi;
      int lineSufHi_Idx = 0;
      int maxIdx_lineSufHi = (30)-1;
      double[] lineSufLo;
      int lineSufLo_Idx = 0;
      int maxIdx_lineSufLo = (30)-1;
      double[] pfRing;
      int pfRing_Idx = 0;
      int maxIdx_pfRing = (30)-1;
      double[] pfSufHi;
      int pfSufHi_Idx = 0;
      int maxIdx_pfSufHi = (30)-1;
      double[] pfSufLo;
      int pfSufLo_Idx = 0;
      int maxIdx_pfSufLo = (30)-1;
      double prevFast = 0;
      double prevSlow = 0;
      double fastK = 0;
      double slowK = 0;
      double tempReal = 0;
      double lineValue = 0;
      double fastBeta = 0;
      double slowBeta = 0;
      double lowest = 0;
      double highest = 0;
      double range = 0;
      double frac1 = 0;
      double frac2 = 0;
      double pf = 0;
      double pff = 0;
      double lineHi = 0;
      double lineLo = 0;
      double pfHi = 0;
      double pfLo = 0;
      double sufHi = 0;
      double sufLo = 0;
      int i = 0;
      int today = 0;
      int fastToday = 0;
      int lineStart = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lastIdx = 0;
      int nLine = 0;
      int nPF = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 23;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 50;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCyclePeriod == Integer.MIN_VALUE ) {
         optInCyclePeriod = 10;
      } else if( optInCyclePeriod < 2 || optInCyclePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      lookbackTotal = stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineRing = new double[optInCyclePeriod];
      maxIdx_lineRing = (optInCyclePeriod)-1;
      lineRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufHi = new double[optInCyclePeriod];
      maxIdx_lineSufHi = (optInCyclePeriod)-1;
      lineSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufLo = new double[optInCyclePeriod];
      maxIdx_lineSufLo = (optInCyclePeriod)-1;
      lineSufLo_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfRing = new double[optInCyclePeriod];
      maxIdx_pfRing = (optInCyclePeriod)-1;
      pfRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufHi = new double[optInCyclePeriod];
      maxIdx_pfSufHi = (optInCyclePeriod)-1;
      pfSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufLo = new double[optInCyclePeriod];
      maxIdx_pfSufLo = (optInCyclePeriod)-1;
      pfSufLo_Idx = 0;
      lastIdx = optInCyclePeriod - 1;
      lookbackSlow = emaLookback(optInSlowPeriod);
      lineStart = startIdx - (lookbackTotal - lookbackSlow);
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookback(optInFastPeriod));
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += (double)inReal[fastToday++];
      }
      prevFast = prevFast / optInFastPeriod;
      while( today < fastToday ) {
         tempReal = (double)inReal[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      while( today <= lineStart ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      frac1 = 0.0;
      frac2 = 0.0;
      pf = 0.0;
      pff = 0.0;
      pfHi = 0.0;
      pfLo = 0.0;
      nPF = 0;
      lineValue = prevFast - prevSlow;
      lineRing[lineRing_Idx] = lineValue;
      lineRing_Idx++;
      if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
      lineHi = lineValue;
      lineLo = lineValue;
      nLine = 1;
      while( today <= startIdx ) {
         tempReal = (double)inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         nLine = nLine + 1;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( nLine >= optInCyclePeriod && lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         if( nLine >= optInCyclePeriod ) {
            range = highest - lowest;
            if( range > 0.0 ) {
               frac1 = (lineValue - lowest) / range * 100.0;
            }
            if( nPF == 0 ) {
               pf = frac1;
            } else {
               pf = Math.fma(0.5, frac1 - pf, pf);
            }
            nPF = nPF + 1;
            pfRing[pfRing_Idx] = pf;
            if( pfRing_Idx == 0 ) {
               pfHi = pf;
               pfLo = pf;
            } else {
               if( pf > pfHi ) {
                  pfHi = pf;
               }
               if( pf < pfLo ) {
                  pfLo = pf;
               }
            }
            highest = pfHi;
            lowest = pfLo;
            if( nPF >= optInCyclePeriod && pfRing_Idx < lastIdx ) {
               tempReal = pfSufHi[pfRing_Idx + 1];
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               tempReal = pfSufLo[pfRing_Idx + 1];
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            pfRing_Idx++;
            if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
            if( pfRing_Idx == 0 ) {
               sufHi = pfRing[lastIdx];
               sufLo = sufHi;
               pfSufHi[lastIdx] = sufHi;
               pfSufLo[lastIdx] = sufLo;
               i = lastIdx;
               while( i > 0 ) {
                  i -= 1;
                  tempReal = pfRing[i];
                  if( tempReal > sufHi ) {
                     sufHi = tempReal;
                  }
                  if( tempReal < sufLo ) {
                     sufLo = tempReal;
                  }
                  pfSufHi[i] = sufHi;
                  pfSufLo[i] = sufLo;
               }
            }
            if( nPF >= optInCyclePeriod ) {
               range = highest - lowest;
               if( range > 0.0 ) {
                  frac2 = (pf - lowest) / range * 100.0;
               }
               if( nPF == optInCyclePeriod ) {
                  pff = frac2;
               } else {
                  pff = Math.fma(0.5, frac2 - pff, pff);
               }
            }
         }
         today = today + 1;
      }
      outReal[0] = pff;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac1 = (lineValue - lowest) / range * 100.0;
         }
         pf = Math.fma(0.5, frac1 - pf, pf);
         pfRing[pfRing_Idx] = pf;
         if( pfRing_Idx == 0 ) {
            pfHi = pf;
            pfLo = pf;
         } else {
            if( pf > pfHi ) {
               pfHi = pf;
            }
            if( pf < pfLo ) {
               pfLo = pf;
            }
         }
         highest = pfHi;
         lowest = pfLo;
         if( pfRing_Idx < lastIdx ) {
            tempReal = pfSufHi[pfRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = pfSufLo[pfRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         pfRing_Idx++;
         if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
         if( pfRing_Idx == 0 ) {
            sufHi = pfRing[lastIdx];
            sufLo = sufHi;
            pfSufHi[lastIdx] = sufHi;
            pfSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = pfRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               pfSufHi[i] = sufHi;
               pfSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac2 = (pf - lowest) / range * 100.0;
         }
         pff = Math.fma(0.5, frac2 - pff, pff);
         outReal[outIdx++] = pff;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a
    * stochastic, each pass smoothed by half. Bounded 0 to 100, read against 25
    * and 75: turning up from below 25 is bullish, turning down from above 75
    * bearish.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/stc">ta-lib.org/functions/stc</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The smoothing factor is fixed at 0.5, as in Schaff's published code.</li>
    * <li>The MACD line is TA-Lib's: both EMAs are seeded with a simple average over windows ending on the same bar. The published code runs the EMAs from the first bar of the data.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped, as in {@code MACD}.</li>
    * <li>Being recursive, an output depends on how much history precedes it. The unstable period warms the two smoothers; the EMA unstable period warms the MACD line.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#stcLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series (typically close)
    * @param optInFastPeriod Period of the fast EMA (default 23; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 50; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCyclePeriod Window of both stochastic stages (default 10;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Schaff Trend Cycle, from 0 to 100. Must hold at least
    *        {@code endIdx - max(startIdx, stcLookback(...)) + 1} values, and never be
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
    * @see Core#macd
    * @see Core#stochf
    * @see Core#stochrsi
    */
   public OutRange stc( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInCyclePeriod,
                        double outReal[] )
   {
      requireIndexRange("STC", startIdx, endIdx);
      int guardStart = clampedStart("STC", startIdx, stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STC", "inReal", inReal, guardInLen);
      requireLength("STC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = stcImpl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("STC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a
    * stochastic, each pass smoothed by half. Bounded 0 to 100, read against 25
    * and 75: turning up from below 25 is bullish, turning down from above 75
    * bearish.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/stc">ta-lib.org/functions/stc</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The smoothing factor is fixed at 0.5, as in Schaff's published code.</li>
    * <li>The MACD line is TA-Lib's: both EMAs are seeded with a simple average over windows ending on the same bar. The published code runs the EMAs from the first bar of the data.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped, as in {@code MACD}.</li>
    * <li>Being recursive, an output depends on how much history precedes it. The unstable period warms the two smoothers; the EMA unstable period warms the MACD line.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#stcLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series (typically close)
    * @param optInFastPeriod Period of the fast EMA (default 23; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 50; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInCyclePeriod Window of both stochastic stages (default 10;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Schaff Trend Cycle, from 0 to 100. Must hold at least
    *        {@code endIdx - max(startIdx, stcLookback(...)) + 1} values, and never be
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
    * @see Core#macd
    * @see Core#stochf
    * @see Core#stochrsi
    */
   public OutRange stc( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInCyclePeriod,
                        double outReal[] )
   {
      requireIndexRange("STC", startIdx, endIdx);
      int guardStart = clampedStart("STC", startIdx, stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STC", "inReal", inReal, guardInLen);
      requireLength("STC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = stcImpl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("STC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#stc} over the same series.
    * Open with {@link Core#stcOpen}; there is no close — the handle is
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
   public static final class StcStream {
      private Core core;
      private int optInFastPeriod;
      private int optInSlowPeriod;
      private int optInCyclePeriod;
      private double prevFast;
      private double prevSlow;
      private double fastK;
      private double slowK;
      private double fastBeta;
      private double slowBeta;
      private double frac1;
      private double frac2;
      private double pf;
      private double pff;
      private double lineHi;
      private double lineLo;
      private double pfHi;
      private double pfLo;
      private int lastIdx;
      private int lineRing_Idx;
      private int pfRing_Idx;
      private int maxIdx_lineRing;
      private int lineSufHi_Idx;
      private int maxIdx_lineSufHi;
      private int lineSufLo_Idx;
      private int maxIdx_lineSufLo;
      private int maxIdx_pfRing;
      private int pfSufHi_Idx;
      private int maxIdx_pfSufHi;
      private int pfSufLo_Idx;
      private int maxIdx_pfSufLo;
      private int cbSize_lineRing;
      private double[] cb_lineRing;
      private int cbSize_lineSufHi;
      private double[] cb_lineSufHi;
      private int cbSize_lineSufLo;
      private double[] cb_lineSufLo;
      private int cbSize_pfRing;
      private double[] cb_pfRing;
      private int cbSize_pfSufHi;
      private double[] cb_pfSufHi;
      private int cbSize_pfSufLo;
      private double[] cb_pfSufLo;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private StcStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#stc} reports over the same bars: the
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
            throw failure("STC advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private StcStream( StcStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInCyclePeriod = other.optInCyclePeriod;
         this.prevFast = other.prevFast;
         this.prevSlow = other.prevSlow;
         this.fastK = other.fastK;
         this.slowK = other.slowK;
         this.fastBeta = other.fastBeta;
         this.slowBeta = other.slowBeta;
         this.frac1 = other.frac1;
         this.frac2 = other.frac2;
         this.pf = other.pf;
         this.pff = other.pff;
         this.lineHi = other.lineHi;
         this.lineLo = other.lineLo;
         this.pfHi = other.pfHi;
         this.pfLo = other.pfLo;
         this.lastIdx = other.lastIdx;
         this.lineRing_Idx = other.lineRing_Idx;
         this.pfRing_Idx = other.pfRing_Idx;
         this.maxIdx_lineRing = other.maxIdx_lineRing;
         this.lineSufHi_Idx = other.lineSufHi_Idx;
         this.maxIdx_lineSufHi = other.maxIdx_lineSufHi;
         this.lineSufLo_Idx = other.lineSufLo_Idx;
         this.maxIdx_lineSufLo = other.maxIdx_lineSufLo;
         this.maxIdx_pfRing = other.maxIdx_pfRing;
         this.pfSufHi_Idx = other.pfSufHi_Idx;
         this.maxIdx_pfSufHi = other.maxIdx_pfSufHi;
         this.pfSufLo_Idx = other.pfSufLo_Idx;
         this.maxIdx_pfSufLo = other.maxIdx_pfSufLo;
         this.cbSize_lineRing = other.cbSize_lineRing;
         this.cb_lineRing = other.cb_lineRing.clone();
         this.cbSize_lineSufHi = other.cbSize_lineSufHi;
         this.cb_lineSufHi = other.cb_lineSufHi.clone();
         this.cbSize_lineSufLo = other.cbSize_lineSufLo;
         this.cb_lineSufLo = other.cb_lineSufLo.clone();
         this.cbSize_pfRing = other.cbSize_pfRing;
         this.cb_pfRing = other.cb_pfRing.clone();
         this.cbSize_pfSufHi = other.cbSize_pfSufHi;
         this.cb_pfSufHi = other.cb_pfSufHi.clone();
         this.cbSize_pfSufLo = other.cbSize_pfSufLo;
         this.cb_pfSufLo = other.cb_pfSufLo.clone();
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
            throw failure("STC update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("STC update", "inReal");
         core.stcStepImpl(this, inReal);
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
            throw nonFiniteBar("STC peek", "inReal");
         StcStream sp = this;
         double tempReal = 0.0;
         double lineValue = 0.0;
         double lowest = 0.0;
         double highest = 0.0;
         double range = 0.0;
         double sufHi = 0.0;
         double sufLo = 0.0;
         int i = 0;
         double cur_outReal = 0.0;
         double frac1 = sp.frac1;
         double frac2 = sp.frac2;
         double lineHi = sp.lineHi;
         double lineLo = sp.lineLo;
         int lineRing_Idx = sp.lineRing_Idx;
         double pf = sp.pf;
         double pfHi = sp.pfHi;
         double pfLo = sp.pfLo;
         int pfRing_Idx = sp.pfRing_Idx;
         double pff = sp.pff;
         double prevFast = sp.prevFast;
         double prevSlow = sp.prevSlow;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         tempReal = inReal;
         prevFast = Math.fma(sp.fastBeta, prevFast, sp.fastK * tempReal);
         prevSlow = Math.fma(sp.slowBeta, prevSlow, sp.slowK * tempReal);
         lineValue = prevFast - prevSlow;
         pkSlot0 = lineRing_Idx;
         pkVal0 = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( lineRing_Idx < sp.lastIdx ) {
            tempReal = sp.cb_lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = sp.cb_lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx = lineRing_Idx + 1;
         if( lineRing_Idx > sp.maxIdx_lineRing ) {
            lineRing_Idx = 0;
         }
         if( lineRing_Idx == 0 ) {
            sufHi = (sp.lastIdx != pkSlot0) ? sp.cb_lineRing[sp.lastIdx] : pkVal0;
            sufLo = sufHi;
            i = sp.lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = (i != pkSlot0) ? sp.cb_lineRing[i] : pkVal0;
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac1 = (lineValue - lowest) / range * 100.0;
         }
         pf = Math.fma(0.5, frac1 - pf, pf);
         pkSlot1 = pfRing_Idx;
         pkVal1 = pf;
         if( pfRing_Idx == 0 ) {
            pfHi = pf;
            pfLo = pf;
         } else {
            if( pf > pfHi ) {
               pfHi = pf;
            }
            if( pf < pfLo ) {
               pfLo = pf;
            }
         }
         highest = pfHi;
         lowest = pfLo;
         if( pfRing_Idx < sp.lastIdx ) {
            tempReal = sp.cb_pfSufHi[pfRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = sp.cb_pfSufLo[pfRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         pfRing_Idx = pfRing_Idx + 1;
         if( pfRing_Idx > sp.maxIdx_pfRing ) {
            pfRing_Idx = 0;
         }
         if( pfRing_Idx == 0 ) {
            sufHi = (sp.lastIdx != pkSlot1) ? sp.cb_pfRing[sp.lastIdx] : pkVal1;
            sufLo = sufHi;
            i = sp.lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = (i != pkSlot1) ? sp.cb_pfRing[i] : pkVal1;
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac2 = (pf - lowest) / range * 100.0;
         }
         pff = Math.fma(0.5, frac2 - pff, pff);
         cur_outReal = pff;
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
      public StcStream clone() {
         return new StcStream(this);
      }
   }
   private void stcStepImpl( StcStream sp, double inReal )
   {
      double tempReal = 0.0;
      double lineValue = 0.0;
      double lowest = 0.0;
      double highest = 0.0;
      double range = 0.0;
      double sufHi = 0.0;
      double sufLo = 0.0;
      int i = 0;
      tempReal = inReal;
      sp.prevFast = Math.fma(sp.fastBeta, sp.prevFast, sp.fastK * tempReal);
      sp.prevSlow = Math.fma(sp.slowBeta, sp.prevSlow, sp.slowK * tempReal);
      lineValue = sp.prevFast - sp.prevSlow;
      sp.cb_lineRing[sp.lineRing_Idx] = lineValue;
      if( sp.lineRing_Idx == 0 ) {
         sp.lineHi = lineValue;
         sp.lineLo = lineValue;
      } else {
         if( lineValue > sp.lineHi ) {
            sp.lineHi = lineValue;
         }
         if( lineValue < sp.lineLo ) {
            sp.lineLo = lineValue;
         }
      }
      highest = sp.lineHi;
      lowest = sp.lineLo;
      if( sp.lineRing_Idx < sp.lastIdx ) {
         tempReal = sp.cb_lineSufHi[sp.lineRing_Idx + 1];
         if( tempReal > highest ) {
            highest = tempReal;
         }
         tempReal = sp.cb_lineSufLo[sp.lineRing_Idx + 1];
         if( tempReal < lowest ) {
            lowest = tempReal;
         }
      }
      sp.lineRing_Idx = sp.lineRing_Idx + 1;
      if( sp.lineRing_Idx > sp.maxIdx_lineRing ) {
         sp.lineRing_Idx = 0;
      }
      if( sp.lineRing_Idx == 0 ) {
         sufHi = sp.cb_lineRing[sp.lastIdx];
         sufLo = sufHi;
         sp.cb_lineSufHi[sp.lastIdx] = sufHi;
         sp.cb_lineSufLo[sp.lastIdx] = sufLo;
         i = sp.lastIdx;
         while( i > 0 ) {
            i -= 1;
            tempReal = sp.cb_lineRing[i];
            if( tempReal > sufHi ) {
               sufHi = tempReal;
            }
            if( tempReal < sufLo ) {
               sufLo = tempReal;
            }
            sp.cb_lineSufHi[i] = sufHi;
            sp.cb_lineSufLo[i] = sufLo;
         }
      }
      range = highest - lowest;
      if( range > 0.0 ) {
         sp.frac1 = (lineValue - lowest) / range * 100.0;
      }
      sp.pf = Math.fma(0.5, sp.frac1 - sp.pf, sp.pf);
      sp.cb_pfRing[sp.pfRing_Idx] = sp.pf;
      if( sp.pfRing_Idx == 0 ) {
         sp.pfHi = sp.pf;
         sp.pfLo = sp.pf;
      } else {
         if( sp.pf > sp.pfHi ) {
            sp.pfHi = sp.pf;
         }
         if( sp.pf < sp.pfLo ) {
            sp.pfLo = sp.pf;
         }
      }
      highest = sp.pfHi;
      lowest = sp.pfLo;
      if( sp.pfRing_Idx < sp.lastIdx ) {
         tempReal = sp.cb_pfSufHi[sp.pfRing_Idx + 1];
         if( tempReal > highest ) {
            highest = tempReal;
         }
         tempReal = sp.cb_pfSufLo[sp.pfRing_Idx + 1];
         if( tempReal < lowest ) {
            lowest = tempReal;
         }
      }
      sp.pfRing_Idx = sp.pfRing_Idx + 1;
      if( sp.pfRing_Idx > sp.maxIdx_pfRing ) {
         sp.pfRing_Idx = 0;
      }
      if( sp.pfRing_Idx == 0 ) {
         sufHi = sp.cb_pfRing[sp.lastIdx];
         sufLo = sufHi;
         sp.cb_pfSufHi[sp.lastIdx] = sufHi;
         sp.cb_pfSufLo[sp.lastIdx] = sufLo;
         i = sp.lastIdx;
         while( i > 0 ) {
            i -= 1;
            tempReal = sp.cb_pfRing[i];
            if( tempReal > sufHi ) {
               sufHi = tempReal;
            }
            if( tempReal < sufLo ) {
               sufLo = tempReal;
            }
            sp.cb_pfSufHi[i] = sufHi;
            sp.cb_pfSufLo[i] = sufLo;
         }
      }
      range = highest - lowest;
      if( range > 0.0 ) {
         sp.frac2 = (sp.pf - lowest) / range * 100.0;
      }
      sp.pff = Math.fma(0.5, sp.frac2 - sp.pff, sp.pff);
      sp.cur_outReal = sp.pff;
   }
   private RetCode stcOpenImpl( StcStream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] lineRing;
      int lineRing_Idx = 0;
      int maxIdx_lineRing = (30)-1;
      double[] lineSufHi;
      int lineSufHi_Idx = 0;
      int maxIdx_lineSufHi = (30)-1;
      double[] lineSufLo;
      int lineSufLo_Idx = 0;
      int maxIdx_lineSufLo = (30)-1;
      double[] pfRing;
      int pfRing_Idx = 0;
      int maxIdx_pfRing = (30)-1;
      double[] pfSufHi;
      int pfSufHi_Idx = 0;
      int maxIdx_pfSufHi = (30)-1;
      double[] pfSufLo;
      int pfSufLo_Idx = 0;
      int maxIdx_pfSufLo = (30)-1;
      double prevFast = 0;
      double prevSlow = 0;
      double fastK = 0;
      double slowK = 0;
      double tempReal = 0;
      double lineValue = 0;
      double fastBeta = 0;
      double slowBeta = 0;
      double lowest = 0;
      double highest = 0;
      double range = 0;
      double frac1 = 0;
      double frac2 = 0;
      double pf = 0;
      double pff = 0;
      double lineHi = 0;
      double lineLo = 0;
      double pfHi = 0;
      double pfLo = 0;
      double sufHi = 0;
      double sufLo = 0;
      int i = 0;
      int today = 0;
      int fastToday = 0;
      int lineStart = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSlow = 0;
      int lastIdx = 0;
      int nLine = 0;
      int nPF = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 23;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 50;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInCyclePeriod == Integer.MIN_VALUE ) {
         optInCyclePeriod = 10;
      } else if( optInCyclePeriod < 2 || optInCyclePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      lookbackTotal = stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      outBegIdx.value = startIdx;
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      /* Rolling extrema, van Herk / Gil-Werman: the window ending in slot j is
       * the current block's prefix extremum joined with the previous block's
       * suffix extremum from slot j+1. The extrema are exact, so the output must
       * stay bit-identical to a full rescan of each window.
       */
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineRing = new double[optInCyclePeriod];
      maxIdx_lineRing = (optInCyclePeriod)-1;
      lineRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufHi = new double[optInCyclePeriod];
      maxIdx_lineSufHi = (optInCyclePeriod)-1;
      lineSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lineSufLo = new double[optInCyclePeriod];
      maxIdx_lineSufLo = (optInCyclePeriod)-1;
      lineSufLo_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfRing = new double[optInCyclePeriod];
      maxIdx_pfRing = (optInCyclePeriod)-1;
      pfRing_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufHi = new double[optInCyclePeriod];
      maxIdx_pfSufHi = (optInCyclePeriod)-1;
      pfSufHi_Idx = 0;
      if( optInCyclePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      pfSufLo = new double[optInCyclePeriod];
      maxIdx_pfSufLo = (optInCyclePeriod)-1;
      pfSufLo_Idx = 0;
      lastIdx = optInCyclePeriod - 1;
      /* The line is TA_MACD's: each SMA seed placed by its own EMA lookback, then
       * both EMAs advanced to lineStart, so that from lineStart on it is
       * TA_EMA(fast) - TA_EMA(slow) bit for bit. That placement reads out of
       * bounds or skips bars unless ema_lookback(n) - n never decreases as n
       * grows. The chain is fed from
       * lineStart, not from the EMA seed: TA_FUNC_UNST_EMA then reaches only the
       * line, while TA_FUNC_UNST_STC moves the whole chain back and so warms the
       * line and both smoothers.
       */
      lookbackSlow = emaLookback(optInSlowPeriod);
      lineStart = startIdx - (lookbackTotal - lookbackSlow);
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      fastToday = startIdx - lookbackTotal + (lookbackSlow - emaLookback(optInFastPeriod));
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[fastToday++];
      }
      prevFast = prevFast / optInFastPeriod;
      while( today < fastToday ) {
         tempReal = inReal[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      while( today <= lineStart ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      /* A zero range holds the previous fraction (0.0 before any), and the test
       * is exact: in a sustained trend PF saturates at 100 and the second
       * range reaches exactly 0 while the output must stay at 100.
       */
      frac1 = 0.0;
      frac2 = 0.0;
      pf = 0.0;
      pff = 0.0;
      pfHi = 0.0;
      pfLo = 0.0;
      nPF = 0;
      lineValue = prevFast - prevSlow;
      lineRing[lineRing_Idx] = lineValue;
      lineRing_Idx++;
      if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
      lineHi = lineValue;
      lineLo = lineValue;
      nLine = 1;
      /* Warm-up, through startIdx inclusive. Each stage starts once its
       * window is full, and each smoother is seeded on its first input.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         nLine = nLine + 1;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( nLine >= optInCyclePeriod && lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         if( nLine >= optInCyclePeriod ) {
            range = highest - lowest;
            if( range > 0.0 ) {
               frac1 = (lineValue - lowest) / range * 100.0;
            }
            if( nPF == 0 ) {
               pf = frac1;
            } else {
               pf = Math.fma(0.5, frac1 - pf, pf);
            }
            nPF = nPF + 1;
            pfRing[pfRing_Idx] = pf;
            if( pfRing_Idx == 0 ) {
               pfHi = pf;
               pfLo = pf;
            } else {
               if( pf > pfHi ) {
                  pfHi = pf;
               }
               if( pf < pfLo ) {
                  pfLo = pf;
               }
            }
            highest = pfHi;
            lowest = pfLo;
            if( nPF >= optInCyclePeriod && pfRing_Idx < lastIdx ) {
               tempReal = pfSufHi[pfRing_Idx + 1];
               if( tempReal > highest ) {
                  highest = tempReal;
               }
               tempReal = pfSufLo[pfRing_Idx + 1];
               if( tempReal < lowest ) {
                  lowest = tempReal;
               }
            }
            pfRing_Idx++;
            if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
            if( pfRing_Idx == 0 ) {
               sufHi = pfRing[lastIdx];
               sufLo = sufHi;
               pfSufHi[lastIdx] = sufHi;
               pfSufLo[lastIdx] = sufLo;
               i = lastIdx;
               while( i > 0 ) {
                  i -= 1;
                  tempReal = pfRing[i];
                  if( tempReal > sufHi ) {
                     sufHi = tempReal;
                  }
                  if( tempReal < sufLo ) {
                     sufLo = tempReal;
                  }
                  pfSufHi[i] = sufHi;
                  pfSufLo[i] = sufLo;
               }
            }
            if( nPF >= optInCyclePeriod ) {
               range = highest - lowest;
               if( range > 0.0 ) {
                  frac2 = (pf - lowest) / range * 100.0;
               }
               if( nPF == optInCyclePeriod ) {
                  pff = frac2;
               } else {
                  pff = Math.fma(0.5, frac2 - pff, pff);
               }
            }
         }
         today = today + 1;
      }
      outReal[0 * outStride] = pff;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         lineValue = prevFast - prevSlow;
         lineRing[lineRing_Idx] = lineValue;
         if( lineRing_Idx == 0 ) {
            lineHi = lineValue;
            lineLo = lineValue;
         } else {
            if( lineValue > lineHi ) {
               lineHi = lineValue;
            }
            if( lineValue < lineLo ) {
               lineLo = lineValue;
            }
         }
         highest = lineHi;
         lowest = lineLo;
         if( lineRing_Idx < lastIdx ) {
            tempReal = lineSufHi[lineRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lineSufLo[lineRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         lineRing_Idx++;
         if( lineRing_Idx > maxIdx_lineRing ) { lineRing_Idx = 0; }
         if( lineRing_Idx == 0 ) {
            sufHi = lineRing[lastIdx];
            sufLo = sufHi;
            lineSufHi[lastIdx] = sufHi;
            lineSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = lineRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lineSufHi[i] = sufHi;
               lineSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac1 = (lineValue - lowest) / range * 100.0;
         }
         pf = Math.fma(0.5, frac1 - pf, pf);
         pfRing[pfRing_Idx] = pf;
         if( pfRing_Idx == 0 ) {
            pfHi = pf;
            pfLo = pf;
         } else {
            if( pf > pfHi ) {
               pfHi = pf;
            }
            if( pf < pfLo ) {
               pfLo = pf;
            }
         }
         highest = pfHi;
         lowest = pfLo;
         if( pfRing_Idx < lastIdx ) {
            tempReal = pfSufHi[pfRing_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = pfSufLo[pfRing_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         pfRing_Idx++;
         if( pfRing_Idx > maxIdx_pfRing ) { pfRing_Idx = 0; }
         if( pfRing_Idx == 0 ) {
            sufHi = pfRing[lastIdx];
            sufLo = sufHi;
            pfSufHi[lastIdx] = sufHi;
            pfSufLo[lastIdx] = sufLo;
            i = lastIdx;
            while( i > 0 ) {
               i -= 1;
               tempReal = pfRing[i];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               pfSufHi[i] = sufHi;
               pfSufLo[i] = sufLo;
            }
         }
         range = highest - lowest;
         if( range > 0.0 ) {
            frac2 = (pf - lowest) / range * 100.0;
         }
         pff = Math.fma(0.5, frac2 - pff, pff);
         outReal[outIdx++ * outStride] = pff;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_lineRing = maxIdx_lineRing + 1;
      if( capCb_lineRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_lineSufHi = maxIdx_lineSufHi + 1;
      if( capCb_lineSufHi > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_lineSufLo = maxIdx_lineSufLo + 1;
      if( capCb_lineSufLo > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_pfRing = maxIdx_pfRing + 1;
      if( capCb_pfRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_pfSufHi = maxIdx_pfSufHi + 1;
      if( capCb_pfSufHi > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_pfSufLo = maxIdx_pfSufLo + 1;
      if( capCb_pfSufLo > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInCyclePeriod = optInCyclePeriod;
      sp.prevFast = prevFast;
      sp.prevSlow = prevSlow;
      sp.fastK = fastK;
      sp.slowK = slowK;
      sp.fastBeta = fastBeta;
      sp.slowBeta = slowBeta;
      sp.frac1 = frac1;
      sp.frac2 = frac2;
      sp.pf = pf;
      sp.pff = pff;
      sp.lineHi = lineHi;
      sp.lineLo = lineLo;
      sp.pfHi = pfHi;
      sp.pfLo = pfLo;
      sp.lastIdx = lastIdx;
      sp.lineRing_Idx = lineRing_Idx;
      sp.pfRing_Idx = pfRing_Idx;
      sp.maxIdx_lineRing = maxIdx_lineRing;
      sp.lineSufHi_Idx = lineSufHi_Idx;
      sp.maxIdx_lineSufHi = maxIdx_lineSufHi;
      sp.lineSufLo_Idx = lineSufLo_Idx;
      sp.maxIdx_lineSufLo = maxIdx_lineSufLo;
      sp.maxIdx_pfRing = maxIdx_pfRing;
      sp.pfSufHi_Idx = pfSufHi_Idx;
      sp.maxIdx_pfSufHi = maxIdx_pfSufHi;
      sp.pfSufLo_Idx = pfSufLo_Idx;
      sp.maxIdx_pfSufLo = maxIdx_pfSufLo;
      sp.cbSize_lineRing = capCb_lineRing;
      sp.cb_lineRing = lineRing;
      sp.cbSize_lineSufHi = capCb_lineSufHi;
      sp.cb_lineSufHi = lineSufHi;
      sp.cbSize_lineSufLo = capCb_lineSufLo;
      sp.cb_lineSufLo = lineSufLo;
      sp.cbSize_pfRing = capCb_pfRing;
      sp.cb_pfRing = pfRing;
      sp.cbSize_pfSufHi = capCb_pfSufHi;
      sp.cb_pfSufHi = pfSufHi;
      sp.cbSize_pfSufLo = capCb_pfSufLo;
      sp.cb_pfSufLo = pfSufLo;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* stcOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   StcStream stcOpenAndFillInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      StcStream sp = new StcStream(this);
      RetCode retCode = stcOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("STC openAndFill", inReal.length, startIdx, stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod));
      }
      throw streamFailure("STC openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind stcOpen (composition seam). */
   StcStream stcOpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod )
   {
      StcStream sp = new StcStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = stcOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("STC open", inReal.length, startIdx, stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod));
      }
      throw streamFailure("STC open", retCode);
   }
   /**
    * Open a live STC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#stc} at that bar.
    * <p>The history must hold at least {@code stcLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public StcStream stcOpen( double inReal[], int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod )
   {
      requireArgument("STC open", "inReal", inReal);
      requireHistory("STC open", inReal.length);
      return stcOpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInCyclePeriod);
   }
   /**
    * {@link Core#stcOpen} that also fills the output array(s) bit-identically
    * to {@link Core#stc} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link StcStream#outRange()}.
    */
   public StcStream stcOpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, int optInCyclePeriod, double outReal[] )
   {
      requireArgument("STC openAndFill", "inReal", inReal);
      requireHistory("STC openAndFill", inReal.length);
      int guardOutLen = openFillCount("STC openAndFill", inReal.length, stcLookback(optInFastPeriod, optInSlowPeriod, optInCyclePeriod));
      requireLength("STC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("STC openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return stcOpenAndFillInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInCyclePeriod, outBegIdx, outNBElement, outReal);
   }
