/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  090404 MF   Fix #978056. Trap sqrt with negative zero values.
 */

   public int stdDevLookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Lookback is driven by the variance. */
      return varianceLookback(optInTimePeriod, optInNbDev) ;

   }
   public RetCode stdDev( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          double optInNbDev,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      /* Calculate the variance. */
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      /* Calculate the square root of each variance, this
       * is the standard deviation.
       *
       * Multiply also by the ratio specified.
       */
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
   public RetCode stdDevUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal[],
                                   int optInTimePeriod,
                                   double optInNbDev,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
   public RetCode stdDev( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          double optInNbDev,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDev == -4e37 ) {
         optInNbDev = 1e0;
      }
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
   public RetCode stdDevUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal[],
                                   int optInTimePeriod,
                                   double optInNbDev,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      int i = 0;
      RetCode retCode;
      double tempReal = 0;
      retCode = varianceUnguarded(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         return retCode ;
      }
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal) * optInNbDev;
            } else {
               outReal[i] = (double)0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !(tempReal < 0.00000000000001) ) {
               outReal[i] = Math.sqrt(tempReal);
            } else {
               outReal[i] = (double)0.0;
            }
         }
      }
      return RetCode.Success ;
   }
