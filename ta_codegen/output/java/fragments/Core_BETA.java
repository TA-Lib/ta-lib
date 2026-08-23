/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MW       Michael Williamson
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  122006 MW    Initial Version
 *  071626 MF,CC Fix reversed inReal0/inReal1 roles in the algorithm
 *               description: inReal0 holds the index prices and inReal1
 *               the stock prices (SourceForge bug 98).
 *  082326 MF    Fix #242. Cancellation-free regression sums (shifted returns
 *               + reseed) and a scale-relative denominator test.
 *  082326 MF,CC #242 follow-up: restore TA_VAR's outlier trigger, at 1e3,
 *               on BOTH axes -- the output reads S_xy and S_y too.
 */

   /**
    * Number of leading input bars {@link Core#BETA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Rolling window length (number of returns) for the
    *        regression sums (default 5; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int BETA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode BETA_Impl( int startIdx,
                      int endIdx,
                      double inReal0[],
                      double inReal1[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double shift_x = 0;
      double shift_y = 0;
      double denom = 0;
      double denom_scale = 0;
      double prev_x = 0;
      double leaving_xx = 0;
      double leaving_yy = 0;
      double S_yy = 0;
      double prev_y = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
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
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      shift_x = 0.0;
      shift_y = 0.0;
      denom = 0.0;
      denom_scale = 0.0;
      prev_x = 0.0;
      leaving_xx = 0.0;
      leaving_yy = 0.0;
      S_yy = 0.0;
      prev_y = 0.0;
      n = 0.0;
      /* sum of x * x */
      /* sum of x * y */
      /* sum of x */
      /* sum of y */
      /* the last price read from inReal0 */
      /* the last price read from inReal1 */
      /* same as last_price_x except used to remove elements from the trailing summation */
      /* same as last_price_y except used to remove elements from the trailing summation */
      /* temporary variable */
      /* origin the x returns are measured against */
      /* origin the y returns are measured against */
      /* n*S_xx - S_x*S_x, the regression denominator */
      /* n*S_xx, the scale denom is extracted from */
      /* price walked forward when rebuilding the window */
      /* squared x deviation the previous bar removed */
      /* squared y deviation the previous bar removed */
      /* sum of y * y, carried ONLY for the outlier trigger */
      /* the 'x' value, which is the last change between values in inReal0 */
      /* the 'y' value, which is the last change between values in inReal1 */
      /* DESCRIPTION OF ALGORITHM:
       *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The index prices
       *   are given in inReal0 and the stock prices are given in inReal1. The size of these vectors
       *   should be equal. The algorithm is to calculate the change between prices in both vectors
       *   and then 'plot' these changes are points in the Euclidean plane. The x value of the point
       *   is market return and the y value is the security return. The beta value is the slope of a
       *   linear regression through these points. A beta of 1 is simple the line y=x, so the stock
       *   varies percisely with the market. A beta of less than one means the stock varies less than
       *   the market and a beta of more than one means the stock varies more than market. A related
       *   value is the Alpha value (see TA_ALPHA) which is the Y-intercept of the same linear regression.
       */
      /* Validate the calculation method type and
       * identify the minimum number of input
       * consume before the first value is output..
       */
      nbInitialElementNeeded = optInTimePeriod;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Consume first input. */
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      /* Measure the returns against a shift near the window, as TA_VAR does for
       * its values (#118) and TA_CORREL for its prices (#242).
       *
       * Returns are near-zero-mean, so on a series that jitters this changes
       * little. It is decisive on a series that DRIFTS: a steadily rising price
       * gives near-identical returns, S_x*S_x then equals n*S_xx to every digit,
       * and the denominator n*S_xx - S_x*S_x is left as pure rounding noise --
       * measured at +/-2e-16 of its own scale, sign included. Wilkinson's BIG and
       * LITTLE are exactly that shape, and beta of such a series against itself
       * came back 0 instead of 1.
       *
       * Anchor on the window's first return here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      i = ++trailingIdx;
      if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
         shift_x = (inReal0[i] - last_price_x) / last_price_x;
      }
      if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
         shift_y = (inReal1[i] - last_price_y) / last_price_y;
      }
      while( i < startIdx ) {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      /* First output always start at index zero */
      n = (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         denom_scale = n * S_xx;
         denom = denom_scale - S_x * S_x;
         /* Re-anchor and rebuild when the shift has gone stale. The same three
          * triggers as TA_VAR: the denominator has shrunk below 1e-6 of the scale
          * it is extracted from; OR the return that just left sat so far from the
          * shift that its squared term dwarfs what remains; OR at least every 32
          * windows.
          *
          * The outlier trigger earns its multiply and compare here, contrary to
          * what "returns are stationary" suggests: a bad tick makes one return
          * enormous, the ordinary ones fall below its ulp and are never really
          * added, and when it leaves the subtraction takes back a term they were
          * never part of. The residue is a consistent OFFSET, so the cancellation
          * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
          * the periodic re-anchor recovers, up to 32*period bars later. Measured
          * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
          * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
          * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
          * is a corpus figure, not a bound. Isolated against the same body without
          * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
          * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
          * The cost is the reseed it triggers, so it tracks the fire rate; on data
          * that never triggers it, the compare is free.
          *
          * BOTH axes are watched, and the y one is not redundant. The denominator
          * is x-only, so it is tempting to conclude -- as an earlier draft of this
          * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
          * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
          * the x side able to see it. Measured on test_beta_outlier_transit's own
          * ladder with the spike moved from px to py: 12 of 24 rungs fail without
          * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
          * experiment that found it inert was run on an x-only corpus, where it is
          * inert by construction. TA_CORREL, fixed by the same #242 work, watches
          * both from the start; this brings BETA level. S_yy exists only to scale
          * this test -- nothing else reads it.
          *
          * The threshold is 1e3 where TA_VAR uses 1e6, because a return amplifies:
          * a tick multiplying the price by k puts k-1 into the return and (k-1)^2
          * into S_xx, so the ratio when that term leaves lands an order or two
          * below the value-scale case var.c was tuned on. At 1e6 a 1e5 tick slips
          * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
          * Pinned by test_beta_outlier_transit.
          *
          * Reading the window here is safe when outReal aliases an input: the
          * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
          * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( denom < 0.000001 * denom_scale || leaving_xx > 1000.0 * S_xx || leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = trailingIdx;
            /* Walk the window forward from the price the trailing cursor already
             * carries. A return needs its predecessor, and reading inReal[j-1]
             * would reach one slot BEFORE the window -- which the batch can do and
             * a streaming ring sized for the window cannot. trailing_last_price_*
             * IS that predecessor, so carrying it forward keeps every read inside
             * [trailingIdx, i-1] and the two paths stay identical.
             */
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            tmp_real = 0.0;
            shift_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  tmp_real += (inReal0[j] - prev_x) / prev_x;
               }
               prev_x = inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  shift_y += (inReal1[j] - prev_y) / prev_y;
               }
               prev_y = inReal1[j];
            }
            shift_x = tmp_real / n;
            shift_y = shift_y / n;
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            S_xx = 0.0;
            S_yy = 0.0;
            S_xy = 0.0;
            S_x = 0.0;
            S_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  x = (inReal0[j] - prev_x) / prev_x - shift_x;
               } else {
                  x = 0 - shift_x;
               }
               prev_x = inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  y = (inReal1[j] - prev_y) / prev_y - shift_y;
               } else {
                  y = 0 - shift_y;
               }
               prev_y = inReal1[j];
               S_xx += x * x;
               S_yy += y * y;
               S_xy += x * y;
               S_x += x;
               S_y += y;
            }
            denom_scale = n * S_xx;
            denom = denom_scale - S_x * S_x;
            /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * whose returns are all the same value. Enforce the invariant HERE and
             * not at the divide: a negative denom always reseeds on the same bar
             * (it makes the first trigger true whenever denom_scale is positive,
             * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
             * divide below can rely on it being >= 0.
             */
            if( denom < 0.0 ) {
               denom = 0.0;
            }
         }
         /* Always read the trailing before writing the output because the input and output
          * buffer can be the same.
          */
         tmp_real = inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = inReal1[trailingIdx];
         trailingIdx += 1;
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         trailing_last_price_y = tmp_real;
         /* Write the output.
          *
          * The denominator is tested against ITS OWN scale, not a fixed band: it
          * is quadratic in the return volatility, so an absolute 1e-14 threshold
          * stops meaning "the regressor does not vary" and starts meaning "the
          * returns are small". The literal is TA_EPSILON, and the plain `>` also
          * rejects a negative denominator rather than dividing by it.
          */
         if( denom > 0.00000000000001 * denom_scale ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / denom;
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Remove the calculation starting with the trailingIdx. */
         leaving_xx = x * x;
         leaving_yy = y * y;
         S_xx -= x * x;
         S_yy -= y * y;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode BETA_Impl( int startIdx,
                      int endIdx,
                      float inReal0[],
                      float inReal1[],
                      int optInTimePeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double shift_x = 0;
      double shift_y = 0;
      double denom = 0;
      double denom_scale = 0;
      double prev_x = 0;
      double leaving_xx = 0;
      double leaving_yy = 0;
      double S_yy = 0;
      double prev_y = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
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
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      shift_x = 0.0;
      shift_y = 0.0;
      denom = 0.0;
      denom_scale = 0.0;
      prev_x = 0.0;
      leaving_xx = 0.0;
      leaving_yy = 0.0;
      S_yy = 0.0;
      prev_y = 0.0;
      n = 0.0;
      nbInitialElementNeeded = optInTimePeriod;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = (double)inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = (double)inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      i = ++trailingIdx;
      if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
         shift_x = ((double)inReal0[i] - last_price_x) / last_price_x;
      }
      if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
         shift_y = ((double)inReal1[i] - last_price_y) / last_price_y;
      }
      while( i < startIdx ) {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      n = (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         denom_scale = n * S_xx;
         denom = denom_scale - S_x * S_x;
         barsSinceReseed -= 1;
         if( denom < 0.000001 * denom_scale || leaving_xx > 1000.0 * S_xx || leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = trailingIdx;
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            tmp_real = 0.0;
            shift_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  tmp_real += ((double)inReal0[j] - prev_x) / prev_x;
               }
               prev_x = (double)inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  shift_y += ((double)inReal1[j] - prev_y) / prev_y;
               }
               prev_y = (double)inReal1[j];
            }
            shift_x = tmp_real / n;
            shift_y = shift_y / n;
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            S_xx = 0.0;
            S_yy = 0.0;
            S_xy = 0.0;
            S_x = 0.0;
            S_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  x = ((double)inReal0[j] - prev_x) / prev_x - shift_x;
               } else {
                  x = 0 - shift_x;
               }
               prev_x = (double)inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  y = ((double)inReal1[j] - prev_y) / prev_y - shift_y;
               } else {
                  y = 0 - shift_y;
               }
               prev_y = (double)inReal1[j];
               S_xx += x * x;
               S_yy += y * y;
               S_xy += x * y;
               S_x += x;
               S_y += y;
            }
            denom_scale = n * S_xx;
            denom = denom_scale - S_x * S_x;
            if( denom < 0.0 ) {
               denom = 0.0;
            }
         }
         tmp_real = (double)inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = (double)inReal1[trailingIdx];
         trailingIdx += 1;
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         trailing_last_price_y = tmp_real;
         if( denom > 0.00000000000001 * denom_scale ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / denom;
         } else {
            outReal[outIdx++] = 0.0;
         }
         leaving_xx = x * x;
         leaving_yy = y * y;
         S_xx -= x * x;
         S_yy -= y * y;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Beta: the slope of a least-squares linear regression of one series'
    * percentage returns (y, from inReal1) against another's (x, from inReal0)
    * over a rolling window. Measures how much a security moves relative to a
    * market index. Beta = 1 moves with the index; &lt; 1 less volatile, &gt; 1
    * more volatile.
    * <p><b>Formula</b>
    * <pre>{@code
    * Per-bar returns: $x_i=(p^0_i-p^0_{i-1})/p^0_{i-1}$ from inReal0, $y_i=(p^1_i-p^1_{i-1})/p^1_{i-1}$ from inReal1. With $n$=period over the window: $\beta = \dfrac{n\,S_{xy}-S_x S_y}{n\,S_{xx}-S_x^2}$, where $S_{xx}=\sum x^2,\ S_{xy}=\sum xy,\ S_x=\sum x,\ S_y=\sum y$.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#BETA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Series whose returns are the regression x (market/index)
    * @param inReal1 Series whose returns are the regression y (security)
    * @param optInTimePeriod Rolling window length (number of returns) for the
    *        regression sums (default 5; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Beta: regression slope of inReal1-returns on
    *        inReal0-returns. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CORREL
    * @see Core#LINEARREG_SLOPE
    * @see Core#VAR
    * @see Core#STDDEV
    */
   public OutRange BETA( int startIdx,
                         int endIdx,
                         double inReal0[],
                         double inReal1[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("BETA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, BETA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BETA", "inReal0", inReal0, guardInLen);
      requireLength("BETA", "inReal1", inReal1, guardInLen);
      requireLength("BETA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = BETA_Impl(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("BETA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Beta: the slope of a least-squares linear regression of one series'
    * percentage returns (y, from inReal1) against another's (x, from inReal0)
    * over a rolling window. Measures how much a security moves relative to a
    * market index. Beta = 1 moves with the index; &lt; 1 less volatile, &gt; 1
    * more volatile.
    * <p><b>Formula</b>
    * <pre>{@code
    * Per-bar returns: $x_i=(p^0_i-p^0_{i-1})/p^0_{i-1}$ from inReal0, $y_i=(p^1_i-p^1_{i-1})/p^1_{i-1}$ from inReal1. With $n$=period over the window: $\beta = \dfrac{n\,S_{xy}-S_x S_y}{n\,S_{xx}-S_x^2}$, where $S_{xx}=\sum x^2,\ S_{xy}=\sum xy,\ S_x=\sum x,\ S_y=\sum y$.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#BETA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 Series whose returns are the regression x (market/index)
    * @param inReal1 Series whose returns are the regression y (security)
    * @param optInTimePeriod Rolling window length (number of returns) for the
    *        regression sums (default 5; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Beta: regression slope of inReal1-returns on
    *        inReal0-returns. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CORREL
    * @see Core#LINEARREG_SLOPE
    * @see Core#VAR
    * @see Core#STDDEV
    */
   public OutRange BETA( int startIdx,
                         int endIdx,
                         float inReal0[],
                         float inReal1[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("BETA", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, BETA_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BETA", "inReal0", inReal0, guardInLen);
      requireLength("BETA", "inReal1", inReal1, guardInLen);
      requireLength("BETA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = BETA_Impl(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("BETA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live BETA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#BETA} over the same series.
    * Open with {@link Core#BETA_Open}; there is no close — the handle is
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
   public static final class BETA_Stream {
      Core core;
      int optInTimePeriod;
      double S_xx;
      double S_xy;
      double S_x;
      double S_y;
      double last_price_x;
      double last_price_y;
      double trailing_last_price_x;
      double trailing_last_price_y;
      double shift_x;
      double shift_y;
      double leaving_xx;
      double leaving_yy;
      double S_yy;
      int barsSinceReseed;
      double n;
      int trailingIdx;
      int j;
      int i;
      int xMask;
      double[] x_inReal0;
      double[] x_inReal1;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      BETA_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#BETA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      BETA_Stream( BETA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.S_xx = other.S_xx;
         this.S_xy = other.S_xy;
         this.S_x = other.S_x;
         this.S_y = other.S_y;
         this.last_price_x = other.last_price_x;
         this.last_price_y = other.last_price_y;
         this.trailing_last_price_x = other.trailing_last_price_x;
         this.trailing_last_price_y = other.trailing_last_price_y;
         this.shift_x = other.shift_x;
         this.shift_y = other.shift_y;
         this.leaving_xx = other.leaving_xx;
         this.leaving_yy = other.leaving_yy;
         this.S_yy = other.S_yy;
         this.barsSinceReseed = other.barsSinceReseed;
         this.n = other.n;
         this.trailingIdx = other.trailingIdx;
         this.j = other.j;
         this.i = other.i;
         this.xMask = other.xMask;
         this.x_inReal0 = other.x_inReal0.clone();
         this.x_inReal1 = other.x_inReal1.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( BETA_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.S_xx = other.S_xx;
         this.S_xy = other.S_xy;
         this.S_x = other.S_x;
         this.S_y = other.S_y;
         this.last_price_x = other.last_price_x;
         this.last_price_y = other.last_price_y;
         this.trailing_last_price_x = other.trailing_last_price_x;
         this.trailing_last_price_y = other.trailing_last_price_y;
         this.shift_x = other.shift_x;
         this.shift_y = other.shift_y;
         this.leaving_xx = other.leaving_xx;
         this.leaving_yy = other.leaving_yy;
         this.S_yy = other.S_yy;
         this.barsSinceReseed = other.barsSinceReseed;
         this.n = other.n;
         this.trailingIdx = other.trailingIdx;
         this.j = other.j;
         this.i = other.i;
         this.xMask = other.xMask;
         if( this.x_inReal0 != null && this.x_inReal0.length == other.x_inReal0.length ) {
            System.arraycopy( other.x_inReal0, 0, this.x_inReal0, 0, other.x_inReal0.length );
         } else {
            this.x_inReal0 = other.x_inReal0.clone();
         }
         if( this.x_inReal1 != null && this.x_inReal1.length == other.x_inReal1.length ) {
            System.arraycopy( other.x_inReal1, 0, this.x_inReal1, 0, other.x_inReal1.length );
         } else {
            this.x_inReal1 = other.x_inReal1.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<BETA_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
      public double update( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("BETA update: BadParam", RetCode.BadParam);
         core.BETA_StepImpl(this, inReal0, inReal1);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal0.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal0[], double inReal1[], double outReal[] ) {
         final int barCount = inReal0.length;
         if( inReal1.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 )
            throw new TaLibArgumentException("BETA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal0[i]) || !Double.isFinite(inReal1[i]) )
               throw new TaLibArgumentException("BETA updateAndFill: BadParam", RetCode.BadParam);
            core.BETA_StepImpl(this, inReal0[i], inReal1[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public double peek( double inReal0, double inReal1 ) {
         if( !Double.isFinite(inReal0) || !Double.isFinite(inReal1) )
            throw new TaLibArgumentException("BETA peek: BadParam", RetCode.BadParam);
         BETA_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new BETA_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.BETA_StepImpl(scratch, inReal0, inReal1);
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
      public BETA_Stream copy() {
         return new BETA_Stream(this);
      }
   }
   void BETA_StepImpl( BETA_Stream sp, double inReal0, double inReal1 )
   {
      double tmp_real = 0.0;
      double denom = 0.0;
      double denom_scale = 0.0;
      double prev_x = 0.0;
      double prev_y = 0.0;
      int windowStart = 0;
      double x = 0.0;
      double y = 0.0;
      if( sp.i >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.i -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
      }
      sp.x_inReal0[sp.i & sp.xMask] = inReal0;
      sp.x_inReal1[sp.i & sp.xMask] = inReal1;
      tmp_real = sp.x_inReal0[sp.i & sp.xMask];
      if( !((-0.00000000000001 < sp.last_price_x) && (sp.last_price_x < 0.00000000000001)) ) {
         x = (tmp_real - sp.last_price_x) / sp.last_price_x - sp.shift_x;
      } else {
         x = 0 - sp.shift_x;
      }
      sp.last_price_x = tmp_real;
      tmp_real = sp.x_inReal1[sp.i++ & sp.xMask];
      if( !((-0.00000000000001 < sp.last_price_y) && (sp.last_price_y < 0.00000000000001)) ) {
         y = (tmp_real - sp.last_price_y) / sp.last_price_y - sp.shift_y;
      } else {
         y = 0 - sp.shift_y;
      }
      sp.last_price_y = tmp_real;
      sp.S_xx += x * x;
      sp.S_yy += y * y;
      sp.S_xy += x * y;
      sp.S_x += x;
      sp.S_y += y;
      denom_scale = sp.n * sp.S_xx;
      denom = denom_scale - sp.S_x * sp.S_x;
      /* Re-anchor and rebuild when the shift has gone stale. The same three
       * triggers as TA_VAR: the denominator has shrunk below 1e-6 of the scale
       * it is extracted from; OR the return that just left sat so far from the
       * shift that its squared term dwarfs what remains; OR at least every 32
       * windows.
       *
       * The outlier trigger earns its multiply and compare here, contrary to
       * what "returns are stationary" suggests: a bad tick makes one return
       * enormous, the ordinary ones fall below its ulp and are never really
       * added, and when it leaves the subtraction takes back a term they were
       * never part of. The residue is a consistent OFFSET, so the cancellation
       * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
       * the periodic re-anchor recovers, up to 32*period bars later. Measured
       * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
       * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
       * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
       * is a corpus figure, not a bound. Isolated against the same body without
       * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
       * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
       * The cost is the reseed it triggers, so it tracks the fire rate; on data
       * that never triggers it, the compare is free.
       *
       * BOTH axes are watched, and the y one is not redundant. The denominator
       * is x-only, so it is tempting to conclude -- as an earlier draft of this
       * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
       * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
       * the x side able to see it. Measured on test_beta_outlier_transit's own
       * ladder with the spike moved from px to py: 12 of 24 rungs fail without
       * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
       * experiment that found it inert was run on an x-only corpus, where it is
       * inert by construction. TA_CORREL, fixed by the same #242 work, watches
       * both from the start; this brings BETA level. S_yy exists only to scale
       * this test -- nothing else reads it.
       *
       * The threshold is 1e3 where TA_VAR uses 1e6, because a return amplifies:
       * a tick multiplying the price by k puts k-1 into the return and (k-1)^2
       * into S_xx, so the ratio when that term leaves lands an order or two
       * below the value-scale case var.c was tuned on. At 1e6 a 1e5 tick slips
       * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
       * Pinned by test_beta_outlier_transit.
       *
       * Reading the window here is safe when outReal aliases an input: the
       * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
       * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
       */
      sp.barsSinceReseed -= 1;
      if( denom < 0.000001 * denom_scale || sp.leaving_xx > 1000.0 * sp.S_xx || sp.leaving_yy > 1000.0 * sp.S_yy || sp.barsSinceReseed <= 0 ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         windowStart = sp.trailingIdx;
         /* Walk the window forward from the price the trailing cursor already
          * carries. A return needs its predecessor, and reading inReal[j-1]
          * would reach one slot BEFORE the window -- which the batch can do and
          * a streaming ring sized for the window cannot. trailing_last_price_*
          * IS that predecessor, so carrying it forward keeps every read inside
          * [trailingIdx, i-1] and the two paths stay identical.
          */
         prev_x = sp.trailing_last_price_x;
         prev_y = sp.trailing_last_price_y;
         tmp_real = 0.0;
         sp.shift_y = 0.0;
         for( sp.j = windowStart; sp.j < sp.i; sp.j += 1 ) {
            if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
               tmp_real += (sp.x_inReal0[sp.j & sp.xMask] - prev_x) / prev_x;
            }
            prev_x = sp.x_inReal0[sp.j & sp.xMask];
            if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
               sp.shift_y += (sp.x_inReal1[sp.j & sp.xMask] - prev_y) / prev_y;
            }
            prev_y = sp.x_inReal1[sp.j & sp.xMask];
         }
         sp.shift_x = tmp_real / sp.n;
         sp.shift_y = sp.shift_y / sp.n;
         prev_x = sp.trailing_last_price_x;
         prev_y = sp.trailing_last_price_y;
         sp.S_xx = 0.0;
         sp.S_yy = 0.0;
         sp.S_xy = 0.0;
         sp.S_x = 0.0;
         sp.S_y = 0.0;
         for( sp.j = windowStart; sp.j < sp.i; sp.j += 1 ) {
            if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
               x = (sp.x_inReal0[sp.j & sp.xMask] - prev_x) / prev_x - sp.shift_x;
            } else {
               x = 0 - sp.shift_x;
            }
            prev_x = sp.x_inReal0[sp.j & sp.xMask];
            if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
               y = (sp.x_inReal1[sp.j & sp.xMask] - prev_y) / prev_y - sp.shift_y;
            } else {
               y = 0 - sp.shift_y;
            }
            prev_y = sp.x_inReal1[sp.j & sp.xMask];
            sp.S_xx += x * x;
            sp.S_yy += y * y;
            sp.S_xy += x * y;
            sp.S_x += x;
            sp.S_y += y;
         }
         denom_scale = sp.n * sp.S_xx;
         denom = denom_scale - sp.S_x * sp.S_x;
         /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * whose returns are all the same value. Enforce the invariant HERE and
          * not at the divide: a negative denom always reseeds on the same bar
          * (it makes the first trigger true whenever denom_scale is positive,
          * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
          * divide below can rely on it being >= 0.
          */
         if( denom < 0.0 ) {
            denom = 0.0;
         }
      }
      /* Always read the trailing before writing the output because the input and output
       * buffer can be the same.
       */
      tmp_real = sp.x_inReal0[sp.trailingIdx & sp.xMask];
      if( !((-0.00000000000001 < sp.trailing_last_price_x) && (sp.trailing_last_price_x < 0.00000000000001)) ) {
         x = (tmp_real - sp.trailing_last_price_x) / sp.trailing_last_price_x - sp.shift_x;
      } else {
         x = 0 - sp.shift_x;
      }
      sp.trailing_last_price_x = tmp_real;
      tmp_real = sp.x_inReal1[sp.trailingIdx & sp.xMask];
      sp.trailingIdx += 1;
      if( !((-0.00000000000001 < sp.trailing_last_price_y) && (sp.trailing_last_price_y < 0.00000000000001)) ) {
         y = (tmp_real - sp.trailing_last_price_y) / sp.trailing_last_price_y - sp.shift_y;
      } else {
         y = 0 - sp.shift_y;
      }
      sp.trailing_last_price_y = tmp_real;
      /* Write the output.
       *
       * The denominator is tested against ITS OWN scale, not a fixed band: it
       * is quadratic in the return volatility, so an absolute 1e-14 threshold
       * stops meaning "the regressor does not vary" and starts meaning "the
       * returns are small". The literal is TA_EPSILON, and the plain `>` also
       * rejects a negative denominator rather than dividing by it.
       */
      if( denom > 0.00000000000001 * denom_scale ) {
         sp.cur_outReal = (sp.n * sp.S_xy - sp.S_x * sp.S_y) / denom;
      } else {
         sp.cur_outReal = 0.0;
      }
      /* Remove the calculation starting with the trailingIdx. */
      sp.leaving_xx = x * x;
      sp.leaving_yy = y * y;
      sp.S_xx -= x * x;
      sp.S_yy -= y * y;
      sp.S_xy -= x * y;
      sp.S_x -= x;
      sp.S_y -= y;
   }
   private RetCode BETA_OpenImpl( BETA_Stream sp, double inReal0[], double inReal1[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double shift_x = 0;
      double shift_y = 0;
      double denom = 0;
      double denom_scale = 0;
      double prev_x = 0;
      double leaving_xx = 0;
      double leaving_yy = 0;
      double S_yy = 0;
      double prev_y = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      shift_x = 0.0;
      shift_y = 0.0;
      denom = 0.0;
      denom_scale = 0.0;
      prev_x = 0.0;
      leaving_xx = 0.0;
      leaving_yy = 0.0;
      S_yy = 0.0;
      prev_y = 0.0;
      n = 0.0;
      /* sum of x * x */
      /* sum of x * y */
      /* sum of x */
      /* sum of y */
      /* the last price read from inReal0 */
      /* the last price read from inReal1 */
      /* same as last_price_x except used to remove elements from the trailing summation */
      /* same as last_price_y except used to remove elements from the trailing summation */
      /* temporary variable */
      /* origin the x returns are measured against */
      /* origin the y returns are measured against */
      /* n*S_xx - S_x*S_x, the regression denominator */
      /* n*S_xx, the scale denom is extracted from */
      /* price walked forward when rebuilding the window */
      /* squared x deviation the previous bar removed */
      /* squared y deviation the previous bar removed */
      /* sum of y * y, carried ONLY for the outlier trigger */
      /* the 'x' value, which is the last change between values in inReal0 */
      /* the 'y' value, which is the last change between values in inReal1 */
      /* DESCRIPTION OF ALGORITHM:
       *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The index prices
       *   are given in inReal0 and the stock prices are given in inReal1. The size of these vectors
       *   should be equal. The algorithm is to calculate the change between prices in both vectors
       *   and then 'plot' these changes are points in the Euclidean plane. The x value of the point
       *   is market return and the y value is the security return. The beta value is the slope of a
       *   linear regression through these points. A beta of 1 is simple the line y=x, so the stock
       *   varies percisely with the market. A beta of less than one means the stock varies less than
       *   the market and a beta of more than one means the stock varies more than market. A related
       *   value is the Alpha value (see TA_ALPHA) which is the Y-intercept of the same linear regression.
       */
      /* Validate the calculation method type and
       * identify the minimum number of input
       * consume before the first value is output..
       */
      nbInitialElementNeeded = optInTimePeriod;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Consume first input. */
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      /* Measure the returns against a shift near the window, as TA_VAR does for
       * its values (#118) and TA_CORREL for its prices (#242).
       *
       * Returns are near-zero-mean, so on a series that jitters this changes
       * little. It is decisive on a series that DRIFTS: a steadily rising price
       * gives near-identical returns, S_x*S_x then equals n*S_xx to every digit,
       * and the denominator n*S_xx - S_x*S_x is left as pure rounding noise --
       * measured at +/-2e-16 of its own scale, sign included. Wilkinson's BIG and
       * LITTLE are exactly that shape, and beta of such a series against itself
       * came back 0 instead of 1.
       *
       * Anchor on the window's first return here; every later re-anchor uses the
       * window mean, which is better centred but costs a pass this one cannot
       * afford before the sums exist.
       */
      i = ++trailingIdx;
      if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
         shift_x = (inReal0[i] - last_price_x) / last_price_x;
      }
      if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
         shift_y = (inReal1[i] - last_price_y) / last_price_y;
      }
      while( i < startIdx ) {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      /* First output always start at index zero */
      n = (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      do {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_yy += y * y;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         denom_scale = n * S_xx;
         denom = denom_scale - S_x * S_x;
         /* Re-anchor and rebuild when the shift has gone stale. The same three
          * triggers as TA_VAR: the denominator has shrunk below 1e-6 of the scale
          * it is extracted from; OR the return that just left sat so far from the
          * shift that its squared term dwarfs what remains; OR at least every 32
          * windows.
          *
          * The outlier trigger earns its multiply and compare here, contrary to
          * what "returns are stationary" suggests: a bad tick makes one return
          * enormous, the ordinary ones fall below its ulp and are never really
          * added, and when it leaves the subtraction takes back a term they were
          * never part of. The residue is a consistent OFFSET, so the cancellation
          * trigger above cannot see it -- denom/denom_scale stays ~1 -- and only
          * the periodic re-anchor recovers, up to 32*period bars later. Measured
          * without it: a 1e8 tick left 286 of 386 bars wrong, the worst by 0.36
          * ABSOLUTE. Cost is ~3% and mostly unmeasurable on the bench corpus
          * (randwalk/GBM/trend-chop), where it fires on 0.00% of bars -- but that
          * is a corpus figure, not a bound. Isolated against the same body without
          * the disjunct it is +16-20% on a stale-quote/illiquid series (1.5% fire
          * rate) and +54-64% on constructed near-flat or gapped shapes (5.1%).
          * The cost is the reseed it triggers, so it tracks the fire rate; on data
          * that never triggers it, the compare is free.
          *
          * BOTH axes are watched, and the y one is not redundant. The denominator
          * is x-only, so it is tempting to conclude -- as an earlier draft of this
          * did -- that a y trigger catches nothing. It catches plenty: the OUTPUT
          * also reads S_xy and S_y, which a y-side outlier corrupts with nothing on
          * the x side able to see it. Measured on test_beta_outlier_transit's own
          * ladder with the spike moved from px to py: 12 of 24 rungs fail without
          * the second disjunct, worst 156x relative; 0 of 24 with it. The earlier
          * experiment that found it inert was run on an x-only corpus, where it is
          * inert by construction. TA_CORREL, fixed by the same #242 work, watches
          * both from the start; this brings BETA level. S_yy exists only to scale
          * this test -- nothing else reads it.
          *
          * The threshold is 1e3 where TA_VAR uses 1e6, because a return amplifies:
          * a tick multiplying the price by k puts k-1 into the return and (k-1)^2
          * into S_xx, so the ratio when that term leaves lands an order or two
          * below the value-scale case var.c was tuned on. At 1e6 a 1e5 tick slips
          * through and leaves a flat 2.5e-5 relative error on 285 of 386 bars.
          * Pinned by test_beta_outlier_transit.
          *
          * Reading the window here is safe when outReal aliases an input: the
          * outputs written so far occupy [0, outIdx-1] while windowStart-1 is
          * startIdx-optInTimePeriod+outIdx, which is >= outIdx.
          */
         barsSinceReseed -= 1;
         if( denom < 0.000001 * denom_scale || leaving_xx > 1000.0 * S_xx || leaving_yy > 1000.0 * S_yy || barsSinceReseed <= 0 ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = trailingIdx;
            /* Walk the window forward from the price the trailing cursor already
             * carries. A return needs its predecessor, and reading inReal[j-1]
             * would reach one slot BEFORE the window -- which the batch can do and
             * a streaming ring sized for the window cannot. trailing_last_price_*
             * IS that predecessor, so carrying it forward keeps every read inside
             * [trailingIdx, i-1] and the two paths stay identical.
             */
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            tmp_real = 0.0;
            shift_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  tmp_real += (inReal0[j] - prev_x) / prev_x;
               }
               prev_x = inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  shift_y += (inReal1[j] - prev_y) / prev_y;
               }
               prev_y = inReal1[j];
            }
            shift_x = tmp_real / n;
            shift_y = shift_y / n;
            prev_x = trailing_last_price_x;
            prev_y = trailing_last_price_y;
            S_xx = 0.0;
            S_yy = 0.0;
            S_xy = 0.0;
            S_x = 0.0;
            S_y = 0.0;
            for( j = windowStart; j < i; j += 1 ) {
               if( !((-0.00000000000001 < prev_x) && (prev_x < 0.00000000000001)) ) {
                  x = (inReal0[j] - prev_x) / prev_x - shift_x;
               } else {
                  x = 0 - shift_x;
               }
               prev_x = inReal0[j];
               if( !((-0.00000000000001 < prev_y) && (prev_y < 0.00000000000001)) ) {
                  y = (inReal1[j] - prev_y) / prev_y - shift_y;
               } else {
                  y = 0 - shift_y;
               }
               prev_y = inReal1[j];
               S_xx += x * x;
               S_yy += y * y;
               S_xy += x * y;
               S_x += x;
               S_y += y;
            }
            denom_scale = n * S_xx;
            denom = denom_scale - S_x * S_x;
            /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
             * extracted as a difference, so its SIGN is not guaranteed on a window
             * whose returns are all the same value. Enforce the invariant HERE and
             * not at the divide: a negative denom always reseeds on the same bar
             * (it makes the first trigger true whenever denom_scale is positive,
             * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
             * divide below can rely on it being >= 0.
             */
            if( denom < 0.0 ) {
               denom = 0.0;
            }
         }
         /* Always read the trailing before writing the output because the input and output
          * buffer can be the same.
          */
         tmp_real = inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x - shift_x;
         } else {
            x = 0 - shift_x;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = inReal1[trailingIdx];
         trailingIdx += 1;
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y - shift_y;
         } else {
            y = 0 - shift_y;
         }
         trailing_last_price_y = tmp_real;
         /* Write the output.
          *
          * The denominator is tested against ITS OWN scale, not a fixed band: it
          * is quadratic in the return volatility, so an absolute 1e-14 threshold
          * stops meaning "the regressor does not vary" and starts meaning "the
          * returns are small". The literal is TA_EPSILON, and the plain `>` also
          * rejects a negative denominator rather than dividing by it.
          */
         if( denom > 0.00000000000001 * denom_scale ) {
            outReal[outIdx++ * outStride] = (n * S_xy - S_x * S_y) / denom;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         /* Remove the calculation starting with the trailingIdx. */
         leaving_xx = x * x;
         leaving_yy = y * y;
         S_xx -= x * x;
         S_yy -= y * y;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = i - trailingIdx + 1;
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
      sp.S_xx = S_xx;
      sp.S_xy = S_xy;
      sp.S_x = S_x;
      sp.S_y = S_y;
      sp.last_price_x = last_price_x;
      sp.last_price_y = last_price_y;
      sp.trailing_last_price_x = trailing_last_price_x;
      sp.trailing_last_price_y = trailing_last_price_y;
      sp.shift_x = shift_x;
      sp.shift_y = shift_y;
      sp.leaving_xx = leaving_xx;
      sp.leaving_yy = leaving_yy;
      sp.S_yy = S_yy;
      sp.barsSinceReseed = barsSinceReseed;
      sp.n = n;
      sp.trailingIdx = trailingIdx;
      sp.j = j;
      sp.i = i;
      sp.xMask = physX - 1;
      sp.x_inReal0 = capX_inReal0;
      sp.x_inReal1 = capX_inReal1;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* BETA_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   BETA_Stream BETA_OpenAndFillInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      BETA_Stream sp = new BETA_Stream(this);
      RetCode retCode = BETA_OpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("BETA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("BETA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("BETA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind BETA_Open (composition seam). */
   BETA_Stream BETA_OpenInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      BETA_Stream sp = new BETA_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = BETA_OpenImpl(sp, inReal0, inReal1, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("BETA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("BETA open: internal error", retCode);
      }
      throw new TaLibArgumentException("BETA open: " + retCode, retCode);
   }
   /**
    * Open a live BETA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#BETA} at that bar.
    * <p>The history must hold at least {@code BETA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public BETA_Stream BETA_Open( double inReal0[], double inReal1[], int optInTimePeriod )
   {
      return BETA_OpenInternal(inReal0, inReal1, 0, optInTimePeriod);
   }
   /**
    * {@link Core#BETA_Open} that also fills the output array(s) bit-identically
    * to {@link Core#BETA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link BETA_Stream#outRange()}.
    */
   public BETA_Stream BETA_OpenAndFill( double inReal0[], double inReal1[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         throw new TaLibArgumentException("BETA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return BETA_OpenAndFillInternal(inReal0, inReal1, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
