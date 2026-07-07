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
 *  102404 AC   Creation
 *  040309 AC   Increased flexibility to allow real bodies matching
 *              on one end (Greg Morris - "Candlestick charting explained")
 */

   public int cdlEngulfingLookback( )
   {
      return 2 ;

   }
   public RetCode cdlEngulfing( int startIdx,
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
      lookbackTotal = cdlEngulfingLookback();
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
       * outInteger is positive (1 to 100) when bullish or negative (-1 to -100) when bearish:
       * - 100 is returned when the second candle's real body begins before and ends after the first candle's real body
       * - 80 is returned when the two real bodies match on one end (Greg Morris contemplate this case in his book
       *   "Candlestick charting explained")
       * The user should consider that an engulfing must appear in a downtrend if bullish or in an uptrend if bearish,
       * while this function does not consider it
       */
      outIdx = 0;
      do {
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (inClose[i] >= inOpen[i - 1] && inOpen[i] < inClose[i - 1] || inClose[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1]) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (inOpen[i] >= inClose[i - 1] && inClose[i] < inOpen[i - 1] || inOpen[i] > inClose[i - 1] && inClose[i] <= inOpen[i - 1]) ) {
            /* white engulfs black */
            /* black engulfs white */
            if( inOpen[i] != inClose[i - 1] && inClose[i] != inOpen[i - 1] ) {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 80;
            }
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
   public RetCode cdlEngulfingUnguarded( int startIdx,
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
      lookbackTotal = cdlEngulfingLookback();
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
         if( ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && (inClose[i] >= inOpen[i - 1] && inOpen[i] < inClose[i - 1] || inClose[i] > inOpen[i - 1] && inOpen[i] <= inClose[i - 1]) || ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && ((inClose[i - 1] >= inOpen[i - 1]) ? 1 : 0 - 1) == 1 && (inOpen[i] >= inClose[i - 1] && inClose[i] < inOpen[i - 1] || inOpen[i] > inClose[i - 1] && inClose[i] <= inOpen[i - 1]) ) {
            if( inOpen[i] != inClose[i - 1] && inClose[i] != inOpen[i - 1] ) {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 80;
            }
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlEngulfing( int startIdx,
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
      lookbackTotal = cdlEngulfingLookback();
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
         if( (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((double)inClose[i] >= (double)inOpen[i - 1] && (double)inOpen[i] < (double)inClose[i - 1] || (double)inClose[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1]) || (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((double)inOpen[i] >= (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] || (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] <= (double)inOpen[i - 1]) ) {
            if( (double)inOpen[i] != (double)inClose[i - 1] && (double)inClose[i] != (double)inOpen[i - 1] ) {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 80;
            }
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlEngulfingUnguarded( int startIdx,
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
      lookbackTotal = cdlEngulfingLookback();
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
         if( (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 0 - 1 && ((double)inClose[i] >= (double)inOpen[i - 1] && (double)inOpen[i] < (double)inClose[i - 1] || (double)inClose[i] > (double)inOpen[i - 1] && (double)inOpen[i] <= (double)inClose[i - 1]) || (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) == 0 - 1 && (((double)inClose[i - 1] >= (double)inOpen[i - 1]) ? 1 : 0 - 1) == 1 && ((double)inOpen[i] >= (double)inClose[i - 1] && (double)inClose[i] < (double)inOpen[i - 1] || (double)inOpen[i] > (double)inClose[i - 1] && (double)inClose[i] <= (double)inOpen[i - 1]) ) {
            if( (double)inOpen[i] != (double)inClose[i - 1] && (double)inClose[i] != (double)inOpen[i - 1] ) {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
            } else {
               outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 80;
            }
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
