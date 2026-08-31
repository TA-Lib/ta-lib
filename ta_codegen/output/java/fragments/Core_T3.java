/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MHL      Matthew Lindblom
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  032003 MHL    Implementation of T3
 *  040503 MF     Adapt for compatibility with published code
 *                for TradeStation and Metastock.
 *                See "Smoothing Techniques For More Accurate Signals"
 *                from Tim Tillson in Stock&Commodities V16:1 Page 33-37
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Allow period of 1: output is an exact copy of the
 *                input, consistent with TA_MA (issues #48, #59). The
 *                natural math is only near-identity at period=1: the
 *                coefficients sum to 1 in real arithmetic but not in
 *                floating point (~1e-14 drift), so the copy is explicit.
 */

   /**
    * Number of leading input bars {@link Core#T3} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod EMA period for each of the six stages (default 5;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInVFactor Volume factor weighting the coefficients (0 = plain
    *        triple EMA, higher = more DEMA-like sharpening) (default 0.7; range 0..1;
    *        {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int T3_Lookback( int optInTimePeriod, double optInVFactor )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInVFactor == REAL_DEFAULT ) {
         optInVFactor = 7e-1;
      } else if( !(optInVFactor >= 0e0 && optInVFactor <= 1e0) ) {
         return -1;
      }
      return 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()] ;

   }
   RetCode T3_Impl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInTimePeriod,
                    double optInVFactor,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInVFactor == REAL_DEFAULT ) {
         optInVFactor = 7e-1;
      } else if( !(optInVFactor >= 0e0 && optInVFactor <= 1e0) ) {
         return RetCode.BadParam;
      }
      /* For an explanation of this function, please read:
       *
       * Magazine articles written by Tim Tillson
       *
       * Essentially, a T3 of time serie 't' is:
       *   EMA1(x,Period) = EMA(x,Period)
       *   EMA2(x,Period) = EMA(EMA1(x,Period),Period)
       *   GD(x,Period,vFactor) = (EMA1(x,Period)*(1+vFactor)) - (EMA2(x,Period)*vFactor)
       *   T3 = GD (GD ( GD(t, Period, vFactor), Period, vFactor), Period, vFactor);
       *
       * T3 offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a T3 with EMA3. Both are called "Triple EMA"
       * in the litterature.
       */
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      /* No smoothing at period of 1: the output is a copy of the input
       * (same convention as TA_MA for every MAType). Explicit because the
       * coefficients below sum to 1 only in real arithmetic; going through
       * the math would leave ~1e-14 floating-point drift on every value.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      /* Initialize e1 */
      tempReal = inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      /* Initialize e2 */
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      /* Write the first output */
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      /* Indicates to the caller the number of output
       * successfully calculated.
       */
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode T3_Impl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInTimePeriod,
                    double optInVFactor,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInVFactor == REAL_DEFAULT ) {
         optInVFactor = 7e-1;
      } else if( !(optInVFactor >= 0e0 && optInVFactor <= 1e0) ) {
         return RetCode.BadParam;
      }
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      tempReal = (double)inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += (double)inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      outIdx = 0;
      outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * (double)inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Tillson's T3: a low-lag moving average built from six chained EMAs,
    * combined via volume-factor-weighted coefficients. Not the same as EMA3,
    * despite both being called "triple EMA".
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2/(period+1); e1=EMA(x), e2=EMA(e1), ... e6=EMA(e5) (six chained EMAs).
    * v = vFactor: c1 = -v^3; c2 = 3(v^2 - c1); c3 = -6v^2 - 3(v - c1); c4 = 1 + 3v - c1 + 3v^2.
    * T3 = c1*e6 + c2*e5 + c3*e4 + c4*e3
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#T3_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to smooth.
    * @param optInTimePeriod EMA period for each of the six stages (default 5;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInVFactor Volume factor weighting the coefficients (0 = plain
    *        triple EMA, higher = more DEMA-like sharpening) (default 0.7; range 0..1;
    *        {@code -4e37} selects the default).
    * @param outReal T3 smoothed line. Must hold at least
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
    * @see Core#EMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#MA
    */
   public OutRange T3( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double optInVFactor,
                       double outReal[] )
   {
      requireIndexRange("T3", startIdx, endIdx);
      int guardStart = clampedStart("T3", startIdx, T3_Lookback(optInTimePeriod, optInVFactor));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("T3", "inReal", inReal, guardInLen);
      requireLength("T3", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = T3_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("T3", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Tillson's T3: a low-lag moving average built from six chained EMAs,
    * combined via volume-factor-weighted coefficients. Not the same as EMA3,
    * despite both being called "triple EMA".
    * <p><b>Formula</b>
    * <pre>{@code
    * k = 2/(period+1); e1=EMA(x), e2=EMA(e1), ... e6=EMA(e5) (six chained EMAs).
    * v = vFactor: c1 = -v^3; c2 = 3(v^2 - c1); c3 = -6v^2 - 3(v - c1); c4 = 1 + 3v - c1 + 3v^2.
    * T3 = c1*e6 + c2*e5 + c3*e4 + c4*e3
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#T3_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to smooth.
    * @param optInTimePeriod EMA period for each of the six stages (default 5;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInVFactor Volume factor weighting the coefficients (0 = plain
    *        triple EMA, higher = more DEMA-like sharpening) (default 0.7; range 0..1;
    *        {@code -4e37} selects the default).
    * @param outReal T3 smoothed line. Must hold at least
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
    * @see Core#EMA
    * @see Core#DEMA
    * @see Core#TEMA
    * @see Core#MA
    */
   public OutRange T3( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double optInVFactor,
                       double outReal[] )
   {
      requireIndexRange("T3", startIdx, endIdx);
      int guardStart = clampedStart("T3", startIdx, T3_Lookback(optInTimePeriod, optInVFactor));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("T3", "inReal", inReal, guardInLen);
      requireLength("T3", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = T3_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("T3", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live T3 stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#T3} over the same series.
    * Open with {@link Core#t3Open}; there is no close — the handle is
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
   public static final class T3Stream {
      Core core;
      int optInTimePeriod;
      double optInVFactor;
      double k;
      double one_minus_k;
      double e1;
      double e2;
      double e3;
      double e4;
      double e5;
      double e6;
      double c1;
      double c2;
      double c3;
      double c4;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      T3Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#T3} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      T3Stream( T3Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInVFactor = other.optInVFactor;
         this.k = other.k;
         this.one_minus_k = other.one_minus_k;
         this.e1 = other.e1;
         this.e2 = other.e2;
         this.e3 = other.e3;
         this.e4 = other.e4;
         this.e5 = other.e5;
         this.e6 = other.e6;
         this.c1 = other.c1;
         this.c2 = other.c2;
         this.c3 = other.c3;
         this.c4 = other.c4;
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
            throw new TaLibArgumentException("T3 update: BadParam", RetCode.BadParam);
         }
         core.t3StepImpl(this, inReal);
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
         requireArgument("T3 updateAndFill", "inReal", inReal);
         requireArgument("T3 updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("T3 updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("T3 updateAndFill: BadParam", RetCode.BadParam);
            }
            core.t3StepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("T3 peek: BadParam", RetCode.BadParam);
         T3Stream sp = this;
         double cur_outReal = sp.cur_outReal;
         double e1 = sp.e1;
         double e2 = sp.e2;
         double e3 = sp.e3;
         double e4 = sp.e4;
         double e5 = sp.e5;
         double e6 = sp.e6;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         e1 = Math.fma(sp.one_minus_k, e1, sp.k * inReal);
         e2 = Math.fma(sp.one_minus_k, e2, sp.k * e1);
         e3 = Math.fma(sp.one_minus_k, e3, sp.k * e2);
         e4 = Math.fma(sp.one_minus_k, e4, sp.k * e3);
         e5 = Math.fma(sp.one_minus_k, e5, sp.k * e4);
         e6 = Math.fma(sp.one_minus_k, e6, sp.k * e5);
         cur_outReal = Math.fma(sp.c4, e3, Math.fma(sp.c3, e4, Math.fma(sp.c1, e6, sp.c2 * e5)));
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
      public T3Stream clone() {
         return new T3Stream(this);
      }
   }
   void t3StepImpl( T3Stream sp, double inReal )
   {
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      sp.e1 = Math.fma(sp.one_minus_k, sp.e1, sp.k * inReal);
      sp.e2 = Math.fma(sp.one_minus_k, sp.e2, sp.k * sp.e1);
      sp.e3 = Math.fma(sp.one_minus_k, sp.e3, sp.k * sp.e2);
      sp.e4 = Math.fma(sp.one_minus_k, sp.e4, sp.k * sp.e3);
      sp.e5 = Math.fma(sp.one_minus_k, sp.e5, sp.k * sp.e4);
      sp.e6 = Math.fma(sp.one_minus_k, sp.e6, sp.k * sp.e5);
      sp.cur_outReal = Math.fma(sp.c4, sp.e3, Math.fma(sp.c3, sp.e4, Math.fma(sp.c1, sp.e6, sp.c2 * sp.e5)));
   }
   private RetCode t3OpenImpl( T3Stream sp, double inReal[], int startIdx, int optInTimePeriod, double optInVFactor, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int lookbackTotal = 0;
      int today = 0;
      int i = 0;
      double k = 0;
      double one_minus_k = 0;
      double e1 = 0;
      double e2 = 0;
      double e3 = 0;
      double e4 = 0;
      double e5 = 0;
      double e6 = 0;
      double c1 = 0;
      double c2 = 0;
      double c3 = 0;
      double c4 = 0;
      double tempReal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInVFactor == REAL_DEFAULT ) {
         optInVFactor = 7e-1;
      } else if( !(optInVFactor >= 0e0 && optInVFactor <= 1e0) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = T3_Lookback(optInTimePeriod, optInVFactor);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInVFactor = optInVFactor;
         sp.k = 0.0;
         sp.one_minus_k = 0.0;
         sp.e1 = 0.0;
         sp.e2 = 0.0;
         sp.e3 = 0.0;
         sp.e4 = 0.0;
         sp.e5 = 0.0;
         sp.e6 = 0.0;
         sp.c1 = 0.0;
         sp.c2 = 0.0;
         sp.c3 = 0.0;
         sp.c4 = 0.0;
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      /* For an explanation of this function, please read:
       *
       * Magazine articles written by Tim Tillson
       *
       * Essentially, a T3 of time serie 't' is:
       *   EMA1(x,Period) = EMA(x,Period)
       *   EMA2(x,Period) = EMA(EMA1(x,Period),Period)
       *   GD(x,Period,vFactor) = (EMA1(x,Period)*(1+vFactor)) - (EMA2(x,Period)*vFactor)
       *   T3 = GD (GD ( GD(t, Period, vFactor), Period, vFactor), Period, vFactor);
       *
       * T3 offers a moving average with less lags then the
       * traditional EMA.
       *
       * Do not confuse a T3 with EMA3. Both are called "Triple EMA"
       * in the litterature.
       */
      lookbackTotal = 6 * (optInTimePeriod - 1) + this.unstablePeriod[FuncUnstId.T3.ordinal()];
      if( startIdx <= lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outNBElement.value = 0;
         outBegIdx.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      k = 2.0 / (optInTimePeriod + 1.0);
      one_minus_k = 1.0 - k;
      /* Initialize e1 */
      tempReal = inReal[today++];
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         tempReal += inReal[today++];
      }
      e1 = tempReal / optInTimePeriod;
      /* Initialize e2 */
      tempReal = e1;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         tempReal += e1;
      }
      e2 = tempReal / optInTimePeriod;
      /* Initialize e3 */
      tempReal = e2;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         tempReal += e2;
      }
      e3 = tempReal / optInTimePeriod;
      /* Initialize e4 */
      tempReal = e3;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         tempReal += e3;
      }
      e4 = tempReal / optInTimePeriod;
      /* Initialize e5 */
      tempReal = e4;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         tempReal += e4;
      }
      e5 = tempReal / optInTimePeriod;
      /* Initialize e6 */
      tempReal = e5;
      for( i = optInTimePeriod - 1; i > 0; i -= 1 ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         tempReal += e5;
      }
      e6 = tempReal / optInTimePeriod;
      /* Skip the unstable period */
      while( today <= startIdx ) {
         /* Do the calculation but do not write the output */
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
      }
      /* Calculate the constants */
      tempReal = optInVFactor * optInVFactor;
      c1 = 0 - tempReal * optInVFactor;
      c2 = 3.0 * (tempReal - c1);
      c3 = (0 - 6.0) * tempReal - 3.0 * (optInVFactor - c1);
      c4 = Math.fma(3.0, tempReal, Math.fma(3.0, optInVFactor, 1.0) - c1);
      /* Write the first output */
      outIdx = 0;
      outReal[outIdx++ * outStride] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      /* Calculate and output the remaining of the range. */
      while( today <= endIdx ) {
         e1 = Math.fma(one_minus_k, e1, k * inReal[today++]);
         e2 = Math.fma(one_minus_k, e2, k * e1);
         e3 = Math.fma(one_minus_k, e3, k * e2);
         e4 = Math.fma(one_minus_k, e4, k * e3);
         e5 = Math.fma(one_minus_k, e5, k * e4);
         e6 = Math.fma(one_minus_k, e6, k * e5);
         outReal[outIdx++ * outStride] = Math.fma(c4, e3, Math.fma(c3, e4, Math.fma(c1, e6, c2 * e5)));
      }
      /* Indicates to the caller the number of output
       * successfully calculated.
       */
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInVFactor = optInVFactor;
      sp.k = k;
      sp.one_minus_k = one_minus_k;
      sp.e1 = e1;
      sp.e2 = e2;
      sp.e3 = e3;
      sp.e4 = e4;
      sp.e5 = e5;
      sp.e6 = e6;
      sp.c1 = c1;
      sp.c2 = c2;
      sp.c3 = c3;
      sp.c4 = c4;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* t3OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   T3Stream t3OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInVFactor, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      T3Stream sp = new T3Stream(this);
      RetCode retCode = t3OpenImpl(sp, inReal, startIdx, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("T3 openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("T3 openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("T3 openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind t3Open (composition seam). */
   T3Stream t3OpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInVFactor )
   {
      T3Stream sp = new T3Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = t3OpenImpl(sp, inReal, startIdx, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("T3 open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("T3 open: internal error", retCode);
      }
      throw new TaLibArgumentException("T3 open: " + retCode, retCode);
   }
   /**
    * Open a live T3 stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#T3} at that bar.
    * <p>The history must hold at least {@code T3_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public T3Stream t3Open( double inReal[], int optInTimePeriod, double optInVFactor )
   {
      requireArgument("T3 open", "inReal", inReal);
      requireHistory("T3 open", inReal.length);
      return t3OpenInternal(inReal, 0, optInTimePeriod, optInVFactor);
   }
   /**
    * {@link Core#t3Open} that also fills the output array(s) bit-identically
    * to {@link Core#T3} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link T3Stream#outRange()}.
    */
   public T3Stream t3OpenAndFill( double inReal[], int optInTimePeriod, double optInVFactor, double outReal[] )
   {
      requireArgument("T3 openAndFill", "inReal", inReal);
      requireHistory("T3 openAndFill", inReal.length);
      int guardOutLen = openFillCount("T3 openAndFill", inReal.length, T3_Lookback(optInTimePeriod, optInVFactor));
      requireLength("T3 openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("T3 openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return t3OpenAndFillInternal(inReal, 0, optInTimePeriod, optInVFactor, outBegIdx, outNBElement, outReal);
   }
