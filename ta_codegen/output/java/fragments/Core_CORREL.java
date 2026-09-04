/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101003 MF   Initial Coding
 *  062804 MF   Resolve div by zero bug on limit case.
 *  082326 MF   Fix #242. Cancellation-free sums (shifted data + reseed, as
 *              TA_VAR does since #118), per-factor degeneracy test and a
 *              range clamp.
 */

   /**
    * Number of leading input bars {@link Core#CORREL} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CORREL_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode CORREL_Impl( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double shiftX = 0;
      double shiftY = 0;
      double ssX = 0;
      double ssY = 0;
      double spXY = 0;
      double leavingX = 0;
      double leavingY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Move up the start index if there is not
       * enough initial data.
       */
      /* One reciprocal instead of three divisions per bar, as TA_VAR does. The
       * extra rounding it costs is invisible next to what the shift recovers, and
       * it is what keeps this form cheaper than the one it replaces.
       */
      invPeriod = 1.0 / (double)optInTimePeriod;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Measure both series against a shift near the window, exactly as TA_VAR
       * does (#118). The running sums then hold deviations rather than raw levels,
       * so ssX = sumX2-(sumX*sumX)*invPeriod is no longer a difference of two
       * ~period*mean^2 quantities. Without this the extracted sum of squares keeps
       * only the digits that survive that subtraction: at a $100 price level with a
       * 1e-5 spread that is three of them, and the correlation of two perfectly
       * correlated series came back as 0, as -1, or as -1.73 (#242).
       *
       * Anchor on the first window value here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      shiftX = inReal0[trailingIdx];
      shiftY = inReal1[trailingIdx];
      /* Calculate the initial values (the window less its last bar). */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = inReal0[j] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = inReal1[j] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      leavingY = 0.0;
      do {
         /* Add the incoming value, measured against the shift. */
         x = inReal0[today] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         ssX = sumX2 - sumX * sumX * invPeriod;
         ssY = sumY2 - sumY * sumY * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         /* Re-anchor and rebuild with a fresh two-pass when the shift has gone
          * stale. Same three triggers as TA_VAR: either sum of squares has shrunk
          * below 1e-6 of the squared deviations it is extracted from; OR the value
          * the PREVIOUS bar removed sat so far from the shift that its squared term
          * dwarfs what remains (a large outlier transiting the window buries the
          * small terms below its ulp, and the residue it leaves is cancellation
          * garbage); OR at least every 32 windows, so a slow drift stays bounded
          * however long the series runs.
          *
          * One bar late is correct, not a compromise. leavingX/leavingY are set by
          * the removal at the BOTTOM of the loop, so the bar on which the outlier
          * actually leaves still computes its own output from sums that legitimately
          * contain it. The trigger then fires on the NEXT bar -- the first one whose
          * sums carry the residue -- and the reseed below recomputes that bar's
          * output before it is written. No bar is ever emitted from the residue.
          *
          * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
          * legitimate answer - two uncorrelated series - not a loss of digits, and
          * reseeding on it would rebuild the window on every bar of ordinary data.
          * This is where the analogy with TA_VAR stops: variance has one extracted
          * quantity and all of it is signal.
          *
          * Reading the window here is safe when outReal aliases an input: the
          * outputs written so far occupy [0, outIdx-1] while windowStart is
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || ssY < 0.000001 * sumY2 || leavingX > 1000000.0 * sumX2 || leavingY > 1000000.0 * sumY2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            /* Both means in one pass over the window: the rebuild below is the
             * only O(period) work on this function's hot path, so it is walked
             * twice, not three times.
             */
            tempReal = 0.0;
            shiftY = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal0[j];
               shiftY += inReal1[j];
            }
            shiftX = tempReal * invPeriod;
            shiftY = shiftY * invPeriod;
            sumY2 = 0.0;
            sumX2 = sumY2;
            sumY = sumX2;
            sumX = sumY;
            sumXY = sumX;
            for( j = windowStart; j <= today; j += 1 ) {
               x = inReal0[j] - shiftX;
               sumX += x;
               sumX2 += x * x;
               y = inReal1[j] - shiftY;
               sumXY += x * y;
               sumY += y;
               sumY2 += y * y;
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            ssY = sumY2 - sumY * sumY * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforce the invariant HERE and not at
             * the divide: a negative ssX always reseeds on the same bar (it makes
             * the first trigger's `negative < non-negative` true whenever sumX2 is
             * positive, and sumX2 == 0 reduces that trigger to `ssX < 0`), so the
             * divide below can rely on both being >= 0 and needs no sign test of
             * its own. CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
            if( ssY < 0.0 ) {
               ssY = 0.0;
            }
         }
         /* Save the trailing values before writing the output, since the input
          * and output might be the same array.
          */
         trailingX = inReal0[trailingIdx] - shiftX;
         trailingY = inReal1[trailingIdx] - shiftY;
         trailingIdx += 1;
         /* Output the new coefficient.
          *
          * Each sum of squares is tested against its OWN scale, not the pair
          * against a fixed band. The product ssX*ssY carries the fourth power of
          * the window's spread, so an absolute threshold on it rejects a perfectly
          * well-defined correlation as soon as the data is small - and, worse,
          * lets a pair of NEGATIVE sums through, their signs cancelling into a
          * plausible-looking result of the wrong sign. Testing each factor
          * separately is what forecloses both.
          *
          * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
          * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
          * that must never reach the square root. A plain `>` rejects it, and it
          * is also the cheaper test: the two fabs() cost ~7% of this function's
          * runtime, and buy a wrong answer.
          *
          * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
          * established both are positive, so the product needs no protection from
          * a negative operand, and the second square root is worth ~25% of the
          * runtime.
          *
          * The product CAN overflow to +Inf, and the one-root form is chosen with
          * that known. TA_REAL_MAX bounds optional PARAMETERS; a batch call's input
          * arrays are not range-checked, so ssX and ssY are bounded only by the
          * double range and their product exceeds it once |x| passes ~1e154. The
          * two-root form would not overflow there -- but the form this replaces
          * built exactly the same product (it tested ssX*ssY against TA_EPSILON), so
          * the exposure is unchanged, and an Inf here yields 0.0 rather than a wrong
          * correlation. Trading a quarter of the runtime for a case that already
          * behaved this way, on inputs 117 orders past any price, is not a trade
          * worth making. Revisit only if input range-checking is ever added.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssY > 0.00000000000001 * sumY2 ) {
            tempReal = spXY / Math.sqrt(ssX * ssY);
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
         /* Remove the trailing values (prepares the next window). */
         leavingX = trailingX * trailingX;
         leavingY = trailingY * trailingY;
         sumX -= trailingX;
         sumX2 -= leavingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= leavingY;
         today += 1;
      } while( today <= endIdx );
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode CORREL_Impl( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double shiftX = 0;
      double shiftY = 0;
      double ssX = 0;
      double ssY = 0;
      double spXY = 0;
      double leavingX = 0;
      double leavingY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      invPeriod = 1.0 / (double)optInTimePeriod;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      shiftX = (double)inReal0[trailingIdx];
      shiftY = (double)inReal1[trailingIdx];
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = (double)inReal0[j] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[j] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      leavingY = 0.0;
      do {
         x = (double)inReal0[today] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         ssX = sumX2 - sumX * sumX * invPeriod;
         ssY = sumY2 - sumY * sumY * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || ssY < 0.000001 * sumY2 || leavingX > 1000000.0 * sumX2 || leavingY > 1000000.0 * sumY2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            tempReal = 0.0;
            shiftY = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += (double)inReal0[j];
               shiftY += (double)inReal1[j];
            }
            shiftX = tempReal * invPeriod;
            shiftY = shiftY * invPeriod;
            sumY2 = 0.0;
            sumX2 = sumY2;
            sumY = sumX2;
            sumX = sumY;
            sumXY = sumX;
            for( j = windowStart; j <= today; j += 1 ) {
               x = (double)inReal0[j] - shiftX;
               sumX += x;
               sumX2 += x * x;
               y = (double)inReal1[j] - shiftY;
               sumXY += x * y;
               sumY += y;
               sumY2 += y * y;
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            ssY = sumY2 - sumY * sumY * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
            if( ssY < 0.0 ) {
               ssY = 0.0;
            }
         }
         trailingX = (double)inReal0[trailingIdx] - shiftX;
         trailingY = (double)inReal1[trailingIdx] - shiftY;
         trailingIdx += 1;
         if( ssX > 0.00000000000001 * sumX2 && ssY > 0.00000000000001 * sumY2 ) {
            tempReal = spXY / Math.sqrt(ssX * ssY);
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
         leavingY = trailingY * trailingY;
         sumX -= trailingX;
         sumX2 -= leavingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= leavingY;
         today += 1;
      } while( today <= endIdx );
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Pearson's correlation coefficient (r) between two input series over a
    * rolling window of optInTimePeriod bars. Measures how linearly the two
    * series move together. r near +1: strong positive co-movement; near -1:
    * strong inverse; near 0: no linear relationship.
    * <p><b>Formula</b>
    * <pre>{@code
    * r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CORREL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First data series (X)
    * @param inReal1 Second data series (Y)
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Correlation coefficient r in [-1, 1]. Must hold at least
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
    * @see Core#BETA
    * @see Core#STDDEV
    * @see Core#VAR
    */
   public OutRange CORREL( int startIdx,
                           int endIdx,
                           double inReal0[],
                           double inReal1[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("CORREL", startIdx, endIdx);
      int guardStart = clampedStart("CORREL", startIdx, CORREL_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CORREL", "inReal0", inReal0, guardInLen);
      requireLength("CORREL", "inReal1", inReal1, guardInLen);
      requireLength("CORREL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CORREL_Impl(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CORREL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Pearson's correlation coefficient (r) between two input series over a
    * rolling window of optInTimePeriod bars. Measures how linearly the two
    * series move together. r near +1: strong positive co-movement; near -1:
    * strong inverse; near 0: no linear relationship.
    * <p><b>Formula</b>
    * <pre>{@code
    * r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CORREL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First data series (X)
    * @param inReal1 Second data series (Y)
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Correlation coefficient r in [-1, 1]. Must hold at least
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
    * @see Core#BETA
    * @see Core#STDDEV
    * @see Core#VAR
    */
   public OutRange CORREL( int startIdx,
                           int endIdx,
                           float inReal0[],
                           float inReal1[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("CORREL", startIdx, endIdx);
      int guardStart = clampedStart("CORREL", startIdx, CORREL_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CORREL", "inReal0", inReal0, guardInLen);
      requireLength("CORREL", "inReal1", inReal1, guardInLen);
      requireLength("CORREL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CORREL_Impl(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CORREL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CORREL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CORREL} over the same series.
    * Open with {@link Core#correlOpen}; there is no close — the handle is
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
   public static final class CorrelStream {
      Core core;
      int optInTimePeriod;
      double sumXY;
      double sumX;
      double sumY;
      double sumX2;
      double sumY2;
      double shiftX;
      double shiftY;
      double leavingX;
      double leavingY;
      double invPeriod;
      int lookbackTotal;
      int trailingIdx;
      int barsSinceReseed;
      int j;
      int today;
      int xMask;
      double[] x_inReal0;
      double[] x_inReal1;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CorrelStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CORREL} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CorrelStream( CorrelStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumXY = other.sumXY;
         this.sumX = other.sumX;
         this.sumY = other.sumY;
         this.sumX2 = other.sumX2;
         this.sumY2 = other.sumY2;
         this.shiftX = other.shiftX;
         this.shiftY = other.shiftY;
         this.leavingX = other.leavingX;
         this.leavingY = other.leavingY;
         this.invPeriod = other.invPeriod;
         this.lookbackTotal = other.lookbackTotal;
         this.trailingIdx = other.trailingIdx;
         this.barsSinceReseed = other.barsSinceReseed;
         this.j = other.j;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal0 = other.x_inReal0.clone();
         this.x_inReal1 = other.x_inReal1.clone();
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
      public double update( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("CORREL update: BadParam", RetCode.BadParam);
         }
         core.correlStepImpl(this, inReal0, inReal1);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal0.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal0[], double inReal1[], double outReal[] ) {
         requireArgument("CORREL updateAndFill", "inReal0", inReal0);
         requireArgument("CORREL updateAndFill", "inReal1", inReal1);
         requireArgument("CORREL updateAndFill", "outReal", outReal);
         final int barCount = inReal0.length;
         if( inReal1.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 )
            throw new TaLibArgumentException("CORREL updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal0[i]) || !Double.isFinite(inReal1[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CORREL updateAndFill: BadParam", RetCode.BadParam);
            }
            core.correlStepImpl(this, inReal0[i], inReal1[i]);
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
      public double peek( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("CORREL peek: BadParam", RetCode.BadParam);
         CorrelStream sp = this;
         double x = 0.0;
         double y = 0.0;
         double ssX = 0.0;
         double ssY = 0.0;
         double spXY = 0.0;
         double tempReal = 0.0;
         int windowStart = 0;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double shiftX = sp.shiftX;
         double shiftY = sp.shiftY;
         double sumX = sp.sumX;
         double sumX2 = sp.sumX2;
         double sumXY = sp.sumXY;
         double sumY = sp.sumY;
         double sumY2 = sp.sumY2;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            j -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inReal0;
         pkSlot1 = today & sp.xMask;
         pkVal1 = inReal1;
         /* Add the incoming value, measured against the shift. */
         x = (((today & sp.xMask) != pkSlot0) ? sp.x_inReal0[today & sp.xMask] : pkVal0) - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = (((today & sp.xMask) != pkSlot1) ? sp.x_inReal1[today & sp.xMask] : pkVal1) - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         ssX = sumX2 - sumX * sumX * sp.invPeriod;
         ssY = sumY2 - sumY * sumY * sp.invPeriod;
         spXY = sumXY - sumX * sumY * sp.invPeriod;
         /* Re-anchor and rebuild with a fresh two-pass when the shift has gone
          * stale. Same three triggers as TA_VAR: either sum of squares has shrunk
          * below 1e-6 of the squared deviations it is extracted from; OR the value
          * the PREVIOUS bar removed sat so far from the shift that its squared term
          * dwarfs what remains (a large outlier transiting the window buries the
          * small terms below its ulp, and the residue it leaves is cancellation
          * garbage); OR at least every 32 windows, so a slow drift stays bounded
          * however long the series runs.
          *
          * One bar late is correct, not a compromise. leavingX/leavingY are set by
          * the removal at the BOTTOM of the loop, so the bar on which the outlier
          * actually leaves still computes its own output from sums that legitimately
          * contain it. The trigger then fires on the NEXT bar -- the first one whose
          * sums carry the residue -- and the reseed below recomputes that bar's
          * output before it is written. No bar is ever emitted from the residue.
          *
          * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
          * legitimate answer - two uncorrelated series - not a loss of digits, and
          * reseeding on it would rebuild the window on every bar of ordinary data.
          * This is where the analogy with TA_VAR stops: variance has one extracted
          * quantity and all of it is signal.
          *
          * Reading the window here is safe when outReal aliases an input: the
          * outputs written so far occupy [0, outIdx-1] while windowStart is
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || ssY < 0.000001 * sumY2 || sp.leavingX > 1000000.0 * sumX2 || sp.leavingY > 1000000.0 * sumY2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = today - sp.lookbackTotal;
            /* Both means in one pass over the window: the rebuild below is the
             * only O(period) work on this function's hot path, so it is walked
             * twice, not three times.
             */
            tempReal = 0.0;
            shiftY = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += ((j & sp.xMask) != pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0;
               shiftY += ((j & sp.xMask) != pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1;
            }
            shiftX = tempReal * sp.invPeriod;
            shiftY = shiftY * sp.invPeriod;
            sumY2 = 0.0;
            sumX2 = sumY2;
            sumY = sumX2;
            sumX = sumY;
            sumXY = sumX;
            for( j = windowStart; j <= today; j += 1 ) {
               x = (((j & sp.xMask) != pkSlot0) ? sp.x_inReal0[j & sp.xMask] : pkVal0) - shiftX;
               sumX += x;
               sumX2 += x * x;
               y = (((j & sp.xMask) != pkSlot1) ? sp.x_inReal1[j & sp.xMask] : pkVal1) - shiftY;
               sumXY += x * y;
               sumY += y;
               sumY2 += y * y;
            }
            ssX = sumX2 - sumX * sumX * sp.invPeriod;
            ssY = sumY2 - sumY * sumY * sp.invPeriod;
            spXY = sumXY - sumX * sumY * sp.invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforce the invariant HERE and not at
             * the divide: a negative ssX always reseeds on the same bar (it makes
             * the first trigger's `negative < non-negative` true whenever sumX2 is
             * positive, and sumX2 == 0 reduces that trigger to `ssX < 0`), so the
             * divide below can rely on both being >= 0 and needs no sign test of
             * its own. CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
            if( ssY < 0.0 ) {
               ssY = 0.0;
            }
         }
         trailingIdx += 1;
         /* Output the new coefficient.
          *
          * Each sum of squares is tested against its OWN scale, not the pair
          * against a fixed band. The product ssX*ssY carries the fourth power of
          * the window's spread, so an absolute threshold on it rejects a perfectly
          * well-defined correlation as soon as the data is small - and, worse,
          * lets a pair of NEGATIVE sums through, their signs cancelling into a
          * plausible-looking result of the wrong sign. Testing each factor
          * separately is what forecloses both.
          *
          * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
          * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
          * that must never reach the square root. A plain `>` rejects it, and it
          * is also the cheaper test: the two fabs() cost ~7% of this function's
          * runtime, and buy a wrong answer.
          *
          * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
          * established both are positive, so the product needs no protection from
          * a negative operand, and the second square root is worth ~25% of the
          * runtime.
          *
          * The product CAN overflow to +Inf, and the one-root form is chosen with
          * that known. TA_REAL_MAX bounds optional PARAMETERS; a batch call's input
          * arrays are not range-checked, so ssX and ssY are bounded only by the
          * double range and their product exceeds it once |x| passes ~1e154. The
          * two-root form would not overflow there -- but the form this replaces
          * built exactly the same product (it tested ssX*ssY against TA_EPSILON), so
          * the exposure is unchanged, and an Inf here yields 0.0 rather than a wrong
          * correlation. Trading a quarter of the runtime for a case that already
          * behaved this way, on inputs 117 orders past any price, is not a trade
          * worth making. Revisit only if input range-checking is ever added.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssY > 0.00000000000001 * sumY2 ) {
            tempReal = spXY / Math.sqrt(ssX * ssY);
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
      public CorrelStream clone() {
         return new CorrelStream(this);
      }
   }
   void correlStepImpl( CorrelStream sp, double inReal0, double inReal1 )
   {
      double x = 0.0;
      double y = 0.0;
      double trailingX = 0.0;
      double trailingY = 0.0;
      double ssX = 0.0;
      double ssY = 0.0;
      double spXY = 0.0;
      double tempReal = 0.0;
      int windowStart = 0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
      }
      sp.x_inReal0[sp.today & sp.xMask] = inReal0;
      sp.x_inReal1[sp.today & sp.xMask] = inReal1;
      /* Add the incoming value, measured against the shift. */
      x = sp.x_inReal0[sp.today & sp.xMask] - sp.shiftX;
      sp.sumX += x;
      sp.sumX2 += x * x;
      y = sp.x_inReal1[sp.today & sp.xMask] - sp.shiftY;
      sp.sumXY += x * y;
      sp.sumY += y;
      sp.sumY2 += y * y;
      ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
      ssY = sp.sumY2 - sp.sumY * sp.sumY * sp.invPeriod;
      spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
      /* Re-anchor and rebuild with a fresh two-pass when the shift has gone
       * stale. Same three triggers as TA_VAR: either sum of squares has shrunk
       * below 1e-6 of the squared deviations it is extracted from; OR the value
       * the PREVIOUS bar removed sat so far from the shift that its squared term
       * dwarfs what remains (a large outlier transiting the window buries the
       * small terms below its ulp, and the residue it leaves is cancellation
       * garbage); OR at least every 32 windows, so a slow drift stays bounded
       * however long the series runs.
       *
       * One bar late is correct, not a compromise. leavingX/leavingY are set by
       * the removal at the BOTTOM of the loop, so the bar on which the outlier
       * actually leaves still computes its own output from sums that legitimately
       * contain it. The trigger then fires on the NEXT bar -- the first one whose
       * sums carry the residue -- and the reseed below recomputes that bar's
       * output before it is written. No bar is ever emitted from the residue.
       *
       * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
       * legitimate answer - two uncorrelated series - not a loss of digits, and
       * reseeding on it would rebuild the window on every bar of ordinary data.
       * This is where the analogy with TA_VAR stops: variance has one extracted
       * quantity and all of it is signal.
       *
       * Reading the window here is safe when outReal aliases an input: the
       * outputs written so far occupy [0, outIdx-1] while windowStart is
       * startIdx-lookbackTotal+outIdx, which is >= outIdx.
       */
      sp.barsSinceReseed -= 1;
      if( ssX < 0.000001 * sp.sumX2 || ssY < 0.000001 * sp.sumY2 || sp.leavingX > 1000000.0 * sp.sumX2 || sp.leavingY > 1000000.0 * sp.sumY2 || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         windowStart = sp.today - sp.lookbackTotal;
         /* Both means in one pass over the window: the rebuild below is the
          * only O(period) work on this function's hot path, so it is walked
          * twice, not three times.
          */
         tempReal = 0.0;
         sp.shiftY = 0.0;
         for( sp.j = windowStart; sp.j <= sp.today; sp.j += 1 ) {
            tempReal += sp.x_inReal0[sp.j & sp.xMask];
            sp.shiftY += sp.x_inReal1[sp.j & sp.xMask];
         }
         sp.shiftX = tempReal * sp.invPeriod;
         sp.shiftY = sp.shiftY * sp.invPeriod;
         sp.sumY2 = 0.0;
         sp.sumX2 = sp.sumY2;
         sp.sumY = sp.sumX2;
         sp.sumX = sp.sumY;
         sp.sumXY = sp.sumX;
         for( sp.j = windowStart; sp.j <= sp.today; sp.j += 1 ) {
            x = sp.x_inReal0[sp.j & sp.xMask] - sp.shiftX;
            sp.sumX += x;
            sp.sumX2 += x * x;
            y = sp.x_inReal1[sp.j & sp.xMask] - sp.shiftY;
            sp.sumXY += x * y;
            sp.sumY += y;
            sp.sumY2 += y * y;
         }
         ssX = sp.sumX2 - sp.sumX * sp.sumX * sp.invPeriod;
         ssY = sp.sumY2 - sp.sumY * sp.sumY * sp.invPeriod;
         spXY = sp.sumXY - sp.sumX * sp.sumY * sp.invPeriod;
         /* A sum of squares is non-negative by definition, but this one is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * sitting inside a flat stretch. Enforce the invariant HERE and not at
          * the divide: a negative ssX always reseeds on the same bar (it makes
          * the first trigger's `negative < non-negative` true whenever sumX2 is
          * positive, and sumX2 == 0 reduces that trigger to `ssX < 0`), so the
          * divide below can rely on both being >= 0 and needs no sign test of
          * its own. CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
          */
         if( ssX < 0.0 ) {
            ssX = 0.0;
         }
         if( ssY < 0.0 ) {
            ssY = 0.0;
         }
      }
      /* Save the trailing values before writing the output, since the input
       * and output might be the same array.
       */
      trailingX = sp.x_inReal0[sp.trailingIdx & sp.xMask] - sp.shiftX;
      trailingY = sp.x_inReal1[sp.trailingIdx & sp.xMask] - sp.shiftY;
      sp.trailingIdx += 1;
      /* Output the new coefficient.
       *
       * Each sum of squares is tested against its OWN scale, not the pair
       * against a fixed band. The product ssX*ssY carries the fourth power of
       * the window's spread, so an absolute threshold on it rejects a perfectly
       * well-defined correlation as soon as the data is small - and, worse,
       * lets a pair of NEGATIVE sums through, their signs cancelling into a
       * plausible-looking result of the wrong sign. Testing each factor
       * separately is what forecloses both.
       *
       * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
       * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
       * that must never reach the square root. A plain `>` rejects it, and it
       * is also the cheaper test: the two fabs() cost ~7% of this function's
       * runtime, and buy a wrong answer.
       *
       * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
       * established both are positive, so the product needs no protection from
       * a negative operand, and the second square root is worth ~25% of the
       * runtime.
       *
       * The product CAN overflow to +Inf, and the one-root form is chosen with
       * that known. TA_REAL_MAX bounds optional PARAMETERS; a batch call's input
       * arrays are not range-checked, so ssX and ssY are bounded only by the
       * double range and their product exceeds it once |x| passes ~1e154. The
       * two-root form would not overflow there -- but the form this replaces
       * built exactly the same product (it tested ssX*ssY against TA_EPSILON), so
       * the exposure is unchanged, and an Inf here yields 0.0 rather than a wrong
       * correlation. Trading a quarter of the runtime for a case that already
       * behaved this way, on inputs 117 orders past any price, is not a trade
       * worth making. Revisit only if input range-checking is ever added.
       */
      if( ssX > 0.00000000000001 * sp.sumX2 && ssY > 0.00000000000001 * sp.sumY2 ) {
         tempReal = spXY / Math.sqrt(ssX * ssY);
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
      /* Remove the trailing values (prepares the next window). */
      sp.leavingX = trailingX * trailingX;
      sp.leavingY = trailingY * trailingY;
      sp.sumX -= trailingX;
      sp.sumX2 -= sp.leavingX;
      sp.sumXY -= trailingX * trailingY;
      sp.sumY -= trailingY;
      sp.sumY2 -= sp.leavingY;
      sp.today += 1;
   }
   private RetCode correlOpenImpl( CorrelStream sp, double inReal0[], double inReal1[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double shiftX = 0;
      double shiftY = 0;
      double ssX = 0;
      double ssY = 0;
      double spXY = 0;
      double leavingX = 0;
      double leavingY = 0;
      double tempReal = 0;
      double invPeriod = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Move up the start index if there is not
       * enough initial data.
       */
      /* One reciprocal instead of three divisions per bar, as TA_VAR does. The
       * extra rounding it costs is invisible next to what the shift recovers, and
       * it is what keeps this form cheaper than the one it replaces.
       */
      invPeriod = 1.0 / (double)optInTimePeriod;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Measure both series against a shift near the window, exactly as TA_VAR
       * does (#118). The running sums then hold deviations rather than raw levels,
       * so ssX = sumX2-(sumX*sumX)*invPeriod is no longer a difference of two
       * ~period*mean^2 quantities. Without this the extracted sum of squares keeps
       * only the digits that survive that subtraction: at a $100 price level with a
       * 1e-5 spread that is three of them, and the correlation of two perfectly
       * correlated series came back as 0, as -1, or as -1.73 (#242).
       *
       * Anchor on the first window value here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      shiftX = inReal0[trailingIdx];
      shiftY = inReal1[trailingIdx];
      /* Calculate the initial values (the window less its last bar). */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         x = inReal0[j] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = inReal1[j] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      today = startIdx;
      outIdx = 0;
      barsSinceReseed = 32 * optInTimePeriod;
      leavingX = 0.0;
      leavingY = 0.0;
      do {
         /* Add the incoming value, measured against the shift. */
         x = inReal0[today] - shiftX;
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today] - shiftY;
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         ssX = sumX2 - sumX * sumX * invPeriod;
         ssY = sumY2 - sumY * sumY * invPeriod;
         spXY = sumXY - sumX * sumY * invPeriod;
         /* Re-anchor and rebuild with a fresh two-pass when the shift has gone
          * stale. Same three triggers as TA_VAR: either sum of squares has shrunk
          * below 1e-6 of the squared deviations it is extracted from; OR the value
          * the PREVIOUS bar removed sat so far from the shift that its squared term
          * dwarfs what remains (a large outlier transiting the window buries the
          * small terms below its ulp, and the residue it leaves is cancellation
          * garbage); OR at least every 32 windows, so a slow drift stays bounded
          * however long the series runs.
          *
          * One bar late is correct, not a compromise. leavingX/leavingY are set by
          * the removal at the BOTTOM of the loop, so the bar on which the outlier
          * actually leaves still computes its own output from sums that legitimately
          * contain it. The trigger then fires on the NEXT bar -- the first one whose
          * sums carry the residue -- and the reseed below recomputes that bar's
          * output before it is written. No bar is ever emitted from the residue.
          *
          * The triggers watch ssX and ssY only, never spXY. A vanishing spXY is a
          * legitimate answer - two uncorrelated series - not a loss of digits, and
          * reseeding on it would rebuild the window on every bar of ordinary data.
          * This is where the analogy with TA_VAR stops: variance has one extracted
          * quantity and all of it is signal.
          *
          * Reading the window here is safe when outReal aliases an input: the
          * outputs written so far occupy [0, outIdx-1] while windowStart is
          * startIdx-lookbackTotal+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( ssX < 0.000001 * sumX2 || ssY < 0.000001 * sumY2 || leavingX > 1000000.0 * sumX2 || leavingY > 1000000.0 * sumY2 || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            /* Both means in one pass over the window: the rebuild below is the
             * only O(period) work on this function's hot path, so it is walked
             * twice, not three times.
             */
            tempReal = 0.0;
            shiftY = 0.0;
            for( j = windowStart; j <= today; j += 1 ) {
               tempReal += inReal0[j];
               shiftY += inReal1[j];
            }
            shiftX = tempReal * invPeriod;
            shiftY = shiftY * invPeriod;
            sumY2 = 0.0;
            sumX2 = sumY2;
            sumY = sumX2;
            sumX = sumY;
            sumXY = sumX;
            for( j = windowStart; j <= today; j += 1 ) {
               x = inReal0[j] - shiftX;
               sumX += x;
               sumX2 += x * x;
               y = inReal1[j] - shiftY;
               sumXY += x * y;
               sumY += y;
               sumY2 += y * y;
            }
            ssX = sumX2 - sumX * sumX * invPeriod;
            ssY = sumY2 - sumY * sumY * invPeriod;
            spXY = sumXY - sumX * sumY * invPeriod;
            /* A sum of squares is non-negative by definition, but this one is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * sitting inside a flat stretch. Enforce the invariant HERE and not at
             * the divide: a negative ssX always reseeds on the same bar (it makes
             * the first trigger's `negative < non-negative` true whenever sumX2 is
             * positive, and sumX2 == 0 reduces that trigger to `ssX < 0`), so the
             * divide below can rely on both being >= 0 and needs no sign test of
             * its own. CHANGING THE TRIGGERS MEANS RE-CHECKING THIS.
             */
            if( ssX < 0.0 ) {
               ssX = 0.0;
            }
            if( ssY < 0.0 ) {
               ssY = 0.0;
            }
         }
         /* Save the trailing values before writing the output, since the input
          * and output might be the same array.
          */
         trailingX = inReal0[trailingIdx] - shiftX;
         trailingY = inReal1[trailingIdx] - shiftY;
         trailingIdx += 1;
         /* Output the new coefficient.
          *
          * Each sum of squares is tested against its OWN scale, not the pair
          * against a fixed band. The product ssX*ssY carries the fourth power of
          * the window's spread, so an absolute threshold on it rejects a perfectly
          * well-defined correlation as soon as the data is small - and, worse,
          * lets a pair of NEGATIVE sums through, their signs cancelling into a
          * plausible-looking result of the wrong sign. Testing each factor
          * separately is what forecloses both.
          *
          * The literal is TA_EPSILON. This is deliberately NOT TA_IS_ZERO_SCALED,
          * whose fabs() would admit a LARGE NEGATIVE ssX -- exactly the operand
          * that must never reach the square root. A plain `>` rejects it, and it
          * is also the cheaper test: the two fabs() cost ~7% of this function's
          * runtime, and buy a wrong answer.
          *
          * sqrt(ssX*ssY) rather than sqrt(ssX)*sqrt(ssY): the guard has already
          * established both are positive, so the product needs no protection from
          * a negative operand, and the second square root is worth ~25% of the
          * runtime.
          *
          * The product CAN overflow to +Inf, and the one-root form is chosen with
          * that known. TA_REAL_MAX bounds optional PARAMETERS; a batch call's input
          * arrays are not range-checked, so ssX and ssY are bounded only by the
          * double range and their product exceeds it once |x| passes ~1e154. The
          * two-root form would not overflow there -- but the form this replaces
          * built exactly the same product (it tested ssX*ssY against TA_EPSILON), so
          * the exposure is unchanged, and an Inf here yields 0.0 rather than a wrong
          * correlation. Trading a quarter of the runtime for a case that already
          * behaved this way, on inputs 117 orders past any price, is not a trade
          * worth making. Revisit only if input range-checking is ever added.
          */
         if( ssX > 0.00000000000001 * sumX2 && ssY > 0.00000000000001 * sumY2 ) {
            tempReal = spXY / Math.sqrt(ssX * ssY);
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
         /* Remove the trailing values (prepares the next window). */
         leavingX = trailingX * trailingX;
         leavingY = trailingY * trailingY;
         sumX -= trailingX;
         sumX2 -= leavingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= leavingY;
         today += 1;
      } while( today <= endIdx );
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inReal0 = new double[physX];
      double[] capX_inReal1 = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal0[fillJ & (physX - 1)] = inReal0[fillJ];
         capX_inReal1[fillJ & (physX - 1)] = inReal1[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumXY = sumXY;
      sp.sumX = sumX;
      sp.sumY = sumY;
      sp.sumX2 = sumX2;
      sp.sumY2 = sumY2;
      sp.shiftX = shiftX;
      sp.shiftY = shiftY;
      sp.leavingX = leavingX;
      sp.leavingY = leavingY;
      sp.invPeriod = invPeriod;
      sp.lookbackTotal = lookbackTotal;
      sp.trailingIdx = trailingIdx;
      sp.barsSinceReseed = barsSinceReseed;
      sp.j = j;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal0 = capX_inReal0;
      sp.x_inReal1 = capX_inReal1;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* correlOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CorrelStream correlOpenAndFillInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CorrelStream sp = new CorrelStream(this);
      RetCode retCode = correlOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CORREL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CORREL openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CORREL openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind correlOpen (composition seam). */
   CorrelStream correlOpenInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      CorrelStream sp = new CorrelStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = correlOpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CORREL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CORREL open: internal error", retCode);
      }
      throw new TaLibArgumentException("CORREL open: " + retCode, retCode);
   }
   /**
    * Open a live CORREL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CORREL} at that bar.
    * <p>The history must hold at least {@code CORREL_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CorrelStream correlOpen( double inReal0[], double inReal1[], int optInTimePeriod )
   {
      requireArgument("CORREL open", "inReal0", inReal0);
      requireHistory("CORREL open", inReal0.length);
      requireArgument("CORREL open", "inReal1", inReal1);
      requireHistoryLength("CORREL open", "inReal1", inReal1.length, inReal0.length);
      return correlOpenInternal(inReal0, inReal1, 0, optInTimePeriod);
   }
   /**
    * {@link Core#correlOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CORREL} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CorrelStream#outRange()}.
    */
   public CorrelStream correlOpenAndFill( double inReal0[], double inReal1[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CORREL openAndFill", "inReal0", inReal0);
      requireHistory("CORREL openAndFill", inReal0.length);
      requireArgument("CORREL openAndFill", "inReal1", inReal1);
      int guardOutLen = openFillCount("CORREL openAndFill", inReal0.length, CORREL_Lookback(optInTimePeriod));
      requireHistoryLength("CORREL openAndFill", "inReal1", inReal1.length, inReal0.length);
      requireLength("CORREL openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         throw new TaLibArgumentException("CORREL openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return correlOpenAndFillInternal(inReal0, inReal1, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
