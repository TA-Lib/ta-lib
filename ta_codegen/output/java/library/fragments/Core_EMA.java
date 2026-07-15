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

   public int emaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 + this.unstablePeriod[FuncUnstId.Ema.ordinal()] ;

   }
   public RetCode emaPrivate( int startIdx,
                              int endIdx,
                              double inReal[],
                              int optInTimePeriod,
                              double optInK_1,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      /* Internal implementation can be called from any other TA function.
       *
       * Faster because there is no parameter check, but it is a double
       * edge sword.
       *
       * The optInK_1 and optInTimePeriod are usually tightly coupled:
       *
       *    optInK_1  = 2 / (optInTimePeriod + 1).
       *
       * These values are going to be related by this equation 99.9% of the
       * time... but there is some exception, this is why both must be provided.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = emaLookback(optInTimePeriod);
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
      outBegIdx.value = startIdx;
      /* Do the EMA calculation using tight loops. */
      /* The first EMA is calculated differently. It
       * then become the seed for subsequent EMA.
       *
       * The algorithm for this seed vary widely.
       * Only 3 are implemented here:
       *
       * TA_MA_CLASSIC:
       *    Use a simple MA of the first 'period'.
       *    This is the approach most widely documented.
       *
       * TA_MA_METASTOCK:
       *    Use first price bar value as a seed
       *    from the begining of all the available
       *    data.
       *
       * TA_MA_TRADESTATION:
       *    Use 4th price bar as a seed, except when
       *    period is 1 who use 2th price bar or something
       *    like that... (not an obvious one...).
       */
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += inReal[today++];
         }
         prevMA = tempReal / optInTimePeriod;
      } else {
         prevMA = inReal[0];
         today = 1;
      }
      while( today <= startIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = (inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode emaPrivate( int startIdx,
                              int endIdx,
                              float inReal[],
                              int optInTimePeriod,
                              double optInK_1,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += (double)inReal[today++];
         }
         prevMA = tempReal / optInTimePeriod;
      } else {
         prevMA = (double)inReal[0];
         today = 1;
      }
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode ema( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double optInK_1 = 0;
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
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Simply call the internal implementation of the EMA. */
      return emaPrivate(startIdx, endIdx, inReal, optInTimePeriod, optInK_1, outBegIdx, outNBElement, outReal) ;
   }
   public RetCode emaUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double optInK_1 = 0;
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      return emaPrivate(startIdx, endIdx, inReal, optInTimePeriod, optInK_1, outBegIdx, outNBElement, outReal) ;
   }
   public RetCode ema( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double optInK_1 = (2.0/(double)((optInTimePeriod+1)));
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
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += (double)inReal[today++];
         }
         prevMA = tempReal / optInTimePeriod;
      } else {
         prevMA = (double)inReal[0];
         today = 1;
      }
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode emaUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double tempReal = 0;
      double prevMA = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double optInK_1 = (2.0/(double)((optInTimePeriod+1)));
      lookbackTotal = emaLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      if( this.compatibility == Compatibility.Default ) {
         today = startIdx - lookbackTotal;
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            tempReal += (double)inReal[today++];
         }
         prevMA = tempReal / optInTimePeriod;
      } else {
         prevMA = (double)inReal[0];
         today = 1;
      }
      while( today <= startIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         prevMA = ((double)inReal[today++] - prevMA) * optInK_1 + prevMA;
         outReal[outIdx++] = prevMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
