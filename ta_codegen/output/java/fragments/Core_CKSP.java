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
 *  093026 MF,CC  Rolling extrema in a fixed number of comparisons per bar.
 *  100726 MF,CC  #492. Under an Auto level the stop window is counted again.
 */

   /**
    * Number of leading input bars {@link Core#cksp} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the first stops (default 1; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param optInStopPeriod Window over which each first stop takes its extreme
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
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
      /* The first stops need the Average True Range at their own bar, and the
       * stop window reaches optInStopPeriod-1 first stops further back. The ATR
       * term is never restated here, which is what makes CKSP inherit
       * TA_FUNC_UNST_ATR.
       *
       * A stop can rest on a first stop optInStopPeriod-1 bars old, whose ATR
       * was that much closer to its seed: the first difference two starts show
       * is the smaller for it, and an Auto level is held against that one.
       */
      return atrLookback(optInTimePeriod) + optInStopPeriod - 1 + (this.unstableCount(FuncUnstId.ATR.ordinal(), optInStopPeriod - 1, optInStopPeriod - 1) - this.unstableCount(FuncUnstId.ATR.ordinal(), 0, 0)) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#cksp}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the first stops (default 1; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param optInStopPeriod Window over which each first stop takes its extreme
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ckspDisplayShift( int optInTimePeriod, double optInMultiplier, int optInStopPeriod, int outputIdx )
   {
      if( ckspLookback( optInTimePeriod, optInMultiplier, optInStopPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
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
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      double[] hsBuf;
      int hsBuf_Idx = 0;
      int maxIdx_hsBuf = (30)-1;
      double[] lsBuf;
      int lsBuf_Idx = 0;
      int maxIdx_lsBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int iq = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
      int lastQ = 0;
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
      double tempReal = 0;
      double hhPre = 0;
      double llPre = 0;
      double hsPre = 0;
      double lsPre = 0;
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      double highStop = 0;
      double lowStop = 0;
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
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      /* Four rolling extrema, van Herk / Gil-Werman, one buffer each: the window
       * ending in slot j is the current block's prefix extremum joined with the
       * previous block's suffix extremum from slot j+1. A slot holds the raw value
       * while its block is filling and is turned into the suffix extremum, in
       * place, when the block completes; slot j+1 is always still the previous
       * block's when slot j is filled. The high and low sides share an index.
       *
       * The extrema are exact, so both outputs must stay bit-identical to
       * TA_MAX( TA_MAX(high) - x*TA_ATR ) and its mirror, whatever the block
       * phase; only the sign of a zero tied between +0.0 and -0.0 is free.
       */
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hhBuf = new double[optInTimePeriod];
      maxIdx_hhBuf = (optInTimePeriod)-1;
      hhBuf_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      llBuf = new double[optInTimePeriod];
      maxIdx_llBuf = (optInTimePeriod)-1;
      llBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hsBuf = new double[optInStopPeriod];
      maxIdx_hsBuf = (optInStopPeriod)-1;
      hsBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lsBuf = new double[optInStopPeriod];
      maxIdx_lsBuf = (optInStopPeriod)-1;
      lsBuf_Idx = 0;
      lastP = optInTimePeriod - 1;
      lastQ = optInStopPeriod - 1;
      /* The Average True Range is carried inline rather than taken from a call,
       * because the two stages advance together one bar at a time and a
       * whole-range buffer between them would not stream.
       *
       * The arithmetic order below is the bit-exactness contract with TA_ATR (do
       * not reorder): True Range from high-low, then the two previous-close
       * distances in that order; the seed summed from 0.0 over the first 'period'
       * True Ranges and divided once; the same two Wilder coefficients, wBeta
       * rounded and wAlpha derived from it, in one fused statement.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
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
      /* Skip the bars the lookback adds for the unstable period. Taking the count
       * from the lookback rather than naming the setting keeps the two from
       * disagreeing.
       */
      i = lookbackTotal - lastQ - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
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
      /* prevATR is now the Average True Range of bar today-1, the first bar with
       * a first stop. Its extreme window is one whole block, taken straight from
       * the input.
       */
      highest = inHigh[today - 1];
      lowest = inLow[today - 1];
      hhBuf[lastP] = highest;
      llBuf[lastP] = lowest;
      ip = lastP;
      while( ip > 0 ) {
         ip -= 1;
         tempHT = inHigh[today - 1 - lastP + ip];
         tempLT = inLow[today - 1 - lastP + ip];
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         hhBuf[ip] = highest;
         llBuf[ip] = lowest;
      }
      hhPre = highest;
      llPre = lowest;
      /* The multiple of the ATR is formed on its own, never fused into the
       * offset, so that each first stop is TA_MAX - x*TA_ATR (or its mirror) as a
       * caller composing the three functions would compute it.
       */
      band = optInMultiplier * prevATR;
      highStop = highest - band;
      lowStop = lowest + band;
      hsPre = highStop;
      lsPre = lowStop;
      hsBuf[hsBuf_Idx] = highStop;
      lsBuf[hsBuf_Idx] = lowStop;
      hsBuf_Idx++;
      if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
      /* Fill the first stop window, through startIdx inclusive. */
      while( today <= startIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         /* Keep this shape: the prefix extreme selected in a local, the
          * block-start override after it, one store. A store made only when the
          * bar sets a new extreme, or the override ahead of the select, compiles
          * to a data-dependent branch in the stream step, where the prefix lives
          * in the handle.
          */
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         if( highStop > hsPre ) {
            hsPre = highStop;
         }
         if( lowStop < lsPre ) {
            lsPre = lowStop;
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         today += 1;
      }
      /* The first stop window is one whole block as well: its extremes are the
       * prefix extremes, and the block is complete.
       */
      sufHi = hsBuf[lastQ];
      sufLo = lsBuf[lastQ];
      iq = lastQ;
      while( iq > 1 ) {
         iq -= 1;
         tempReal = hsBuf[iq];
         if( tempReal > sufHi ) {
            sufHi = tempReal;
         }
         hsBuf[iq] = sufHi;
         tempReal = lsBuf[iq];
         if( tempReal < sufLo ) {
            sufLo = tempReal;
         }
         lsBuf[iq] = sufLo;
      }
      outHighStop[0] = hsPre;
      outLowStop[0] = lsPre;
      outIdx = 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         highest = hsPre;
         lowest = lsPre;
         if( highStop > highest ) {
            highest = highStop;
         }
         if( lowStop < lowest ) {
            lowest = lowStop;
         }
         if( hsBuf_Idx == 0 ) {
            highest = highStop;
            lowest = lowStop;
         }
         hsPre = highest;
         lsPre = lowest;
         if( hsBuf_Idx < lastQ ) {
            tempReal = hsBuf[hsBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lsBuf[hsBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         outHighStop[outIdx] = highest;
         outLowStop[outIdx] = lowest;
         /* A completed block becomes its suffix extrema. Nothing this bar reads
          * them, so both passes stay below the output stores, where the peek
          * frame never runs them.
          */
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         if( hsBuf_Idx == 0 ) {
            sufHi = hsBuf[lastQ];
            sufLo = lsBuf[lastQ];
            iq = lastQ;
            while( iq > 1 ) {
               iq -= 1;
               tempReal = hsBuf[iq];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hsBuf[iq] = sufHi;
               tempReal = lsBuf[iq];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lsBuf[iq] = sufLo;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
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
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      double[] hsBuf;
      int hsBuf_Idx = 0;
      int maxIdx_hsBuf = (30)-1;
      double[] lsBuf;
      int lsBuf_Idx = 0;
      int maxIdx_lsBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int iq = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
      int lastQ = 0;
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
      double tempReal = 0;
      double hhPre = 0;
      double llPre = 0;
      double hsPre = 0;
      double lsPre = 0;
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      double highStop = 0;
      double lowStop = 0;
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
      hhBuf = new double[optInTimePeriod];
      maxIdx_hhBuf = (optInTimePeriod)-1;
      hhBuf_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      llBuf = new double[optInTimePeriod];
      maxIdx_llBuf = (optInTimePeriod)-1;
      llBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hsBuf = new double[optInStopPeriod];
      maxIdx_hsBuf = (optInStopPeriod)-1;
      hsBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lsBuf = new double[optInStopPeriod];
      maxIdx_lsBuf = (optInStopPeriod)-1;
      lsBuf_Idx = 0;
      lastP = optInTimePeriod - 1;
      lastQ = optInStopPeriod - 1;
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
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
      i = lookbackTotal - lastQ - optInTimePeriod;
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
      highest = (double)inHigh[today - 1];
      lowest = (double)inLow[today - 1];
      hhBuf[lastP] = highest;
      llBuf[lastP] = lowest;
      ip = lastP;
      while( ip > 0 ) {
         ip -= 1;
         tempHT = (double)inHigh[today - 1 - lastP + ip];
         tempLT = (double)inLow[today - 1 - lastP + ip];
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         hhBuf[ip] = highest;
         llBuf[ip] = lowest;
      }
      hhPre = highest;
      llPre = lowest;
      band = optInMultiplier * prevATR;
      highStop = highest - band;
      lowStop = lowest + band;
      hsPre = highStop;
      lsPre = lowStop;
      hsBuf[hsBuf_Idx] = highStop;
      lsBuf[hsBuf_Idx] = lowStop;
      hsBuf_Idx++;
      if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
      while( today <= startIdx ) {
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
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         if( highStop > hsPre ) {
            hsPre = highStop;
         }
         if( lowStop < lsPre ) {
            lsPre = lowStop;
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         today += 1;
      }
      sufHi = hsBuf[lastQ];
      sufLo = lsBuf[lastQ];
      iq = lastQ;
      while( iq > 1 ) {
         iq -= 1;
         tempReal = hsBuf[iq];
         if( tempReal > sufHi ) {
            sufHi = tempReal;
         }
         hsBuf[iq] = sufHi;
         tempReal = lsBuf[iq];
         if( tempReal < sufLo ) {
            sufLo = tempReal;
         }
         lsBuf[iq] = sufLo;
      }
      outHighStop[0] = hsPre;
      outLowStop[0] = lsPre;
      outIdx = 1;
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
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         highest = hsPre;
         lowest = lsPre;
         if( highStop > highest ) {
            highest = highStop;
         }
         if( lowStop < lowest ) {
            lowest = lowStop;
         }
         if( hsBuf_Idx == 0 ) {
            highest = highStop;
            lowest = lowStop;
         }
         hsPre = highest;
         lsPre = lowest;
         if( hsBuf_Idx < lastQ ) {
            tempReal = hsBuf[hsBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lsBuf[hsBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         outHighStop[outIdx] = highest;
         outLowStop[outIdx] = lowest;
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         if( hsBuf_Idx == 0 ) {
            sufHi = hsBuf[lastQ];
            sufLo = lsBuf[lastQ];
            iq = lastQ;
            while( iq > 1 ) {
               iq -= 1;
               tempReal = hsBuf[iq];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hsBuf[iq] = sufHi;
               tempReal = lsBuf[iq];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lsBuf[iq] = sufLo;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Chande and Kroll's two-line volatility stop. The highest high is offset
    * down, and the lowest low up, by a multiple of the Average True Range, and
    * each line then takes the extreme of its own recent values, so it moves
    * only when a new extreme enters the stop window or an old one leaves it.
    * Price above both lines reads as an uptrend and price below both as a
    * downtrend.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cksp">ta-lib.org/functions/cksp</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The two lines are named for how they are built, not for the position they protect, because published implementations give "long stop" and "short stop" to opposite lines. {@code outHighStop} is the line most platforms plot as the short stop, {@code outLowStop} the one they plot as the long stop.</li>
    * <li>Neither line is always above the other: a large multiplier pushes the high line below the low one.</li>
    * <li>The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.</li>
    * <li>The book's own settings are reported as a multiplier of 3 and a stop period of 20; the defaults here are the ones charting platforms ship.</li>
    * <li>A stop period of 1 leaves the first stops unchanged, which is the Chandelier Exit on both sides.</li>
    * <li>Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ckspLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the first stops (default 1; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param optInStopPeriod Window over which each first stop takes its extreme
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outHighStop Highest of the recent first high stops; usually plotted
    *        as the short stop. Must hold at least
    *        {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outLowStop Lowest of the recent first low stops; usually plotted as
    *        the long stop. Must hold at least
    *        {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, and never be
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
    * @see Core#atr
    * @see Core#max
    * @see Core#min
    * @see Core#supertrend
    * @see Core#kc
    * @see Core#donchian
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
    * Chande and Kroll's two-line volatility stop. The highest high is offset
    * down, and the lowest low up, by a multiple of the Average True Range, and
    * each line then takes the extreme of its own recent values, so it moves
    * only when a new extreme enters the stop window or an old one leaves it.
    * Price above both lines reads as an uptrend and price below both as a
    * downtrend.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cksp">ta-lib.org/functions/cksp</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The two lines are named for how they are built, not for the position they protect, because published implementations give "long stop" and "short stop" to opposite lines. {@code outHighStop} is the line most platforms plot as the short stop, {@code outLowStop} the one they plot as the long stop.</li>
    * <li>Neither line is always above the other: a large multiplier pushes the high line below the low one.</li>
    * <li>The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.</li>
    * <li>The book's own settings are reported as a multiplier of 3 and a stop period of 20; the defaults here are the ones charting platforms ship.</li>
    * <li>A stop period of 1 leaves the first stops unchanged, which is the Chandelier Exit on both sides.</li>
    * <li>Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ckspLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 10; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the first stops (default 1; minimum 0; {@link Core#REAL_DEFAULT}
    *        selects the default).
    * @param optInStopPeriod Window over which each first stop takes its extreme
    *        (default 9; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outHighStop Highest of the recent first high stops; usually plotted
    *        as the short stop. Must hold at least
    *        {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outLowStop Lowest of the recent first low stops; usually plotted as
    *        the long stop. Must hold at least
    *        {@code endIdx - max(startIdx, ckspLookback(...)) + 1} values, and never be
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
    * @see Core#atr
    * @see Core#max
    * @see Core#min
    * @see Core#supertrend
    * @see Core#kc
    * @see Core#donchian
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
      private int lastP;
      private int lastQ;
      private double prevATR;
      private double wAlpha;
      private double wBeta;
      private double hhPre;
      private double llPre;
      private double hsPre;
      private double lsPre;
      private int hhBuf_Idx;
      private int hsBuf_Idx;
      private int maxIdx_hhBuf;
      private int llBuf_Idx;
      private int maxIdx_llBuf;
      private int maxIdx_hsBuf;
      private int lsBuf_Idx;
      private int maxIdx_lsBuf;
      private double lag1_inClose;
      private int cbSize_hhBuf;
      private double[] cb_hhBuf;
      private int cbSize_llBuf;
      private double[] cb_llBuf;
      private int cbSize_hsBuf;
      private double[] cb_hsBuf;
      private int cbSize_lsBuf;
      private double[] cb_lsBuf;
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
         this.lastP = other.lastP;
         this.lastQ = other.lastQ;
         this.prevATR = other.prevATR;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.hhPre = other.hhPre;
         this.llPre = other.llPre;
         this.hsPre = other.hsPre;
         this.lsPre = other.lsPre;
         this.hhBuf_Idx = other.hhBuf_Idx;
         this.hsBuf_Idx = other.hsBuf_Idx;
         this.maxIdx_hhBuf = other.maxIdx_hhBuf;
         this.llBuf_Idx = other.llBuf_Idx;
         this.maxIdx_llBuf = other.maxIdx_llBuf;
         this.maxIdx_hsBuf = other.maxIdx_hsBuf;
         this.lsBuf_Idx = other.lsBuf_Idx;
         this.maxIdx_lsBuf = other.maxIdx_lsBuf;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_hhBuf = other.cbSize_hhBuf;
         this.cb_hhBuf = other.cb_hhBuf.clone();
         this.cbSize_llBuf = other.cbSize_llBuf;
         this.cb_llBuf = other.cb_llBuf.clone();
         this.cbSize_hsBuf = other.cbSize_hsBuf;
         this.cb_hsBuf = other.cb_hsBuf.clone();
         this.cbSize_lsBuf = other.cbSize_lsBuf;
         this.cb_lsBuf = other.cb_lsBuf.clone();
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
         double val2 = 0.0;
         double val3 = 0.0;
         double greatest = 0.0;
         double tempCY = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double tempReal = 0.0;
         double highest = 0.0;
         double lowest = 0.0;
         double band = 0.0;
         double highStop = 0.0;
         double lowStop = 0.0;
         double cur_outHighStop = 0.0;
         double cur_outLowStop = 0.0;
         int hhBuf_Idx = sp.hhBuf_Idx;
         double hhPre = sp.hhPre;
         int hsBuf_Idx = sp.hsBuf_Idx;
         double hsPre = sp.hsPre;
         double llPre = sp.llPre;
         double lsPre = sp.lsPre;
         double prevATR = sp.prevATR;
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = sp.lag1_inClose;
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < sp.lastP ) {
            tempReal = sp.cb_hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = sp.cb_llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf_Idx = hhBuf_Idx + 1;
         if( hhBuf_Idx > sp.maxIdx_hhBuf ) {
            hhBuf_Idx = 0;
         }
         band = sp.optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         highest = hsPre;
         lowest = lsPre;
         if( highStop > highest ) {
            highest = highStop;
         }
         if( lowStop < lowest ) {
            lowest = lowStop;
         }
         if( hsBuf_Idx == 0 ) {
            highest = highStop;
            lowest = lowStop;
         }
         hsPre = highest;
         lsPre = lowest;
         if( hsBuf_Idx < sp.lastQ ) {
            tempReal = sp.cb_hsBuf[hsBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = sp.cb_lsBuf[hsBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hsBuf_Idx = hsBuf_Idx + 1;
         if( hsBuf_Idx > sp.maxIdx_hsBuf ) {
            hsBuf_Idx = 0;
         }
         cur_outHighStop = highest;
         cur_outLowStop = lowest;
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
      /** Highest of the recent first high stops; usually plotted as the short stop. */
      public double highStop;
      /** Lowest of the recent first low stops; usually plotted as the long stop. */
      public double lowStop;
   }
   private void ckspStepImpl( CkspStream sp, double inHigh, double inLow, double inClose )
   {
      int ip = 0;
      int iq = 0;
      double val2 = 0.0;
      double val3 = 0.0;
      double greatest = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      double tempReal = 0.0;
      double sufHi = 0.0;
      double sufLo = 0.0;
      double highest = 0.0;
      double lowest = 0.0;
      double band = 0.0;
      double highStop = 0.0;
      double lowStop = 0.0;
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      greatest = tempHT - tempLT;
      /* val1 */
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.prevATR = Math.fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
      highest = sp.hhPre;
      lowest = sp.llPre;
      if( tempHT > highest ) {
         highest = tempHT;
      }
      if( tempLT < lowest ) {
         lowest = tempLT;
      }
      if( sp.hhBuf_Idx == 0 ) {
         highest = tempHT;
         lowest = tempLT;
      }
      sp.hhPre = highest;
      sp.llPre = lowest;
      if( sp.hhBuf_Idx < sp.lastP ) {
         tempReal = sp.cb_hhBuf[sp.hhBuf_Idx + 1];
         if( tempReal > highest ) {
            highest = tempReal;
         }
         tempReal = sp.cb_llBuf[sp.hhBuf_Idx + 1];
         if( tempReal < lowest ) {
            lowest = tempReal;
         }
      }
      sp.cb_hhBuf[sp.hhBuf_Idx] = tempHT;
      sp.cb_llBuf[sp.hhBuf_Idx] = tempLT;
      sp.hhBuf_Idx = sp.hhBuf_Idx + 1;
      if( sp.hhBuf_Idx > sp.maxIdx_hhBuf ) {
         sp.hhBuf_Idx = 0;
      }
      band = sp.optInMultiplier * sp.prevATR;
      highStop = highest - band;
      lowStop = lowest + band;
      highest = sp.hsPre;
      lowest = sp.lsPre;
      if( highStop > highest ) {
         highest = highStop;
      }
      if( lowStop < lowest ) {
         lowest = lowStop;
      }
      if( sp.hsBuf_Idx == 0 ) {
         highest = highStop;
         lowest = lowStop;
      }
      sp.hsPre = highest;
      sp.lsPre = lowest;
      if( sp.hsBuf_Idx < sp.lastQ ) {
         tempReal = sp.cb_hsBuf[sp.hsBuf_Idx + 1];
         if( tempReal > highest ) {
            highest = tempReal;
         }
         tempReal = sp.cb_lsBuf[sp.hsBuf_Idx + 1];
         if( tempReal < lowest ) {
            lowest = tempReal;
         }
      }
      sp.cb_hsBuf[sp.hsBuf_Idx] = highStop;
      sp.cb_lsBuf[sp.hsBuf_Idx] = lowStop;
      sp.hsBuf_Idx = sp.hsBuf_Idx + 1;
      if( sp.hsBuf_Idx > sp.maxIdx_hsBuf ) {
         sp.hsBuf_Idx = 0;
      }
      sp.cur_outHighStop = highest;
      sp.cur_outLowStop = lowest;
      /* A completed block becomes its suffix extrema. Nothing this bar reads
       * them, so both passes stay below the output stores, where the peek
       * frame never runs them.
       */
      if( sp.hhBuf_Idx == 0 ) {
         sufHi = sp.cb_hhBuf[sp.lastP];
         sufLo = sp.cb_llBuf[sp.lastP];
         ip = sp.lastP;
         while( ip > 1 ) {
            ip -= 1;
            tempReal = sp.cb_hhBuf[ip];
            if( tempReal > sufHi ) {
               sufHi = tempReal;
            }
            sp.cb_hhBuf[ip] = sufHi;
            tempReal = sp.cb_llBuf[ip];
            if( tempReal < sufLo ) {
               sufLo = tempReal;
            }
            sp.cb_llBuf[ip] = sufLo;
         }
      }
      if( sp.hsBuf_Idx == 0 ) {
         sufHi = sp.cb_hsBuf[sp.lastQ];
         sufLo = sp.cb_lsBuf[sp.lastQ];
         iq = sp.lastQ;
         while( iq > 1 ) {
            iq -= 1;
            tempReal = sp.cb_hsBuf[iq];
            if( tempReal > sufHi ) {
               sufHi = tempReal;
            }
            sp.cb_hsBuf[iq] = sufHi;
            tempReal = sp.cb_lsBuf[iq];
            if( tempReal < sufLo ) {
               sufLo = tempReal;
            }
            sp.cb_lsBuf[iq] = sufLo;
         }
      }
      sp.lag1_inClose = inClose;
   }
   private RetCode ckspOpenImpl( CkspStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, int optInStopPeriod, MInteger outBegIdx, MInteger outNBElement, double outHighStop[], double outLowStop[], int outStride )
   {
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      double[] hsBuf;
      int hsBuf_Idx = 0;
      int maxIdx_hsBuf = (30)-1;
      double[] lsBuf;
      int lsBuf_Idx = 0;
      int maxIdx_lsBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int iq = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
      int lastQ = 0;
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
      double tempReal = 0;
      double hhPre = 0;
      double llPre = 0;
      double hsPre = 0;
      double lsPre = 0;
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      double highStop = 0;
      double lowStop = 0;
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ckspLookback(optInTimePeriod, optInMultiplier, optInStopPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Four rolling extrema, van Herk / Gil-Werman, one buffer each: the window
       * ending in slot j is the current block's prefix extremum joined with the
       * previous block's suffix extremum from slot j+1. A slot holds the raw value
       * while its block is filling and is turned into the suffix extremum, in
       * place, when the block completes; slot j+1 is always still the previous
       * block's when slot j is filled. The high and low sides share an index.
       *
       * The extrema are exact, so both outputs must stay bit-identical to
       * TA_MAX( TA_MAX(high) - x*TA_ATR ) and its mirror, whatever the block
       * phase; only the sign of a zero tied between +0.0 and -0.0 is free.
       */
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hhBuf = new double[optInTimePeriod];
      maxIdx_hhBuf = (optInTimePeriod)-1;
      hhBuf_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      llBuf = new double[optInTimePeriod];
      maxIdx_llBuf = (optInTimePeriod)-1;
      llBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hsBuf = new double[optInStopPeriod];
      maxIdx_hsBuf = (optInStopPeriod)-1;
      hsBuf_Idx = 0;
      if( optInStopPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      lsBuf = new double[optInStopPeriod];
      maxIdx_lsBuf = (optInStopPeriod)-1;
      lsBuf_Idx = 0;
      lastP = optInTimePeriod - 1;
      lastQ = optInStopPeriod - 1;
      /* The Average True Range is carried inline rather than taken from a call,
       * because the two stages advance together one bar at a time and a
       * whole-range buffer between them would not stream.
       *
       * The arithmetic order below is the bit-exactness contract with TA_ATR (do
       * not reorder): True Range from high-low, then the two previous-close
       * distances in that order; the seed summed from 0.0 over the first 'period'
       * True Ranges and divided once; the same two Wilder coefficients, wBeta
       * rounded and wAlpha derived from it, in one fused statement.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
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
      /* Skip the bars the lookback adds for the unstable period. Taking the count
       * from the lookback rather than naming the setting keeps the two from
       * disagreeing.
       */
      i = lookbackTotal - lastQ - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
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
      /* prevATR is now the Average True Range of bar today-1, the first bar with
       * a first stop. Its extreme window is one whole block, taken straight from
       * the input.
       */
      highest = inHigh[today - 1];
      lowest = inLow[today - 1];
      hhBuf[lastP] = highest;
      llBuf[lastP] = lowest;
      ip = lastP;
      while( ip > 0 ) {
         ip -= 1;
         tempHT = inHigh[today - 1 - lastP + ip];
         tempLT = inLow[today - 1 - lastP + ip];
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         hhBuf[ip] = highest;
         llBuf[ip] = lowest;
      }
      hhPre = highest;
      llPre = lowest;
      /* The multiple of the ATR is formed on its own, never fused into the
       * offset, so that each first stop is TA_MAX - x*TA_ATR (or its mirror) as a
       * caller composing the three functions would compute it.
       */
      band = optInMultiplier * prevATR;
      highStop = highest - band;
      lowStop = lowest + band;
      hsPre = highStop;
      lsPre = lowStop;
      hsBuf[hsBuf_Idx] = highStop;
      lsBuf[hsBuf_Idx] = lowStop;
      hsBuf_Idx++;
      if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
      /* Fill the first stop window, through startIdx inclusive. */
      while( today <= startIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         /* Keep this shape: the prefix extreme selected in a local, the
          * block-start override after it, one store. A store made only when the
          * bar sets a new extreme, or the override ahead of the select, compiles
          * to a data-dependent branch in the stream step, where the prefix lives
          * in the handle.
          */
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         if( highStop > hsPre ) {
            hsPre = highStop;
         }
         if( lowStop < lsPre ) {
            lsPre = lowStop;
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         today += 1;
      }
      /* The first stop window is one whole block as well: its extremes are the
       * prefix extremes, and the block is complete.
       */
      sufHi = hsBuf[lastQ];
      sufLo = lsBuf[lastQ];
      iq = lastQ;
      while( iq > 1 ) {
         iq -= 1;
         tempReal = hsBuf[iq];
         if( tempReal > sufHi ) {
            sufHi = tempReal;
         }
         hsBuf[iq] = sufHi;
         tempReal = lsBuf[iq];
         if( tempReal < sufLo ) {
            sufLo = tempReal;
         }
         lsBuf[iq] = sufLo;
      }
      outHighStop[0 * outStride] = hsPre;
      outLowStop[0 * outStride] = lsPre;
      outIdx = 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         highest = hhPre;
         lowest = llPre;
         if( tempHT > highest ) {
            highest = tempHT;
         }
         if( tempLT < lowest ) {
            lowest = tempLT;
         }
         if( hhBuf_Idx == 0 ) {
            highest = tempHT;
            lowest = tempLT;
         }
         hhPre = highest;
         llPre = lowest;
         if( hhBuf_Idx < lastP ) {
            tempReal = hhBuf[hhBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = llBuf[hhBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hhBuf[hhBuf_Idx] = tempHT;
         llBuf[hhBuf_Idx] = tempLT;
         hhBuf_Idx++;
         if( hhBuf_Idx > maxIdx_hhBuf ) { hhBuf_Idx = 0; }
         band = optInMultiplier * prevATR;
         highStop = highest - band;
         lowStop = lowest + band;
         highest = hsPre;
         lowest = lsPre;
         if( highStop > highest ) {
            highest = highStop;
         }
         if( lowStop < lowest ) {
            lowest = lowStop;
         }
         if( hsBuf_Idx == 0 ) {
            highest = highStop;
            lowest = lowStop;
         }
         hsPre = highest;
         lsPre = lowest;
         if( hsBuf_Idx < lastQ ) {
            tempReal = hsBuf[hsBuf_Idx + 1];
            if( tempReal > highest ) {
               highest = tempReal;
            }
            tempReal = lsBuf[hsBuf_Idx + 1];
            if( tempReal < lowest ) {
               lowest = tempReal;
            }
         }
         hsBuf[hsBuf_Idx] = highStop;
         lsBuf[hsBuf_Idx] = lowStop;
         hsBuf_Idx++;
         if( hsBuf_Idx > maxIdx_hsBuf ) { hsBuf_Idx = 0; }
         outHighStop[outIdx * outStride] = highest;
         outLowStop[outIdx * outStride] = lowest;
         /* A completed block becomes its suffix extrema. Nothing this bar reads
          * them, so both passes stay below the output stores, where the peek
          * frame never runs them.
          */
         if( hhBuf_Idx == 0 ) {
            sufHi = hhBuf[lastP];
            sufLo = llBuf[lastP];
            ip = lastP;
            while( ip > 1 ) {
               ip -= 1;
               tempReal = hhBuf[ip];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hhBuf[ip] = sufHi;
               tempReal = llBuf[ip];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               llBuf[ip] = sufLo;
            }
         }
         if( hsBuf_Idx == 0 ) {
            sufHi = hsBuf[lastQ];
            sufLo = lsBuf[lastQ];
            iq = lastQ;
            while( iq > 1 ) {
               iq -= 1;
               tempReal = hsBuf[iq];
               if( tempReal > sufHi ) {
                  sufHi = tempReal;
               }
               hsBuf[iq] = sufHi;
               tempReal = lsBuf[iq];
               if( tempReal < sufLo ) {
                  sufLo = tempReal;
               }
               lsBuf[iq] = sufLo;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_hhBuf = maxIdx_hhBuf + 1;
      if( capCb_hhBuf > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_llBuf = maxIdx_llBuf + 1;
      if( capCb_llBuf > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_hsBuf = maxIdx_hsBuf + 1;
      if( capCb_hsBuf > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      int capCb_lsBuf = maxIdx_lsBuf + 1;
      if( capCb_lsBuf > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMultiplier = optInMultiplier;
      sp.optInStopPeriod = optInStopPeriod;
      sp.lastP = lastP;
      sp.lastQ = lastQ;
      sp.prevATR = prevATR;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.hhPre = hhPre;
      sp.llPre = llPre;
      sp.hsPre = hsPre;
      sp.lsPre = lsPre;
      sp.hhBuf_Idx = hhBuf_Idx;
      sp.hsBuf_Idx = hsBuf_Idx;
      sp.maxIdx_hhBuf = maxIdx_hhBuf;
      sp.llBuf_Idx = llBuf_Idx;
      sp.maxIdx_llBuf = maxIdx_llBuf;
      sp.maxIdx_hsBuf = maxIdx_hsBuf;
      sp.lsBuf_Idx = lsBuf_Idx;
      sp.maxIdx_lsBuf = maxIdx_lsBuf;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_hhBuf = capCb_hhBuf;
      sp.cb_hhBuf = hhBuf;
      sp.cbSize_llBuf = capCb_llBuf;
      sp.cb_llBuf = llBuf;
      sp.cbSize_hsBuf = capCb_hsBuf;
      sp.cb_hsBuf = hsBuf;
      sp.cbSize_lsBuf = capCb_lsBuf;
      sp.cb_lsBuf = lsBuf;
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
