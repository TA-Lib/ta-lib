/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MW       Michael Williamson
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  122006 MW   Initial Version
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
   double x; /* the 'x' value, which is the last change between values in inReal0 */
   double y; /* the 'y' value, which is the last change between values in inReal1 */
   double n = 0.0f;
   int i, outIdx;
   int trailingIdx, nbInitialElementNeeded;

   /** DESCRIPTION OF ALGORITHM:
    *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The stock prices
    *   are given in inReal0 and the index prices are give in inReal1. The size of these vectors
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

   /* Process remaining of lookback until ready to output the first value. */
   i = ++trailingIdx;

   while( i < startIdx )
   {
      tmp_real = inReal0[i];
      if( !TA_IS_ZERO(last_price_x) )
         x = (tmp_real-last_price_x)/last_price_x;
      else
         x = 0.0;
      last_price_x = tmp_real;

      tmp_real = inReal1[i++];
      if( !TA_IS_ZERO(last_price_y) )
         y = (tmp_real-last_price_y)/last_price_y;
      else
         y = 0.0;
      last_price_y = tmp_real;

      S_xx += x*x;
      S_xy += x*y;
      S_x += x;
      S_y += y;
   }

   outIdx = 0; /* First output always start at index zero */
   n = (double)optInTimePeriod;
   do
   {
      tmp_real = inReal0[i];
      if( !TA_IS_ZERO(last_price_x) )
         x = (tmp_real-last_price_x)/last_price_x;
      else
         x = 0.0;
      last_price_x = tmp_real;

      tmp_real = inReal1[i++];
      if( !TA_IS_ZERO(last_price_y) )
         y = (tmp_real-last_price_y)/last_price_y;
      else
         y = 0.0;
      last_price_y = tmp_real;

      S_xx += x*x;
      S_xy += x*y;
      S_x += x;
      S_y += y;

      /* Always read the trailing before writing the output because the input and output
       * buffer can be the same.
       */
      tmp_real = inReal0[trailingIdx];
      if( !TA_IS_ZERO(trailing_last_price_x) )
         x = (tmp_real-trailing_last_price_x)/trailing_last_price_x;
      else
         x = 0.0;
      trailing_last_price_x = tmp_real;

      tmp_real = inReal1[trailingIdx++];
      if( !TA_IS_ZERO(trailing_last_price_y) )
         y = (tmp_real-trailing_last_price_y)/trailing_last_price_y;
      else
         y = 0.0;
      trailing_last_price_y = tmp_real;

      /* Write the output */
      tmp_real = (n * S_xx) - (S_x * S_x);
      if( !TA_IS_ZERO(tmp_real) )
         outReal[outIdx++] = ((n * S_xy) - (S_x * S_y)) / tmp_real;
      else
         outReal[outIdx++] = 0.0;

      /* Remove the calculation starting with the trailingIdx. */
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
