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
 *  092826 MF,CC  First version (issue #464).
 *  100226 MF,CC  #497. An odd period is refused before the range is written.
 *  100726 MF,CC  #492. The Auto rule grows with the period and stops at the
 *                slowest alpha's bound.
 */

   /**
    * Number of leading input bars {@link Core#frama} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Number of bars in the window, split into two equal
    *        halves (default 16; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int framaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 16;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int root;
      root = (int)Math.sqrt((double)optInTimePeriod);
      /* The range check cannot demand an even period; without this the lookback
       * answers a usable number for a call that cannot run.
       */
      if( optInTimePeriod % 2 != 0 ) {
         return -1 ;
      }
      return optInTimePeriod + this.unstableCount(FuncUnstId.FRAMA.ordinal(), ((9 * (4 + 4) * (root + 2) / 2 < 99 * 10) ? 9 * (4 + 4) * (root + 2) / 2 : 99 * 10), ((9 * (8 + 4) * (root + 2) / 2 < 99 * 19) ? 9 * (8 + 4) * (root + 2) / 2 : 99 * 19)) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#frama}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of bars in the window, split into two equal
    *        halves (default 16; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int framaDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( framaLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode framaImpl( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double expScale = 0;
      double[] slot_sufHigh;
      double[] slot_sufLow;
      double[] slot_oldHigh;
      double[] slot_oldLow;
      int slot_Idx = 0;
      int maxIdx_slot = (32)-1;
      double tmpHigh = 0;
      double tmpLow = 0;
      double preHigh = 0;
      double preLow = 0;
      double hi1 = 0;
      double lo1 = 0;
      double hi2 = 0;
      double lo2 = 0;
      double r1 = 0;
      double r2 = 0;
      double r = 0;
      double price = 0;
      double alpha = 0;
      double prevFRAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int half = 0;
      int lastSlot = 0;
      int seedIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 16;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      expScale = -4.6 / 0.6931471805599453;
      /* -4.6/ln(2): alpha = exp(-4.6*(D-1)) with D-1 = log2((R1+R2)/R). */
      /* Id, Type, Static Size */
      if( optInTimePeriod % 2 != 0 ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = framaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      half = optInTimePeriod / 2;
      lastSlot = half - 1;
      /* van Herk / Gil-Werman over blocks of `half` bars. Slot j holds the current
       * block's bar j (or, past the running position, the previous block's suffix
       * extremes), and in oldHigh/oldLow the newer half's extremes from `half` bars
       * ago, which are the older half's extremes now: R2[t] = R1[t-half].
       *
       * Every slot store must stay below the output store: that is what keeps the
       * stream's Peek read-only and O(1). The slots hold copies, so outReal may
       * alias an input.
       */
      if( half < 1 ) return RetCode.INTERNAL_ERROR;
      slot_sufHigh = new double[half];
      slot_sufLow = new double[half];
      slot_oldHigh = new double[half];
      slot_oldLow = new double[half];
      maxIdx_slot = (half)-1;
      slot_Idx = 0;
      today = startIdx - lookbackTotal + 1;
      seedIdx = startIdx - (lookbackTotal - optInTimePeriod) - 1;
      /* The first block's suffix reads must see a bar inside the window. */
      i = 0;
      while( i < half ) {
         slot_sufHigh[i] = inHigh[today];
         slot_sufLow[i] = inLow[today];
         slot_oldHigh[i] = inHigh[today];
         slot_oldLow[i] = inLow[today];
         i += 1;
      }
      preHigh = 0.0;
      preLow = 0.0;
      /* Through seedIdx only the blocks are built: the seed discards every value
       * before it, so no alpha is computed there.
       */
      while( today <= seedIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      prevFRAMA = (inHigh[seedIdx] + inLow[seedIdx]) / 2.0;
      outIdx = 0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         price = (tmpHigh + tmpLow) / 2.0;
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         hi2 = slot_oldHigh[slot_Idx];
         lo2 = slot_oldLow[slot_Idx];
         r1 = hi1 - lo1;
         r2 = hi2 - lo2;
         r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
         /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
          * clamped or flat bar returns the price with no transcendental. Keep the
          * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
          * returns the price exactly too.
          */
         if( r1 > 0.0 && r2 > 0.0 && r1 + r2 > r ) {
            alpha = Math.exp(expScale * Math.log((r1 + r2) / r));
            prevFRAMA = Math.fma(1.0 - alpha, prevFRAMA, alpha * price);
         } else {
            prevFRAMA = price;
         }
         if( today >= startIdx ) {
            outReal[outIdx++] = prevFRAMA;
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode framaImpl( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double expScale = 0;
      double[] slot_sufHigh;
      double[] slot_sufLow;
      double[] slot_oldHigh;
      double[] slot_oldLow;
      int slot_Idx = 0;
      int maxIdx_slot = (32)-1;
      double tmpHigh = 0;
      double tmpLow = 0;
      double preHigh = 0;
      double preLow = 0;
      double hi1 = 0;
      double lo1 = 0;
      double hi2 = 0;
      double lo2 = 0;
      double r1 = 0;
      double r2 = 0;
      double r = 0;
      double price = 0;
      double alpha = 0;
      double prevFRAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int half = 0;
      int lastSlot = 0;
      int seedIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 16;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      expScale = -4.6 / 0.6931471805599453;
      if( optInTimePeriod % 2 != 0 ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = framaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      half = optInTimePeriod / 2;
      lastSlot = half - 1;
      if( half < 1 ) return RetCode.INTERNAL_ERROR;
      slot_sufHigh = new double[half];
      slot_sufLow = new double[half];
      slot_oldHigh = new double[half];
      slot_oldLow = new double[half];
      maxIdx_slot = (half)-1;
      slot_Idx = 0;
      today = startIdx - lookbackTotal + 1;
      seedIdx = startIdx - (lookbackTotal - optInTimePeriod) - 1;
      i = 0;
      while( i < half ) {
         slot_sufHigh[i] = (double)inHigh[today];
         slot_sufLow[i] = (double)inLow[today];
         slot_oldHigh[i] = (double)inHigh[today];
         slot_oldLow[i] = (double)inLow[today];
         i += 1;
      }
      preHigh = 0.0;
      preLow = 0.0;
      while( today <= seedIdx ) {
         tmpHigh = (double)inHigh[today];
         tmpLow = (double)inLow[today];
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      prevFRAMA = ((double)inHigh[seedIdx] + (double)inLow[seedIdx]) / 2.0;
      outIdx = 0;
      while( today <= endIdx ) {
         tmpHigh = (double)inHigh[today];
         tmpLow = (double)inLow[today];
         price = (tmpHigh + tmpLow) / 2.0;
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         hi2 = slot_oldHigh[slot_Idx];
         lo2 = slot_oldLow[slot_Idx];
         r1 = hi1 - lo1;
         r2 = hi2 - lo2;
         r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
         if( r1 > 0.0 && r2 > 0.0 && r1 + r2 > r ) {
            alpha = Math.exp(expScale * Math.log((r1 + r2) / r));
            prevFRAMA = Math.fma(1.0 - alpha, prevFRAMA, alpha * price);
         } else {
            prevFRAMA = price;
         }
         if( today >= startIdx ) {
            outReal[outIdx++] = prevFRAMA;
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing
    * factor adapts each bar to the fractal dimension of the window, estimated
    * from the high-low ranges of the window and of its two halves. A straight
    * run gives dimension 1 and the output follows the price; dense congestion
    * gives dimension 2 and very slow smoothing.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/frama">ta-lib.org/functions/frama</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Where either half of the window is flat, alpha is 1 and the output is the price; Ehlers' listing keeps the previous bar's dimension there instead.</li>
    * <li>The period must be even; an odd period is rejected.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#framaLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the window, split into two equal
    *        halves (default 16; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Adaptive moving average line. Must hold at least
    *        {@code endIdx - max(startIdx, framaLookback(...)) + 1} values, and never
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#kama
    * @see Core#mama
    * @see Core#medprice
    */
   public OutRange frama( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("FRAMA", startIdx, endIdx);
      int guardStart = clampedStart("FRAMA", startIdx, framaLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FRAMA", "inHigh", inHigh, guardInLen);
      requireLength("FRAMA", "inLow", inLow, guardInLen);
      requireLength("FRAMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = framaImpl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("FRAMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing
    * factor adapts each bar to the fractal dimension of the window, estimated
    * from the high-low ranges of the window and of its two halves. A straight
    * run gives dimension 1 and the output follows the price; dense congestion
    * gives dimension 2 and very slow smoothing.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/frama">ta-lib.org/functions/frama</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Where either half of the window is flat, alpha is 1 and the output is the price; Ehlers' listing keeps the previous bar's dimension there instead.</li>
    * <li>The period must be even; an odd period is rejected.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#framaLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the window, split into two equal
    *        halves (default 16; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Adaptive moving average line. Must hold at least
    *        {@code endIdx - max(startIdx, framaLookback(...)) + 1} values, and never
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#kama
    * @see Core#mama
    * @see Core#medprice
    */
   public OutRange frama( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("FRAMA", startIdx, endIdx);
      int guardStart = clampedStart("FRAMA", startIdx, framaLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FRAMA", "inHigh", inHigh, guardInLen);
      requireLength("FRAMA", "inLow", inLow, guardInLen);
      requireLength("FRAMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = framaImpl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("FRAMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live FRAMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#frama} over the same series.
    * Open with {@link Core#framaOpen}; there is no close — the handle is
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
   public static final class FramaStream {
      private Core core;
      private int optInTimePeriod;
      private double expScale;
      private double preHigh;
      private double preLow;
      private double prevFRAMA;
      private int lastSlot;
      private int slot_Idx;
      private int maxIdx_slot;
      private int cbSize_slot;
      private double[] cb_slot_sufHigh;
      private double[] cb_slot_sufLow;
      private double[] cb_slot_oldHigh;
      private double[] cb_slot_oldLow;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private FramaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#frama} reports over the same bars: the
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
            throw failure("FRAMA advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private FramaStream( FramaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.expScale = other.expScale;
         this.preHigh = other.preHigh;
         this.preLow = other.preLow;
         this.prevFRAMA = other.prevFRAMA;
         this.lastSlot = other.lastSlot;
         this.slot_Idx = other.slot_Idx;
         this.maxIdx_slot = other.maxIdx_slot;
         this.cbSize_slot = other.cbSize_slot;
         this.cb_slot_sufHigh = other.cb_slot_sufHigh.clone();
         this.cb_slot_sufLow = other.cb_slot_sufLow.clone();
         this.cb_slot_oldHigh = other.cb_slot_oldHigh.clone();
         this.cb_slot_oldLow = other.cb_slot_oldLow.clone();
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
      public double update( double inHigh, double inLow ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("FRAMA update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("FRAMA update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.framaStepImpl(this, inHigh, inLow);
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("FRAMA peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         FramaStream sp = this;
         double tmpHigh = 0.0;
         double tmpLow = 0.0;
         double hi1 = 0.0;
         double lo1 = 0.0;
         double hi2 = 0.0;
         double lo2 = 0.0;
         double r1 = 0.0;
         double r2 = 0.0;
         double r = 0.0;
         double price = 0.0;
         double alpha = 0.0;
         double cur_outReal = 0.0;
         double preHigh = sp.preHigh;
         double preLow = sp.preLow;
         double prevFRAMA = sp.prevFRAMA;
         tmpHigh = inHigh;
         tmpLow = inLow;
         price = (tmpHigh + tmpLow) / 2.0;
         if( sp.slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( sp.slot_Idx == sp.lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = sp.cb_slot_sufHigh[sp.slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = sp.cb_slot_sufLow[sp.slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         hi2 = sp.cb_slot_oldHigh[sp.slot_Idx];
         lo2 = sp.cb_slot_oldLow[sp.slot_Idx];
         r1 = hi1 - lo1;
         r2 = hi2 - lo2;
         r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
         /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
          * clamped or flat bar returns the price with no transcendental. Keep the
          * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
          * returns the price exactly too.
          */
         if( r1 > 0.0 && r2 > 0.0 && r1 + r2 > r ) {
            alpha = Math.exp(sp.expScale * Math.log((r1 + r2) / r));
            prevFRAMA = Math.fma(1.0 - alpha, prevFRAMA, alpha * price);
         } else {
            prevFRAMA = price;
         }
         cur_outReal = prevFRAMA;
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
      public FramaStream clone() {
         return new FramaStream(this);
      }
   }
   private void framaStepImpl( FramaStream sp, double inHigh, double inLow )
   {
      double tmpHigh = 0.0;
      double tmpLow = 0.0;
      double hi1 = 0.0;
      double lo1 = 0.0;
      double hi2 = 0.0;
      double lo2 = 0.0;
      double r1 = 0.0;
      double r2 = 0.0;
      double r = 0.0;
      double price = 0.0;
      double alpha = 0.0;
      int i = 0;
      tmpHigh = inHigh;
      tmpLow = inLow;
      price = (tmpHigh + tmpLow) / 2.0;
      if( sp.slot_Idx == 0 ) {
         sp.preHigh = tmpHigh;
         sp.preLow = tmpLow;
      } else {
         if( tmpHigh > sp.preHigh ) {
            sp.preHigh = tmpHigh;
         }
         if( tmpLow < sp.preLow ) {
            sp.preLow = tmpLow;
         }
      }
      if( sp.slot_Idx == sp.lastSlot ) {
         hi1 = sp.preHigh;
         lo1 = sp.preLow;
      } else {
         hi1 = sp.cb_slot_sufHigh[sp.slot_Idx + 1];
         if( sp.preHigh > hi1 ) {
            hi1 = sp.preHigh;
         }
         lo1 = sp.cb_slot_sufLow[sp.slot_Idx + 1];
         if( sp.preLow < lo1 ) {
            lo1 = sp.preLow;
         }
      }
      hi2 = sp.cb_slot_oldHigh[sp.slot_Idx];
      lo2 = sp.cb_slot_oldLow[sp.slot_Idx];
      r1 = hi1 - lo1;
      r2 = hi2 - lo2;
      r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
      /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
       * clamped or flat bar returns the price with no transcendental. Keep the
       * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
       * returns the price exactly too.
       */
      if( r1 > 0.0 && r2 > 0.0 && r1 + r2 > r ) {
         alpha = Math.exp(sp.expScale * Math.log((r1 + r2) / r));
         sp.prevFRAMA = Math.fma(1.0 - alpha, sp.prevFRAMA, alpha * price);
      } else {
         sp.prevFRAMA = price;
      }
      sp.cur_outReal = sp.prevFRAMA;
      sp.cb_slot_oldHigh[sp.slot_Idx] = hi1;
      sp.cb_slot_oldLow[sp.slot_Idx] = lo1;
      sp.cb_slot_sufHigh[sp.slot_Idx] = tmpHigh;
      sp.cb_slot_sufLow[sp.slot_Idx] = tmpLow;
      if( sp.slot_Idx == sp.lastSlot ) {
         i = sp.lastSlot;
         while( i > 0 ) {
            i -= 1;
            if( sp.cb_slot_sufHigh[i + 1] > sp.cb_slot_sufHigh[i] ) {
               sp.cb_slot_sufHigh[i] = sp.cb_slot_sufHigh[i + 1];
            }
            if( sp.cb_slot_sufLow[i + 1] < sp.cb_slot_sufLow[i] ) {
               sp.cb_slot_sufLow[i] = sp.cb_slot_sufLow[i + 1];
            }
         }
      }
      sp.slot_Idx = sp.slot_Idx + 1;
      if( sp.slot_Idx > sp.maxIdx_slot ) {
         sp.slot_Idx = 0;
      }
   }
   private RetCode framaOpenImpl( FramaStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double expScale = 0;
      double[] slot_sufHigh;
      double[] slot_sufLow;
      double[] slot_oldHigh;
      double[] slot_oldLow;
      int slot_Idx = 0;
      int maxIdx_slot = (32)-1;
      double tmpHigh = 0;
      double tmpLow = 0;
      double preHigh = 0;
      double preLow = 0;
      double hi1 = 0;
      double lo1 = 0;
      double hi2 = 0;
      double lo2 = 0;
      double r1 = 0;
      double r2 = 0;
      double r = 0;
      double price = 0;
      double alpha = 0;
      double prevFRAMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int half = 0;
      int lastSlot = 0;
      int seedIdx = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 16;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      expScale = -4.6 / 0.6931471805599453;
      /* -4.6/ln(2): alpha = exp(-4.6*(D-1)) with D-1 = log2((R1+R2)/R). */
      /* Id, Type, Static Size */
      if( optInTimePeriod % 2 != 0 ) {
         return RetCode.BAD_PARAM ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = framaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      half = optInTimePeriod / 2;
      lastSlot = half - 1;
      /* van Herk / Gil-Werman over blocks of `half` bars. Slot j holds the current
       * block's bar j (or, past the running position, the previous block's suffix
       * extremes), and in oldHigh/oldLow the newer half's extremes from `half` bars
       * ago, which are the older half's extremes now: R2[t] = R1[t-half].
       *
       * Every slot store must stay below the output store: that is what keeps the
       * stream's Peek read-only and O(1). The slots hold copies, so outReal may
       * alias an input.
       */
      if( half < 1 ) return RetCode.INTERNAL_ERROR;
      slot_sufHigh = new double[half];
      slot_sufLow = new double[half];
      slot_oldHigh = new double[half];
      slot_oldLow = new double[half];
      maxIdx_slot = (half)-1;
      slot_Idx = 0;
      today = startIdx - lookbackTotal + 1;
      seedIdx = startIdx - (lookbackTotal - optInTimePeriod) - 1;
      /* The first block's suffix reads must see a bar inside the window. */
      i = 0;
      while( i < half ) {
         slot_sufHigh[i] = inHigh[today];
         slot_sufLow[i] = inLow[today];
         slot_oldHigh[i] = inHigh[today];
         slot_oldLow[i] = inLow[today];
         i += 1;
      }
      preHigh = 0.0;
      preLow = 0.0;
      /* Through seedIdx only the blocks are built: the seed discards every value
       * before it, so no alpha is computed there.
       */
      while( today <= seedIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      prevFRAMA = (inHigh[seedIdx] + inLow[seedIdx]) / 2.0;
      outIdx = 0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         price = (tmpHigh + tmpLow) / 2.0;
         if( slot_Idx == 0 ) {
            preHigh = tmpHigh;
            preLow = tmpLow;
         } else {
            if( tmpHigh > preHigh ) {
               preHigh = tmpHigh;
            }
            if( tmpLow < preLow ) {
               preLow = tmpLow;
            }
         }
         if( slot_Idx == lastSlot ) {
            hi1 = preHigh;
            lo1 = preLow;
         } else {
            hi1 = slot_sufHigh[slot_Idx + 1];
            if( preHigh > hi1 ) {
               hi1 = preHigh;
            }
            lo1 = slot_sufLow[slot_Idx + 1];
            if( preLow < lo1 ) {
               lo1 = preLow;
            }
         }
         hi2 = slot_oldHigh[slot_Idx];
         lo2 = slot_oldLow[slot_Idx];
         r1 = hi1 - lo1;
         r2 = hi2 - lo2;
         r = ((hi1 > hi2) ? hi1 : hi2) - ((lo1 < lo2) ? lo1 : lo2);
         /* R >= max(R1,R2) makes R1+R2 <= R exactly the alpha >= 1 clamp, so a
          * clamped or flat bar returns the price with no transcendental. Keep the
          * recursion as alpha*P + (1-alpha)*prev: a computed alpha of 1.0 then
          * returns the price exactly too.
          */
         if( r1 > 0.0 && r2 > 0.0 && r1 + r2 > r ) {
            alpha = Math.exp(expScale * Math.log((r1 + r2) / r));
            prevFRAMA = Math.fma(1.0 - alpha, prevFRAMA, alpha * price);
         } else {
            prevFRAMA = price;
         }
         if( today >= startIdx ) {
            outReal[outIdx++ * outStride] = prevFRAMA;
         }
         slot_oldHigh[slot_Idx] = hi1;
         slot_oldLow[slot_Idx] = lo1;
         slot_sufHigh[slot_Idx] = tmpHigh;
         slot_sufLow[slot_Idx] = tmpLow;
         if( slot_Idx == lastSlot ) {
            i = lastSlot;
            while( i > 0 ) {
               i -= 1;
               if( slot_sufHigh[i + 1] > slot_sufHigh[i] ) {
                  slot_sufHigh[i] = slot_sufHigh[i + 1];
               }
               if( slot_sufLow[i + 1] < slot_sufLow[i] ) {
                  slot_sufLow[i] = slot_sufLow[i + 1];
               }
            }
         }
         slot_Idx++;
         if( slot_Idx > maxIdx_slot ) { slot_Idx = 0; }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_slot = maxIdx_slot + 1;
      if( capCb_slot > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.expScale = expScale;
      sp.preHigh = preHigh;
      sp.preLow = preLow;
      sp.prevFRAMA = prevFRAMA;
      sp.lastSlot = lastSlot;
      sp.slot_Idx = slot_Idx;
      sp.maxIdx_slot = maxIdx_slot;
      sp.cbSize_slot = capCb_slot;
      sp.cb_slot_sufHigh = slot_sufHigh;
      sp.cb_slot_sufLow = slot_sufLow;
      sp.cb_slot_oldHigh = slot_oldHigh;
      sp.cb_slot_oldLow = slot_oldLow;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* framaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   FramaStream framaOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      FramaStream sp = new FramaStream(this);
      RetCode retCode = framaOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("FRAMA openAndFill", inHigh.length, startIdx, framaLookback(optInTimePeriod));
      }
      throw streamFailure("FRAMA openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind framaOpen (composition seam). */
   FramaStream framaOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      FramaStream sp = new FramaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = framaOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("FRAMA open", inHigh.length, startIdx, framaLookback(optInTimePeriod));
      }
      throw streamFailure("FRAMA open", retCode);
   }
   /**
    * Open a live FRAMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#frama} at that bar.
    * <p>The history must hold at least {@code framaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public FramaStream framaOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("FRAMA open", "inHigh", inHigh);
      requireHistory("FRAMA open", inHigh.length);
      requireArgument("FRAMA open", "inLow", inLow);
      requireHistoryLength("FRAMA open", "inLow", inLow.length, inHigh.length);
      return framaOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#framaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#frama} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link FramaStream#outRange()}.
    */
   public FramaStream framaOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("FRAMA openAndFill", "inHigh", inHigh);
      requireHistory("FRAMA openAndFill", inHigh.length);
      requireArgument("FRAMA openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("FRAMA openAndFill", inHigh.length, framaLookback(optInTimePeriod));
      requireHistoryLength("FRAMA openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("FRAMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw streamFailure("FRAMA openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return framaOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
