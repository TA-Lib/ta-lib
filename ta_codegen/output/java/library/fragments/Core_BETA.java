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
/**** Streaming API *****/

   /**
    * A live BETA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#beta} over the same series.
    * Open with {@link Core#betaOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class BetaStream {
      final Core core;
      int optInTimePeriod;
      double S_xx;
      double S_xy;
      double S_x;
      double S_y;
      double last_price_x;
      double last_price_y;
      double trailing_last_price_x;
      double trailing_last_price_y;
      double x;
      double y;
      double n;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal0;
      double[] ring_trailingIdx_inReal1;
      double cur_outReal;

      BetaStream( Core core ) { this.core = core; }

      BetaStream( BetaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.S_xx = other.S_xx;
         this.S_xy = other.S_xy;
         this.S_x = other.S_x;
         this.S_y = other.S_y;
         this.last_price_x = other.last_price_x;
         this.last_price_y = other.last_price_y;
         this.trailing_last_price_x = other.trailing_last_price_x;
         this.trailing_last_price_y = other.trailing_last_price_y;
         this.x = other.x;
         this.y = other.y;
         this.n = other.n;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal0 = other.ring_trailingIdx_inReal0.clone();
         this.ring_trailingIdx_inReal1 = other.ring_trailingIdx_inReal1.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal0, double inReal1 ) {
         core.betaStreamStep(this, inReal0, inReal1);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal0, double inReal1 ) {
         BetaStream scratch = new BetaStream(this);
         core.betaStreamStep(scratch, inReal0, inReal1);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public BetaStream copy() {
         return new BetaStream(this);
      }
   }
   void betaStreamStep( BetaStream sp, double inReal0, double inReal1 )
   {
      double tmp_real = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal0[0] = inReal0;
         sp.ring_trailingIdx_inReal1[0] = inReal1;
      }
      tmp_real = inReal0;
      if( !((-0.00000000000001 < sp.last_price_x) && (sp.last_price_x < 0.00000000000001)) ) {
         sp.x = (tmp_real - sp.last_price_x) / sp.last_price_x;
      } else {
         sp.x = 0.0;
      }
      sp.last_price_x = tmp_real;
      tmp_real = inReal1;
      if( !((-0.00000000000001 < sp.last_price_y) && (sp.last_price_y < 0.00000000000001)) ) {
         sp.y = (tmp_real - sp.last_price_y) / sp.last_price_y;
      } else {
         sp.y = 0.0;
      }
      sp.last_price_y = tmp_real;
      sp.S_xx += sp.x * sp.x;
      sp.S_xy += sp.x * sp.y;
      sp.S_x += sp.x;
      sp.S_y += sp.y;
      /* Always read the trailing before writing the output because the input and output
       * buffer can be the same.
       */
      tmp_real = sp.ring_trailingIdx_inReal0[sp.ringPos_trailingIdx];
      if( !((-0.00000000000001 < sp.trailing_last_price_x) && (sp.trailing_last_price_x < 0.00000000000001)) ) {
         sp.x = (tmp_real - sp.trailing_last_price_x) / sp.trailing_last_price_x;
      } else {
         sp.x = 0.0;
      }
      sp.trailing_last_price_x = tmp_real;
      tmp_real = sp.ring_trailingIdx_inReal1[sp.ringPos_trailingIdx];
      if( !((-0.00000000000001 < sp.trailing_last_price_y) && (sp.trailing_last_price_y < 0.00000000000001)) ) {
         sp.y = (tmp_real - sp.trailing_last_price_y) / sp.trailing_last_price_y;
      } else {
         sp.y = 0.0;
      }
      sp.trailing_last_price_y = tmp_real;
      /* Write the output */
      tmp_real = sp.n * sp.S_xx - sp.S_x * sp.S_x;
      if( !((-0.00000000000001 < tmp_real) && (tmp_real < 0.00000000000001)) ) {
         sp.cur_outReal = (sp.n * sp.S_xy - sp.S_x * sp.S_y) / tmp_real;
      } else {
         sp.cur_outReal = 0.0;
      }
      /* Remove the calculation starting with the trailingIdx. */
      sp.S_xx -= sp.x * sp.x;
      sp.S_xy -= sp.x * sp.y;
      sp.S_x -= sp.x;
      sp.S_y -= sp.y;
      sp.ring_trailingIdx_inReal0[sp.ringPos_trailingIdx] = inReal0;
      sp.ring_trailingIdx_inReal1[sp.ringPos_trailingIdx] = inReal1;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode betaOpenBody( BetaStream sp, double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
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
            lastValue_outReal = (n * S_xy - S_x * S_y) / tmp_real;
         } else {
            lastValue_outReal = 0.0;
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
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal0 = new double[allocN_trailingIdx];
      System.arraycopy(inReal0, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal0, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inReal1 = new double[allocN_trailingIdx];
      System.arraycopy(inReal1, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal1, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.S_xx = S_xx;
      sp.S_xy = S_xy;
      sp.S_x = S_x;
      sp.S_y = S_y;
      sp.last_price_x = last_price_x;
      sp.last_price_y = last_price_y;
      sp.trailing_last_price_x = trailing_last_price_x;
      sp.trailing_last_price_y = trailing_last_price_y;
      sp.x = x;
      sp.y = y;
      sp.n = n;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal0 = capRing_trailingIdx_inReal0;
      sp.ring_trailingIdx_inReal1 = capRing_trailingIdx_inReal1;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode betaOpenAndFillBody( BetaStream sp, double inReal0[], double inReal1[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal0 = new double[allocN_trailingIdx];
      System.arraycopy(inReal0, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal0, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inReal1 = new double[allocN_trailingIdx];
      System.arraycopy(inReal1, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal1, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.S_xx = S_xx;
      sp.S_xy = S_xy;
      sp.S_x = S_x;
      sp.S_y = S_y;
      sp.last_price_x = last_price_x;
      sp.last_price_y = last_price_y;
      sp.trailing_last_price_x = trailing_last_price_x;
      sp.trailing_last_price_y = trailing_last_price_y;
      sp.x = x;
      sp.y = y;
      sp.n = n;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal0 = capRing_trailingIdx_inReal0;
      sp.ring_trailingIdx_inReal1 = capRing_trailingIdx_inReal1;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind betaOpen (composition seam). */
   BetaStream betaOpenInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      BetaStream sp = new BetaStream(this);
      RetCode retCode = betaOpenBody(sp, inReal0, inReal1, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BETA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BETA open: internal error");
      }
      throw new IllegalArgumentException("TA_BETA open: " + retCode);
   }
   /**
    * Open a live BETA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#beta} at that bar.
    * <p>The history must hold at least {@code betaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public BetaStream betaOpen( double inReal0[], double inReal1[], int optInTimePeriod )
   {
      return betaOpenInternal(inReal0, inReal1, 0, optInTimePeriod);
   }
   /**
    * {@link Core#betaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#beta} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public BetaStream betaOpenAndFill( double inReal0[], double inReal1[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      BetaStream sp = new BetaStream(this);
      RetCode retCode = betaOpenAndFillBody(sp, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BETA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BETA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_BETA openAndFill: " + retCode);
   }
