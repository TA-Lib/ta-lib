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

   public int multLookback( )
   {
      return 0 ;

   }
   public RetCode mult( int startIdx,
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
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode multUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal0[],
                                 double inReal1[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = inReal0[i] * inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode mult( int startIdx,
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
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = (double)inReal0[i] * (double)inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode multUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal0[],
                                 float inReal1[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         outReal[outIdx] = (double)inReal0[i] * (double)inReal1[i];
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
