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

   public int adLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode ad( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      double inClose[],
                      double inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
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
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode adUnguarded( int startIdx,
                               int endIdx,
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               double inVolume[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = inHigh[currentBar];
         low = inLow[currentBar];
         tmp = high - low;
         close = inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode ad( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      float inClose[],
                      float inVolume[],
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = (double)inHigh[currentBar];
         low = (double)inLow[currentBar];
         tmp = high - low;
         close = (double)inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
   public RetCode adUnguarded( int startIdx,
                               int endIdx,
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               float inVolume[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               double outReal[] )
   {
      int nbBar = 0;
      int currentBar = 0;
      int outIdx = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double ad = 0;
      nbBar = endIdx - startIdx + 1;
      outNBElement.value = nbBar;
      outBegIdx.value = startIdx;
      currentBar = startIdx;
      outIdx = 0;
      ad = 0.0;
      while( nbBar != 0 ) {
         high = (double)inHigh[currentBar];
         low = (double)inLow[currentBar];
         tmp = high - low;
         close = (double)inClose[currentBar];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[currentBar];
         }
         outReal[outIdx++] = ad;
         currentBar += 1;
         nbBar -= 1;
      }
      return RetCode.Success ;
   }
