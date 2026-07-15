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
 */

   public int wmaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode wma( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
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
      /* To make the rest more efficient, handle exception
       * case where the user is asking for a period of '1'.
       * In that case outputs equals inputs for the requested
       * range.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      /* Calculate the divider (always an integer value).
       * By induction: 1+2+3+4+'n' = n(n+1)/2
       * '>>1' is usually faster than '/2' for unsigned.
       */
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
      /* The algo used here use a very basic property of
       * multiplication/addition: (x*2) = x+x
       *
       * As an example, a 3 period weighted can be
       * interpreted in two way:
       *  (x1*1)+(x2*2)+(x3*3)
       *      OR
       *  x1+x2+x2+x3+x3+x3 (this is the periodSum)
       *
       * When you move forward in the time serie
       * you can quickly adjust the periodSum for the
       * period by substracting:
       *   x1+x2+x3 (This is the periodSub)
       * Making the new periodSum equals to:
       *   x2+x3+x3
       *
       * You can then add the new price bar
       * which is x4+x4+x4 giving:
       *   x2+x3+x3+x4+x4+x4
       *
       * At this point one iteration is completed and you can
       * see that we are back to the step 1 of this example.
       *
       * Why making it so un-intuitive? The number of memory
       * access and floating point operations are kept to a
       * minimum with this algo.
       */
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      /* Evaluate the initial periodSum/periodSub and trailingValue. */
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      /* Tight loop for the requested range. */
      while( inIdx <= endIdx ) {
         /* Add the current price bar to the sum
          * who are carried through the iterations.
          */
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         /* Save the trailing value for being substract at
          * the next iteration.
          * (must be saved here just in case outReal and
          *  inReal are the same buffer).
          */
         trailingValue = inReal[trailingIdx++];
         /* Calculate the WMA for this price bar. */
         outReal[outIdx++] = periodSum / divider;
         /* Prepare the periodSum for the next iteration. */
         periodSum -= periodSub;
      }
      /* Set output limits. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode wmaUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         tempReal = inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         trailingValue = inReal[trailingIdx++];
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode wma( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         trailingValue = (double)inReal[trailingIdx++];
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode wmaUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int inIdx = 0;
      int outIdx = 0;
      int i = 0;
      int trailingIdx = 0;
      int divider = 0;
      double periodSum = 0;
      double periodSub = 0;
      double tempReal = 0;
      double trailingValue = 0;
      int lookbackTotal = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         System.arraycopy(inReal, startIdx, outReal, 0, (int)outNBElement.value * 1);
         return RetCode.Success ;
      }
      divider = optInTimePeriod * (optInTimePeriod + 1) >> 1;
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      periodSub = (double)0.0;
      periodSum = periodSub;
      inIdx = trailingIdx;
      i = 1;
      while( inIdx < startIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSum += tempReal * i;
         i += 1;
      }
      trailingValue = 0.0;
      while( inIdx <= endIdx ) {
         tempReal = (double)inReal[inIdx++];
         periodSub += tempReal;
         periodSub -= trailingValue;
         periodSum += tempReal * optInTimePeriod;
         trailingValue = (double)inReal[trailingIdx++];
         outReal[outIdx++] = periodSum / divider;
         periodSum -= periodSub;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
