/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  093026 KL,CC  Creation (#477).
 */

   /**
    * Number of leading input bars {@link Core#cksp} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod ATR and extreme window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier ATR multiplier (default 1; minimum 0;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInStopPeriod Stop window (default 9; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ckspLookback( int optInTimePeriod, double optInMultiplier, int optInStopPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 1e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return -1;
      }
      if( optInStopPeriod == Integer.MIN_VALUE ) {
         optInStopPeriod = 9;
      } else if( optInStopPeriod < 1 || optInStopPeriod > 100000 ) {
         return -1;
      }
      /* Two stages. The first needs the Average True Range at its bar and the
       * extreme of the p bars ending there; atr_lookback(p) is p + unst, which is
       * never below max_lookback(p) = p - 1, so it covers both. The second adds
       * the q - 1 earlier first-stage bars its own window reads.
       *
       * The ATR term is written as the callee's lookback and never restated, which
       * is what makes CKSP inherit TA_FUNC_UNST_ATR rather than own an unstable
       * period of its own (supertrend.c:16-25).
       */
      return atrLookback(optInTimePeriod) + (optInStopPeriod - 1) ;

   }
   RetCode ckspImpl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     double optInMultiplier,
                     int optInStopPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outHighStop[],
                     double outLowStop[] )
   {
      int i = 0;
      int jh = 0;
      int jl = 0;
      int kh = 0;
      int kl = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int stageOneIdx = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double hh = 0;
      double ll = 0;
      double best = 0;
      double[] hRing;
      int hRing_Idx = 0;
      int maxIdx_hRing = (50)-1;
      double[] lRing;
      int lRing_Idx = 0;
      int maxIdx_lRing = (50)-1;
      double[] fhRing;
      int fhRing_Idx = 0;
      int maxIdx_fhRing = (50)-1;
      double[] flRing;
      int flRing_Idx = 0;
      int maxIdx_flRing = (50)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 1e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStopPeriod == Integer.MIN_VALUE ) {
         optInStopPeriod = 9;
      } else if( optInStopPeriod < 1 || optInStopPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outHighStop == outLowStop ) {
         return RetCode.BAD_PARAM ;
      }
      /* Four windows, all carried as rings and all walked oldest-first, the
       * cci.c:112-117 shape: an index that wrapped would be one more thing the
       * stream derivation has to prove, and the walk is the same values either
       * way.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hRing = new double[optInTimePeriod];
      maxIdx_hRing = (optInTimePeriod)-1;
      hRing_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lRing = new double[optInTimePeriod];
      maxIdx_lRing = (optInTimePeriod)-1;
      lRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      fhRing = new double[optInStopPeriod];
      maxIdx_fhRing = (optInStopPeriod)-1;
      fhRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      flRing = new double[optInStopPeriod];
      maxIdx_flRing = (optInStopPeriod)-1;
      flRing_Idx = 0;
      /* The first stage is entered q-1 bars before the first output, because the
       * second stage's window reaches that far back. Each leg is anchored on its
       * own bar rather than on the caller's startIdx (the kc.c:73-77 rule): the
       * Average True Range is seeded as if TA_ATR had been entered here, and the
       * extremes are the p-windows ending on these same bars.
       */
      stageOneIdx = startIdx - (optInStopPeriod - 1);
      /* The Average True Range, carried inline rather than taken from a call: the
       * two stages advance together one bar at a time and a whole-range buffer
       * between them would not stream (supertrend.c:47-50).
       *
       * The arithmetic order is the bit-exactness contract with TA_ATR and is not
       * to be reordered: the range first, then the two previous-close distances in
       * that order; the seed summed from 0.0 over the first optInTimePeriod True
       * Ranges and divided once; wBeta rounded first and wAlpha derived from it.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = stageOneIdx - atrLookback(optInTimePeriod) + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Skip the Average True Range's unstable period. The count comes from the
       * lookback rather than from the setting, so the two cannot disagree.
       */
      i = atrLookback(optInTimePeriod) - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      /* `today` is now stageOneIdx and prevATR is the Average True Range of the
       * bar before it. Seed the price rings with the p-1 bars the first extreme
       * window needs behind that bar.
       */
      i = stageOneIdx - optInTimePeriod + 1;
      while( i < stageOneIdx ) {
         hRing[hRing_Idx] = inHigh[i];
         lRing[lRing_Idx] = inLow[i];
         i += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      }
      /* The prologue leaves prevATR as the Average True Range of stageOneIdx and
       * `today` one past it, so that bar is finished here rather than in the loop:
       * entering the loop with it would apply a second Wilder update and shift the
       * whole series one bar early. supertrend.c takes the same step for the same
       * reason.
       */
      today = stageOneIdx;
      hRing[hRing_Idx] = inHigh[today];
      lRing[lRing_Idx] = inLow[today];
      hh = hRing[hRing_Idx];
      for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      for( jh = 0; jh < hRing_Idx; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      ll = lRing[lRing_Idx];
      for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      for( jl = 0; jl < lRing_Idx; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
      flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
      outIdx = 0;
      if( today >= startIdx ) {
         outHighStop[outIdx] = fhRing[fhRing_Idx];
         outLowStop[outIdx] = flRing[flRing_Idx];
         outIdx = outIdx + 1;
      }
      today += 1;
      hRing_Idx++;
      if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
      lRing_Idx++;
      if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      fhRing_Idx++;
      if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
      flRing_Idx++;
      if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         hRing[hRing_Idx] = tempHT;
         lRing[lRing_Idx] = tempLT;
         /* The extremes of the p bars ending here. The newest sits at the ring's
          * own index, so the oldest is the slot after it and the walk is two
          * straight runs.
          */
         hh = hRing[hRing_Idx];
         for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         for( jh = 0; jh < hRing_Idx; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         ll = lRing[lRing_Idx];
         for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         for( jl = 0; jl < lRing_Idx; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
         flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
         if( today >= startIdx ) {
            /* The second stage, over the q first-stage bars ending here. At q = 1
             * both runs are empty and the value is the bar's own, which is the
             * Chandelier Exit form.
             */
            hh = fhRing[fhRing_Idx];
            for( kh = fhRing_Idx + 1; kh < optInStopPeriod; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            for( kh = 0; kh < fhRing_Idx; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            ll = flRing[flRing_Idx];
            for( kl = flRing_Idx + 1; kl < optInStopPeriod; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            for( kl = 0; kl < flRing_Idx; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            outHighStop[outIdx] = hh;
            outLowStop[outIdx] = ll;
            outIdx = outIdx + 1;
         }
         today += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
         fhRing_Idx++;
         if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
         flRing_Idx++;
         if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode ckspImpl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     double optInMultiplier,
                     int optInStopPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outHighStop[],
                     double outLowStop[] )
   {
      int i = 0;
      int jh = 0;
      int jl = 0;
      int kh = 0;
      int kl = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int stageOneIdx = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double hh = 0;
      double ll = 0;
      double best = 0;
      double[] hRing;
      int hRing_Idx = 0;
      int maxIdx_hRing = (50)-1;
      double[] lRing;
      int lRing_Idx = 0;
      int maxIdx_lRing = (50)-1;
      double[] fhRing;
      int fhRing_Idx = 0;
      int maxIdx_fhRing = (50)-1;
      double[] flRing;
      int flRing_Idx = 0;
      int maxIdx_flRing = (50)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 1e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStopPeriod == Integer.MIN_VALUE ) {
         optInStopPeriod = 9;
      } else if( optInStopPeriod < 1 || optInStopPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outHighStop == outLowStop ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hRing = new double[optInTimePeriod];
      maxIdx_hRing = (optInTimePeriod)-1;
      hRing_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lRing = new double[optInTimePeriod];
      maxIdx_lRing = (optInTimePeriod)-1;
      lRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      fhRing = new double[optInStopPeriod];
      maxIdx_fhRing = (optInStopPeriod)-1;
      fhRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      flRing = new double[optInStopPeriod];
      maxIdx_flRing = (optInStopPeriod)-1;
      flRing_Idx = 0;
      stageOneIdx = startIdx - (optInStopPeriod - 1);
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = stageOneIdx - atrLookback(optInTimePeriod) + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      i = atrLookback(optInTimePeriod) - optInTimePeriod;
      while( i != 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      i = stageOneIdx - optInTimePeriod + 1;
      while( i < stageOneIdx ) {
         hRing[hRing_Idx] = (double)inHigh[i];
         lRing[lRing_Idx] = (double)inLow[i];
         i += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      }
      today = stageOneIdx;
      hRing[hRing_Idx] = (double)inHigh[today];
      lRing[lRing_Idx] = (double)inLow[today];
      hh = hRing[hRing_Idx];
      for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      for( jh = 0; jh < hRing_Idx; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      ll = lRing[lRing_Idx];
      for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      for( jl = 0; jl < lRing_Idx; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
      flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
      outIdx = 0;
      if( today >= startIdx ) {
         outHighStop[outIdx] = fhRing[fhRing_Idx];
         outLowStop[outIdx] = flRing[flRing_Idx];
         outIdx = outIdx + 1;
      }
      today += 1;
      hRing_Idx++;
      if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
      lRing_Idx++;
      if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      fhRing_Idx++;
      if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
      flRing_Idx++;
      if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         hRing[hRing_Idx] = tempHT;
         lRing[lRing_Idx] = tempLT;
         hh = hRing[hRing_Idx];
         for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         for( jh = 0; jh < hRing_Idx; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         ll = lRing[lRing_Idx];
         for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         for( jl = 0; jl < lRing_Idx; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
         flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
         if( today >= startIdx ) {
            hh = fhRing[fhRing_Idx];
            for( kh = fhRing_Idx + 1; kh < optInStopPeriod; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            for( kh = 0; kh < fhRing_Idx; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            ll = flRing[flRing_Idx];
            for( kl = flRing_Idx + 1; kl < optInStopPeriod; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            for( kl = 0; kl < flRing_Idx; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            outHighStop[outIdx] = hh;
            outLowStop[outIdx] = ll;
            outIdx = outIdx + 1;
         }
         today += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
         fhRing_Idx++;
         if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
         flRing_Idx++;
         if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Chande Kroll Stop places a pair of trailing stops a multiple of the
    * Average True Range away from the recent extremes, then takes the extreme
    * of those stops over a second, usually longer, window. The result is a stop
    * that follows price but only ratchets after the shorter stop has held for a
    * while. The high stop sits below price and is the level a long position
    * would give up at; the low stop sits above price and is the short side's.
    * Neither line is always above the other: when the multiplier is large
    * enough the two cross, which is the signal that the range has widened past
    * what the stops can straddle.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cksp">ta-lib.org/functions/cksp</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#ckspLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar, read only by the True Range.
    * @param optInTimePeriod ATR and extreme window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier ATR multiplier (default 1; minimum 0;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInStopPeriod Stop window (default 9; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outHighStop Trailing stop below price, the high side. Must hold at
    *        least {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
    * @param outLowStop Trailing stop above price, the low side. Must hold at
    *        least {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    */
   public OutRange cksp( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         int optInTimePeriod,
                         double optInMultiplier,
                         int optInStopPeriod,
                         double outHighStop[],
                         double outLowStop[] )
   {
      requireIndexRange("CKSP", startIdx, endIdx);
      int guardStart = clampedStart("CKSP", startIdx, ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CKSP", "inHigh", inHigh, guardInLen);
      requireLength("CKSP", "inLow", inLow, guardInLen);
      requireLength("CKSP", "inClose", inClose, guardInLen);
      requireLength("CKSP", "outHighStop", outHighStop, guardOutLen);
      requireLength("CKSP", "outLowStop", outLowStop, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ckspImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CKSP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chande Kroll Stop places a pair of trailing stops a multiple of the
    * Average True Range away from the recent extremes, then takes the extreme
    * of those stops over a second, usually longer, window. The result is a stop
    * that follows price but only ratchets after the shorter stop has held for a
    * while. The high stop sits below price and is the level a long position
    * would give up at; the low stop sits above price and is the short side's.
    * Neither line is always above the other: when the multiplier is large
    * enough the two cross, which is the signal that the range has widened past
    * what the stops can straddle.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cksp">ta-lib.org/functions/cksp</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#ckspLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar, read only by the True Range.
    * @param optInTimePeriod ATR and extreme window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier ATR multiplier (default 1; minimum 0;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInStopPeriod Stop window (default 9; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outHighStop Trailing stop below price, the high side. Must hold at
    *        least {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
    * @param outLowStop Trailing stop above price, the low side. Must hold at
    *        least {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    */
   public OutRange cksp( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         int optInTimePeriod,
                         double optInMultiplier,
                         int optInStopPeriod,
                         double outHighStop[],
                         double outLowStop[] )
   {
      requireIndexRange("CKSP", startIdx, endIdx);
      int guardStart = clampedStart("CKSP", startIdx, ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CKSP", "inHigh", inHigh, guardInLen);
      requireLength("CKSP", "inLow", inLow, guardInLen);
      requireLength("CKSP", "inClose", inClose, guardInLen);
      requireLength("CKSP", "outHighStop", outHighStop, guardOutLen);
      requireLength("CKSP", "outLowStop", outLowStop, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ckspImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CKSP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CKSP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cksp} over the same series.
    * Open with {@link Core#ckspOpen}; there is no close — the handle is
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
   public static final class CkspStream {
      private Core core;
      private int optInTimePeriod;
      private double optInMultiplier;
      private int optInStopPeriod;
      private double prevATR;
      private double wAlpha;
      private double wBeta;
      private int hRing_Idx;
      private int lRing_Idx;
      private int fhRing_Idx;
      private int flRing_Idx;
      private int maxIdx_hRing;
      private int maxIdx_lRing;
      private int maxIdx_fhRing;
      private int maxIdx_flRing;
      private double lag1_inClose;
      private int cbSize_hRing;
      private double[] cb_hRing;
      private int cbSize_lRing;
      private double[] cb_lRing;
      private int cbSize_fhRing;
      private double[] cb_fhRing;
      private int cbSize_flRing;
      private double[] cb_flRing;
      private double cur_outHighStop;
      private double cur_outLowStop;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CkspStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#cksp} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(CkspOut)} keeps answering the previous
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
            throw failure("CKSP advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CkspStream( CkspStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMultiplier = other.optInMultiplier;
         this.optInStopPeriod = other.optInStopPeriod;
         this.prevATR = other.prevATR;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.hRing_Idx = other.hRing_Idx;
         this.lRing_Idx = other.lRing_Idx;
         this.fhRing_Idx = other.fhRing_Idx;
         this.flRing_Idx = other.flRing_Idx;
         this.maxIdx_hRing = other.maxIdx_hRing;
         this.maxIdx_lRing = other.maxIdx_lRing;
         this.maxIdx_fhRing = other.maxIdx_fhRing;
         this.maxIdx_flRing = other.maxIdx_flRing;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_hRing = other.cbSize_hRing;
         this.cb_hRing = other.cb_hRing.clone();
         this.cbSize_lRing = other.cbSize_lRing;
         this.cb_lRing = other.cb_lRing.clone();
         this.cbSize_fhRing = other.cbSize_fhRing;
         this.cb_fhRing = other.cb_fhRing.clone();
         this.cbSize_flRing = other.cbSize_flRing;
         this.cb_flRing = other.cb_flRing.clone();
         this.cur_outHighStop = other.cur_outHighStop;
         this.cur_outLowStop = other.cur_outLowStop;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(CkspOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, CkspOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CKSP update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("CKSP update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CKSP update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.ckspStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         out.highStop = this.cur_outHighStop;
         out.lowStop = this.cur_outLowStop;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, CkspOut out ) {
         requireArgument("CKSP peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CKSP peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         CkspStream sp = this;
         int jh = 0;
         int jl = 0;
         int kh = 0;
         int kl = 0;
         double val2 = 0.0;
         double val3 = 0.0;
         double greatest = 0.0;
         double tempCY = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double hh = 0.0;
         double ll = 0.0;
         double best = 0.0;
         double cur_outHighStop = 0.0;
         double cur_outLowStop = 0.0;
         double prevATR = sp.prevATR;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         int pkSlot3 = -1;
         double pkVal3 = 0.0;
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = sp.lag1_inClose;
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
         pkSlot0 = sp.hRing_Idx;
         pkVal0 = tempHT;
         pkSlot1 = sp.lRing_Idx;
         pkVal1 = tempLT;
         /* The extremes of the p bars ending here. The newest sits at the ring's
          * own index, so the oldest is the slot after it and the walk is two
          * straight runs.
          */
         hh = (sp.hRing_Idx != pkSlot0) ? sp.cb_hRing[sp.hRing_Idx] : pkVal0;
         for( jh = sp.hRing_Idx + 1; jh < sp.optInTimePeriod; jh += 1 ) {
            best = sp.cb_hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         for( jh = 0; jh < sp.hRing_Idx; jh += 1 ) {
            best = sp.cb_hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         ll = (sp.lRing_Idx != pkSlot1) ? sp.cb_lRing[sp.lRing_Idx] : pkVal1;
         for( jl = sp.lRing_Idx + 1; jl < sp.optInTimePeriod; jl += 1 ) {
            best = sp.cb_lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         for( jl = 0; jl < sp.lRing_Idx; jl += 1 ) {
            best = sp.cb_lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         pkSlot2 = sp.fhRing_Idx;
         pkVal2 = hh - sp.optInMultiplier * prevATR;
         pkSlot3 = sp.flRing_Idx;
         pkVal3 = ll + sp.optInMultiplier * prevATR;
         /* The second stage, over the q first-stage bars ending here. At q = 1
          * both runs are empty and the value is the bar's own, which is the
          * Chandelier Exit form.
          */
         hh = (sp.fhRing_Idx != pkSlot2) ? sp.cb_fhRing[sp.fhRing_Idx] : pkVal2;
         for( kh = sp.fhRing_Idx + 1; kh < sp.optInStopPeriod; kh += 1 ) {
            best = sp.cb_fhRing[kh];
            if( best > hh ) {
               hh = best;
            }
         }
         for( kh = 0; kh < sp.fhRing_Idx; kh += 1 ) {
            best = sp.cb_fhRing[kh];
            if( best > hh ) {
               hh = best;
            }
         }
         ll = (sp.flRing_Idx != pkSlot3) ? sp.cb_flRing[sp.flRing_Idx] : pkVal3;
         for( kl = sp.flRing_Idx + 1; kl < sp.optInStopPeriod; kl += 1 ) {
            best = sp.cb_flRing[kl];
            if( best < ll ) {
               ll = best;
            }
         }
         for( kl = 0; kl < sp.flRing_Idx; kl += 1 ) {
            best = sp.cb_flRing[kl];
            if( best < ll ) {
               ll = best;
            }
         }
         cur_outHighStop = hh;
         cur_outLowStop = ll;
         out.highStop = cur_outHighStop;
         out.lowStop = cur_outLowStop;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( CkspOut out ) {
         requireArgument("CKSP value", "out", out);
         out.highStop = this.cur_outHighStop;
         out.lowStop = this.cur_outLowStop;
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
      public CkspStream clone() {
         return new CkspStream(this);
      }
   }

   /**
    * The outputs of one CKSP bar, written by the stream into an object the
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
   public static final class CkspOut {
      /** Trailing stop below price, the high side. */
      public double highStop;
      /** Trailing stop above price, the low side. */
      public double lowStop;
   }
   private void ckspStepImpl( CkspStream sp, double inHigh, double inLow, double inClose )
   {
      int jh = 0;
      int jl = 0;
      int kh = 0;
      int kl = 0;
      double val2 = 0.0;
      double val3 = 0.0;
      double greatest = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      double hh = 0.0;
      double ll = 0.0;
      double best = 0.0;
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      greatest = tempHT - tempLT;
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.prevATR = Math.fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
      sp.cb_hRing[sp.hRing_Idx] = tempHT;
      sp.cb_lRing[sp.lRing_Idx] = tempLT;
      /* The extremes of the p bars ending here. The newest sits at the ring's
       * own index, so the oldest is the slot after it and the walk is two
       * straight runs.
       */
      hh = sp.cb_hRing[sp.hRing_Idx];
      for( jh = sp.hRing_Idx + 1; jh < sp.optInTimePeriod; jh += 1 ) {
         best = sp.cb_hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      for( jh = 0; jh < sp.hRing_Idx; jh += 1 ) {
         best = sp.cb_hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      ll = sp.cb_lRing[sp.lRing_Idx];
      for( jl = sp.lRing_Idx + 1; jl < sp.optInTimePeriod; jl += 1 ) {
         best = sp.cb_lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      for( jl = 0; jl < sp.lRing_Idx; jl += 1 ) {
         best = sp.cb_lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      sp.cb_fhRing[sp.fhRing_Idx] = hh - sp.optInMultiplier * sp.prevATR;
      sp.cb_flRing[sp.flRing_Idx] = ll + sp.optInMultiplier * sp.prevATR;
      /* The second stage, over the q first-stage bars ending here. At q = 1
       * both runs are empty and the value is the bar's own, which is the
       * Chandelier Exit form.
       */
      hh = sp.cb_fhRing[sp.fhRing_Idx];
      for( kh = sp.fhRing_Idx + 1; kh < sp.optInStopPeriod; kh += 1 ) {
         best = sp.cb_fhRing[kh];
         if( best > hh ) {
            hh = best;
         }
      }
      for( kh = 0; kh < sp.fhRing_Idx; kh += 1 ) {
         best = sp.cb_fhRing[kh];
         if( best > hh ) {
            hh = best;
         }
      }
      ll = sp.cb_flRing[sp.flRing_Idx];
      for( kl = sp.flRing_Idx + 1; kl < sp.optInStopPeriod; kl += 1 ) {
         best = sp.cb_flRing[kl];
         if( best < ll ) {
            ll = best;
         }
      }
      for( kl = 0; kl < sp.flRing_Idx; kl += 1 ) {
         best = sp.cb_flRing[kl];
         if( best < ll ) {
            ll = best;
         }
      }
      sp.cur_outHighStop = hh;
      sp.cur_outLowStop = ll;
      sp.hRing_Idx = sp.hRing_Idx + 1;
      if( sp.hRing_Idx > sp.maxIdx_hRing ) {
         sp.hRing_Idx = 0;
      }
      sp.lRing_Idx = sp.lRing_Idx + 1;
      if( sp.lRing_Idx > sp.maxIdx_lRing ) {
         sp.lRing_Idx = 0;
      }
      sp.fhRing_Idx = sp.fhRing_Idx + 1;
      if( sp.fhRing_Idx > sp.maxIdx_fhRing ) {
         sp.fhRing_Idx = 0;
      }
      sp.flRing_Idx = sp.flRing_Idx + 1;
      if( sp.flRing_Idx > sp.maxIdx_flRing ) {
         sp.flRing_Idx = 0;
      }
      sp.lag1_inClose = inClose;
   }
   private RetCode ckspOpenImpl( CkspStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, int optInStopPeriod, MInteger outBegIdx, MInteger outNBElement, double outHighStop[], double outLowStop[], int outStride )
   {
      int i = 0;
      int jh = 0;
      int jl = 0;
      int kh = 0;
      int kl = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int stageOneIdx = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double hh = 0;
      double ll = 0;
      double best = 0;
      double[] hRing;
      int hRing_Idx = 0;
      int maxIdx_hRing = (50)-1;
      double[] lRing;
      int lRing_Idx = 0;
      int maxIdx_lRing = (50)-1;
      double[] fhRing;
      int fhRing_Idx = 0;
      int maxIdx_fhRing = (50)-1;
      double[] flRing;
      int flRing_Idx = 0;
      int maxIdx_flRing = (50)-1;
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
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 1e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInStopPeriod == Integer.MIN_VALUE ) {
         optInStopPeriod = 9;
      } else if( optInStopPeriod < 1 || optInStopPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Four windows, all carried as rings and all walked oldest-first, the
       * cci.c:112-117 shape: an index that wrapped would be one more thing the
       * stream derivation has to prove, and the walk is the same values either
       * way.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hRing = new double[optInTimePeriod];
      maxIdx_hRing = (optInTimePeriod)-1;
      hRing_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lRing = new double[optInTimePeriod];
      maxIdx_lRing = (optInTimePeriod)-1;
      lRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      fhRing = new double[optInStopPeriod];
      maxIdx_fhRing = (optInStopPeriod)-1;
      fhRing_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      flRing = new double[optInStopPeriod];
      maxIdx_flRing = (optInStopPeriod)-1;
      flRing_Idx = 0;
      /* The first stage is entered q-1 bars before the first output, because the
       * second stage's window reaches that far back. Each leg is anchored on its
       * own bar rather than on the caller's startIdx (the kc.c:73-77 rule): the
       * Average True Range is seeded as if TA_ATR had been entered here, and the
       * extremes are the p-windows ending on these same bars.
       */
      stageOneIdx = startIdx - (optInStopPeriod - 1);
      /* The Average True Range, carried inline rather than taken from a call: the
       * two stages advance together one bar at a time and a whole-range buffer
       * between them would not stream (supertrend.c:47-50).
       *
       * The arithmetic order is the bit-exactness contract with TA_ATR and is not
       * to be reordered: the range first, then the two previous-close distances in
       * that order; the seed summed from 0.0 over the first optInTimePeriod True
       * Ranges and divided once; wBeta rounded first and wAlpha derived from it.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = stageOneIdx - atrLookback(optInTimePeriod) + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Skip the Average True Range's unstable period. The count comes from the
       * lookback rather than from the setting, so the two cannot disagree.
       */
      i = atrLookback(optInTimePeriod) - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      /* `today` is now stageOneIdx and prevATR is the Average True Range of the
       * bar before it. Seed the price rings with the p-1 bars the first extreme
       * window needs behind that bar.
       */
      i = stageOneIdx - optInTimePeriod + 1;
      while( i < stageOneIdx ) {
         hRing[hRing_Idx] = inHigh[i];
         lRing[lRing_Idx] = inLow[i];
         i += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      }
      /* The prologue leaves prevATR as the Average True Range of stageOneIdx and
       * `today` one past it, so that bar is finished here rather than in the loop:
       * entering the loop with it would apply a second Wilder update and shift the
       * whole series one bar early. supertrend.c takes the same step for the same
       * reason.
       */
      today = stageOneIdx;
      hRing[hRing_Idx] = inHigh[today];
      lRing[lRing_Idx] = inLow[today];
      hh = hRing[hRing_Idx];
      for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      for( jh = 0; jh < hRing_Idx; jh += 1 ) {
         best = hRing[jh];
         if( best > hh ) {
            hh = best;
         }
      }
      ll = lRing[lRing_Idx];
      for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      for( jl = 0; jl < lRing_Idx; jl += 1 ) {
         best = lRing[jl];
         if( best < ll ) {
            ll = best;
         }
      }
      fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
      flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
      outIdx = 0;
      if( today >= startIdx ) {
         outHighStop[outIdx * outStride] = fhRing[fhRing_Idx];
         outLowStop[outIdx * outStride] = flRing[flRing_Idx];
         outIdx = outIdx + 1;
      }
      today += 1;
      hRing_Idx++;
      if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
      lRing_Idx++;
      if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
      fhRing_Idx++;
      if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
      flRing_Idx++;
      if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         hRing[hRing_Idx] = tempHT;
         lRing[lRing_Idx] = tempLT;
         /* The extremes of the p bars ending here. The newest sits at the ring's
          * own index, so the oldest is the slot after it and the walk is two
          * straight runs.
          */
         hh = hRing[hRing_Idx];
         for( jh = hRing_Idx + 1; jh < optInTimePeriod; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         for( jh = 0; jh < hRing_Idx; jh += 1 ) {
            best = hRing[jh];
            if( best > hh ) {
               hh = best;
            }
         }
         ll = lRing[lRing_Idx];
         for( jl = lRing_Idx + 1; jl < optInTimePeriod; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         for( jl = 0; jl < lRing_Idx; jl += 1 ) {
            best = lRing[jl];
            if( best < ll ) {
               ll = best;
            }
         }
         fhRing[fhRing_Idx] = hh - optInMultiplier * prevATR;
         flRing[flRing_Idx] = ll + optInMultiplier * prevATR;
         if( today >= startIdx ) {
            /* The second stage, over the q first-stage bars ending here. At q = 1
             * both runs are empty and the value is the bar's own, which is the
             * Chandelier Exit form.
             */
            hh = fhRing[fhRing_Idx];
            for( kh = fhRing_Idx + 1; kh < optInStopPeriod; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            for( kh = 0; kh < fhRing_Idx; kh += 1 ) {
               best = fhRing[kh];
               if( best > hh ) {
                  hh = best;
               }
            }
            ll = flRing[flRing_Idx];
            for( kl = flRing_Idx + 1; kl < optInStopPeriod; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            for( kl = 0; kl < flRing_Idx; kl += 1 ) {
               best = flRing[kl];
               if( best < ll ) {
                  ll = best;
               }
            }
            outHighStop[outIdx * outStride] = hh;
            outLowStop[outIdx * outStride] = ll;
            outIdx = outIdx + 1;
         }
         today += 1;
         hRing_Idx++;
         if( hRing_Idx > maxIdx_hRing ) { hRing_Idx = 0; }
         lRing_Idx++;
         if( lRing_Idx > maxIdx_lRing ) { lRing_Idx = 0; }
         fhRing_Idx++;
         if( fhRing_Idx > maxIdx_fhRing ) { fhRing_Idx = 0; }
         flRing_Idx++;
         if( flRing_Idx > maxIdx_flRing ) { flRing_Idx = 0; }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_hRing = maxIdx_hRing + 1;
      if( capCb_hRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_lRing = maxIdx_lRing + 1;
      if( capCb_lRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_fhRing = maxIdx_fhRing + 1;
      if( capCb_fhRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_flRing = maxIdx_flRing + 1;
      if( capCb_flRing > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMultiplier = optInMultiplier;
      sp.optInStopPeriod = optInStopPeriod;
      sp.prevATR = prevATR;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.hRing_Idx = hRing_Idx;
      sp.lRing_Idx = lRing_Idx;
      sp.fhRing_Idx = fhRing_Idx;
      sp.flRing_Idx = flRing_Idx;
      sp.maxIdx_hRing = maxIdx_hRing;
      sp.maxIdx_lRing = maxIdx_lRing;
      sp.maxIdx_fhRing = maxIdx_fhRing;
      sp.maxIdx_flRing = maxIdx_flRing;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_hRing = capCb_hRing;
      sp.cb_hRing = hRing;
      sp.cbSize_lRing = capCb_lRing;
      sp.cb_lRing = lRing;
      sp.cbSize_fhRing = capCb_fhRing;
      sp.cb_fhRing = fhRing;
      sp.cbSize_flRing = capCb_flRing;
      sp.cb_flRing = flRing;
      sp.cur_outHighStop = outHighStop[(outNBElement.value - 1) * outStride];
      sp.cur_outLowStop = outLowStop[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* ckspOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CkspStream ckspOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, int optInStopPeriod, MInteger outBegIdx, MInteger outNBElement, double outHighStop[], double outLowStop[] )
   {
      CkspStream sp = new CkspStream(this);
      RetCode retCode = ckspOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CKSP openAndFill", inHigh.length, startIdx, ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod));
      }
      throw streamFailure("CKSP openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ckspOpen (composition seam). */
   CkspStream ckspOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, int optInStopPeriod )
   {
      CkspStream sp = new CkspStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outHighStop = new double[1];
      double[] sink_outLowStop = new double[1];
      RetCode retCode = ckspOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, sink_outHighStop, sink_outLowStop, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CKSP open", inHigh.length, startIdx, ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod));
      }
      throw streamFailure("CKSP open", retCode);
   }
   /**
    * Open a live CKSP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cksp} at that bar.
    * <p>The history must hold at least {@code ckspLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CkspStream ckspOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier, int optInStopPeriod )
   {
      requireArgument("CKSP open", "inHigh", inHigh);
      requireHistory("CKSP open", inHigh.length);
      requireArgument("CKSP open", "inLow", inLow);
      requireArgument("CKSP open", "inClose", inClose);
      requireHistoryLength("CKSP open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CKSP open", "inClose", inClose.length, inHigh.length);
      return ckspOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier, optInStopPeriod);
   }
   /**
    * {@link Core#ckspOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cksp} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CkspStream#outRange()}.
    */
   public CkspStream ckspOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier, int optInStopPeriod, double outHighStop[], double outLowStop[] )
   {
      requireArgument("CKSP openAndFill", "inHigh", inHigh);
      requireHistory("CKSP openAndFill", inHigh.length);
      requireArgument("CKSP openAndFill", "inLow", inLow);
      requireArgument("CKSP openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CKSP openAndFill", inHigh.length, ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod));
      requireHistoryLength("CKSP openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CKSP openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("CKSP openAndFill", "outHighStop", outHighStop, guardOutLen);
      requireLength("CKSP openAndFill", "outLowStop", outLowStop, guardOutLen);
      if( (Object)outHighStop == (Object)inHigh || (Object)outHighStop == (Object)inLow || (Object)outHighStop == (Object)inClose || (Object)outLowStop == (Object)inHigh || (Object)outLowStop == (Object)inLow || (Object)outLowStop == (Object)inClose || (Object)outHighStop == (Object)outLowStop ) {
         throw streamFailure("CKSP openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ckspOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier, optInStopPeriod, outBegIdx, outNBElement, outHighStop, outLowStop);
   }
