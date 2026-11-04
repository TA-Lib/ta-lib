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
 *  103126 MF,CC  Creation (#568).
 */

   /**
    * Number of leading input bars {@link Core#ce} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 22; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the lines (default 3; minimum 0; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ceLookback( int optInTimePeriod, double optInMultiplier )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 22;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return -1;
      }
      /* The ATR term is never restated here, which is what makes CE inherit
       * TA_FUNC_UNST_ATR. It is at least the extreme window's own lookback.
       */
      return atrLookback(optInTimePeriod) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#ce}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 22; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the lines (default 3; minimum 0; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ceDisplayShift( int optInTimePeriod, double optInMultiplier, int outputIdx )
   {
      if( ceLookback( optInTimePeriod, optInMultiplier ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode ceImpl( int startIdx,
                   int endIdx,
                   double inHigh[],
                   double inLow[],
                   double inClose[],
                   int optInTimePeriod,
                   double optInMultiplier,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outRealLong[],
                   double outRealShort[] )
   {
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
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
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 22;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( outRealLong == outRealShort ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ceLookback(optInTimePeriod, optInMultiplier);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      /* Two rolling extrema, van Herk / Gil-Werman, one buffer each: the window
       * ending in slot j is the current block's prefix extremum joined with the
       * previous block's suffix extremum from slot j+1. A slot holds the raw value
       * while its block is filling and is turned into the suffix extremum, in
       * place, when the block completes; slot j+1 is always still the previous
       * block's when slot j is filled. The high and low sides share an index.
       *
       * The extrema are exact, so both outputs must stay bit-identical to
       * TA_MAX(high) - x*TA_ATR and its mirror, whatever the block phase; only
       * the sign of a zero tied between +0.0 and -0.0 is free.
       */
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hhBuf = new double[optInTimePeriod];
      maxIdx_hhBuf = (optInTimePeriod)-1;
      hhBuf_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      llBuf = new double[optInTimePeriod];
      maxIdx_llBuf = (optInTimePeriod)-1;
      llBuf_Idx = 0;
      lastP = optInTimePeriod - 1;
      /* The Average True Range is carried inline rather than taken from a call,
       * so that the extreme window and the average advance together one bar at
       * a time and the body streams.
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
      i = lookbackTotal - optInTimePeriod;
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
      /* prevATR is now the Average True Range of bar today-1, the first output
       * bar. Its extreme window is one whole block, taken straight from the
       * input.
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
       * offset, so that each line is TA_MAX - x*TA_ATR (or its mirror) as a
       * caller composing the three functions would compute it.
       */
      band = optInMultiplier * prevATR;
      outRealLong[0] = highest - band;
      outRealShort[0] = lowest + band;
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
         band = optInMultiplier * prevATR;
         outRealLong[outIdx] = highest - band;
         outRealShort[outIdx] = lowest + band;
         /* A completed block becomes its suffix extrema. Nothing this bar reads
          * them, so the pass stays below the output stores, where the peek
          * frame never runs it.
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
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode ceImpl( int startIdx,
                   int endIdx,
                   float inHigh[],
                   float inLow[],
                   float inClose[],
                   int optInTimePeriod,
                   double optInMultiplier,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outRealLong[],
                   double outRealShort[] )
   {
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
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
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 22;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( outRealLong == outRealShort ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ceLookback(optInTimePeriod, optInMultiplier);
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
      lastP = optInTimePeriod - 1;
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
      i = lookbackTotal - optInTimePeriod;
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
      outRealLong[0] = highest - band;
      outRealShort[0] = lowest + band;
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
         outRealLong[outIdx] = highest - band;
         outRealShort[outIdx] = lowest + band;
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
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Le Beau's Chandelier Exit, a two-line volatility stop. The highest high of
    * the window is offset down, and the lowest low up, by a multiple of the
    * Average True Range, so each line hangs from the recent extreme at a
    * distance that widens as the true range grows. The long line is the exit
    * for a long position and hangs below the recent highs; the short line is
    * the mirror for a short position, above the recent lows.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ce">ta-lib.org/functions/ce</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The extremes are taken over the last TimePeriod bars. Le Beau anchors the highest high at the entry of the trade, which a function over a price series cannot know; the rolling window is what charting platforms plot under this name.</li>
    * <li>The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.</li>
    * <li>Both lines are emitted on every bar, each from the highs or the lows. A popular variant takes the extremes of the closes and keeps a single stop that only moves toward price until the trend flips; that variant is not this function.</li>
    * <li>Neither line is always on its side of the other: with a small multiplier the long line sits above the short one.</li>
    * <li>Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * <li>The two lines are the Chande Kroll Stop with a stop period of 1, {@code outRealLong} being CKSP's {@code outHighStop}.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ceLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 22; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the lines (default 3; minimum 0; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outRealLong Highest high less the multiple of the Average True
    *        Range; the stop for a long position. Must hold at least
    *        {@code endIdx - max(startIdx, ceLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outRealShort Lowest low plus the multiple of the Average True
    *        Range; the stop for a short position. Must hold at least
    *        {@code endIdx - max(startIdx, ceLookback(...)) + 1} values, and never be
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#atr
    * @see Core#max
    * @see Core#min
    * @see Core#cksp
    * @see Core#supertrend
    * @see Core#kc
    * @see Core#sar
    */
   public OutRange ce( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInTimePeriod,
                       double optInMultiplier,
                       double outRealLong[],
                       double outRealShort[] )
   {
      requireIndexRange("CE", startIdx, endIdx);
      int guardStart = clampedStart("CE", startIdx, ceLookback(optInTimePeriod, optInMultiplier));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CE", "inHigh", inHigh, guardInLen);
      requireLength("CE", "inLow", inLow, guardInLen);
      requireLength("CE", "inClose", inClose, guardInLen);
      requireLength("CE", "outRealLong", outRealLong, guardOutLen);
      requireLength("CE", "outRealShort", outRealShort, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ceImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Le Beau's Chandelier Exit, a two-line volatility stop. The highest high of
    * the window is offset down, and the lowest low up, by a multiple of the
    * Average True Range, so each line hangs from the recent extreme at a
    * distance that widens as the true range grows. The long line is the exit
    * for a long position and hangs below the recent highs; the short line is
    * the mirror for a short position, above the recent lows.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ce">ta-lib.org/functions/ce</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The extremes are taken over the last TimePeriod bars. Le Beau anchors the highest high at the entry of the trade, which a function over a price series cannot know; the rolling window is what charting platforms plot under this name.</li>
    * <li>The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.</li>
    * <li>Both lines are emitted on every bar, each from the highs or the lows. A popular variant takes the extremes of the closes and keeps a single stop that only moves toward price until the trend flips; that variant is not this function.</li>
    * <li>Neither line is always on its side of the other: with a small multiplier the long line sits above the short one.</li>
    * <li>Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * <li>The two lines are the Chande Kroll Stop with a stop period of 1, {@code outRealLong} being CKSP's {@code outHighStop}.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ceLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Window of the highest high and lowest low, and
    *        smoothing period of the Average True Range (default 22; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to
    *        offset the lines (default 3; minimum 0; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outRealLong Highest high less the multiple of the Average True
    *        Range; the stop for a long position. Must hold at least
    *        {@code endIdx - max(startIdx, ceLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outRealShort Lowest low plus the multiple of the Average True
    *        Range; the stop for a short position. Must hold at least
    *        {@code endIdx - max(startIdx, ceLookback(...)) + 1} values, and never be
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#atr
    * @see Core#max
    * @see Core#min
    * @see Core#cksp
    * @see Core#supertrend
    * @see Core#kc
    * @see Core#sar
    */
   public OutRange ce( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInTimePeriod,
                       double optInMultiplier,
                       double outRealLong[],
                       double outRealShort[] )
   {
      requireIndexRange("CE", startIdx, endIdx);
      int guardStart = clampedStart("CE", startIdx, ceLookback(optInTimePeriod, optInMultiplier));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CE", "inHigh", inHigh, guardInLen);
      requireLength("CE", "inLow", inLow, guardInLen);
      requireLength("CE", "inClose", inClose, guardInLen);
      requireLength("CE", "outRealLong", outRealLong, guardOutLen);
      requireLength("CE", "outRealShort", outRealShort, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ceImpl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ce} over the same series.
    * Open with {@link Core#ceOpen}; there is no close — the handle is
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
   public static final class CeStream {
      private Core core;
      private int optInTimePeriod;
      private double optInMultiplier;
      private int lastP;
      private double prevATR;
      private double wAlpha;
      private double wBeta;
      private double hhPre;
      private double llPre;
      private int hhBuf_Idx;
      private int maxIdx_hhBuf;
      private int llBuf_Idx;
      private int maxIdx_llBuf;
      private double lag1_inClose;
      private int cbSize_hhBuf;
      private double[] cb_hhBuf;
      private int cbSize_llBuf;
      private double[] cb_llBuf;
      private double cur_outRealLong;
      private double cur_outRealShort;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CeStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ce} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(CeOut)} keeps answering the previous
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
            throw failure("CE advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CeStream( CeStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMultiplier = other.optInMultiplier;
         this.lastP = other.lastP;
         this.prevATR = other.prevATR;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.hhPre = other.hhPre;
         this.llPre = other.llPre;
         this.hhBuf_Idx = other.hhBuf_Idx;
         this.maxIdx_hhBuf = other.maxIdx_hhBuf;
         this.llBuf_Idx = other.llBuf_Idx;
         this.maxIdx_llBuf = other.maxIdx_llBuf;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_hhBuf = other.cbSize_hhBuf;
         this.cb_hhBuf = other.cb_hhBuf.clone();
         this.cbSize_llBuf = other.cbSize_llBuf;
         this.cb_llBuf = other.cb_llBuf.clone();
         this.cur_outRealLong = other.cur_outRealLong;
         this.cur_outRealShort = other.cur_outRealShort;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(CeOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, CeOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("CE update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("CE update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CE update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.ceStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         out.realLong = this.cur_outRealLong;
         out.realShort = this.cur_outRealShort;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, CeOut out ) {
         requireArgument("CE peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("CE peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         CeStream sp = this;
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
         double cur_outRealLong = 0.0;
         double cur_outRealShort = 0.0;
         int hhBuf_Idx = sp.hhBuf_Idx;
         double hhPre = sp.hhPre;
         double llPre = sp.llPre;
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
         cur_outRealLong = highest - band;
         cur_outRealShort = lowest + band;
         out.realLong = cur_outRealLong;
         out.realShort = cur_outRealShort;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( CeOut out ) {
         requireArgument("CE value", "out", out);
         out.realLong = this.cur_outRealLong;
         out.realShort = this.cur_outRealShort;
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
      public CeStream clone() {
         return new CeStream(this);
      }
   }

   /**
    * The outputs of one CE bar, written by the stream into an object the
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
   public static final class CeOut {
      /** Highest high less the multiple of the Average True Range; the stop for a long position. */
      public double realLong;
      /** Lowest low plus the multiple of the Average True Range; the stop for a short position. */
      public double realShort;
   }
   private void ceStepImpl( CeStream sp, double inHigh, double inLow, double inClose )
   {
      int ip = 0;
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
      /* Keep this shape: the prefix extreme selected in a local, the
       * block-start override after it, one store. A store made only when the
       * bar sets a new extreme, or the override ahead of the select, compiles
       * to a data-dependent branch in the stream step, where the prefix lives
       * in the handle.
       */
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
      sp.cur_outRealLong = highest - band;
      sp.cur_outRealShort = lowest + band;
      /* A completed block becomes its suffix extrema. Nothing this bar reads
       * them, so the pass stays below the output stores, where the peek
       * frame never runs it.
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
      sp.lag1_inClose = inClose;
   }
   private RetCode ceOpenImpl( CeStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, MInteger outBegIdx, MInteger outNBElement, double outRealLong[], double outRealShort[], int outStride )
   {
      double[] hhBuf;
      int hhBuf_Idx = 0;
      int maxIdx_hhBuf = (30)-1;
      double[] llBuf;
      int llBuf_Idx = 0;
      int maxIdx_llBuf = (30)-1;
      int i = 0;
      int ip = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int lastP = 0;
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
      double sufHi = 0;
      double sufLo = 0;
      double highest = 0;
      double lowest = 0;
      double band = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 22;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ceLookback(optInTimePeriod, optInMultiplier);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Two rolling extrema, van Herk / Gil-Werman, one buffer each: the window
       * ending in slot j is the current block's prefix extremum joined with the
       * previous block's suffix extremum from slot j+1. A slot holds the raw value
       * while its block is filling and is turned into the suffix extremum, in
       * place, when the block completes; slot j+1 is always still the previous
       * block's when slot j is filled. The high and low sides share an index.
       *
       * The extrema are exact, so both outputs must stay bit-identical to
       * TA_MAX(high) - x*TA_ATR and its mirror, whatever the block phase; only
       * the sign of a zero tied between +0.0 and -0.0 is free.
       */
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      hhBuf = new double[optInTimePeriod];
      maxIdx_hhBuf = (optInTimePeriod)-1;
      hhBuf_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      llBuf = new double[optInTimePeriod];
      maxIdx_llBuf = (optInTimePeriod)-1;
      llBuf_Idx = 0;
      lastP = optInTimePeriod - 1;
      /* The Average True Range is carried inline rather than taken from a call,
       * so that the extreme window and the average advance together one bar at
       * a time and the body streams.
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
      i = lookbackTotal - optInTimePeriod;
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
      /* prevATR is now the Average True Range of bar today-1, the first output
       * bar. Its extreme window is one whole block, taken straight from the
       * input.
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
       * offset, so that each line is TA_MAX - x*TA_ATR (or its mirror) as a
       * caller composing the three functions would compute it.
       */
      band = optInMultiplier * prevATR;
      outRealLong[0 * outStride] = highest - band;
      outRealShort[0 * outStride] = lowest + band;
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
         band = optInMultiplier * prevATR;
         outRealLong[outIdx * outStride] = highest - band;
         outRealShort[outIdx * outStride] = lowest + band;
         /* A completed block becomes its suffix extrema. Nothing this bar reads
          * them, so the pass stays below the output stores, where the peek
          * frame never runs it.
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
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMultiplier = optInMultiplier;
      sp.lastP = lastP;
      sp.prevATR = prevATR;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.hhPre = hhPre;
      sp.llPre = llPre;
      sp.hhBuf_Idx = hhBuf_Idx;
      sp.maxIdx_hhBuf = maxIdx_hhBuf;
      sp.llBuf_Idx = llBuf_Idx;
      sp.maxIdx_llBuf = maxIdx_llBuf;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_hhBuf = capCb_hhBuf;
      sp.cb_hhBuf = hhBuf;
      sp.cbSize_llBuf = capCb_llBuf;
      sp.cb_llBuf = llBuf;
      sp.cur_outRealLong = outRealLong[(outNBElement.value - 1) * outStride];
      sp.cur_outRealShort = outRealShort[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* ceOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CeStream ceOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, MInteger outBegIdx, MInteger outNBElement, double outRealLong[], double outRealShort[] )
   {
      CeStream sp = new CeStream(this);
      RetCode retCode = ceOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CE openAndFill", inHigh.length, startIdx, ceLookback(optInTimePeriod, optInMultiplier));
      }
      throw streamFailure("CE openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ceOpen (composition seam). */
   CeStream ceOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier )
   {
      CeStream sp = new CeStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealLong = new double[1];
      double[] sink_outRealShort = new double[1];
      RetCode retCode = ceOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, sink_outRealLong, sink_outRealShort, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CE open", inHigh.length, startIdx, ceLookback(optInTimePeriod, optInMultiplier));
      }
      throw streamFailure("CE open", retCode);
   }
   /**
    * Open a live CE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ce} at that bar.
    * <p>The history must hold at least {@code ceLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link InsufficientHistoryException} too, and a null argument
    * {@link IllegalArgumentException}, both ahead of everything above.
    */
   public CeStream ceOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier )
   {
      requireArgument("CE open", "inHigh", inHigh);
      requireHistory("CE open", inHigh.length);
      requireArgument("CE open", "inLow", inLow);
      requireArgument("CE open", "inClose", inClose);
      requireHistoryLength("CE open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CE open", "inClose", inClose.length, inHigh.length);
      return ceOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier);
   }
   /**
    * {@link Core#ceOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ce} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CeStream#outRange()}.
    */
   public CeStream ceOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier, double outRealLong[], double outRealShort[] )
   {
      requireArgument("CE openAndFill", "inHigh", inHigh);
      requireHistory("CE openAndFill", inHigh.length);
      requireArgument("CE openAndFill", "inLow", inLow);
      requireArgument("CE openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CE openAndFill", inHigh.length, ceLookback(optInTimePeriod, optInMultiplier));
      requireHistoryLength("CE openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CE openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("CE openAndFill", "outRealLong", outRealLong, guardOutLen);
      requireLength("CE openAndFill", "outRealShort", outRealShort, guardOutLen);
      if( (Object)outRealLong == (Object)inHigh || (Object)outRealLong == (Object)inLow || (Object)outRealLong == (Object)inClose || (Object)outRealShort == (Object)inHigh || (Object)outRealShort == (Object)inLow || (Object)outRealShort == (Object)inClose || (Object)outRealLong == (Object)outRealShort ) {
         throw streamFailure("CE openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ceOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outRealLong, outRealShort);
   }
