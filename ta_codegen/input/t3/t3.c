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

int t3_lookback(int optInTimePeriod, double optInVFactor)
{
   (void)optInVFactor;
   return 6 * (optInTimePeriod-1) + TA_GetUnstablePeriod(TA_FUNC_UNST_T3);
}

TA_RetCode t3(int startIdx, int endIdx, const double inReal[], int optInTimePeriod, double optInVFactor, int *outBegIdx, int *outNBElement, double outReal[])
{
   int outIdx, lookbackTotal;
   int today, i;
   double k, one_minus_k;
   double e1, e2, e3, e4, e5, e6;
   double c1, c2, c3, c4;
   double tempReal;

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
    *
    */
   lookbackTotal = 6 * (optInTimePeriod - 1) + TA_GetUnstablePeriod(TA_FUNC_UNST_T3);
   if( startIdx <= lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      *outNBElement = 0;
      *outBegIdx = 0;
      return TA_SUCCESS;
   }

   /* No smoothing at period of 1: the output is a copy of the input
    * (same convention as TA_MA for every MAType). Explicit because the
    * coefficients below sum to 1 only in real arithmetic; going through
    * the math would leave ~1e-14 floating-point drift on every value.
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx = startIdx;
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx )
         outReal[outIdx++] = inReal[today++];
      *outNBElement = outIdx;
      return TA_SUCCESS;
   }

   *outBegIdx = startIdx;
   today = startIdx - lookbackTotal;

   k = 2.0/(optInTimePeriod+1.0);
   one_minus_k = 1.0-k;

   /* Initialize e1 */
   tempReal = inReal[today++];
   for( i=optInTimePeriod-1; i > 0 ; i-- )
      tempReal += inReal[today++];
   e1 = tempReal / optInTimePeriod;

   /* Initialize e2 */
   tempReal = e1;
   for( i=optInTimePeriod-1; i > 0 ; i-- )
   {
      e1 = (k*inReal[today++])+(one_minus_k*e1);
      tempReal += e1;
   }
   e2 = tempReal / optInTimePeriod;

   /* Initialize e3 */
   tempReal = e2;
   for( i=optInTimePeriod-1; i > 0 ; i-- )
   {
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      tempReal += e2;
   }
   e3 = tempReal / optInTimePeriod;

   /* Initialize e4 */
   tempReal = e3;
   for( i=optInTimePeriod-1; i > 0 ; i-- )
   {
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      e3  = (k*e2)+(one_minus_k*e3);
      tempReal += e3;
   }
   e4 = tempReal / optInTimePeriod;

   /* Initialize e5 */
   tempReal = e4;
   for( i=optInTimePeriod-1; i > 0 ; i-- )
   {
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      e3  = (k*e2)+(one_minus_k*e3);
      e4  = (k*e3)+(one_minus_k*e4);
      tempReal += e4;
   }
   e5 = tempReal / optInTimePeriod;

   /* Initialize e6 */
   tempReal = e5;
   for( i=optInTimePeriod-1; i > 0 ; i-- )
   {
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      e3  = (k*e2)+(one_minus_k*e3);
      e4  = (k*e3)+(one_minus_k*e4);
      e5  = (k*e4)+(one_minus_k*e5);
      tempReal += e5;
   }
   e6 = tempReal / optInTimePeriod;

   /* Skip the unstable period */
   while( today <= startIdx )
   {
      /* Do the calculation but do not write the output */
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      e3  = (k*e2)+(one_minus_k*e3);
      e4  = (k*e3)+(one_minus_k*e4);
      e5  = (k*e4)+(one_minus_k*e5);
      e6  = (k*e5)+(one_minus_k*e6);
   }

   /* Calculate the constants */
   tempReal = optInVFactor * optInVFactor;
   c1 = -(tempReal * optInVFactor);
   c2 = 3.0 * (tempReal - c1);
   c3 = -6.0 * tempReal - 3.0 * (optInVFactor-c1);
   c4 = 1.0 + 3.0 * optInVFactor - c1 + 3.0 * tempReal;

   /* Write the first output */
   outIdx = 0;
   outReal[outIdx++] = c1*e6+c2*e5+c3*e4+c4*e3;

   /* Calculate and output the remaining of the range. */
   while( today <= endIdx )
   {
      e1  = (k*inReal[today++])+(one_minus_k*e1);
      e2  = (k*e1)+(one_minus_k*e2);
      e3  = (k*e2)+(one_minus_k*e3);
      e4  = (k*e3)+(one_minus_k*e4);
      e5  = (k*e4)+(one_minus_k*e5);
      e6  = (k*e5)+(one_minus_k*e6);
      outReal[outIdx++] = c1*e6+c2*e5+c3*e4+c4*e3;
   }

   /* Indicates to the caller the number of output
    * successfully calculated.
    */
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
