/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JD       jdoyle
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  120802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  111705 MF,JD  Fix#1359452 for handling properly start/end range.
 */

int ad_lookback(void)
{
   /* This function have no lookback needed. */
   return 0;
}

TA_RetCode ad(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int nbBar, currentBar, outIdx;

   double high, low, close, tmp;
   double ad;

   /* Note: Results from this function might vary slightly
    *       from Metastock outputs. The reason being that
    *       Metastock use float instead of double and this
    *       cause a different floating-point precision to
    *       be used.
    *
    *       For most function, this is not an apparent difference
    *       but for function using large cummulative values (like
    *       this AD function), minor imprecision adds up and becomes
    *       significative.
    *
    *       For better precision, TA-Lib use double in all its
    *       its calculations.
    */

   /* Default return values */
   nbBar = endIdx-startIdx+1;
   *outNBElement = nbBar;
   *outBegIdx = startIdx;
   currentBar = startIdx;
   outIdx = 0;
   ad = 0.0;

   while( nbBar != 0 )
   {
      high  = inHigh[currentBar];
      low   = inLow[currentBar];
      tmp   = high-low;
      close = inClose[currentBar];

      if( tmp > 0.0 )
         ad += (((close-low)-(high-close))/tmp)*((double)inVolume[currentBar]);

      outReal[outIdx++] = ad;

      currentBar++;
      nbBar--;
   }

   return TA_SUCCESS;
}
