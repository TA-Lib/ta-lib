/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091526 KL     First version (proposal-drafts issue #74).
 */

   /**
    * Number of leading input bars {@link Core#cti} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing values correlated against the
    *        ramp (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ctiLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode ctiImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumX = 0;
      double sumX2 = 0;
      double sumXY = 0;
      double x = 0;
      double trailingX = 0;
      double leavingX = 0;
      double shift = 0;
      double ssX = 0;
      double spXY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      double dPeriod = 0;
      double sumY = 0;
      double ssY = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int j = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      /* The ramp side is data-independent and collapses to two constants. With y
       * running 0..optInTimePeriod-1 as BARS AGO,
       *
       *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
       *
       * Both are computed in double because their integer forms overflow: at the
       * top of the parameter range n*n*n is 1e15, far past what an int holds, and
       * the product would wrap silently rather than fail. ssY is strictly positive
       * for every n >= 2, which is why only the price side can make a window
       * degenerate.
       */
      sumY = dPeriod * (dPeriod - 1.0) * 0.5;
      ssY = dPeriod * (dPeriod * dPeriod - 1.0) / 12.0;
      /* Measure the price side against a shift near the window, as correl.c does
       * (#242). Transcribing the author's listing literally would carry
       * n*Sxx - Sx*Sx on raw price levels, which is the quantity that returned 0,
       * -1 and -1.73 from a perfectly correlated pair: MEASURED on the card, the
       * naive form errs by 5.7e-03 at a 1e2 price level with a 1e-5 spread, on an
       * indicator whose entire range is [-1, +1].
       *
       * Anchor on the first window value here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      shift = inReal[trailingIdx];
      /* The initial window, less its last bar. This bar's y is startIdx-j, since
       * the first output is computed with `today` at startIdx.
       */
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = inReal[j] - shift;
         sumX += x;
         sumX2 += x * x;
         sumXY += x * (double)(startIdx - j);
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      do {
         /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
          * leaves sumXY alone.
          */
         x = inReal[today] - shift;
         sumX += x;
         sumX2 += x * x;
         ssX = sumX2 - sumX * sumX * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         /* Re-anchor and rebuild when the shift has gone stale: the price sum of
          * squares has shrunk below 1e-6 of the squared deviations it is extracted
          * from; OR the value the PREVIOUS bar removed sat so far from the shift
          * that its squared term dwarfs what remains; OR at least every 32
          * windows. Same three triggers as correl.c, watching the price side only
          * -- the ramp side is exact constants and cannot drift.
          *
          * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
          * with no linear trend, and reseeding on it would rebuild on every bar of
          * ordinary sideways data.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || leavingX > 1000000.0 * sumX2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            sumXY = 0.0;
            sumX2 = sumXY;
            sumX = sumX2;
            for( j = windowStart; j <= today; j += 1 ) {
               x = inReal[j] - shift;
               sumX += x;
               sumX2 += x * x;
               sumXY += x * (double)(today - j);
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforced here rather than at the
             * divide, exactly as correl.c does: a negative ssX always reseeds on
             * the same bar, so the divide below can rely on it being >= 0.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
         }
         /* Save the trailing value before writing the output, since the input and
          * output might be the same array.
          */
         trailingX = inReal[trailingIdx] - shift;
         trailingIdx += 1;
         /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
          * series correlates NEGATIVELY with it. Ehlers' listing counts the same
          * way and takes Y = -count, which is the positively-sloped line the
          * indicator is defined against; negating the coefficient once is the same
          * thing, and it keeps the O(1) slide identity below written the way
          * linearreg.c:102-114 states it. Dropping this negation silently inverts
          * the whole indicator -- Pearson r is odd in either variable -- and a
          * magnitude or |r| assertion cannot see it.
          *
          * ssY is a positive constant, so only ssX can make the window degenerate.
          * It is tested against its own scale rather than an absolute band: the
          * product carries the fourth power of the window's spread, so a fixed
          * threshold rejects a well-defined correlation as soon as the data is
          * small. The product is tested on its own because neither factor's test
          * implies it -- at that fourth power it can underflow to exactly 0.0
          * while both are still ordinary normals, and a zero divisor there gives
          * NaN, which the clamp does not catch.
          *
          * An all-flat window therefore emits exactly 0.0 rather than NaN, which
          * is correl.c's precedent and what #112 requires of a successful call.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssX * ssY > 0.0 ) {
            tempReal = (0 - spXY) / Math.sqrt(ssX * ssY);
            /* A correlation coefficient cannot leave [-1,1]; rounding in the
             * three sums can still put it a few ulp outside.
             */
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            } else if( tempReal < 0 - 1.0 ) {
               tempReal = 0 - 1.0;
            }
            outReal[outIdx++] = tempReal;
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Slide the window one bar in O(1). Advancing it ages every retained
          * value by one bar, which raises each of their weights by 1 and so adds
          * the whole deviation sum; the departing value leaves at the full weight
          * optInTimePeriod-1 and also loses its place in that sum, which is the
          * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
          * and sumX must still be the COMPLETE window's sum when it is read here
          * -- it is decremented on the line below, not above.
          */
         leavingX = trailingX * trailingX;
         sumXY = sumXY + sumX - dPeriod * trailingX;
         sumX -= trailingX;
         sumX2 -= leavingX;
         today += 1;
      } while( today <= endIdx );
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode ctiImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double sumX = 0;
      double sumX2 = 0;
      double sumXY = 0;
      double x = 0;
      double trailingX = 0;
      double leavingX = 0;
      double shift = 0;
      double ssX = 0;
      double spXY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      double dPeriod = 0;
      double sumY = 0;
      double ssY = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int j = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      sumY = dPeriod * (dPeriod - 1.0) * 0.5;
      ssY = dPeriod * (dPeriod * dPeriod - 1.0) / 12.0;
      shift = (double)inReal[trailingIdx];
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = (double)inReal[j] - shift;
         sumX += x;
         sumX2 += x * x;
         sumXY += x * (double)(startIdx - j);
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      do {
         x = (double)inReal[today] - shift;
         sumX += x;
         sumX2 += x * x;
         ssX = sumX2 - sumX * sumX * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || leavingX > 1000000.0 * sumX2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += (double)inReal[j];
            }
            shift = tempReal * invPeriod;
            sumXY = 0.0;
            sumX2 = sumXY;
            sumX = sumX2;
            for( j = windowStart; j <= today; j += 1 ) {
               x = (double)inReal[j] - shift;
               sumX += x;
               sumX2 += x * x;
               sumXY += x * (double)(today - j);
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
         }
         trailingX = (double)inReal[trailingIdx] - shift;
         trailingIdx += 1;
         if( ssX > 0.00000000000001 * sumX2 && ssX * ssY > 0.0 ) {
            tempReal = (0 - spXY) / Math.sqrt(ssX * ssY);
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            } else if( tempReal < 0 - 1.0 ) {
               tempReal = 0 - 1.0;
            }
            outReal[outIdx++] = tempReal;
         } else {
            outReal[outIdx++] = 0.0;
         }
         leavingX = trailingX * trailingX;
         sumXY = sumXY + sumX - dPeriod * trailingX;
         sumX -= trailingX;
         sumX2 -= leavingX;
         today += 1;
      } while( today <= endIdx );
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of
    * the last {@code optInTimePeriod} closes against a straight line of
    * positive slope. Bounded in -1..+1 by construction — {@code +1} is a
    * perfectly linear uptrend, {@code -1} a perfectly linear downtrend,
    * {@code 0} no linear trend. Trend strength and direction in one bounded
    * number, with no smoothing, no recursion and no filter coefficients.
    * Arithmetically it is <a
    * href="https://ta-lib.org/functions/correl">{@code CORREL}</a> of the
    * series against a ramp, with the ramp side collapsed to closed-form
    * constants.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cti">ta-lib.org/functions/cti</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>The ramp's direction is the whole sign of the indicator.</b> This implementation carries {@code y} as <i>bars ago</i>, which runs backward in time, so a rising series correlates negatively with it and the coefficient is negated once at the output. Ehlers' own listing counts the same way and takes {@code Y = -count}, which is the same thing. Getting this wrong inverts the indicator completely rather than perturbing it, because Pearson's {@code r} is odd in either variable — and no magnitude or {@code |r|} assertion can see it. The bars-ago orientation is kept because the O(1) window slide is written for it.</li>
    * <li><b>The sums are taken against a shift, not on raw price levels.</b> Transcribing the published listing literally would compute {@code n·Σx² − (Σx)²} on the levels themselves, which is the cancellation that made {@code CORREL} return {@code 0}, {@code -1} and {@code -1.73} from perfectly correlated inputs. MEASURED: at a price level of 1e2 with a 1e-5 spread the naive form errs by 5.7e-03 absolute — on an indicator whose entire range is 2 wide — against 1.6e-10 for the shift-and-reseed form.</li>
    * <li><b>There is a conditioning floor, and it is not a defect.</b> Once the window's spread falls below roughly 1e-8 of its level, the input doubles no longer carry the answer and no re-anchoring can recover it. That regime is a property of the input, not of this function.</li>
    * <li><b>A window with no spread returns exactly {@code 0.0}.</b> Only the price side can degenerate — the ramp's sum of squares is a positive constant for every {@code n ≥ 2} — and the answer follows {@code CORREL}'s precedent rather than the author's listing, which holds the previous value. Holding would make this function path-dependent; returning NaN from a successful call is not permitted. Implementations disagree here: the listing holds, {@code CORREL} gives {@code 0}, pandas gives {@code NaN}, Pine gives {@code na}.</li>
    * <li><b>The result is clamped into -1..+1</b>, as {@code CORREL} is: rounding in three sums can put a coefficient a few ulp outside its own range.</li>
    * <li>{@code optInTimePeriod} starts at 2, not 1: at {@code n = 1} the closed form {@code n²(n²−1)/12} is identically zero and every window is degenerate.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ctiLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure the trend of.
    * @param optInTimePeriod Number of trailing values correlated against the
    *        ramp (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Correlation against the ramp, in -1..+1. Must hold at least
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
    * @see Core#correl
    * @see Core#linearregSlope
    * @see Core#vhf
    */
   public OutRange cti( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CTI", startIdx, endIdx);
      int guardStart = clampedStart("CTI", startIdx, ctiLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CTI", "inReal", inReal, guardInLen);
      requireLength("CTI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ctiImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CTI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of
    * the last {@code optInTimePeriod} closes against a straight line of
    * positive slope. Bounded in -1..+1 by construction — {@code +1} is a
    * perfectly linear uptrend, {@code -1} a perfectly linear downtrend,
    * {@code 0} no linear trend. Trend strength and direction in one bounded
    * number, with no smoothing, no recursion and no filter coefficients.
    * Arithmetically it is <a
    * href="https://ta-lib.org/functions/correl">{@code CORREL}</a> of the
    * series against a ramp, with the ramp side collapsed to closed-form
    * constants.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cti">ta-lib.org/functions/cti</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>The ramp's direction is the whole sign of the indicator.</b> This implementation carries {@code y} as <i>bars ago</i>, which runs backward in time, so a rising series correlates negatively with it and the coefficient is negated once at the output. Ehlers' own listing counts the same way and takes {@code Y = -count}, which is the same thing. Getting this wrong inverts the indicator completely rather than perturbing it, because Pearson's {@code r} is odd in either variable — and no magnitude or {@code |r|} assertion can see it. The bars-ago orientation is kept because the O(1) window slide is written for it.</li>
    * <li><b>The sums are taken against a shift, not on raw price levels.</b> Transcribing the published listing literally would compute {@code n·Σx² − (Σx)²} on the levels themselves, which is the cancellation that made {@code CORREL} return {@code 0}, {@code -1} and {@code -1.73} from perfectly correlated inputs. MEASURED: at a price level of 1e2 with a 1e-5 spread the naive form errs by 5.7e-03 absolute — on an indicator whose entire range is 2 wide — against 1.6e-10 for the shift-and-reseed form.</li>
    * <li><b>There is a conditioning floor, and it is not a defect.</b> Once the window's spread falls below roughly 1e-8 of its level, the input doubles no longer carry the answer and no re-anchoring can recover it. That regime is a property of the input, not of this function.</li>
    * <li><b>A window with no spread returns exactly {@code 0.0}.</b> Only the price side can degenerate — the ramp's sum of squares is a positive constant for every {@code n ≥ 2} — and the answer follows {@code CORREL}'s precedent rather than the author's listing, which holds the previous value. Holding would make this function path-dependent; returning NaN from a successful call is not permitted. Implementations disagree here: the listing holds, {@code CORREL} gives {@code 0}, pandas gives {@code NaN}, Pine gives {@code na}.</li>
    * <li><b>The result is clamped into -1..+1</b>, as {@code CORREL} is: rounding in three sums can put a coefficient a few ulp outside its own range.</li>
    * <li>{@code optInTimePeriod} starts at 2, not 1: at {@code n = 1} the closed form {@code n²(n²−1)/12} is identically zero and every window is degenerate.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ctiLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure the trend of.
    * @param optInTimePeriod Number of trailing values correlated against the
    *        ramp (default 20; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Correlation against the ramp, in -1..+1. Must hold at least
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
    * @see Core#correl
    * @see Core#linearregSlope
    * @see Core#vhf
    */
   public OutRange cti( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CTI", startIdx, endIdx);
      int guardStart = clampedStart("CTI", startIdx, ctiLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CTI", "inReal", inReal, guardInLen);
      requireLength("CTI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ctiImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CTI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CTI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cti} over the same series.
    * Open with {@link Core#ctiOpen}; there is no close — the handle is
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
   public static final class CtiStream {
      private Core core;
      private int optInTimePeriod;
      private double sumX;
      private double sumX2;
      private double sumXY;
      private double leavingX;
      private double shift;
      private double invPeriod;
      private double dPeriod;
      private double sumY;
      private double ssY;
      private int lookbackTotal;
      private int trailingIdx;
      private int barsSinceReseed;
      private int j;
      private int today;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CtiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#cti} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#MAX_INDEX}; past that
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
       * has reached bar {@link Core#MAX_INDEX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("CTI advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CtiStream( CtiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumX = other.sumX;
         this.sumX2 = other.sumX2;
         this.sumXY = other.sumXY;
         this.leavingX = other.leavingX;
         this.shift = other.shift;
         this.invPeriod = other.invPeriod;
         this.dPeriod = other.dPeriod;
         this.sumY = other.sumY;
         this.ssY = other.ssY;
         this.lookbackTotal = other.lookbackTotal;
         this.trailingIdx = other.trailingIdx;
         this.barsSinceReseed = other.barsSinceReseed;
         this.j = other.j;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
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
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("CTI update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("CTI update: BAD_PARAM", RetCode.BAD_PARAM);
         core.ctiStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("CTI peek: BAD_PARAM", RetCode.BAD_PARAM);
         CtiStream sp = this;
         double x = 0.0;
         double ssX = 0.0;
         double spXY = 0.0;
         double tempReal = 0.0;
         int windowStart = 0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double shift = sp.shift;
         double sumX = sp.sumX;
         double sumX2 = sp.sumX2;
         double sumXY = sp.sumXY;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inReal;
         /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
          * leaves sumXY alone.
          */
         x = (((sp.today & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0) - shift;
         sumX += x;
         sumX2 += x * x;
         ssX = sumX2 - sumX * sumX * sp.invPeriod;
         spXY = sumXY - sumX * sp.sumY * sp.invPeriod;
         /* Re-anchor and rebuild when the shift has gone stale: the price sum of
          * squares has shrunk below 1e-6 of the squared deviations it is extracted
          * from; OR the value the PREVIOUS bar removed sat so far from the shift
          * that its squared term dwarfs what remains; OR at least every 32
          * windows. Same three triggers as correl.c, watching the price side only
          * -- the ramp side is exact constants and cannot drift.
          *
          * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
          * with no linear trend, and reseeding on it would rebuild on every bar of
          * ordinary sideways data.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || sp.leavingX > 1000000.0 * sumX2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = sp.today - sp.lookbackTotal;
            tempReal = 0.0;
            for( j = windowStart; j <= sp.today; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
            }
            shift = tempReal * sp.invPeriod;
            sumXY = 0.0;
            sumX2 = sumXY;
            sumX = sumX2;
            for( j = windowStart; j <= sp.today; j += 1 ) {
               x = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0) - shift;
               sumX += x;
               sumX2 += x * x;
               sumXY += x * (double)(sp.today - j);
            }
            ssX = sumX2 - sumX * sumX * sp.invPeriod;
            spXY = sumXY - sumX * sp.sumY * sp.invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforced here rather than at the
             * divide, exactly as correl.c does: a negative ssX always reseeds on
             * the same bar, so the divide below can rely on it being >= 0.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
         }
         trailingIdx += 1;
         /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
          * series correlates NEGATIVELY with it. Ehlers' listing counts the same
          * way and takes Y = -count, which is the positively-sloped line the
          * indicator is defined against; negating the coefficient once is the same
          * thing, and it keeps the O(1) slide identity below written the way
          * linearreg.c:102-114 states it. Dropping this negation silently inverts
          * the whole indicator -- Pearson r is odd in either variable -- and a
          * magnitude or |r| assertion cannot see it.
          *
          * ssY is a positive constant, so only ssX can make the window degenerate.
          * It is tested against its own scale rather than an absolute band: the
          * product carries the fourth power of the window's spread, so a fixed
          * threshold rejects a well-defined correlation as soon as the data is
          * small. The product is tested on its own because neither factor's test
          * implies it -- at that fourth power it can underflow to exactly 0.0
          * while both are still ordinary normals, and a zero divisor there gives
          * NaN, which the clamp does not catch.
          *
          * An all-flat window therefore emits exactly 0.0 rather than NaN, which
          * is correl.c's precedent and what #112 requires of a successful call.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssX * sp.ssY > 0.0 ) {
            tempReal = (0 - spXY) / Math.sqrt(ssX * sp.ssY);
            /* A correlation coefficient cannot leave [-1,1]; rounding in the
             * three sums can still put it a few ulp outside.
             */
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            } else if( tempReal < 0 - 1.0 ) {
               tempReal = 0 - 1.0;
            }
            cur_outReal = tempReal;
         } else {
            cur_outReal = 0.0;
         }
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
      public CtiStream clone() {
         return new CtiStream(this);
      }
   }
   private void ctiStepImpl( CtiStream sp, double inReal )
   {
      double x = 0.0;
      double trailingX = 0.0;
      double ssX = 0.0;
      double spXY = 0.0;
      double tempReal = 0.0;
      int windowStart = 0;
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
       * leaves sumXY alone.
       */
      x = sp.x_inReal[sp.today & sp.xMask] - sp.shift;
      sp.sumX += x;
      sp.sumX2 += x * x;
      ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
      spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
      /* Re-anchor and rebuild when the shift has gone stale: the price sum of
       * squares has shrunk below 1e-6 of the squared deviations it is extracted
       * from; OR the value the PREVIOUS bar removed sat so far from the shift
       * that its squared term dwarfs what remains; OR at least every 32
       * windows. Same three triggers as correl.c, watching the price side only
       * -- the ramp side is exact constants and cannot drift.
       *
       * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
       * with no linear trend, and reseeding on it would rebuild on every bar of
       * ordinary sideways data.
       */
      sp.barsSinceReseed -= 1;
      if( ssX < 0.000001 * sp.sumX2 || sp.leavingX > 1000000.0 * sp.sumX2 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         windowStart = sp.today - sp.lookbackTotal;
         tempReal = 0.0;
         for( sp.j = windowStart; sp.j <= sp.today; sp.j += 1 ) {
            tempReal += sp.x_inReal[sp.j & sp.xMask];
         }
         sp.shift = tempReal * sp.invPeriod;
         sp.sumXY = 0.0;
         sp.sumX2 = sp.sumXY;
         sp.sumX = sp.sumX2;
         for( sp.j = windowStart; sp.j <= sp.today; sp.j += 1 ) {
            x = sp.x_inReal[sp.j & sp.xMask] - sp.shift;
            sp.sumX += x;
            sp.sumX2 += x * x;
            sp.sumXY += x * (double)(sp.today - sp.j);
         }
         ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
         spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
         /* A sum of squares is non-negative by definition, but this one is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * sitting inside a flat stretch. Enforced here rather than at the
          * divide, exactly as correl.c does: a negative ssX always reseeds on
          * the same bar, so the divide below can rely on it being >= 0.
          */
         if( ssX < 0.0 ) {
            ssX = 0.0;
         }
      }
      /* Save the trailing value before writing the output, since the input and
       * output might be the same array.
       */
      trailingX = sp.x_inReal[sp.trailingIdx & sp.xMask] - sp.shift;
      sp.trailingIdx += 1;
      /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
       * series correlates NEGATIVELY with it. Ehlers' listing counts the same
       * way and takes Y = -count, which is the positively-sloped line the
       * indicator is defined against; negating the coefficient once is the same
       * thing, and it keeps the O(1) slide identity below written the way
       * linearreg.c:102-114 states it. Dropping this negation silently inverts
       * the whole indicator -- Pearson r is odd in either variable -- and a
       * magnitude or |r| assertion cannot see it.
       *
       * ssY is a positive constant, so only ssX can make the window degenerate.
       * It is tested against its own scale rather than an absolute band: the
       * product carries the fourth power of the window's spread, so a fixed
       * threshold rejects a well-defined correlation as soon as the data is
       * small. The product is tested on its own because neither factor's test
       * implies it -- at that fourth power it can underflow to exactly 0.0
       * while both are still ordinary normals, and a zero divisor there gives
       * NaN, which the clamp does not catch.
       *
       * An all-flat window therefore emits exactly 0.0 rather than NaN, which
       * is correl.c's precedent and what #112 requires of a successful call.
       */
      if( ssX > 0.00000000000001 * sp.sumX2 && ssX * sp.ssY > 0.0 ) {
         tempReal = (0 - spXY) / Math.sqrt(ssX * sp.ssY);
         /* A correlation coefficient cannot leave [-1,1]; rounding in the
          * three sums can still put it a few ulp outside.
          */
         if( tempReal > 1.0 ) {
            tempReal = 1.0;
         } else if( tempReal < 0 - 1.0 ) {
            tempReal = 0 - 1.0;
         }
         sp.cur_outReal = tempReal;
      } else {
         sp.cur_outReal = 0.0;
      }
      /* Slide the window one bar in O(1). Advancing it ages every retained
       * value by one bar, which raises each of their weights by 1 and so adds
       * the whole deviation sum; the departing value leaves at the full weight
       * optInTimePeriod-1 and also loses its place in that sum, which is the
       * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
       * and sumX must still be the COMPLETE window's sum when it is read here
       * -- it is decremented on the line below, not above.
       */
      sp.leavingX = trailingX * trailingX;
      sp.sumXY = sp.sumXY + sp.sumX - sp.dPeriod * trailingX;
      sp.sumX -= trailingX;
      sp.sumX2 -= sp.leavingX;
      sp.today += 1;
   }
   private RetCode ctiOpenImpl( CtiStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumX = 0;
      double sumX2 = 0;
      double sumXY = 0;
      double x = 0;
      double trailingX = 0;
      double leavingX = 0;
      double shift = 0;
      double ssX = 0;
      double spXY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      double dPeriod = 0;
      double sumY = 0;
      double ssY = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int j = 0;
      int barsSinceReseed = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      dPeriod = (double)optInTimePeriod;
      invPeriod = 1.0 / dPeriod;
      /* The ramp side is data-independent and collapses to two constants. With y
       * running 0..optInTimePeriod-1 as BARS AGO,
       *
       *    sumY = n(n-1)/2            ssY = sumY2 - sumY*sumY/n = n(n*n-1)/12
       *
       * Both are computed in double because their integer forms overflow: at the
       * top of the parameter range n*n*n is 1e15, far past what an int holds, and
       * the product would wrap silently rather than fail. ssY is strictly positive
       * for every n >= 2, which is why only the price side can make a window
       * degenerate.
       */
      sumY = dPeriod * (dPeriod - 1.0) * 0.5;
      ssY = dPeriod * (dPeriod * dPeriod - 1.0) / 12.0;
      /* Measure the price side against a shift near the window, as correl.c does
       * (#242). Transcribing the author's listing literally would carry
       * n*Sxx - Sx*Sx on raw price levels, which is the quantity that returned 0,
       * -1 and -1.73 from a perfectly correlated pair: MEASURED on the card, the
       * naive form errs by 5.7e-03 at a 1e2 price level with a 1e-5 spread, on an
       * indicator whose entire range is [-1, +1].
       *
       * Anchor on the first window value here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      shift = inReal[trailingIdx];
      /* The initial window, less its last bar. This bar's y is startIdx-j, since
       * the first output is computed with `today` at startIdx.
       */
      sumXY = 0.0;
      sumX2 = sumXY;
      sumX = sumX2;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = inReal[j] - shift;
         sumX += x;
         sumX2 += x * x;
         sumXY += x * (double)(startIdx - j);
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      do {
         /* The incoming bar is zero bars ago, so it moves sumX and sumX2 and
          * leaves sumXY alone.
          */
         x = inReal[today] - shift;
         sumX += x;
         sumX2 += x * x;
         ssX = sumX2 - sumX * sumX * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         /* Re-anchor and rebuild when the shift has gone stale: the price sum of
          * squares has shrunk below 1e-6 of the squared deviations it is extracted
          * from; OR the value the PREVIOUS bar removed sat so far from the shift
          * that its squared term dwarfs what remains; OR at least every 32
          * windows. Same three triggers as correl.c, watching the price side only
          * -- the ramp side is exact constants and cannot drift.
          *
          * A vanishing spXY is NOT a trigger. It is a legitimate answer, a window
          * with no linear trend, and reseeding on it would rebuild on every bar of
          * ordinary sideways data.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || leavingX > 1000000.0 * sumX2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            tempReal = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal[j];
            }
            shift = tempReal * invPeriod;
            sumXY = 0.0;
            sumX2 = sumXY;
            sumX = sumX2;
            for( j = windowStart; j <= today; j += 1 ) {
               x = inReal[j] - shift;
               sumX += x;
               sumX2 += x * x;
               sumXY += x * (double)(today - j);
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforced here rather than at the
             * divide, exactly as correl.c does: a negative ssX always reseeds on
             * the same bar, so the divide below can rely on it being >= 0.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
         }
         /* Save the trailing value before writing the output, since the input and
          * output might be the same array.
          */
         trailingX = inReal[trailingIdx] - shift;
         trailingIdx += 1;
         /* THE SIGN. y here is BARS AGO, so it runs backward in time and a rising
          * series correlates NEGATIVELY with it. Ehlers' listing counts the same
          * way and takes Y = -count, which is the positively-sloped line the
          * indicator is defined against; negating the coefficient once is the same
          * thing, and it keeps the O(1) slide identity below written the way
          * linearreg.c:102-114 states it. Dropping this negation silently inverts
          * the whole indicator -- Pearson r is odd in either variable -- and a
          * magnitude or |r| assertion cannot see it.
          *
          * ssY is a positive constant, so only ssX can make the window degenerate.
          * It is tested against its own scale rather than an absolute band: the
          * product carries the fourth power of the window's spread, so a fixed
          * threshold rejects a well-defined correlation as soon as the data is
          * small. The product is tested on its own because neither factor's test
          * implies it -- at that fourth power it can underflow to exactly 0.0
          * while both are still ordinary normals, and a zero divisor there gives
          * NaN, which the clamp does not catch.
          *
          * An all-flat window therefore emits exactly 0.0 rather than NaN, which
          * is correl.c's precedent and what #112 requires of a successful call.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssX * ssY > 0.0 ) {
            tempReal = (0 - spXY) / Math.sqrt(ssX * ssY);
            /* A correlation coefficient cannot leave [-1,1]; rounding in the
             * three sums can still put it a few ulp outside.
             */
            if( tempReal > 1.0 ) {
               tempReal = 1.0;
            } else if( tempReal < 0 - 1.0 ) {
               tempReal = 0 - 1.0;
            }
            outReal[outIdx++ * outStride] = tempReal;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         /* Slide the window one bar in O(1). Advancing it ages every retained
          * value by one bar, which raises each of their weights by 1 and so adds
          * the whole deviation sum; the departing value leaves at the full weight
          * optInTimePeriod-1 and also loses its place in that sum, which is the
          * remaining -1. The two together are exactly -optInTimePeriod*trailingX,
          * and sumX must still be the COMPLETE window's sum when it is read here
          * -- it is decremented on the line below, not above.
          */
         leavingX = trailingX * trailingX;
         sumXY = sumXY + sumX - dPeriod * trailingX;
         sumX -= trailingX;
         sumX2 -= leavingX;
         today += 1;
      } while( today <= endIdx );
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
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumX = sumX;
      sp.sumX2 = sumX2;
      sp.sumXY = sumXY;
      sp.leavingX = leavingX;
      sp.shift = shift;
      sp.invPeriod = invPeriod;
      sp.dPeriod = dPeriod;
      sp.sumY = sumY;
      sp.ssY = ssY;
      sp.lookbackTotal = lookbackTotal;
      sp.trailingIdx = trailingIdx;
      sp.barsSinceReseed = barsSinceReseed;
      sp.j = j;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* ctiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CtiStream ctiOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CtiStream sp = new CtiStream(this);
      RetCode retCode = ctiOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("CTI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("CTI openAndFill: internal error", retCode);
      }
      throw new TALibArgumentException("CTI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ctiOpen (composition seam). */
   CtiStream ctiOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CtiStream sp = new CtiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ctiOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("CTI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("CTI open: internal error", retCode);
      }
      throw new TALibArgumentException("CTI open: " + retCode, retCode);
   }
   /**
    * Open a live CTI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cti} at that bar.
    * <p>The history must hold at least {@code ctiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CtiStream ctiOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("CTI open", "inReal", inReal);
      requireHistory("CTI open", inReal.length);
      return ctiOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#ctiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cti} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CtiStream#outRange()}.
    */
   public CtiStream ctiOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CTI openAndFill", "inReal", inReal);
      requireHistory("CTI openAndFill", inReal.length);
      int guardOutLen = openFillCount("CTI openAndFill", inReal.length, ctiLookback(optInTimePeriod));
      requireLength("CTI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TALibArgumentException("CTI openAndFill: " + RetCode.BAD_PARAM, RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ctiOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
