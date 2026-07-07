/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090807 MF     Initial Version
 */

   public int divLookback( )
   {
      return 0 ;

   }
   public RetCode div( int startIdx,
                       int endIdx,
                       double inReal0[],
                       double inReal1[],
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = inReal0[i] / inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode divUnguarded( int startIdx,
                                int endIdx,
                                double inReal0[],
                                double inReal1[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = inReal0[i] / inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode div( int startIdx,
                       int endIdx,
                       float inReal0[],
                       float inReal1[],
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
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = (double)inReal0[i] / (double)inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode divUnguarded( int startIdx,
                                int endIdx,
                                float inReal0[],
                                float inReal1[],
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = (double)inReal0[i] / (double)inReal1[i];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
