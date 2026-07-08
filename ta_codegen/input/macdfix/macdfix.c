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
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *
 */

int macdfix_lookback(int optInSignalPeriod)
{
   /* The lookback is driven by the signal line output.
    *
    * (must also account for the initial data consume
    *  by the fix 26 period EMA).
    */
   return ema_lookback( 26 )
   + ema_lookback( optInSignalPeriod );
}

TA_RetCode macdfix(int startIdx, int endIdx, const double inReal[], int optInSignalPeriod, int *outBegIdx, int *outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[])
{
   return macd( startIdx, endIdx, inReal,
      0, /* 0 indicate fix 12 == 0.15  for optInFastPeriod */
      0, /* 0 indicate fix 26 == 0.075 for optInSlowPeriod */
      optInSignalPeriod,
      outBegIdx,
      outNBElement,
      outMACD,
      outMACDSignal,
      outMACDHist );
}
