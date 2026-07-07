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
 *  112605 MF   Fix outBegIdx when startIdx != 0
 */

   public int medPriceLookback( )
   {
      /* This function have no lookback needed. */
      return 0 ;

   }
   public RetCode medPrice( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* MEDPRICE = (High + Low ) / 2
       * This is the high and low of the same price bar.
       *
       * See MIDPRICE to use instead the highest high and lowest
       * low over multiple price bar.
       */
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPriceUnguarded( int startIdx,
                                     int endIdx,
                                     double inHigh[],
                                     double inLow[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = (inHigh[i] + inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPrice( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode medPriceUnguarded( int startIdx,
                                     int endIdx,
                                     float inHigh[],
                                     float inLow[],
                                     MInteger outBegIdx,
                                     MInteger outNBElement,
                                     double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         outReal[outIdx++] = ((double)inHigh[i] + (double)inLow[i]) / 2.0;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
