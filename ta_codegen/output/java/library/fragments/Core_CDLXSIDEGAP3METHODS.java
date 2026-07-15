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
 *  011605 AC   Creation
 */

   public int cdlXSideGap3MethodsLookback( )
   {
      return 2 ;

   }
   public RetCode cdlXSideGap3Methods( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
       * - first candle: white (black) candle
       * - second candle: white (black) candle
       * - upside (downside) gap between the first and the second real bodies
       * - third candle: black (white) candle that opens within the second real body and closes within the first real body
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish;
       * the user should consider that up/downside gap 3 methods is significant when it appears in a trend, while this
       * function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && /* 1st and 2nd of same color */
             ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && /* 3rd opposite color */
             inOpen[i] < Math.max(inClose[i - 1], inOpen[i - 1]) &&  /* 3rd opens within 2nd rb */
             inOpen[i] > Math.min(inClose[i - 1], inOpen[i - 1]) &&
             inClose[i] < Math.max(inClose[i - 2], inOpen[i - 2]) && /* 3rd closes within 1st rb */
             inClose[i] > Math.min(inClose[i - 2], inOpen[i - 2]) &&
             (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2]))) ) /* when 1st is white upside gap when 1st is black downside gap */
         {
            outInteger[outIdx++] = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlXSideGap3MethodsUnguarded( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
         if( ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) && inOpen[i] < Math.max(inClose[i - 1], inOpen[i - 1]) && inOpen[i] > Math.min(inClose[i - 1], inOpen[i - 1]) && inClose[i] < Math.max(inClose[i - 2], inOpen[i - 2]) && inClose[i] > Math.min(inClose[i - 2], inOpen[i - 2]) && (((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min(inOpen[i - 1], inClose[i - 1]) > Math.max(inOpen[i - 2], inClose[i - 2])) || ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max(inOpen[i - 1], inClose[i - 1]) < Math.min(inOpen[i - 2], inClose[i - 2]))) ) {
            outInteger[outIdx++] = ((inClose[i - 2] >= inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlXSideGap3Methods( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && (double)inOpen[i] < Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inOpen[i] > Math.min((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inClose[i] < Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && (double)inClose[i] > Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) || (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2]))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlXSideGap3MethodsUnguarded( int startIdx,
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
      lookbackTotal = cdlXSideGap3MethodsLookback();
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
         if( (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) && (double)inOpen[i] < Math.max((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inOpen[i] > Math.min((double)inClose[i - 1], (double)inOpen[i - 1]) && (double)inClose[i] < Math.max((double)inClose[i - 2], (double)inOpen[i - 2]) && (double)inClose[i] > Math.min((double)inClose[i - 2], (double)inOpen[i - 2]) && ((((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 1 && (Math.min((double)inOpen[i - 1], (double)inClose[i - 1]) > Math.max((double)inOpen[i - 2], (double)inClose[i - 2])) || (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) == 0 - 1 && (Math.max((double)inOpen[i - 1], (double)inClose[i - 1]) < Math.min((double)inOpen[i - 2], (double)inClose[i - 2]))) ) {
            outInteger[outIdx++] = (((double)inClose[i - 2] >= (double)inOpen[i - 2]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
