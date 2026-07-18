/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CR       Chris (crokusek@hotmail.com)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010503 MF     Initial Coding
 *  031703 MF     Fix #701060. Correct logic when using a range with
 *                startIdx/endIdx. Thanks to Chris for reporting this.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071226 MF,CC  Widen the triangular-weight factor to double: (i+1)*(i+1)
 *                and i*(i+1) overflowed a 32-bit int at extreme periods
 *                (past ~92682), silently returning garbage. Bit-identical
 *                for every period where the int product fits.
 */

   public int trimaLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode trima( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
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
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
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
      /* TRIMA Description
       * =================
       * The triangular MA is a weighted moving average. Instead of the
       * TA_WMA who put more weigth on the latest price bar, the triangular
       * put more weigth on the data in the middle of the specified period.
       *
       * Examples:
       *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
       *
       *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
       *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
       *
       *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
       *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
       *
       * Generally Accepted Implementation
       * ==================================
       * Using algebra, it can be demonstrated that the TRIMA is equivalent to
       * doing a SMA of a SMA. The following explain the rules:
       *
       *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
       *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
       *
       * In other word:
       *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
       *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
       *
       * The SMA of a SMA is the algorithm generaly found in books.
       *
       * Tradestation Implementation
       * ===========================
       * Tradestation deviate from the generally accepted implementation by
       * making the TRIMA to be as follow:
       *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
       * This formula is done regardless if the period is even or odd.
       *
       * In other word:
       *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
       *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
       *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
       *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
       *
       * It is not clear to me if the Tradestation approach is a bug or a deliberate
       * decision to do things differently.
       *
       * Metastock Implementation
       * ========================
       * Output is the same as the generally accepted implementation.
       *
       * TA-Lib Implementation
       * =====================
       * Output is also the same as the generally accepted implementation.
       *
       * For speed optimization and avoid memory allocation, TA-Lib use
       * a better algorithm than the usual SMA of a SMA.
       *
       * The calculation from one TRIMA value to the next is done by doing 4
       * little adjustment (the following show a TRIMA 4-period):
       *
       * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
       * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
       *
       * To go from TRIMA 'd' to 'e', the following is done:
       *       1) 'a' and 'b' are substract from the numerator.
       *       2) 'd' is added to the numerator.
       *       3) 'e' is added to the numerator.
       *       4) Calculate TRIMA by doing numerator / 6
       *       5) Repeat sequence for next output
       *
       * These operations are the same steps done by TA-LIB:
       *       1) is done by numeratorSub
       *       2) is done by numeratorAdd.
       *       3) is obtain from the latest input
       *       4) Calculate and write TRIMA in the output
       *       5) Repeat for next output.
       *
       * Of course, numerotrAdd and numeratorSub needs to be
       * adjusted for each iteration.
       *
       * The update of numeratorSub needs values from the input at
       * the trailingIdx and middleIdx position.
       *
       * The update of numeratorAdd needs values from the input at
       * the middleIdx and todayIdx.
       */
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         /* Logic for Odd period */
         /* Calculate the factor which is 1 divided by the
          * sumation of the weight.
          *
          * The sum of the weight is calculated as follow:
          *
          * The simple sumation serie 1+2+3... n can be
          * express as n(n+1)/2
          *
          * From this logic, a "triangular" sumation formula
          * can be found depending if the period is odd or even.
          *
          * Odd Period Formula:
          *  period = 5 and with n=(int)(period/2)
          *  the formula for a "triangular" serie is:
          *    1+2+3+2+1 = (n*(n+1))+n+1
          *              = (n+1)*(n+1)
          *              = 3 * 3 = 9
          *
          * Even period Formula:
          *   period = 6 and with n=(int)(period/2)
          *   the formula for a "triangular" serie is:
          *    1+2+3+3+2+1 = n*(n+1)
          *                = 3 * 4 = 12
          */
         /* Note: the (i+1) factors are widened to double so the product
          *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
          *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
          *       past period ~92682). For every period where the int product
          *       fits, the widened value is identical.
          */
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         /* Even logic.
          *
          * Very similar to the odd logic, except:
          *  - calculation of the factor is different.
          *  - the coverage of the numeratorSub and numeratorAdd is
          *    slightly different.
          *  - Adjustment of numeratorAdd is different. See Step (2).
          */
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         /* widen: i*(i+1) overflows int past period ~92682 */
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode trimaUnguarded( int startIdx,
                                  int endIdx,
                                  double inReal[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode trima( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
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
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   public RetCode trimaUnguarded( int startIdx,
                                  int endIdx,
                                  float inReal[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live TRIMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#trima} over the same series.
    * Open with {@link Core#trimaOpen}; there is no close — the handle is
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
   public static final class TrimaStream {
      final Core core;
      int optInTimePeriod;
      double numerator;
      double numeratorSub;
      double numeratorAdd;
      double factor;
      double tempReal;
      int ringPos_middleIdx;
      int ringCap_middleIdx;
      double[] ring_middleIdx_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      TrimaStream( Core core ) { this.core = core; }

      TrimaStream( TrimaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.numerator = other.numerator;
         this.numeratorSub = other.numeratorSub;
         this.numeratorAdd = other.numeratorAdd;
         this.factor = other.factor;
         this.tempReal = other.tempReal;
         this.ringPos_middleIdx = other.ringPos_middleIdx;
         this.ringCap_middleIdx = other.ringCap_middleIdx;
         this.ring_middleIdx_inReal = other.ring_middleIdx_inReal.clone();
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.trimaStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         TrimaStream scratch = new TrimaStream(this);
         core.trimaStreamStep(scratch, inReal);
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
      public TrimaStream copy() {
         return new TrimaStream(this);
      }
   }
   void trimaStreamStep( TrimaStream sp, double inReal )
   {
      if( sp.optInTimePeriod % 2 == 1 ) {
         if( sp.ringCap_middleIdx == 0 ) {
            sp.ring_middleIdx_inReal[0] = inReal;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            sp.ring_trailingIdx_inReal[0] = inReal;
         }
         /* Step (1) */
         sp.numerator -= sp.numeratorSub;
         sp.numeratorSub -= sp.tempReal;
         sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
         sp.numeratorSub += sp.tempReal;
         /* Step (2) */
         sp.numerator += sp.numeratorAdd;
         sp.numeratorAdd -= sp.tempReal;
         sp.tempReal = inReal;
         sp.numeratorAdd += sp.tempReal;
         /* Step (3) */
         sp.numerator += sp.tempReal;
         /* Step (4) */
         sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
         sp.cur_outReal = sp.numerator * sp.factor;
         sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
         sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
         if( sp.ringPos_middleIdx >= sp.ringCap_middleIdx ) {
            sp.ringPos_middleIdx = 0;
         }
         sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
         sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
         if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
            sp.ringPos_trailingIdx = 0;
         }
      } else {
         if( sp.ringCap_middleIdx == 0 ) {
            sp.ring_middleIdx_inReal[0] = inReal;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            sp.ring_trailingIdx_inReal[0] = inReal;
         }
         /* Step (1) */
         sp.numerator -= sp.numeratorSub;
         sp.numeratorSub -= sp.tempReal;
         sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
         sp.numeratorSub += sp.tempReal;
         /* Step (2) */
         sp.numeratorAdd -= sp.tempReal;
         sp.numerator += sp.numeratorAdd;
         sp.tempReal = inReal;
         sp.numeratorAdd += sp.tempReal;
         /* Step (3) */
         sp.numerator += sp.tempReal;
         /* Step (4) */
         sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
         sp.cur_outReal = sp.numerator * sp.factor;
         sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
         sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
         if( sp.ringPos_middleIdx >= sp.ringCap_middleIdx ) {
            sp.ringPos_middleIdx = 0;
         }
         sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
         sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
         if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
            sp.ringPos_trailingIdx = 0;
         }
      }
   }
   private RetCode trimaOpenBody( TrimaStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod % 2 == 1 ) {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
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
            return RetCode.OutOfRangeEndIndex ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Logic for Odd period */
         /* Calculate the factor which is 1 divided by the
          * sumation of the weight.
          *
          * The sum of the weight is calculated as follow:
          *
          * The simple sumation serie 1+2+3... n can be
          * express as n(n+1)/2
          *
          * From this logic, a "triangular" sumation formula
          * can be found depending if the period is odd or even.
          *
          * Odd Period Formula:
          *  period = 5 and with n=(int)(period/2)
          *  the formula for a "triangular" serie is:
          *    1+2+3+2+1 = (n*(n+1))+n+1
          *              = (n+1)*(n+1)
          *              = 3 * 3 = 9
          *
          * Even period Formula:
          *   period = 6 and with n=(int)(period/2)
          *   the formula for a "triangular" serie is:
          *    1+2+3+3+2+1 = n*(n+1)
          *                = 3 * 4 = 12
          */
         /* Note: the (i+1) factors are widened to double so the product
          *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
          *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
          *       past period ~92682). For every period where the int product
          *       fits, the widened value is identical.
          */
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         lastValue_outReal = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            lastValue_outReal = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      } else {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
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
            return RetCode.OutOfRangeEndIndex ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Even logic.
          *
          * Very similar to the odd logic, except:
          *  - calculation of the factor is different.
          *  - the coverage of the numeratorSub and numeratorAdd is
          *    slightly different.
          *  - Adjustment of numeratorAdd is different. See Step (2).
          */
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         /* widen: i*(i+1) overflows int past period ~92682 */
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         lastValue_outReal = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            lastValue_outReal = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = lastValue_outReal;
         return RetCode.Success;
      }
   }
   private RetCode trimaOpenAndFillBody( TrimaStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod % 2 == 1 ) {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
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
            return RetCode.OutOfRangeEndIndex ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Logic for Odd period */
         /* Calculate the factor which is 1 divided by the
          * sumation of the weight.
          *
          * The sum of the weight is calculated as follow:
          *
          * The simple sumation serie 1+2+3... n can be
          * express as n(n+1)/2
          *
          * From this logic, a "triangular" sumation formula
          * can be found depending if the period is odd or even.
          *
          * Odd Period Formula:
          *  period = 5 and with n=(int)(period/2)
          *  the formula for a "triangular" serie is:
          *    1+2+3+2+1 = (n*(n+1))+n+1
          *              = (n+1)*(n+1)
          *              = 3 * 3 = 9
          *
          * Even period Formula:
          *   period = 6 and with n=(int)(period/2)
          *   the formula for a "triangular" serie is:
          *    1+2+3+3+2+1 = n*(n+1)
          *                = 3 * 4 = 12
          */
         /* Note: the (i+1) factors are widened to double so the product
          *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
          *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
          *       past period ~92682). For every period where the int product
          *       fits, the widened value is identical.
          */
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      } else {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
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
            return RetCode.OutOfRangeEndIndex ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Even logic.
          *
          * Very similar to the odd logic, except:
          *  - calculation of the factor is different.
          *  - the coverage of the numeratorSub and numeratorAdd is
          *    slightly different.
          *  - Adjustment of numeratorAdd is different. See Step (2).
          */
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         /* widen: i*(i+1) overflows int past period ~92682 */
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = outReal[outNBElement.value - 1];
         return RetCode.Success;
      }
   }
   /* Internal startIdx-anchored open behind trimaOpen (composition seam). */
   TrimaStream trimaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      TrimaStream sp = new TrimaStream(this);
      RetCode retCode = trimaOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TRIMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TRIMA open: internal error");
      }
      throw new IllegalArgumentException("TA_TRIMA open: " + retCode);
   }
   /**
    * Open a live TRIMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#trima} at that bar.
    * <p>The history must hold at least {@code trimaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public TrimaStream trimaOpen( double inReal[], int optInTimePeriod )
   {
      return trimaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#trimaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#trima} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public TrimaStream trimaOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TrimaStream sp = new TrimaStream(this);
      RetCode retCode = trimaOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_TRIMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_TRIMA openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_TRIMA openAndFill: " + retCode);
   }
