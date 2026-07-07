/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  121104 AC   Creation
 */

   public int cdl3OutsideLookback( )
   {
      return 3 ;

   }
   public RetCode cdl3Outside( int startIdx,
                               int endIdx,
                               double inOpen[],
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdl3OutsideLookback();
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Do the calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first: black (white) real body
       * - second: white (black) real body that engulfs the prior real body
       * - third: candle that closes higher (lower) than the second candle
       * outInteger is positive (1 to 100) for the three outside up or negative (-1 to -100) for the three outside down;
       * the user should consider that a three outside up must appear in a downtrend and three outside down must appear
       * in an uptrend, while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] > inOpen[i - 2] && inOpen[i - 1] < inClose[i - 2] && inClose[i] > inClose[i - 1] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && inOpen[i - 1] > inClose[i - 2] && inClose[i - 1] < inOpen[i - 2] && inClose[i] < inClose[i - 1] ) {
            /* white engulfs black */
            /* third candle higher */
            /* black engulfs white */
            /* third candle lower */
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3OutsideUnguarded( int startIdx,
                                        int endIdx,
                                        double inOpen[],
                                        double inHigh[],
                                        double inLow[],
                                        double inClose[],
                                        MInteger outBegIdx,
                                        MInteger outNBElement,
                                        int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = cdl3OutsideLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && inClose[i - 1] > inOpen[i - 2] && inOpen[i - 1] < inClose[i - 2] && inClose[i] > inClose[i - 1] || ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && inOpen[i - 1] > inClose[i - 2] && inClose[i - 1] < inOpen[i - 2] && inClose[i] < inClose[i - 1] ) {
            outInteger[outIdx++] = ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3Outside( int startIdx,
                               int endIdx,
                               float inOpen[],
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               MInteger outBegIdx,
                               MInteger outNBElement,
                               int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdl3OutsideLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] < (double)inClose[i - 2] && (double)inClose[i] > (double)inClose[i - 1] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (double)inOpen[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 1] < (double)inOpen[i - 2] && (double)inClose[i] < (double)inClose[i - 1] ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdl3OutsideUnguarded( int startIdx,
                                        int endIdx,
                                        float inOpen[],
                                        float inHigh[],
                                        float inLow[],
                                        float inClose[],
                                        MInteger outBegIdx,
                                        MInteger outNBElement,
                                        int outInteger[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = cdl3OutsideLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (double)inClose[i - 1] > (double)inOpen[i - 2] && (double)inOpen[i - 1] < (double)inClose[i - 2] && (double)inClose[i] > (double)inClose[i - 1] || (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (double)inOpen[i - 1] > (double)inClose[i - 2] && (double)inClose[i - 1] < (double)inOpen[i - 2] && (double)inClose[i] < (double)inClose[i - 1] ) {
            outInteger[outIdx++] = (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
