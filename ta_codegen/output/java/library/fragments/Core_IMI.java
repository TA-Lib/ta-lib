/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AB       Anatoliy Belsky
 *  MF       Mario Fortier
 *  WZ       wony (github @wony-zheng)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  181012 AB    Initial Version
 *  070526 MF,CC  Fix #98: the unstable period grew the summation window
 *                to period+u bars; window is now always 'period'.
 *  070726 WZ,CC  (#14) IMI has no unstable period; drop the unstable-period
 *                term from the lookback so TA_SetUnstablePeriod is a no-op.
 *  071326 MF,CC  Fix #112: an all-flat window (every close==open) leaves
 *                upsum==downsum==0, so 100*(0/0) emitted NaN from a *successful*
 *                call. Guard the divide, returning IMI's neutral center 50.0.
 */

   public int imiLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode imi( int startIdx,
                       int endIdx,
                       double inOpen[],
                       double inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            /* #112: an all-flat window (every close==open) leaves upsum==downsum==0.
             * Guard the 0/0 so a successful call never emits NaN; IMI is a 0..100
             * oscillator, so no up/down bias returns its neutral center, 50.0.
             */
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imiUnguarded( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = inClose[i];
            double open = inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imi( int startIdx,
                       int endIdx,
                       float inOpen[],
                       float inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = (double)inClose[i];
            double open = (double)inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode imiUnguarded( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inClose[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      int lookback = 0;
      int outIdx = 0;
      outIdx = 0;
      lookback = imiLookback(optInTimePeriod);
      if( startIdx < lookback ) {
         startIdx = lookback;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      while( startIdx <= endIdx ) {
         double upsum = 0.0;
         double downsum = 0.0;
         int i;
         for( i = startIdx - (optInTimePeriod - 1); i <= startIdx; i += 1 ) {
            double close = (double)inClose[i];
            double open = (double)inOpen[i];
            if( close > open ) {
               upsum += close - open;
            } else {
               downsum += open - close;
            }
            outReal[outIdx] = (upsum + downsum == 0.0) ? 50.0 : 100.0 * (upsum / (upsum + downsum));
         }
         startIdx += 1;
         outIdx += 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
