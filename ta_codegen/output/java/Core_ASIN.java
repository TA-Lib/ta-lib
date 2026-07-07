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

   public int asinLookback( )
   {
      return 0 ;

   }
   public RetCode asin( int startIdx,
                        int endIdx,
                        double inReal[],
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
         outReal[outIdx] = Math.asin(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode asinUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.asin(inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode asin( int startIdx,
                        int endIdx,
                        float inReal[],
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
         outReal[outIdx] = Math.asin((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode asinUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int i = 0;
      for( i = startIdx, outIdx = 0; i <= endIdx; i += 1, outIdx += 1 ) {
         outReal[outIdx] = Math.asin((double)inReal[i]);
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
