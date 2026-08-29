/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  122104 MF,CF  Fix#1089506 for out-of-bound access to ep_temp.
 *  082726 MF,CC  Answer a rejected minus_dm before reading ep_temp, not after:
 *                the read was of an uninitialised local.
 */

   /**
    * Number of leading input bars {@link Core#SAR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInAcceleration Step added to the acceleration factor on each new
    *        extreme point (default 0.02; minimum 0; {@code -4e37} selects the
    *        default).
    * @param optInMaximum Ceiling on the acceleration factor (default 0.2;
    *        minimum 0; {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SAR_Lookback( double optInAcceleration, double optInMaximum )
   {
      if( optInAcceleration == REAL_DEFAULT ) {
         optInAcceleration = 2e-2;
      } else if( !(optInAcceleration >= 0e0 && optInAcceleration <= REAL_MAX) ) {
         return -1;
      }
      if( optInMaximum == REAL_DEFAULT ) {
         optInMaximum = 2e-1;
      } else if( !(optInMaximum >= 0e0 && optInMaximum <= REAL_MAX) ) {
         return -1;
      }
      /* SAR always sacrify one price bar to establish the
       * initial extreme price.
       */
      return 1 ;

   }
   RetCode SAR_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double optInAcceleration,
                     double optInMaximum,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInAcceleration == REAL_DEFAULT ) {
         optInAcceleration = 2e-2;
      } else if( !(optInAcceleration >= 0e0 && optInAcceleration <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInMaximum == REAL_DEFAULT ) {
         optInMaximum = 2e-1;
      } else if( !(optInMaximum >= 0e0 && optInMaximum <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      /* > 0 indicates long. == 0 indicates short */
      /* Implementation of the SAR has been a little bit open to interpretation
       * since Wilder (the original author) did not define a precise algorithm
       * on how to bootstrap the algorithm. Take any existing software application
       * and you will see slight variation on how the algorithm was adapted.
       *
       * What is the initial trade direction? Long or short?
       * ===================================================
       * The interpretation of what should be the initial SAR values is
       * open to interpretation, particularly since the caller to the function
       * does not specify the initial direction of the trade.
       *
       * In TA-Lib, the following logic is used:
       *  - Calculate +DM and -DM between the first and
       *    second bar. The highest directional indication will
       *    indicate the assumed direction of the trade for the second
       *    price bar.
       *  - In the case of a tie between +DM and -DM,
       *    the direction is LONG by default.
       *
       * What is the initial "extreme point" and thus SAR?
       * =================================================
       * The following shows how different people took different approach:
       *  - Metastock use the first price bar high/low depending of
       *    the direction. No SAR is calculated for the first price
       *    bar.
       *  - Tradestation use the closing price of the second bar. No
       *    SAR are calculated for the first price bar.
       *  - Wilder (the original author) use the SIP from the
       *    previous trade (cannot be implement here since the
       *    direction and length of the previous trade is unknonw).
       *  - The Magazine TASC seems to follow Wilder approach which
       *    is not practical here.
       *
       * TA-Lib "consume" the first price bar and use its high/low as the
       * initial SAR of the second price bar. I found that approach to be
       * the closest to Wilders idea of having the first entry day use
       * the previous extreme point, except that here the extreme point is
       * derived solely from the first price bar. I found the same approach
       * to be used by Metastock.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       *
       * Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Make sure the acceleration and maximum are coherent.
       * If not, correct the acceleration.
       */
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      /* Identify if the initial direction is long or short.
       * (ep is just used as a temp buffer here, the name
       *  of the parameter is not significant).
       */
      OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
      tempInt.value = _xr0.begIdx();
      tempInt.value = _xr0.count();
      retCode = RetCode.Success;
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      /* Write the first SAR. */
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = inLow[todayIdx];
         sar = newHigh;
      }
      /* Cheat on the newLow and newHigh for the
       * first iteration.
       */
      newLow = inLow[todayIdx];
      newHigh = inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = inLow[todayIdx];
         newHigh = inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            /* Switch to short if the low penetrates the SAR value. */
            if( newLow <= sar ) {
               /* Switch and Overide the SAR with the ep */
               isLong = 0;
               sar = ep;
               /* Make sure the overide SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               /* Output the overide SAR */
               outReal[outIdx++] = sar;
               /* Adjust af and ep */
               af = optInAcceleration;
               ep = newLow;
               /* Calculate the new SAR */
               sar = Math.fma(af, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               /* No switch */
               /* Output the SAR (was calculated in the previous iteration) */
               outReal[outIdx++] = sar;
               /* Adjust af and ep. */
               if( newHigh > ep ) {
                  ep = newHigh;
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               /* Calculate the new SAR */
               sar = Math.fma(af, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         /* Switch to long if the high penetrates the SAR value. */
         } else if( newHigh >= sar ) {
            /* Switch and Overide the SAR with the ep */
            isLong = 1;
            sar = ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            /* Output the overide SAR */
            outReal[outIdx++] = sar;
            /* Adjust af and ep */
            af = optInAcceleration;
            ep = newHigh;
            /* Calculate the new SAR */
            sar = Math.fma(af, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            outReal[outIdx++] = sar;
            /* Adjust af and ep. */
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            /* Calculate the new SAR */
            sar = Math.fma(af, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode SAR_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     double optInAcceleration,
                     double optInMaximum,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInAcceleration == REAL_DEFAULT ) {
         optInAcceleration = 2e-2;
      } else if( !(optInAcceleration >= 0e0 && optInAcceleration <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInMaximum == REAL_DEFAULT ) {
         optInMaximum = 2e-1;
      } else if( !(optInMaximum >= 0e0 && optInMaximum <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
      tempInt.value = _xr0.begIdx();
      tempInt.value = _xr0.count();
      retCode = RetCode.Success;
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      todayIdx = startIdx;
      newHigh = (double)inHigh[todayIdx - 1];
      newLow = (double)inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = (double)inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = (double)inLow[todayIdx];
         sar = newHigh;
      }
      newLow = (double)inLow[todayIdx];
      newHigh = (double)inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = (double)inLow[todayIdx];
         newHigh = (double)inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            if( newLow <= sar ) {
               isLong = 0;
               sar = ep;
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               outReal[outIdx++] = sar;
               af = optInAcceleration;
               ep = newLow;
               sar = Math.fma(af, ep - sar, sar);
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               outReal[outIdx++] = sar;
               if( newHigh > ep ) {
                  ep = newHigh;
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               sar = Math.fma(af, ep - sar, sar);
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         } else if( newHigh >= sar ) {
            isLong = 1;
            sar = ep;
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            outReal[outIdx++] = sar;
            af = optInAcceleration;
            ep = newHigh;
            sar = Math.fma(af, ep - sar, sar);
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            outReal[outIdx++] = sar;
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            sar = Math.fma(af, ep - sar, sar);
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level
    * that accelerates toward price via an acceleration factor. Signals trend
    * direction and trailing exit points. SAR below price = uptrend (long); SAR
    * above price = downtrend (short). Price crossing SAR flips direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * SAR_next = SAR + af * (EP - SAR)
    * EP = extreme point (highest high in long / lowest low in short); af starts at Acceleration, += Acceleration each new EP, capped at Maximum.
    * On penetration: reverse, SAR := prior EP, reset af = Acceleration. SAR clamped each bar so it does not penetrate the prior/current bar's range.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SAR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInAcceleration Step added to the acceleration factor on each new
    *        extreme point (default 0.02; minimum 0; {@code -4e37} selects the
    *        default).
    * @param optInMaximum Ceiling on the acceleration factor (default 0.2;
    *        minimum 0; {@code -4e37} selects the default).
    * @param outReal Parabolic SAR stop/reverse level per bar. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SAREXT
    * @see Core#MINUS_DM
    * @see Core#PLUS_DM
    */
   public OutRange SAR( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double optInAcceleration,
                        double optInMaximum,
                        double outReal[] )
   {
      requireIndexRange("SAR", startIdx, endIdx);
      int guardStart = clampedStart("SAR", startIdx, SAR_Lookback(optInAcceleration, optInMaximum));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SAR", "inHigh", inHigh, guardInLen);
      requireLength("SAR", "inLow", inLow, guardInLen);
      requireLength("SAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SAR_Impl(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level
    * that accelerates toward price via an acceleration factor. Signals trend
    * direction and trailing exit points. SAR below price = uptrend (long); SAR
    * above price = downtrend (short). Price crossing SAR flips direction.
    * <p><b>Formula</b>
    * <pre>{@code
    * SAR_next = SAR + af * (EP - SAR)
    * EP = extreme point (highest high in long / lowest low in short); af starts at Acceleration, += Acceleration each new EP, capped at Maximum.
    * On penetration: reverse, SAR := prior EP, reset af = Acceleration. SAR clamped each bar so it does not penetrate the prior/current bar's range.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SAR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInAcceleration Step added to the acceleration factor on each new
    *        extreme point (default 0.02; minimum 0; {@code -4e37} selects the
    *        default).
    * @param optInMaximum Ceiling on the acceleration factor (default 0.2;
    *        minimum 0; {@code -4e37} selects the default).
    * @param outReal Parabolic SAR stop/reverse level per bar. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#SAREXT
    * @see Core#MINUS_DM
    * @see Core#PLUS_DM
    */
   public OutRange SAR( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        double optInAcceleration,
                        double optInMaximum,
                        double outReal[] )
   {
      requireIndexRange("SAR", startIdx, endIdx);
      int guardStart = clampedStart("SAR", startIdx, SAR_Lookback(optInAcceleration, optInMaximum));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SAR", "inHigh", inHigh, guardInLen);
      requireLength("SAR", "inLow", inLow, guardInLen);
      requireLength("SAR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SAR_Impl(startIdx, endIdx, inHigh, inLow, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("SAR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SAR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SAR} over the same series.
    * Open with {@link Core#sarOpen}; there is no close — the handle is
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
   public static final class SarStream {
      Core core;
      double optInAcceleration;
      double optInMaximum;
      int isLong;
      double newHigh;
      double newLow;
      double af;
      double ep;
      double sar;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      SarStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#SAR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      SarStream( SarStream other ) {
         this.core = other.core;
         this.optInAcceleration = other.optInAcceleration;
         this.optInMaximum = other.optInMaximum;
         this.isLong = other.isLong;
         this.newHigh = other.newHigh;
         this.newLow = other.newLow;
         this.af = other.af;
         this.ep = other.ep;
         this.sar = other.sar;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( SarStream other ) {
         this.core = other.core;
         this.optInAcceleration = other.optInAcceleration;
         this.optInMaximum = other.optInMaximum;
         this.isLong = other.isLong;
         this.newHigh = other.newHigh;
         this.newLow = other.newLow;
         this.af = other.af;
         this.ep = other.ep;
         this.sar = other.sar;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("SAR update: BadParam", RetCode.BadParam);
         core.sarStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
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
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("SAR updateAndFill", "inHigh", inHigh);
         requireArgument("SAR updateAndFill", "inLow", inLow);
         requireArgument("SAR updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("SAR updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("SAR updateAndFill: BadParam", RetCode.BadParam);
            core.sarStepImpl(this, inHigh[i], inLow[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("SAR peek: BadParam", RetCode.BadParam);
         SarStream scratch = new SarStream(this);
         core.sarStepImpl(scratch, inHigh, inLow);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public SarStream copy() {
         return new SarStream(this);
      }
   }
   void sarStepImpl( SarStream sp, double inHigh, double inLow )
   {
      double prevHigh = 0.0;
      double prevLow = 0.0;
      prevLow = sp.newLow;
      prevHigh = sp.newHigh;
      sp.newLow = inLow;
      sp.newHigh = inHigh;
      if( sp.isLong == 1 ) {
         /* Switch to short if the low penetrates the SAR value. */
         if( sp.newLow <= sp.sar ) {
            /* Switch and Overide the SAR with the ep */
            sp.isLong = 0;
            sp.sar = sp.ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar < prevHigh ) {
               sp.sar = prevHigh;
            }
            if( sp.sar < sp.newHigh ) {
               sp.sar = sp.newHigh;
            }
            /* Output the overide SAR */
            sp.cur_outReal = sp.sar;
            /* Adjust af and ep */
            sp.af = sp.optInAcceleration;
            sp.ep = sp.newLow;
            /* Calculate the new SAR */
            sp.sar = Math.fma(sp.af, sp.ep - sp.sar, sp.sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar < prevHigh ) {
               sp.sar = prevHigh;
            }
            if( sp.sar < sp.newHigh ) {
               sp.sar = sp.newHigh;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            sp.cur_outReal = sp.sar;
            /* Adjust af and ep. */
            if( sp.newHigh > sp.ep ) {
               sp.ep = sp.newHigh;
               sp.af += sp.optInAcceleration;
               if( sp.af > sp.optInMaximum ) {
                  sp.af = sp.optInMaximum;
               }
            }
            /* Calculate the new SAR */
            sp.sar = Math.fma(sp.af, sp.ep - sp.sar, sp.sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sp.sar > prevLow ) {
               sp.sar = prevLow;
            }
            if( sp.sar > sp.newLow ) {
               sp.sar = sp.newLow;
            }
         }
      /* Switch to long if the high penetrates the SAR value. */
      } else if( sp.newHigh >= sp.sar ) {
         /* Switch and Overide the SAR with the ep */
         sp.isLong = 1;
         sp.sar = sp.ep;
         /* Make sure the overide SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar > prevLow ) {
            sp.sar = prevLow;
         }
         if( sp.sar > sp.newLow ) {
            sp.sar = sp.newLow;
         }
         /* Output the overide SAR */
         sp.cur_outReal = sp.sar;
         /* Adjust af and ep */
         sp.af = sp.optInAcceleration;
         sp.ep = sp.newHigh;
         /* Calculate the new SAR */
         sp.sar = Math.fma(sp.af, sp.ep - sp.sar, sp.sar);
         /* Make sure the new SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar > prevLow ) {
            sp.sar = prevLow;
         }
         if( sp.sar > sp.newLow ) {
            sp.sar = sp.newLow;
         }
      } else {
         /* No switch */
         /* Output the SAR (was calculated in the previous iteration) */
         sp.cur_outReal = sp.sar;
         /* Adjust af and ep. */
         if( sp.newLow < sp.ep ) {
            sp.ep = sp.newLow;
            sp.af += sp.optInAcceleration;
            if( sp.af > sp.optInMaximum ) {
               sp.af = sp.optInMaximum;
            }
         }
         /* Calculate the new SAR */
         sp.sar = Math.fma(sp.af, sp.ep - sp.sar, sp.sar);
         /* Make sure the new SAR is within
          * yesterday's and today's range.
          */
         if( sp.sar < prevHigh ) {
            sp.sar = prevHigh;
         }
         if( sp.sar < sp.newHigh ) {
            sp.sar = sp.newHigh;
         }
      }
   }
   private RetCode sarOpenImpl( SarStream sp, double inHigh[], double inLow[], int startIdx, double optInAcceleration, double optInMaximum, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      RetCode retCode;
      int isLong = 0;
      int todayIdx = 0;
      int outIdx = 0;
      MInteger tempInt = new MInteger();
      double newHigh = 0;
      double newLow = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double af = 0;
      double ep = 0;
      double sar = 0;
      double[] ep_temp = new double[1];
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInAcceleration == REAL_DEFAULT ) {
         optInAcceleration = 2e-2;
      } else if( !(optInAcceleration >= 0e0 && optInAcceleration <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( optInMaximum == REAL_DEFAULT ) {
         optInMaximum = 2e-1;
      } else if( !(optInMaximum >= 0e0 && optInMaximum <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* > 0 indicates long. == 0 indicates short */
      /* Implementation of the SAR has been a little bit open to interpretation
       * since Wilder (the original author) did not define a precise algorithm
       * on how to bootstrap the algorithm. Take any existing software application
       * and you will see slight variation on how the algorithm was adapted.
       *
       * What is the initial trade direction? Long or short?
       * ===================================================
       * The interpretation of what should be the initial SAR values is
       * open to interpretation, particularly since the caller to the function
       * does not specify the initial direction of the trade.
       *
       * In TA-Lib, the following logic is used:
       *  - Calculate +DM and -DM between the first and
       *    second bar. The highest directional indication will
       *    indicate the assumed direction of the trade for the second
       *    price bar.
       *  - In the case of a tie between +DM and -DM,
       *    the direction is LONG by default.
       *
       * What is the initial "extreme point" and thus SAR?
       * =================================================
       * The following shows how different people took different approach:
       *  - Metastock use the first price bar high/low depending of
       *    the direction. No SAR is calculated for the first price
       *    bar.
       *  - Tradestation use the closing price of the second bar. No
       *    SAR are calculated for the first price bar.
       *  - Wilder (the original author) use the SIP from the
       *    previous trade (cannot be implement here since the
       *    direction and length of the previous trade is unknonw).
       *  - The Magazine TASC seems to follow Wilder approach which
       *    is not practical here.
       *
       * TA-Lib "consume" the first price bar and use its high/low as the
       * initial SAR of the second price bar. I found that approach to be
       * the closest to Wilders idea of having the first entry day use
       * the previous extreme point, except that here the extreme point is
       * derived solely from the first price bar. I found the same approach
       * to be used by Metastock.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       *
       * Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < 1 ) {
         startIdx = 1;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Make sure the acceleration and maximum are coherent.
       * If not, correct the acceleration.
       */
      af = optInAcceleration;
      if( af > optInMaximum ) {
         optInAcceleration = optInMaximum;
         af = optInAcceleration;
      }
      /* Identify if the initial direction is long or short.
       * (ep is just used as a temp buffer here, the name
       *  of the parameter is not significant).
       */
      OutRange _xr0 = MINUS_DM(startIdx, startIdx, inHigh, inLow, 1, ep_temp);
      tempInt.value = _xr0.begIdx();
      tempInt.value = _xr0.count();
      retCode = RetCode.Success;
      if( ep_temp[0] > 0 ) {
         isLong = 0;
      } else {
         isLong = 1;
      }
      outBegIdx.value = startIdx;
      outIdx = 0;
      /* Write the first SAR. */
      todayIdx = startIdx;
      newHigh = inHigh[todayIdx - 1];
      newLow = inLow[todayIdx - 1];
      if( isLong == 1 ) {
         ep = inHigh[todayIdx];
         sar = newLow;
      } else {
         ep = inLow[todayIdx];
         sar = newHigh;
      }
      /* Cheat on the newLow and newHigh for the
       * first iteration.
       */
      newLow = inLow[todayIdx];
      newHigh = inHigh[todayIdx];
      while( todayIdx <= endIdx ) {
         prevLow = newLow;
         prevHigh = newHigh;
         newLow = inLow[todayIdx];
         newHigh = inHigh[todayIdx];
         todayIdx += 1;
         if( isLong == 1 ) {
            /* Switch to short if the low penetrates the SAR value. */
            if( newLow <= sar ) {
               /* Switch and Overide the SAR with the ep */
               isLong = 0;
               sar = ep;
               /* Make sure the overide SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
               /* Output the overide SAR */
               outReal[outIdx++ * outStride] = sar;
               /* Adjust af and ep */
               af = optInAcceleration;
               ep = newLow;
               /* Calculate the new SAR */
               sar = Math.fma(af, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar < prevHigh ) {
                  sar = prevHigh;
               }
               if( sar < newHigh ) {
                  sar = newHigh;
               }
            } else {
               /* No switch */
               /* Output the SAR (was calculated in the previous iteration) */
               outReal[outIdx++ * outStride] = sar;
               /* Adjust af and ep. */
               if( newHigh > ep ) {
                  ep = newHigh;
                  af += optInAcceleration;
                  if( af > optInMaximum ) {
                     af = optInMaximum;
                  }
               }
               /* Calculate the new SAR */
               sar = Math.fma(af, ep - sar, sar);
               /* Make sure the new SAR is within
                * yesterday's and today's range.
                */
               if( sar > prevLow ) {
                  sar = prevLow;
               }
               if( sar > newLow ) {
                  sar = newLow;
               }
            }
         /* Switch to long if the high penetrates the SAR value. */
         } else if( newHigh >= sar ) {
            /* Switch and Overide the SAR with the ep */
            isLong = 1;
            sar = ep;
            /* Make sure the overide SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
            /* Output the overide SAR */
            outReal[outIdx++ * outStride] = sar;
            /* Adjust af and ep */
            af = optInAcceleration;
            ep = newHigh;
            /* Calculate the new SAR */
            sar = Math.fma(af, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar > prevLow ) {
               sar = prevLow;
            }
            if( sar > newLow ) {
               sar = newLow;
            }
         } else {
            /* No switch */
            /* Output the SAR (was calculated in the previous iteration) */
            outReal[outIdx++ * outStride] = sar;
            /* Adjust af and ep. */
            if( newLow < ep ) {
               ep = newLow;
               af += optInAcceleration;
               if( af > optInMaximum ) {
                  af = optInMaximum;
               }
            }
            /* Calculate the new SAR */
            sar = Math.fma(af, ep - sar, sar);
            /* Make sure the new SAR is within
             * yesterday's and today's range.
             */
            if( sar < prevHigh ) {
               sar = prevHigh;
            }
            if( sar < newHigh ) {
               sar = newHigh;
            }
         }
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInAcceleration = optInAcceleration;
      sp.optInMaximum = optInMaximum;
      sp.isLong = isLong;
      sp.newHigh = newHigh;
      sp.newLow = newLow;
      sp.af = af;
      sp.ep = ep;
      sp.sar = sar;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* sarOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SarStream sarOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, double optInAcceleration, double optInMaximum, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SarStream sp = new SarStream(this);
      RetCode retCode = sarOpenImpl(sp, inHigh, inLow, startIdx, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SAR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SAR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SAR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind sarOpen (composition seam). */
   SarStream sarOpenInternal( double inHigh[], double inLow[], int startIdx, double optInAcceleration, double optInMaximum )
   {
      SarStream sp = new SarStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = sarOpenImpl(sp, inHigh, inLow, startIdx, optInAcceleration, optInMaximum, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SAR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SAR open: internal error", retCode);
      }
      throw new TaLibArgumentException("SAR open: " + retCode, retCode);
   }
   /**
    * Open a live SAR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SAR} at that bar.
    * <p>The history must hold at least {@code SAR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SarStream sarOpen( double inHigh[], double inLow[], double optInAcceleration, double optInMaximum )
   {
      requireArgument("SAR open", "inHigh", inHigh);
      requireHistory("SAR open", inHigh.length);
      requireArgument("SAR open", "inLow", inLow);
      requireHistoryLength("SAR open", "inLow", inLow.length, inHigh.length);
      return sarOpenInternal(inHigh, inLow, 0, optInAcceleration, optInMaximum);
   }
   /**
    * {@link Core#sarOpen} that also fills the output array(s) bit-identically
    * to {@link Core#SAR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SarStream#outRange()}.
    */
   public SarStream sarOpenAndFill( double inHigh[], double inLow[], double optInAcceleration, double optInMaximum, double outReal[] )
   {
      requireArgument("SAR openAndFill", "inHigh", inHigh);
      requireHistory("SAR openAndFill", inHigh.length);
      requireArgument("SAR openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("SAR openAndFill", inHigh.length, SAR_Lookback(optInAcceleration, optInMaximum));
      requireHistoryLength("SAR openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("SAR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("SAR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return sarOpenAndFillInternal(inHigh, inLow, 0, optInAcceleration, optInMaximum, outBegIdx, outNBElement, outReal);
   }
