/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  071726 MF,CC Implement Negative Volume Index (#126).
 */

   public int nviLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode nvi( int startIdx,
                       int endIdx,
                       double inClose[],
                       double inVolume[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* The index is a running cumulative value seeded at 1000, updated only on
       * bars whose volume decreased versus the prior bar (Negative Volume).
       */
      prevNVI = 1000.0;
      prevClose = inClose[startIdx];
      prevVolume = inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         tempVolume = inVolume[i];
         /* prevClose != 0 guards the percentage-change division: a zero previous
          * close is a degenerate input that would otherwise emit NaN/Inf; carry
          * the index forward unchanged instead. Never triggers on real prices.
          */
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
            prevNVI += (tempClose - prevClose) / prevClose * prevNVI;
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode nviUnguarded( int startIdx,
                                int endIdx,
                                double inClose[],
                                double inVolume[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      prevNVI = 1000.0;
      prevClose = inClose[startIdx];
      prevVolume = inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = inClose[i];
         tempVolume = inVolume[i];
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
            prevNVI += (tempClose - prevClose) / prevClose * prevNVI;
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode nvi( int startIdx,
                       int endIdx,
                       float inClose[],
                       float inVolume[],
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      prevNVI = 1000.0;
      prevClose = (double)inClose[startIdx];
      prevVolume = (double)inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         tempVolume = (double)inVolume[i];
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
            prevNVI += (tempClose - prevClose) / prevClose * prevNVI;
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode nviUnguarded( int startIdx,
                                int endIdx,
                                float inClose[],
                                float inVolume[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      double prevNVI = 0;
      double prevClose = 0;
      double prevVolume = 0;
      double tempClose = 0;
      double tempVolume = 0;
      prevNVI = 1000.0;
      prevClose = (double)inClose[startIdx];
      prevVolume = (double)inVolume[startIdx];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempClose = (double)inClose[i];
         tempVolume = (double)inVolume[i];
         if( tempVolume < prevVolume && prevClose != 0.0 ) {
            prevNVI += (tempClose - prevClose) / prevClose * prevNVI;
         }
         outReal[outIdx++] = prevNVI;
         prevClose = tempClose;
         prevVolume = tempVolume;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
