/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MW       Michael Williamson
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  122006 MW    Initial Version
 *  071626 MF,CC Fix reversed inReal0/inReal1 roles in the algorithm
 *               description: inReal0 holds the index prices and inReal1
 *               the stock prices (SourceForge bug 98).
 */

   public int betaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   public RetCode beta( int startIdx,
                        int endIdx,
                        double inReal0[],
                        double inReal1[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      n = 0.0;
      /* sum of x * x */
      /* sum of x * y */
      /* sum of x */
      /* sum of y */
      /* the last price read from inReal0 */
      /* the last price read from inReal1 */
      /* same as last_price_x except used to remove elements from the trailing summation */
      /* same as last_price_y except used to remove elements from the trailing summation */
      /* temporary variable */
      /* the 'x' value, which is the last change between values in inReal0 */
      /* the 'y' value, which is the last change between values in inReal1 */
      /* DESCRIPTION OF ALGORITHM:
       *   The Beta 'algorithm' is a measure of a stocks volatility vs from index. The index prices
       *   are given in inReal0 and the stock prices are given in inReal1. The size of these vectors
       *   should be equal. The algorithm is to calculate the change between prices in both vectors
       *   and then 'plot' these changes are points in the Euclidean plane. The x value of the point
       *   is market return and the y value is the security return. The beta value is the slope of a
       *   linear regression through these points. A beta of 1 is simple the line y=x, so the stock
       *   varies percisely with the market. A beta of less than one means the stock varies less than
       *   the market and a beta of more than one means the stock varies more than market. A related
       *   value is the Alpha value (see TA_ALPHA) which is the Y-intercept of the same linear regression.
       */
      /* Validate the calculation method type and
       * identify the minimum number of input
       * consume before the first value is output..
       */
      nbInitialElementNeeded = optInTimePeriod;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Consume first input. */
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      /* Process remaining of lookback until ready to output the first value. */
      i = ++trailingIdx;
      while( i < startIdx ) {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      /* First output always start at index zero */
      n = (double)optInTimePeriod;
      do {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         /* Always read the trailing before writing the output because the input and output
          * buffer can be the same.
          */
         tmp_real = inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x;
         } else {
            x = 0.0;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = inReal1[trailingIdx++];
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y;
         } else {
            y = 0.0;
         }
         trailing_last_price_y = tmp_real;
         /* Write the output */
         tmp_real = n * S_xx - S_x * S_x;
         if( !((-0.00000000000001 < tmp_real) && (tmp_real < 0.00000000000001)) ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / tmp_real;
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Remove the calculation starting with the trailingIdx. */
         S_xx -= x * x;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode betaUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal0[],
                                 double inReal1[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      n = 0.0;
      nbInitialElementNeeded = optInTimePeriod;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      i = ++trailingIdx;
      while( i < startIdx ) {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      n = (double)optInTimePeriod;
      do {
         tmp_real = inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         tmp_real = inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x;
         } else {
            x = 0.0;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = inReal1[trailingIdx++];
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y;
         } else {
            y = 0.0;
         }
         trailing_last_price_y = tmp_real;
         tmp_real = n * S_xx - S_x * S_x;
         if( !((-0.00000000000001 < tmp_real) && (tmp_real < 0.00000000000001)) ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / tmp_real;
         } else {
            outReal[outIdx++] = 0.0;
         }
         S_xx -= x * x;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode beta( int startIdx,
                        int endIdx,
                        float inReal0[],
                        float inReal1[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      n = 0.0;
      nbInitialElementNeeded = optInTimePeriod;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = (double)inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = (double)inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      i = ++trailingIdx;
      while( i < startIdx ) {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      n = (double)optInTimePeriod;
      do {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         tmp_real = (double)inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x;
         } else {
            x = 0.0;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = (double)inReal1[trailingIdx++];
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y;
         } else {
            y = 0.0;
         }
         trailing_last_price_y = tmp_real;
         tmp_real = n * S_xx - S_x * S_x;
         if( !((-0.00000000000001 < tmp_real) && (tmp_real < 0.00000000000001)) ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / tmp_real;
         } else {
            outReal[outIdx++] = 0.0;
         }
         S_xx -= x * x;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode betaUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal0[],
                                 float inReal1[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      double S_xx = 0;
      double S_xy = 0;
      double S_x = 0;
      double S_y = 0;
      double last_price_x = 0;
      double last_price_y = 0;
      double trailing_last_price_x = 0;
      double trailing_last_price_y = 0;
      double tmp_real = 0;
      double x = 0;
      double y = 0;
      double n = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int nbInitialElementNeeded = 0;
      S_xx = 0.0;
      S_xy = 0.0;
      S_x = 0.0;
      S_y = 0.0;
      last_price_x = 0.0;
      last_price_y = 0.0;
      trailing_last_price_x = 0.0;
      trailing_last_price_y = 0.0;
      tmp_real = 0.0;
      n = 0.0;
      nbInitialElementNeeded = optInTimePeriod;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      trailingIdx = startIdx - nbInitialElementNeeded;
      trailing_last_price_x = (double)inReal0[trailingIdx];
      last_price_x = trailing_last_price_x;
      trailing_last_price_y = (double)inReal1[trailingIdx];
      last_price_y = trailing_last_price_y;
      i = ++trailingIdx;
      while( i < startIdx ) {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
      }
      outIdx = 0;
      n = (double)optInTimePeriod;
      do {
         tmp_real = (double)inReal0[i];
         if( !((-0.00000000000001 < last_price_x) && (last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - last_price_x) / last_price_x;
         } else {
            x = 0.0;
         }
         last_price_x = tmp_real;
         tmp_real = (double)inReal1[i++];
         if( !((-0.00000000000001 < last_price_y) && (last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - last_price_y) / last_price_y;
         } else {
            y = 0.0;
         }
         last_price_y = tmp_real;
         S_xx += x * x;
         S_xy += x * y;
         S_x += x;
         S_y += y;
         tmp_real = (double)inReal0[trailingIdx];
         if( !((-0.00000000000001 < trailing_last_price_x) && (trailing_last_price_x < 0.00000000000001)) ) {
            x = (tmp_real - trailing_last_price_x) / trailing_last_price_x;
         } else {
            x = 0.0;
         }
         trailing_last_price_x = tmp_real;
         tmp_real = (double)inReal1[trailingIdx++];
         if( !((-0.00000000000001 < trailing_last_price_y) && (trailing_last_price_y < 0.00000000000001)) ) {
            y = (tmp_real - trailing_last_price_y) / trailing_last_price_y;
         } else {
            y = 0.0;
         }
         trailing_last_price_y = tmp_real;
         tmp_real = n * S_xx - S_x * S_x;
         if( !((-0.00000000000001 < tmp_real) && (tmp_real < 0.00000000000001)) ) {
            outReal[outIdx++] = (n * S_xy - S_x * S_y) / tmp_real;
         } else {
            outReal[outIdx++] = 0.0;
         }
         S_xx -= x * x;
         S_xy -= x * y;
         S_x -= x;
         S_y -= y;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
