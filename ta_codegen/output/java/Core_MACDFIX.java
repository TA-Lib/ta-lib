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

   public int macdFixLookback( int optInSignalPeriod )
   {
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* The lookback is driven by the signal line output.
       *
       * (must also account for the initial data consume
       *  by the fix 26 period EMA).
       */
      return emaLookback(26) + emaLookback(optInSignalPeriod) ;

   }
   public RetCode macdFix( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInSignalPeriod,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outMACD[],
                           double outMACDSignal[],
                           double outMACDHist[] )
   {
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      return macdUnguarded(startIdx, endIdx, inReal, 0, 0, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
      /* 0 indicate fix 12 == 0.15  for optInFastPeriod */
      /* 0 indicate fix 26 == 0.075 for optInSlowPeriod */
   }
   public RetCode macdFixUnguarded( int startIdx,
                                    int endIdx,
                                    double inReal[],
                                    int optInSignalPeriod,
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    double outMACD[],
                                    double outMACDSignal[],
                                    double outMACDHist[] )
   {
      return macdUnguarded(startIdx, endIdx, inReal, 0, 0, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
   }
   public RetCode macdFix( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInSignalPeriod,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outMACD[],
                           double outMACDSignal[],
                           double outMACDHist[] )
   {
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      return macdUnguarded(startIdx, endIdx, inReal, 0, 0, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
   }
   public RetCode macdFixUnguarded( int startIdx,
                                    int endIdx,
                                    float inReal[],
                                    int optInSignalPeriod,
                                    MInteger outBegIdx,
                                    MInteger outNBElement,
                                    double outMACD[],
                                    double outMACDSignal[],
                                    double outMACDHist[] )
   {
      return macdUnguarded(startIdx, endIdx, inReal, 0, 0, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
   }
