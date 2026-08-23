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
 *  082326 MF,CC #242 follow-up: restore TA_VAR's outlier trigger, at 1e3.
 */

int beta_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode beta(int startIdx, int endIdx,
   const double inReal0[],
   const double inReal1[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double S_xx = 0.0f; /* sum of x * x */
   double S_xy = 0.0f; /* sum of x * y */
   double S_x = 0.0f; /* sum of x */
   double S_y = 0.0f; /* sum of y */
   double last_price_x = 0.0f; /* the last price read from inReal0 */
   double last_price_y = 0.0f; /* the last price read from inReal1 */
   double trailing_last_price_x = 0.0f; /* same as last_price_x except used to remove elements from the trailing summation */
   double trailing_last_price_y = 0.0f; /* same as last_price_y except used to remove elements from the trailing summation */
   double tmp_real = 0.0f; /* temporary variable */
   double shift_x = 0.0f; /* origin the x returns are measured against */
   double shift_y = 0.0f; /* origin the y returns are measured against */
   double denom = 0.0f; /* n*S_xx - S_x*S_x, the regression denominator */
   double denom_scale = 0.0f; /* n*S_xx, the scale denom is extracted from */
   double prev_x = 0.0f; /* price walked forward when rebuilding the window */
   double leaving_xx = 0.0f; /* squared x deviation the previous bar removed */
   double prev_y = 0.0f;
   int j, windowStart, barsSinceReseed;
   double x; /* the 'x' value, which is the last change between values in inReal0 */
   double y; /* the 'y' value, which is the last change between values in inReal1 */
   double n = 0.0f;
   int i, outIdx;
   int trailingIdx, nbInitialElementNeeded;

   /** DESCRIPTION OF ALGORITHM:
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
   if( startIdx < nbInitialElementNeeded )
      startIdx = nbInitialElementNeeded;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   /* Consume first input. */
   trailingIdx = startIdx-nbInitialElementNeeded;
   last_price_x = trailing_last_price_x = inReal0[trailingIdx];
   last_price_y = trailing_last_price_y = inReal1[trailingIdx];

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

   if( !TA_IS_ZERO(last_price_x) )
      shift_x = (inReal0[i]-last_price_x)/last_price_x;
   if( !TA_IS_ZERO(last_price_y) )
      shift_y = (inReal1[i]-last_price_y)/last_price_y;

   while( i < startIdx )
   {
      tmp_real = inReal0[i];
      if( !TA_IS_ZERO(last_price_x) )
         x = (tmp_real-last_price_x)/last_price_x - shift_x;
      else
         x = -shift_x;
      last_price_x = tmp_real;

      tmp_real = inReal1[i++];
      if( !TA_IS_ZERO(last_price_y) )
         y = (tmp_real-last_price_y)/last_price_y - shift_y;
      else
         y = -shift_y;
      last_price_y = tmp_real;

      S_xx += x*x;
      S_xy += x*y;
      S_x += x;
      S_y += y;
   }

   outIdx = 0; /* First output always start at index zero */
   n = (double)optInTimePeriod;
   barsSinceReseed = 32 * optInTimePeriod;
   do
   {
      tmp_real = inReal0[i];
      if( !TA_IS_ZERO(last_price_x) )
         x = (tmp_real-last_price_x)/last_price_x - shift_x;
      else
         x = -shift_x;
      last_price_x = tmp_real;

      tmp_real = inReal1[i++];
      if( !TA_IS_ZERO(last_price_y) )
         y = (tmp_real-last_price_y)/last_price_y - shift_y;
      else
         y = -shift_y;
      last_price_y = tmp_real;

      S_xx += x*x;
      S_xy += x*y;
      S_x += x;
      S_y += y;

      denom_scale = n * S_xx;
      denom = denom_scale - (S_x * S_x);

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
       * ABSOLUTE. It costs at most ~3% (mostly unmeasurable against a +/-1.6%
       * noise floor), against the 7-11% the shift itself spends.
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
      barsSinceReseed--;
      if( denom < 0.000001 * denom_scale
         || leaving_xx > 1000.0 * S_xx
         || barsSinceReseed <= 0 )
      {
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
         for( j=windowStart; j < i; j++ )
         {
            if( !TA_IS_ZERO(prev_x) )
               tmp_real += (inReal0[j]-prev_x)/prev_x;
            prev_x = inReal0[j];
            if( !TA_IS_ZERO(prev_y) )
               shift_y += (inReal1[j]-prev_y)/prev_y;
            prev_y = inReal1[j];
         }
         shift_x = tmp_real/n;
         shift_y = shift_y/n;

         prev_x = trailing_last_price_x;
         prev_y = trailing_last_price_y;
         S_xx = 0.0;
         S_xy = 0.0;
         S_x = 0.0;
         S_y = 0.0;
         for( j=windowStart; j < i; j++ )
         {
            if( !TA_IS_ZERO(prev_x) )
               x = (inReal0[j]-prev_x)/prev_x - shift_x;
            else
               x = -shift_x;
            prev_x = inReal0[j];
            if( !TA_IS_ZERO(prev_y) )
               y = (inReal1[j]-prev_y)/prev_y - shift_y;
            else
               y = -shift_y;
            prev_y = inReal1[j];
            S_xx += x*x;
            S_xy += x*y;
            S_x += x;
            S_y += y;
         }

         denom_scale = n * S_xx;
         denom = denom_scale - (S_x * S_x);

         /* n*S_xx - S_x*S_x is non-negative by Cauchy-Schwarz, but it is
          * extracted as a difference, so its SIGN is not guaranteed on a window
          * whose returns are all the same value. Enforce the invariant HERE and
          * not at the divide: a negative denom always reseeds on the same bar
          * (it makes the first trigger true whenever denom_scale is positive,
          * and denom_scale == 0 reduces that trigger to `denom < 0`), so the
          * divide below can rely on it being >= 0.
          */
         if( denom < 0.0 )
            denom = 0.0;
      }

      /* Always read the trailing before writing the output because the input and output
       * buffer can be the same.
       */
      tmp_real = inReal0[trailingIdx];
      if( !TA_IS_ZERO(trailing_last_price_x) )
         x = (tmp_real-trailing_last_price_x)/trailing_last_price_x - shift_x;
      else
         x = -shift_x;
      trailing_last_price_x = tmp_real;

      tmp_real = inReal1[trailingIdx];
      trailingIdx++;
      if( !TA_IS_ZERO(trailing_last_price_y) )
         y = (tmp_real-trailing_last_price_y)/trailing_last_price_y - shift_y;
      else
         y = -shift_y;
      trailing_last_price_y = tmp_real;

      /* Write the output.
       *
       * The denominator is tested against ITS OWN scale, not a fixed band: it
       * is quadratic in the return volatility, so an absolute 1e-14 threshold
       * stops meaning "the regressor does not vary" and starts meaning "the
       * returns are small". The literal is TA_EPSILON, and the plain `>` also
       * rejects a negative denominator rather than dividing by it.
       */
      if( denom > 0.00000000000001 * denom_scale )
         outReal[outIdx++] = ((n * S_xy) - (S_x * S_y)) / denom;
      else
         outReal[outIdx++] = 0.0;

      /* Remove the calculation starting with the trailingIdx. */
      leaving_xx = x*x;
      S_xx -= x*x;
      S_xy -= x*y;
      S_x -= x;
      S_y -= y;
   } while( i <= endIdx );

   /* All done. Indicate the output limits and return. */
   *outNBElement = outIdx;
   *outBegIdx = startIdx;

   return TA_SUCCESS;
}
