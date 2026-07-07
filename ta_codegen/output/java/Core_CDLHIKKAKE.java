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
 *  120305 AC   Creation
 */

   public int cdlHikkakeLookback( )
   {
      return 5 ;

   }
   public RetCode cdlHikkake( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = cdlHikkakeLookback();
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
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         /* copy here the pattern recognition code below */
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 &&
             (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            patternIdx = 0;
         }
         i += 1;
      }
      i = startIdx;
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - first and second candle: inside bar (2nd has lower high and higher low than 1st)
       * - third candle: lower high and lower low than 2nd (higher high and higher low than 2nd)
       * outInteger[hikkakebar] is positive (1 to 100) or negative (-1 to -100) meaning bullish or bearish hikkake
       * Confirmation could come in the next 3 days with:
       * - a day that closes higher than the high (lower than the low) of the 2nd candle
       * outInteger[confirmationbar] is equal to 100 + the bullish hikkake result or -100 - the bearish hikkake result
       * Note: if confirmation and a new hikkake come at the same bar, only the new hikkake is reported (the new hikkake
       * overwrites the confirmation of the old hikkake)
       */
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] &&
             inLow[i - 1] > inLow[i - 2] &&   /* 1st + 2nd: lower high and higher low */
             (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) /* (bull) 3rd: lower high and lower low (bear) 3rd: higher high and higher low */
         {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 &&
             (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) /* search for confirmation if hikkake was no more than 3 bars ago close higher than the high of 2nd close lower than the low of 2nd */
         {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
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
   public RetCode cdlHikkakeUnguarded( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( inHigh[i - 1] < inHigh[i - 2] && inLow[i - 1] > inLow[i - 2] && (inHigh[i] < inHigh[i - 1] && inLow[i] < inLow[i - 1] || inHigh[i] > inHigh[i - 1] && inLow[i] > inLow[i - 1]) ) {
            patternResult = 100 * ((inHigh[i] < inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && inClose[i] > inHigh[patternIdx - 1] || patternResult < 0 && inClose[i] < inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkake( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode cdlHikkakeUnguarded( int startIdx,
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
      int patternIdx = 0;
      int patternResult = 0;
      lookbackTotal = cdlHikkakeLookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      patternIdx = 0;
      patternResult = 0;
      i = startIdx - 3;
      while( i < startIdx ) {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            patternIdx = 0;
         }
         i += 1;
      }
      i = startIdx;
      outIdx = 0;
      do {
         if( (double)inHigh[i - 1] < (double)inHigh[i - 2] && (double)inLow[i - 1] > (double)inLow[i - 2] && ((double)inHigh[i] < (double)inHigh[i - 1] && (double)inLow[i] < (double)inLow[i - 1] || (double)inHigh[i] > (double)inHigh[i - 1] && (double)inLow[i] > (double)inLow[i - 1]) ) {
            patternResult = 100 * (((double)inHigh[i] < (double)inHigh[i - 1]) ? 1 : 0 - 1);
            patternIdx = i;
            outInteger[outIdx++] = patternResult;
         } else if( i <= patternIdx + 3 && (patternResult > 0 && (double)inClose[i] > (double)inHigh[patternIdx - 1] || patternResult < 0 && (double)inClose[i] < (double)inLow[patternIdx - 1]) ) {
            outInteger[outIdx++] = patternResult + 100 * ((patternResult > 0) ? 1 : 0 - 1);
            patternIdx = 0;
         } else {
            outInteger[outIdx++] = 0;
         }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
