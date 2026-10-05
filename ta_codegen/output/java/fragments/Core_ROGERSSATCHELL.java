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
 *  100526 KL,CC  Creation (#483).
 */

   /**
    * Number of leading input bars {@link Core#rogerssatchell} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window. Default 10, range 1
    *        to 100000. {@code n = 1} is the paper's own single-bar estimator (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInAnnualization Periods per year. Default 252, range 0 to
    *        {@code TA_REAL_MAX}. Pass 1 for the per-bar figure (default 252; minimum
    *        0; {@link Core#REAL_DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int rogerssatchellLookback( int optInTimePeriod, double optInAnnualization )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return -1;
      }
      /* The window's lookback and nothing else: there is no callee, and a term
       * reads only its own bar -- no previous close -- so no bar is consumed to
       * form it. Same as sum_lookback (sum/sum.c:16-19) and var_lookback
       * (var/var.c:21-26), and one less than an estimator that differences
       * against C[i-1]. optInAnnualization scales the output and cannot move the
       * first bar, as optInNbDev cannot in var.c:23.
       */
      return optInTimePeriod - 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#rogerssatchell}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of bars in the window. Default 10, range 1
    *        to 100000. {@code n = 1} is the paper's own single-bar estimator (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInAnnualization Periods per year. Default 252, range 0 to
    *        {@code TA_REAL_MAX}. Pass 1 for the per-bar figure (default 252; minimum
    *        0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int rogerssatchellDisplayShift( int optInTimePeriod, double optInAnnualization, int outputIdx )
   {
      if( rogerssatchellLookback( optInTimePeriod, optInAnnualization ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode rogerssatchellImpl( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               int optInTimePeriod,
                               double optInAnnualization,
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      double o = 0;
      double h = 0;
      double l = 0;
      double c = 0;
      double p1 = 0;
      double p2 = 0;
      double term = 0;
      double periodTotal = 0;
      double windowTotal = 0;
      double peakTotal = 0;
      double sqrtA = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceRebuild = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
       * eq. (2) on p.505: with the log price measured from the bar's open,
       * S1 = ln(H/O), I1 = ln(L/O) and X1 = ln(C/O), one bar's unbiased estimate
       * of its variance is S1(S1 - X1) + I1(I1 - X1), which is ln(H/C)ln(H/O) +
       * ln(L/C)ln(L/O). Eq. (3) is what sets this estimator apart: that
       * expectation is sigma^2 whatever the drift, so a bar that opens at its low
       * and closes at its high -- all drift, no dispersion -- reads exactly zero,
       * where a range-only estimator reads volatility.
       *
       * The paper stops there. The window mean, the root and the annual scale are
       * the convention of every implementation of it, not the authors'.
       */
      /* Once, so that optInAnnualization = 1.0 is an exact identity rather than a
       * multiply by a rounded 1.0, and so the per-bar and annualised outputs
       * differ by exactly this factor.
       */
      sqrtA = Math.sqrt(optInAnnualization);
      trailingIdx = startIdx - nbInitialElementNeeded;
      periodTotal = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         /* The two products are SEPARATE statements on purpose. Written as one
          * expression the generator's FMA detector fuses the first product into
          * the add (backends/fma.rs:406-433; a log call counts as a float factor
          * at :252-285), which moves 159 of 243 outputs on the corpus at n = 10
          * and buys nothing measurable. VWMA splits its product for the same
          * reason (vwma/vwma.c:78-81).
          *
          * The guard is the whole bar, tested exactly rather than against a fixed
          * band (#253): a bar with any price at or below zero contributes a 0.0
          * term and still counts toward the window's n. Zeroing only the products
          * that touch the bad price has no implementation behind it, and dropping
          * the bar from the window makes n data-dependent.
          */
         o = inOpen[j];
         h = inHigh[j];
         l = inLow[j];
         c = inClose[j];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
      }
      /* outReal may be any of the four input arrays: the output written on a bar
       * lands at or before the window's own trailing index, so every input slot
       * this loop still reads is one no write has reached yet.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceRebuild = 32 * optInTimePeriod;
      peakTotal = periodTotal;
      do {
         o = inOpen[i];
         h = inHigh[i];
         l = inLow[i];
         c = inClose[i];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
         peakTotal = (periodTotal > peakTotal) ? periodTotal : peakTotal;
         /* The sum this bar's output is taken from, before the trailing term is
          * removed for the next one.
          */
         windowTotal = periodTotal;
         o = inOpen[trailingIdx];
         h = inHigh[trailingIdx];
         l = inLow[trailingIdx];
         c = inClose[trailingIdx];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal -= term;
         trailingIdx += 1;
         /* Rebuild as a fresh window sum when the running sum has collapsed to
          * below 1e-6 of the largest it has held since the last rebuild, or at
          * least every 32 windows -- VAR's rule (var.c:101-113). Measured against
          * the PEAK, not the current sum: what a running sum of add-then-subtract
          * carries is rounding at the scale of the largest window it has seen, so
          * once a quiet stretch arrives the current sum can be nothing but that
          * rounding. On an all-flat window the rebuild restores an exact 0.0,
          * where a plain running sum leaves a residual that is negative about
          * forty per cent of the time -- and a negative sum under an
          * unconditional root is where the composition of shipped functions
          * produces NaN.
          */
         barsSinceRebuild -= 1;
         if( windowTotal < 0.000001 * peakTotal || barsSinceRebuild <= 0 ) {
            barsSinceRebuild = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            windowTotal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               o = inOpen[j];
               h = inHigh[j];
               l = inLow[j];
               c = inClose[j];
               if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
                  p1 = Math.log(h / c) * Math.log(h / o);
                  p2 = Math.log(l / c) * Math.log(l / o);
                  term = p1 + p2;
               } else {
                  term = 0.0;
               }
               windowTotal += term;
            }
            /* The rebuilt window becomes the carried state, with its trailing
             * term removed again so the two paths leave the same thing behind.
             */
            periodTotal = windowTotal;
            peakTotal = windowTotal;
            o = inOpen[windowStart];
            h = inHigh[windowStart];
            l = inLow[windowStart];
            c = inClose[windowStart];
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
               p1 = Math.log(h / c) * Math.log(h / o);
               p2 = Math.log(l / c) * Math.log(l / o);
               term = p1 + p2;
            } else {
               term = 0.0;
            }
            periodTotal -= term;
         }
         /* The divide, then the root, then the scale -- the spelling every
          * implementation of this estimator uses, and what makes A = 1.0 exact.
          *
          * A sum at or below zero answers 0.0 rather than reaching the root. A
          * fresh sum of terms from consistent bars cannot be negative, so this
          * only catches a bar whose high or low sits strictly inside its open and
          * close, which is not a bar the estimator is defined on. VAR floors its
          * variance for the same reason, so that STDDEV can root it
          * unconditionally (var.c:165-166).
          */
         if( windowTotal > 0.0 ) {
            outReal[outIdx] = sqrtA * Math.sqrt(windowTotal / (double)optInTimePeriod);
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx = outIdx + 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode rogerssatchellImpl( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               int optInTimePeriod,
                               double optInAnnualization,
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      double o = 0;
      double h = 0;
      double l = 0;
      double c = 0;
      double p1 = 0;
      double p2 = 0;
      double term = 0;
      double periodTotal = 0;
      double windowTotal = 0;
      double peakTotal = 0;
      double sqrtA = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceRebuild = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      sqrtA = Math.sqrt(optInAnnualization);
      trailingIdx = startIdx - nbInitialElementNeeded;
      periodTotal = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         o = (double)inOpen[j];
         h = (double)inHigh[j];
         l = (double)inLow[j];
         c = (double)inClose[j];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
      }
      i = startIdx;
      outIdx = 0;
      barsSinceRebuild = 32 * optInTimePeriod;
      peakTotal = periodTotal;
      do {
         o = (double)inOpen[i];
         h = (double)inHigh[i];
         l = (double)inLow[i];
         c = (double)inClose[i];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
         peakTotal = (periodTotal > peakTotal) ? periodTotal : peakTotal;
         windowTotal = periodTotal;
         o = (double)inOpen[trailingIdx];
         h = (double)inHigh[trailingIdx];
         l = (double)inLow[trailingIdx];
         c = (double)inClose[trailingIdx];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal -= term;
         trailingIdx += 1;
         barsSinceRebuild -= 1;
         if( windowTotal < 0.000001 * peakTotal || barsSinceRebuild <= 0 ) {
            barsSinceRebuild = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            windowTotal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               o = (double)inOpen[j];
               h = (double)inHigh[j];
               l = (double)inLow[j];
               c = (double)inClose[j];
               if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
                  p1 = Math.log(h / c) * Math.log(h / o);
                  p2 = Math.log(l / c) * Math.log(l / o);
                  term = p1 + p2;
               } else {
                  term = 0.0;
               }
               windowTotal += term;
            }
            periodTotal = windowTotal;
            peakTotal = windowTotal;
            o = (double)inOpen[windowStart];
            h = (double)inHigh[windowStart];
            l = (double)inLow[windowStart];
            c = (double)inClose[windowStart];
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
               p1 = Math.log(h / c) * Math.log(h / o);
               p2 = Math.log(l / c) * Math.log(l / o);
               term = p1 + p2;
            } else {
               term = 0.0;
            }
            periodTotal -= term;
         }
         if( windowTotal > 0.0 ) {
            outReal[outIdx] = sqrtA * Math.sqrt(windowTotal / (double)optInTimePeriod);
         } else {
            outReal[outIdx] = 0.0;
         }
         outIdx = outIdx + 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Rogers-Satchell volatility: a range-based estimator that reads one bar's
    * open, high, low and close as a single unbiased estimate of that bar's
    * variance, then reports the root of the mean over the last {@code n} bars,
    * scaled to periods per year. What separates it from the other range
    * estimators is that it is unbiased <b>whatever the drift</b>. A bar that
    * opens at its low and closes at its high has travelled in one direction and
    * dispersed nothing around that path, and this estimator reads it as exactly
    * zero, where Parkinson and Garman-Klass read a wide range as volatility.
    * The price of that is a blind spot of its own: the estimator has no
    * close-to-open term, so overnight gaps are invisible to it. Read the output
    * as a fraction in log-return units — not price units, not percent. At the
    * default {@code optInAnnualization} of 252 it is an annualised figure for
    * daily bars; pass 1 to leave the per-bar figure, 52 for weekly bars, 12 for
    * monthly.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/rogerssatchell">ta-lib.org/functions/rogerssatchell</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#rogerssatchellLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the window. Default 10, range 1
    *        to 100000. {@code n = 1} is the paper's own single-bar estimator (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInAnnualization Periods per year. Default 252, range 0 to
    *        {@code TA_REAL_MAX}. Pass 1 for the per-bar figure (default 252; minimum
    *        0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal Estimated volatility, in log-return units. Must hold at
    *        least {@code endIdx - max(startIdx, rogerssatchellLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
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
    * @see Core#natr
    * @see Core#var
    */
   public OutRange rogerssatchell( int startIdx,
                                   int endIdx,
                                   double inOpen[],
                                   double inHigh[],
                                   double inLow[],
                                   double inClose[],
                                   int optInTimePeriod,
                                   double optInAnnualization,
                                   double outReal[] )
   {
      requireIndexRange("ROGERSSATCHELL", startIdx, endIdx);
      int guardStart = clampedStart("ROGERSSATCHELL", startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ROGERSSATCHELL", "inOpen", inOpen, guardInLen);
      requireLength("ROGERSSATCHELL", "inHigh", inHigh, guardInLen);
      requireLength("ROGERSSATCHELL", "inLow", inLow, guardInLen);
      requireLength("ROGERSSATCHELL", "inClose", inClose, guardInLen);
      requireLength("ROGERSSATCHELL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = rogerssatchellImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ROGERSSATCHELL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rogers-Satchell volatility: a range-based estimator that reads one bar's
    * open, high, low and close as a single unbiased estimate of that bar's
    * variance, then reports the root of the mean over the last {@code n} bars,
    * scaled to periods per year. What separates it from the other range
    * estimators is that it is unbiased <b>whatever the drift</b>. A bar that
    * opens at its low and closes at its high has travelled in one direction and
    * dispersed nothing around that path, and this estimator reads it as exactly
    * zero, where Parkinson and Garman-Klass read a wide range as volatility.
    * The price of that is a blind spot of its own: the estimator has no
    * close-to-open term, so overnight gaps are invisible to it. Read the output
    * as a fraction in log-return units — not price units, not percent. At the
    * default {@code optInAnnualization} of 252 it is an annualised figure for
    * daily bars; pass 1 to leave the per-bar figure, 52 for weekly bars, 12 for
    * monthly.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/rogerssatchell">ta-lib.org/functions/rogerssatchell</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#rogerssatchellLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the window. Default 10, range 1
    *        to 100000. {@code n = 1} is the paper's own single-bar estimator (default
    *        10; range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInAnnualization Periods per year. Default 252, range 0 to
    *        {@code TA_REAL_MAX}. Pass 1 for the per-bar figure (default 252; minimum
    *        0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal Estimated volatility, in log-return units. Must hold at
    *        least {@code endIdx - max(startIdx, rogerssatchellLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
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
    * @see Core#natr
    * @see Core#var
    */
   public OutRange rogerssatchell( int startIdx,
                                   int endIdx,
                                   float inOpen[],
                                   float inHigh[],
                                   float inLow[],
                                   float inClose[],
                                   int optInTimePeriod,
                                   double optInAnnualization,
                                   double outReal[] )
   {
      requireIndexRange("ROGERSSATCHELL", startIdx, endIdx);
      int guardStart = clampedStart("ROGERSSATCHELL", startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ROGERSSATCHELL", "inOpen", inOpen, guardInLen);
      requireLength("ROGERSSATCHELL", "inHigh", inHigh, guardInLen);
      requireLength("ROGERSSATCHELL", "inLow", inLow, guardInLen);
      requireLength("ROGERSSATCHELL", "inClose", inClose, guardInLen);
      requireLength("ROGERSSATCHELL", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = rogerssatchellImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ROGERSSATCHELL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ROGERSSATCHELL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#rogerssatchell} over the same series.
    * Open with {@link Core#rogerssatchellOpen}; there is no close — the handle is
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
   public static final class RogerssatchellStream {
      private Core core;
      private int optInTimePeriod;
      private double optInAnnualization;
      private double periodTotal;
      private double peakTotal;
      private double sqrtA;
      private int trailingIdx;
      private int nbInitialElementNeeded;
      private int barsSinceRebuild;
      private int j;
      private int windowStart;
      private int i;
      private int xMask;
      private double[] x_inOpen;
      private double[] x_inHigh;
      private double[] x_inLow;
      private double[] x_inClose;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private RogerssatchellStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#rogerssatchell} reports over the same bars: the
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
            throw failure("ROGERSSATCHELL advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private RogerssatchellStream( RogerssatchellStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInAnnualization = other.optInAnnualization;
         this.periodTotal = other.periodTotal;
         this.peakTotal = other.peakTotal;
         this.sqrtA = other.sqrtA;
         this.trailingIdx = other.trailingIdx;
         this.nbInitialElementNeeded = other.nbInitialElementNeeded;
         this.barsSinceRebuild = other.barsSinceRebuild;
         this.j = other.j;
         this.windowStart = other.windowStart;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inOpen = other.x_inOpen.clone();
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
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
      public double update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ROGERSSATCHELL update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("ROGERSSATCHELL update", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.rogerssatchellStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      public double peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("ROGERSSATCHELL peek", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         RogerssatchellStream sp = this;
         double o = 0.0;
         double h = 0.0;
         double l = 0.0;
         double c = 0.0;
         double p1 = 0.0;
         double p2 = 0.0;
         double term = 0.0;
         double windowTotal = 0.0;
         int barsSinceRebuild = sp.barsSinceRebuild;
         double cur_outReal = 0.0;
         int j = sp.j;
         double peakTotal = sp.peakTotal;
         double periodTotal = sp.periodTotal;
         int trailingIdx = sp.trailingIdx;
         int windowStart = sp.windowStart;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         int pkSlot3 = -1;
         double pkVal3 = 0.0;
         pkSlot0 = sp.i & sp.xMask;
         pkVal0 = inOpen;
         pkSlot1 = sp.i & sp.xMask;
         pkVal1 = inHigh;
         pkSlot2 = sp.i & sp.xMask;
         pkVal2 = inLow;
         pkSlot3 = sp.i & sp.xMask;
         pkVal3 = inClose;
         o = ((sp.i & sp.xMask) != pkSlot0) ? sp.x_inOpen[sp.i & sp.xMask] : pkVal0;
         h = ((sp.i & sp.xMask) != pkSlot1) ? sp.x_inHigh[sp.i & sp.xMask] : pkVal1;
         l = ((sp.i & sp.xMask) != pkSlot2) ? sp.x_inLow[sp.i & sp.xMask] : pkVal2;
         c = ((sp.i & sp.xMask) != pkSlot3) ? sp.x_inClose[sp.i & sp.xMask] : pkVal3;
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
         peakTotal = (periodTotal > peakTotal) ? periodTotal : peakTotal;
         /* The sum this bar's output is taken from, before the trailing term is
          * removed for the next one.
          */
         windowTotal = periodTotal;
         o = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inOpen[trailingIdx & sp.xMask] : pkVal0;
         h = ((trailingIdx & sp.xMask) != pkSlot1) ? sp.x_inHigh[trailingIdx & sp.xMask] : pkVal1;
         l = ((trailingIdx & sp.xMask) != pkSlot2) ? sp.x_inLow[trailingIdx & sp.xMask] : pkVal2;
         c = ((trailingIdx & sp.xMask) != pkSlot3) ? sp.x_inClose[trailingIdx & sp.xMask] : pkVal3;
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal -= term;
         trailingIdx += 1;
         /* Rebuild as a fresh window sum when the running sum has collapsed to
          * below 1e-6 of the largest it has held since the last rebuild, or at
          * least every 32 windows -- VAR's rule (var.c:101-113). Measured against
          * the PEAK, not the current sum: what a running sum of add-then-subtract
          * carries is rounding at the scale of the largest window it has seen, so
          * once a quiet stretch arrives the current sum can be nothing but that
          * rounding. On an all-flat window the rebuild restores an exact 0.0,
          * where a plain running sum leaves a residual that is negative about
          * forty per cent of the time -- and a negative sum under an
          * unconditional root is where the composition of shipped functions
          * produces NaN.
          */
         barsSinceRebuild -= 1;
         if( windowTotal < 0.000001 * peakTotal || barsSinceRebuild <= 0 ) {
            barsSinceRebuild = 32 * sp.optInTimePeriod;
            windowStart = sp.i - sp.nbInitialElementNeeded;
            windowTotal = 0.0;
            for( j = windowStart; j <= sp.i; j += 1 ) {
               o = ((j & sp.xMask) != pkSlot0) ? sp.x_inOpen[j & sp.xMask] : pkVal0;
               h = ((j & sp.xMask) != pkSlot1) ? sp.x_inHigh[j & sp.xMask] : pkVal1;
               l = ((j & sp.xMask) != pkSlot2) ? sp.x_inLow[j & sp.xMask] : pkVal2;
               c = ((j & sp.xMask) != pkSlot3) ? sp.x_inClose[j & sp.xMask] : pkVal3;
               if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
                  p1 = Math.log(h / c) * Math.log(h / o);
                  p2 = Math.log(l / c) * Math.log(l / o);
                  term = p1 + p2;
               } else {
                  term = 0.0;
               }
               windowTotal += term;
            }
            /* The rebuilt window becomes the carried state, with its trailing
             * term removed again so the two paths leave the same thing behind.
             */
            periodTotal = windowTotal;
            peakTotal = windowTotal;
            o = ((windowStart & sp.xMask) != pkSlot0) ? sp.x_inOpen[windowStart & sp.xMask] : pkVal0;
            h = ((windowStart & sp.xMask) != pkSlot1) ? sp.x_inHigh[windowStart & sp.xMask] : pkVal1;
            l = ((windowStart & sp.xMask) != pkSlot2) ? sp.x_inLow[windowStart & sp.xMask] : pkVal2;
            c = ((windowStart & sp.xMask) != pkSlot3) ? sp.x_inClose[windowStart & sp.xMask] : pkVal3;
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
               p1 = Math.log(h / c) * Math.log(h / o);
               p2 = Math.log(l / c) * Math.log(l / o);
               term = p1 + p2;
            } else {
               term = 0.0;
            }
            periodTotal -= term;
         }
         /* The divide, then the root, then the scale -- the spelling every
          * implementation of this estimator uses, and what makes A = 1.0 exact.
          *
          * A sum at or below zero answers 0.0 rather than reaching the root. A
          * fresh sum of terms from consistent bars cannot be negative, so this
          * only catches a bar whose high or low sits strictly inside its open and
          * close, which is not a bar the estimator is defined on. VAR floors its
          * variance for the same reason, so that STDDEV can root it
          * unconditionally (var.c:165-166).
          */
         if( windowTotal > 0.0 ) {
            cur_outReal = sp.sqrtA * Math.sqrt(windowTotal / (double)sp.optInTimePeriod);
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
      public RogerssatchellStream clone() {
         return new RogerssatchellStream(this);
      }
   }
   private void rogerssatchellStepImpl( RogerssatchellStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double o = 0.0;
      double h = 0.0;
      double l = 0.0;
      double c = 0.0;
      double p1 = 0.0;
      double p2 = 0.0;
      double term = 0.0;
      double windowTotal = 0.0;
      sp.x_inOpen[sp.i & sp.xMask] = inOpen;
      sp.x_inHigh[sp.i & sp.xMask] = inHigh;
      sp.x_inLow[sp.i & sp.xMask] = inLow;
      sp.x_inClose[sp.i & sp.xMask] = inClose;
      o = sp.x_inOpen[sp.i & sp.xMask];
      h = sp.x_inHigh[sp.i & sp.xMask];
      l = sp.x_inLow[sp.i & sp.xMask];
      c = sp.x_inClose[sp.i & sp.xMask];
      if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
         p1 = Math.log(h / c) * Math.log(h / o);
         p2 = Math.log(l / c) * Math.log(l / o);
         term = p1 + p2;
      } else {
         term = 0.0;
      }
      sp.periodTotal += term;
      sp.peakTotal = (sp.periodTotal > sp.peakTotal) ? sp.periodTotal : sp.peakTotal;
      /* The sum this bar's output is taken from, before the trailing term is
       * removed for the next one.
       */
      windowTotal = sp.periodTotal;
      o = sp.x_inOpen[sp.trailingIdx & sp.xMask];
      h = sp.x_inHigh[sp.trailingIdx & sp.xMask];
      l = sp.x_inLow[sp.trailingIdx & sp.xMask];
      c = sp.x_inClose[sp.trailingIdx & sp.xMask];
      if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
         p1 = Math.log(h / c) * Math.log(h / o);
         p2 = Math.log(l / c) * Math.log(l / o);
         term = p1 + p2;
      } else {
         term = 0.0;
      }
      sp.periodTotal -= term;
      sp.trailingIdx += 1;
      /* Rebuild as a fresh window sum when the running sum has collapsed to
       * below 1e-6 of the largest it has held since the last rebuild, or at
       * least every 32 windows -- VAR's rule (var.c:101-113). Measured against
       * the PEAK, not the current sum: what a running sum of add-then-subtract
       * carries is rounding at the scale of the largest window it has seen, so
       * once a quiet stretch arrives the current sum can be nothing but that
       * rounding. On an all-flat window the rebuild restores an exact 0.0,
       * where a plain running sum leaves a residual that is negative about
       * forty per cent of the time -- and a negative sum under an
       * unconditional root is where the composition of shipped functions
       * produces NaN.
       */
      sp.barsSinceRebuild -= 1;
      if( windowTotal < 0.000001 * sp.peakTotal || sp.barsSinceRebuild <= 0 ) {
         sp.barsSinceRebuild = 32 * sp.optInTimePeriod;
         sp.windowStart = sp.i - sp.nbInitialElementNeeded;
         windowTotal = 0.0;
         for( sp.j = sp.windowStart; sp.j <= sp.i; sp.j += 1 ) {
            o = sp.x_inOpen[sp.j & sp.xMask];
            h = sp.x_inHigh[sp.j & sp.xMask];
            l = sp.x_inLow[sp.j & sp.xMask];
            c = sp.x_inClose[sp.j & sp.xMask];
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
               p1 = Math.log(h / c) * Math.log(h / o);
               p2 = Math.log(l / c) * Math.log(l / o);
               term = p1 + p2;
            } else {
               term = 0.0;
            }
            windowTotal += term;
         }
         /* The rebuilt window becomes the carried state, with its trailing
          * term removed again so the two paths leave the same thing behind.
          */
         sp.periodTotal = windowTotal;
         sp.peakTotal = windowTotal;
         o = sp.x_inOpen[sp.windowStart & sp.xMask];
         h = sp.x_inHigh[sp.windowStart & sp.xMask];
         l = sp.x_inLow[sp.windowStart & sp.xMask];
         c = sp.x_inClose[sp.windowStart & sp.xMask];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         sp.periodTotal -= term;
      }
      /* The divide, then the root, then the scale -- the spelling every
       * implementation of this estimator uses, and what makes A = 1.0 exact.
       *
       * A sum at or below zero answers 0.0 rather than reaching the root. A
       * fresh sum of terms from consistent bars cannot be negative, so this
       * only catches a bar whose high or low sits strictly inside its open and
       * close, which is not a bar the estimator is defined on. VAR floors its
       * variance for the same reason, so that STDDEV can root it
       * unconditionally (var.c:165-166).
       */
      if( windowTotal > 0.0 ) {
         sp.cur_outReal = sp.sqrtA * Math.sqrt(windowTotal / (double)sp.optInTimePeriod);
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.i += 1;
   }
   private RetCode rogerssatchellOpenImpl( RogerssatchellStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double o = 0;
      double h = 0;
      double l = 0;
      double c = 0;
      double p1 = 0;
      double p2 = 0;
      double term = 0;
      double periodTotal = 0;
      double windowTotal = 0;
      double peakTotal = 0;
      double sqrtA = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int windowStart = 0;
      int nbInitialElementNeeded = 0;
      int barsSinceRebuild = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInAnnualization == REAL_DEFAULT ) {
         optInAnnualization = 2.52e2;
      } else if( !(optInAnnualization >= 0e0 && optInAnnualization <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Rogers and Satchell, The Annals of Applied Probability 1(4):504-512 (1991),
       * eq. (2) on p.505: with the log price measured from the bar's open,
       * S1 = ln(H/O), I1 = ln(L/O) and X1 = ln(C/O), one bar's unbiased estimate
       * of its variance is S1(S1 - X1) + I1(I1 - X1), which is ln(H/C)ln(H/O) +
       * ln(L/C)ln(L/O). Eq. (3) is what sets this estimator apart: that
       * expectation is sigma^2 whatever the drift, so a bar that opens at its low
       * and closes at its high -- all drift, no dispersion -- reads exactly zero,
       * where a range-only estimator reads volatility.
       *
       * The paper stops there. The window mean, the root and the annual scale are
       * the convention of every implementation of it, not the authors'.
       */
      /* Once, so that optInAnnualization = 1.0 is an exact identity rather than a
       * multiply by a rounded 1.0, and so the per-bar and annualised outputs
       * differ by exactly this factor.
       */
      sqrtA = Math.sqrt(optInAnnualization);
      trailingIdx = startIdx - nbInitialElementNeeded;
      periodTotal = 0.0;
      for( j = trailingIdx; j < startIdx; j += 1 ) {
         /* The two products are SEPARATE statements on purpose. Written as one
          * expression the generator's FMA detector fuses the first product into
          * the add (backends/fma.rs:406-433; a log call counts as a float factor
          * at :252-285), which moves 159 of 243 outputs on the corpus at n = 10
          * and buys nothing measurable. VWMA splits its product for the same
          * reason (vwma/vwma.c:78-81).
          *
          * The guard is the whole bar, tested exactly rather than against a fixed
          * band (#253): a bar with any price at or below zero contributes a 0.0
          * term and still counts toward the window's n. Zeroing only the products
          * that touch the bad price has no implementation behind it, and dropping
          * the bar from the window makes n data-dependent.
          */
         o = inOpen[j];
         h = inHigh[j];
         l = inLow[j];
         c = inClose[j];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
      }
      /* outReal may be any of the four input arrays: the output written on a bar
       * lands at or before the window's own trailing index, so every input slot
       * this loop still reads is one no write has reached yet.
       */
      i = startIdx;
      outIdx = 0;
      barsSinceRebuild = 32 * optInTimePeriod;
      peakTotal = periodTotal;
      do {
         o = inOpen[i];
         h = inHigh[i];
         l = inLow[i];
         c = inClose[i];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal += term;
         peakTotal = (periodTotal > peakTotal) ? periodTotal : peakTotal;
         /* The sum this bar's output is taken from, before the trailing term is
          * removed for the next one.
          */
         windowTotal = periodTotal;
         o = inOpen[trailingIdx];
         h = inHigh[trailingIdx];
         l = inLow[trailingIdx];
         c = inClose[trailingIdx];
         if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
            p1 = Math.log(h / c) * Math.log(h / o);
            p2 = Math.log(l / c) * Math.log(l / o);
            term = p1 + p2;
         } else {
            term = 0.0;
         }
         periodTotal -= term;
         trailingIdx += 1;
         /* Rebuild as a fresh window sum when the running sum has collapsed to
          * below 1e-6 of the largest it has held since the last rebuild, or at
          * least every 32 windows -- VAR's rule (var.c:101-113). Measured against
          * the PEAK, not the current sum: what a running sum of add-then-subtract
          * carries is rounding at the scale of the largest window it has seen, so
          * once a quiet stretch arrives the current sum can be nothing but that
          * rounding. On an all-flat window the rebuild restores an exact 0.0,
          * where a plain running sum leaves a residual that is negative about
          * forty per cent of the time -- and a negative sum under an
          * unconditional root is where the composition of shipped functions
          * produces NaN.
          */
         barsSinceRebuild -= 1;
         if( windowTotal < 0.000001 * peakTotal || barsSinceRebuild <= 0 ) {
            barsSinceRebuild = 32 * optInTimePeriod;
            windowStart = i - nbInitialElementNeeded;
            windowTotal = 0.0;
            for( j = windowStart; j <= i; j += 1 ) {
               o = inOpen[j];
               h = inHigh[j];
               l = inLow[j];
               c = inClose[j];
               if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
                  p1 = Math.log(h / c) * Math.log(h / o);
                  p2 = Math.log(l / c) * Math.log(l / o);
                  term = p1 + p2;
               } else {
                  term = 0.0;
               }
               windowTotal += term;
            }
            /* The rebuilt window becomes the carried state, with its trailing
             * term removed again so the two paths leave the same thing behind.
             */
            periodTotal = windowTotal;
            peakTotal = windowTotal;
            o = inOpen[windowStart];
            h = inHigh[windowStart];
            l = inLow[windowStart];
            c = inClose[windowStart];
            if( o > 0.0 && h > 0.0 && l > 0.0 && c > 0.0 ) {
               p1 = Math.log(h / c) * Math.log(h / o);
               p2 = Math.log(l / c) * Math.log(l / o);
               term = p1 + p2;
            } else {
               term = 0.0;
            }
            periodTotal -= term;
         }
         /* The divide, then the root, then the scale -- the spelling every
          * implementation of this estimator uses, and what makes A = 1.0 exact.
          *
          * A sum at or below zero answers 0.0 rather than reaching the root. A
          * fresh sum of terms from consistent bars cannot be negative, so this
          * only catches a bar whose high or low sits strictly inside its open and
          * close, which is not a bar the estimator is defined on. VAR floors its
          * variance for the same reason, so that STDDEV can root it
          * unconditionally (var.c:165-166).
          */
         if( windowTotal > 0.0 ) {
            outReal[outIdx * outStride] = sqrtA * Math.sqrt(windowTotal / (double)optInTimePeriod);
         } else {
            outReal[outIdx * outStride] = 0.0;
         }
         outIdx = outIdx + 1;
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inOpen = new double[physX];
      double[] capX_inHigh = new double[physX];
      double[] capX_inLow = new double[physX];
      double[] capX_inClose = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inOpen[fillJ & (physX - 1)] = inOpen[fillJ];
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
         capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInAnnualization = optInAnnualization;
      sp.periodTotal = periodTotal;
      sp.peakTotal = peakTotal;
      sp.sqrtA = sqrtA;
      sp.trailingIdx = trailingIdx;
      sp.nbInitialElementNeeded = nbInitialElementNeeded;
      sp.barsSinceRebuild = barsSinceRebuild;
      sp.j = j;
      sp.windowStart = windowStart;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inOpen = capX_inOpen;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* rogerssatchellOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RogerssatchellStream rogerssatchellOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInAnnualization, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RogerssatchellStream sp = new RogerssatchellStream(this);
      RetCode retCode = rogerssatchellOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ROGERSSATCHELL openAndFill", inOpen.length, startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization));
      }
      throw streamFailure("ROGERSSATCHELL openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind rogerssatchellOpen (composition seam). */
   RogerssatchellStream rogerssatchellOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInAnnualization )
   {
      RogerssatchellStream sp = new RogerssatchellStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rogerssatchellOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ROGERSSATCHELL open", inOpen.length, startIdx, rogerssatchellLookback(optInTimePeriod, optInAnnualization));
      }
      throw streamFailure("ROGERSSATCHELL open", retCode);
   }
   /**
    * Open a live ROGERSSATCHELL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#rogerssatchell} at that bar.
    * <p>The history must hold at least {@code rogerssatchellLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public RogerssatchellStream rogerssatchellOpen( double inOpen[], double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInAnnualization )
   {
      requireArgument("ROGERSSATCHELL open", "inOpen", inOpen);
      requireHistory("ROGERSSATCHELL open", inOpen.length);
      requireArgument("ROGERSSATCHELL open", "inHigh", inHigh);
      requireArgument("ROGERSSATCHELL open", "inLow", inLow);
      requireArgument("ROGERSSATCHELL open", "inClose", inClose);
      requireHistoryLength("ROGERSSATCHELL open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("ROGERSSATCHELL open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("ROGERSSATCHELL open", "inClose", inClose.length, inOpen.length);
      return rogerssatchellOpenInternal(inOpen, inHigh, inLow, inClose, 0, optInTimePeriod, optInAnnualization);
   }
   /**
    * {@link Core#rogerssatchellOpen} that also fills the output array(s) bit-identically
    * to {@link Core#rogerssatchell} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link RogerssatchellStream#outRange()}.
    */
   public RogerssatchellStream rogerssatchellOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInAnnualization, double outReal[] )
   {
      requireArgument("ROGERSSATCHELL openAndFill", "inOpen", inOpen);
      requireHistory("ROGERSSATCHELL openAndFill", inOpen.length);
      requireArgument("ROGERSSATCHELL openAndFill", "inHigh", inHigh);
      requireArgument("ROGERSSATCHELL openAndFill", "inLow", inLow);
      requireArgument("ROGERSSATCHELL openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ROGERSSATCHELL openAndFill", inOpen.length, rogerssatchellLookback(optInTimePeriod, optInAnnualization));
      requireHistoryLength("ROGERSSATCHELL openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("ROGERSSATCHELL openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("ROGERSSATCHELL openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("ROGERSSATCHELL openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw streamFailure("ROGERSSATCHELL openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rogerssatchellOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, optInTimePeriod, optInAnnualization, outBegIdx, outNBElement, outReal);
   }
